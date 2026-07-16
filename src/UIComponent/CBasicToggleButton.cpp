#include "CBasicToggleButton.h"
#include "Application/CApplication.h"

#define MARGIN 1

AFQToggleButton::AFQToggleButton(QWidget* parent) : QAbstractButton(parent)
{
    _Init();

    setCheckable(true);
    setChecked(false);
    //setEnabled(false);
}

//QSize AFQToggleButton::sizeHint() const
//{
//	return QSize(2 * (m_ButtonHalfHeight + m_buttonMargin), m_ButtonHalfHeight + 2 * m_buttonMargin);
//}

void AFQToggleButton::SetChecked(bool checked)
{
    setChecked(checked);
    ChangeState(checked);
}

void AFQToggleButton::ChangeState(bool state)
{
    qDebug() << "state: " << state;
    qDebug() << "checked: " << isChecked();
    if (state) { // Unchecked -> Checked
        m_pSwitchAnimation->setStartValue(MARGIN);
        m_pSwitchAnimation->setEndValue(width() - m_buttonHeight + MARGIN);
    }
    else { // Checked -> Unchecked
        m_pSwitchAnimation->setStartValue(offset());
        m_pSwitchAnimation->setEndValue(MARGIN);
    }

    m_pSwitchAnimation->setDuration(100);
    m_pSwitchAnimation->start();
}

void AFQToggleButton::paintEvent(QPaintEvent* event)
{
    int verticalTotalMargin = MARGIN * 2; // Top Margin + Bottom Margin
    int circleDiameter = height() - verticalTotalMargin;
    int rectRoundedRadius = m_buttonHeight / 2;

    QPainter p(this);
    p.setPen(Qt::NoPen);

    QBrush rectColor;
    QBrush circleColor;

    // Set Brush Color
    if (isEnabled()) 
    {
        if (isChecked()) 
            rectColor = m_slideBrush;
        else
            rectColor = m_offSlideBrush;
        circleColor = m_buttonBrush;
    }
    else // Disabled
    {
        if (isChecked())
            rectColor = m_disabledSlideBrush;
        else
            rectColor = m_disabledOffSlideBrush;
        circleColor = m_disabledButtonBrush;
    }

   // Draw Rounded Rect
   p.setBrush(rectColor);
   p.setRenderHint(QPainter::Antialiasing, true);
   p.drawRoundedRect(QRect(0, 0, width(), height()), rectRoundedRadius, rectRoundedRadius);

   // Draw Ellipse
   p.setBrush(circleColor);
   p.drawEllipse(QRectF(offset(), MARGIN, circleDiameter, circleDiameter));
 
}

void AFQToggleButton::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() & Qt::LeftButton && !m_animatingLock) {
        if (rect().contains(event->pos())) {
            ChangeState(!isChecked());
        }
    }
    QAbstractButton::mouseReleaseEvent(event);
}

void AFQToggleButton::enterEvent(QEnterEvent* event)
{
    QAbstractButton::enterEvent(event);
}

void AFQToggleButton::resizeEvent(QResizeEvent* event)
{
    if (this->size() == this->minimumSize()) {
        m_buttonHeight = this->height();

        if (isChecked())
            setOffset(width() - m_buttonHeight);
        else
            setOffset(MARGIN);
        //m_buttonY = m_ButtonHalfHeight / 2;
    }

    SetChecked(isChecked());
    
    QWidget::resizeEvent(event);
}

void AFQToggleButton::_Init()
{
    if (!MAINFRAME)
        return;

    m_slideBrush = QBrush(QColor(1, 130, 255));
    m_offSlideBrush = QBrush(QColor(72, 74, 78));
    m_disabledSlideBrush = QBrush(QColor(29, 57, 87));
    m_disabledOffSlideBrush = QBrush(QColor(45, 48, 53));
}
