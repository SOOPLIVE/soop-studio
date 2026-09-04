/*
 * Window Area Capture UI
 *
 * WindowCaptureAreaWidget
 * - A nearly transparent selection window that covers the entire monitor.
 * - Handles mouse input and draws guide lines, selection borders, and text.
 *
 * A full-screen DimOverlayWindow darkens the monitor.
 * The selected area is removed from the overlay using a QRegion mask,
 * allowing the underlying screen to remain visible in real time.
 *
 *   ┌──────────────────────────────┐
 *   │██████████████████████████████│
 *   │████┌──────────────┐██████████│
 *   │████│    CLEAR     │██████████│
 *   │████└──────────────┘██████████│
 *   │██████████████████████████████│
 *   └──────────────────────────────┘
 */

#include "CWindowCaptureAreaWidget.h"

#include <set>
#include <vector>
#include <utility>

#include <QGuiApplication>
#include <QPalette>
#include <QRegion>
#include <QWindow>

#include <dwmapi.h>
#include <psapi.h>

#include "obs-studio/libobs/util/windows/window-helpers.h"

#pragma comment(lib, "dwmapi.lib")

struct WinApiMonitorInfo
{
    HMONITOR hMonitor;
    RECT rcMonitor;
    std::wstring deviceName;
    int enumOrderIndex;
};

namespace {
    constexpr qreal DIM_OVERLAY_OPACITY = 130.0 / 255.0;

    class DimOverlayWindow final : public QWidget
    {
    public:
        DimOverlayWindow() : QWidget(nullptr)
        {
            setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint |
                Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus |
                Qt::NoDropShadowWindowHint);

            setAttribute(Qt::WA_ShowWithoutActivating);

            QPalette palette = this->palette();
            palette.setColor(QPalette::Window, Qt::black);
            setPalette(palette);

            setAutoFillBackground(true);
            setWindowOpacity(DIM_OVERLAY_OPACITY);
        }
    };

    static QRegion MakeBorderRegion(const QRect& rect, int thickness = 3)
    {
        if (!rect.isValid() || rect.isEmpty()) {
            return QRegion();
        }

        const QRect outer = rect.adjusted(-thickness, -thickness, thickness, thickness);
        const QRect inner = rect.adjusted(thickness, thickness, -thickness, -thickness);
        QRegion region(outer);
        if (inner.isValid() && !inner.isEmpty()) {
            region -= QRegion(inner);
        }

        return region;
    }
} // namespace


static BOOL CALLBACK MonitorEnumProcCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData)
{
    auto* monitorList = reinterpret_cast<std::vector<WinApiMonitorInfo>*>(dwData);
    if (!monitorList)
        return FALSE;

    MONITORINFOEXW miex = {};
    miex.cbSize = sizeof(miex);
    if (GetMonitorInfoW(hMonitor, &miex)) {
        monitorList->push_back({
            hMonitor,
            miex.rcMonitor,
            std::wstring(miex.szDevice),
            static_cast<int>(monitorList->size()) });
    }

    return TRUE;
}

struct EnumWindowsCallbackData
{
    QVector<window_area_info>* pVecWindowAreaInfo;
    std::set<HWND>* pAddedHwndsSet;
};

static BOOL CALLBACK EnumWindowsProcToAddDistinctWindows(HWND hwnd, LPARAM lParam)
{
    auto* data = reinterpret_cast<EnumWindowsCallbackData*>(lParam);
    if (!data)
        return TRUE;

    if (data->pAddedHwndsSet->count(hwnd) || !IsWindowVisible(hwnd) || !IsWindowEnabled(hwnd)) {
        return TRUE;
    }

    char className[256] = {};

    GetClassNameA(hwnd, className, sizeof(className));
    std::string classNameString(className);
    if (classNameString == "CabinetWClass" || classNameString == "ExploreWClass") {
        char windowTitle[512] = {};

        GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
        std::string title(windowTitle);
        if (!title.empty()) {
            std::string exeName = "explorer.exe";
            std::string generatedId = "[" + exeName + "]:[" + title + "]:[" + classNameString + "]";
            data->pVecWindowAreaInfo->append({
                hwnd,
                generatedId,
                title,
                classNameString,
                exeName });
            data->pAddedHwndsSet->insert(hwnd);
        }
    }

    return TRUE;
}

static inline QRect PhysicalRectToLogical(const RECT& physicalRect,
    QScreen* targetScreen,
    const POINT& physicalMousePoint)
{
    if (!targetScreen)
        return QRect();

    HMONITOR hMonitor = MonitorFromPoint(physicalMousePoint, MONITOR_DEFAULTTONEAREST);
    if (!hMonitor)
        return QRect();

    MONITORINFOEXW monitorInfo = {};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfoW(hMonitor, &monitorInfo))
        return QRect();

    const qreal dpr = targetScreen->devicePixelRatio();
    if (dpr <= 0.0)
        return QRect();

    const QRect logicalScreenRect = targetScreen->geometry();
    const RECT& physicalScreenRect = monitorInfo.rcMonitor;

    const int logicalX =
        logicalScreenRect.left() + qRound((physicalRect.left - physicalScreenRect.left) / dpr);
    const int logicalY =
        logicalScreenRect.top() + qRound((physicalRect.top - physicalScreenRect.top) / dpr);
    const int logicalWidth =
        qRound((physicalRect.right - physicalRect.left) / dpr);
    const int logicalHeight =
        qRound((physicalRect.bottom - physicalRect.top) / dpr);

    return QRect(logicalX, logicalY, logicalWidth, logicalHeight);
}

static inline bool GetClientScreenRect(HWND hwnd, RECT* rect)
{
    if (!IsWindow(hwnd) || !rect)
        return false;

    if (!GetClientRect(hwnd, rect))
        return false;

    POINT topLeft = { rect->left, rect->top };
    POINT bottomRight = { rect->right, rect->bottom };

    if (!ClientToScreen(hwnd, &topLeft) || !ClientToScreen(hwnd, &bottomRight)) {
        return false;
    }

    rect->left = topLeft.x;
    rect->top = topLeft.y;
    rect->right = bottomRight.x;
    rect->bottom = bottomRight.y;

    return true;
}

static const char* wgc_partial_match_classes[] = {
    "Chrome",
    "Mozilla",
    nullptr,
};

static const char* wgc_whole_match_classes[] = {
    "ApplicationFrameWindow",
    "Windows.UI.Core.CoreWindow",
    "GAMINGSERVICESUI_HOSTING_WINDOW_CLASS",
    "XLMAIN",            // Microsoft Excel
    "PPTFrameClass",     // Microsoft PowerPoint
    "screenClass",       // PowerPoint Slide Show
    "PodiumParent",      // PowerPoint Presenter View
    "OpusApp",           // Microsoft Word
    "OMain",             // Microsoft Access
    "Framework::CFrame", // Microsoft OneNote
    "rctrl_renwnd32",    // Microsoft Outlook
    "MSWinPub",          // Microsoft Publisher
    "OfficeApp-Frame",   // Microsoft 365
    "SDL_app",
    "MSPaintApp",
    nullptr,
};

static const char* wgc_match_process[] = {
    "starcraft.exe",
    "league of legends.exe",
    "7daystodie.exe",
    "among us.exe",
    "steam.exe",
    "goose goose duck.exe",
    "fallguys_client_game.exe", "geegee.exe",
    nullptr,
};

enum window_area_capture_method
{
    METHOD_AUTO,
    METHOD_BITBLT,
    METHOD_WGC,
};

static enum window_area_capture_method choose_method(const char* currentClass, const char* currentExe)
{
    if (!currentClass || !currentExe) {
        return METHOD_BITBLT;
    }

    const char** matchClass = wgc_partial_match_classes;
    while (*matchClass) {
        if (astrstri(currentClass, *matchClass) != nullptr) {
            return METHOD_WGC;
        }
        ++matchClass;
    }

    matchClass = wgc_whole_match_classes;
    while (*matchClass) {
        if (astrcmpi(currentClass, *matchClass) == 0) {
            return METHOD_WGC;
        }
        ++matchClass;
    }

    const char** process = wgc_match_process;
    while (*process) {
        if (astrcmpi(currentExe, *process) == 0) {
            return METHOD_WGC;
        }
        ++process;
    }

    return METHOD_BITBLT;
}

static bool IsModernPaint(HANDLE process)
{
    char path[MAX_PATH] = {};

    if (GetModuleFileNameExA(process, nullptr, path, MAX_PATH)) {
        std::string fullPath(path);
        if (fullPath.find("WindowsApps") != std::string::npos) {
            return true;
        }
    }

    return false;
}

static HANDLE GetProcessHandleFromHwnd(HWND hwnd)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0)
        return nullptr;

    return OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
}

QList<QPointer<WindowCaptureAreaWidget>> WindowCaptureAreaWidget::activeInstances;

void WindowCaptureAreaWidget::launch(obs_source_t* source,
    QWidget* parent,
    WindowSelectedCallback callback)
{
    auto instances = std::move(activeInstances);
    activeInstances.clear();

    for (auto& instance : instances) {
        if (!instance)
            continue;

        instance->hide();
        instance->deleteLater();
    }

    std::unique_ptr<obs_properties_t, decltype(&obs_properties_destroy)> properties(
        source
        ? obs_source_properties(source)
        : obs_get_source_properties("window_capture"),
        obs_properties_destroy);

    if (!properties)
        return;

    int priority = 0;
    OBSDataAutoRelease settings = nullptr;
    if (source) {
        settings = obs_source_get_settings(source);
        priority = obs_data_get_int(settings, "priority");
    }

    obs_property_t* property = obs_properties_get(properties.get(), "window");
    if (!property)
        return;

    QVector<window_area_info> allWindowsInfo;
    char* className = nullptr;
    char* title = nullptr;
    char* executable = nullptr;
    std::set<HWND> addedWindows;
    const size_t count = obs_property_list_item_count(property);
    for (size_t i = 0; i < count; ++i) {
        const std::string name = obs_property_list_item_name(property, i);
        const char* value = obs_property_list_item_string(property, i);
        const std::string id = value ? value : "";

        bfree(className);
        bfree(title);
        bfree(executable);

        className = nullptr;
        title = nullptr;
        executable = nullptr;

        ms_build_window_strings(id.c_str(), &className, &title, &executable);
        HWND window =
            ms_find_window(INCLUDE_MINIMIZED,
                static_cast<window_priority>(priority),
                className,
                title,
                executable);

        if (!window)
            continue;

        if (addedWindows.count(window)) {
            continue;
        }

        allWindowsInfo.append({ window, id, name, className ? className : "", executable ? executable : "" });
        addedWindows.insert(window);
    }

    bfree(className);
    bfree(title);
    bfree(executable);

    EnumWindowsCallbackData enumData = { &allWindowsInfo, &addedWindows };

    EnumWindows(EnumWindowsProcToAddDistinctWindows, reinterpret_cast<LPARAM>(&enumData));

    EscKeyFilter* keyFilter = new EscKeyFilter(qApp);
    qApp->installEventFilter(keyFilter);

    for (QScreen* screen : QGuiApplication::screens()) {
        if (!screen)
            continue;

        auto* widget = new WindowCaptureAreaWidget(screen, allWindowsInfo, source, parent, callback);
        activeInstances.append(widget);

        connect(widget, &WindowCaptureAreaWidget::selectionOperationFinished,
            qApp, [keyFilter]() {

                qApp->removeEventFilter(keyFilter);
                keyFilter->deleteLater();

                auto instances = std::move(activeInstances);
                activeInstances.clear();

                for (auto& instance : instances) {
                    if (!instance)
                        continue;
                    instance->hide();
                    instance->deleteLater();
                }
            });
        widget->show();
    }

    if (activeInstances.isEmpty()) {
        qApp->removeEventFilter(keyFilter);
        keyFilter->deleteLater();
        return;
    }

    connect(keyFilter, &EscKeyFilter::escapeKeyPressed, activeInstances.first(),
        &WindowCaptureAreaWidget::selectionOperationFinished);
}

void WindowCaptureAreaWidget::ApplyResult(obs_data_t* settings, const WindowCaptureAreaResult& result)
{
    if (!settings)
        return;

    obs_data_set_bool(settings, "desktop_monitor", result.desktopMonitor);
    if (result.desktopMonitor) {
        obs_data_set_int(settings, "monitor", result.monitor);
    }
    else {
        obs_data_set_string(settings, "window", result.windowId.c_str());
    }

    obs_data_set_bool(settings, "use_subregion", result.useSubregion);

    if (!result.useSubregion)
        return;

    obs_data_set_int(settings, "subregion_x", result.subregionX);
    obs_data_set_int(settings, "subregion_y", result.subregionY);
    obs_data_set_int(settings, "subregion_width", result.subregionWidth);
    obs_data_set_int(settings, "subregion_height", result.subregionHeight);
}

WindowCaptureAreaWidget::WindowCaptureAreaWidget(QScreen* screen_,
    const QVector<window_area_info>& allWindowsInfo,
    obs_source_t* source,
    QWidget* parent,
    WindowSelectedCallback callback)
    : QWidget(parent)
    , weakSource(source ? OBSGetWeakRef(source) : nullptr)
    , props(nullptr, obs_properties_destroy)
    , screen(screen_)
    , windowAreaInfos(allWindowsInfo)
    , selectionCallback(std::move(callback))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    //setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setMouseTracking(true);

    dimOverlay = new DimOverlayWindow();

    if (screen) {
        // Selection window
        winId();

        if (windowHandle())
            windowHandle()->setScreen(screen);

        setGeometry(screen->geometry());

        // Dim window
        dimOverlay->winId();

        if (dimOverlay->windowHandle())
            dimOverlay->windowHandle()->setScreen(screen);

        dimOverlay->setGeometry(screen->geometry());
    }
}

WindowCaptureAreaWidget::~WindowCaptureAreaWidget()
{
    if (dimOverlay) {
        dimOverlay->hide();
        delete dimOverlay;
        dimOverlay = nullptr;
    }
}

void WindowCaptureAreaWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    UpdateDimOverlay();
    RaiseSelectionWindow();
}

void WindowCaptureAreaWidget::hideEvent(QHideEvent* event)
{
    if (dimOverlay) {
        dimOverlay->hide();
    }

    QWidget::hideEvent(event);
}

void WindowCaptureAreaWidget::UpdateDimOverlay()
{
    if (!screen || !dimOverlay)
        return;

    const QRect screenRect = screen->geometry();

    QRect clearRect;
    if (dragging && currentDragRect.isValid())
        clearRect = currentDragRect;
    else if (!highlightedRect.isEmpty())
        clearRect = highlightedRect;

    clearRect = clearRect.intersected(screenRect);

    if (!isVisible()) {
        dimOverlay->hide();
        return;
    }

    const QRect localScreenRect(QPoint(0, 0), screenRect.size());

    QRegion dimRegion;

    if (clearRect.isEmpty()) {
        dimRegion = QRegion(localScreenRect);
    }
    else {
        const QRect localClearRect = clearRect.translated(-screenRect.topLeft());

        dimRegion = QRegion(localScreenRect);
        dimRegion -= QRegion(localClearRect);
    }

    if (dimRegion.isEmpty()) {
        if (dimOverlay->isVisible())
            dimOverlay->hide();

        RaiseSelectionWindow();
        return;
    }

    dimOverlay->setMask(dimRegion);

    if (!dimOverlay->isVisible())
        dimOverlay->show();

    RaiseSelectionWindow();
}

void WindowCaptureAreaWidget::RaiseSelectionWindow()
{
    HWND hwnd = reinterpret_cast<HWND>(winId());
    if (!hwnd)
        return;

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

int WindowCaptureAreaWidget::GetCurrentMonitorIndex()
{
    POINT physicalMousePoint = {};

    GetCursorPos(&physicalMousePoint);
    std::vector<WinApiMonitorInfo> monitors;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProcCallback, reinterpret_cast<LPARAM>(&monitors));
    HMONITOR hMonitor = MonitorFromPoint(physicalMousePoint, MONITOR_DEFAULTTONEAREST);

    if (!hMonitor)
        return 0;

    for (const auto& monitor : monitors) {
        if (monitor.hMonitor == hMonitor) {
            return monitor.enumOrderIndex;
        }
    }

    return 0;
}

void WindowCaptureAreaWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    //painter.setClipRegion(event->region());
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), QColor(0, 0, 0, 1));
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    const QPoint widgetTopLeftGlobal = geometry().topLeft();
    if (!highlightedRect.isEmpty()) {
        const QRect localHighlightRect = highlightedRect.adjusted(-1, -1, 1, 1).translated(-widgetTopLeftGlobal);
        QPen highlightPen(QColor(1, 130, 255, 254));
        highlightPen.setWidth(2);
        painter.setPen(highlightPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(localHighlightRect);
    }

    if (dragging && currentDragRect.isValid()) {
        const QRect localDragRect = currentDragRect.adjusted(-1, -1, 1, 1).translated(-widgetTopLeftGlobal);
        QPen dragPen(QColor(1, 130, 255, 254));
        dragPen.setWidth(2);
        dragPen.setStyle(Qt::DotLine);
        painter.setPen(dragPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(localDragRect);
    }

    const QPoint localMousePos = mapFromGlobal(mousePos);
    if (mouseInside) {
        QPen guidePen(QColor(1, 130, 255, 254));
        guidePen.setWidth(1);
        painter.setPen(guidePen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(0, localMousePos.y(), width() - 1, localMousePos.y());
        painter.drawLine(localMousePos.x(), 0, localMousePos.x(), height() - 1);
    }

    if (!dragRectSizeString.isEmpty()) {
        constexpr int xOffset = 15;
        constexpr int yOffset = 10;
        constexpr int padding = 5;

        const QPoint anchor = localMousePos + QPoint(xOffset, yOffset);
        QFontMetrics fm(painter.font());
        const QRect textBound = fm.boundingRect(dragRectSizeString);
        QRectF backgroundBox(anchor.x(), anchor.y(),
            textBound.width() + padding * 2,
            fm.height() + padding * 2);

        if (backgroundBox.right() > width()) {
            backgroundBox.moveRight(width() - 1);
        }

        if (backgroundBox.bottom() > height()) {
            backgroundBox.moveBottom(height() - 1);
        }

        if (backgroundBox.left() < 0)
            backgroundBox.moveLeft(0);
        if (backgroundBox.top() < 0)
            backgroundBox.moveTop(0);

        const QPointF textPosition(backgroundBox.left() + padding,
            backgroundBox.top() + padding + fm.ascent());

        painter.setBrush(QColor(0, 130, 254, 150));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(backgroundBox, 4.0, 4.0);
        painter.setPen(Qt::white);
        painter.drawText(textPosition, dragRectSizeString);
        painter.setBrush(Qt::NoBrush);
    }

    bool shouldDrawHighlightLabel = currentTargetIdx != -1 && !highlightedRect.isEmpty();
    QString highlightLabelText;
    if (shouldDrawHighlightLabel) {
        if (currentTargetIdx == -99) {
            QScreen* screenForLabel = QGuiApplication::screenAt(highlightedRect.center());
            QString screenName = screenForLabel ? screenForLabel->name() : QStringLiteral("Desktop");
            if (screenForLabel && screenName.isEmpty()) {
                const QList<QScreen*> screens = QGuiApplication::screens();
                const int screenIndex = screens.indexOf(screenForLabel);
                screenName = screenIndex != -1 ? QStringLiteral("Screen %1").arg(screenIndex + 1)
                    : QStringLiteral("Unknown Screen");
            }

            highlightLabelText = QStringLiteral("Desktop: %1 (%2x%3)")
                .arg(screenName)
                .arg(highlightedRect.width())
                .arg(highlightedRect.height());
        }

        else if (currentTargetIdx >= 0 && currentTargetIdx < windowAreaInfos.size()) {
            const auto& info = windowAreaInfos.at(currentTargetIdx);
            highlightLabelText = QStringLiteral("%1\n(%2x%3)")
                .arg(QString::fromStdString(info.name))
                .arg(highlightedRect.width())
                .arg(highlightedRect.height());
        }
        else {
            shouldDrawHighlightLabel = false;
        }
    }

    if (shouldDrawHighlightLabel && !highlightLabelText.isEmpty()) {

        QFontMetrics fm(painter.font());
        constexpr int padding = 10;

        const QRect textRect = fm.boundingRect(QRect(0, 0, width() / 3, 500),
            Qt::AlignCenter | Qt::TextWordWrap, highlightLabelText);

        const QSize labelSize(textRect.width() + padding * 2,
            textRect.height() + padding * 2);

        const QPoint center = highlightedRect.center();
        const QPoint globalTopLeft(center.x() - labelSize.width() / 2,
            center.y() - labelSize.height() / 2);

        const QRectF localLabelRect = QRectF(globalTopLeft, labelSize).translated(-widgetTopLeftGlobal);
        painter.setBrush(QColor(0, 0, 0, 150));
        painter.setPen(Qt::NoPen);
        painter.drawRect(localLabelRect);

        QPen borderPen(QColor(1, 130, 254));
        borderPen.setWidth(1);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(borderPen);
        painter.drawRect(localLabelRect);
        painter.setPen(Qt::white);
        painter.drawText(localLabelRect,
            Qt::AlignCenter | Qt::TextWordWrap,
            highlightLabelText);
    }
}

void WindowCaptureAreaWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        emit selectionOperationFinished();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (currentTargetIdx == -1 || highlightedRect.isEmpty()) {
        return;
    }

    if (!highlightedRect.contains(event->globalPos())) {
        return;
    }

    dragging = true;

    dragTargetRect = highlightedRect;
    dragTargetIdx = currentTargetIdx;

    dragStartPosition = event->globalPos();
    currentDragRect = QRect(dragStartPosition, dragStartPosition);

    UpdateDimOverlay();
    update();
}

static bool CanUseWindowDpiContext(HWND hwnd)
{
    if (!IsWindow(hwnd))
        return false;

    const DPI_AWARENESS_CONTEXT context = GetWindowDpiAwarenessContext(hwnd);
    if (!context)
        return false;

    DPI_AWARENESS_CONTEXT previous = SetThreadDpiAwarenessContext(context);
    if (!previous)
        return false;

    SetThreadDpiAwarenessContext(previous);
    return true;
}

static void LogWindowDpiInfo(HWND hwnd, const QRect& highlightedRect)
{
    if (!IsWindow(hwnd))
        return;

    const UINT dpi = GetDpiForWindow(hwnd);
    const DPI_AWARENESS_CONTEXT context = GetWindowDpiAwarenessContext(hwnd);
    const DPI_AWARENESS awareness = GetAwarenessFromDpiAwarenessContext(context);

    SetLastError(ERROR_SUCCESS);
    DPI_AWARENESS_CONTEXT previous = SetThreadDpiAwarenessContext(context);
    const DWORD error = GetLastError();

    RECT clientRect = {};
    GetClientRect(hwnd, &clientRect);

    if (previous)
        SetThreadDpiAwarenessContext(previous);

    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;

    const double scaleX = highlightedRect.width() > 0
        ? static_cast<double>(clientWidth) / highlightedRect.width()
        : 0.0;

    const double scaleY = highlightedRect.height() > 0
        ? static_cast<double>(clientHeight) / highlightedRect.height()
        : 0.0;

    blog(LOG_INFO,
        "DPI TEST: dpi=%u awareness=%d contextSwitch=%s error=%lu "
        "highlight=%dx%d client=%dx%d scale=%.3fx%.3f",
        dpi,
        static_cast<int>(awareness),
        previous ? "SUCCESS" : "FAILED",
        error,
        highlightedRect.width(), highlightedRect.height(),
        clientWidth, clientHeight,
        scaleX, scaleY);
}

void WindowCaptureAreaWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !dragging) {
        return;
    }

    dragging = false;
    if (currentTargetIdx == -1) {
        emit selectionOperationFinished();
        return;
    }

    const bool useSubregion =
        currentDragRect.isValid() && currentDragRect.width() > 5 && currentDragRect.height() > 5;

    WindowCaptureAreaResult result;
    result.useSubregion = useSubregion;

    int captionException = 0;
    bool useLogicalCoordinates = false;

    if (currentTargetIdx != -99) {
        result.desktopMonitor = false;
        const auto& windowInfo = windowAreaInfos.at(currentTargetIdx);
        result.windowId = windowInfo.id;

        //LogWindowDpiInfo(windowInfo.window, highlightedRect);

        if (choose_method(windowInfo.className.c_str(),
            windowInfo.executableName.c_str()) == METHOD_BITBLT) {
            useLogicalCoordinates = !CanUseWindowDpiContext(windowInfo.window);
        }

        if (useSubregion) {
            if (windowInfo.className == "MSPaintApp") {
                HANDLE paintProcess = GetProcessHandleFromHwnd(windowInfo.window);
                if (paintProcess) {
                    if (IsModernPaint(paintProcess)) {
                        RECT windowRect = {};
                        RECT clientRect = {};

                        GetClientScreenRect(windowInfo.window, &clientRect);
                        GetWindowRect(windowInfo.window, &windowRect);
                        captionException = clientRect.top - windowRect.top;
                    }

                    CloseHandle(paintProcess);
                }
            }
        }
    }

    else {
        result.desktopMonitor = true;
        result.monitor = GetCurrentMonitorIndex();
    }

    if (useSubregion) {
        QScreen* targetScreen = QGuiApplication::screenAt(highlightedRect.center());
        if (!targetScreen)
            targetScreen = screen;

        if (!targetScreen) {
            targetScreen = QGuiApplication::primaryScreen();
        }

        if (!targetScreen) {
            emit selectionOperationFinished();
            return;
        }

        const qreal dpr = useLogicalCoordinates ? 1.0 : targetScreen->devicePixelRatio();
        const int relativeX = currentDragRect.x() - highlightedRect.x();
        const int relativeY = currentDragRect.y() - highlightedRect.y();

        result.subregionX = qRound(relativeX * dpr);
        result.subregionY = qRound(relativeY * dpr) + captionException;
        result.subregionWidth = qRound(currentDragRect.width() * dpr);
        result.subregionHeight = qRound(currentDragRect.height() * dpr);
    }

    obs_source_t* source = nullptr;
    if (weakSource) {
        source = obs_weak_source_get_source(weakSource);
    }

    if (source) {
        OBSDataAutoRelease settings = obs_source_get_settings(source);
        ApplyResult(settings, result);
        obs_source_update(source, settings);
        obs_source_release(source);
    }

    if (selectionCallback) {
        selectionCallback(result);
    }

    emit selectionOperationFinished();
}

void WindowCaptureAreaWidget::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint prevMousePos = mousePos;
    const QRect prevHighlightRect = highlightedRect;
    const QRect prevDragRect = currentDragRect;
    const int prevTargetIdx = currentTargetIdx;

    mousePos = event->globalPos();
    if (dragging) {
        const bool insideDragTarget =
            dragTargetRect.isValid() && dragTargetRect.contains(mousePos);
        if (!insideDragTarget) {
            QScreen* screenUnderMouse = QGuiApplication::screenAt(mousePos);
            if (screenUnderMouse) {
                highlightedRect = screenUnderMouse->geometry();
                currentTargetIdx = -99;
            }
            else {
                highlightedRect = QRect();
                currentTargetIdx = -1;
            }
        }
        else {
            highlightedRect = dragTargetRect;
            currentTargetIdx = dragTargetIdx;
        }

        currentDragRect = QRect(dragStartPosition, mousePos).normalized();
        if (currentDragRect.isValid() && currentDragRect.width() > 0 && currentDragRect.height() > 0) {
            dragRectSizeString =
                QStringLiteral("%1 X %2").arg(currentDragRect.width()).arg(currentDragRect.height());
        }
        else {
            dragRectSizeString.clear();
        }
    }

    else {
        dragRectSizeString = QStringLiteral("%1, %2").arg(mousePos.x()).arg(mousePos.y());
        RECT windowRect = {};

        BOOL insideWindow = FALSE;
        int foundWindowIndex = -1;
        POINT physicalMousePoint = {};

        GetCursorPos(&physicalMousePoint);
        for (int i = 0; i < windowAreaInfos.size(); ++i) {
            HWND window = windowAreaInfos.at(i).window;
            if (!window || !IsWindow(window)) {
                continue;
            }

            if (GetClientScreenRect(window, &windowRect) && PtInRect(&windowRect, physicalMousePoint)) {
                insideWindow = TRUE;
                foundWindowIndex = i;
                break;
            }
        }

        if (insideWindow) {
            QScreen* targetScreen = QGuiApplication::screenAt(mousePos);
            if (!targetScreen)
                targetScreen = screen;
            highlightedRect = PhysicalRectToLogical(windowRect, targetScreen, physicalMousePoint);
            currentTargetIdx = foundWindowIndex;
        }
        else {
            highlightedRect = screen->geometry();
            currentTargetIdx = -99;
        }
    }

    UpdateDimOverlay();

    const QPoint widgetOrigin = geometry().topLeft();
    const QPoint prevLocalMouse = prevMousePos - widgetOrigin;
    const QPoint newLocalMouse = mousePos - widgetOrigin;

    QRegion dirtyRegion;
    constexpr int guideMargin = 3;
    if (!prevMousePos.isNull()) {
        dirtyRegion += QRect(0,
            prevLocalMouse.y() - guideMargin,
            width(),
            guideMargin * 2 + 1);

        dirtyRegion += QRect(prevLocalMouse.x() - guideMargin,
            0,
            guideMargin * 2 + 1,
            height());
    }

    dirtyRegion += QRect(0,
        newLocalMouse.y() - guideMargin,
        width(),
        guideMargin * 2 + 1);

    dirtyRegion += QRect(newLocalMouse.x() - guideMargin,
        0,
        guideMargin * 2 + 1,
        height());

    constexpr int infoWidth = 260;
    constexpr int infoHeight = 90;

    if (!prevMousePos.isNull()) {
        dirtyRegion += QRect(prevLocalMouse.x() - infoWidth,
            prevLocalMouse.y() - infoHeight,
            infoWidth * 2,
            infoHeight * 2);
    }

    dirtyRegion += QRect(newLocalMouse.x() - infoWidth,
        newLocalMouse.y() - infoHeight,
        infoWidth * 2,
        infoHeight * 2);

    if (prevHighlightRect != highlightedRect || prevTargetIdx != currentTargetIdx) {
        if (!prevHighlightRect.isEmpty()) {
            dirtyRegion +=
                prevHighlightRect.translated(-widgetOrigin).adjusted(-20, -20, 20, 20);
        }

        if (!highlightedRect.isEmpty()) {
            dirtyRegion +=
                highlightedRect.translated(-widgetOrigin).adjusted(-20, -20, 20, 20);
        }
    }

    if (prevDragRect != currentDragRect) {
        if (!prevDragRect.isEmpty()) {
            const QRect prevLocalDrag = prevDragRect.translated(-widgetOrigin);
            dirtyRegion += MakeBorderRegion(prevLocalDrag, 4);
        }

        if (!currentDragRect.isEmpty()) {
            const QRect newLocalDrag = currentDragRect.translated(-widgetOrigin);
            dirtyRegion += MakeBorderRegion(newLocalDrag, 4);
        }
    }

    dirtyRegion &= rect();
    if (!dirtyRegion.isEmpty()) {
        update(dirtyRegion);
    }
}

void WindowCaptureAreaWidget::enterEvent(QEnterEvent* event)
{
    mouseInside = true;
    update();

    QWidget::enterEvent(event);
}

void WindowCaptureAreaWidget::leaveEvent(QEvent* event)
{
    mouseInside = false;
    dragRectSizeString.clear();
    highlightedRect = QRect();
    currentTargetIdx = -1;

    UpdateDimOverlay();
    update();

    QWidget::leaveEvent(event);
}

void WindowCaptureAreaWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        emit selectionOperationFinished();
        return;
    }

    QWidget::keyPressEvent(event);
}