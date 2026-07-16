#include "CWheelScrollArea.h"
#include "Blocks/CBlockManager.h"

AFQVerticalWheelScrollArea::AFQVerticalWheelScrollArea(QWidget *parent)
    : QScrollArea{parent}
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void AFQVerticalWheelScrollArea::wheelEvent(QWheelEvent* event)
{
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - event->angleDelta().y());
}

void AFQVerticalWheelScrollArea::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_isDragging = true;
        m_lastMousePos = event->pos();
    }
}

void AFQVerticalWheelScrollArea::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        m_isDragging = false;

}

void AFQVerticalWheelScrollArea::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging)
    {
        QPoint delta = event->pos() - m_lastMousePos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        m_lastMousePos = event->pos();
    }
}
