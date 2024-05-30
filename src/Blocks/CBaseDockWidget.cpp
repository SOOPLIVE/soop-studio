#include "CBaseDockWidget.h"
#include "QMouseEvent"
#include "QHoverEvent"
#include "QPainter"

#include <QStyle>

AFQBaseDockWidget::AFQBaseDockWidget(QWidget* parent) :
	QDockWidget(parent)
{
	this->setMouseTracking(true);
	this->setAttribute(Qt::WA_Hover);
	this->installEventFilter(this);
	
	connect(this, &QDockWidget::topLevelChanged, this, &AFQBaseDockWidget::qslotTopLevelChanged);
}

void AFQBaseDockWidget::qslotTopLevelChanged(bool toplevel)
{
	m_bIsFloating = toplevel;
	SetStyle();
}

void AFQBaseDockWidget::closeEvent(QCloseEvent* event)
{
	emit closed();
}

bool AFQBaseDockWidget::event(QEvent* e)
{
	switch (e->type()) {
	case QEvent::MouseButtonPress: {
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
		if (mouseEvent->button() == Qt::LeftButton) {
			if (_IsOverTitleArea(mouseEvent->position().toPoint()))
			{
				m_bIsPressed = true;
				SetStyle();
			}
		}
		break;
	}
	case QEvent::MouseButtonRelease: {
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
		if (mouseEvent->button() == Qt::LeftButton) {
			if (_IsOverTitleArea(mouseEvent->position().toPoint()))
			{
				m_bIsPressed = false;
				SetStyle();
			}
		}
		break;
	}
	default:
		break;
	}
	return QDockWidget::event(e);
}

void AFQBaseDockWidget::SetStyle()
{
	if (!titleBarWidget() || !widget())
		return;

	if (m_bIsFloating && m_bIsPressed) {
		titleBarWidget()->setStyleSheet("#AFDockTitle { border-top-left-radius: 0px;		\
														border-top-right-radius: 0px;		\
														border-left:1px solid #00E0FF;		\
														border-right:1px solid #00E0FF;		\
														border-top:1px solid #00E0FF;}");
		widget()->setStyleSheet("#widget_DockContents { border-top-right-radius: 0px;		\
														border-bottom-right-radius: 0px;	\
														border-top-left-radius: 0px;		\
														border-bottom-left-radius: 0px;		\
														border-left:1px solid #00E0FF;		\
														border-right:1px solid #00E0FF;		\
														border-bottom:1px solid #00E0FF;}	\
								  #frame_DockContents { border-top-right-radius: 0px;		\
														border-bottom-right-radius: 0px;	\
														border-top-left-radius: 0px;		\
														border-bottom-left-radius: 0px;		\
														border-left:1px solid #00E0FF;		\
														border-right:1px solid #00E0FF;		\
														border-bottom:1px solid #00E0FF;}");
	}
	else if (!m_bIsFloating && m_bIsPressed) {
		titleBarWidget()->setStyleSheet("#AFDockTitle { border-top-left-radius: 10px;		\
														border-top-right-radius: 10px;		\
														border-left:1px solid #00E0FF;		\
														border-right:1px solid #00E0FF;		\
														border-top:1px solid #00E0FF;}");
		widget()->setStyleSheet("#widget_DockContents { border-top-right-radius: 0px;		\
														border-bottom-right-radius: 10px;	\
														border-top-left-radius: 0px;		\
														border-bottom-left-radius: 10px;	\
														border-left:1px solid #00E0FF;		\
														border-right:1px solid #00E0FF;		\
														border-bottom:1px solid #00E0FF;}	\
								  #frame_DockContents { border-top-right-radius: 0px;		\
														border-bottom-right-radius: 10px;	\
														border-top-left-radius: 0px;		\
														border-bottom-left-radius: 10px;	\
														border-left:1px solid #00E0FF;		\
														border-right:1px solid #00E0FF;		\
														border-bottom:1px solid #00E0FF;}");
	}
	else if (m_bIsFloating && !m_bIsPressed) {
		titleBarWidget()->setStyleSheet("#AFDockTitle { border-top-left-radius: 0px;		\
														border-top-right-radius: 0px;		\
														border-left:1px solid #111;		\
														border-right:1px solid #111;		\
														border-top:1px solid #111;}");
		widget()->setStyleSheet("#widget_DockContents { border-top-right-radius: 0px;		\
														border-bottom-right-radius: 0px;	\
														border-top-left-radius: 0px;		\
														border-bottom-left-radius: 0px;	\
														border-left:1px solid #111;		\
														border-right:1px solid #111;		\
														border-bottom:1px solid #111;}		\
								  #frame_DockContents { border-top-right-radius: 0px;		\
													    border-bottom-right-radius: 0px;	\
													    border-top-left-radius: 0px;		\
													    border-bottom-left-radius: 0px;		\
														border-left:1px solid #111;		\
														border-right:1px solid #111;		\
														border-bottom:1px solid #111;}");

	}
	else { // !m_bIsFloating && !m_bIsPressed
		titleBarWidget()->setStyleSheet("#AFDockTitle { border-top-left-radius: 10px;		\
														border-top-right-radius: 10px;		\
														border-left:1px solid #111;		\
														border-right:1px solid #111;		\
														border-top:1px solid #111;}");
		widget()->setStyleSheet("#widget_DockContents { border-top-right-radius: 0px;		\
														border-bottom-right-radius: 10px;	\
														border-top-left-radius: 0px;		\
														border-bottom-left-radius: 10px;	\
														border-left:1px solid #111;		\
														border-right:1px solid #111;		\
														border-bottom:1px solid #111;}		\
								  #frame_DockContents { border-top-right-radius: 0px;		\
													    border-bottom-right-radius: 10px;	\
													    border-top-left-radius: 0px;		\
													    border-bottom-left-radius: 10px;	\
														border-left:1px solid #111;		\
														border-right:1px solid #111;		\
														border-bottom:1px solid #111;}");
	}

	style()->polish(this);
}

bool AFQBaseDockWidget::_IsOverTitleArea(QPoint point)
{
	if (!titleBarWidget())
		return false;

	QRect titleArea = titleBarWidget()->rect();
	if (titleArea.contains(point))
		return true;
	return false;
}