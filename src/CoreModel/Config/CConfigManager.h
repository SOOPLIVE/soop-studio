#pragma once

#include <obs.hpp>
#include <util/util.hpp>

#include <qobject.h>

#include <memory>
#include <filesystem>
#include "Common/StudioDefine.h"

#ifdef __APPLE__
#define BASE_PATH ".."
#else
#define BASE_PATH "../.."
#endif

#if defined(ENABLE_PORTABLE_CONFIG) || defined(_WIN32)
#define ALLOW_PORTABLE_MODE 1
#else
#define ALLOW_PORTABLE_MODE 0
#endif

#define SOOP_BROADCAST_NOTICE_DATE_CHECK "HideSoopBroadcastNoticeDate"


// Forward
class AFArgOption;
class AFStateAppContext;

//
class AFConfigManager final
{
public:
    AFConfigManager() {}
    inline ~AFConfigManager() {}

public:
    bool CheckExistingCookieId();

    bool InitGlobal();
    void InitBasic();

    config_t* GetAppConfig() const { return m_appConfig; }
    config_t* GetUserConfig() const { return m_userConfig; }
    config_t* GetActiveConfig() const { return m_activeConfig; }

    void SwapOtherToBasic(ConfigFile& other);
    void SafeSwapOtherToBasic(ConfigFile& other);

    std::filesystem::path GetUserConfigPath() { return m_userConfigLocation; }
    std::filesystem::path GetUserScenesPath() { return m_userScenesLocation; }
    std::filesystem::path GetUserProfilePath() { return m_userProfilesLocation; }

    bool InitGlobalConfigDefaults();
    bool InitGlobalLocationDefaults();
         
    bool InitBasicConfigDefaults();
    void InitBasicConfigDefaults2();

    const char* GetModuleUUID() { return m_moduleUUID.c_str(); };

    bool GetFirstRun() { return m_init_firstrun; };

    void SetBasicProgramConfig();
         
    bool MigrateGlobalSettings();
    void MigrateLegacySettings(uint32_t lastVersion);
         
    bool InitUserConfig(std::filesystem::path& userConfigLocation, uint32_t lastVersion);
    void InitUserConfigDefaults();
    void SetProgramConfig() { SetBasicProgramConfig(); InitUserConfigDefaults(); };
    void ResetProgramConfig();
         
    void SetBasicStreamConfig() {}
    void SetStreamConfig() { SetBasicStreamConfig(); };
    void ResetStreamConfig() {}

    void SetAVOutputConfig(uint32_t cx, uint32_t cy) {
        SetAudioConfig();
        SetVideoConfig(cx, cy);
        SetOutputConfig();
    };
    void ResetAVOutputConfig(uint32_t cx, uint32_t cy) {
        ResetAudioConfig();
        ResetVideoConfig(cx, cy);
        ResetOutputConfig();
    };

    void SetBasicOutputConfig();
    void SetOutputConfig() { SetBasicOutputConfig(); };
    void ResetOutputConfig();

    void SetBasicAudioConfig();
    void SetGlobalAudioConfig();
    void SetAudioConfig() { SetBasicAudioConfig(); SetGlobalAudioConfig(); };
    void ResetAudioConfig();
         
    void SetBasicVideoConfig(uint32_t cx, uint32_t cy);
    void SetGlobalVideoConfig();
    void SetVideoConfig(uint32_t cx, uint32_t cy) { SetBasicVideoConfig(cx, cy); SetGlobalVideoConfig(); };
    void ResetVideoConfig(uint32_t cx, uint32_t cy);
         
    void SetBasicHotkeysConfig() {}
         
    void InitAccessibilityConfig();
    void ResetAccessibilityConfig();
         
    void InitBroadInfoConfig();
    // for sarsa
    void InitSarsaConfig();

    void InitBookmarkMenuConfig();
         
    void SetBasicAdvancedConfig();
    void SetAdvancedConfig() { SetBasicAdvancedConfig(); };
    void SetGlobalAdvancedConfig();
    void ResetAdvancedConfig();

    bool GetFileSafeName(const char* name, std::string& file);
    bool GetClosestUnusedFileName(std::string& path, const char* extension);
    bool GetUnusedName(std::string& name);
         
    bool CheckSavvyTempFolder();
       
    inline std::filesystem::path GetProfileSubPath() const {
        return std::filesystem::u8path(LOCAL_FOLDER_NAME + "/basic/profiles");
    }

    inline std::filesystem::path GetScenesSubPath() const {
        return std::filesystem::u8path(LOCAL_FOLDER_NAME + "/basic/scenes");
    }
private: 
         
    bool _MakeUserProfileDirs();

    bool _UpdatePre22MultiviewLayout(const char* layout);
         
    bool _InitGlobalConfig();
    bool _InitBasicConfig();
         
    void _move_basic_to_profiles(void);
    void _move_basic_to_scene_collections(void);

private:
    std::string m_moduleUUID;

    ConfigFile m_appConfig;
    ConfigFile m_userConfig;
    ConfigFile m_activeConfig;

    std::filesystem::path m_userConfigLocation;
    std::filesystem::path m_userScenesLocation;
    std::filesystem::path m_userProfilesLocation;

    bool m_initedGlobal = false;
    bool m_initedBasic = false;
    bool m_init_firstrun = false;
};

