#include "CWindowCaptureAreaWidget.h"

#include <set>
// Windows API
#include <dwmapi.h>
#include <psapi.h>

#include "obs-studio/libobs/util/windows/window-helpers.h"

#pragma comment(lib, "dwmapi.lib")

struct WinApiMonitorInfo {
    HMONITOR hMonitor;
    RECT rcMonitor;
    std::wstring deviceName;
    int enumOrderIndex;
};

static BOOL CALLBACK MonitorEnumProcCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) {
    auto* monitorList = reinterpret_cast<std::vector<WinApiMonitorInfo>*>(dwData);
    if (!monitorList) return FALSE;

    MONITORINFOEXW miex;
    miex.cbSize = sizeof(miex);
    if (GetMonitorInfoW(hMonitor, &miex)) {
        monitorList->push_back({ hMonitor, miex.rcMonitor, std::wstring(miex.szDevice),
            static_cast<int>(monitorList->size()) });
    }
    return TRUE;
}

struct EnumWindowsCallbackData {
    QVector<window_area_info>* pVecWindowAreaInfo;
    std::set<HWND>* pAddedHwndsSet;
};

static BOOL CALLBACK EnumWindowsProcToAddDistinctWindows(HWND hwnd, LPARAM lParam) {
    auto* data = reinterpret_cast<EnumWindowsCallbackData*>(lParam);

    if (data->pAddedHwndsSet->count(hwnd) || !IsWindowVisible(hwnd) || !IsWindowEnabled(hwnd)) {
        return TRUE;
    }

    char className[256] = { 0, };
    GetClassNameA(hwnd, className, sizeof(className));
    std::string sClassName(className);

    if (sClassName == "CabinetWClass" || sClassName == "ExploreWClass") {
        char windowTitle[512] = { 0, };
        GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
        std::string sTitle(windowTitle);

        if (!sTitle.empty()) {
            std::string sExeName = "explorer.exe";
            std::string generated_id = "[" + sExeName + "]:[" + sTitle + "]:[" + sClassName + "]";

            data->pVecWindowAreaInfo->append({ hwnd, generated_id, sTitle });
            data->pAddedHwndsSet->insert(hwnd);
        }
    }
    return TRUE;
}

static inline QRect PhysicalRectToLogical(const RECT& physicalRect)
{
    HMONITOR hMon = MonitorFromRect(&physicalRect, MONITOR_DEFAULTTONEAREST);
    if (!hMon) {
        hMon = MonitorFromWindow((HWND)nullptr, MONITOR_DEFAULTTOPRIMARY);
        if (!hMon) return QRect();
    }

    QScreen* screenOfRect = nullptr;
    MONITORINFOEXW miex;
    miex.cbSize = sizeof(miex);
    if (GetMonitorInfoW(hMon, &miex)) {
        std::wstring targetDeviceName = miex.szDevice;
        for (QScreen* s : QGuiApplication::screens()) {
            if ((s->geometry().x() == (int)miex.rcMonitor.left) &&
                (s->geometry().y() == (int)miex.rcMonitor.top)) {
                screenOfRect = s;
                break;
            }
        }
    }

    if (!screenOfRect) {
        screenOfRect = QGuiApplication::primaryScreen();
        if (!screenOfRect) return QRect();
    }

    const qreal dpr = screenOfRect->devicePixelRatio();
    const QRect logicalScreenGeometry = screenOfRect->geometry();

    if (dpr <= 0) return QRect();

    MONITORINFO physMonInfo;
    physMonInfo.cbSize = sizeof(physMonInfo);
    if (!GetMonitorInfoA(hMon, &physMonInfo)) {
        return QRect();
    }
    const RECT& physicalMonitorRect = physMonInfo.rcMonitor;

    const long logicalWidth = qRound((physicalRect.right - physicalRect.left) / dpr);
    const long logicalHeight = qRound((physicalRect.bottom - physicalRect.top) / dpr);

    const long logicalLeft = logicalScreenGeometry.left() + qRound((physicalRect.left - physicalMonitorRect.left) / dpr);
    const long logicalTop = logicalScreenGeometry.top() + qRound((physicalRect.top - physicalMonitorRect.top) / dpr);

    return QRect(logicalLeft, logicalTop, logicalWidth, logicalHeight);
}

static inline bool GetClientScreenRect(HWND hwnd, RECT* pRect)
{
    if (!IsWindow(hwnd) || pRect == nullptr) {
        return false;
    }
    if (!GetClientRect(hwnd, pRect)) {
        return false;
    }
    POINT topLeft = { pRect->left, pRect->top };       // {0, 0}
    POINT bottomRight = { pRect->right, pRect->bottom }; // {width, height}

    if (!ClientToScreen(hwnd, &topLeft) || !ClientToScreen(hwnd, &bottomRight)) {
        return false;
    }
    pRect->left = topLeft.x;
    pRect->top = topLeft.y;
    pRect->right = bottomRight.x;
    pRect->bottom = bottomRight.y;

    return true;
}

static const char* wgc_partial_match_classes[] = {
    "Chrome",
    "Mozilla",
    NULL,
};

static const char* wgc_whole_match_classes[] = {
    "ApplicationFrameWindow",
    "Windows.UI.Core.CoreWindow",
    "GAMINGSERVICESUI_HOSTING_WINDOW_CLASS",
    "XLMAIN",            /* Microsoft Excel */
    "PPTFrameClass",     /* Microsoft PowerPoint */
    "screenClass",       /* Microsoft PowerPoint (Slide Show) */
    "PodiumParent",      /* Microsoft PowerPoint (Presenter View) */
    "OpusApp",           /* Microsoft Word */
    "OMain",             /* Microsoft Access */
    "Framework::CFrame", /* Microsoft OneNote */
    "rctrl_renwnd32",    /* Microsoft Outlook */
    "MSWinPub",          /* Microsoft Publisher */
    "OfficeApp-Frame",   /* Microsoft 365 Software */
    "SDL_app",
    "MSPaintApp",		/* Microsoft Paint */
    NULL,
};

static const char* wgc_match_process[] = {
    "starcraft.exe",
    "league of legends.exe",
    "7daystodie.exe",
    "among us.exe",
    "steam.exe",
    "goose goose duck.exe",
    "fallguys_client_game.exe",
    "geegee.exe",
    NULL,
};

enum window_area_capture_method {
    METHOD_AUTO,
    METHOD_BITBLT,
    METHOD_WGC,
};

static enum window_area_capture_method choose_method(const char* current_class, const char* current_exe)
{
    if (!current_class || !current_exe)
        return METHOD_BITBLT;

    const char** match_class = wgc_partial_match_classes;
    while (*match_class) {
        if (astrstri(current_class, *match_class) != NULL) {
            return METHOD_WGC;
        }
        match_class++;
    }

    match_class = wgc_whole_match_classes;
    while (*match_class) {
        if (astrcmpi(current_class, *match_class) == 0) {
            return METHOD_WGC;
        }
        match_class++;
    }

    const char** process = wgc_match_process;
    while (*process) {
        if (astrcmpi(current_exe, *process) == 0) {
            return METHOD_WGC;
        }
        process++;
    }

    return METHOD_BITBLT;
}

bool IsModernPaint(HANDLE hProcess) {
    char path[MAX_PATH];
    if (GetModuleFileNameExA(hProcess, NULL, path, MAX_PATH)) {
        std::string fullPath(path);        
        if (fullPath.find("WindowsApps") != std::string::npos) {
            return true;
        }
    }
    return false;
}

HANDLE GetProcessHandleFromHwnd(HWND hwnd) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == 0) {
        return NULL;
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    return hProcess;
}

//================================================================================
// WindowCaptureAreaWidget
//================================================================================

QList<WindowCaptureAreaWidget*> WindowCaptureAreaWidget::s_activeInstances;

void WindowCaptureAreaWidget::launch(obs_source_t* source, QWidget* parent)
{
    qDeleteAll(s_activeInstances);
    s_activeInstances.clear();

    std::unique_ptr<obs_properties_t, decltype(&obs_properties_destroy)> props(obs_source_properties(source), obs_properties_destroy);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    int priority = (int)obs_data_get_int(settings, "priority");
    obs_property_t* p = obs_properties_get(props.get(), "window");

    QVector<window_area_info> all_windows_info;
    char* class_ = nullptr, * title_ = nullptr, * executable_ = nullptr;
    std::set<HWND> added_hwnds;
    size_t count = obs_property_list_item_count(p);
    for (size_t i = 0; i < count; i++) {
        std::string name = obs_property_list_item_name(p, i);
        const char* val = obs_property_list_item_string(p, i);
        std::string id = val ? val : "";
        bfree(class_); bfree(title_); bfree(executable_);
        class_ = nullptr; title_ = nullptr; executable_ = nullptr;
        ms_build_window_strings(id.c_str(), &class_, &title_, &executable_);
        HWND window = ms_find_window(INCLUDE_MINIMIZED, (window_priority)priority, class_, title_, executable_);
        if (!window) continue;
        if (window && added_hwnds.count(window)) continue;        
        all_windows_info.append({ window, id, name, class_, executable_});
        if (window) added_hwnds.insert(window);
    }
    bfree(class_); bfree(title_); bfree(executable_);

    EnumWindowsCallbackData enum_data = { &all_windows_info, &added_hwnds };
    EnumWindows(EnumWindowsProcToAddDistinctWindows, reinterpret_cast<LPARAM>(&enum_data));

    EscKeyFilter* keyFilter = new EscKeyFilter(qApp);
    qApp->installEventFilter(keyFilter);

    for (QScreen* screen : QGuiApplication::screens()) {
        if (!screen) continue;
        auto* widget = new WindowCaptureAreaWidget(screen, all_windows_info, source, parent);
        s_activeInstances.append(widget);

        connect(widget, &WindowCaptureAreaWidget::selectionOperationFinished, [keyFilter]() {

            qApp->removeEventFilter(keyFilter);
            keyFilter->deleteLater();

            qDeleteAll(s_activeInstances);
            s_activeInstances.clear();
        });

        widget->show();
    }

    if (!s_activeInstances.isEmpty()) {
        connect(keyFilter, &EscKeyFilter::escapeKeyPressed,
            s_activeInstances.first(), &WindowCaptureAreaWidget::selectionOperationFinished);
    }
}

WindowCaptureAreaWidget::WindowCaptureAreaWidget(
    QScreen* screen,
    const QVector<window_area_info>& all_windows_info,
    obs_source_t* source,
    QWidget* parent)
    : QWidget(parent)
    , m_weakSource(OBSGetWeakRef(source))
    , m_props(nullptr, obs_properties_destroy)
    , m_screen(screen)
    , m_allWindowsInfo(all_windows_info)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    setMouseTracking(true);

    if (m_screen) {
        setGeometry(m_screen->geometry());
        setWindowState(Qt::WindowFullScreen);
        m_desktopPixmap = m_screen->grabWindow(0);
    }
}

WindowCaptureAreaWidget::~WindowCaptureAreaWidget()
{    
}

void WindowCaptureAreaWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_desktopPixmap.isNull()) return;

    QRegion updateRegion = event->region();
    QRect fullWidgetRect = this->rect();
    QPoint widgetTopLeftGlobal = geometry().topLeft();

    qreal dpr = m_desktopPixmap.devicePixelRatio();
    QRect physicalSourceRectForBg(0, 0, qRound(fullWidgetRect.width() * dpr), qRound(fullWidgetRect.height() * dpr));
    painter.drawPixmap(fullWidgetRect, m_desktopPixmap, physicalSourceRectForBg);

    painter.fillRect(fullWidgetRect, QColor(0, 0, 0, 130));

    QRect localClearArea;
    if (m_isDragging && m_currentDragRect.isValid()) {
        localClearArea = m_currentDragRect.adjusted(-1, -1, 1, 1).translated(-widgetTopLeftGlobal);
    }
    else if (!m_rcHighlightedRect.isEmpty()) {
        localClearArea = m_rcHighlightedRect.adjusted(-1, -1, 1, 1).translated(-widgetTopLeftGlobal);        
    }

    QRect clearIntersection = fullWidgetRect.intersected(localClearArea);
    if (!clearIntersection.isEmpty() && updateRegion.intersects(clearIntersection)) {
        QRect logicalSourceRect(clearIntersection.topLeft(), clearIntersection.size());
        QRect physicalSourceRectForClear(
            qRound(logicalSourceRect.left() * dpr), qRound(logicalSourceRect.top() * dpr),
            qRound(logicalSourceRect.width() * dpr), qRound(logicalSourceRect.height() * dpr));
        painter.drawPixmap(clearIntersection, m_desktopPixmap, physicalSourceRectForClear);
    }

    if (!m_rcHighlightedRect.isEmpty()) {
        QRect localHighlightBorderRect = m_rcHighlightedRect.adjusted(-1, -1, 1, 1).translated(-widgetTopLeftGlobal);
        if (updateRegion.intersects(localHighlightBorderRect)) {
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            QPen highlightPen(QColor(1, 130, 255, 254));
            highlightPen.setWidth(2);
            painter.setPen(highlightPen);
            painter.drawRect(localHighlightBorderRect);
        }
    }

    if (m_isDragging && m_currentDragRect.isValid()) {
        QRect localActiveDragBorderRect = m_currentDragRect.adjusted(-1, -1, 1, 1).translated(-widgetTopLeftGlobal);
        if (updateRegion.intersects(localActiveDragBorderRect)) {
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            QPen dragRectPen(QColor(1, 130, 255, 254));
            dragRectPen.setWidth(2);
            dragRectPen.setStyle(Qt::DotLine);
            painter.setPen(dragRectPen);
            painter.drawRect(localActiveDragBorderRect);
        }
    }

    QPoint local_mouse_pos = mapFromGlobal(m_mousePos);
    if (m_bMouseInside) {
        QPen guidePen(QColor(1, 130, 255, 254));
        guidePen.setWidth(1);
        painter.setPen(guidePen);
        painter.drawLine(0, local_mouse_pos.y(), width() - 1, local_mouse_pos.y());
        painter.drawLine(local_mouse_pos.x(), 0, local_mouse_pos.x(), height() - 1);
    }

    if (!m_dragRectSizeString.isEmpty()) {
        int xOffset = 15;
        int yOffset = 10;
        QPoint boxTopLeftAnchor = local_mouse_pos + QPoint(xOffset, yOffset);
        QFont font = painter.font();
        painter.setFont(font);
        QFontMetrics fm(font);
        QRect textContentBound = fm.boundingRect(m_dragRectSizeString);
        int textWidth = textContentBound.width();
        int textHeight = fm.height();
        int padding = 5;
        QRectF backgroundBox(boxTopLeftAnchor.x(), boxTopLeftAnchor.y(),
            textWidth + 2 * padding, textHeight + 2 * padding);
        if (backgroundBox.right() > width()) { /* ... */ }
        if (backgroundBox.bottom() > height()) { /* ... */ }
        if (backgroundBox.left() < 0) { /* ... */ }
        if (backgroundBox.top() < 0) { /* ... */ }
        QPointF textDrawPos(backgroundBox.left() + padding, backgroundBox.top() + padding + fm.ascent());
        painter.setBrush(QColor(0, 130, 254, 150));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(backgroundBox, 4.0, 4.0);
        painter.setPen(Qt::white);
        painter.drawText(textDrawPos, m_dragRectSizeString);
        painter.setBrush(Qt::NoBrush);
    }

    bool should_draw_highlight_label = (m_idx != -1 && !m_rcHighlightedRect.isEmpty());
    QString highlight_label_text_to_draw;

    if (should_draw_highlight_label) {
        if (m_idx == -99) {
            QScreen* screenForLabel = QGuiApplication::screenAt(m_rcHighlightedRect.center());
            QString screenName = screenForLabel ? screenForLabel->name() : "Desktop";
            if (screenForLabel && screenName.isEmpty()) {
                QList<QScreen*> screens = QGuiApplication::screens();
                int screenIdx = screens.indexOf(screenForLabel);
                screenName = (screenIdx != -1) ? QString("Screen %1").arg(screenIdx + 1) : "Unknown Screen";
            }
            highlight_label_text_to_draw = QString("Desktop: %1 (%2x%3)")
                .arg(screenName)
                .arg(m_rcHighlightedRect.width())
                .arg(m_rcHighlightedRect.height());
        }
        else if (m_idx >= 0 && m_idx < m_allWindowsInfo.size()) {
            const auto& info = m_allWindowsInfo.at(m_idx);
            highlight_label_text_to_draw = QString("%1\n(%2x%3)")
                .arg(info.name.c_str())
                .arg(m_rcHighlightedRect.width()).arg(m_rcHighlightedRect.height());
        }
        else {
            should_draw_highlight_label = false;
        }
    }

    if (should_draw_highlight_label && !highlight_label_text_to_draw.isEmpty()) {
        QFont labelFont = painter.font();
        QFontMetrics labelFm(labelFont);

        int labelPadding = 10;
        QRect textBoundingRect = labelFm.boundingRect(QRect(0, 0, width() / 3, 500), Qt::AlignCenter | Qt::TextWordWrap, highlight_label_text_to_draw);

        QSize actualTextSize(textBoundingRect.width(), textBoundingRect.height());
        QSize paddedLabelContentSize(actualTextSize.width() + 2 * labelPadding,
            actualTextSize.height() + 2 * labelPadding);

        QPoint globalTargetRectCenter = m_rcHighlightedRect.center();

        QPoint globalLabelBoxTopLeft(
            globalTargetRectCenter.x() - paddedLabelContentSize.width() / 2,
            globalTargetRectCenter.y() - paddedLabelContentSize.height() / 2
        );

        QRectF globalLabelBackgroundBox(globalLabelBoxTopLeft, paddedLabelContentSize);

        QRectF localLabelBackgroundBox = globalLabelBackgroundBox.translated(-widgetTopLeftGlobal);
        
        painter.setBrush(QColor(0, 0, 0, 150));
        painter.setPen(Qt::NoPen);
        painter.drawRect(localLabelBackgroundBox);

        QPen labelBoxBorderPen(QColor(1, 130, 254));
        labelBoxBorderPen.setWidth(1);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(labelBoxBorderPen);
        painter.drawRect(localLabelBackgroundBox);
        painter.setPen(Qt::white);
        painter.drawText(localLabelBackgroundBox, Qt::AlignCenter | Qt::TextWordWrap, highlight_label_text_to_draw);

        painter.setBrush(Qt::NoBrush);
    }
}

void WindowCaptureAreaWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        emit selectionOperationFinished();
        return;
    }
    if (event->button() != Qt::LeftButton) return;

    if (m_idx != -1 && !m_rcHighlightedRect.isEmpty() && m_rcHighlightedRect.contains(event->globalPos())) {
        m_isDragging = true;
        m_dragStartPosition = event->globalPos();
        m_currentDragRect = QRect(m_dragStartPosition, m_dragStartPosition);
        update();
    }
}

void WindowCaptureAreaWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_isDragging) {
        m_isDragging = false;

        int captionException = 0;
        if (m_currentDragRect.isValid() && m_currentDragRect.width() > 5 && m_currentDragRect.height() > 5) {
            obs_source_t* source = obs_weak_source_get_source(m_weakSource);
            if (source && m_idx != -1) {
                OBSDataAutoRelease settings = obs_source_get_settings(source);
                bool noneDpr = false;
                if (m_idx != -99) {
                    obs_data_set_bool(settings, "desktop_monitor", false);
                    const auto& windowInfo = m_allWindowsInfo.at(m_idx);
                    obs_data_set_string(settings, "window", windowInfo.id.c_str());

                    if (windowInfo.className == "MSPaintApp") {
                        HANDLE hPaint = GetProcessHandleFromHwnd(windowInfo.window);
                        if (IsModernPaint(hPaint)) {
                            RECT rect, rcWindow;
                            GetClientScreenRect(windowInfo.window, &rcWindow);
                            GetWindowRect(windowInfo.window, &rect);
                            captionException = rcWindow.top - rect.top;
                        }
                    }
                    
                    if (METHOD_BITBLT == choose_method(windowInfo.className.c_str(), windowInfo.executableName.c_str()))
                        noneDpr = true;
                }
                else {
                    obs_data_set_bool(settings, "desktop_monitor", true);

                    POINT physical_mouse_point;
                    GetCursorPos(&physical_mouse_point);
                    
                    std::vector<WinApiMonitorInfo> winApiOrderedMonitors;
                    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProcCallback, reinterpret_cast<LPARAM>(&winApiOrderedMonitors));

                    int monitor_index = -1;
                    HMONITOR hMon = MonitorFromPoint(physical_mouse_point, MONITOR_DEFAULTTONEAREST);                    
                    if (hMon) {
                        for (const auto& winApiMonitor : winApiOrderedMonitors) {
                            if (winApiMonitor.hMonitor == hMon) {
                                monitor_index = winApiMonitor.enumOrderIndex;
                                break;
                            }
                        }
                    }
                    if (monitor_index == -1) monitor_index = 0;

                    obs_data_set_int(settings, "monitor", monitor_index);
                }
                
                QScreen* targetScreen = QGuiApplication::screenAt(m_rcHighlightedRect.center());
                if (!targetScreen) targetScreen = m_screen;
                if (!targetScreen) targetScreen = QGuiApplication::primaryScreen();

                qreal dpr = noneDpr ? 1. : targetScreen->devicePixelRatio();

                int logical_relative_x = m_currentDragRect.x() - m_rcHighlightedRect.x();
                int logical_relative_y = m_currentDragRect.y() - m_rcHighlightedRect.y();
                int logical_width = m_currentDragRect.width();
                int logical_height = m_currentDragRect.height();

                int physical_relative_x = qRound(logical_relative_x * dpr);
                int physical_relative_y = qRound(logical_relative_y * dpr);
                int physical_width = qRound(logical_width * dpr);
                int physical_height = qRound(logical_height * dpr);

                obs_data_set_bool(settings, "use_subregion", true);
                obs_data_set_int(settings, "subregion_x", physical_relative_x);
                obs_data_set_int(settings, "subregion_y", physical_relative_y + captionException);
                obs_data_set_int(settings, "subregion_width", physical_width);
                obs_data_set_int(settings, "subregion_height", physical_height);
                
                obs_source_update(source, settings);
                obs_source_release(source);
            }
            emit selectionOperationFinished();
        }
        else {
            obs_source_t* source = obs_weak_source_get_source(m_weakSource);
            if (source && m_idx != -1) {
                OBSDataAutoRelease settings = obs_source_get_settings(source);
                if (m_idx != -99) {
                    obs_data_set_bool(settings, "desktop_monitor", false);
                    const auto& windowInfo = m_allWindowsInfo.at(m_idx);
                    obs_data_set_string(settings, "window", windowInfo.id.c_str());
                }
                else {
                    obs_data_set_bool(settings, "desktop_monitor", true);

                    POINT physical_mouse_point;
                    GetCursorPos(&physical_mouse_point);
                    
                    std::vector<WinApiMonitorInfo> winApiOrderedMonitors;
                    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProcCallback, reinterpret_cast<LPARAM>(&winApiOrderedMonitors));

                    int monitor_index = -1;
                    HMONITOR hMon = MonitorFromPoint(physical_mouse_point, MONITOR_DEFAULTTONEAREST);
                    if (hMon) {
                        for (const auto& winApiMonitor : winApiOrderedMonitors) {
                            if (winApiMonitor.hMonitor == hMon) {
                                monitor_index = winApiMonitor.enumOrderIndex;
                                break;
                            }
                        }
                    }
                    if (monitor_index == -1) monitor_index = 0;
                    obs_data_set_int(settings, "monitor", monitor_index);
                }
                obs_data_set_bool(settings, "use_subregion", false);
                obs_source_update(source, settings);
                obs_source_release(source);
            }
            emit selectionOperationFinished();
        }
    }
}

void WindowCaptureAreaWidget::mouseMoveEvent(QMouseEvent* event)
{
    m_mousePos = event->globalPos();

    if (m_isDragging) {
        POINT physical_mouse_point;
        GetCursorPos(&physical_mouse_point);

        bool is_mouse_inside_initial_target_area = m_targetWindowScreenRect.isValid() &&
            m_targetWindowScreenRect.contains(m_mousePos);

        if (!is_mouse_inside_initial_target_area) {
            QScreen* currentScreenUnderMouse = QGuiApplication::screenAt(m_mousePos);
            if (currentScreenUnderMouse) {
                m_rcHighlightedRect = m_screen->geometry();
                m_idx = -99;
            }
            else {
                m_rcHighlightedRect = QRect();
                m_idx = -1;
            }
        }
        else {
            m_rcHighlightedRect = m_targetWindowScreenRect;
            m_idx = m_targetIdx;
        }

        m_currentDragRect = QRect(m_dragStartPosition, m_mousePos).normalized();

        if (m_currentDragRect.isValid() && m_currentDragRect.width() > 0 && m_currentDragRect.height() > 0) {
            m_dragRectSizeString = QStringLiteral("%1 X %2")
                .arg(m_currentDragRect.width())
                .arg(m_currentDragRect.height());
        }
        else {
            m_dragRectSizeString.clear();
        }
    }
    else {
        m_dragRectSizeString = QStringLiteral("%1, %2")
            .arg(m_mousePos.x())
            .arg(m_mousePos.y());

        RECT rcWindow;
        BOOL isInWindowRect = FALSE;
        int found_window_info_index = -1;

        POINT physical_mouse_point;
        GetCursorPos(&physical_mouse_point);
        for (int i = 0; i < m_allWindowsInfo.size(); ++i) {
            HWND hwnd_to_check = m_allWindowsInfo.at(i).window;
            if (!hwnd_to_check || !IsWindow(hwnd_to_check)) continue;
            if (GetClientScreenRect(hwnd_to_check, &rcWindow) && PtInRect(&rcWindow, physical_mouse_point)) {
                isInWindowRect = TRUE;
                found_window_info_index = i;
                break;
            }
        }

        if (isInWindowRect) {
            m_rcHighlightedRect = PhysicalRectToLogical(rcWindow);
            m_idx = found_window_info_index;
        }
        else {
            m_rcHighlightedRect = m_screen->geometry();
            m_idx = -99;            
        }
        m_targetWindowScreenRect = m_rcHighlightedRect;
        m_targetIdx = m_idx;
    }

    update();
}

void WindowCaptureAreaWidget::enterEvent(QEnterEvent* event)
{
    m_bMouseInside = true;
    update();
    QWidget::enterEvent(event);
}

void WindowCaptureAreaWidget::leaveEvent(QEvent* event)
{
    m_bMouseInside = false;
    m_dragRectSizeString = "";
    m_rcHighlightedRect = QRect();
    m_idx = -1;
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
