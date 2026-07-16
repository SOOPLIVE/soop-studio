#include "CTopBaseWindow.h"

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

template<typename T>
static bool isMaximizedOrFullScreen(AFTTopBaseWindow<T>* widget)
{
    if (auto state = widget->windowState(); state.testFlag(Qt::WindowMaximized) || state.testFlag(Qt::WindowFullScreen))
        return true;
    return false;
}

template<typename T>
static RECT titlebarRect(HWND handle, AFTTopBaseWindow<T>* widget)
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

template<typename T>
static bool wmNcHitTest(qintptr* result, AFTTopBaseWindow<T>* widget, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
            *result = (hit == HTLEFT || hit == HTRIGHT) ? hit : (widget->MoveInAllArea() ? HTCAPTION : HTCLIENT);
        }
        else if (widget->HeightResizeEnabled())
        {
            *result = (hit == HTTOP || hit == HTBOTTOM) ? hit : (widget->MoveInAllArea() ? HTCAPTION : HTCLIENT);
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
        else if (widget->HasTitleBar() || widget->MoveInAllArea())
        {
            *result = HTCAPTION;
        }
        else
        {
            *result = HTCLIENT;
        }
        return true;
    }

    if (widget->MoveInAllArea())
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

            bool Caption = pushwidget->property("MoveInAllArea").toBool();

            if (!Caption && widgetGeo.contains(adjustedPos))
            {
                *result = HTCLIENT;
                return true;
            }
        }
        *result = HTCAPTION;
    }
    else
        *result = HTCLIENT;
    
    return true;
}

static bool wmCreate(HWND hwnd)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);
    SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
    return false;
}
template<typename T>
static bool wmActivate(qintptr* result, AFTTopBaseWindow<T>* widget, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

template<>
bool AFTTopBaseWindow<QWidget>::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
#ifdef _WIN32
    PMSG msg = (PMSG)message;

    switch (msg->message) {
    case WM_NCCALCSIZE:
        if (msg->wParam)
            return wmNcCalcSize(result, msg->hwnd, msg->lParam, isWidgetFullscreen(const_cast<AFTTopBaseWindow<QWidget>*>(this)));
        break;
    case WM_NCHITTEST:
        return wmNcHitTest(result, this, msg->hwnd, msg->message, msg->wParam, msg->lParam);
    case WM_CREATE:
        return wmCreate(msg->hwnd);
    case WM_ACTIVATE:
        return wmActivate(result, const_cast<AFTTopBaseWindow<QWidget>*>(this), msg->hwnd, msg->message, msg->wParam, msg->lParam);
    case WM_GETDPISCALEDSIZE:
        return true;
    case WM_SIZE:
        switch (msg->wParam)
        {
        case SIZE_MINIMIZED:
            m_wasMinimize = true;
        }
        return false;
    case WM_SYSCOMMAND:
        if ((msg->wParam & 0xFFF0) == SC_MAXIMIZE)
            m_aboutToMaximize = true;
        else if ((msg->wParam & 0xFFF0) == SC_RESTORE)
            if (m_wasMinimize)
            {
                m_wasMinimize = false;
                m_aboutToMaximize = false;
            }
            else
                m_aboutToMaximize = true;
        else if ((msg->wParam & 0xFFF0) == SC_MINIMIZE)
            m_wasMinimize = true;
        return QWidget::nativeEvent(eventType, message, result);
    default:
        break;
    }
#endif
    return false;
}

template<>
void AFTTopBaseWindow<QWidget>::changeEvent(QEvent* event) 
{
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent* stateEvent = static_cast<QWindowStateChangeEvent*>(event);

        if (this->windowState() & Qt::WindowMaximized) {
            if (m_pController)
                m_pController->CallSignalMaximized(true);
        }
        else if (stateEvent->oldState() & Qt::WindowMaximized) {
            if (m_pController)
                m_pController->CallSignalMaximized(false);
        }
    }

    QWidget::changeEvent(event);
}

template<>
bool AFTTopBaseDialog::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
#ifdef _WIN32
    PMSG msg = (PMSG)message;
    switch (msg->message) {
    case WM_NCCALCSIZE:
        if (msg->wParam)
            return wmNcCalcSize(result, msg->hwnd, msg->lParam, isWidgetFullscreen(const_cast<AFTTopBaseDialog*>(this)));
        break;
    case WM_NCHITTEST:
        return wmNcHitTest(result, this, msg->hwnd, msg->message, msg->wParam, msg->lParam);
    case WM_CREATE:
        return wmCreate(msg->hwnd);
    case WM_ACTIVATE:
        return wmActivate(result, const_cast<AFTTopBaseDialog*>(this), msg->hwnd, msg->message, msg->wParam, msg->lParam);
    case WM_GETDPISCALEDSIZE:
        return true;
    default:
        break;
    }
#endif
    return false;
}

template<>
void AFTTopBaseDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent* stateEvent = static_cast<QWindowStateChangeEvent*>(event);

        if (this->windowState() & Qt::WindowMaximized) {
            if (m_pController)
                m_pController->CallSignalMaximized(true);
        }
        else if (stateEvent->oldState() & Qt::WindowMaximized) {
            if (m_pController)
                m_pController->CallSignalMaximized(false);
        }
    }

    QDialog::changeEvent(event);
}

void AFQBaseWindowController::ExitSizeMoveSignalTrigger()
{
    emit qsignalExitSizeMove();
}

void AFQBaseWindowController::CallSignalMaximized(bool maximized) 
{
    emit qsignalMaximized(maximized);
}
