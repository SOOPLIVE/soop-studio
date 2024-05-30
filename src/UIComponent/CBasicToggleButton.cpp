
#include "CBasicToggleButton.h"

AFQToggleButton::AFQToggleButton(QWidget* parent) : QAbstractButton(parent)
{
	setOffset(m_iButtonHeight / 2);
	m_iButtonY = m_iButtonHeight / 2;
    setCheckable(true);
    setChecked(false);
}

QSize AFQToggleButton::sizeHint() const
{
	return QSize(2 * (m_iButtonHeight + m_iButtonMargin), m_iButtonHeight + 2 * m_iButtonMargin);
}

void AFQToggleButton::ChangeState(bool state)
{
    if (state) {
        m_SwitchAnimation->setStartValue(m_iButtonHeight / 2);
        m_SwitchAnimation->setEndValue(width() - m_iButtonHeight - 5);
        m_SwitchAnimation->setDuration(100);
        m_SwitchAnimation->start();
    }
    else {
        m_SwitchAnimation->setStartValue(offset());
        m_SwitchAnimation->setEndValue(m_iButtonHeight / 2);
        m_SwitchAnimation->setDuration(100);
        m_SwitchAnimation->start();
    }
}

void AFQToggleButton::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    p.setPen(Qt::NoPen);
    if (isEnabled()) {
        p.setBrush(isChecked() ? m_SlideBrush : m_OffSlideBrush);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawRoundedRect(QRect(1, 0, width()-4, height()), 8, 8);
        p.setBrush(m_ButtonBrush);
        p.drawEllipse(QRectF(offset() - (m_iButtonHeight / 2), m_iButtonY - (m_iButtonHeight / 2), height(), height()));
    }
    else {
        p.setBrush(m_ButtonBrush);
        p.drawRoundedRect(QRect(m_iButtonMargin, m_iButtonMargin, width() - 2 * m_iButtonMargin, height() - 2 * m_iButtonMargin), 8, 8);
        p.setBrush(m_ButtonBrush);
        p.drawEllipse(QRectF(offset() - (m_iButtonHeight / 2), m_iButtonY - (m_iButtonHeight / 2), height(), height()));
    }
}

void AFQToggleButton::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() & Qt::LeftButton && !m_bAnimatingLock) {
        ChangeState(!isChecked());
    }
    QAbstractButton::mouseReleaseEvent(event);
}

void AFQToggleButton::enterEvent(QEnterEvent* event)
{
    QAbstractButton::enterEvent(event);
}


