
#include "CRoundedTextLabel.h"

#include <QPainter>

AFQRoundedTextLabel::AFQRoundedTextLabel(QWidget* parent)
:QLabel(parent) {}

void AFQRoundedTextLabel::SetContentMargins(int l, int t, int r, int b)
{
    QMargins margins = this->contentsMargins();
    margins.setLeft(l);
    margins.setTop(t);
    margins.setRight(r);
    margins.setBottom(b);
    this->setContentsMargins(margins);
}

void AFQRoundedTextLabel::paintEvent(QPaintEvent* event)
{
    QLabel::paintEvent(event);
    
    uint32_t thisHeight = height();
    
    QPainter p(this);
    p.setPen(Qt::NoPen);
    
    if (isEnabled())
    {
        QPen pen;
        pen.setWidth(1);
        pen.setColor(m_LineBrush.color());
        
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        
        p.setRenderHint(QPainter::Antialiasing, true);
        p.drawRoundedRect(QRect(0, 0, width(), height()),
                          thisHeight / 2,  thisHeight / 2);
    }

}
