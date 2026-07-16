#pragma once

#include <obs.hpp>

#include <functional>

static const char* limitFrameUserList[] = {
    "",
};

namespace AFProfileUtil {
    OBSData GetDataFromJsonFile(const char* jsonFile);
	bool SetDataToJsonFile(const char* jsonFile, obs_data_t* obsData);
    bool CopyProfile(const char* fromPartial, const char* to);
    void EnumProfiles(std::function<bool(const char*, const char*)>&& cb);
    bool GetProfileDir(const char* findName, const char*& profileDir);
    bool ProfileExists(const char* findName);

    void EnumSceneCollections(std::function<bool(const char*, const char*)>&& cb);
    bool SceneCollectionExists(const char* findName);

    void AddMissingFiles(void* data, obs_source_t* source);
};