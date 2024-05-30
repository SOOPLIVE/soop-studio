#include "CListViewScrollAreaContent.h"

#include <QMimeData>
#include <QEvent>
#include <QPainter>
#include <QDropEvent>
#include <QScrollBar>

#include "CSceneSourceDockWidget.h"
#include "CListViewScrollArea.h"


#define AF_SCENE_LIST_DRAG_MARGIN	(15)
#define AF_SCENE_ITEM_HEIGHT        (44)

AFQListScrollAreaContent::AFQListScrollAreaContent(QWidget* parent) :
	QWidget(parent)
{
	setAcceptDrops(true);

	setContentsMargins(0, 0, 0, 0);

	m_timerScroll = new QTimer(this);
	m_timerScroll->setInterval(40);
	connect(m_timerScroll, &QTimer::timeout,
			this, &AFQListScrollAreaContent::qSlotUpdateScrollTimer);
}

AFQListScrollAreaContent::~AFQListScrollAreaContent()
{

}

void AFQListScrollAreaContent::qSlotUpdateScrollTimer()
{
	if (!m_scrollArea)
		return;

	int value = m_scrollArea->verticalScrollBar()->value();

	m_scrollArea->verticalScrollBar()->setValue(value + m_adjustScrollPosY);

	AFQListViewScrollArea* listScrollArea = qobject_cast<AFQListViewScrollArea*>(m_scrollArea);
	listScrollArea->ShowScrollBar();
}

void AFQListScrollAreaContent::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	QStyleOption option;
	option.initFrom(this);

	style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);

	if(m_isDrag)
		painter.setPen(QPen(QColor(255, 255, 255), 1));
	else
		painter.setPen(QPen(QColor(36, 39, 45), 1));

	QLine line;
	line.setP1(m_posLineStart);
	line.setP2(m_posLineEnd);
	painter.drawLine(line);

	QWidget::paintEvent(event);
}

void AFQListScrollAreaContent::dragEnterEvent(QDragEnterEvent* event)
{
	if (event->mimeData()->hasFormat(SCENE_ITEM_DRAG_MIME)) {
		event->accept();
	}
	else {
		event->ignore();
	}
}

void AFQListScrollAreaContent::dragMoveEvent(QDragMoveEvent* event)
{
	if (event->mimeData()->hasFormat(SCENE_ITEM_DRAG_MIME)) {
		event->accept();

		m_isDrag = true;

		QByteArray encodedData = event->mimeData()->data(SCENE_ITEM_DRAG_MIME);
		QString dataString = QString::fromStdString(encodedData.toStdString());

		QPoint pos = event->position().toPoint();

		QStringList dataList = dataString.split('|');
		if (dataList.size() >= 3) {
			int startItemIndex = dataList.at(0).toInt();
			int startPosX	   = dataList.at(1).toInt();
			int startPosY	   = dataList.at(2).toInt();

			int itemIndex = _GetSceneIndex(pos.y());
			DragDirection direction = _GetDirectionDrag(startItemIndex, itemIndex, m_totalSceneCount);

			int indicatorY = _FindIndicatorPosition(direction, itemIndex);

			_SetIndicatorLine(indicatorY);

			int yPos = mapToParent(pos).y();
			int height = visibleRegion().boundingRect().height();
			if (yPos < AF_SCENE_LIST_DRAG_MARGIN) {
				m_adjustScrollPosY = -(height /10);
				m_timerScroll->start();
			}
			else if(yPos > height - AF_SCENE_LIST_DRAG_MARGIN){
				m_adjustScrollPosY = (height /10);
				m_timerScroll->start();
			}
			else {
				m_adjustScrollPosY = 0;
				if(m_timerScroll->isActive())
					m_timerScroll->stop();
			}

			this->update();
		}
	}
	else {
		event->ignore();
		m_isDrag = false;
	}
}

void AFQListScrollAreaContent::dropEvent(QDropEvent* event)
{
	if (event->mimeData()->hasFormat(SCENE_ITEM_DRAG_MIME)) {
		event->setDropAction(Qt::MoveAction);
		event->accept();

		QByteArray encodedData = event->mimeData()->data(SCENE_ITEM_DRAG_MIME);
		QString dataString = QString::fromStdString(encodedData.toStdString());

		QPoint pos = event->position().toPoint();

		QStringList dataList = dataString.split('|');
		if (dataList.size() >= 3) {
			int startItemIndex = dataList.at(0).toInt();
			int startPosX = dataList.at(1).toInt();
			int startPosY = dataList.at(2).toInt();

			int destIndex = _GetSceneIndex(pos.y());
			emit qsignalSwapItem(startItemIndex, destIndex);

			if (m_timerScroll->isActive())
				m_timerScroll->stop();

			this->update();
		}

		m_isDrag = false;
	}
	else {
		event->ignore();
	}
}

void AFQListScrollAreaContent::dragLeaveEvent(QDragLeaveEvent* event)
{
	m_isDrag = false;
	
	if (m_timerScroll->isActive())
		m_timerScroll->stop();

	this->update();

	QWidget::dragLeaveEvent(event);
}

void AFQListScrollAreaContent::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
}

void AFQListScrollAreaContent::SetTotalSceneCount(int count)
{
	m_totalSceneCount = count;
}

void AFQListScrollAreaContent::SetScrollAreaPtr(QScrollArea* scrollArea)
{
	m_scrollArea = scrollArea;
}

int AFQListScrollAreaContent::_GetSceneIndex(int posY)
{
	const int spacing = 4;
	const int totalItemHeight = AF_SCENE_ITEM_HEIGHT + spacing;

	return ((posY - spacing) / totalItemHeight);
}

DragDirection AFQListScrollAreaContent::_GetDirectionDrag(int startItemIndex, int itemIndex, int totalIndex)
{
	if (itemIndex >= totalIndex)
		return DragDirection::None;

	if (itemIndex == startItemIndex)
		return DragDirection::None;

	if (itemIndex == 0 && startItemIndex == 0)
		return DragDirection::None;

	if ((itemIndex == totalIndex - 1) && (startItemIndex == totalIndex - 1))
		return DragDirection::None;

	if (startItemIndex > itemIndex)
		return DragDirection::Top;
	else
		return DragDirection::Bottom;
}

void AFQListScrollAreaContent::_SetIndicatorLine(int indicatorY)
{
	m_posLineStart = QPoint(7, indicatorY);
	m_posLineEnd = QPoint(195, indicatorY);
}

int	AFQListScrollAreaContent::_FindIndicatorPosition(DragDirection direction, int itemIndex)
{
	const int spacing = 4;
	const int totalItemHeight = spacing + AF_SCENE_ITEM_HEIGHT;

	if (DragDirection::Bottom == direction)
		return itemIndex * totalItemHeight + AF_SCENE_ITEM_HEIGHT + spacing;
	else if (DragDirection::Top == direction)
		return itemIndex * totalItemHeight + spacing/2;
	else
		return -10;
}