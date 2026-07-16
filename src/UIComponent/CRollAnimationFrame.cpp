
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

	switch (m_framesList.count())
	{
	case 0:
		newFrame->setGeometry(m_frontRect);
		m_frontFrameCount = 0;
		break;
	case 1:
		newFrame->VisibleToggleButton(false);
		newFrame->setGeometry(m_bottomRect);
		break;
	default:
		newFrame->VisibleToggleButton(false);
		newFrame->setGeometry(m_topRect);
	}
	m_framesList.append(newFrame);
	m_framesList[m_frontFrameCount]->raise();
}

void AFQRollAnimationFrame::wheelEvent(QWheelEvent* e)
{
	if (m_framesList.count() > 1)
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
	if (m_animatingLock)
		return;

	m_animatingLock = true;

	QPropertyAnimation* backAnimation = new QPropertyAnimation(m_framesList[m_frontFrameCount], "geometry", this);
	backAnimation->setDuration(100);
	backAnimation->setStartValue(m_frontRect);
	backAnimation->setEndValue(m_topRect);

	AFQLoginToggleFrame* hidetoggleFrame = reinterpret_cast<AFQLoginToggleFrame*>(m_framesList[m_frontFrameCount]);
	hidetoggleFrame->VisibleToggleButton(false);

	int bottomFrameIndex = (m_frontFrameCount + 1 == m_framesList.count()) ? 0 : m_frontFrameCount + 1;
	QPropertyAnimation* frontAnimation = new QPropertyAnimation(m_framesList[bottomFrameIndex], "geometry", this);
	frontAnimation->setDuration(100);
	frontAnimation->setStartValue(m_bottomRect);
	frontAnimation->setEndValue(m_frontRect);
	connect(frontAnimation, &QPropertyAnimation::finished, [=] {
		m_animatingLock = false;
		});

	AFQLoginToggleFrame* showtoggleFrame = reinterpret_cast<AFQLoginToggleFrame*>(m_framesList[bottomFrameIndex]);
	showtoggleFrame->VisibleToggleButton(true);
	showtoggleFrame->raise();

	int adjustFrameIndex = (m_frontFrameCount - 1 < 0) ? m_framesList.count() - 1 : m_frontFrameCount - 1;

	if (m_framesList.count() > 3)
	{
		m_framesList[adjustFrameIndex]->hide();
		adjustFrameIndex = (m_frontFrameCount - 2 < 0) ? m_framesList.count() + (m_frontFrameCount - 2) : m_frontFrameCount - 2;
		m_framesList[adjustFrameIndex]->show();
	}

	m_framesList[adjustFrameIndex]->setGeometry(m_bottomRect);

	backAnimation->start();
	frontAnimation->start();

	m_frontFrameCount = bottomFrameIndex;
}

void AFQRollAnimationFrame::_RollAnimationWheelDown()
{
	if (m_animatingLock)
		return;

	m_animatingLock = true;

	QPropertyAnimation* backAnimation = new QPropertyAnimation(m_framesList[m_frontFrameCount], "geometry", this);
	backAnimation->setDuration(100);
	backAnimation->setStartValue(m_frontRect);
	backAnimation->setEndValue(m_bottomRect);
	AFQLoginToggleFrame* hidetoggleFrame = reinterpret_cast<AFQLoginToggleFrame*>(m_framesList[m_frontFrameCount]);
	hidetoggleFrame->VisibleToggleButton(false);

	int topFrameIndex = (m_frontFrameCount - 1 < 0) ? m_framesList.count() - 1 : m_frontFrameCount - 1;
	QPropertyAnimation* frontAnimation = new QPropertyAnimation(m_framesList[topFrameIndex], "geometry", this);
	frontAnimation->setDuration(100);
	frontAnimation->setStartValue(m_topRect);
	frontAnimation->setEndValue(m_frontRect);
	connect(frontAnimation, &QPropertyAnimation::finished, [=] {
		m_animatingLock = false;
		});

	AFQLoginToggleFrame* showtoggleFrame = reinterpret_cast<AFQLoginToggleFrame*>(m_framesList[topFrameIndex]);
	showtoggleFrame->VisibleToggleButton(true);
	showtoggleFrame->raise();

	int adjustFrameIndex = (m_frontFrameCount + 1 == m_framesList.count()) ? 0 : m_frontFrameCount + 1;
	if (m_framesList.count() > 3)
	{
		m_framesList[adjustFrameIndex]->hide();
		adjustFrameIndex = (m_frontFrameCount + 2 > m_framesList.count() - 1) ? (m_frontFrameCount + 2) - (m_framesList.count()) : m_frontFrameCount + 2;
		m_framesList[adjustFrameIndex]->show();
	}
	m_framesList[adjustFrameIndex]->setGeometry(m_topRect);

	backAnimation->start();
	frontAnimation->start();

	m_frontFrameCount = topFrameIndex;
}

