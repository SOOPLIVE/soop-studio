#include "CLogManager.h"


#ifdef _WIN32
#include <windows.h>
#endif
#include <wchar.h>
#include <sstream>


#include <util/platform.h>


#include "CoreModel/Log/COBSProfiler.h"


static const int DEF_VA_BUFFER_SIZE = 2048;


void AFLogManager::DeleteOldestFile(bool has_prefix, const char* location)
{
	AFConfigManager& confManager = AFConfigManager::GetSingletonInstance();

	BPtr<char> logDir(confManager.GetConfigPathPtr(location));
	std::string oldestLog;
	uint64_t oldest_ts = (uint64_t)-1;
	struct os_dirent* entry;

	unsigned int maxLogs = (unsigned int)config_get_uint(confManager.GetGlobal(),
		"General", "MaxLogs");

	os_dir_t* dir = os_opendir(logDir);
	if (dir)
	{
		unsigned int count = 0;

		while ((entry = os_readdir(dir)) != NULL)
		{
			if (entry->directory || *entry->d_name == '.')
				continue;

			uint64_t ts =
				_ConvertLogName(has_prefix, entry->d_name);

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
	if (m_bCreatedLogFile == true)
		return;


	std::stringstream dst;

	_GetLastLog(false, "SOOPStudio/logs", m_strLastLogFile);
#ifdef _WIN32
	_GetLastLog(true, "SOOPStudio/crashes", m_strLastCrashLogFile);
#endif

	m_strCurrentLogFile = GenerateTimeDateFilename("txt");
	dst << "SOOPStudio/logs/" << m_strCurrentLogFile.c_str();

	BPtr<char> path(AFConfigManager::GetSingletonInstance().
					GetConfigPathPtr(dst.str().c_str()));

#ifdef _WIN32
	BPtr<wchar_t> wpath;
	os_utf8_to_wcs_ptr(path, 0, &wpath);
	m_LogFile.open(wpath, std::ios_base::in | std::ios_base::out | std:: ios_base::trunc);
#else
	m_LogFile.open(path, std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
#endif

	if (m_LogFile.is_open()) 
	{
		DeleteOldestFile(false, "SOOPStudio/logs");
		base_set_log_handler(_DoLog, &m_LogFile);
	}
	else
		blog(LOG_ERROR, "Failed to open log file");


	m_bCreatedLogFile = true;
}

void AFLogManager::OBSBaseLog(int log_level, const char* format, ...)
{
	va_list tmpList;
	static char buff[DEF_VA_BUFFER_SIZE] = { 0, };

	va_start(tmpList, format);
	vsprintf(buff, format, tmpList);
	va_end(tmpList);

	blog(log_level, buff);
}

void AFLogManager::CreateMainOBSProfiler()
{
	if (m_pMainOBSProfiler == nullptr)
		m_pMainOBSProfiler = new AFOBSProfiler();
}

void AFLogManager::DeleteMainOBSProfiler()
{
	if (m_pMainOBSProfiler != nullptr)
	{
		delete m_pMainOBSProfiler;
		m_pMainOBSProfiler = nullptr;
	}
}

void AFLogManager::_DoLog(int log_level, const char* msg, va_list args, void* param)
{
	auto& confManger = AFConfigManager::GetSingletonInstance();
	AFArgOption* pTmpArgOption = confManger.GetArgOption();

	auto& logManager = AFLogManager::GetSingletonInstance();
	log_handler_t* tmpLogHandlerCallback = logManager.GetDefLogHandler();

	std::fstream& logFile = *static_cast<std::fstream*>(param);
	char str[4096];

#ifndef _WIN32
	va_list args2;
	va_copy(args2, args);
#endif

	vsnprintf(str, sizeof(str), msg, args);

#ifdef _WIN32
	if (IsDebuggerPresent())
	{
		int wNum = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
		if (wNum > 1)
		{
			static std::wstring wide_buf;
			static std::mutex wide_mutex;

			std::lock_guard<std::mutex> lock(wide_mutex);
			wide_buf.reserve(wNum + 1);
			wide_buf.resize(wNum - 1);
			MultiByteToWideChar(CP_UTF8, 0, str, -1, &wide_buf[0],
				wNum);
			wide_buf.push_back('\n');

			OutputDebugStringW(wide_buf.c_str());
		}
	}
#endif

#if !defined(_WIN32) && defined(_DEBUG)
    (*tmpLogHandlerCallback)(log_level, msg, args2, nullptr);
#endif

	if (log_level <= LOG_INFO || pTmpArgOption->GetLogVerbose())
	{
#if !defined(_WIN32) && !defined(_DEBUG)
        (*tmpLogHandlerCallback)(log_level, msg, args2, nullptr);
#endif
		if (!AFLogManager::_TooManyRepeatedEntries(logFile, msg, str))
			AFLogManager::_LogStringChunk(logFile, str, log_level);
	}

#if defined(_WIN32) && defined(OBS_DEBUGBREAK_ON_ERROR)
	if (log_level <= LOG_ERROR && IsDebuggerPresent())
		__debugbreak();
#endif

#ifndef _WIN32
	va_end(args2);
#endif
}

void AFLogManager::_LogString(std::fstream& logFile, const char* timeString, char* str,
							  int log_level)
{
	static std::mutex logfile_mutex;
	std::string msg;
	msg += timeString;
	msg += str;

	logfile_mutex.lock();
	logFile << msg << std::endl;
	logfile_mutex.unlock();

	//if (!!obsLogViewer)
	//	QMetaObject::invokeMethod(obsLogViewer.data(), "AddLine",
	//		Qt::QueuedConnection,
	//		Q_ARG(int, log_level),
	//		Q_ARG(QString, QString(msg.c_str())));
	//
}
uint64_t AFLogManager::_ConvertLogName(bool has_prefix, const char* name)
{
	BaseLexer lex;
	std::string year, month, day, hour, minute, second;

	lexer_start(lex, name);

	if (has_prefix)
	{
		std::string temp;
		if (!_GetToken(lex, temp, BASETOKEN_ALPHA))
			return 0;
	}

	if (!_GetToken(lex, year, BASETOKEN_DIGIT))
		return 0;
	if (!_ExpectToken(lex, "-", BASETOKEN_OTHER))
		return 0;
	if (!_GetToken(lex, month, BASETOKEN_DIGIT))
		return 0;
	if (!_ExpectToken(lex, "-", BASETOKEN_OTHER))
		return 0;
	if (!_GetToken(lex, day, BASETOKEN_DIGIT))
		return 0;
	if (!_GetToken(lex, hour, BASETOKEN_DIGIT))
		return 0;
	if (!_ExpectToken(lex, "-", BASETOKEN_OTHER))
		return 0;
	if (!_GetToken(lex, minute, BASETOKEN_DIGIT))
		return 0;
	if (!_ExpectToken(lex, "-", BASETOKEN_OTHER))
		return 0;
	if (!_GetToken(lex, second, BASETOKEN_DIGIT))
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
	BPtr<char> logDir(AFConfigManager::GetSingletonInstance().
					  GetConfigPathPtr(subdir_to_use));
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
