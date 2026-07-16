
#pragma once

#include <QWidget>
#include <QMouseEvent>

class OverlaySourceWidget : public QWidget {

public:
    explicit OverlaySourceWidget(QWidget* parent = nullptr);
    virtual ~OverlaySourceWidget();

public:
    void moveRelative(QPointF point) { relativePos = point; }
    QPointF posRelative() { return relativePos; }

protected:
    enum ResizeDirection {
        None = 0x0,
        Left = 0x1,
        Right = 0x2,
        Top = 0x4,
        Bottom = 0x8,
        TopLeft = Top | Left,
        TopRight = Top | Right,
        BottomLeft = Bottom | Left,
        BottomRight = Bottom | Right
    };
    Q_DECLARE_FLAGS(ResizeDirections, ResizeDirection)

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    ResizeDirections detectResizeDirection(const QPoint& pos);
    void updateCursor(const QPoint& pos);
    void adjustPositionIfOutOfBounds();
    void updateRelativePosition();
    void applyRelativePosition();

    bool resizing = false;
    bool dragging = false;
    QPoint dragOffset;
    QRect startGeom;
    QPoint startMouse;
    ResizeDirections resizeDir = None;
    const int margin = 8;
    QPointF relativePos;
#pragma endregion private member var
};
