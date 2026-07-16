#include "CApplicationInitializer.h"


#include <sstream>
#include <fstream>
#include <memory>

#ifdef _WIN32
#include <filesystem>
#include <wincrypt.h>
#include <Wintrust.h>
#include <Softpub.h>

#pragma comment(lib, "Wintrust.lib")
#pragma comment(lib, "Crypt32.lib")
#else
#include <signal.h>
#include <pthread.h>
#endif


#include <curl/curl.h>
#include <obs.hpp>
#include <util/platform.h>
#include <util/windows/win-version.h>

#include "platform/platform.hpp"
#include "Common/StringMiscUtils.h"
#include "Common/StudioDefine.h"
#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CMakeDirectory.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Log/CLogManager.h"

#include "Application/CApplication.h"

#define MAX_CRASH_REPORT_SIZE (150 * 1024)

AFApplicationInitializer::AFApplicationInitializer()
	:m_logManager(std::make_unique<AFLogManager>()),
	m_argOption(std::make_unique<AFArgOption>())
{}
AFApplicationInitializer::~AFApplicationInitializer()
{
#ifdef _WIN32
	_ReleaseRTWorkQ();
#endif
}

#ifdef _WIN32
void AFApplicationInitializer::LoadDebugPrivilege()
{
	const DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
	TOKEN_PRIVILEGES tp;
	HANDLE token;
	LUID val;

	if (!OpenProcessToken(GetCurrentProcess(), flags, &token))
		return;
	


	if (!!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &val))
	{
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(token, false, &tp, sizeof(tp), NULL,
			NULL);
	}

	if (!!LookupPrivilegeValue(NULL, SE_INC_BASE_PRIORITY_NAME, &val))
	{
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		if (!AdjustTokenPrivileges(token, false, &tp, sizeof(tp), NULL, NULL))
		{
			blog(LOG_INFO, "Could not set privilege to increase GPU priority");
		}
	}

	CloseHandle(token);
}

static constexpr char vcRunErrorTitle[] = "Outdated Visual C++ Runtime";
static constexpr char vcRunErrorMsg[] = "SOOP Studio requires a newer version of the Microsoft Visual C++ "
										"Redistributables.\n\nYou will now be directed to the download page.";
static constexpr char vcRunInstallerUrl[] = "https://obsproject.com/visual-studio-2022-runtimes";

static bool vc_runtime_outdated()
{
	win_version_info ver;
	if(!get_dll_ver(L"msvcp140.dll", &ver))
		return true;
	/* Major is always 14 (hence 140.dll), so we only care about minor. */
	if(ver.minor >= 40)
		return false;

	int choice = MessageBoxA(NULL, vcRunErrorMsg, vcRunErrorTitle, MB_OKCANCEL | MB_ICONERROR | MB_TASKMODAL);
	if(choice == IDOK) {
		/* Open the URL in the default browser. */
		ShellExecuteA(NULL, "open", vcRunInstallerUrl, NULL, NULL, SW_SHOWNORMAL);
	}

	return true;
}
#endif

void AFApplicationInitializer::AppEntrySetting(int argc, char* argv[])
{
#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);

	struct sigaction sig_handler;

	sig_handler.sa_handler = AFQApplication::SigIntSignalHandler;
	sigemptyset(&sig_handler.sa_mask);
	sig_handler.sa_flags = 0;

	sigaction(SIGINT, &sig_handler, NULL);

	/* Block SIGPIPE in all threads, this can happen if a thread calls write on
	a closed pipe. */
	sigset_t sigpipe_mask;
	sigemptyset(&sigpipe_mask);
	sigaddset(&sigpipe_mask, SIGPIPE);
	sigset_t saved_mask;
	if (pthread_sigmask(SIG_BLOCK, &sigpipe_mask, &saved_mask) == -1) {
		perror("pthread_sigmask");
		exit(1);
	}
#endif

#ifdef _WIN32
	// Abort as early as possible if MSVC runtime is outdated
	/*if(vc_runtime_outdated())
		return;*/

	// Try to keep this as early as possible
	install_dll_blocklist_hook();

	obs_init_win32_crash_handler();
	SetErrorMode(SEM_FAILCRITICALERRORS);
	LoadDebugPrivilege();
	base_set_crash_handler(_MainCrashHandler, nullptr);

	m_hRtwq = LoadLibrary(L"RTWorkQ.dll");
	if (m_hRtwq) {
		typedef HRESULT(STDAPICALLTYPE* PFN_RtwqStartup)();
		PFN_RtwqStartup func = (PFN_RtwqStartup)GetProcAddress(m_hRtwq, "RtwqStartup");
		func();
	}
#endif

#if defined(__APPLE__) && !defined(_DEBUG)
    InitPLCrashReporter(_MainCrashHandler);
#endif
    
	base_get_log_handler(LOGMANAGER.GetDefLogHandler(), nullptr);
	base_get_api_log_handler(LOGMANAGER.GetDefAPILogHandler(), nullptr);

	auto& argOption = ARGOPTION;
	//
	argOption.LoadArgProgram(argc, argv);

#if ALLOW_PORTABLE_MODE
	if (argOption.GetPortableMode() == false) 
	{
		argOption.SetPortableMode(
			os_file_exists(BASE_PATH "/portable_mode") ||
			os_file_exists(BASE_PATH "/obs_portable_mode") ||
			os_file_exists(BASE_PATH "/portable_mode.txt") ||
			os_file_exists(BASE_PATH "/obs_portable_mode.txt")
		);
	}

	//if (!opt_disable_updater) {
	//	opt_disable_updater =
	//		os_file_exists(BASE_PATH "/disable_updater") ||
	//		os_file_exists(BASE_PATH "/disable_updater.txt");
	//}

	if (argOption.GetDisableMissingFilesCheck() == false)
	{
		argOption.SetDisableMissingFilesCheck(
			os_file_exists(BASE_PATH "/disable_missing_files_check") ||
			os_file_exists(BASE_PATH "/disable_missing_files_check.txt")
		);
	}
#endif

	//_CheckSafeModeSentinel();

	curl_global_init(CURL_GLOBAL_ALL);
}

void AFApplicationInitializer::AppEntryRelease()
{
#ifdef _WIN32
	_ReleaseRTWorkQ();
	log_blocked_dlls();
#endif

	//_DeleteSafeModeSentinel();
	//blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
	base_set_log_handler(nullptr, nullptr);
}

void AFApplicationInitializer::AppSetGlobalConfig(bool bStateAppActive)
{
#ifdef _WIN32
	bool disableAudioDucking = config_get_bool(APPCONFIG, "Audio", "DisableAudioDucking");
	if (disableAudioDucking)
		DisableAudioDucking(true);
#endif

#ifdef __APPLE__
	if (config_get_bool(APPCONFIG, "Video", "DisableOSXVSync"))
		EnableOSXVSync(false);
#endif

    App()->UpdateHotkeyFocusSetting(false);

	_MoveBasicToProfiles();
	_MoveBasicToSceneCollections();

    if (AFMakeDirectoryUtil::MakeUserProfileDirs() == false)
		throw "Failed to create profile directories";
}

#ifdef _WIN32
#define		CRASH_MESSAGE																	\
			"Woops, Freecshot Plus has crashed!\n\nWould you like to copy the crash log "	\
			"to the clipboard? The crash log will still be saved to:\n\n%s"

void AFApplicationInitializer::_MainCrashHandler(const char* format, va_list args, void*)
{
	char* text = new char[MAX_CRASH_REPORT_SIZE];

	vsnprintf(text, MAX_CRASH_REPORT_SIZE, format, args);
	text[MAX_CRASH_REPORT_SIZE - 1] = 0;

	std::string crashFilePath = LOCAL_FOLDER_NAME + "/crashes";

	LOGMANAGER.DeleteOldestFile(true, crashFilePath.c_str());

	std::string name = crashFilePath + "/";
	name += "Crash " + GenerateTimeDateFilename("txt");

	BPtr<char> path(GetAppConfigPathPtr(name.c_str()));

	std::fstream file;

#ifdef _WIN32
	BPtr<wchar_t> wpath;
	os_utf8_to_wcs_ptr(path, 0, &wpath);
	file.open(wpath, std::ios_base::in | std::ios_base::out | std::ios_base::trunc |
			  std::ios_base::binary);
#else
	file.open(path, ios_base::in | ios_base::out | ios_base::trunc |
		ios_base::binary);
#endif
	file << text;
	file.close();

	std::string pathString(path.Get());

#ifdef _WIN32
	std::replace(pathString.begin(), pathString.end(), '/', '\\');
#endif

	std::string absolutePath =
		canonical(std::filesystem::path(pathString)).u8string();

	size_t size = snprintf(nullptr, 0, CRASH_MESSAGE, absolutePath.c_str());

	std::unique_ptr<char[]> message_buffer(new char[size + 1]);

	snprintf(message_buffer.get(), size + 1, CRASH_MESSAGE,
		absolutePath.c_str());

	std::string finalMessage =
		std::string(message_buffer.get(), message_buffer.get() + size);

	int ret = MessageBoxA(NULL, finalMessage.c_str(), "Freecshot Plus has crashed!",
		MB_YESNO | MB_ICONERROR | MB_TASKMODAL);

	if (ret == IDYES) {
		size_t len = strlen(text);

		HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, len);
		if (mem != NULL)
		{
			LPVOID lockedMem = GlobalLock(mem);
			if (lockedMem != NULL)
			{
				memcpy(lockedMem, text, len);
				GlobalUnlock(mem);
			}
		}

		OpenClipboard(0);
		EmptyClipboard();
		SetClipboardData(CF_TEXT, mem);
		CloseClipboard();
	}

	_exit(-1);
}
#endif

#ifdef __APPLE__
void AFApplicationInitializer::_MainCrashHandler(siginfo_t* info, ucontext_t* uap, void* context)
{
    std::string out = PrintLogCrash(context);
}
#endif

void AFApplicationInitializer::_CheckSafeModeSentinel()
{
#ifndef NDEBUG
	/* Safe Mode detection is disabled in Debug builds to keep developers
	 * somewhat sane. */
	return;
#else
	if (ARGOPTION.GetDisableShutdownCheck())
		return;

	BPtr sentinelPath = GetAppConfigPathPtr("SOOPStudio/safe_mode");
	if (os_file_exists(sentinelPath)) {
		ARGOPTION.SetUncleanShutdown(true);
		return;
	}

	os_quick_write_utf8_file(sentinelPath, nullptr, 0, false);
#endif
}

void AFApplicationInitializer::_DeleteSafeModeSentinel()
{
#ifndef NDEBUG
	return;
#else
	BPtr sentinelPath = GetAppConfigPathPtr("SOOPStudio/safe_mode");
	os_unlink(sentinelPath);
#endif
}

void AFApplicationInitializer::_MoveBasicToProfiles()
{
	char path[512];

	if(GetAppConfigPath(path, 512, (LOCAL_FOLDER_NAME + "/basic").c_str()) <= 0) {
		return;
	}

	const std::filesystem::path basicPath = std::filesystem::u8path(path);

	if(!std::filesystem::exists(basicPath)) {
		return;
	}

	const std::filesystem::path profilesPath =
		CONFIG_CONTEXT.GetUserProfilePath() / std::filesystem::u8path((LOCAL_FOLDER_NAME + "/basic/profiles").c_str());

	if(std::filesystem::exists(profilesPath)) {
		return;
	}

	try {
		std::filesystem::create_directories(profilesPath);
	} catch(const std::filesystem::filesystem_error& error) {
		blog(LOG_ERROR, "Failed to create profiles directory for migration from basic profile\n%s",
			 error.what());
		return;
	}

	const std::filesystem::path newProfilePath = profilesPath / std::filesystem::u8path(Str("Untitled"));

	for(auto& entry : std::filesystem::directory_iterator(basicPath)) {
		if(entry.is_directory()) {
			continue;
		}

		if(entry.path().filename().u8string() == "scenes.json") {
			continue;
		}

		if(!std::filesystem::exists(newProfilePath)) {
			try {
				std::filesystem::create_directory(newProfilePath);
			} catch(const std::filesystem::filesystem_error& error) {
				blog(LOG_ERROR, "Failed to create profile directory for 'Untitled'\n%s", error.what());
				return;
			}
		}

		const std::filesystem::path destinationFile = newProfilePath / entry.path().filename();

		const auto copyOptions = std::filesystem::copy_options::overwrite_existing;

		try {
			std::filesystem::copy(entry.path(), destinationFile, copyOptions);
		} catch(const std::filesystem::filesystem_error& error) {
			blog(LOG_ERROR, "Failed to copy basic profile file '%s' to new profile 'Untitled'\n%s",
				 entry.path().filename().u8string().c_str(), error.what());

			return;
		}
	}
}

void AFApplicationInitializer::_MoveBasicToSceneCollections()
{
	char path[512];

	if(GetAppConfigPath(path, 512, (LOCAL_FOLDER_NAME + "/basic").c_str()) <= 0) {
		return;
	}

	const std::filesystem::path basicPath = std::filesystem::u8path(path);

	if(!std::filesystem::exists(basicPath)) {
		return;
	}

	const std::filesystem::path sceneCollectionPath =
		CONFIG_CONTEXT.GetUserScenesPath() / std::filesystem::u8path((LOCAL_FOLDER_NAME + "/basic/scenes").c_str());

	if(std::filesystem::exists(sceneCollectionPath)) {
		return;
	}

	try {
		std::filesystem::create_directories(sceneCollectionPath);
	} catch(const std::filesystem::filesystem_error& error) {
		blog(LOG_ERROR,
			 "Failed to create scene collection directory for migration from basic scene collection\n%s",
			 error.what());
		return;
	}

	const std::filesystem::path sourceFile = basicPath / std::filesystem::u8path("scenes.json");
	const std::filesystem::path destinationFile =
		(sceneCollectionPath / std::filesystem::u8path(Str("Untitled"))).replace_extension(".json");

	try {
		std::filesystem::rename(sourceFile, destinationFile);
	} catch(const std::filesystem::filesystem_error& error) {
		blog(LOG_ERROR, "Failed to rename basic scene collection file:\n%s", error.what());
		return;
	}
}

#ifdef _WIN32
void AFApplicationInitializer::_ReleaseRTWorkQ()
{
	if (m_hRtwq)
	{
		typedef HRESULT(STDAPICALLTYPE* PFN_RtwqShutdown)();
		PFN_RtwqShutdown func = (PFN_RtwqShutdown)GetProcAddress(m_hRtwq, "RtwqShutdown");
		func();
		FreeLibrary(m_hRtwq);

		m_hRtwq = NULL;
	}
}
#endif
