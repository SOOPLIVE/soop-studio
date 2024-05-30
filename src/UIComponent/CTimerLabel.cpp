#include "CTimerLabel.h"

#include <QStyleOption>
#include <QPainter>

AFQTimerLabel::AFQTimerLabel(QWidget* parent) : QLabel(parent)
{
    m_UpdateTimer = new QTimer(this);
    connect(m_UpdateTimer, &QTimer::timeout, this, &AFQTimerLabel::UpdateTime);
}

void AFQTimerLabel::UpdateTime()
{
    m_Seconds++;

    // Calculate hours, minutes, and seconds
    int hours = m_Seconds / 3600;
    int minutes = (m_Seconds % 3600) / 60;
    int seconds = m_Seconds % 60;

    // Format the elapsed time as a string
    QString timeStr = QString("%1:%2:%3").arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));

    // Update the QLabel with the formatted time
    setText(timeStr);
}

void AFQTimerLabel::StartCount()
{
    m_Seconds = 0;
    setText("00:00:00");
    m_UpdateTimer->start(1000);

}

void AFQTimerLabel::StopCount()
{
    m_UpdateTimer->stop();
    m_Seconds = 0;

}

bool AFQTimerLabel::TimerStatus()
{
    return m_UpdateTimer->isActive();
}

