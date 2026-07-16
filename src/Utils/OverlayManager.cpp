
#include "OverlayManager.h"
#include "CoreModel/Action/CHotkeyContext.h"
#include "UIComponent/CMessageAlert.h"
#include "MainFrame/CMainFrame.h"

void OverlayManager::Initialize()
{
    auto activeConfig = ACTIVECONFIG;
    //
    config_set_default_string(activeConfig, "Overlay", "Window", "");
    window = config_get_string(activeConfig, "Overlay", "Window");
    //config_set_default_int(activeConfig, "Overlay", "Priority", (int64_t)0);
    //priority = (enum window_priority)config_get_int(activeConfig, "Overlay", "Priority");

    config_set_default_bool(activeConfig, "Overlay", "Editable", false);
    editable = config_get_bool(activeConfig, "Overlay", "Editable");

    config_set_default_string(activeConfig, "Hotkeys", "OBSBasic.Overlay.Editable", "{\"bindings\":[{\"control\":true,\"key\":\"OBS_KEY_ASCIITILDE\"}]}");
    hotkey = HOTKEY_CONTEXT.RegisterHotkey("OBSBasic.Overlay.Editable", Str("LiveOverlay.Editable.Toggle"), [](void*, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
        if(pressed) {
            auto editable = OVERLAY_MANAGER.Editable();
            OVERLAY_MANAGER.Editable(editable == true ? false : true);
        }
    });

    config_set_default_bool(activeConfig, "Overlay", "Chat", true);
    chat = config_get_bool(activeConfig, "Overlay", "Chat");
    config_set_default_bool(activeConfig, "Overlay", "Time", true);
    time = config_get_bool(activeConfig, "Overlay", "Time");
    config_set_default_bool(activeConfig, "Overlay", "Gift", true);
    gift = config_get_bool(activeConfig, "Overlay", "Gift");
    config_set_default_bool(activeConfig, "Overlay", "User", true);
    user = config_get_bool(activeConfig, "Overlay", "User");
    config_set_default_bool(activeConfig, "Overlay", "Up", true);
    up = config_get_bool(activeConfig, "Overlay", "Up");

    config_set_default_bool(activeConfig, "Overlay", "Enable", false);
    enable = config_get_bool(activeConfig, "Overlay", "Enable");

    //

    if(enable == true)
        _Enable(enable);
}

void OverlayManager::Finalize()
{
    AFQBorderPopupBaseWidget* popup = nullptr;
    MAIN_BLOCKMANAGER->GetPopup(ENUM_WINDOW_TYPE::SoopOverlay, popup);
    if (popup)
        popup->close();

    if(enable == true)
        _Enable(false);

    auto activeConfig = ACTIVECONFIG;
    //        
    config_set_bool(activeConfig, "Overlay", "Enable", enable);

    config_set_bool(activeConfig, "Overlay", "Up", up);
    config_set_bool(activeConfig, "Overlay", "User", user);
    config_set_bool(activeConfig, "Overlay", "Gift", gift);
    config_set_bool(activeConfig, "Overlay", "Time", time);
    config_set_bool(activeConfig, "Overlay", "Chat", chat);

    HOTKEY_CONTEXT.UnRegisterHotkey(hotkey);
    config_set_bool(activeConfig, "Overlay", "Editable", editable);

    //config_set_int(activeConfig, "Overlay", "Priority", (int64_t)priority);
    config_set_string(activeConfig, "Overlay", "Window", window.c_str());
}

void OverlayManager::_qslotTimerTarget()
{
    char* cls = nullptr;
    char* title = nullptr;
    char* exe = nullptr;
    ms_build_window_strings(window.c_str(), &cls, &title, &exe);

    auto window = ms_find_window_top_level(INCLUDE_MINIMIZED, priority, cls, title, exe);

    bfree(exe);
    bfree(title);
    bfree(cls);

    //

    do
    {
        if (window == target)
            break;

        blog(LOG_DEBUG, "window : %p, target : %p", window, target);

        target = window;

        if (SetForegroundWindow(target) == FALSE)
            blog(LOG_ERROR, "%s (%d) : target : %p", __FILE__, __LINE__, target);
    } while (false);
}

void OverlayManager::_qslotTimerWidget()
{
    do
    {
        auto window = GetForegroundWindow();
        //blog(LOG_DEBUG, "window : %p, target : %p, widget : %p, editable : %d", window, target, widget, editable);
        if ((enable == false) || (target == nullptr))
        {
            if (widget != nullptr)
                widget.reset();

            break;
        }

        if (window == target)
        {
            if (refresh == true)
            {
                refresh = false;

                if (widget != nullptr)
                    widget.reset();
            }

            if (widget == nullptr)
            {
                widget = std::make_shared<OverlaySceneWidget>(nullptr, editable);
                if (widget == nullptr)
                {
                    blog(LOG_ERROR, "%s (%d) : widget : %d", __FILE__, __LINE__, widget);
                    break;
                }

                if (chat == true)
                    widget->Chat(editable);
                
                if (time == true)
                    widget->Time(editable);

                if (gift == true)
                    widget->Gift(editable);

                if (user == true)
                    widget->User(editable);

                if (up == true)
                    widget->Up(editable);
            }

            _Track();

            if (editable == true)
            {
                DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
                DWORD myThread = GetCurrentThreadId();
                AttachThreadInput(myThread, fgThread, TRUE);
                auto overlay = reinterpret_cast<HWND>(widget->winId());
                if (SetForegroundWindow(overlay) == false)
                    blog(LOG_ERROR, "%s (%d) : overlay : %d", __FILE__, __LINE__, overlay);
                AttachThreadInput(myThread, fgThread, FALSE);
            }
        }
        else {
            if (widget != nullptr)
            {
                auto overlay = reinterpret_cast<HWND>(widget->winId());
                if (window == overlay)
                    _Track();
                else
                    widget.reset();
            }
        }
    } while (false);
}

void OverlayManager::Window(const char* window)
{
    this->window = window;

    QMetaObject::invokeMethod(this, "_qslotTimerTarget", Qt::QueuedConnection);
} 

void OverlayManager::Editable(bool able)
{
    editable = able;

    refresh = true;

    if (SetForegroundWindow(target) == false)
        blog(LOG_ERROR, "%s (%d) : target : %d", __FILE__, __LINE__, target);
}

QString OverlayManager::EditableHotkey()
{
    std::pair<obs_hotkey_id, QString> context;
    context.first = hotkey;

    auto callback = [](void* data, size_t /*idx*/, obs_hotkey_binding_t* binding) -> bool {
        auto* ctx = static_cast<std::pair<obs_hotkey_id, QString>*>(data);

        if (obs_hotkey_binding_get_hotkey_id(binding) == ctx->first) {
            obs_key_combination_t combo = obs_hotkey_binding_get_key_combination(binding);

            struct dstr dstr_key = { 0 };
            obs_key_combination_to_str(combo, &dstr_key);

            if (dstr_key.array)
                ctx->second = dstr_key.array;

            dstr_free(&dstr_key);

            return false; // break
        }

        return true; // continue
        };

    obs_enum_hotkey_bindings(callback, &context);

    return context.second;
}

void OverlayManager::RefreshHotkey()
{
    QWidget* widget = nullptr;
    MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::SoopOverlay, widget);
    if (widget != nullptr)
        QMetaObject::invokeMethod(widget, "_qslotRefreshHotkey", Qt::QueuedConnection);
}

void OverlayManager::Chat(bool able)
{
    chat = able;

    if (SetForegroundWindow(target) == false)
        blog(LOG_ERROR, "%s (%d) : target : %d", __FILE__, __LINE__, target);
}

void OverlayManager::Time(bool able)
{
    time = able;

    if (SetForegroundWindow(target) == false)
        blog(LOG_ERROR, "%s (%d) : target : %d", __FILE__, __LINE__, target);
}

void OverlayManager::Gift(bool able)
{
    gift = able;

    if (SetForegroundWindow(target) == false)
        blog(LOG_ERROR, "%s (%d) : target : %d", __FILE__, __LINE__, target);
}

void OverlayManager::User(bool able)
{
    user = able;

    if (SetForegroundWindow(target) == false)
        blog(LOG_ERROR, "%s (%d) : target : %d", __FILE__, __LINE__, target);
}

void OverlayManager::Up(bool able)
{
    up = able;

    if (SetForegroundWindow(target) == false)
        blog(LOG_ERROR, "%s (%d) : target : %d", __FILE__, __LINE__, target);
}

void OverlayManager::Enable(bool able)
{
    enable = able;

    _Enable(enable);
}

QRect OverlayManager::TargetRect(QWidget* widget)
{
    RECT rect = { 0, };
    if (GetWindowRect(target, &rect) == FALSE)
        blog(LOG_ERROR, "%s (%d) : GetLastError() : 0x%08X", __FILE__, __LINE__, GetLastError());

    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    
    qreal dpr = 1.0;
    if (widget != nullptr)
        dpr = widget->devicePixelRatio();

    int logicalWidth = static_cast<int>(w / dpr);
    int logicalHeight = static_cast<int>(h / dpr);
    const int minLogicalWidth = OverlaySceneWidget::min_w;
    const int minLogicalHeight = OverlaySceneWidget::min_h;

    if (logicalWidth < minLogicalWidth)
        logicalWidth = minLogicalWidth;

    if (logicalHeight < minLogicalHeight)
        logicalHeight = minLogicalHeight;

    return QRect(rect.left, rect.top, logicalWidth, logicalHeight);
}

void OverlayManager::Reset(QWidget* parent)
{
    AFQMessagBoxAlert alert(parent, QTStr("Popup.LiveOverlay.InitLayout.Alert.Title"),
        QTStr("Popup.LiveOverlay.InitLayout.Alert.Info"), QTStr("Reset"));
    if (QDialog::Accepted == alert.exec())
        _Reset();
}

void OverlayManager::_Reset()
{
    auto activeConfig = ACTIVECONFIG;
    //
    config_remove_value(activeConfig, "Overlay", "Setting.X");
    config_remove_value(activeConfig, "Overlay", "Setting.Y");
    config_remove_value(activeConfig, "Overlay", "Setting.Opacity");

    config_remove_value(activeConfig, "Overlay", "Chat.X");
    config_remove_value(activeConfig, "Overlay", "Chat.Y");
    config_remove_value(activeConfig, "Overlay", "Chat.W");
    config_remove_value(activeConfig, "Overlay", "Chat.H");

    config_remove_value(activeConfig, "Overlay", "Time.X");
    config_remove_value(activeConfig, "Overlay", "Time.Y");

    config_remove_value(activeConfig, "Overlay", "Gift.X");
    config_remove_value(activeConfig, "Overlay", "Gift.Y");

    config_remove_value(activeConfig, "Overlay", "User.X");
    config_remove_value(activeConfig, "Overlay", "User.Y");

    config_remove_value(activeConfig, "Overlay", "Up.X");
    config_remove_value(activeConfig, "Overlay", "Up.Y");
}

void OverlayManager::_Enable(bool able)
{
    if (able == true)
    {
        do
        {
            if (timerTarget != nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : timerTarget : %p", __FILE__, __LINE__, timerTarget);
                break;
            }

            _qslotTimerTarget();
            
            timerTarget = QSharedPointer<QTimer>(new QTimer(this));
            connect(timerTarget.data(), &QTimer::timeout, this, &OverlayManager::_qslotTimerTarget);
            if (timerTarget == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : timerTarget : %p", __FILE__, __LINE__, timerTarget);
                break;
            }

            timerTarget->start(1000);
        } while (false);

        do
        {
            if (timerWidget != nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : timerWidget : %p", __FILE__, __LINE__, timerWidget);
                break;
            }

            _qslotTimerWidget();
            
            timerWidget = QSharedPointer<QTimer>(new QTimer(this));
            connect(timerWidget.data(), &QTimer::timeout, this, &OverlayManager::_qslotTimerWidget);
            if (timerWidget == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : timerWidget : %p", __FILE__, __LINE__, timerWidget);
                break;
            }

            timerWidget->start(100);
        } while (false);

        do
        {
            if (hook[0] != nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : hook[0] : %p", __FILE__, __LINE__, hook[0]);
                break;
            }

            hook[0] = SetWinEventHook(
                EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
                nullptr, _WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT
            );
        } while (false);

        do
        {
            if (hook[1] != nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : hook[1] : %p", __FILE__, __LINE__, hook[1]);
                break;
            }

            hook[1] = SetWinEventHook(
                EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE,
                nullptr, _WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
            );
        } while (false);
    }
    else
    {
        do
        {
            if (hook[1] == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : hook[1] : %p", __FILE__, __LINE__, hook[1]);
                break;
            }

            if (UnhookWinEvent(hook[1]) == FALSE)
                blog(LOG_ERROR, "%s (%d) : hook[1] : %p", __FILE__, __LINE__, hook[1]);

            hook[1] = nullptr;
        } while (false);

        do
        {
            if (hook[0] == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : hook[0] : %p", __FILE__, __LINE__, hook[0]);
                break;
            }

            if (UnhookWinEvent(hook[0]) == FALSE)
                blog(LOG_ERROR, "%s (%d) : hook[0] : %p", __FILE__, __LINE__, hook[0]);

            hook[0] = nullptr;
        } while (false);

        do
        {
            if (timerWidget == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : timerWidget : %p", __FILE__, __LINE__, timerWidget);
                break;
            }

            timerWidget->stop();
            disconnect(timerWidget.data());
            timerWidget.clear();

            if (widget != nullptr)
                widget.reset();

            refresh = false;
        } while (false);

        do
        {
            if (timerTarget == nullptr)
            {
                blog(LOG_ERROR, "%s (%d) : timerTarget : %p", __FILE__, __LINE__, timerTarget);
                break;
            }

            timerTarget->stop();
            disconnect(timerTarget.data());
            _qslotTimerTarget();
            timerTarget.clear();

            target = nullptr;
        } while (false);
    }
}

void CALLBACK OverlayManager::_WinEventProc(HWINEVENTHOOK, DWORD event, HWND, LONG, LONG, DWORD, DWORD)
{
    if ((event == EVENT_SYSTEM_FOREGROUND) || (event == EVENT_OBJECT_LOCATIONCHANGE))
        QMetaObject::invokeMethod(&OVERLAY_MANAGER, "_qslotTimerWidget", Qt::QueuedConnection);
}

void OverlayManager::_Track()
{
    do
    {
        if (widget == nullptr)
        {
            blog(LOG_ERROR, "%s (%d) : widget : %d", __FILE__, __LINE__, widget);
            break;
        }

        auto overlay = reinterpret_cast<HWND>(widget->winId());
        auto rect = TargetRect(widget.get());
        auto x = rect.x();
        auto y = rect.y();
        if (SetWindowPos(overlay, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE) == FALSE)
            blog(LOG_ERROR, "%s (%d) : GetLastError() : 0x%08X", __FILE__, __LINE__, GetLastError());

        auto w = rect.width();
        auto h = rect.height();
        widget->resize(w, h);

        if (widget->isHidden() == true)
            widget->show();
            
        widget->repaint();
    } while (false);
}
