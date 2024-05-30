#pragma once


#include <random>
#include <string>
#include <chrono>
#include <ratio>

#include "util/util.hpp"
#include "util/platform.h"

static std::string GenId()
{
	std::random_device rd;
	std::mt19937_64 e2(rd());
	std::uniform_int_distribution<uint64_t> dist(0, 0xFFFFFFFFFFFFFFFF);

	uint64_t id = dist(e2);

	char id_str[20];
	snprintf(id_str, sizeof(id_str), "%16llX", (unsigned long long)id);
	return std::string(id_str);
}

static std::string GenerateTimeDateFilename(const char* extension, bool noSpace = false)
{
	time_t now = time(0);
	char file[256] = {0,};
	struct tm* cur_time = nullptr;

	cur_time = localtime(&now);
	snprintf(file, sizeof(file), "%d-%02d-%02d%c%02d-%02d-%02d.%s",
			 cur_time->tm_year + 1900, cur_time->tm_mon + 1,
			 cur_time->tm_mday, noSpace ? '_' : ' ', cur_time->tm_hour,
			 cur_time->tm_min, cur_time->tm_sec, extension);

	return std::string(file);
}


static inline int SumChars(const char* str)
{
	int val = 0;
	for (; *str != 0; str++)
		val += *str;

	return val;
}

static std::string CurrentTimeString()
{
	using namespace std::chrono;

	struct tm tstruct;
	char buf[80];

	auto tp = system_clock::now();
	auto now = system_clock::to_time_t(tp);
	tstruct = *localtime(&now);

	size_t written = strftime(buf, sizeof(buf), "%T", &tstruct);
	if (std::ratio_less<system_clock::period, seconds::period>::value &&
		written && (sizeof(buf) - written) > 5) {
		auto tp_secs = time_point_cast<seconds>(tp);
		auto millis = duration_cast<milliseconds>(tp - tp_secs).count();

		snprintf(buf + written, sizeof(buf) - written, ".%03u",
			static_cast<unsigned>(millis));
	}

	return buf;
}

static std::string CurrentDateTimeString()
{
	time_t now = time(0);
	struct tm tstruct;
	char buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d, %X", &tstruct);
	return buf;
}

inline void remove_reserved_file_characters(std::string& s)
{
	std::replace(s.begin(), s.end(), '\\', '/');
	std::replace(s.begin(), s.end(), '*', '_');
	std::replace(s.begin(), s.end(), '?', '_');
	std::replace(s.begin(), s.end(), '"', '_');
	std::replace(s.begin(), s.end(), '|', '_');
	std::replace(s.begin(), s.end(), ':', '_');
	std::replace(s.begin(), s.end(), '>', '_');
	std::replace(s.begin(), s.end(), '<', '_');
}
static std::string GetFormatString(const char* format, const char* prefix, const char* suffix)
{
	std::string f = format;
	if(prefix && *prefix)
	{
		std::string str_prefix = prefix;
		if(str_prefix.back() != ' ')
			str_prefix += " ";

		size_t insert_pos = 0;
		size_t tmp;

		tmp = f.find_last_of('/');
		if(tmp != std::string::npos && tmp > insert_pos)
			insert_pos = tmp + 1;

		tmp = f.find_last_of('\\');
		if(tmp != std::string::npos && tmp > insert_pos)
			insert_pos = tmp + 1;

		f.insert(insert_pos, str_prefix);
	}

	if(suffix && *suffix) {
		if(*suffix != ' ')
			f += " ";
		f += suffix;
	}

	remove_reserved_file_characters(f);

	return f;
}
static std::string GetFormatExt(const char* container)
{
	std::string ext = container;
	if(ext == "fragmented_mp4")
		ext = "mp4";
	else if(ext == "fragmented_mov")
		ext = "mov";
	else if(ext == "hls")
		ext = "m3u8";
	else if(ext == "mpegts")
		ext = "ts";

	return ext;
}

inline std::string GenerateSpecifiedFilename(const char* extension, bool noSpace, const char* format)
{
	BPtr<char> filename = os_generate_formatted_filename(extension, !noSpace, format);
	return std::string(filename);
}

inline void ensure_directory_exists(std::string& path)
{
	replace(path.begin(), path.end(), '\\', '/');

	size_t last = path.rfind('/');
	if(last == std::string::npos)
		return;

	std::string directory = path.substr(0, last);
	os_mkdirs(directory.c_str());
}
inline void FindBestFilename(std::string& strPath, bool noSpace)
{
	int num = 2;

	if(!os_file_exists(strPath.c_str()))
		return;

	const char* ext = strrchr(strPath.c_str(), '.');
	if(!ext)
		return;

	int extStart = int(ext - strPath.c_str());
	for(;;) {
		std::string testPath = strPath;
		std::string numStr;

		numStr = noSpace ? "_" : " (";
		numStr += std::to_string(num++);
		if(!noSpace)
			numStr += ")";

		testPath.insert(extStart, numStr);

		if(!os_file_exists(testPath.c_str())) {
			strPath = testPath;
			break;
		}
	}
}
static std::string GetOutputFilename(const char* path, const char* container, bool noSpace,
									 bool overwrite, const char* format)
{
	os_dir_t* dir = path && path[0] ? os_opendir(path) : nullptr;

	if(!dir) {
		/*if(main->isVisible())
			OBSMessageBox::warning(main,
						   QTStr("Output.BadPath.Title"),
						   QTStr("Output.BadPath.Text"));
		else
			main->SysTrayNotify(QTStr("Output.BadPath.Text"),
						QSystemTrayIcon::Warning);*/
		return "";
	}

	os_closedir(dir);

	std::string strPath= path;
	char lastChar = strPath.back();
	if(lastChar != '/' && lastChar != '\\')
		strPath += "/";

	std::string ext = GetFormatExt(container);
	strPath += GenerateSpecifiedFilename(ext.c_str(), noSpace, format);
	ensure_directory_exists(strPath);
	if(!overwrite)
		FindBestFilename(strPath, noSpace);

	return strPath;
}
