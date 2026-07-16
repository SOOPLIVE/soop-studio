#include "CMainBaseWidget.h"

#include <QMouseEvent>
#include "Application/CApplication.h"
#include <QWindow>

#ifdef _WIN32
#include <Windows.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "Comctl32.lib")
#endif

#ifdef _WIN32
const bool g_Win10 = true;

template<typename T> static int getDpiScale(T value, double dpi)
{
    return qRound(value * dpi / 96.0);
}

static bool isMaximized(HWND handle)
{
    WINDOWPLACEMENT placement = { 0 };
    placement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(handle, &placement)) 
    {
        return placement.showCmd == SW_SHOWMAXIMIZED;
    }
    return false;
}

static bool wmNcCalcSize(qintptr* result, HWND hwnd, LPARAM lParam, bool fullscreen)
{
    UINT dpi = GetDpiForWindow(hwnd);

    int fx = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
    int fy = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
    int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

    auto params = (NCCALCSIZE_PARAMS*)lParam;
    RECT& rect0 = params->rgrc[0];

    if (g_Win10)
    {
        if (!fullscreen)
        {
            rect0.right -= fx + padding;
            rect0.left += fx + padding;
            rect0.bottom -= fy + padding;
            if (isMaximized(hwnd)) {
                rect0.top += fy + padding;
            }
        }
    }
    else if (isMaximized(hwnd)) 
    {
        rect0.right -= fx + padding;
        rect0.left += fx + padding;
        rect0.bottom -= fy + padding;
        rect0.top += fy + padding;
    }

    *result = 0;
    return true;
}

static bool isMaximizedOrFullScreen(AFCQMainBaseWidget* widget)
{
    if (auto state = widget->windowState(); state.testFlag(Qt::WindowMaximized) || state.testFlag(Qt::WindowFullScreen)) 
        return true;
    return false;
}

static RECT titlebarRect(HWND handle, AFCQMainBaseWidget* widget)
{
    int height = widget->TitleBarHeight();
    if (height == 0)
        return { 0, 0, 0, 0 };

    UINT dpi = GetDpiForWindow(handle);

    if (height < 0) 
    {
        SIZE titlebarSize = { 0 };
        HTHEME theme = OpenThemeData(handle, L"WINDOW");
        GetThemePartSize(theme, nullptr, WP_CAPTION, CS_ACTIVE, nullptr, TS_TRUE, &titlebarSize);
        CloseThemeData(theme);

        height = getDpiScale(titlebarSize.cy, dpi);
    }
    else 
    {
        height = getDpiScale(height, dpi);
    }

    RECT rect;
    GetClientRect(handle, &rect);
    rect.bottom = rect.top + height;
    return rect;
}

bool wmNcHitTest(qintptr* result, AFCQMainBaseWidget* widget, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    bool isMaxAndFull = isMaximizedOrFullScreen(widget);
    auto hit = DefWindowProc(hwnd, message, wParam, lParam);

    QPoint hitPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    switch (hit) 
    {
    case HTLEFT:
    case HTRIGHT:
    case HTNOWHERE:
    case HTTOPLEFT:
    case HTTOP:
    case HTTOPRIGHT:
    case HTBOTTOMRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT:
        if (isMaxAndFull) 
        {
            *result = HTCLIENT;
        }
        else if (widget->ResizeEnabled()) 
        {
            *result = hit;
        }
        else if (widget->WidthResizeEnabled()) 
        {
            *result = (hit == HTLEFT || hit == HTRIGHT) ? hit : (widget->MoveAllArea() ? HTCAPTION : HTCLIENT);
        }
        else if (widget->HeightResizeEnabled()) 
        {
            *result = (hit == HTTOP || hit == HTBOTTOM) ? hit : (widget->MoveAllArea() ? HTCAPTION : HTCLIENT);
        }
        return true;
    default:
        break;
    }

    UINT dpi = GetDpiForWindow(hwnd);
    int fy = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
    int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    int cy = fy + padding;

    POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    ScreenToClient(hwnd, &pt);
    if (!g_Win10) {
        int fx = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
        int cx = fx + padding;
        RECT rc;
        GetClientRect(hwnd, &rc);
        rc.left += cx;
        rc.top += cy;
        rc.right -= cx;
        rc.bottom -= cy;

        if (!isMaxAndFull) 
        {
            if (widget->WidthResizeEnabled() && widget->HeightResizeEnabled())
            {
                if (pt.x < rc.left && pt.y < rc.top) {
                    *result = HTTOPLEFT;
                    return true;
                }
                else if (pt.x < rc.left && pt.y > rc.bottom)
                {
                    *result = HTBOTTOMLEFT;
                    return true;
                }
                else if (pt.x > rc.right && pt.y < rc.top)
                {
                    *result = HTTOPRIGHT;
                    return true;
                }
                else if (pt.x > rc.right && pt.y > rc.bottom) 
                {
                    *result = HTBOTTOMRIGHT;
                    return true;
                }
            }

            if (widget->WidthResizeEnabled()) 
            {
                if (pt.x < rc.left) {
                    *result = HTLEFT;
                    return true;
                }
                else if (pt.x > rc.right) 
                {
                    *result = HTRIGHT;
                    return true;
                }
            }

            if (widget->HeightResizeEnabled()) 
            {
                if (pt.y < rc.top) {
                    *result = HTTOP;
                    return true;
                }
                else if (pt.y > rc.bottom)
                {
                    *result = HTBOTTOM;
                    return true;
                }
            }
        }

        if (widget->HasTitleBar() && (pt.y < titlebarRect(hwnd, widget).bottom)) 
        {
            *result = HTCAPTION;
            return true;
        }
    }
    else if (pt.y < cy) 
    {
        if (!isMaxAndFull && widget->HeightResizeEnabled())
        {
            *result = HTTOP;
        }
        else if (widget->HasTitleBar() || widget->MoveAllArea())
        {
            *result = HTCAPTION;
        }
        else
        {
            *result = HTCLIENT;
        }
        return true;
    }
    else if (widget->HasTitleBar() && (pt.y < titlebarRect(hwnd, widget).bottom)) 
    {
        QScreen* screen = widget->screen();
        if (!screen) 
            return false;

        QRect screenPosition = screen->geometry();
        qreal devicePixelRatio = screen->devicePixelRatio(); 

        QPoint adjustedPos = QPoint((hitPoint.x() - screenPosition.x()) / devicePixelRatio + screenPosition.x(),
            (hitPoint.y() - screenPosition.y()) / devicePixelRatio + screenPosition.y());


        QWidget* pushwidget = QApplication::widgetAt(adjustedPos);

        if (pushwidget && pushwidget->isWidgetType()) 
        {
            QRect wGeo = widget->geometry();
            QPoint globalPos = pushwidget->mapToGlobal(QPoint(0, 0));
            QRect widgetGeo = QRect(globalPos.x(), globalPos.y(), pushwidget->width(), pushwidget->height());


            if (qobject_cast<QPushButton*>(pushwidget) != nullptr && widgetGeo.contains(adjustedPos)) 
            {
                *result = HTCLIENT;
                return true;
            }
        }
        *result = HTCAPTION;
        return true;
    }
    *result = widget->MoveAllArea() ? HTCAPTION : HTCLIENT;
    return true;
}

static bool wmCreate(HWND hwnd)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);
    SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
    return false;
}
static bool wmActivate(qintptr* result, AFCQMainBaseWidget* widget, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect = titlebarRect(hwnd, widget);
    InvalidateRect(hwnd, &rect, FALSE);
    *result = DefWindowProc(hwnd, message, wParam, lParam);
    return true;
}

static bool isWidgetFullscreen(QWidget* widget)
{
    return widget->windowState().testFlag(Qt::WindowFullScreen);
}
#endif


AFCQMainBaseWidget::AFCQMainBaseWidget(QWidget* parent, Qt::WindowFlags flag,
    bool widthResizable, bool heightResizable) :
    QWidget(parent, flag)
{
    this->setMouseTracking(true);
    this->setAttribute(Qt::WA_Hover);
    this->installEventFilter(this);

    m_widthResizable = widthResizable;
    m_heightResizable = heightResizable;
    
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

bool AFCQMainBaseWidget::ResizeEnabled() const
{
    return m_widthResizable && m_heightResizable;
}

bool AFCQMainBaseWidget::WidthResizeEnabled() const
{
    return m_widthResizable;
}

bool AFCQMainBaseWidget::HeightResizeEnabled() const
{
    return m_heightResizable;
}

bool AFCQMainBaseWidget::MoveAllArea() const
{
    return m_moveAllArea;
}


bool AFCQMainBaseWidget::HasTitleBar() const
{
    return TitleBarHeight();
}

int AFCQMainBaseWidget::TitleBarHeight() const
{
    return m_titleBarHeight;
}

#if 0 //Rubberband
void AFCQMainBaseWidget::mouseHoverEvent(QHoverEvent* e)
{
    _UpdateCursorShape(this->mapToGlobal(e->position().toPoint()));
}

void AFCQMainBaseWidget::mouseLeaveEvent(QEvent* e)
{
    if (!m_leftButtonPressed) {
        this->unsetCursor();
    }
}

void AFCQMainBaseWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() & Qt::LeftButton) {
        m_leftButtonPressed = true;
        _CalculateCursorPosition(e->globalPosition().toPoint(), this->frameGeometry(), m_mousePressedEdge);
        m_currentPoint = e->pos();
        if (!m_mousePressedEdge.testFlag(Edge::None)) {
            m_rubberband->setGeometry(this->frameGeometry());
        } else {
            m_maximumSize = maximumSize();
            m_minimumSize = minimumSize();
            if (isMaximized())
            {
                m_isFixedSizeSet = false;
            }
            else
            {
                QSize fixedSize = size();
                setFixedSize(fixedSize);
                m_isFixedSizeSet = true;
            }
        }
        if (this->rect().marginsRemoved(QMargins(borderWidth(), borderWidth(), borderWidth(), borderWidth())).contains(e->pos())) {
            m_dragStart = true;
            m_dragPosition = e->pos();
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
        m_leftButtonPressed = false;

        if(!m_dragStart)
            _AdjustWidgetSizeToScreen();

        m_dragStart = false;
        emit qsignalBaseWindowMouseRelease();
    }
}

void AFCQMainBaseWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (m_leftButtonPressed) 
    {
        // Move Window
        if (m_dragStart) 
        {
            if (isMaximized())
            {
                qslotMaximizeWindow();  
                _AdjustMaximizeDragPosition(e);
            }
            else {
                if (!m_isFixedSizeSet)
                {
                    QSize fixedSize = size();
                    setFixedSize(fixedSize);
                }

                move(e->globalPosition().toPoint() - m_currentPoint);
            }

            _SetCursorToDefault();
            return;
        }

        // Resize Window
        if (!m_mousePressedEdge.testFlag(Edge::None)) 
        {
            if (isMaximized())
            {
                if (m_mousePressedEdge == Edge::Top)
                    m_dragStart = true;
                
                return;
            }

            int left = m_rubberband->frameGeometry().left();
            int top = m_rubberband->frameGeometry().top();
            int right = m_rubberband->frameGeometry().right();
            int bottom = m_rubberband->frameGeometry().bottom();

            switch (m_mousePressedEdge) {
            case Edge::Top:
                if (!m_isFixedHeight)
                    top = e->globalPosition().toPoint().y();
                break;
            case Edge::Bottom:
                if (!m_isFixedHeight)
                    bottom = e->globalPosition().toPoint().y();
                break;
            case Edge::Left:
                if (!m_isFixedWidth)
                    left = e->globalPosition().toPoint().x();
                break;
            case Edge::Right:
                if (!m_isFixedWidth)
                    right = e->globalPosition().toPoint().x();
                break;
            case Edge::TopLeft:
                if (!m_isFixedWidth && !m_isFixedHeight) {
                    top = e->globalPosition().toPoint().y();
                    left = e->globalPosition().toPoint().x();
                }
                break;
            case Edge::TopRight:
                if (!m_isFixedWidth && !m_isFixedHeight) {
                    right = e->globalPosition().toPoint().x();
                    top = e->globalPosition().toPoint().y();
                }
                break;
            case Edge::BottomLeft:
                if (!m_isFixedWidth && !m_isFixedHeight) {
                    bottom = e->globalPosition().toPoint().y();
                    left = e->globalPosition().toPoint().x();
                }
                break;
            case Edge::BottomRight:
                if (!m_isFixedWidth && !m_isFixedHeight) {
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
            m_rubberband->setGeometry(QRect(QPoint(left, top), QPoint(right, bottom)));
        }
    }
    else {
        _UpdateCursorShape(e->globalPosition().toPoint());
    }
}


void AFCQMainBaseWidget::closeEvent(QCloseEvent* event)
{
    emit qsignalCloseTriggered();
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
#endif

bool AFCQMainBaseWidget::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
#ifdef _WIN32
    PMSG msg = (PMSG)message;
    switch (msg->message) {
    case WM_NCCALCSIZE:
        if (msg->wParam)
            return wmNcCalcSize(result, msg->hwnd, msg->lParam, isWidgetFullscreen(const_cast<AFCQMainBaseWidget*>(this)));
        break;
    case WM_NCHITTEST:
        return wmNcHitTest(result, this, msg->hwnd, msg->message, msg->wParam, msg->lParam);
    case WM_CREATE:
        return wmCreate(msg->hwnd);
    case WM_ACTIVATE:
        return wmActivate(result, const_cast<AFCQMainBaseWidget*>(this), msg->hwnd, msg->message, msg->wParam, msg->lParam);
    case WM_GETDPISCALEDSIZE:
        return true;
    default:
        break;
    }
#endif
    return false;
}

#if 0 //Rubberband
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
        if (m_cursorChanged) {
            this->unsetCursor();
        }
        return;
    }
    if (!m_leftButtonPressed) {
        _CalculateCursorPosition(pos, this->frameGeometry(), m_mouseMoveEdge);
        m_cursorChanged = true;
        if (m_mouseMoveEdge.testFlag(Edge::Top) || m_mouseMoveEdge.testFlag(Edge::Bottom)) {
            if (!m_isFixedHeight)
                this->setCursor(Qt::SizeVerCursor);
        }
        else if (m_mouseMoveEdge.testFlag(Edge::Left) || m_mouseMoveEdge.testFlag(Edge::Right)) {
            if (!m_isFixedWidth)
                this->setCursor(Qt::SizeHorCursor);
        }
        else if (m_mouseMoveEdge.testFlag(Edge::TopLeft) || m_mouseMoveEdge.testFlag(Edge::BottomRight)) {
            if (!m_isFixedWidth && !m_isFixedHeight)
                this->setCursor(Qt::SizeFDiagCursor);
        }
        else if (m_mouseMoveEdge.testFlag(Edge::TopRight) || m_mouseMoveEdge.testFlag(Edge::BottomLeft)) {
            if (!m_isFixedWidth && !m_isFixedHeight)
                this->setCursor(Qt::SizeBDiagCursor);
        }
        else if (m_cursorChanged) {
            this->unsetCursor();
            m_cursorChanged = false;
        }
    }
}

void AFCQMainBaseWidget::_CalculateCursorPosition(const QPoint& pos, const QRect& framerect, Edges& _edge)
{
    bool onLeft = pos.x() >= framerect.x() - m_borderWidth && pos.x() <= framerect.x() + m_borderWidth &&
        pos.y() <= framerect.y() + framerect.height() - m_borderWidth  && pos.y() >= framerect.y() + m_borderWidth;

    bool onRight = pos.x() >= framerect.x() + framerect.width() - m_borderWidth && pos.x() <= framerect.x() + framerect.width() &&
        pos.y() >= framerect.y() + m_borderWidth && pos.y() <= framerect.y() + framerect.height() - m_borderWidth;

    bool onBottom = pos.x() >= framerect.x() + m_borderWidth && pos.x() <= framerect.x() + framerect.width() - m_borderWidth &&
        pos.y() >= framerect.y() + framerect.height() - m_borderWidth && pos.y() <= framerect.y() + framerect.height();

    bool onTop = pos.x() >= framerect.x() + m_borderWidth && pos.x() <= framerect.x() + framerect.width() - m_borderWidth &&
        pos.y() >= framerect.y() && pos.y() <= framerect.y() + m_borderWidth;

    bool  onBottomLeft = pos.x() <= framerect.x() + m_borderWidth && pos.x() >= framerect.x() &&
        pos.y() <= framerect.y() + framerect.height() && pos.y() >= framerect.y() + framerect.height() - m_borderWidth;

    bool onBottomRight = pos.x() >= framerect.x() + framerect.width() - m_borderWidth && pos.x() <= framerect.x() + framerect.width() &&
        pos.y() >= framerect.y() + framerect.height() - m_borderWidth && pos.y() <= framerect.y() + framerect.height();

    bool onTopRight = pos.x() >= framerect.x() + framerect.width() - m_borderWidth && pos.x() <= framerect.x() + framerect.width() &&
        pos.y() >= framerect.y() && pos.y() <= framerect.y() + m_borderWidth;

    bool onTopLeft = pos.x() >= framerect.x() && pos.x() <= framerect.x() + m_borderWidth &&
        pos.y() >= framerect.y() && pos.y() <= framerect.y() + m_borderWidth;

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
    if (!(m_mousePressedEdge == Edge::Bottom ||
        m_mousePressedEdge == Edge::BottomLeft ||
        m_mousePressedEdge == Edge::BottomRight))
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
        m_rubberband->setGeometry(QRect(QPoint(widgetLeft, screenTop), QPoint(widgetRight, widgetBottom)));
    }

    if (widgetBottom > screenBottom)
    {
        this->setGeometry(QRect(QPoint(widgetLeft, widgetTop), QPoint(widgetRight, screenBottom)));
        m_rubberband->setGeometry(QRect(QPoint(widgetLeft, widgetTop), QPoint(widgetRight, screenBottom)));
    }
}


void AFCQMainBaseWidget::_AdjustMaximizeDragPosition(QMouseEvent* event)
{
#define POPUP_TITLE_HEIGHT  40

    int width = ( -1 != m_restoreNormalWidth ? m_restoreNormalWidth : this->width());

    QPoint newPos = event->globalPosition().toPoint();
    int newX = newPos.x() - width / 2;
    int newY = newPos.y() - POPUP_TITLE_HEIGHT / 2;
    move(newX, newY);
    m_currentPoint.setX(width / 2);
    m_currentPoint.setY(POPUP_TITLE_HEIGHT / 2);
}
#endif
