#include <algorithm>
#include <sstream>
#include "obs-config.h"
#include "qt-wrappers.hpp"
#include "platform/platform.hpp"   //[copy-obs] copied

#include <QSettings>
#include <QVariant>
#include <QFileInfo>
#include <QDir>

#include <util/windows/win-version.h>
#include <util/platform.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <Dwmapi.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

#include <util/windows/WinHandle.hpp>
#include <util/windows/HRError.hpp>
#include <util/windows/ComPtr.hpp>


#include "CoreModel/Locale/CLocaleTextManager.h"


using namespace std;

static inline bool check_path(const char *data, const char *path,
			      string &output)
{
	ostringstream str;
	str << path << data;
	output = str.str();

	blog(LOG_DEBUG, "Attempted path: %s", output.c_str());

	return os_file_exists(output.c_str());
}

bool GetDataFilePath(const char *data, string &output)
{
	if (check_path(data, "data/obs-studio/", output))
		return true;

	return check_path(data, OBS_DATA_PATH "/obs-studio/", output);
}

string GetDefaultVideoSavePath()
{
	wchar_t path_utf16[MAX_PATH];
	char path_utf8[MAX_PATH] = {};

	SHGetFolderPathW(NULL, CSIDL_MYVIDEO, NULL, SHGFP_TYPE_CURRENT,
			 path_utf16);

	os_wcs_to_utf8(path_utf16, wcslen(path_utf16), path_utf8, MAX_PATH);
	//return string(path_utf8);
	return string(path_utf8) + "\\SOOP";
}

static vector<string> GetUserPreferredLocales()
{
	vector<string> result;

	ULONG num, length = 0;
	if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &num, nullptr,
					 &length))
		return result;

	vector<wchar_t> buffer(length);
	if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &num,
					 &buffer.front(), &length))
		return result;

	result.reserve(num);
	auto start = begin(buffer);
	auto end_ = end(buffer);
	decltype(start) separator;
	while ((separator = find(start, end_, 0)) != end_) {
		if (result.size() == num)
			break;

		char conv[MAX_PATH] = {};
		os_wcs_to_utf8(&*start, separator - start, conv, MAX_PATH);

		result.emplace_back(conv);

		start = separator + 1;
	}

	return result;
}

vector<string> GetPreferredLocales()
{
	vector<string> windows_locales = GetUserPreferredLocales();
	auto obs_locales = AFLocaleTextManager::GetLocaleNames();
	auto windows_to_obs = [&obs_locales](string windows) {
		string lang_match;

		for (auto &locale_pair : obs_locales) {
			auto &locale = locale_pair.first;
			if (locale == windows.substr(0, locale.size()))
				return locale;

			if (lang_match.size())
				continue;

			if (locale.substr(0, 2) == windows.substr(0, 2))
				lang_match = locale;
		}

		return lang_match;
	};

	vector<string> result;
	result.reserve(obs_locales.size());

	for (const string &locale : windows_locales) {
		string match = windows_to_obs(locale);
		if (!match.size())
			continue;

		if (find(begin(result), end(result), match) != end(result))
			continue;

		result.emplace_back(match);
	}

	return result;
}

uint32_t GetWindowsVersion()
{
	static uint32_t ver = 0;

	if (ver == 0) {
		struct win_version_info ver_info;

		get_win_ver(&ver_info);
		ver = (ver_info.major << 8) | ver_info.minor;
	}

	return ver;
}

uint32_t GetWindowsBuild()
{
	static uint32_t build = 0;

	if (build == 0) {
		struct win_version_info ver_info;

		get_win_ver(&ver_info);
		build = ver_info.build;
	}

	return build;
}

bool IsAlwaysOnTop(QWidget *window)
{
	DWORD exStyle = GetWindowLong((HWND)window->winId(), GWL_EXSTYLE);
	return (exStyle & WS_EX_TOPMOST) != 0;
}

void SetAlwaysOnTop(QWidget *window, bool enable)
{
    QWidget *parentMostWindow = window->window();
    HWND hwnd = (HWND)parentMostWindow->winId();
	
    SetWindowPos(hwnd, enable ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void SetProcessPriority(const char *priority)
{
	if (!priority)
		return;

	if (strcmp(priority, "High") == 0)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else if (strcmp(priority, "AboveNormal") == 0)
		SetPriorityClass(GetCurrentProcess(),
				 ABOVE_NORMAL_PRIORITY_CLASS);
	else if (strcmp(priority, "Normal") == 0)
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	else if (strcmp(priority, "BelowNormal") == 0)
		SetPriorityClass(GetCurrentProcess(),
				 BELOW_NORMAL_PRIORITY_CLASS);
	else if (strcmp(priority, "Idle") == 0)
		SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
}

void SetWin32DropStyle(QWidget *window)
{
	HWND hwnd = (HWND)window->winId();
	LONG_PTR ex_style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	ex_style |= WS_EX_ACCEPTFILES;
	SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex_style);
}

bool SetDisplayAffinitySupported(void)
{
	static bool checked = false;
	static bool supported;

	/* this has to be version gated as setting WDA_EXCLUDEFROMCAPTURE on
	   older Windows builds behaves like WDA_MONITOR (black box) */

	if (!checked) {
		if (GetWindowsVersion() > 0x0A00 ||
		    GetWindowsVersion() == 0x0A00 && GetWindowsBuild() >= 19041)
			supported = true;
		else
			supported = false;

		checked = true;
	}

	return supported;
}

bool DisableAudioDucking(bool disable)
{
	ComPtr<IMMDeviceEnumerator> devEmum;
	ComPtr<IMMDevice> device;
	ComPtr<IAudioSessionManager2> sessionManager2;
	ComPtr<IAudioSessionControl> sessionControl;
	ComPtr<IAudioSessionControl2> sessionControl2;

	HRESULT result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
					  CLSCTX_INPROC_SERVER,
					  __uuidof(IMMDeviceEnumerator),
					  (void **)&devEmum);
	if (FAILED(result))
		return false;

	result = devEmum->GetDefaultAudioEndpoint(eRender, eConsole, &device);
	if (FAILED(result))
		return false;

	result = device->Activate(__uuidof(IAudioSessionManager2),
				  CLSCTX_INPROC_SERVER, nullptr,
				  (void **)&sessionManager2);
	if (FAILED(result))
		return false;

	result = sessionManager2->GetAudioSessionControl(nullptr, 0,
							 &sessionControl);
	if (FAILED(result))
		return false;

	result = sessionControl->QueryInterface(&sessionControl2);
	if (FAILED(result))
		return false;

	result = sessionControl2->SetDuckingPreference(disable);
	return SUCCEEDED(result);
}

struct RunOnceMutexData {
	WinHandle handle;

	inline RunOnceMutexData(HANDLE h) : handle(h) {}
};

RunOnceMutex::RunOnceMutex(RunOnceMutex &&rom)
{
	delete data;
	data = rom.data;
	rom.data = nullptr;
}

RunOnceMutex::~RunOnceMutex()
{
	delete data;
}

RunOnceMutex &RunOnceMutex::operator=(RunOnceMutex &&rom)
{
	delete data;
	data = rom.data;
	rom.data = nullptr;
	return *this;
}

RunOnceMutex CheckIfAlreadyRunning(bool &already_running)
{
	string name;

		name = "FreecshotPlusCore";
	//} else {
	//	char path[500];
	//	char absPath[512];
	//	*path = 0;
	//	*absPath = 0;
	//	CONFIG_CONTEXT.GetConfigPath(path, sizeof(path), "");
	//	os_get_abs_path(path, absPath, sizeof(absPath));
	//	name = "ANETAStudio-Portable";
	//	name += absPath;
	//}

	BPtr<wchar_t> wname;
	os_utf8_to_wcs_ptr(name.c_str(), name.size(), &wname);

	if (wname) {
		wchar_t *temp = wname;
		while (*temp) {
			if (!iswalnum(*temp))
				*temp = L'_';
			temp++;
		}
	}

	HANDLE h = OpenMutexW(SYNCHRONIZE, false, wname.Get());
	already_running = !!h;

	if (!already_running)
		h = CreateMutexW(nullptr, false, wname.Get());

	RunOnceMutex rom(h ? new RunOnceMutexData(h) : nullptr);
	return rom;
}

struct MonitorData {
	const wchar_t *id;
	MONITORINFOEX info;
	bool found;
};

static BOOL CALLBACK GetMonitorCallback(HMONITOR monitor, HDC, LPRECT,
					LPARAM param)
{
	MonitorData *data = (MonitorData *)param;

	if (GetMonitorInfoW(monitor, &data->info)) {
		if (wcscmp(data->info.szDevice, data->id) == 0) {
			data->found = true;
			return false;
		}
	}

	return true;
}

/* Based on https://www.winehq.org/pipermail/wine-devel/2008-September/069387.html */
typedef const char *(CDECL *WINEGETVERSION)(void);
bool IsRunningOnWine()
{
	WINEGETVERSION func;
	HMODULE nt;

	nt = GetModuleHandleW(L"ntdll");
	if (!nt)
		return false;

	func = (WINEGETVERSION)GetProcAddress(nt, "wine_get_version");
	if (func) {
		blog(LOG_WARNING, "Running on Wine version \"%s\"", func());
		return true;
	}

	return false;
}

HWND hwnd;
void TaskbarOverlayInit()
{
	//[copy-obs]remove
	//hwnd = (HWND)MAINFRAME->winId();
}

void TaskbarOverlaySetStatus(TaskbarOverlayStatus status)
{
	ITaskbarList4 *taskbarIcon;
	auto hr = CoCreateInstance(CLSID_TaskbarList, NULL,
				   CLSCTX_INPROC_SERVER,
				   IID_PPV_ARGS(&taskbarIcon));

	if (FAILED(hr)) {
		taskbarIcon->Release();
		return;
	}

	hr = taskbarIcon->HrInit();

	if (FAILED(hr)) {
		taskbarIcon->Release();
		return;
	}

	QIcon qicon;
	switch (status) {
	case TaskbarOverlayStatusActive:
		qicon = QIcon::fromTheme("obs-active", QIcon(":/res/images/active.png"));
		break;
	case TaskbarOverlayStatusPaused:
		qicon = QIcon::fromTheme("obs-paused", QIcon(":/res/images/paused.png"));
		break;
	case TaskbarOverlayStatusInactive:
		taskbarIcon->SetOverlayIcon(hwnd, nullptr, nullptr);
		taskbarIcon->Release();
		return;
	}

	HICON hicon = nullptr;
	if (!qicon.isNull()) {
		Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &p);
		hicon = qt_pixmapToWinHICON(qicon.pixmap(GetSystemMetrics(SM_CXSMICON)));
	}
	if(!hicon)
		return;

	taskbarIcon->SetOverlayIcon(hwnd, hicon, nullptr);
	DestroyIcon(hicon);
	taskbarIcon->Release();
}

bool HighContrastEnabled()
{
	HIGHCONTRAST hc = {};
	hc.cbSize = sizeof(HIGHCONTRAST);

	if(SystemParametersInfo(SPI_GETHIGHCONTRAST, hc.cbSize, &hc, 0))
		return hc.dwFlags & HCF_HIGHCONTRASTON;

	return false;
}

inline QString appendPath(const QString& root, const QString& subKey)
{
	QString path = root;
	QString cleanSubKey = QDir::toNativeSeparators(subKey);

	if(!path.endsWith('\\') && !cleanSubKey.startsWith('\\')) {
		path += '\\';
	}
	path += cleanSubKey;

	return QDir::toNativeSeparators(path);
}

bool RegKeyExists(const QString& key)
{
	QString path = QDir::toNativeSeparators(key);
	int lastSlash = path.lastIndexOf('\\');
	if(lastSlash == -1) return false;

	QString parent = path.left(lastSlash);
	QString target = path.mid(lastSlash + 1);

	QSettings settings(parent, QSettings::NativeFormat);
	return settings.childGroups().contains(target, Qt::CaseInsensitive);
}
bool RegKeyExists(const QString& key, const QString& subKey)
{
	QString fullPath = appendPath(key, subKey);

	int lastSlash = fullPath.lastIndexOf('\\');

	if(lastSlash == -1) return false;

	QString parentPath = fullPath.left(lastSlash);
	QString targetKeyName = fullPath.mid(lastSlash + 1);

	QSettings parentSettings(parentPath, QSettings::NativeFormat);

	return parentSettings.childGroups().contains(targetKeyName, Qt::CaseInsensitive);
}
bool RegValueExists(const QString& key, const QString& valueName)
{
	QSettings settings(key, QSettings::NativeFormat);
	return settings.contains(valueName);
}

inline QVariant getValue(const QString& root, const QString& subKey, const QString& valueName, const QVariant& defaultValue = QVariant())
{
	QString fullPath = appendPath(root, subKey);
	QSettings settings(fullPath, QSettings::NativeFormat);

	return settings.value(valueName, defaultValue);
}

bool GetRegKeyValue(const QString& key, const QString& subKey, const QString& valueName, QString& value)
{
	QString value_ = getValue(key, subKey, valueName).toString();
	if(value_.isEmpty())
		return false;
	//
	value = value_;
	return true;
}

UninstallResult RunUninstallerWithUAC(const QString& exePath, const QString& params, uint32_t timeoutMs)
{
	SHELLEXECUTEINFOW sei = {sizeof(SHELLEXECUTEINFOW)};
	sei.lpVerb = L"runas"; // Need UAC
	sei.lpFile = reinterpret_cast<LPCWSTR>(exePath.utf16());
	sei.lpParameters = reinterpret_cast<LPCWSTR>(params.utf16());
	sei.nShow = SW_HIDE;
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;

	const std::wstring workDir = QFileInfo(exePath).absoluteDir().absolutePath().toStdWString();
	sei.lpDirectory = workDir.c_str();

	if(!::ShellExecuteExW(&sei)) {
		DWORD err = ::GetLastError();
		if(err == ERROR_CANCELLED /*1223*/) {
			blog(LOG_WARNING, "[FreecShot Unistall]:UAC cancelled");
			return UninstallResult::UacCanceled;
		}
		blog(LOG_WARNING, "[FreecShot Uninstall]:ShellExecuteExW failed: %lu", err);
		return UninstallResult::LaunchFailed;
	}

	// Wait for shutdown (Timeout)
	DWORD wait = ::WaitForSingleObject(sei.hProcess, timeoutMs);
	if(wait == WAIT_TIMEOUT) {
		blog(LOG_WARNING, "[FreecShot Uninstall]:Uninstaller timeout");
		::CloseHandle(sei.hProcess);
		return UninstallResult::Timeout;
	}

	DWORD exitCode = 0xFFFFFFFF;
	if(!::GetExitCodeProcess(sei.hProcess, &exitCode)) {
		blog(LOG_WARNING, "[FreecShot Uninstall]:GetExitCodeProcess failed: %lu", GetLastError());
		::CloseHandle(sei.hProcess);
		return UninstallResult::FailedWithExitCode;
	}

	::CloseHandle(sei.hProcess);

	// Representative exit codes by installer type (including MSI)
	switch(exitCode) {
		case 0:    return UninstallResult::Succeeded;        // Success
		case 3010: return UninstallResult::RebootRequired;   // Reboot required (MSI)
		case 1602: return UninstallResult::UserCanceled;     // User canceled (MSI)
		case 1603: // Fatal error (MSI)
		case 1618: // Another installation is already in progress (MSI)
		default:
			blog(LOG_WARNING, "[FreecShot Uninstall]:Uninstaller exit code: %d", static_cast<int>(exitCode));
			return UninstallResult::FailedWithExitCode;
	}
}
