#pragma once

#include <QObject>
#include <QScrollArea>
#include <QMouseEvent>
#include <QScrollbar>

class AFQVerticalWheelScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit AFQVerticalWheelScrollArea(QWidget* parent = nullptr);

protected:
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
    bool m_isDragging = false;
    QPoint m_lastMousePos = QPoint(0, 0);
};

