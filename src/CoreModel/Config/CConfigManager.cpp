#include "CConfigManager.h"

#include "Application/CApplication.h"

#include "qt-wrappers.hpp"

#include <util/profiler.hpp>

#include "Common/StringMiscUtils.h"
#include "Common/SettingsMiscDef.h"
#include "Common/StudioDefine.h"

#include "CArgOption.h"
#include "CStateAppContext.h"
#include "CMakeDirectory.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Profile/CProfile.h"
#include "CoreModel/Encoder/CEncoder.h"

#include "platform/platform.hpp"

#include "PopupWindows/SettingPopup/CSettingAccessibilityAreaWidget.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/Profile/CMainProfile.h"
#include "MainFrame/SceneCollection/CMainSceneCollection.h"


static const double scaled_vals[] = {1.0,        1.25, (1.0 / 0.75), 1.5,
                                    (1.0 / 0.6), 1.75, 2.0,          2.25,
                                     2.5,        2.75, 3.0,          0.0};


#ifdef __APPLE__
#define DEFAULT_CONTAINER "fragmented_mov"
#elif OBS_RELEASE_CANDIDATE == 0 && OBS_BETA == 0
#define DEFAULT_CONTAINER "mkv"
#else
#define DEFAULT_CONTAINER "hybrid_mp4"
#endif


inline void GetScreenInfo(uint32_t& screenCount,
                          uint32_t& primaryScreenWidth, uint32_t& primaryScreenHeight,
                          float& devicePixelRatio)
{
    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    uint32_t cx = primaryScreen->size().width();
    uint32_t cy = primaryScreen->size().height();
    //
    QList<QScreen*> screens = QGuiApplication::screens();
    uint32_t cntScreen = (uint32_t)screens.count();
    //
    float pixelRatio = MAINFRAME->devicePixelRatioF();
    
    screenCount = cntScreen;
    primaryScreenWidth = cx;
    primaryScreenHeight = cy;
    devicePixelRatio = pixelRatio;
}

//
bool AFConfigManager::CheckExistingCookieId()
{
    if(m_initedBasic == false)
        return false;

    if(config_has_user_value(m_activeConfig, "Panels", "CookieId"))
        return true;

    config_set_string(m_activeConfig, "Panels", "CookieId", GenId().c_str());

    return true;
}

bool AFConfigManager::InitGlobal()
{
    if(m_initedGlobal == false)
    {
        bool res = _InitGlobalConfig();
        if(res)
            m_initedGlobal = true;

        return res;
    }

    return true;
}
void AFConfigManager::InitBasic()
{
    if(!m_initedBasic)
    {
        bool res = _InitBasicConfig();
        if(res)
            m_initedBasic = true;
    }
}

void AFConfigManager::SwapOtherToBasic(ConfigFile& other)
{
    other.Swap(m_activeConfig);
    InitBasicConfigDefaults();
}

void AFConfigManager::SafeSwapOtherToBasic(ConfigFile& other)
{
    m_activeConfig.SaveSafe("tmp");
    other.SaveSafe("tmp");
    SwapOtherToBasic(other);
}

bool AFConfigManager::InitGlobalConfigDefaults()
{
    config_set_default_uint(m_appConfig, "General", "MaxLogs", 30);
    config_set_default_int(m_appConfig, "General", "InfoIncrement", -1);
    config_set_default_string(m_appConfig, "General", "ProcessPriority", "AboveNormal");
    config_set_default_bool(m_appConfig, "General", "EnableAutoUpdates", true);

#if _WIN32
    config_set_default_string(m_appConfig, "Video", "Renderer", "Direct3D 11");
#else
    config_set_default_string(m_appConfig, "Video", "Renderer", "OpenGL");
#endif

#ifdef _WIN32
    config_set_default_bool(m_appConfig, "Audio", "DisableAudioDucking", true);
    config_set_default_bool(m_appConfig, "General", "BrowserHWAccel", true);
#endif

#ifdef __APPLE__
    config_set_default_bool(m_appConfig, "General", "BrowserHWAccel", true);
    config_set_default_bool(m_appConfig, "Video", "DisableOSXVSync", true);
    config_set_default_bool(m_appConfig, "Video", "ResetOSXVSyncOnExit", true);
#endif

    config_set_default_int(m_appConfig, "Audio", "MainAudioVolume", 4096);
    config_set_default_int(m_appConfig, "Audio", "MainMicVolume", 4096);
    config_set_default_bool(m_appConfig, "Audio", "MainAudioMute", false);
    config_set_default_bool(m_appConfig, "Audio", "MainMicMute", false);
    //
    SetGlobalAudioConfig();
    SetGlobalVideoConfig();

    return true;
}
bool AFConfigManager::InitGlobalLocationDefaults()
{
    char path[512];

    int len = GetAppConfigPath(path, sizeof(path), nullptr);
    if(len <= 0) {
        OBSErrorBox(NULL, "Unable to get global configuration path.");
        return false;
    }

    config_set_default_string(m_appConfig, "Locations", "Configuration", path);
    config_set_default_string(m_appConfig, "Locations", "SceneCollections", path);
    config_set_default_string(m_appConfig, "Locations", "Profiles", path);
    // fixed %appdata% path
    if(!os_file_exists(config_get_string(m_appConfig, "Locations", "Configuration"))) {
        config_set_string(m_appConfig, "Locations", "Configuration", path);
        config_set_string(m_appConfig, "Locations", "SceneCollections", path);
        config_set_string(m_appConfig, "Locations", "Profiles", path);
    }

    return true;
}

bool AFConfigManager::InitBasicConfigDefaults()
{
    if(m_initedGlobal == false  /*|| m_initedBasic == false*/)
        return false;

    uint32_t cntScreen = 0;
    uint32_t cxPrimaryScreen = 0;
    uint32_t cyPrimaryScreen = 0;
    float devicePixelRatio = .0f;
    GetScreenInfo(cntScreen, cxPrimaryScreen, cyPrimaryScreen, devicePixelRatio);
    //
    if(cntScreen == 0)
    {
        OBSErrorBox(NULL, "There appears to be no monitors.  Er, this "
                   "technically shouldn't be possible.");
        return false;
    }

    uint32_t cx = 1280, cy = 720;
    //uint32_t cx = cxPrimaryScreen, cy = cyPrimaryScreen;

    //cx *= devicePixelRatio;
    //cy *= devicePixelRatio;

    //bool oldResolutionDefaults = config_get_bool(m_userConfig, "General", "Pre19Defaults");

    ///* use 1920x1080 for new default base res if main monitor is above
    // * 1920x1080, but don't apply for people from older builds -- only to
    // * new users */
    //if(!oldResolutionDefaults && (cx * cy) > (1920 * 1080))
    //{
    //    cx = 1920;
    //    cy = 1080;
    //}

    SetBasicOutputConfig();
    SetBasicAudioConfig();
    SetBasicVideoConfig(cx, cy);

    bool changed = false;

    /* ----------------------------------------------------- */
    /* set twitch chat extensions to "both" if prev version  */
    /* is under 24.1                                         */
    if(config_get_bool(m_userConfig, "General", "Pre24.1Defaults") &&
       !config_has_user_value(m_activeConfig, "Twitch", "AddonChoice"))
    {
        config_set_int(m_activeConfig, "Twitch", "AddonChoice", 3);
        changed = true;
    }

    if(changed) {
        m_activeConfig.SaveSafe("tmp");
    }

    CheckExistingCookieId();

    return true;
}

static const char* GetDefaultSimpleEncoder(bool oldEncDefaults)
{
    if (oldEncDefaults)
        return SIMPLE_ENCODER_X264;

    if (AFEncoderUtil::EncoderAvailable("ffmpeg_nvenc"))
        return SIMPLE_ENCODER_NVENC;

    if (AFEncoderUtil::EncoderAvailable("h264_texture_amf"))
        return SIMPLE_ENCODER_AMD;

    if (AFEncoderUtil::EncoderAvailable("obs_qsv11"))
        return SIMPLE_ENCODER_QSV;

    return SIMPLE_ENCODER_X264;
}

static const char* GetDefaultAdvEncoder(bool oldEncDefaults)
{
    if (oldEncDefaults)
        return "obs_x264";

    if (AFEncoderUtil::EncoderAvailable("ffmpeg_nvenc"))
        return "obs_nvenc_h264_tex";

    if (AFEncoderUtil::EncoderAvailable("h264_texture_amf"))
        return "h264_texture_amf";

    if (AFEncoderUtil::EncoderAvailable("obs_qsv11"))
        return "obs_qsv11";

    return "obs_x264";
}

void AFConfigManager::InitBasicConfigDefaults2()
{
    bool oldEncDefaults = config_get_bool(m_userConfig, "General", "Pre23Defaults");
    const char* defaultSimpleEncoder = GetDefaultSimpleEncoder(oldEncDefaults);

    config_set_default_string(m_activeConfig, "SimpleOutput", "StreamEncoder",
                        defaultSimpleEncoder);
    config_set_default_string(m_activeConfig, "SimpleOutput", "RecEncoder",
                        defaultSimpleEncoder);

    const char* aac_default = "ffmpeg_aac";
    if(AFEncoderUtil::EncoderAvailable("CoreAudio_AAC"))
        aac_default = "CoreAudio_AAC";
    else if(AFEncoderUtil::EncoderAvailable("libfdk_aac"))
        aac_default = "libfdk_aac";

    config_set_default_string(m_activeConfig, "AdvOut", "AudioEncoder", aac_default);
    config_set_default_string(m_activeConfig, "AdvOut", "RecAudioEncoder", aac_default);

    const char* defaultAdvEncoder = GetDefaultAdvEncoder(oldEncDefaults);
    config_set_default_string(m_activeConfig, "AdvOut", "Encoder", defaultAdvEncoder);
}

void AFConfigManager::SetBasicProgramConfig()
{
    config_set_default_bool(m_activeConfig, "General", "OpenStatsOnStartup", false);
    m_activeConfig.SaveSafe("tmp");
}

static std::string GetOBSGlobalIniPath() {
    return "/" + LOCAL_FOLDER_NAME + "/global.ini";
}
static std::string GetOBSUserIniPath() {
    return "/" + LOCAL_FOLDER_NAME + "/user.ini";
}

bool AFConfigManager::MigrateGlobalSettings()
{
    char path[512];

    int len = GetAppConfigPath(path, sizeof(path), nullptr);
    if(len <= 0) {
        OBSErrorBox(nullptr, "Unable to get global configuration path.");
        return false;
    }

    const std::string OBSGlobalIniPath = GetOBSGlobalIniPath();
    const std::string OBSUserIniPath = GetOBSUserIniPath();

    std::string legacyConfigFileString;
    legacyConfigFileString.reserve(strlen(path) + OBSGlobalIniPath.size());
    legacyConfigFileString.append(path).append(OBSGlobalIniPath);

    const std::filesystem::path legacyGlobalConfigFile = std::filesystem::u8path(legacyConfigFileString);

    std::string configFileString;
    configFileString.reserve(strlen(path) + OBSUserIniPath.size());
    configFileString.append(path).append(OBSUserIniPath);

    const std::filesystem::path userConfigFile = std::filesystem::u8path(configFileString);

    if(std::filesystem::exists(userConfigFile)) {
        OBSErrorBox(nullptr,
                   "Unable to migrate global configuration - user configuration file already exists.");
        return false;
    }

    try {
        std::filesystem::copy(legacyGlobalConfigFile, userConfigFile);
    } catch(const std::filesystem::filesystem_error&) {
        OBSErrorBox(nullptr, "Unable to migrate global configuration - copy failed.");
        return false;
    }

    return true;
}
void AFConfigManager::MigrateLegacySettings(uint32_t lastVersion)
{
    bool hasChanges = false;

    const uint32_t v19 = MAKE_SEMANTIC_VERSION(19, 0, 0);
    const uint32_t v21 = MAKE_SEMANTIC_VERSION(21, 0, 0);
    const uint32_t v23 = MAKE_SEMANTIC_VERSION(23, 0, 0);
    const uint32_t v24 = MAKE_SEMANTIC_VERSION(24, 0, 0);
    const uint32_t v24_1 = MAKE_SEMANTIC_VERSION(24, 1, 0);

    const std::map<uint32_t, std::string> defaultsMap {
        {{v19, "Pre19Defaults"}, {v21, "Pre21Defaults"}, {v23, "Pre23Defaults"}, {v24_1, "Pre24.1Defaults"}}};

    for(auto& [version, configKey] : defaultsMap) {
        if(!config_has_user_value(m_userConfig, "General", configKey.c_str())) {
            bool useOldDefaults = lastVersion && lastVersion < version;
            config_set_bool(m_userConfig, "General", configKey.c_str(), useOldDefaults);

            hasChanges = true;
        }
    }

    /*if(config_has_user_value(m_userConfig, "BasicWindow", "MultiviewLayout")) {
        const char* layout = config_get_string(m_userConfig, "BasicWindow", "MultiviewLayout");
        bool layoutUpdated = _UpdatePre22MultiviewLayout(layout);
        hasChanges = hasChanges | layoutUpdated;
    }*/

    if(lastVersion && lastVersion < v24) {
        bool disableHotkeysInFocus = config_get_bool(m_userConfig, "General", "DisableHotkeysInFocus");

        if(disableHotkeysInFocus) {
            config_set_string(m_userConfig, "General", "HotkeyFocusType", "DisableHotkeysInFocus");
        }

        hasChanges = true;
    }

    if(hasChanges) {
        m_userConfig.SaveSafe("tmp");
    }
}

bool AFConfigManager::InitUserConfig(std::filesystem::path& userConfigLocation, uint32_t lastVersion)
{
    ProfileScope("AFConfigManager::InitUserConfig");
    //
    const std::string userConfigFile = userConfigLocation.u8string() + "/" + LOCAL_FOLDER_NAME  + "/user.ini";

    int errorCode = m_userConfig.Open(userConfigFile.c_str(), CONFIG_OPEN_ALWAYS);

    if(errorCode != CONFIG_SUCCESS) {
        OBSErrorBox(nullptr, "Failed to open user.ini: %d", errorCode);
        return false;
    }

    MigrateLegacySettings(lastVersion);
    InitUserConfigDefaults();

    return true;
}
void AFConfigManager::InitUserConfigDefaults()
{
    InitAccessibilityConfig();
    InitBroadInfoConfig();
    InitSarsaConfig();
    InitBookmarkMenuConfig();
    //
    config_set_default_bool(m_userConfig, "General", "ConfirmOnExit", true);
    config_set_default_string(m_userConfig, "General", "HotkeyFocusType", "NeverDisableHotkeys");

    config_set_default_bool(m_userConfig, "BasicWindow", "PreviewEnabled", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "PreviewProgramMode", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "SceneDuplicationMode", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "SwapScenesMode", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "SnappingEnabled", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "ScreenSnapping", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "SourceSnapping", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "CenterSnapping", false);
    config_set_default_double(m_userConfig, "BasicWindow", "SnapDistance", 20.0);
    config_set_default_bool(m_userConfig, "BasicWindow", "SpacingHelpersEnabled", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "RecordWhenStreaming", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "KeepRecordingWhenStreamStops", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "SysTrayEnabled", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "SysTrayWhenStarted", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "SaveProjectors", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "ShowTransitions", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "ShowListboxToolbars", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "ShowStatusBar", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "ShowSourceIcons", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "ShowContextToolbars", true);
    config_set_default_bool(m_userConfig, "BasicWindow", "StudioModeLabels", true);

    config_set_default_bool(m_userConfig, "BasicWindow", "VerticalVolControl", false);

    config_set_default_bool(m_userConfig, "BasicWindow", "MultiviewMouseSwitch", true);

    config_set_default_bool(m_userConfig, "BasicWindow", "MultiviewDrawNames", true);

    config_set_default_bool(m_userConfig, "BasicWindow", "MultiviewDrawAreas", true);

    config_set_default_bool(m_userConfig, "BasicWindow", "MediaControlsCountdownTimer", true);
    
    config_set_default_bool(m_userConfig, "BasicWindow", "ShowPopupLeft", true);
    // -------------------------------------

    config_set_default_bool(m_userConfig, "BasicWindow", "WarnBeforeStartingStream", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "WarnBeforeStoppingStream", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "WarnBeforeStoppingRecord", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "OverflowHidden", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "OverflowAlwaysVisible", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "OverflowSelectionHidden", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "ShowSafeAreas", false);
    config_set_default_bool(m_userConfig, "General", "AutomaticCollectionSearch", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "TransitionOnDoubleClick", false);
    config_set_default_bool(m_userConfig, "BasicWindow", "StudioPortraitLayout", false);
    //config_set_default_bool(m_userConfig, "BasicWindow", "ShowVirtualCamToolip", true);
    m_userConfig.SaveSafe("tmp");
}
void AFConfigManager::ResetProgramConfig()
{
    config_remove_value(m_userConfig, "General", "ConfirmOnExit");
    config_remove_value(m_userConfig, "General", "HotkeyFocusType");

    config_remove_value(m_userConfig, "BasicWindow", "PreviewEnabled");
    config_remove_value(m_userConfig, "BasicWindow", "PreviewProgramMode");
    config_remove_value(m_userConfig, "BasicWindow", "SceneDuplicationMode");
    config_remove_value(m_userConfig, "BasicWindow", "SwapScenesMode");
    config_remove_value(m_userConfig, "BasicWindow", "SnappingEnabled");
    config_remove_value(m_userConfig, "BasicWindow", "ScreenSnapping");
    config_remove_value(m_userConfig, "BasicWindow", "SourceSnapping");
    config_remove_value(m_userConfig, "BasicWindow", "CenterSnapping");
    config_remove_value(m_userConfig, "BasicWindow", "SnapDistance");
    config_remove_value(m_userConfig, "BasicWindow", "SpacingHelpersEnabled");
    config_remove_value(m_userConfig, "BasicWindow", "RecordWhenStreaming");
    config_remove_value(m_userConfig, "BasicWindow", "KeepRecordingWhenStreamStops");
    config_remove_value(m_userConfig, "BasicWindow", "SysTrayEnabled");
    config_remove_value(m_userConfig, "BasicWindow", "SysTrayWhenStarted");
    config_remove_value(m_userConfig, "BasicWindow", "SaveProjectors");
    config_remove_value(m_userConfig, "BasicWindow", "ShowTransitions");
    config_remove_value(m_userConfig, "BasicWindow", "ShowListboxToolbars");
    config_remove_value(m_userConfig, "BasicWindow", "ShowStatusBar");
    config_remove_value(m_userConfig, "BasicWindow", "ShowSourceIcons");
    config_remove_value(m_userConfig, "BasicWindow", "ShowContextToolbars");
    config_remove_value(m_userConfig, "BasicWindow", "StudioModeLabels");
    config_remove_value(m_userConfig, "BasicWindow", "VerticalVolControl");

    config_remove_value(m_userConfig, "BasicWindow", "MultiviewMouseSwitch");

    config_remove_value(m_userConfig, "BasicWindow", "MultiviewDrawNames");

    config_remove_value(m_userConfig, "BasicWindow", "MultiviewDrawAreas");

    config_remove_value(m_userConfig, "BasicWindow", "ShowPopupLeft");
    config_remove_value(m_userConfig, "BasicWindow", "MediaControlsCountdownTimer");
    // -------------------------------------

    config_remove_value(m_activeConfig, "General", "OpenStatsOnStartup");
    config_remove_value(m_userConfig, "BasicWindow", "WarnBeforeStartingStream");
    config_remove_value(m_userConfig, "BasicWindow", "WarnBeforeStoppingStream");
    config_remove_value(m_userConfig, "BasicWindow", "WarnBeforeStoppingRecord");
    config_remove_value(m_userConfig, "BasicWindow", "OverflowHidden");
    config_remove_value(m_userConfig, "BasicWindow", "OverflowAlwaysVisible");
    config_remove_value(m_userConfig, "BasicWindow", "OverflowSelectionHidden");
    config_remove_value(m_userConfig, "BasicWindow", "ShowSafeAreas");
    config_remove_value(m_userConfig, "General", "AutomaticCollectionSearch");
    config_remove_value(m_userConfig, "BasicWindow", "TransitionOnDoubleClick");
    config_remove_value(m_userConfig, "BasicWindow", "StudioPortraitLayout");
    config_remove_value(m_userConfig, "BasicWindow", "ShowVirtualCamToolip");

    const char* installLang = config_get_string(m_userConfig, "General", "LanguageBase");
    if(installLang) {
        config_set_string(m_userConfig, "General", "Language", installLang);
    }

    InitUserConfigDefaults();
}

void AFConfigManager::SetBasicOutputConfig()
{
    bool changed = false;

    /* ----------------------------------------------------- */
    /* move over old FFmpeg track settings                   */
    if(config_has_user_value(m_activeConfig, "AdvOut", "FFAudioTrack") &&
        !config_has_user_value(m_activeConfig, "AdvOut", "Pre22.1Settings")) {

        int track = (int)config_get_int(m_activeConfig, "AdvOut", "FFAudioTrack");
        config_set_int(m_activeConfig, "AdvOut", "FFAudioMixes", 1LL << (track - 1));
        config_set_bool(m_activeConfig, "AdvOut", "Pre22.1Settings", true);
        changed = true;
    }

    /* ----------------------------------------------------- */
    /* move over mixer values in advanced if older config */
    if(config_has_user_value(m_activeConfig, "AdvOut", "RecTrackIndex") &&
        !config_has_user_value(m_activeConfig, "AdvOut", "RecTracks")) {

        uint64_t track = config_get_uint(m_activeConfig, "AdvOut", "RecTrackIndex");
        track = 1ULL << (track - 1);
        config_set_uint(m_activeConfig, "AdvOut", "RecTracks", track);
        config_remove_value(m_activeConfig, "AdvOut", "RecTrackIndex");
        changed = true;
    }

    /* ----------------------------------------------------- */
    /* move bitrate enforcement setting to new value         */
    if(config_has_user_value(m_activeConfig, "SimpleOutput", "EnforceBitrate") &&
        !config_has_user_value(m_activeConfig, "Stream1", "IgnoreRecommended") &&
        !config_has_user_value(m_activeConfig, "Stream1", "MovedOldEnforce")) {
        bool enforce = config_get_bool(m_activeConfig, "SimpleOutput", "EnforceBitrate");
        config_set_bool(m_activeConfig, "Stream1", "IgnoreRecommended", !enforce);
        config_set_bool(m_activeConfig, "Stream1", "MovedOldEnforce", true);
        changed = true;
    }

    /* ----------------------------------------------------- */
    /* enforce minimum retry delay of 1 second prior to 27.1 */
    if(config_has_user_value(m_activeConfig, "Output", "RetryDelay")) {
        int retryDelay = config_get_uint(m_activeConfig, "Output", "RetryDelay");
        if(retryDelay < 1) {
            config_set_uint(m_activeConfig, "Output", "RetryDelay", 1);
            changed = true;
        }
    }

    /* ----------------------------------------------------- */
    /* Migrate old container selection (if any) to new key.  */
    auto MigrateFormat = [&](const char* section) {
        bool has_old_key = config_has_user_value(m_activeConfig, section, "RecFormat");
        bool has_new_key = config_has_user_value(m_activeConfig, section, "RecFormat2");
        if(!has_new_key && !has_old_key)
            return;

        std::string old_format = config_get_string(m_activeConfig, section, has_new_key ? "RecFormat2" : "RecFormat");
        std::string new_format = old_format;
        if(old_format == "ts")
            new_format = "mpegts";
        else if(old_format == "m3u8")
            new_format = "hls";
        else if(old_format == "fmp4")
            new_format = "fragmented_mp4";
        else if(old_format == "fmov")
            new_format = "fragmented_mov";

        if(new_format != old_format || !has_new_key) {
            config_set_string(m_activeConfig, section, "RecFormat2", new_format.c_str());
            changed = true;
        }
    };

    MigrateFormat("AdvOut");
    MigrateFormat("SimpleOutput");

    /* ----------------------------------------------------- */
    /* Migrate output scale setting to GPU scaling options.  */

    if(config_get_bool(m_activeConfig, "AdvOut", "Rescale") &&
        !config_has_user_value(m_activeConfig, "AdvOut", "RescaleFilter")) {
        config_set_int(m_activeConfig, "AdvOut", "RescaleFilter", OBS_SCALE_BILINEAR);
    }

    if(config_get_bool(m_activeConfig, "AdvOut", "RecRescale") &&
        !config_has_user_value(m_activeConfig, "AdvOut", "RecRescaleFilter")) {
        config_set_int(m_activeConfig, "AdvOut", "RecRescaleFilter", OBS_SCALE_BILINEAR);
    }

    /* ----------------------------------------------------- */
    std::string defaultOutputPath = GetDefaultVideoSavePath();

    config_set_default_string(m_activeConfig, "Output", "Mode", "Simple");

    config_set_default_bool(m_activeConfig, "Stream1", "IgnoreRecommended", false);
    config_set_default_bool(m_activeConfig, "Stream1", "EnableMultitrackVideo", false);
    config_set_default_bool(m_activeConfig, "Stream1", "MultitrackVideoMaximumAggregateBitrateAuto", true);
    config_set_default_bool(m_activeConfig, "Stream1", "MultitrackVideoMaximumVideoTracksAuto", true);

    config_set_default_string(m_activeConfig, "SimpleOutput", "FilePath", defaultOutputPath.c_str());
    config_set_default_string(m_activeConfig, "SimpleOutput", "RecFormat2", DEFAULT_CONTAINER);
    config_set_default_uint(m_activeConfig, "SimpleOutput", "VBitrate", 2000);
    config_set_default_uint(m_activeConfig, "SimpleOutput", "ABitrate", 160);
    config_set_default_bool(m_activeConfig, "SimpleOutput", "UseAdvanced", false);
    config_set_default_string(m_activeConfig, "SimpleOutput", "Preset", "veryfast");
    config_set_default_string(m_activeConfig, "SimpleOutput", "NVENCPreset2", "p5");
    config_set_default_string(m_activeConfig, "SimpleOutput", "RecQuality", "Stream");
    config_set_default_bool(m_activeConfig, "SimpleOutput", "RecRB", false);
    config_set_default_int(m_activeConfig, "SimpleOutput", "RecRBTime", 20);
    config_set_default_int(m_activeConfig, "SimpleOutput", "RecRBSize", 512);
    config_set_default_string(m_activeConfig, "SimpleOutput", "RecRBPrefix", "Replay");
    config_set_default_string(m_activeConfig, "SimpleOutput", "StreamAudioEncoder", "aac");
    config_set_default_string(m_activeConfig, "SimpleOutput", "RecAudioEncoder", "aac");
    config_set_default_uint(m_activeConfig, "SimpleOutput", "RecTracks", (1 << 0));

    config_set_default_bool(m_activeConfig, "AdvOut", "ApplyServiceSettings", true);
    config_set_default_bool(m_activeConfig, "AdvOut", "UseRescale", false);
    config_set_default_uint(m_activeConfig, "AdvOut", "TrackIndex", 1);
    config_set_default_uint(m_activeConfig, "AdvOut", "VodTrackIndex", 2);
    //config_set_default_string(m_activeConfig, "AdvOut", "Encoder", "obs_x264");

    config_set_default_string(m_activeConfig, "AdvOut", "RecType", "Standard");

    config_set_default_string(m_activeConfig, "AdvOut", "RecFilePath", defaultOutputPath.c_str());
    config_set_default_string(m_activeConfig, "AdvOut", "RecFormat2", DEFAULT_CONTAINER);
    config_set_default_bool(m_activeConfig, "AdvOut", "RecUseRescale", false);
    config_set_default_uint(m_activeConfig, "AdvOut", "RecTracks", (1 << 0));
    config_set_default_string(m_activeConfig, "AdvOut", "RecEncoder", "none");
    config_set_default_uint(m_activeConfig, "AdvOut", "FLVTrack", 1);
    config_set_default_uint(m_activeConfig, "AdvOut", "StreamMultiTrackAudioMixes", 1);

    config_set_default_bool(m_activeConfig, "AdvOut", "FFOutputToFile", true);
    config_set_default_string(m_activeConfig, "AdvOut", "FFFilePath", defaultOutputPath.c_str());
    config_set_default_string(m_activeConfig, "AdvOut", "FFExtension", "mp4");
    config_set_default_uint(m_activeConfig, "AdvOut", "FFVBitrate", 2500);
    config_set_default_uint(m_activeConfig, "AdvOut", "FFVGOPSize", 250);
    config_set_default_bool(m_activeConfig, "AdvOut", "FFUseRescale", false);
    config_set_default_bool(m_activeConfig, "AdvOut", "FFIgnoreCompat", false);
    config_set_default_uint(m_activeConfig, "AdvOut", "FFABitrate", 160);
    config_set_default_uint(m_activeConfig, "AdvOut", "FFAudioMixes", 1);

    config_set_default_uint(m_activeConfig, "AdvOut", "Track1Bitrate", 160);
    config_set_default_uint(m_activeConfig, "AdvOut", "Track2Bitrate", 160);
    config_set_default_uint(m_activeConfig, "AdvOut", "Track3Bitrate", 160);
    config_set_default_uint(m_activeConfig, "AdvOut", "Track4Bitrate", 160);
    config_set_default_uint(m_activeConfig, "AdvOut", "Track5Bitrate", 160);
    config_set_default_uint(m_activeConfig, "AdvOut", "Track6Bitrate", 160);

    config_set_default_uint(m_activeConfig, "AdvOut", "RecSplitFileTime", 15);
    config_set_default_uint(m_activeConfig, "AdvOut", "RecSplitFileSize", 2048);

    config_set_default_bool(m_activeConfig, "AdvOut", "RecRB", false);
    config_set_default_uint(m_activeConfig, "AdvOut", "RecRBTime", 20);
    config_set_default_int(m_activeConfig, "AdvOut", "RecRBSize", 512);

    config_set_default_string(m_activeConfig, "Output", "FilenameFormatting", "%CCYY-%MM-%DD %hh-%mm-%ss");

    config_set_default_bool(m_activeConfig, "Output", "DelayEnable", false);
    config_set_default_uint(m_activeConfig, "Output", "DelaySec", 20);
    config_set_default_bool(m_activeConfig, "Output", "DelayPreserve", true);

    config_set_default_bool(m_activeConfig, "Output", "Reconnect", true);
    config_set_default_uint(m_activeConfig, "Output", "RetryDelay", 2);
    config_set_default_uint(m_activeConfig, "Output", "MaxRetries", 25);

    config_set_default_string(m_activeConfig, "Output", "BindIP", "default");
    config_set_default_string(m_activeConfig, "Output", "IPFamily", "IPv4+IPv6");
    config_set_default_bool(m_activeConfig, "Output", "NewSocketLoopEnable", false);
    config_set_default_bool(m_activeConfig, "Output", "LowLatencyEnable", false);

    //_MakeDefaultVideoSaveDir();
    if(os_file_exists(defaultOutputPath.c_str()) == false)
        os_mkdir(defaultOutputPath.c_str());

    //InitBasicConfigDefaults2();

    /* ----------------------------------------------------- */
    //if(changed)
    {
        m_activeConfig.SaveSafe("tmp");
    }
    /* ----------------------------------------------------- */
}
void AFConfigManager::ResetOutputConfig()
{
    // Delete configs
    config_remove_value(m_activeConfig, "Output", "Mode");
    config_remove_value(m_activeConfig, "Stream1", "IgnoreRecommended");
    config_remove_value(m_activeConfig, "SimpleOutput", "FilePath");
    config_remove_value(m_activeConfig, "SimpleOutput", "RecFormat2");
    config_remove_value(m_activeConfig, "SimpleOutput", "VBitrate");
    config_remove_value(m_activeConfig, "SimpleOutput", "ABitrate");
    config_remove_value(m_activeConfig, "SimpleOutput", "UseAdvanced");
    config_remove_value(m_activeConfig, "SimpleOutput", "Preset");
    config_remove_value(m_activeConfig, "SimpleOutput", "NVENCPreset2");
    config_remove_value(m_activeConfig, "SimpleOutput", "RecQuality");
    config_remove_value(m_activeConfig, "SimpleOutput", "RecRB");
    config_remove_value(m_activeConfig, "SimpleOutput", "RecRBTime");
    config_remove_value(m_activeConfig, "SimpleOutput", "RecRBSize");
    config_remove_value(m_activeConfig, "SimpleOutput", "RecRBPrefix");
    config_remove_value(m_activeConfig, "SimpleOutput", "StreamAudioEncoder");
    config_remove_value(m_activeConfig, "SimpleOutput", "RecAudioEncoder");
    config_remove_value(m_activeConfig, "SimpleOutput", "RecTracks");

    config_remove_value(m_activeConfig, "AdvOut", "ApplyServiceSettings");
    config_remove_value(m_activeConfig, "AdvOut", "FFRescale");
    config_remove_value(m_activeConfig, "AdvOut", "RecRescale");
    config_remove_value(m_activeConfig, "AdvOut", "UseRescale");
    config_remove_value(m_activeConfig, "AdvOut", "TrackIndex");
    config_remove_value(m_activeConfig, "AdvOut", "VodTrackIndex");
    config_remove_value(m_activeConfig, "AdvOut", "Encoder");

    config_remove_value(m_activeConfig, "AdvOut", "RecType");

    config_remove_value(m_activeConfig, "AdvOut", "RecFilePath");
    config_remove_value(m_activeConfig, "AdvOut", "RecFormat2");
    config_remove_value(m_activeConfig, "AdvOut", "RecUseRescale");
    config_remove_value(m_activeConfig, "AdvOut", "RecTracks");
    config_remove_value(m_activeConfig, "AdvOut", "RecEncoder");
    config_remove_value(m_activeConfig, "AdvOut", "FLVTrack");

    config_remove_value(m_activeConfig, "AdvOut", "FFOutputToFile");
    config_remove_value(m_activeConfig, "AdvOut", "FFFilePath");
    config_remove_value(m_activeConfig, "AdvOut", "FFExtension");
    config_remove_value(m_activeConfig, "AdvOut", "FFVBitrate");
    config_remove_value(m_activeConfig, "AdvOut", "FFVGOPSize");
    config_remove_value(m_activeConfig, "AdvOut", "FFUseRescale");
    config_remove_value(m_activeConfig, "AdvOut", "FFIgnoreCompat");
    config_remove_value(m_activeConfig, "AdvOut", "FFABitrate");
    config_remove_value(m_activeConfig, "AdvOut", "FFAudioMixes");

    config_remove_value(m_activeConfig, "AdvOut", "Track1Bitrate");
    config_remove_value(m_activeConfig, "AdvOut", "Track2Bitrate");
    config_remove_value(m_activeConfig, "AdvOut", "Track3Bitrate");
    config_remove_value(m_activeConfig, "AdvOut", "Track4Bitrate");
    config_remove_value(m_activeConfig, "AdvOut", "Track5Bitrate");
    config_remove_value(m_activeConfig, "AdvOut", "Track6Bitrate");

    config_remove_value(m_activeConfig, "AdvOut", "RecSplitFileTime");
    config_remove_value(m_activeConfig, "AdvOut", "RecSplitFileSize");

    config_remove_value(m_activeConfig, "AdvOut", "RecRB");
    config_remove_value(m_activeConfig, "AdvOut", "RecRBTime");
    config_remove_value(m_activeConfig, "AdvOut", "RecRBSize");

    config_remove_value(m_activeConfig, "Output", "FilenameFormatting");

    config_remove_value(m_activeConfig, "Output", "DelayEnable");
    config_remove_value(m_activeConfig, "Output", "DelaySec");
    config_remove_value(m_activeConfig, "Output", "DelayPreserve");

    config_remove_value(m_activeConfig, "Output", "Reconnect");
    config_remove_value(m_activeConfig, "Output", "RetryDelay");
    config_remove_value(m_activeConfig, "Output", "MaxRetries");

    config_remove_value(m_activeConfig, "Output", "BindIP");
    config_remove_value(m_activeConfig, "Output", "IPFamily");
    config_remove_value(m_activeConfig, "Output", "NewSocketLoopEnable");
    config_remove_value(m_activeConfig, "Output", "LowLatencyEnable");

    config_remove_value(m_activeConfig, "SimpleOutput", "StreamEncoder");
    config_remove_value(m_activeConfig, "SimpleOutput", "RecEncoder");

    config_remove_value(m_activeConfig, "AdvOut", "AudioEncoder");
    config_remove_value(m_activeConfig, "AdvOut", "RecAudioEncoder");

    // Set default values
    SetBasicOutputConfig();
}

void AFConfigManager::SetBasicAudioConfig()
{
    config_set_default_string(m_activeConfig, "Audio", "MonitoringDeviceId", "default");
    config_set_default_string(m_activeConfig, "Audio", "MonitoringDeviceName",
                              Str("Basic.Settings.Advanced.Audio.MonitoringDevice.Default"));
    config_set_default_uint(m_activeConfig, "Audio", "SampleRate", 48000);
    config_set_default_string(m_activeConfig, "Audio", "ChannelSetup", "Stereo");
    config_set_default_double(m_activeConfig, "Audio", "MeterDecayRate", VOLUME_METER_DECAY_FAST);
    config_set_default_uint(m_activeConfig, "Audio", "PeakMeterType", 0);
    m_activeConfig.SaveSafe("tmp");
}
void AFConfigManager::SetGlobalAudioConfig()
{
#ifdef _WIN32
    config_set_default_bool(m_appConfig, "Audio", "DisableAudioDucking", true);
    m_appConfig.SaveSafe("tmp");
#endif
}
void AFConfigManager::ResetAudioConfig()
{
    // Remove exists
    config_remove_value(m_activeConfig, "Audio", "MonitoringDeviceId");
    config_remove_value(m_activeConfig, "Audio", "MonitoringDeviceName");
    config_remove_value(m_activeConfig, "Audio", "SampleRate");
    config_remove_value(m_activeConfig, "Audio", "ChannelSetup");
    config_remove_value(m_activeConfig, "Audio", "MeterDecayRate");
    config_remove_value(m_activeConfig, "Audio", "PeakMeterType");
#ifdef _WIN32
    config_remove_value(m_appConfig, "Audio", "DisableAudioDucking");
#endif
    // Set defaults
    SetBasicAudioConfig();
    SetGlobalAudioConfig();
}

void AFConfigManager::SetBasicVideoConfig(uint32_t cx, uint32_t cy)
{
    config_set_default_uint(m_activeConfig, "Video", "BaseCX", cx);
    config_set_default_uint(m_activeConfig, "Video", "BaseCY", cy);

    /* don't allow BaseCX/BaseCY to be susceptible to defaults changing */
    if(!config_has_user_value(m_activeConfig, "Video", "BaseCX") ||
        !config_has_user_value(m_activeConfig, "Video", "BaseCY")) {
        config_set_uint(m_activeConfig, "Video", "BaseCX", cx);
        config_set_uint(m_activeConfig, "Video", "BaseCY", cy);
    }

    int i = 0;
    uint32_t scale_cx = cx;
    uint32_t scale_cy = cy;

    /* use a default scaled resolution that has a pixel count no higher
     * than 1280x720 */
    /*while (((scale_cx * scale_cy) > (1280 * 720)) && scaled_vals[i] > 0.0)
    {
    	double scale = scaled_vals[i++];
    	scale_cx = uint32_t(double(cx) / scale);
    	scale_cy = uint32_t(double(cy) / scale);
    }*/

    /*config_set_default_uint(m_activeConfig, "Video", "OutputCX", scale_cx);
    config_set_default_uint(m_activeConfig, "Video", "OutputCY", scale_cy);*/

    /* don't allow OutputCX/OutputCY to be susceptible to defaults
     * changing */
    /*if (!config_has_user_value(m_activeConfig, "Video", "OutputCX") ||
    	!config_has_user_value(m_activeConfig, "Video", "OutputCY"))
    {
    	config_set_uint(m_activeConfig, "Video", "OutputCX", scale_cx);
    	config_set_uint(m_activeConfig, "Video", "OutputCY", scale_cy);
    }*/

    config_set_default_uint(m_activeConfig, "Video", "OutputCX", 1280);
    config_set_default_uint(m_activeConfig, "Video", "OutputCY", 720);

    config_set_default_uint(m_activeConfig, "Video", "FPSType", 0);
    config_set_default_string(m_activeConfig, "Video", "FPSCommon", "30");
    config_set_default_uint(m_activeConfig, "Video", "FPSInt", 30);
    config_set_default_uint(m_activeConfig, "Video", "FPSNum", 30);
    config_set_default_uint(m_activeConfig, "Video", "FPSDen", 1);
    config_set_default_string(m_activeConfig, "Video", "ScaleType", "bicubic");
    config_set_default_string(m_activeConfig, "Video", "ColorFormat", "NV12");
    config_set_default_string(m_activeConfig, "Video", "ColorSpace", "709");
    config_set_default_string(m_activeConfig, "Video", "ColorRange", "Partial");
    config_set_default_uint(m_activeConfig, "Video", "SdrWhiteLevel", 300);
    config_set_default_uint(m_activeConfig, "Video", "HdrNominalPeakLevel", 1000);
    config_set_default_bool(m_activeConfig, "AdvOut", "Rescale", false);
    m_activeConfig.SaveSafe("tmp");
}
void AFConfigManager::SetGlobalVideoConfig()
{
#if _WIN32
    config_set_default_string(m_appConfig, "Video", "Renderer", "Direct3D 11");
#else
    config_set_default_string(m_appConfig, "Video", "Renderer", "OpenGL");
    config_set_default_bool(m_appConfig, "Video", "DisableOSXVSync", true);
    config_set_default_bool(m_appConfig, "Video", "ResetOSXVSyncOnExit", true);
#endif
    m_appConfig.SaveSafe("tmp");
}
void AFConfigManager::ResetVideoConfig(uint32_t cx, uint32_t cy)
{
    config_remove_value(m_activeConfig, "Video", "BaseCX");
    config_remove_value(m_activeConfig, "Video", "BaseCY");
    config_remove_value(m_activeConfig, "Video", "OutputCX");
    config_remove_value(m_activeConfig, "Video", "OutputCY");
    config_remove_value(m_activeConfig, "Video", "FPSType");
    config_remove_value(m_activeConfig, "Video", "FPSCommon");
    config_remove_value(m_activeConfig, "Video", "FPSInt");
    config_remove_value(m_activeConfig, "Video", "FPSNum");
    config_remove_value(m_activeConfig, "Video", "FPSDen");
    config_remove_value(m_activeConfig, "Video", "ScaleType");
    config_remove_value(m_activeConfig, "Video", "ColorFormat");
    config_remove_value(m_activeConfig, "Video", "ColorSpace");
    config_remove_value(m_activeConfig, "Video", "ColorRange");
    config_remove_value(m_activeConfig, "Video", "SdrWhiteLevel");
    config_remove_value(m_activeConfig, "Video", "HdrNominalPeakLevel");
    config_remove_value(m_activeConfig, "AdvOut", "Rescale");
#if _WIN32
    config_remove_value(m_appConfig, "Video", "Renderer");
#else
    config_remove_value(m_appConfig, "Video", "Renderer");
    config_remove_value(m_appConfig, "Video", "DisableOSXVSync");
    config_remove_value(m_appConfig, "Video", "ResetOSXVSyncOnExit");
#endif
    SetBasicVideoConfig(cx, cy);
    SetGlobalVideoConfig();
}

void AFConfigManager::InitAccessibilityConfig()
{
    config_set_default_int(m_userConfig, "Accessibility", "SelectRed", 0xff8201);
    config_set_default_int(m_userConfig, "Accessibility", "SelectGreen", 0x5141c3);
    config_set_default_int(m_userConfig, "Accessibility", "SelectBlue", 0xffe000);
    config_set_default_int(m_userConfig, "Accessibility", "MixerGreen", 0x2e4821);
    config_set_default_int(m_userConfig, "Accessibility", "MixerYellow", 0x275e63);
    config_set_default_int(m_userConfig, "Accessibility", "MixerRed", 0x222255);
    config_set_default_int(m_userConfig, "Accessibility", "MixerGreenActive", 0x4cff4c);
    config_set_default_int(m_userConfig, "Accessibility", "MixerYellowActive", 0x4cffff);
    config_set_default_int(m_userConfig, "Accessibility", "MixerRedActive", 0x4c4cff);
    config_set_default_int(m_userConfig, "Accessibility", "ColorPreset", ColorPreset::COLOR_PRESET_DEFAULT);
    m_userConfig.SaveSafe("tmp");
}
void AFConfigManager::ResetAccessibilityConfig()
{
    config_remove_value(m_userConfig, "Accessibility", "SelectRed");
    config_remove_value(m_userConfig, "Accessibility", "SelectGreen");
    config_remove_value(m_userConfig, "Accessibility", "SelectBlue");
    config_remove_value(m_userConfig, "Accessibility", "MixerGreen");
    config_remove_value(m_userConfig, "Accessibility", "MixerYellow");
    config_remove_value(m_userConfig, "Accessibility", "MixerRed");
    config_remove_value(m_userConfig, "Accessibility", "MixerGreenActive");
    config_remove_value(m_userConfig, "Accessibility", "MixerYellowActive");
    config_remove_value(m_userConfig, "Accessibility", "MixerRedActive");

    config_remove_value(m_userConfig, "Accessibility", "ColorPreset");

    InitAccessibilityConfig();
}

void AFConfigManager::InitBroadInfoConfig()
{
    config_set_default_bool(m_userConfig, "BroadInfo", "NotifyOnTitleChange", true);
    m_userConfig.SaveSafe("tmp");
}

// for sarsa
void AFConfigManager::InitSarsaConfig()
{
    config_set_default_bool(m_userConfig, "SARSA", "UseTextMode", false);
    config_set_default_bool(m_userConfig, "SARSA", "UseWakeWord", false);
    config_set_default_bool(m_userConfig, "SARSA", "UseBGM", false);
    config_set_default_bool(m_userConfig, "SARSA", "FirstStart", true);
	config_set_default_int(m_userConfig, "SARSA", "UseMinsimCheckCnt", 0);
    m_userConfig.SaveSafe("tmp");
}

void AFConfigManager::InitBookmarkMenuConfig()
{
    const QByteArray now = QDateTime::currentDateTime()
        .toString(Qt::ISODateWithMs).toUtf8();

    config_set_default_bool(m_userConfig, "FavoriteMenu", "chat", true);
    config_set_default_string(m_userConfig, "FavoriteMenu", "chat_favorite_at", now.constData());
    config_set_default_bool(m_userConfig, "FavoriteMenu", "overlay", false);
    config_set_default_string(m_userConfig, "FavoriteMenu", "overlay_favorite_at", "");
    config_set_default_bool(m_userConfig, "FavoriteMenu", "mission", false);
    config_set_default_string(m_userConfig, "FavoriteMenu", "mission_favorite_at", "");
    config_set_default_bool(m_userConfig, "FavoriteMenu", "vote", false);
    config_set_default_string(m_userConfig, "FavoriteMenu", "vote_favorite_at", "");
    config_set_default_bool(m_userConfig, "FavoriteMenu", "extensions", false);
    config_set_default_string(m_userConfig, "FavoriteMenu", "extensions_favorite_at", "");
    config_set_default_bool(m_userConfig, "FavoriteMenu", "aquacontrol", false);
    config_set_default_string(m_userConfig, "FavoriteMenu", "aquacontrol_favorite_at", "");
    config_set_default_bool(m_userConfig, "FavoriteMenu", "savevod", false);
    config_set_default_string(m_userConfig, "FavoriteMenu", "savevod_favorite_at", "");
    config_set_default_bool(m_userConfig, "FavoriteMenu", "breaktime", false);
    config_set_default_string(m_userConfig, "FavoriteMenu", "breaktime_favorite_at", "");

    m_userConfig.SaveSafe("tmp");
}

void AFConfigManager::SetBasicAdvancedConfig()
{
#ifdef _WIN32
    config_set_default_bool(m_activeConfig, "Output", "NewSocketLoopEnable", false);
    config_set_default_bool(m_activeConfig, "Output", "LowLatencyEnable", false);
#endif
    config_set_default_string(m_activeConfig, "Output", "FilenameFormatting", "%CCYY-%MM-%DD %hh-%mm-%ss");
    config_set_default_string(m_activeConfig, "SimpleOutput", "RecRBPrefix", "Replay");

    config_set_default_bool(m_activeConfig, "Output", "Reconnect", true);
    config_set_default_uint(m_activeConfig, "Output", "RetryDelay", 2);
    config_set_default_uint(m_activeConfig, "Output", "MaxRetries", 25);

    config_set_default_string(m_activeConfig, "Output", "IPFamily", "IPv4+IPv6");
    config_set_default_string(m_activeConfig, "Output", "BindIP", "default");
    m_activeConfig.SaveSafe("tmp");
}
void AFConfigManager::SetGlobalAdvancedConfig()
{
#ifdef _WIN32
    config_set_default_string(m_appConfig, "General", "ProcessPriority", "AboveNormal");
#endif
    //No Use ConfirmOnExit - Streaming always show end dialog, Recording - use WarnBeforeStoppingRecord
    config_set_default_bool(m_userConfig, "General", "ConfirmOnExit", true);

    // _WIN32 || __APPLE__
    config_set_default_bool(m_appConfig, "General", "BrowserHWAccel", true);
    m_appConfig.SaveSafe("tmp");
}
void AFConfigManager::ResetAdvancedConfig()
{
    // Remove basic configs
#ifdef _WIN32
    config_remove_value(m_activeConfig, "Output", "NewSocketLoopEnable");
    config_remove_value(m_activeConfig, "Output", "LowLatencyEnable");
#endif
    config_remove_value(m_activeConfig, "Output", "FilenameFormatting");
    config_remove_value(m_activeConfig, "SimpleOutput", "RecRBPrefix");

    config_remove_value(m_activeConfig, "Output", "Reconnect");
    config_remove_value(m_activeConfig, "Output", "RetryDelay");
    config_remove_value(m_activeConfig, "Output", "MaxRetries");

    config_remove_value(m_activeConfig, "Output", "IPFamily");
    config_remove_value(m_activeConfig, "Output", "BindIP");

    // Remove global configs
#ifdef _WIN32
    config_remove_value(m_appConfig, "General", "ProcessPriority");
#endif
    config_remove_value(m_userConfig, "General", "ConfirmOnExit");

    // _WIN32 || __APPLE__
    config_remove_value(m_appConfig, "General", "BrowserHWAccel");

    // Set default
    SetBasicAdvancedConfig();
    SetGlobalAdvancedConfig();
}

bool AFConfigManager::GetFileSafeName(const char* name, std::string& file)
{
    size_t base_len = strlen(name);
    size_t len = os_utf8_to_wcs(name, base_len, nullptr, 0);
    std::wstring wfile;

    if(!len)
        return false;

    wfile.resize(len);
    os_utf8_to_wcs(name, base_len, &wfile[0], len + 1);

    for(size_t i = wfile.size(); i > 0; i--) {
        size_t im1 = i - 1;

        if(iswspace(wfile[im1])) {
            wfile[im1] = '_';
        } else if(wfile[im1] != '_' && !iswalnum(wfile[im1])) {
            wfile.erase(im1, 1);
        }
    }

    if(wfile.size() == 0)
        wfile = L"characters_only";

    len = os_wcs_to_utf8(wfile.c_str(), wfile.size(), nullptr, 0);
    if(!len)
        return false;

    file.resize(len);
    os_wcs_to_utf8(wfile.c_str(), wfile.size(), &file[0], len + 1);
    return true;
}
bool AFConfigManager::GetClosestUnusedFileName(std::string& path, const char* extension)
{
    size_t len = path.size();
    if(extension) {
        path += ".";
        path += extension;
    }

    if(!os_file_exists(path.c_str()))
        return true;

    int index = 1;

    do {
        path.resize(len);
        path += std::to_string(++index);
        if(extension) {
            path += ".";
            path += extension;
        }
    } while(os_file_exists(path.c_str()));

    return true;
}
bool AFConfigManager::GetUnusedName(std::string& name)
{
    if(!MAIN_SCENECOLLECTION->GetSceneCollectionByName(name))
        return false;

    std::string newName;
    int inc = 2;
    do {
        newName = name;
        newName += " ";
        newName += std::to_string(inc++);
    } while(MAIN_SCENECOLLECTION->GetSceneCollectionByName(newName));

    name = newName;
    return true;
}

bool AFConfigManager::CheckSavvyTempFolder()
{
	char savvypath[512] = { 0, };
	if (GetAppConfigPath(savvypath, sizeof(savvypath), (LOCAL_FOLDER_NAME + "/savvy").c_str()) <= 0)
		return true;

    if(!AFMakeDirectoryUtil::DoMkDir(savvypath))
        return false;
    return true;
}

bool AFConfigManager::_MakeUserProfileDirs()
{
    const std::filesystem::path userProfilePath = m_userProfilesLocation / GetProfileSubPath();
    const std::filesystem::path userScenesPath = m_userScenesLocation / GetScenesSubPath();

    if(!std::filesystem::exists(userProfilePath)) {
        try {
            std::filesystem::create_directories(userProfilePath);
        } catch(const std::filesystem::filesystem_error& error) {
            blog(LOG_ERROR, "Failed to create user profile directory '%s'\n%s",
                 userProfilePath.u8string().c_str(), error.what());
            return false;
        }
    }

    if(!std::filesystem::exists(userScenesPath)) {
        try {
            std::filesystem::create_directories(userScenesPath);
        } catch(const std::filesystem::filesystem_error& error) {
            blog(LOG_ERROR, "Failed to create user scene collection directory '%s'\n%s",
                 userScenesPath.u8string().c_str(), error.what());
            return false;
        }
    }

    return true;
}

bool AFConfigManager::_UpdatePre22MultiviewLayout(const char* layout)
{
    if(!layout)
        return false;

    /*if(astrcmpi(layout, "horizontaltop") == 0) {
        config_set_int(m_userConfig, "BasicWindow", "MultiviewLayout",
                   static_cast<int>(MultiviewLayout::HORIZONTAL_TOP_8_SCENES));
        return true;
    }

    if(astrcmpi(layout, "horizontalbottom") == 0) {
        config_set_int(m_userConfig, "BasicWindow", "MultiviewLayout",
                   static_cast<int>(MultiviewLayout::HORIZONTAL_BOTTOM_8_SCENES));
        return true;
    }

    if(astrcmpi(layout, "verticalleft") == 0) {
        config_set_int(m_userConfig, "BasicWindow", "MultiviewLayout",
                   static_cast<int>(MultiviewLayout::VERTICAL_LEFT_8_SCENES));
        return true;
    }

    if(astrcmpi(layout, "verticalright") == 0) {
        config_set_int(m_userConfig, "BasicWindow", "MultiviewLayout",
                   static_cast<int>(MultiviewLayout::VERTICAL_RIGHT_8_SCENES));
        return true;
    }*/

    return false;
}

bool AFConfigManager::_InitGlobalConfig()
{
    ProfileScope("AFConfigManager::_InitGlobalConfig");

    char path[512];
    bool changed = false;

    int len = GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/global.ini").c_str());
    if(len <= 0)
        return false;

    int errorcode = m_appConfig.Open(path, CONFIG_OPEN_ALWAYS);
    if(errorcode != CONFIG_SUCCESS) {
        OBSErrorBox(NULL, "Failed to open global.ini: %d", errorcode);
        return false;
    }

    uint32_t lastVersion = config_get_int(m_appConfig, "General", "LastVersion");
    if(lastVersion < MAKE_SEMANTIC_VERSION(31, 0, 0)) {
        bool migratedUserSettings = config_get_bool(m_appConfig, "General", "Pre31Migrated");

        if(!migratedUserSettings) {
            bool migrated = MigrateGlobalSettings();

            config_set_bool(m_appConfig, "General", "Pre31Migrated", migrated);
            config_save_safe(m_appConfig, "tmp", nullptr);
        }
    }

    InitGlobalConfigDefaults();
    InitGlobalLocationDefaults();

    const char* uuid = config_get_string(m_appConfig, "General", "ModuleUUID");
    if (!uuid)
    {
        m_moduleUUID = QUuid::createUuid().toString().toStdString();
        config_set_string(m_appConfig, "General", "ModuleUUID", m_moduleUUID.c_str());

        m_init_firstrun = true;
    }
    else
        m_moduleUUID = uuid;

    if(ARGOPTION.GetPortableMode()) {
        m_userConfigLocation = std::filesystem::u8path(config_get_default_string(m_appConfig, "Locations", "Configuration"));
        m_userScenesLocation = std::filesystem::u8path(config_get_default_string(m_appConfig, "Locations", "SceneCollections"));
        m_userProfilesLocation = std::filesystem::u8path(config_get_default_string(m_appConfig, "Locations", "Profiles"));
    } else {
        m_userConfigLocation = std::filesystem::u8path(config_get_string(m_appConfig, "Locations", "Configuration"));
        m_userScenesLocation = std::filesystem::u8path(config_get_string(m_appConfig, "Locations", "SceneCollections"));
        m_userProfilesLocation = std::filesystem::u8path(config_get_string(m_appConfig, "Locations", "Profiles"));
    }

    bool userConfigResult = InitUserConfig(m_userConfigLocation, lastVersion);
    return userConfigResult;

}

bool AFConfigManager::_InitBasicConfig()
{
    ProfileScope("AFConfigManager::_InitBasicConfig");

    MAIN_PROFILE->RefreshProfiles(true);

    auto& stringProfile = ARGOPTION.startingProfile();
    std::string currentProfileName {config_get_string(m_userConfig, "Basic", "Profile")};
    if(currentProfileName.empty()) {
        currentProfileName = Str("Untitled");
        config_set_string(m_userConfig, "Basic", "Profile", Str("Untitled"));
    }

    const std::optional<OBSProfile> currentProfile = MAIN_PROFILE->GetProfileByName(currentProfileName);
    const std::optional<OBSProfile> foundProfile = MAIN_PROFILE->GetProfileByName(stringProfile);

    try {
        if(foundProfile) {
            MAIN_PROFILE->ActivateProfile(foundProfile.value());
        } else if(currentProfile) {
            MAIN_PROFILE->ActivateProfile(currentProfile.value());
        } else {
            const OBSProfile& newProfile = MAIN_PROFILE->CreateProfile(currentProfileName);
            MAIN_PROFILE->ActivateProfile(newProfile);
        }
    } catch(const std::logic_error&) {
        OBSErrorBox(NULL, "Failed to open basic.ini: %d", -1);
        return false;
    }

    return true;
}
     
void AFConfigManager::_move_basic_to_profiles(void)
{
    char path[512] = {0,};
    char new_path[512] = {0,};
    os_glob_t* glob = NULL;

    /* if not first time use */
    if(GetAppConfigPath(path, 512, (LOCAL_FOLDER_NAME + "/basic").c_str()) <= 0)
        return;

    const std::filesystem::path basicPath = std::filesystem::u8path(path);
    if(!std::filesystem::exists(basicPath))
        return;

    const std::filesystem::path profilesPath = USERPROFILE_PATH / std::filesystem::u8path(LOCAL_FOLDER_NAME + "/basic/profiles");
    if(std::filesystem::exists(profilesPath))
        return;

    try {
        std::filesystem::create_directories(profilesPath);
    } catch(const std::filesystem::filesystem_error& error) {
        blog(LOG_ERROR, "Failed to create profiles directory for migration from basic profile\n%s",
             error.what());
        return;
    }

    const std::filesystem::path newProfilePath = profilesPath / std::filesystem::u8path(Str("Untitled"));
    for(auto& entry : std::filesystem::directory_iterator(basicPath)) {
        if(entry.is_directory())
            continue;

        if(entry.path().filename().u8string() == "scenes.json")
            continue;

        if(!std::filesystem::exists(newProfilePath)) {
            try {
                std::filesystem::create_directory(newProfilePath);
            } catch(const std::filesystem::filesystem_error& error) {
                blog(LOG_ERROR, "Failed to create profile directory for 'Untitled'\n%s", error.what());
                return;
            }
        }

        const std::filesystem::path destinationFile = newProfilePath / entry.path().filename();

        const auto copyOptions = std::filesystem::copy_options::overwrite_existing;

        try {
            std::filesystem::copy(entry.path(), destinationFile, copyOptions);
        } catch(const std::filesystem::filesystem_error& error) {
            blog(LOG_ERROR, "Failed to copy basic profile file '%s' to new profile 'Untitled'\n%s",
                 entry.path().filename().u8string().c_str(), error.what());

            return;
        }
    }
}
void AFConfigManager::_move_basic_to_scene_collections(void)
{
    char path[512] = {0,};
    if(GetAppConfigPath(path, 512, (LOCAL_FOLDER_NAME + "/basic").c_str()) <= 0)
        return;

    const std::filesystem::path basicPath = std::filesystem::u8path(path);
    if(!std::filesystem::exists(basicPath))
        return;

    const std::filesystem::path sceneCollectionPath = USERSCENES_PATH / std::filesystem::u8path(LOCAL_FOLDER_NAME + "/basic/scenes");
    if(std::filesystem::exists(sceneCollectionPath))
        return;

    try {
        std::filesystem::create_directories(sceneCollectionPath);
    } catch(const std::filesystem::filesystem_error& error) {
        blog(LOG_ERROR,
             "Failed to create scene collection directory for migration from basic scene collection\n%s",
             error.what());
        return;
    }

    const std::filesystem::path sourceFile = basicPath / std::filesystem::u8path("scenes.json");
    const std::filesystem::path destinationFile =
        (sceneCollectionPath / std::filesystem::u8path(Str("Untitled"))).replace_extension(".json");

    try {
        std::filesystem::rename(sourceFile, destinationFile);
    } catch(const std::filesystem::filesystem_error& error) {
        blog(LOG_ERROR, "Failed to rename basic scene collection file:\n%s", error.what());
        return;
    }
}