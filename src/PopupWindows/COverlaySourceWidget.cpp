
#include "COverlaySourceWidget.h"

OverlaySourceWidget::OverlaySourceWidget(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);

    if (parent)
    {
        parent->installEventFilter(this);
    }
}

OverlaySourceWidget::~OverlaySourceWidget()
{
    if (parentWidget())
    {
        parentWidget()->removeEventFilter(this);
    }
}

void OverlaySourceWidget::mousePressEvent(QMouseEvent* event) {
    raise();

    QPoint pos = event->position().toPoint();
    if (event->button() == Qt::LeftButton) {
        resizeDir = detectResizeDirection(pos);
        if (resizeDir != None) {
            resizing = true;
            startGeom = geometry();
            startMouse = mapToParent(pos);
        }
        else {
            dragging = true;
            dragOffset = pos;
        }
    }
}

void OverlaySourceWidget::mouseMoveEvent(QMouseEvent* event) {
    QPoint pos = event->position().toPoint();
    QPoint globalPos = mapToParent(pos);

    if (resizing) {
        QRect newGeom = startGeom;
        QPoint delta = globalPos - startMouse;

        QSize minSize = minimumSize();
        QSize maxSize = maximumSize();
        QSize parentSize = parentWidget() ? parentWidget()->size() : QSize(INT_MAX, INT_MAX);

        // Horizontal resizing
        if (resizeDir & Left) {
            int newWidth = startGeom.width() - delta.x();

            int maxDeltaX = startGeom.right() - qMax(0, parentSize.width() - maxSize.width());
            int maxLeft = qMin(startGeom.left() + delta.x(), startGeom.right() - minSize.width());
            int boundedLeft = qMax(0, maxLeft);

            newGeom.setLeft(boundedLeft);
        }

        if (resizeDir & Right) {
            int newWidth = startGeom.width() + delta.x();
            int maxWidth = qMin(maxSize.width(), parentSize.width() - startGeom.left());
            int boundedWidth = qBound(minSize.width(), newWidth, maxWidth);
            newGeom.setWidth(boundedWidth);
        }

        // Vertical resizing
        if (resizeDir & Top) {
            int newHeight = startGeom.height() - delta.y();

            int maxTop = qMin(startGeom.top() + delta.y(), startGeom.bottom() - minSize.height());
            int boundedTop = qMax(0, maxTop);

            newGeom.setTop(boundedTop);
        }

        if (resizeDir & Bottom) {
            int newHeight = startGeom.height() + delta.y();
            int maxHeight = qMin(maxSize.height(), parentSize.height() - startGeom.top());
            int boundedHeight = qBound(minSize.height(), newHeight, maxHeight);
            newGeom.setHeight(boundedHeight);
        }

        setGeometry(newGeom);
    }
    else if (dragging) {
        QPoint newTopLeft = mapToParent(pos - dragOffset);
        if (parentWidget()) {
            QSize parentSize = parentWidget()->size();
            QSize widgetSize = size();

            int maxX = parentSize.width() - widgetSize.width();
            int maxY = parentSize.height() - widgetSize.height();

            newTopLeft.setX(qBound(0, newTopLeft.x(), maxX));
            newTopLeft.setY(qBound(0, newTopLeft.y(), maxY));
        }
        move(newTopLeft);
    }
    else {
        updateCursor(pos);
    }
}


void OverlaySourceWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        resizing = false;
        dragging = false;
        resizeDir = None;
        updateRelativePosition();
    }
}

bool OverlaySourceWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        adjustPositionIfOutOfBounds();
    }
    return QWidget::eventFilter(watched, event);
}

OverlaySourceWidget::ResizeDirections OverlaySourceWidget::detectResizeDirection(const QPoint& pos) {
    if (minimumSize() == maximumSize()) // setFixedSize
        return None;

    bool left = pos.x() < margin;
    bool right = pos.x() > width() - margin;
    bool top = pos.y() < margin;
    bool bottom = pos.y() > height() - margin;

    ResizeDirections dir = None;
    if (left)   dir |= Left;
    if (right)  dir |= Right;
    if (top)    dir |= Top;
    if (bottom) dir |= Bottom;
    return dir;
}

void OverlaySourceWidget::updateCursor(const QPoint& pos) {
    switch (detectResizeDirection(pos)) {
    case TopLeft:
    case BottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case TopRight:
    case BottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case Top:
    case Bottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case Left:
    case Right:
        setCursor(Qt::SizeHorCursor);
        break;
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

void OverlaySourceWidget::adjustPositionIfOutOfBounds() {
    applyRelativePosition();
}

void OverlaySourceWidget::updateRelativePosition()
{
    if (!parentWidget()) return;

    QSize parentSize = parentWidget()->size();
    if (parentSize.width() == 0 || parentSize.height() == 0)
        return;

    QPoint topLeft = pos();
    relativePos.setX(double(topLeft.x()) / parentSize.width());
    relativePos.setY(double(topLeft.y()) / parentSize.height());
}

void OverlaySourceWidget::applyRelativePosition()
{
    if (!parentWidget()) return;

    QSize parentSize = parentWidget()->size();

    int newWidth = qMin(width(), parentSize.width());
    int newHeight = qMin(height(), parentSize.height());

    newWidth = qBound(minimumWidth(), newWidth, maximumWidth());
    newHeight = qBound(minimumHeight(), newHeight, maximumHeight());

    resize(newWidth, newHeight);

    int newX = int(relativePos.x() * parentSize.width());
    int newY = int(relativePos.y() * parentSize.height());

    newX = qBound(0, newX, parentSize.width() - newWidth);
    newY = qBound(0, newY, parentSize.height() - newHeight);

    move(newX, newY);
}
