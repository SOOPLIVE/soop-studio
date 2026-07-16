#include "CMouseClickSlider.h"

void AFQMouseClickSlider::mousePressEvent(QMouseEvent* event)
{
    m_mouseButton = event->button();

    _SetValueToMousePos(event);
    QSlider::mousePressEvent(event);
}


void AFQMouseClickSlider::mouseReleaseEvent(QMouseEvent* event)
{
    m_mouseButton = Qt::MouseButton::NoButton;
    QSlider::mouseReleaseEvent(event);
}

void AFQMouseClickSlider::mouseMoveEvent(QMouseEvent* event)
{
    _SetValueToMousePos(event);    
    QSlider::mouseMoveEvent(event);

}

void AFQMouseClickSlider::_SetValueToMousePos(QMouseEvent* event)
{
   // if (event->button() == Qt::LeftButton)
    if(Qt::MouseButton::LeftButton == m_mouseButton)
    {
        if (orientation() == Qt::Vertical)
            setValue(minimum() + ((maximum() - minimum()) * (height() - event->y())) / height());
        else
            setValue(minimum() + ((maximum() - minimum()) * event->x()) / width());
        //setValue(minimum() + (maximum() - minimum()) * (static_cast<float>(event->x()) / static_cast<float>(width())));
        event->accept();
    }
}