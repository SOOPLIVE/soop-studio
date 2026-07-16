#include "CProfile.h"

#include "Application/CApplication.h"

#include "Common/SettingsMiscDef.h"
#include "Common/StudioDefine.h"

#include "CoreModel/Config/CConfigManager.h"

#include "MainFrame/Profile/CMainProfile.h"

//
namespace AFProfileUtil
{
    OBSData GetDataFromJsonFile(const char* jsonFile)
    {
        const OBSProfile& currentProfile = MAIN_PROFILE->GetCurrentProfile();
        const std::filesystem::path jsonFilePath = currentProfile.path / std::filesystem::u8path(jsonFile);

        OBSDataAutoRelease data = nullptr;
        if(!jsonFilePath.empty()) {
            BPtr<char> jsonData = os_quick_read_utf8_file(jsonFilePath.u8string().c_str());

            if(!!jsonData) {
                data = obs_data_create_from_json(jsonData);
            }
        }

        if(!data) {
            data = obs_data_create();
        }

        return data.Get();
    }
	bool SetDataToJsonFile(const char* jsonFile, obs_data_t* obsData)
	{
		char full_path[512];
		int ret = GetProfilePath(full_path, sizeof(full_path), jsonFile);
		if(ret > 0) {
			if(obsData) {
				obs_data_save_json_safe(obsData, full_path, "tmp", "bak");

				return true;
			}
		}
		return false;
	}
    bool CopyProfile(const char* fromPartial, const char* to)
    {
        os_glob_t* glob = NULL;
        char path[514] = {0,};
        char dir[512] = {0,};
        int ret = GetAppConfigPath(dir, sizeof(dir), (LOCAL_FOLDER_NAME + "/basic/profiles/").c_str());
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

            const char* fileName = strrchr(filePath, '/');
            if(fileName) {
                fileName++;
            } else {
                fileName = filePath;
            }

            if(strcmp(fileName, "service.json") == 0) {
                continue;
            }

            ret = snprintf(path, sizeof(path), "%s/%s", to, strrchr(filePath, '/') + 1);
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
    void EnumProfiles(std::function<bool(const char*, const char*)>&& cb)
    {
        char path[512] = {0,};
        os_glob_t* glob = NULL;

        int ret = GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/basic/profiles/*").c_str());
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
            file += "/user.ini";

            ConfigFile config;
            ret = config.Open(file.c_str(), CONFIG_OPEN_EXISTING);
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
    bool GetProfileDir(const char* findName, const char*& profileDir)
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
    bool ProfileExists(const char* findName)
    {
        const char* profileDir = nullptr;
        return GetProfileDir(findName, profileDir);
    }
    

    void EnumSceneCollections(std::function<bool(const char*, const char*)>&& cb)
    {
        char path[512];
        os_glob_t* glob;

        int ret = GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/basic/scenes/*.json").c_str());
        if(ret <= 0) {
            blog(LOG_WARNING, "Failed to get config path for scene "
                      "collections");
            return;
        }

        if(os_glob(path, 0, &glob) != 0) {
            blog(LOG_WARNING, "Failed to glob scene collections");
            return;
        }

        for(size_t i = 0; i < glob->gl_pathc; i++) {
            const char* filePath = glob->gl_pathv[i].path;

            if(glob->gl_pathv[i].directory)
                continue;

            OBSDataAutoRelease data =
                obs_data_create_from_json_file_safe(filePath, "bak");
            std::string name = obs_data_get_string(data, "name");

            /* if no name found, use the file name as the name
             * (this only happens when switching to the new version) */
            if(name.empty()) {
                name = strrchr(filePath, '/') + 1;
                name.resize(name.size() - 5);
            }

            if(!cb(name.c_str(), filePath))
                break;
        }

        os_globfree(glob);
    }
    bool SceneCollectionExists(const char* findName)
    {
        bool found = false;
        auto func = [&](const char* name, const char*) {
            if(strcmp(name, findName) == 0) {
                found = true;
                return false;
            }

            return true;
        };

        EnumSceneCollections(func);
        return found;
    }
    void AddMissingFiles(void* data, obs_source_t* source)
    {
        obs_missing_files_t* f = (obs_missing_files_t*)data;
        obs_missing_files_t* sf = obs_source_get_missing_files(source);

        obs_missing_files_append(f, sf);
        obs_missing_files_destroy(sf);
    }
};