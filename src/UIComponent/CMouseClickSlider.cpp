#include "CMouseClickSlider.h"

void AFQMouseClickSlider::mousePressEvent(QMouseEvent* event)
{
    QSlider::mousePressEvent(event);
    _SetValueToMousePos(event);
}

void AFQMouseClickSlider::_SetValueToMousePos(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (orientation() == Qt::Vertical)
            setValue(minimum() + ((maximum() - minimum()) * (height() - event->y())) / height());
        else
            setValue(minimum() + ((maximum() - minimum()) * event->x()) / width());
        //setValue(minimum() + (maximum() - minimum()) * (static_cast<float>(event->x()) / static_cast<float>(width())));
        event->accept();
    }
}