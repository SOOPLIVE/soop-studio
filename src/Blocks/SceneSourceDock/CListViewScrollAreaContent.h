#pragma once

#include <QWidget>
#include <QTimer>
#include <QScrollArea>

enum class DragDirection { Top, Bottom, None };

class AFQListScrollAreaContent : public QWidget
{
	Q_OBJECT;

public:
	AFQListScrollAreaContent(QWidget* parent = nullptr);
	~AFQListScrollAreaContent();

signals:
	void qsignalSwapItem(int from, int dest);

private slots:
	void qSlotUpdateScrollTimer();

protected:
	void paintEvent(QPaintEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

public:
	void		  SetTotalSceneCount(int count);
	void		  SetScrollAreaPtr(QScrollArea* scrollArea);

private:
	int			  _GetSceneIndex(int posY);
	DragDirection _GetDirectionDrag(int startItemIndex, int itemIndex, int totalIndex);
	void		  _SetIndicatorLine(int indicatorY);
	int			  _FindIndicatorPosition(DragDirection direction, int itemIndex);

private:
	bool	m_isDrag = false;
	int		m_totalSceneCount = 0;
	int		m_adjustScrollPosY = 0;
	QPoint	m_posLineStart;
	QPoint	m_posLineEnd;

	QScrollArea* m_scrollArea = nullptr;
	QTimer* m_timerScroll = nullptr;

};