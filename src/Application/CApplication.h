#pragma once

#include <QApplication>
#include <QTranslator>
#include <QPointer>
#include <QFileSystemWatcher>

#ifndef _WIN32
#include <QSocketNotifier>
#else
#include <QSessionManager>
#endif
#include <obs.hpp>
#include <util/lexer.h>
#include <util/profiler.h>
#include <util/util.hpp>
#include <util/platform.h>
#include <obs-frontend-api.h>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <filesystem>
#include <QAbstractNativeEventFilter>

#include "window-main.hpp"
#include "MainFrame/CMainFrame.h"

typedef std::function<void()> VoidFunc;

#define MAINFRAME           App()->GetMainView()
#define DYNAMIC_COMPOSIT    MAINFRAME->GetMainWindow()
#define APPNAME             "FreecShot Plus"

#define CONFIG_CONTEXT      App()->GetConfig()
#define APPCONFIG           App()->GetAppConfig()
#define USERCONFIG          App()->GetUserConfig()
#define ACTIVECONFIG        App()->GetActiveConfig()
//
#define USERCONFIG_PATH     CONFIG_CONTEXT.GetUserConfigPath()
#define USERSCENES_PATH     CONFIG_CONTEXT.GetUserScenesPath()
#define USERPROFILE_PATH    CONFIG_CONTEXT.GetUserProfilePath()
//
#define INHIBITSLEEP_CONTEXT App()->GetInhibitSleepContext()
#define STATISTICS          App()->GetStatistics()
#define LOGMANAGER          App()->GetLogManager()

//
#define ARGOPTION           App()->GetArgOption()
#define STATEAPP            App()->GetStateApp()

#define HOTKEY_CONTEXT      App()->GetHotkey()
#define AUTH_CONTEXT        App()->GetAuth()
#define LOADSAVE_CONTEXT    App()->GetLoadSave()

#define LOCALE_CONTEXT      App()->GetLocaleManager()
#define ICON_CONTEXT        App()->GetIconManager()

//Forward
class AFLogManager;
class AFArgOption;
//
class AFInhibitSleepContext;
class AFStatistics;
//
class AFStateAppContext;
class AFConfigManager;
class AFServiceManager;
class AFHotkeyContext;
class AFAuthManager;
class CAppStyling;
class AFLoadSaveManager;
class AFLocaleTextManager;
class AFIconContext;
//
class AFMainFrame;
class AFMakeDirectory;

class AFQTranslator final : public QTranslator
{
    Q_OBJECT

public:
    AFQTranslator() = default;
    ~AFQTranslator() = default;

public:
    bool isEmpty() const override { return false; }

    QString translate(const char* context, const char* sourceText,
                      const char* disambiguation,
                      int n) const override;
};

class WinShutDownEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray&, void* message, qintptr* result) override;
};

class AFQApplication : public QApplication
{
    Q_OBJECT

public:
    AFQApplication(int& argc, char** argv);
    ~AFQApplication();

public slots:

private slots:
#ifdef _WIN32
    void qslotCommitData(QSessionManager& manager);
#endif
    void qslotProcessSigInt();
    
    //focus Test
    void qslotFocusChanged(QWidget* old, QWidget* now);

signals:
    void StyleChanged();

public:
    static int              RunProgramProcess(AFQApplication* app, int argc, char* argv[]);
    //
    void                    AppInit();
    bool                    MainFrameInit();

    bool                    GetNoUpdate() { return m_noUpdate; }
    bool                    SetNoUpdate(bool noUpdate) { return m_noUpdate = noUpdate; }

    AFLogManager&           GetLogManager() const;
    AFArgOption&            GetArgOption() const;

    AFMainFrame*            GetMainView() const { return m_mainView.data(); }

    AFStatistics&           GetStatistics() const { return *m_statistics; }

    AFInhibitSleepContext&  GetInhibitSleepContext() const { return *m_inhibitSleepContext; }
 
    AFStateAppContext&      GetStateApp() const { return *m_statesApp; };

    AFConfigManager&        GetConfig() const { return *m_config; }
    AFHotkeyContext&        GetHotkey() const { return *m_hotkey; }
    AFAuthManager&          GetAuth() const { return *m_auth; }
    CAppStyling&            GetAppStyling() const { return *m_appStyling; }
    AFLoadSaveManager&      GetLoadSave() const { return *m_loadSave; }
    AFLocaleTextManager&    GetLocaleManager() const { return *m_localeManager; }
    AFIconContext&          GetIconManager() const { return *m_iconManager; }

    //
    config_t*               GetAppConfig() const;
    config_t*               GetUserConfig() const;
    config_t*               GetActiveConfig() const;

    // [Hotkey]
    void                    ResetHotkeyState(bool inFocus);
    void                    UpdateHotkeyFocusSetting(bool reset = true);
    void                    DisableHotkeys();
    bool                    HotkeysEnabledInFocus() const { return m_enableHotkeysInFocus; }
    
#ifdef _WIN32
    //void                    UpdaterKill();
#endif
    void                    SetVersionInfo();

#ifndef _WIN32
    static void             SigIntSignalHandler(int s);
#endif

    void SetQss(QString type);
    const char* InputAudioSource() const;
    const char* OutputAudioSource() const;

    bool			GetEnableHotkeysInFocus() { return m_enableHotkeysInFocus; };
    void			SetEnableHotkeysInFocus(bool value) { m_enableHotkeysInFocus = value; };
    bool			GetEnableHotkeysOutOfFocus() { return m_enableHotkeysOutOfFocus; };
    void			SetEnableHotkeysOutOfFocus(bool value) { m_enableHotkeysOutOfFocus = value; };

    bool CheckClickableWidget(QObject* obj, bool disableMove = false);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    bool notify(QObject* receiver, QEvent* e) override;
    
    bool _LibOBSInitialize();
    void _InitializeWithArguments();

private:
#ifndef _WIN32
    static int                  s_sigintFd[2];
    QSocketNotifier*            m_pSNInt = nullptr;
#endif

    QPointer<AFMainFrame>       m_mainView;

    std::unique_ptr<AFInhibitSleepContext> m_inhibitSleepContext;

    std::unique_ptr<AFStatistics>       m_statistics;

    std::unique_ptr<AFStateAppContext>  m_statesApp;
    std::unique_ptr<AFConfigManager>    m_config;
    std::unique_ptr<AFHotkeyContext>    m_hotkey;
    std::unique_ptr<AFAuthManager>      m_auth;
    std::unique_ptr<CAppStyling>        m_appStyling;
    std::unique_ptr<AFLoadSaveManager>  m_loadSave;
    std::unique_ptr<AFLocaleTextManager> m_localeManager;
    std::unique_ptr<AFIconContext>      m_iconManager;

    bool                        m_libobsInitialized = false;
    bool                        m_enableHotkeysInFocus = true;
    bool                        m_enableHotkeysOutOfFocus = true;

    bool                        m_noUpdate = true;

    QString                     m_Freecshot_Type;
    QString                     m_streamerID;
    QString                     m_cookie;
    QString                     m_type;
    QString                     m_Update_Path;

    QString                     m_localeStr;
    QString                     m_install_Type;

    std::chrono::steady_clock::time_point m_startTime;
};

inline AFQApplication* App()
{
    return static_cast<AFQApplication*>(qApp);
}

bool WindowPositionValid(QRect rect);


extern bool g_bRestart;
extern bool g_bRestartSafe;
extern bool g_opt_minimize_tray;
extern bool g_opt_always_on_top;
//
extern bool g_bIsDownloadingUpdate;

extern const char* Str(const char* lookupVal);
extern QString  QTStr(const char* lookupVal);
extern std::string GetChannelId(const char* platform);

extern QObject* CreateShortcutFilter();
// path
extern QString GetLocalAppDataPath();
extern int GetAppConfigPath(char* path, size_t size, const char* name);
extern char* GetAppConfigPathPtr(const char* name);
extern int GetProgramDataPath(char* path, size_t size, const char* name);
extern char* GetProgramDataPathPtr(const char* name);
extern int GetProfilePath(char* path, size_t size, const char* file);