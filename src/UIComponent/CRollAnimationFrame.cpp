
#include "CRollAnimationFrame.h"

#include <QWheelEvent>
#include <QPropertyAnimation>

#include "CLoginToggleFrame.h"

// Rotate Order (4 frames)
// 3    0    1    2    3
// 0 -> 1 -> 2 -> 3 -> 0
// 1    2    3    0    1

AFQRollAnimationFrame::AFQRollAnimationFrame(QWidget *parent) : QFrame(parent) {}

void AFQRollAnimationFrame::AddFrame(QString platformName, QString iconPath)
{
	AFQLoginToggleFrame* newFrame = new AFQLoginToggleFrame(this);
	newFrame->LoginToggleFrameInit(platformName, iconPath);

	switch (m_FramesList.count())
	{
	case 0:
		newFrame->setGeometry(m_FrontRect);
		m_iFrontFrameCount = 0;
		break;
	case 1:
		newFrame->VisibleToggleButton(false);
		newFrame->setGeometry(m_BottomRect);
		break;
	default:
		newFrame->VisibleToggleButton(false);
		newFrame->setGeometry(m_TopRect);
	}
	m_FramesList.append(newFrame);
	m_FramesList[m_iFrontFrameCount]->raise();
}

void AFQRollAnimationFrame::wheelEvent(QWheelEvent* e)
{
	if (m_FramesList.count() > 1)
	{
		if (e->angleDelta().y() > 0) // up Wheel
		{
			_RollAnimationWheelUp();
		}
		else if (e->angleDelta().y() < 0) //down Wheel
		{
			_RollAnimationWheelDown();
		}
	}
}

void AFQRollAnimationFrame::_RollAnimationWheelUp()
{
	if (m_bAnimatingLock)
		return;

	m_bAnimatingLock = true;

	QPropertyAnimation* backAnimation = new QPropertyAnimation(m_FramesList[m_iFrontFrameCount], "geometry", this);
	backAnimation->setDuration(100);
	backAnimation->setStartValue(m_FrontRect);
	backAnimation->setEndValue(m_TopRect);

	AFQLoginToggleFrame* hidetoggleFrame = reinterpret_cast<AFQLoginToggleFrame*>(m_FramesList[m_iFrontFrameCount]);
	hidetoggleFrame->VisibleToggleButton(false);

	int bottomFrameIndex = (m_iFrontFrameCount + 1 == m_FramesList.count()) ? 0 : m_iFrontFrameCount + 1;
	QPropertyAnimation* frontAnimation = new QPropertyAnimation(m_FramesList[bottomFrameIndex], "geometry", this);
	frontAnimation->setDuration(100);
	frontAnimation->setStartValue(m_BottomRect);
	frontAnimation->setEndValue(m_FrontRect);
	connect(frontAnimation, &QPropertyAnimation::finished, [=] {
		m_bAnimatingLock = false;
		});

	AFQLoginToggleFrame* showtoggleFrame = reinterpret_cast<AFQLoginToggleFrame*>(m_FramesList[bottomFrameIndex]);
	showtoggleFrame->VisibleToggleButton(true);
	showtoggleFrame->raise();

	int adjustFrameIndex = (m_iFrontFrameCount - 1 < 0) ? m_FramesList.count() - 1 : m_iFrontFrameCount - 1;

	if (m_FramesList.count() > 3)
	{
		m_FramesList[adjustFrameIndex]->hide();
		adjustFrameIndex = (m_iFrontFrameCount - 2 < 0) ? m_FramesList.count() + (m_iFrontFrameCount - 2) : m_iFrontFrameCount - 2;
		m_FramesList[adjustFrameIndex]->show();
	}

	m_FramesList[adjustFrameIndex]->setGeometry(m_BottomRect);

	backAnimation->start();
	frontAnimation->start();

	m_iFrontFrameCount = bottomFrameIndex;
}

void AFQRollAnimationFrame::_RollAnimationWheelDown()
{
	if (m_bAnimatingLock)
		return;

	m_bAnimatingLock = true;

	QPropertyAnimation* backAnimation = new QPropertyAnimation(m_FramesList[m_iFrontFrameCount], "geometry", this);
	backAnimation->setDuration(100);
	backAnimation->setStartValue(m_FrontRect);
	backAnimation->setEndValue(m_BottomRect);
	AFQLoginToggleFrame* hidetoggleFrame = reinterpret_cast<AFQLoginToggleFrame*>(m_FramesList[m_iFrontFrameCount]);
	hidetoggleFrame->VisibleToggleButton(false);

	int topFrameIndex = (m_iFrontFrameCount - 1 < 0) ? m_FramesList.count() - 1 : m_iFrontFrameCount - 1;
	QPropertyAnimation* frontAnimation = new QPropertyAnimation(m_FramesList[topFrameIndex], "geometry", this);
	frontAnimation->setDuration(100);
	frontAnimation->setStartValue(m_TopRect);
	frontAnimation->setEndValue(m_FrontRect);
	connect(frontAnimation, &QPropertyAnimation::finished, [=] {
		m_bAnimatingLock = false;
		});

	AFQLoginToggleFrame* showtoggleFrame = reinterpret_cast<AFQLoginToggleFrame*>(m_FramesList[topFrameIndex]);
	showtoggleFrame->VisibleToggleButton(true);
	showtoggleFrame->raise();

	int adjustFrameIndex = (m_iFrontFrameCount + 1 == m_FramesList.count()) ? 0 : m_iFrontFrameCount + 1;
	if (m_FramesList.count() > 3)
	{
		m_FramesList[adjustFrameIndex]->hide();
		adjustFrameIndex = (m_iFrontFrameCount + 2 > m_FramesList.count() - 1) ? (m_iFrontFrameCount + 2) - (m_FramesList.count()) : m_iFrontFrameCount + 2;
		m_FramesList[adjustFrameIndex]->show();
	}
	m_FramesList[adjustFrameIndex]->setGeometry(m_TopRect);

	backAnimation->start();
	frontAnimation->start();

	m_iFrontFrameCount = topFrameIndex;
}

