#include "CMainBaseWidget.h"

#include <QMouseEvent>
#include <QGuiapplication>

AFCQMainBaseWidget::AFCQMainBaseWidget(QWidget* parent, Qt::WindowFlags flag) :
    QWidget(parent, flag)
{
    this->setMouseTracking(true);
    this->setAttribute(Qt::WA_Hover);
    this->installEventFilter(this);
    
    setMinimumSize(QSize(10, 10));
}

AFCQMainBaseWidget::~AFCQMainBaseWidget() {}

void AFCQMainBaseWidget::qslotMaximizeWindow()
{
    if (isMaximized())
    {
        changeWidgetBorder(false);
        showNormal();
    }
    else
    {
        changeWidgetBorder(true);         
        showMaximized();
    }
    emit qsignalBaseWindowMaximized(isMaximized());
}

void AFCQMainBaseWidget::qslotMinimizeWindow()
{
    showMinimized();
}

void AFCQMainBaseWidget::mouseHoverEvent(QHoverEvent* e)
{
    _UpdateCursorShape(this->mapToGlobal(e->position().toPoint()));
}

void AFCQMainBaseWidget::mouseLeaveEvent(QEvent* e)
{
    if (!m_bLeftButtonPressed) {
        this->unsetCursor();
    }
}

void AFCQMainBaseWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() & Qt::LeftButton) {
        m_bLeftButtonPressed = true;
        _CalculateCursorPosition(e->globalPosition().toPoint(), this->frameGeometry(), m_MousePressedEdge);
        m_CurrentPoint = e->pos();
        if (!m_MousePressedEdge.testFlag(Edge::None)) {
            m_Rubberband->setGeometry(this->frameGeometry());
        } else {
            m_maximumSize = maximumSize();
            m_minimumSize = minimumSize();
            if (isMaximized())
            {
                m_IsFixedSizeSet = false;
            }
            else
            {
                QSize fixedSize = size();
                setFixedSize(fixedSize);
                m_IsFixedSizeSet = true;
            }
        }
        if (this->rect().marginsRemoved(QMargins(borderWidth(), borderWidth(), borderWidth(), borderWidth())).contains(e->pos())) {
            m_bDragStart = true;
            m_DragPosition = e->pos();
        }
    }
}

void AFCQMainBaseWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if(!m_minimumSize.isEmpty()) {
        setMaximumSize(m_maximumSize);
        setMinimumSize(m_minimumSize);
    }
    //
    if (e->button() & Qt::LeftButton) 
    {
        m_bLeftButtonPressed = false;

        if(!m_bDragStart)
            _AdjustWidgetSizeToScreen();

        m_bDragStart = false;
        emit qsignalBaseWindowMouseRelease();
    }
}

void AFCQMainBaseWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (m_bLeftButtonPressed) 
    {
        // Move Window
        if (m_bDragStart) 
        {
            if (isMaximized())
            {
                qslotMaximizeWindow();  
                _AdjustMaximizeDragPosition(e);
            }
            else {
                if (!m_IsFixedSizeSet)
                {
                    QSize fixedSize = size();
                    setFixedSize(fixedSize);
                }

                move(e->globalPosition().toPoint() - m_CurrentPoint);
            }

            _SetCursorToDefault();
            return;
        }

        // Resize Window
        if (!m_MousePressedEdge.testFlag(Edge::None)) 
        {
            if (isMaximized())
            {
                if (m_MousePressedEdge == Edge::Top)
                    m_bDragStart = true;
                
                return;
            }

            int left = m_Rubberband->frameGeometry().left();
            int top = m_Rubberband->frameGeometry().top();
            int right = m_Rubberband->frameGeometry().right();
            int bottom = m_Rubberband->frameGeometry().bottom();

            switch (m_MousePressedEdge) {
            case Edge::Top:
                if (!m_bIsFixedHeight)
                    top = e->globalPosition().toPoint().y();
                break;
            case Edge::Bottom:
                if (!m_bIsFixedHeight)
                    bottom = e->globalPosition().toPoint().y();
                break;
            case Edge::Left:
                if (!m_bIsFixedWidth)
                    left = e->globalPosition().toPoint().x();
                break;
            case Edge::Right:
                if (!m_bIsFixedWidth)
                    right = e->globalPosition().toPoint().x();
                break;
            case Edge::TopLeft:
                if (!m_bIsFixedWidth && !m_bIsFixedHeight) {
                    top = e->globalPosition().toPoint().y();
                    left = e->globalPosition().toPoint().x();
                }
                break;
            case Edge::TopRight:
                if (!m_bIsFixedWidth && !m_bIsFixedHeight) {
                    right = e->globalPosition().toPoint().x();
                    top = e->globalPosition().toPoint().y();
                }
                break;
            case Edge::BottomLeft:
                if (!m_bIsFixedWidth && !m_bIsFixedHeight) {
                    bottom = e->globalPosition().toPoint().y();
                    left = e->globalPosition().toPoint().x();
                }
                break;
            case Edge::BottomRight:
                if (!m_bIsFixedWidth && !m_bIsFixedHeight) {
                    bottom = e->globalPosition().toPoint().y();
                    right = e->globalPosition().toPoint().x();
                }
                break;
            }
            QRect newRect(QPoint(left, top), QPoint(right, bottom));
            if (newRect.width() <= this->minimumWidth()) {
                left = this->frameGeometry().x();
            }
            if (newRect.height() <= this->minimumHeight()) {
                top = this->frameGeometry().y();
            }
            this->setGeometry(QRect(QPoint(left, top), QPoint(right, bottom)));
            m_Rubberband->setGeometry(QRect(QPoint(left, top), QPoint(right, bottom)));
        }
    }
    else {
        _UpdateCursorShape(e->globalPosition().toPoint());
    }
}

bool AFCQMainBaseWidget::event(QEvent* e)
{
    switch (e->type())
    {
    case QEvent::HoverMove:
        mouseHoverEvent(static_cast<QHoverEvent*>(e));
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void AFCQMainBaseWidget::_SetCursorToDefault()
{
    if (this->cursor().shape() != Qt::ArrowCursor)
    {
        this->unsetCursor();
    }
}

void AFCQMainBaseWidget::_UpdateCursorShape(const QPoint& pos)
{
    if (this->isFullScreen() || this->isMaximized()) {
        if (m_bCursorChanged) {
            this->unsetCursor();
        }
        return;
    }
    if (!m_bLeftButtonPressed) {
        _CalculateCursorPosition(pos, this->frameGeometry(), m_MouseMoveEdge);
        m_bCursorChanged = true;
        if (m_MouseMoveEdge.testFlag(Edge::Top) || m_MouseMoveEdge.testFlag(Edge::Bottom)) {
            if (!m_bIsFixedHeight)
                this->setCursor(Qt::SizeVerCursor);
        }
        else if (m_MouseMoveEdge.testFlag(Edge::Left) || m_MouseMoveEdge.testFlag(Edge::Right)) {
            if (!m_bIsFixedWidth)
                this->setCursor(Qt::SizeHorCursor);
        }
        else if (m_MouseMoveEdge.testFlag(Edge::TopLeft) || m_MouseMoveEdge.testFlag(Edge::BottomRight)) {
            if (!m_bIsFixedWidth && !m_bIsFixedHeight)
                this->setCursor(Qt::SizeFDiagCursor);
        }
        else if (m_MouseMoveEdge.testFlag(Edge::TopRight) || m_MouseMoveEdge.testFlag(Edge::BottomLeft)) {
            if (!m_bIsFixedWidth && !m_bIsFixedHeight)
                this->setCursor(Qt::SizeBDiagCursor);
        }
        else if (m_bCursorChanged) {
            this->unsetCursor();
            m_bCursorChanged = false;
        }
    }
}

void AFCQMainBaseWidget::_CalculateCursorPosition(const QPoint& pos, const QRect& framerect, Edges& _edge)
{
    bool onLeft = pos.x() >= framerect.x() - m_iBorderWidth && pos.x() <= framerect.x() + m_iBorderWidth &&
        pos.y() <= framerect.y() + framerect.height() - m_iBorderWidth  && pos.y() >= framerect.y() + m_iBorderWidth;

    bool onRight = pos.x() >= framerect.x() + framerect.width() - m_iBorderWidth && pos.x() <= framerect.x() + framerect.width() &&
        pos.y() >= framerect.y() + m_iBorderWidth && pos.y() <= framerect.y() + framerect.height() - m_iBorderWidth;

    bool onBottom = pos.x() >= framerect.x() + m_iBorderWidth && pos.x() <= framerect.x() + framerect.width() - m_iBorderWidth &&
        pos.y() >= framerect.y() + framerect.height() - m_iBorderWidth && pos.y() <= framerect.y() + framerect.height();

    bool onTop = pos.x() >= framerect.x() + m_iBorderWidth && pos.x() <= framerect.x() + framerect.width() - m_iBorderWidth &&
        pos.y() >= framerect.y() && pos.y() <= framerect.y() + m_iBorderWidth;

    bool  onBottomLeft = pos.x() <= framerect.x() + m_iBorderWidth && pos.x() >= framerect.x() &&
        pos.y() <= framerect.y() + framerect.height() && pos.y() >= framerect.y() + framerect.height() - m_iBorderWidth;

    bool onBottomRight = pos.x() >= framerect.x() + framerect.width() - m_iBorderWidth && pos.x() <= framerect.x() + framerect.width() &&
        pos.y() >= framerect.y() + framerect.height() - m_iBorderWidth && pos.y() <= framerect.y() + framerect.height();

    bool onTopRight = pos.x() >= framerect.x() + framerect.width() - m_iBorderWidth && pos.x() <= framerect.x() + framerect.width() &&
        pos.y() >= framerect.y() && pos.y() <= framerect.y() + m_iBorderWidth;

    bool onTopLeft = pos.x() >= framerect.x() && pos.x() <= framerect.x() + m_iBorderWidth &&
        pos.y() >= framerect.y() && pos.y() <= framerect.y() + m_iBorderWidth;

    if (onLeft) {
        _edge = Left;
    }
    else if (onRight) {
        _edge = Right;
    }
    else if (onBottom) {
        _edge = Bottom;
    }
    else if (onTop) {
        _edge = Top;
    }
    else if (onBottomLeft) {
        _edge = BottomLeft;
    }
    else if (onBottomRight) {
        _edge = BottomRight;
    }
    else if (onTopRight) {
        _edge = TopRight;
    }
    else if (onTopLeft) {
        _edge = TopLeft;
    }
    else {
        _edge = None;
    }
}

void AFCQMainBaseWidget::_AdjustWidgetSizeToScreen()
{
    if (!(m_MousePressedEdge == Edge::Bottom ||
        m_MousePressedEdge == Edge::BottomLeft ||
        m_MousePressedEdge == Edge::BottomRight))
        return;

    QPoint cursorPos = QCursor::pos();
    QScreen* screen = QGuiApplication::screenAt(cursorPos);

    if (screen == nullptr)
        screen = QGuiApplication::screenAt(this->frameGeometry().center());

    if (screen == nullptr)
        return;


    QRect boundingBox = screen->availableGeometry();

    int screenTop = boundingBox.top();
    int screenBottom = boundingBox.bottom();

    int widgetLeft = this->frameGeometry().left();
    int widgetTop = this->frameGeometry().top();
    int widgetRight = this->frameGeometry().right();
    int widgetBottom = this->frameGeometry().bottom();

    if (widgetTop < screenTop)
    {
        widgetBottom += screenTop - widgetTop;
        widgetTop = screenTop;

        this->setGeometry(QRect(QPoint(widgetLeft, screenTop), QPoint(widgetRight, widgetBottom)));
        m_Rubberband->setGeometry(QRect(QPoint(widgetLeft, screenTop), QPoint(widgetRight, widgetBottom)));
    }

    if (widgetBottom > screenBottom)
    {
        this->setGeometry(QRect(QPoint(widgetLeft, widgetTop), QPoint(widgetRight, screenBottom)));
        m_Rubberband->setGeometry(QRect(QPoint(widgetLeft, widgetTop), QPoint(widgetRight, screenBottom)));
    }
}


void AFCQMainBaseWidget::_AdjustMaximizeDragPosition(QMouseEvent* event)
{
#define POPUP_TITLE_HEIGHT  40

    int width = ( -1 != m_RestoreNormalWidth ? m_RestoreNormalWidth : this->width());

    QPoint newPos = event->globalPosition().toPoint();
    int newX = newPos.x() - width / 2;
    int newY = newPos.y() - POPUP_TITLE_HEIGHT / 2;
    move(newX, newY);
    m_CurrentPoint.setX(width / 2);
    m_CurrentPoint.setY(POPUP_TITLE_HEIGHT / 2);
}