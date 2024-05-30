#include "CTransitionWidgetStudioMode.h"

#include <QPainter>

AFTransitionWidgetStudioMode::AFTransitionWidgetStudioMode(QWidget* parent)
:AFQBaseClickWidget(parent) {}

void AFTransitionWidgetStudioMode::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    
    uint32_t thisHeight = height();
    
    QPainter p(this);
    
    if (isEnabled())
    {
        QPen pen;
        pen.setWidth(1);
        pen.setColor(m_LineColor);
        
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawRoundedRect(QRect(0, 0, width(), height()), 4, 4);
    }
}
