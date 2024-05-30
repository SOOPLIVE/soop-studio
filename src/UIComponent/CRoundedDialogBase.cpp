#include "CRoundedDialogBase.h"
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>

#include "Application/CApplication.h"

AFQRoundedDialogBase::AFQRoundedDialogBase(QWidget* parent, Qt::WindowFlags flag, bool bModal) :
    QDialog(parent, flag)
{
    installEventFilter(this);
    setModal(bModal);

    this->setMouseTracking(true);
    this->setAttribute(Qt::WA_Hover);
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::Window);
    this->setAttribute(Qt::WA_TranslucentBackground);

    setMinimumSize(QSize(10, 10));
}

AFQRoundedDialogBase::~AFQRoundedDialogBase() {}

void AFQRoundedDialogBase::qslotMaximizeWindow()
{
    if (isMaximized())
    {
        showNormal();
    }
    else
    {
        showMaximized();
    }
}

void AFQRoundedDialogBase::qslotMinimizeWindow()
{
    showMinimized();
}

void AFQRoundedDialogBase::mouseHoverEvent(QHoverEvent* e)
{
    _UpdateCursorShape(this->mapToGlobal(e->position().toPoint()));
}

void AFQRoundedDialogBase::mouseLeaveEvent(QEvent* e)
{
    if (!m_bLeftButtonPressed) {
        this->unsetCursor();
    }
}

void AFQRoundedDialogBase::mousePressEvent(QMouseEvent* e)
{
    if (e->button() & Qt::LeftButton) {
        m_bLeftButtonPressed = true;
        _CalculateCursorPosition(e->globalPosition().toPoint(), this->frameGeometry(), m_MousePressedEdge);
        m_CurrentPoint = e->globalPosition().toPoint();
        if (!m_MousePressedEdge.testFlag(Edge::None)) {
            m_Rubberband->setGeometry(this->frameGeometry());
        } else {
            m_maximumSize = maximumSize();
            m_minimumSize = minimumSize();
            QSize fixedSize = size();
            setFixedSize(fixedSize);
        }

        QCursor* appCursor = App()->overrideCursor();
        if (appCursor && appCursor->shape() == Qt::CrossCursor) {
            return;
        }

        if (m_bDragTitleOnly)
        {
            QRect titleRect = this->rect().marginsRemoved(QMargins(borderWidth(), borderWidth(), borderWidth(), borderWidth()));
            titleRect.setHeight(m_fTitleHeight);

            if (titleRect.contains(e->pos()))
            {
                m_bDragStart = true;
                m_DragPosition = e->pos();
            }
        }
        else
        {
            if (this->rect().marginsRemoved(QMargins(borderWidth(), borderWidth(), borderWidth(), borderWidth())).contains(e->pos())) {
                m_bDragStart = true;
                m_DragPosition = e->pos();
            }
        }
    }
}

void AFQRoundedDialogBase::mouseReleaseEvent(QMouseEvent* e)
{
    if(!m_minimumSize.isEmpty()) {
        setMaximumSize(m_maximumSize);
        setMinimumSize(m_minimumSize);
    }
    if (e->button() & Qt::LeftButton) {
        m_bLeftButtonPressed = false;
        m_bDragStart = false;
    }
}

void AFQRoundedDialogBase::mouseMoveEvent(QMouseEvent* e)
{
    if (m_bLeftButtonPressed) 
    {
        // Move Window
        if (m_bDragStart) 
        {
            if (isMaximized())
            {
                qslotMaximizeWindow();
                move(e->globalPosition().toPoint() - QPoint(width() / 2, 10));
            }

            m_NewPoint = QPoint(e->globalPosition().toPoint() - m_CurrentPoint);
            QPoint movePos(x() + m_NewPoint.x(), y() + m_NewPoint.y());
            move(movePos);
            m_CurrentPoint = e->globalPosition().toPoint();

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
                if (!m_bIsFixedWidth && !m_bIsFixedHeight)
                {
                    top = e->globalPosition().toPoint().y();
                    left = e->globalPosition().toPoint().x();
                }
                break;
            case Edge::TopRight:
                if (!m_bIsFixedWidth && !m_bIsFixedHeight)
                {
                    right = e->globalPosition().toPoint().x();
                    top = e->globalPosition().toPoint().y();
                }
                break;
            case Edge::BottomLeft:
                if (!m_bIsFixedWidth && !m_bIsFixedHeight)
                {
                    bottom = e->globalPosition().toPoint().y();
                    left = e->globalPosition().toPoint().x();
                }
                break;
            case Edge::BottomRight:
                if (!m_bIsFixedWidth && !m_bIsFixedHeight)
                {
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

bool AFQRoundedDialogBase::event(QEvent* e)
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

void AFQRoundedDialogBase::paintEvent(QPaintEvent* ev)
{
    // If the transparency flag/hint aren't set then just use the default paint event.
    if (!(windowFlags() & Qt::FramelessWindowHint) && !testAttribute(Qt::WA_TranslucentBackground)) {
        QDialog::paintEvent(ev);
        return;
    }

    // Initialize the painter.
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Have style sheet?
    if (testAttribute(Qt::WA_StyleSheetTarget)) {
        // Let QStylesheetStyle have its way with us.
        QStyleOption opt;
        opt.initFrom(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
        p.end();
        return;
    }

    // Paint thyself.
    QRectF rect(QPointF(0, 0), size());
    // Check for a border size.
    qreal penWidth = m_borderWidth;
    if (penWidth < 0.0) {
        // Look up the default border (frame) width for the current style.
        QStyleOption opt;
        opt.initFrom(this);
        penWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, this);
    }
    // Got pen?
    if (penWidth > 0.0) {
        p.setPen(QPen(palette().brush(foregroundRole()), penWidth));
        // Ensure border fits inside the available space.
        const qreal dlta = penWidth * 0.5;
        rect.adjust(dlta, dlta, -dlta, -dlta);
    }
    else {
        // QPainter comes with a default 1px pen when initialized on a QWidget.
        p.setPen(Qt::NoPen);
    }
    // Set the brush from palette role.
    p.setBrush(palette().brush(backgroundRole()));
    // Got radius?  Otherwise draw a quicker rect.
    if (m_radius > 0.0)
        p.drawRoundedRect(rect, m_radius, m_radius, Qt::AbsoluteSize);
    else
        p.drawRect(rect);

    // C'est finí
    p.end();
}

void AFQRoundedDialogBase::_SetCursorToDefault()
{
    if (this->cursor().shape() != Qt::ArrowCursor)
    {
        this->unsetCursor();
    }
}

void AFQRoundedDialogBase::_UpdateCursorShape(const QPoint& pos)
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

void AFQRoundedDialogBase::_CalculateCursorPosition(const QPoint& pos, const QRect& framerect, Edges& _edge)
{
    bool onLeft = pos.x() >= framerect.x() - m_iBorderWidth && pos.x() <= framerect.x() + m_iBorderWidth &&
        pos.y() <= framerect.y() + framerect.height() - m_iBorderWidth && pos.y() >= framerect.y() + m_iBorderWidth;

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

