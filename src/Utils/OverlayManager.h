
#pragma once

#include <QTimer>

#include "util/windows/window-helpers.h"

#include "PopupWindows/COverlaySceneWidget.h"


class OverlaySceneWidget;

class OverlayManager final : public QObject {
    Q_OBJECT

public:
    explicit OverlayManager() = default;
    virtual ~OverlayManager() {}

private slots:
    void _qslotTimerTarget();
    void _qslotTimerWidget();

public:
    void Initialize();
    void Finalize();

    const char* Window() { return window.c_str(); }
    void Window(const char* window);
    //int Priority() { return priority; }
    //void Priority(int priority) { this->priority = (enum window_priority)priority; }
    bool Editable() { return editable; }
    void Editable(bool able);
    QString EditableHotkey();
    void RefreshHotkey();
    bool Chat() { return chat; }
    void Chat(bool able);
    bool Time() { return time; }
    void Time(bool able);
    bool Gift() { return gift; }
    void Gift(bool able);
    bool User() { return user; }
    void User(bool able);
    bool Up() { return up; }
    void Up(bool able);

    bool Enable() { return enable; }
    void Enable(bool able);

    QRect TargetRect(QWidget* widget); // QRect(Physical, Physical, Logical, Logical)
    void Reset(QWidget* parent);

private:
    void _Reset();
    void _Enable(bool able);
    static void CALLBACK _WinEventProc(HWINEVENTHOOK, DWORD event, HWND, LONG, LONG, DWORD, DWORD);
    void _Track();

private:
    std::string window;
    enum window_priority priority = window_priority::WINDOW_PRIORITY_CLASS;

    bool editable = false;
    obs_hotkey_id hotkey = 0;

    bool chat = true;
    bool time = true;
    bool gift = true;
    bool user = true;
    bool up = true;

    bool enable = false;

    HWND target = nullptr;
    QSharedPointer<QTimer> timerTarget;
    
    bool refresh = false;
    std::shared_ptr<OverlaySceneWidget> widget; 
    QSharedPointer<QTimer> timerWidget; 

    HWINEVENTHOOK hook[2] = { nullptr, };
};
