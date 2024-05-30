#pragma once


#include <string>


#include <obs.hpp>
#include <util/util.hpp>


#include "CStateAppContext.h"


#ifdef __APPLE__
#define BASE_PATH ".."
#else
#define BASE_PATH "../.."
#endif

#if defined(LINUX_PORTABLE) || defined(_WIN32)
#define ALLOW_PORTABLE_MODE 1
#else
#define ALLOW_PORTABLE_MODE 0
#endif



// Forward
class AFArgOption;



class AFConfigManager final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFConfigManager& GetSingletonInstance()
    {
        static AFConfigManager* instance = nullptr;
        if (instance == nullptr)
        {
            instance = new AFConfigManager;
            instance->_MakeArg();
        }
        return *instance;
    };
    ~AFConfigManager();
private:
    // singleton constructor
    AFConfigManager() = default;
    AFConfigManager(const AFConfigManager&) = delete;
    AFConfigManager(/* rValue */AFConfigManager&& other) noexcept = delete;
    AFConfigManager& operator=(const AFConfigManager&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    inline void                         ResetHotkeyState(bool inFocus);
    bool                                CheckExistingCookieId();

    bool                                InitGlobal();
    void                                InitBasic(uint32_t cntAssocScreen,
                                                  uint32_t cxPrimaryScreen, uint32_t cyPrimaryScreen,
                                                  float devicePixelRatio);


    inline config_t*                    GetGlobal() { return m_GlobalConfig; };
    inline config_t*                    GetBasic() { return m_BasicConfig; };
    inline void                         SafeSaveBasic() { m_BasicConfig.SaveSafe("tmp"); };
    inline void                         SafeSaveGlobal() { m_GlobalConfig.SaveSafe("tmp"); };


    void                                SwapOtherToBasic(ConfigFile& other, uint32_t cntAssocScreen,
                                                             uint32_t cxPrimaryScreen, uint32_t cyPrimaryScreen,
                                                             float devicePixelRatio);
    void                                SafeSwapOtherToBasic(ConfigFile& other, uint32_t cntAssocScreen,
                                                             uint32_t cxPrimaryScreen, uint32_t cyPrimaryScreen,
                                                             float devicePixelRatio);


    int                                 GetConfigPath(char* path, size_t size, const char* name) const;
    char*                               GetConfigPathPtr(const char* name);
    int                                 GetProgramDataPath(char* path, size_t size, const char* name) const;
    char*                               GetProgramDataPathPtr(const char* name);
    int                                 GetProfilePath(char* path, size_t size, const char* file) const;


    bool                                SetDefaultValuesBasicConfig(uint32_t cntAssocScreen,
                                                                    uint32_t cxPrimaryScreen, uint32_t cyPrimaryScreen,
                                                                    float devicePixelRatio);
    void                                SetDefaultValues2BasicConfig();

    const char*                        GetModuleUUID()       { return m_strModuleUUID.c_str(); };

#pragma region config tabs
    void                                SetBasicProgramConfig();
    void                                SetGlobalProgramConfig();
    void                                SetProgramConfig() { SetBasicProgramConfig(); SetGlobalProgramConfig(); };
    void                                ResetProgramConfig();
    
    void                                SetBasicStreamConfig();
    void                                SetStreamConfig() { SetBasicStreamConfig(); };
    void                                ResetStreamConfig();
    
    void                                SetAVOutputConfig(uint32_t cx, uint32_t cy) {   SetAudioConfig();
                                                                                        SetVideoConfig(cx, cy);
                                                                                        SetOutputConfig();};
    void                                ResetAVOutputConfig(uint32_t cx, uint32_t cy) { ResetAudioConfig();
                                                                                        ResetVideoConfig(cx, cy);
                                                                                        ResetOutputConfig();};

    void                                SetBasicOutputConfig();
    void                                SetOutputConfig() { SetBasicOutputConfig(); };
    void                                ResetOutputConfig();
    
    void                                SetBasicAudioConfig();
    void                                SetGlobalAudioConfig();
    void                                SetAudioConfig() { SetBasicAudioConfig(); SetGlobalAudioConfig(); };
    void                                ResetAudioConfig();
    
    void                                SetBasicVideoConfig(uint32_t cx, uint32_t cy);
    void                                SetGlobalVideoConfig();
    void                                SetVideoConfig(uint32_t cx, uint32_t cy) { SetBasicVideoConfig(cx, cy); SetGlobalVideoConfig(); };
    void                                ResetVideoConfig(uint32_t cx, uint32_t cy);

    void                                SetBasicHotkeysConfig();
    
    void                                SetGlobalAccessibilityConfig();
    void                                SetAccessibilityConfig() { SetGlobalAccessibilityConfig(); };
    void                                ResetAccessibilityConfig();
    
    void                                SetBasicAdvancedConfig();
    void                                SetAdvancedConfig() { SetBasicAdvancedConfig(); };
    void                                SetGlobalAdvancedConfig();
    void                                ResetAdvancedConfig();
#pragma endregion

    bool                                GetEnableHotkeysInFocus() { return m_StatesApp.GetEnableHotkeysInFocus(); }
    bool                                GetEnableHotkeysOutOfFocus() { return m_StatesApp.GetEnableHotkeysOutOfFocus(); }
    void                                UpdateHotkeyFocusSetting(bool resetState, bool appActiveState);
    void                                DisableHotkeys(bool appActiveState);

    bool                                GetFileSafeName(const char* name, std::string& file);
    bool                                GetClosestUnusedFileName(std::string& path, const char* extension);
    bool                                GetUnusedSceneCollectionFile(std::string& name, std::string& file);

    AFArgOption*                        GetArgOption() { return m_pArgOption; };
    AFStateAppContext*                  GetStates() { return &m_StatesApp; };
#pragma endregion public func

#pragma region private func
private:
    void                                _MakeArg();


    bool                                _do_mkdir(const char* path);
    bool                                _MakeUserDirs();
    bool                                _MakeUserProfileDirs();

    std::string                         _GetSceneCollectionFileFromName(const char* name);
    std::string                         _GetProfileDirFromName(const char* name);



    bool                                _UpdateNvencPresets();

    void                                _SetDefaultValuesGlobalConfig();
    bool                                _InitGlobalConfig();
    bool                                _InitBasicConfig(uint32_t cntAssocScreen,
                                                         uint32_t cxPrimaryScreen, uint32_t cyPrimaryScreen,
                                                         float devicePixelRatio);

    void                                _move_basic_to_profiles(void);
    void                                _move_basic_to_scene_collections(void);
#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
    std::string                         m_strStartingCollection;
    std::string                         m_strStartingProfile;
    std::string                         m_strStartingScene;
    std::string                         m_strModuleUUID;


    AFArgOption*                        m_pArgOption = nullptr;
    AFStateAppContext                   m_StatesApp;
    ConfigFile                          m_GlobalConfig;
    ConfigFile                          m_BasicConfig;


    bool                                m_bInitedGlobal = false;
    bool                                m_bInitedBasic = false;
#pragma endregion private member var
};

inline void AFConfigManager::ResetHotkeyState(bool inFocus)
{
    obs_hotkey_enable_background_press(
        (inFocus && m_StatesApp.GetEnableHotkeysInFocus()) ||
        (!inFocus && m_StatesApp.GetEnableHotkeysOutOfFocus())
    );
};
