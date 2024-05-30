#pragma once

#include <QApplication>
#include <QTranslator>
#include <QPointer>
#include <QScreen>
#ifndef _WIN32
#include <QSocketNotifier>
#else
#include <QSessionManager>
#endif

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Statistics/CStatistics.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"


#include "ViewModel/CApplicationInitializer.h"


typedef std::function<void()> VoidFunc;


#define APP                 App()
#define MAINFRAME           APP->GetMainView()
#define DYNAMIC_COMPOSIT    MAINFRAME->GetMainWindow()

//Forward
class AFArgOption;


class AFQTranslator final : public QTranslator
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT
public:
    AFQTranslator() = default;
    ~AFQTranslator() = default;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    bool isEmpty() const override { return false; }

    QString translate(const char* context, const char* sourceText,
                      const char* disambiguation,
                      int n) const override;
#pragma endregion public func
};

class AFQApplication : public QApplication
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    AFQApplication(int& argc, char** argv);
    ~AFQApplication();

public slots:

private slots:
#ifdef _WIN32
    void                    qslotCommitData(QSessionManager& manager);
#endif
    void                    qslotProcessSigInt();


signals:
    void StyleChanged();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void                    AppInit();
    bool                    MainFrameInit();
    
    void                    AppRelease();
    void                    MainFrameRelease();

    inline AFMainFrame*     GetMainView() const { return mainView.data(); }
    inline AFStatistics*    GetStatistics() const { return m_Statistics.data(); }

    static int              RunProgramProcess(AFQApplication* app,
                                              AFArgOption* pArgOption, int argc, char* argv[]);

    // [Hotkey]
    inline void             ResetHotkeyState(bool inFocus);
    void                    UpdateHotkeyFocusSetting(bool reset = true);
    void                    DisableHotkeys();
    inline bool             HotkeysEnabledInFocus() const;

#ifndef _WIN32
    static void             SigIntSignalHandler(int s);
#endif
    
    void SetQss(QString type);
    const char* InputAudioSource() const;
    const char* OutputAudioSource() const;

    bool                    GetEnableHotkeysInFocus();
#pragma endregion public func

#pragma region private func
private:
    bool notify(QObject* receiver, QEvent* e) override;
    
    bool                    _LibOBSInitialize();
#pragma endregion private func



#pragma region private member var
private:
#ifndef _WIN32
    static int                  sigintFd[2];
    QSocketNotifier*            snInt = nullptr;
#endif


    QPointer<AFMainFrame>           mainView;
    QPointer<AFStatistics>          m_Statistics;

    
    //                              AFInhibitSleepContext
    
    
    bool                            m_bLibobsInitialized = false;


    bool m_EnableHotkeysInFocus = true;
    bool m_EnableHotkeysOutOfFocus = true;

#pragma endregion private member var
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

extern config_t* GetBasicConfig();
extern config_t* GetGlobalConfig();
extern obs_service_t* GetService();
extern const char* Str(const char* lookupVal);
extern QString  QTStr(const char* lookupVal);

extern QObject* CreateShortcutFilter();