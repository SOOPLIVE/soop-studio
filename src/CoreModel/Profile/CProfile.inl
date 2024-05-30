#pragma once

#include <functional>
#include <util/platform.h>

#include "CoreModel/Config/CConfigManager.h"


static OBSData GetDataFromJsonFile(const char* jsonFile)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	char fullPath[512] = {0, };
	OBSDataAutoRelease data = nullptr;

	int ret = confManager.GetProfilePath(fullPath, sizeof(fullPath), jsonFile);
	if(ret > 0) {
		BPtr<char> jsonData = os_quick_read_utf8_file(fullPath);
		if(!!jsonData) {
			data = obs_data_create_from_json(jsonData);
		}
	}

	if(!data)
		data = obs_data_create();

	return data.Get();
}
static bool CopyProfile(const char* fromPartial, const char* to)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	os_glob_t* glob = NULL;
	char path[514] = {0, };
	char dir[512] = {0,};;
	int ret = confManager.GetConfigPath(dir, sizeof(dir), "SOOPStudio/basic/profiles/");
	if(ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return false;
	}

	snprintf(path, sizeof(path), "%s%s/*", dir, fromPartial);

	if(os_glob(path, 0, &glob) != 0) {
		blog(LOG_WARNING, "Failed to glob profile '%s'", fromPartial);
		return false;
	}

	for(size_t i = 0; i < glob->gl_pathc; i++) {
		const char* filePath = glob->gl_pathv[i].path;
		if(glob->gl_pathv[i].directory)
			continue;

		ret = snprintf(path, sizeof(path), "%s/%s", to,
				   strrchr(filePath, '/') + 1);
		if(ret > 0) {
			if(os_copyfile(filePath, path) != 0) {
				blog(LOG_WARNING,
					 "CopyProfile: Failed to "
					 "copy file %s to %s",
					 filePath, path);
			}
		}
	}
	os_globfree(glob);

	return true;
}
static void EnumProfiles(std::function<bool(const char*, const char*)>&& cb)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	char path[512] = {0,};
	os_glob_t* glob = NULL;

	int ret = confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/basic/profiles/*");
	if(ret <= 0) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return;
	}

	if(os_glob(path, 0, &glob) != 0) {
		blog(LOG_WARNING, "Failed to glob profiles");
		return;
	}

	for(size_t i = 0; i < glob->gl_pathc; i++)
	{
		const char* filePath = glob->gl_pathv[i].path;
		const char* dirName = strrchr(filePath, '/') + 1;

		if(!glob->gl_pathv[i].directory)
			continue;

		if(strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0)
			continue;

		std::string file = filePath;
		file += "/basic.ini";

		ConfigFile config;
		int ret = config.Open(file.c_str(), CONFIG_OPEN_EXISTING);
		if(ret != CONFIG_SUCCESS)
			continue;

		const char* name = config_get_string(config, "General", "Name");
		if(!name)
			name = strrchr(filePath, '/') + 1;

		if(!cb(name, filePath))
			break;
	}

	os_globfree(glob);
}
static bool GetProfileDir(const char* findName, const char*& profileDir)
{
	bool found = false;
	auto func = [&](const char* name, const char* path) {
		if(strcmp(name, findName) == 0) {
			found = true;
			profileDir = strrchr(path, '/') + 1;
			return false;
		}
		return true;
	};

	EnumProfiles(func);
	return found;
}
static bool ProfileExists(const char* findName)
{
	const char* profileDir = nullptr;
	return GetProfileDir(findName, profileDir);
}
