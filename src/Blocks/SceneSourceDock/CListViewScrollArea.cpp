#include "CListViewScrollArea.h"

#include <QScrollBar>

AFQListViewScrollArea::AFQListViewScrollArea(QWidget* parent)
	: QScrollArea(parent)
{
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	m_timerScrollVisible = new QTimer(this);
	m_timerScrollVisible->setInterval(200);
	m_timerScrollVisible->setSingleShot(true);
	connect(m_timerScrollVisible, &QTimer::timeout,
			this, &AFQListViewScrollArea::qSlotHideScrollBar);
}

AFQListViewScrollArea::~AFQListViewScrollArea()
{

}


void AFQListViewScrollArea::ShowScrollBar()
{
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	if(m_timerScrollVisible)
		m_timerScrollVisible->start();
}

void AFQListViewScrollArea::qSlotHideScrollBar()
{
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	if (m_timerScrollVisible)
		m_timerScrollVisible->stop();
}

void AFQListViewScrollArea::resizeEvent(QResizeEvent * event)
{
	QScrollArea::resizeEvent(event);
}

void AFQListViewScrollArea::wheelEvent(QWheelEvent* event)
{
	ShowScrollBar();

	QScrollArea::wheelEvent(event);
}