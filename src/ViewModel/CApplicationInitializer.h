#pragma once


#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

//Forward
class AFMakeDirectory;

class AFApplicationInitializer final
{
#pragma region QT Field, CTOR/DTOR
public:
    AFApplicationInitializer() = default;
    ~AFApplicationInitializer();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
#ifdef _WIN32
    static void             LoadDebugPrivilege();
#endif

    void                    AppEntrySetting(int argc, char* argv[]);
    void                    AppEntryRelease();
    void                    AppSetGlobalConfig(AFMakeDirectory* pDirMaker, bool bStateAppActive);
#pragma endregion public func

#pragma region private func
private:
    // callback for libobs
#ifdef _WIN32
    static void             _MainCrashHandler(const char* format, va_list args, void* caller);
#endif
    //


    void                    _CheckSafeModeSentinel();
    void                    _DeleteSafeModeSentinel();


    void                    _MoveBasicToProfiles();
    void                    _MoveBasicToSceneCollections();

#ifdef _WIN32
    void                    _ReleaseRTWorkQ();
#endif

#pragma endregion private func

#pragma region private member var
private:
#ifdef _WIN32
    HMODULE                 m_hRtwq = NULL;
#endif
#pragma endregion private member var
};

#ifdef _WIN32
extern "C" void install_dll_blocklist_hook(void);
extern "C" void log_blocked_dlls(void);
#endif
