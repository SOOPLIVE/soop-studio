#pragma once


#include <string>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

//Forward
class AFLogManager;
class AFArgOption;

class AFApplicationInitializer final
{
public:
    AFApplicationInitializer();
    ~AFApplicationInitializer();

public:
#ifdef _WIN32
    static void             LoadDebugPrivilege();
#endif

    void                    AppEntrySetting(int argc, char* argv[]);
    void                    AppEntryRelease();
    void                    AppSetGlobalConfig(bool bStateAppActive);

    inline AFLogManager&    GetLogManager() { return *m_logManager; }
    inline AFArgOption&     GetArgOption() { return *m_argOption; }

private:
    static bool             m_bIsSigned;
    // callback for libobs
#ifdef _WIN32
    static void             _MainCrashHandler(const char* format, va_list args, void* caller);
#endif
    
#ifdef __APPLE__
    static void             _MainCrashHandler(siginfo_t* info, ucontext_t* uap, void* context);
#endif
    //


    void                    _CheckSafeModeSentinel();
    void                    _DeleteSafeModeSentinel();


    void                    _MoveBasicToProfiles();
    void                    _MoveBasicToSceneCollections();

#ifdef _WIN32
    void                    _ReleaseRTWorkQ();
#endif

private:
#ifdef _WIN32
    HMODULE                 m_hRtwq = NULL;
#endif

    std::unique_ptr<AFLogManager>   m_logManager;
    std::unique_ptr<AFArgOption>    m_argOption;
};

#ifdef _WIN32
extern "C" void install_dll_blocklist_hook(void);
extern "C" void log_blocked_dlls(void);
#endif
