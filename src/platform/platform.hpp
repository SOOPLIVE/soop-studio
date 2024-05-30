#pragma once

#include <util/c99defs.h>

#include <string>
#include <vector>

class QWidget;

/* Gets the path of obs-studio specific data files (such as locale) */
bool GetDataFilePath(const char *data, std::string &path);

std::string GetDefaultVideoSavePath();

std::vector<std::string> GetPreferredLocales();

bool IsAlwaysOnTop(QWidget *window);
void SetAlwaysOnTop(QWidget *window, bool enable);

bool SetDisplayAffinitySupported(void);

enum TaskbarOverlayStatus {
	TaskbarOverlayStatusInactive,
	TaskbarOverlayStatusActive,
	TaskbarOverlayStatusPaused,
};
void TaskbarOverlayInit();
void TaskbarOverlaySetStatus(TaskbarOverlayStatus status);

#ifdef _WIN32
class RunOnceMutex;
RunOnceMutex
#else
void
#endif
CheckIfAlreadyRunning(bool &already_running);

#ifdef _WIN32
uint32_t GetWindowsVersion();
uint32_t GetWindowsBuild();
void SetProcessPriority(const char *priority);
void SetWin32DropStyle(QWidget *window);
bool DisableAudioDucking(bool disable);

struct RunOnceMutexData;

class RunOnceMutex {
	RunOnceMutexData *data = nullptr;

public:
	RunOnceMutex(RunOnceMutexData *data_) : data(data_) {}
	RunOnceMutex(const RunOnceMutex &rom) = delete;
	RunOnceMutex(RunOnceMutex &&rom);
	~RunOnceMutex();

	RunOnceMutex &operator=(const RunOnceMutex &rom) = delete;
	RunOnceMutex &operator=(RunOnceMutex &&rom);
};

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
QString GetMonitorName(const QString &id);
#endif
bool IsRunningOnWine();
#endif

#ifdef __APPLE__
typedef enum {
	kAudioDeviceAccess = 0,
	kVideoDeviceAccess = 1,
	kScreenCapture = 2,
	kAccessibility = 3
} MacPermissionType;

typedef enum {
	kPermissionNotDetermined = 0,
	kPermissionRestricted = 1,
	kPermissionDenied = 2,
	kPermissionAuthorized = 3,
} MacPermissionStatus;

void EnableOSXVSync(bool enable);
void EnableOSXDockIcon(bool enable);
bool isInBundle();
void InstallNSApplicationSubclass();
void InstallNSThreadLocks();
void disableColorSpaceConversion(QWidget *window);
void SetMacOSDarkMode(bool dark);
int GetHeightDock(QWidget* window);

MacPermissionStatus CheckPermissionWithPrompt(MacPermissionType type,
					      bool prompt_for_permission);
#define CheckPermission(x) CheckPermissionWithPrompt(x, false)
#define RequestPermission(x) CheckPermissionWithPrompt(x, true)
void OpenMacOSPrivacyPreferences(const char *tab);
#endif
