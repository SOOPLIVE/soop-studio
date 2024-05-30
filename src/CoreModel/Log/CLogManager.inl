#pragma once


#include <mutex>


#include "Common/StringMiscUtils.h"
#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Config/CConfigManager.h"

#define MAX_REPEATED_LINES 30
#define MAX_CHAR_VARIATION (255 * 3)



inline bool AFLogManager::_TooManyRepeatedEntries(std::fstream& logFile, const char* msg, const char* output_str)
{
	auto& confManger = AFConfigManager::GetSingletonInstance();
	AFArgOption* pTmpArgOption = confManger.GetArgOption();


	static std::mutex log_mutex;
	static const char* last_msg_ptr = nullptr;
	static int last_char_sum = 0;
	static char cmp_str[4096];
	static int rep_count = 0;

	int new_sum = SumChars(output_str);

	std::lock_guard<std::mutex> guard(log_mutex);

	if (pTmpArgOption->GetUnfilteredLog()) 
		return false;
	

	if (last_msg_ptr == msg) 
	{
		int diff = std::abs(new_sum - last_char_sum);
		if (diff < MAX_CHAR_VARIATION) 
			return (rep_count++ >= MAX_REPEATED_LINES);
	}

	if (rep_count > MAX_REPEATED_LINES) 
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

inline void AFLogManager::_LogStringChunk(std::fstream& logFile, char* str, int log_level)
{
	char* nextLine = str;
	std::string timeString = CurrentTimeString();
	timeString += ": ";

	while (*nextLine) {
		char* nextLine = strchr(str, '\n');
		if (!nextLine)
			break;

		if (nextLine != str && nextLine[-1] == '\r') {
			nextLine[-1] = 0;
		}
		else {
			nextLine[0] = 0;
		}

		_LogString(logFile, timeString.c_str(), str, log_level);
		nextLine++;
		str = nextLine;
	}

	_LogString(logFile, timeString.c_str(), str, log_level);
}