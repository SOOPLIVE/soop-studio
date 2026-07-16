#include "CListViewScrollArea.h"

#include <QScrollBar>

AFQListViewScrollArea::AFQListViewScrollArea(QWidget* parent)
	: QScrollArea(parent)
{
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	m_pTimerScrollVisible = new QTimer(this);
	m_pTimerScrollVisible->setInterval(200);
	m_pTimerScrollVisible->setSingleShot(true);
	connect(m_pTimerScrollVisible, &QTimer::timeout,
			this, &AFQListViewScrollArea::qSlotHideScrollBar);
}

AFQListViewScrollArea::~AFQListViewScrollArea()
{

}


void AFQListViewScrollArea::ShowScrollBar()
{
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	if(m_pTimerScrollVisible)
		m_pTimerScrollVisible->start();
}

void AFQListViewScrollArea::qSlotHideScrollBar()
{
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	if (m_pTimerScrollVisible)
		m_pTimerScrollVisible->stop();
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
