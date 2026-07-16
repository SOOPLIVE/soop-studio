#include "CTimerLabel.h"

#include <QStyleOption>
#include <QPainter>

AFQTimerLabel::AFQTimerLabel(QWidget* parent) : QLabel(parent),
    m_certainMinutecheck(false)
{
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &AFQTimerLabel::UpdateTime);
}

void AFQTimerLabel::qslotVodSplitSaved()
{
    m_certainTimeSeconds = 0;
    m_certainCheckTime = CHECK_AFTER_BROAD_TIME;
    _CertainTimeBroad(false);
}

void AFQTimerLabel::UpdateTime()
{
    m_broadTimeSeconds++;
    m_certainTimeSeconds++;
    if (m_broadTimeSeconds <= 0)
        return;

    if (m_certainTimeSeconds == m_certainCheckTime)
        _CertainTimeBroad(true);

    if (m_certainTimeSeconds == CHECK_AI_MANAGERBROAD)
    {
        m_aiManagerAble = true;
        emit qsignalAIManagerCheckTime(true);
    }

    auto hhmmss = GetHHMMSS();
    QString time = QString("<font color='#FCFCFD'><b>%1</b></font>%2").arg(m_prefix.toStdString().c_str()).arg(hhmmss);
    setText(time);
}

void AFQTimerLabel::StartCount()
{
    m_broadTimeSeconds = 0;

    QString time = QString("<font color='#FCFCFD'><b>%1</b></font>%2").arg(m_prefix.toStdString().c_str()).arg("00:00:00");
    setText(time);
    m_updateTimer->start(1000);

    m_certainCheckTime = CHECK_FIRST_BROAD_TIME;
}

void AFQTimerLabel::StopCount()
{
    m_updateTimer->stop();
    ResetCetainTime();

    m_broadTimeSeconds = 0;
}

void AFQTimerLabel::ResetCetainTime()
{
    m_certainTimeSeconds = 0;
    _CertainTimeBroad(false);
    m_aiManagerAble = false;
}

bool AFQTimerLabel::TimerStatus()
{
    return m_updateTimer->isActive();
}

QString AFQTimerLabel::GetHHMMSS()
{
    int hours = m_broadTimeSeconds / 3600;
    int minutes = (m_broadTimeSeconds % 3600) / 60;
    int seconds = m_broadTimeSeconds % 60;

    return QString("%1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

void AFQTimerLabel::_CertainTimeBroad(bool broad)
{
    m_certainMinutecheck = broad;
    emit qsignalCertainMinuteBroad(broad);
}
