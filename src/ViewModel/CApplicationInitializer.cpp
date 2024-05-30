#include "CApplicationInitializer.h"


#include <sstream>
#include <fstream>
#include <memory>

#ifdef _WIN32
#include <filesystem>
#else
#include <signal.h>
#include <pthread.h>
#endif


#include <curl/curl.h>


#include <obs.hpp>
#include <util/platform.h>


#include "platform/platform.hpp"

#include "Common/StringMiscUtils.h"
#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CMakeDirectory.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Log/CLogManager.h"


#ifndef _WIN32
    #include "Application/CApplication.h"
#endif


#define MAX_CRASH_REPORT_SIZE (150 * 1024)


AFApplicationInitializer::~AFApplicationInitializer()
{
#ifdef _WIN32
	_ReleaseRTWorkQ();
#endif
}

#ifdef _WIN32
void AFApplicationInitializer::LoadDebugPrivilege()
{
	auto& logManger = AFLogManager::GetSingletonInstance();

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
			logManger.OBSBaseLog(LOG_INFO, "Could not set privilege to "
								 "increase GPU priority");
		}
	}

	CloseHandle(token);
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

	base_get_log_handler(AFLogManager::GetSingletonInstance().GetDefLogHandler(), nullptr);


	AFArgOption* pTmpArgOption = AFConfigManager::GetSingletonInstance().GetArgOption();

	pTmpArgOption->LoadArgProgram(argc, argv);

#if ALLOW_PORTABLE_MODE
	if (pTmpArgOption && pTmpArgOption->GetPortableMode() == false) 
	{
		pTmpArgOption->SetPortableMode(
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

	if (pTmpArgOption && pTmpArgOption->GetDisableMissingFilesCheck() == false)
	{
		pTmpArgOption->SetDisableMissingFilesCheck(
			os_file_exists(BASE_PATH
				"/disable_missing_files_check") ||
			os_file_exists(BASE_PATH
				"/disable_missing_files_check.txt")
		);
	}
#endif


	_CheckSafeModeSentinel();

	//upgrade_settings();

	curl_global_init(CURL_GLOBAL_ALL);
}

void AFApplicationInitializer::AppEntryRelease()
{
	auto& logManger = AFLogManager::GetSingletonInstance();

#ifdef _WIN32
	_ReleaseRTWorkQ();
	log_blocked_dlls();
#endif

	_DeleteSafeModeSentinel();
	logManger.OBSBaseLog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
	base_set_log_handler(nullptr, nullptr);
}

void AFApplicationInitializer::AppSetGlobalConfig(AFMakeDirectory* pDirMaker, bool bStateAppActive)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();

	config_set_default_string(confManager.GetGlobal(), "Basic", "Profile",
							  localeTextManager.Str("Untitled"));
	config_set_default_string(confManager.GetGlobal(), "Basic", "ProfileDir",
							  localeTextManager.Str("Untitled"));
	config_set_default_string(confManager.GetGlobal(), "Basic", "SceneCollection",
							  localeTextManager.Str("Untitled"));
	config_set_default_string(confManager.GetGlobal(), "Basic", "SceneCollectionFile",
							  localeTextManager.Str("Untitled"));
	config_set_default_bool(confManager.GetGlobal(), "Basic", "ConfigOnNewProfile", true);

	if (!config_has_user_value(confManager.GetGlobal(), "Basic", "Profile"))
	{
		config_set_string(confManager.GetGlobal(), "Basic", "Profile",
						  localeTextManager.Str("Untitled"));
		config_set_string(confManager.GetGlobal(), "Basic", "ProfileDir",
					      localeTextManager.Str("Untitled"));
	}

	if (!config_has_user_value(confManager.GetGlobal(), "Basic", "SceneCollection"))
	{
		config_set_string(confManager.GetGlobal(), "Basic", "SceneCollection",
					      localeTextManager.Str("Untitled"));
		config_set_string(confManager.GetGlobal(), "Basic", "SceneCollectionFile",
						  localeTextManager.Str("Untitled"));
	}

#ifdef _WIN32
	bool disableAudioDucking =
		config_get_bool(confManager.GetGlobal(), "Audio", "DisableAudioDucking");
	if (disableAudioDucking)
		DisableAudioDucking(true);
#endif

#ifdef __APPLE__
	if (config_get_bool(confManager.GetGlobal(), "Video", "DisableOSXVSync"))
		EnableOSXVSync(false);
#endif


    confManager.UpdateHotkeyFocusSetting(false, bStateAppActive);

	_MoveBasicToProfiles();
	_MoveBasicToSceneCollections();

    if (pDirMaker && pDirMaker->MakeUserProfileDirs() == false)
		throw "Failed to create profile directories";
}

#ifdef _WIN32

#define		CRASH_MESSAGE																\
			"Woops, SOOPStudio has crashed!\n\nWould you like to copy the crash log "	\
			"to the clipboard? The crash log will still be saved to:\n\n%s"				

void AFApplicationInitializer::_MainCrashHandler(const char* format, va_list args, void*)
{
	char* text = new char[MAX_CRASH_REPORT_SIZE];

	vsnprintf(text, MAX_CRASH_REPORT_SIZE, format, args);
	text[MAX_CRASH_REPORT_SIZE - 1] = 0;

	std::string crashFilePath = "SOOPStudio/crashes";

	AFLogManager::GetSingletonInstance().DeleteOldestFile(true, crashFilePath.c_str());

	std::string name = crashFilePath + "/";
	name += "Crash " + GenerateTimeDateFilename("txt");

	BPtr<char> path(AFConfigManager::GetSingletonInstance().GetConfigPathPtr(name.c_str()));

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

	int ret = MessageBoxA(NULL, finalMessage.c_str(), "SOOPStudio has crashed!",
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

	exit(-1);
}
#endif

void AFApplicationInitializer::_CheckSafeModeSentinel()
{
#ifndef NDEBUG
	/* Safe Mode detection is disabled in Debug builds to keep developers
	 * somewhat sane. */
	return;
#else
	AFArgOption* pTmpArgOption = AFConfigManager::GetSingletonInstance().GetArgOption();

	if (pTmpArgOption->GetDisableShutdownCheck())
		return;

	BPtr sentinelPath = AFConfigManager::GetSingletonInstance().GetConfigPathPtr("SOOPStudio/safe_mode");
	if (os_file_exists(sentinelPath)) {
		//unclean_shutdown = true; 
		return;
	}

	os_quick_write_utf8_file(sentinelPath, nullptr, 0, false);
#endif
}

void AFApplicationInitializer::_DeleteSafeModeSentinel()
{
	BPtr sentinelPath = AFConfigManager::GetSingletonInstance().GetConfigPathPtr("SOOPStudio/safe_mode");
	os_unlink(sentinelPath);
}

void AFApplicationInitializer::_MoveBasicToProfiles()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();

	char path[512];
	char new_path[512];
	os_glob_t* glob;

	/* if not first time use */
	if (confManager.GetConfigPath(path, 512, "SOOPStudio/basic") <= 0)
		return;
	if (!os_file_exists(path))
		return;

	/* if the profiles directory doesn't already exist */
	if (confManager.GetConfigPath(new_path, 512, "SOOPStudio/basic/profiles") <= 0)
		return;
	if (os_file_exists(new_path))
		return;

	if (os_mkdir(new_path) == MKDIR_ERROR)
		return;

	strcat(new_path, "/");
	strcat(new_path, localeTextManager.Str("Untitled"));
	if (os_mkdir(new_path) == MKDIR_ERROR)
		return;

	strcat(path, "/*.*");
	if (os_glob(path, 0, &glob) != 0)
		return;

	strcpy(path, new_path);

	for (size_t i = 0; i < glob->gl_pathc; i++)
	{
		struct os_globent ent = glob->gl_pathv[i];
		char* file;

		if (ent.directory)
			continue;

		file = strrchr(ent.path, '/');
		if (!file++)
			continue;

		if (astrcmpi(file, "scenes.json") == 0)
			continue;

		strcpy(new_path, path);
		strcat(new_path, "/");
		strcat(new_path, file);
		os_rename(ent.path, new_path);
	}

	os_globfree(glob);
}

void AFApplicationInitializer::_MoveBasicToSceneCollections()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();

	char path[512];
	char new_path[512];

	if (confManager.GetConfigPath(path, 512, "SOOPStudio/basic") <= 0)
		return;
	if (!os_file_exists(path))
		return;

	if (confManager.GetConfigPath(new_path, 512, "SOOPStudio/basic/scenes") <= 0)
		return;
	if (os_file_exists(new_path))
		return;

	if (os_mkdir(new_path) == MKDIR_ERROR)
		return;

	strcat(path, "/scenes.json");
	strcat(new_path, "/");
	strcat(new_path, localeTextManager.Str("Untitled"));
	strcat(new_path, ".json");

	os_rename(path, new_path);
}

#ifdef _WIN32
void AFApplicationInitializer::_ReleaseRTWorkQ()
{
	if (m_hRtwq)
	{
		typedef HRESULT(STDAPICALLTYPE* PFN_RtwqShutdown)();
		PFN_RtwqShutdown func =
			(PFN_RtwqShutdown)GetProcAddress(m_hRtwq, "RtwqShutdown");
		func();
		FreeLibrary(m_hRtwq);

		m_hRtwq = NULL;
	}
}
#endif
