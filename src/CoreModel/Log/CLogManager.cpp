
#include "CLogManager.h"

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32
#include <wchar.h>
#include <sstream>


#include <util/platform.h>

#include "Common/StringMiscUtils.h"
#include "Common/StudioDefine.h"

#include "Application/CApplication.h"

#include "COBSProfiler.h"
#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Config/CConfigManager.h"


static const int DEF_VA_BUFFER_SIZE = 2048;


AFLogManager::AFLogManager()
	:m_profiler(std::make_unique<AFOBSProfiler>())
{}
AFLogManager::~AFLogManager() {}

void AFLogManager::DeleteOldestFile(bool has_prefix, const char* location)
{
	BPtr<char> logDir(GetAppConfigPathPtr(location));
	std::string oldestLog;
	uint64_t oldest_ts = (uint64_t)-1;
	struct os_dirent* entry;

	unsigned int maxLogs = (unsigned int)config_get_uint(APPCONFIG, "General", "MaxLogs");

	if (maxLogs < 30)
	{
		maxLogs = 30;
		config_set_int(APPCONFIG, "General", "MaxLogs", 30);
		config_save_safe(APPCONFIG, "tmp", nullptr);
	}

	os_dir_t* dir = os_opendir(logDir);
	if (dir)
	{
		unsigned int count = 0;

		while ((entry = os_readdir(dir)) != NULL)
		{
			if (entry->directory || *entry->d_name == '.')
				continue;

			uint64_t ts = _ConvertLogName(has_prefix, entry->d_name);
			if (ts)
			{
				if (ts < oldest_ts)
				{
					oldestLog = entry->d_name;
					oldest_ts = ts;
				}

				count++;
			}
		}

		os_closedir(dir);

		if (count > maxLogs)
		{
			std::stringstream delPath;

			delPath << logDir << "/" << oldestLog;
			os_unlink(delPath.str().c_str());
		}
	}
}

void AFLogManager::CreateLogFile()
{
	if (m_createdLogFile == true)
		return;


	std::stringstream dst;
	std::stringstream dst_api;

	_GetLastLog(false, (LOCAL_FOLDER_NAME + "/logs").c_str(), m_lastLogFile);
	_GetLastLog(false, (LOCAL_FOLDER_NAME + "/logs/api").c_str(), m_lastAPILogFile);
#ifdef _WIN32
	_GetLastLog(true, (LOCAL_FOLDER_NAME + "/crashes").c_str(), m_lastCrashLogFile);
#endif

	m_currentAPILogFile = m_currentLogFile = GenerateTimeDateFilename("txt");
	dst << LOCAL_FOLDER_NAME + "/logs/" << m_currentLogFile.c_str();
	dst_api << LOCAL_FOLDER_NAME + "/logs/api/" << m_currentAPILogFile.c_str();

	BPtr<char> path(GetAppConfigPathPtr(dst.str().c_str()));
	BPtr<char> path_api(GetAppConfigPathPtr(dst_api.str().c_str()));

#ifdef _WIN32
	BPtr<wchar_t> wpath;
	os_utf8_to_wcs_ptr(path, 0, &wpath);
	m_logFile.open(wpath, std::ios_base::in | std::ios_base::out | std:: ios_base::trunc);


	BPtr<wchar_t> wpath_api;
	os_utf8_to_wcs_ptr(path_api, 0, &wpath_api);
	m_logAPIFile.open(wpath_api, std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

#else
	m_logFile.open(path, std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
#endif

	if (m_logFile.is_open()) 
	{
		DeleteOldestFile(false, (LOCAL_FOLDER_NAME + "/logs").c_str());
		base_set_log_handler(AFLogUtil::DoLog, &m_logFile);
	}
	else
		blog(LOG_ERROR, "Failed to open log file");

	if (m_logAPIFile.is_open())
	{
		DeleteOldestFile(false, (LOCAL_FOLDER_NAME + "/logs/api").c_str());
		base_set_api_log_handler(AFLogUtil::DoLog, &m_logAPIFile);
	}
	else
		blog(LOG_ERROR, "Failed to open log file");


	m_createdLogFile = true;
}

uint64_t AFLogManager::_ConvertLogName(bool has_prefix, const char* name)
{
	BaseLexer lex;
	std::string year, month, day, hour, minute, second;

	lexer_start(lex, name);

	if(has_prefix)
	{
		std::string temp;
		if(!_GetToken(lex, temp, BASETOKEN_ALPHA))
			return 0;
	}

	if(!_GetToken(lex, year, BASETOKEN_DIGIT))
		return 0;
	if(!_ExpectToken(lex, "-", BASETOKEN_OTHER))
		return 0;
	if(!_GetToken(lex, month, BASETOKEN_DIGIT))
		return 0;
	if(!_ExpectToken(lex, "-", BASETOKEN_OTHER))
		return 0;
	if(!_GetToken(lex, day, BASETOKEN_DIGIT))
		return 0;
	if(!_GetToken(lex, hour, BASETOKEN_DIGIT))
		return 0;
	if(!_ExpectToken(lex, "-", BASETOKEN_OTHER))
		return 0;
	if(!_GetToken(lex, minute, BASETOKEN_DIGIT))
		return 0;
	if(!_ExpectToken(lex, "-", BASETOKEN_OTHER))
		return 0;
	if(!_GetToken(lex, second, BASETOKEN_DIGIT))
		return 0;

	std::stringstream timestring;
	timestring << year << month << day << hour << minute << second;
	return std::stoull(timestring.str());
}

bool AFLogManager::_GetToken(lexer* lex, std::string& str, base_token_type type)
{
	base_token token;
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != type)
		return false;

	str.assign(token.text.array, token.text.len);
	return true;
}

bool AFLogManager::_ExpectToken(lexer* lex, const char* str, base_token_type type)
{
	base_token token;
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != type)
		return false;

	return strref_cmp(&token.text, str) == 0;
}

void AFLogManager::_GetLastLog(bool has_prefix, const char* subdir_to_use, std::string& last)
{
	BPtr<char> logDir(GetAppConfigPathPtr(subdir_to_use));
	struct os_dirent* entry;
	os_dir_t* dir = os_opendir(logDir);
	uint64_t highest_ts = 0;

	if (dir)
	{
		while ((entry = os_readdir(dir)) != NULL)
		{
			if (entry->directory || *entry->d_name == '.')
				continue;

			uint64_t ts =
				_ConvertLogName(has_prefix, entry->d_name);

			if (ts > highest_ts)
			{
				last = entry->d_name;
				highest_ts = ts;
			}
		}

		os_closedir(dir);
	}
}

namespace AFLogUtil
{
#define MAX_REPEATED_LINES 30
#define MAX_CHAR_VARIATION (255 * 3)

	void DoLog(int log_level, const char* msg, va_list args, void* param)
	{
		log_handler_t* tmpLogHandlerCallback = LOGMANAGER.GetDefLogHandler();

		std::fstream& logFile = *static_cast<std::fstream*>(param);
		char str[8192];

#ifndef _WIN32
		va_list args2;
		va_copy(args2, args);
#endif

		vsnprintf(str, sizeof(str), msg, args);

#ifdef _WIN32
		if(IsDebuggerPresent())
		{
			int wNum = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
			if(wNum > 1)
			{
				static std::wstring wide_buf;
				static std::mutex wide_mutex;

				std::lock_guard<std::mutex> lock(wide_mutex);
				wide_buf.reserve(wNum + 1);
				wide_buf.resize(wNum - 1);
				MultiByteToWideChar(CP_UTF8, 0, str, -1, &wide_buf[0], wNum);
				wide_buf.push_back('\n');

				OutputDebugStringW(wide_buf.c_str());
			}
		}
#endif

#if !defined(_WIN32) && defined(_DEBUG)
		(*tmpLogHandlerCallback)(log_level, msg, args2, nullptr);
#endif

		if(log_level <= LOG_INFO || ARGOPTION.GetLogVerbose())
		{
#if !defined(_WIN32) && !defined(_DEBUG)
			(*tmpLogHandlerCallback)(log_level, msg, args2, nullptr);
#endif
			if(!TooManyRepeatedEntries(logFile, msg, str))
				LogStringChunk(logFile, str, log_level);
		}

#if defined(_WIN32) && defined(OBS_DEBUGBREAK_ON_ERROR)
		if(log_level <= LOG_ERROR && IsDebuggerPresent())
			__debugbreak();
#endif

#ifndef _WIN32
		va_end(args2);
#endif
	}

	inline bool TooManyRepeatedEntries(std::fstream& logFile, const char* msg, const char* output_str)
	{
		static std::mutex log_mutex;
		static const char* last_msg_ptr = nullptr;
		static int last_char_sum = 0;
		static char cmp_str[4096];
		static int rep_count = 0;

		int new_sum = SumChars(output_str);

		std::lock_guard<std::mutex> guard(log_mutex);

		if(ARGOPTION.GetUnfilteredLog())
			return false;


		if(last_msg_ptr == msg)
		{
			int diff = std::abs(new_sum - last_char_sum);
			if(diff < MAX_CHAR_VARIATION)
				return (rep_count++ >= MAX_REPEATED_LINES);
		}

		if(rep_count > MAX_REPEATED_LINES)
		{
			logFile << CurrentTimeString()
				<< ": Last log entry repeated for "
				<< std::to_string(rep_count - MAX_REPEATED_LINES)
				<< " more lines" << std::endl;
		}

		last_msg_ptr = msg;
		strcpy(cmp_str, output_str);
		last_char_sum = new_sum;
		rep_count = 0;

		return false;
	}

	inline void LogStringChunk(std::fstream& logFile, char* str, int log_level)
	{
		char* nextLine = str;
		std::string timeString = CurrentTimeString();
		timeString += ": ";

		while(*nextLine) {
			nextLine = strchr(str, '\n');
			if(!nextLine)
				break;

			if(nextLine != str && nextLine[-1] == '\r') {
				nextLine[-1] = 0;
			} else {
				nextLine[0] = 0;
			}

			LogString(logFile, timeString.c_str(), str, log_level);
			nextLine++;
			str = nextLine;
		}

		LogString(logFile, timeString.c_str(), str, log_level);
	}

	void LogString(std::fstream& logFile, const char* timeString, char* str, int log_level)
	{
		static std::mutex logfile_mutex;
		std::string msg;
		msg += timeString;
		msg += str;

		logfile_mutex.lock();
		logFile << msg << std::endl;
		logfile_mutex.unlock();
	}
};