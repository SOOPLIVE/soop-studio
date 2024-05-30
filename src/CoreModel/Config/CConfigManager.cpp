#include "CConfigManager.h"



#include <util/bmem.h>
#include <util/dstr.hpp>
#include <util/platform.h>
#include <util/profiler.hpp>


#include "Common/StringMiscUtils.h"
#include "Common/SettingsMiscDef.h"
#include "include/qt-wrapper.h"
#include "platform/platform.hpp"


#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "PopupWindows/SettingPopup/CAccessibilitySettingAreaWidget.h"


static const double scaled_vals[] = { 1.0,         1.25, (1.0 / 0.75), 1.5,
									  (1.0 / 0.6), 1.75, 2.0,          2.25,
									  2.5,         2.75, 3.0,          0.0 };







#define CONFIG_PATH BASE_PATH "/config"



#if OBS_RELEASE_CANDIDATE == 0 && OBS_BETA == 0
#define DEFAULT_CONTAINER "mkv"
#elif defined(__APPLE__)
#define DEFAULT_CONTAINER "fragmented_mov"
#else
#define DEFAULT_CONTAINER "fragmented_mp4"
#endif



AFConfigManager::~AFConfigManager()
{
	if (m_pArgOption != nullptr)
		delete m_pArgOption;
}

bool AFConfigManager::CheckExistingCookieId()
{
	if (m_bInitedBasic == false)
		return false;


	if (config_has_user_value(m_BasicConfig, "Panels", "CookieId"))
		return true;

	config_set_string(m_BasicConfig, "Panels", "CookieId",
					  GenId().c_str());

	return true;
}

bool AFConfigManager::InitGlobal()
{
	if (m_bInitedGlobal == false)
	{
		bool res = _InitGlobalConfig();

		if (res)
			m_bInitedGlobal = true;


		return res;
	}

	return true;
}

void AFConfigManager::InitBasic(uint32_t cntAssocScreen, 
								uint32_t cxPrimaryScreen, uint32_t cyPrimaryScreen,
								float devicePixelRatio)
{
	if (m_bInitedBasic == false)
	{
		bool res = _InitBasicConfig(cntAssocScreen, 
									cxPrimaryScreen, cyPrimaryScreen, devicePixelRatio);

		if (res)
			m_bInitedBasic = true;
	}
}

void AFConfigManager::SwapOtherToBasic(ConfigFile& other, uint32_t cntAssocScreen,
                                       uint32_t cxPrimaryScreen, uint32_t cyPrimaryScreen,
                                       float devicePixelRatio)
{
    other.Swap(m_BasicConfig);
    SetDefaultValuesBasicConfig(cntAssocScreen,
                                cxPrimaryScreen, cyPrimaryScreen,
                                devicePixelRatio);
}

void AFConfigManager::SafeSwapOtherToBasic(ConfigFile& other, uint32_t cntAssocScreen,
                                       uint32_t cxPrimaryScreen, uint32_t cyPrimaryScreen,
                                       float devicePixelRatio)
{
    m_BasicConfig.SaveSafe("tmp");
    other.SaveSafe("tmp");
    SwapOtherToBasic(other, cntAssocScreen,
                     cxPrimaryScreen, cyPrimaryScreen,
                     devicePixelRatio);
}

int AFConfigManager::GetConfigPath(char* path, size_t size, const char* name) const
{
#if ALLOW_PORTABLE_MODE
	if (m_pArgOption && m_pArgOption->GetPortableMode()) 
	{
		if (name && *name)
			return snprintf(path, size, CONFIG_PATH "/%s", name);
		else 
			return snprintf(path, size, CONFIG_PATH);
	}
	else 
	{
		return os_get_config_path(path, size, name);
	}
#else
	return os_get_config_path(path, size, name);
#endif
}

char* AFConfigManager::GetConfigPathPtr(const char* name)
{
#if ALLOW_PORTABLE_MODE
	if (m_pArgOption && m_pArgOption->GetPortableMode())
	{
		char path[512];

		if (snprintf(path, sizeof(path), CONFIG_PATH "/%s", name) > 0)
			return bstrdup(path);
		else 
			return NULL;
	}
	else 
	{
		return os_get_config_path_ptr(name);
	}
#else
	return os_get_config_path_ptr(name);
#endif
}

int AFConfigManager::GetProgramDataPath(char* path, size_t size, const char* name) const
{
	return os_get_program_data_path(path, size, name);
}

char* AFConfigManager::GetProgramDataPathPtr(const char* name)
{
	return os_get_program_data_path_ptr(name);
}

int AFConfigManager::GetProfilePath(char* path, size_t size, const char* file) const
{
	char profiles_path[512];
	const char* profile =
		config_get_string(m_GlobalConfig, "Basic", "ProfileDir");
	int ret;

	if (!profile)
		return -1;
	if (!path)
		return -1;
	if (!file)
		file = "";

	ret = GetConfigPath(profiles_path, 512, "SOOPStudio/basic/profiles");
	if (ret <= 0)
		return ret;

	if (!*file)
		return snprintf(path, size, "%s/%s", profiles_path, profile);

	return snprintf(path, size, "%s/%s/%s", profiles_path, profile, file);
}

bool AFConfigManager::SetDefaultValuesBasicConfig(uint32_t cntAssocScreen, 
												  uint32_t cxPrimaryScreen, uint32_t cyPrimaryScreen,
												  float devicePixelRatio)
{
	if (m_bInitedGlobal == false  /*|| m_bInitedBasic == false*/)
		return false;


	if (cntAssocScreen == 0)
	{
		AFErrorBox(NULL, "There appears to be no monitors.  Er, this "
				   "technically shouldn't be possible.");
		return false;
	}


	uint32_t cx = cxPrimaryScreen, cy = cyPrimaryScreen;

	cx *= devicePixelRatio;
	cy *= devicePixelRatio;

	bool oldResolutionDefaults = config_get_bool(m_GlobalConfig, "General", "Pre19Defaults");

	/* use 1920x1080 for new default base res if main monitor is above
	 * 1920x1080, but don't apply for people from older builds -- only to
	 * new users */
	if (!oldResolutionDefaults && (cx * cy) > (1920 * 1080)) 
	{
		cx = 1920;
		cy = 1080;
	}

    SetBasicOutputConfig();
    SetBasicAudioConfig();
    SetBasicVideoConfig(cx, cy);

	bool changed = false;


	/* ----------------------------------------------------- */
	/* set twitch chat extensions to "both" if prev version  */
	/* is under 24.1                                         */
	if (config_get_bool(m_GlobalConfig, "General", "Pre24.1Defaults") &&
		!config_has_user_value(m_BasicConfig, "Twitch", "AddonChoice")) 
	{
		config_set_int(m_BasicConfig, "Twitch", "AddonChoice", 3);

		changed = true;
	}




	CheckExistingCookieId();

	return true;
}

void AFConfigManager::SetDefaultValues2BasicConfig()
{
	// _UpdateNvencPresets
	if (m_bInitedGlobal == false/* || m_bInitedBasic == false*/)
		return;


	bool oldEncDefaults = config_get_bool(m_GlobalConfig, "General",
					      "Pre23Defaults");
	//bool useNV = EncoderAvailable("ffmpeg_nvenc") && !oldEncDefaults;

	config_set_default_string(m_BasicConfig, "SimpleOutput", "StreamEncoder",
				 // useNV ? SIMPLE_ENCODER_NVENC
					/*:*/ SIMPLE_ENCODER_X264);
	config_set_default_string(m_BasicConfig, "SimpleOutput", "RecEncoder",
				 // useNV ? SIMPLE_ENCODER_NVENC
					/*:*/ SIMPLE_ENCODER_X264);

	const char *aac_default = "ffmpeg_aac";
	//if (EncoderAvailable("CoreAudio_AAC"))
	//	aac_default = "CoreAudio_AAC";
	//else if (EncoderAvailable("libfdk_aac"))
	//	aac_default = "libfdk_aac";

	config_set_default_string(m_BasicConfig, "AdvOut", "AudioEncoder",
							  aac_default);
	config_set_default_string(m_BasicConfig, "AdvOut", "RecAudioEncoder",
							  aac_default);

	/*if (_UpdateNvencPresets())
		config_save_safe(m_BasicConfig, "tmp", nullptr);*/
}

void AFConfigManager::SetBasicProgramConfig() {
	config_set_default_bool(m_BasicConfig, "General", "OpenStatsOnStartup",
                            false);

	config_save_safe(m_BasicConfig, "tmp", nullptr);
}

void AFConfigManager::SetGlobalProgramConfig() {
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "PreviewEnabled",
							true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"PreviewProgramMode", false);
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"SceneDuplicationMode", true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "SwapScenesMode",
							true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "SnappingEnabled",
							true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "ScreenSnapping",
							true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "SourceSnapping",
							true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "CenterSnapping",
							false);
	config_set_default_double(m_GlobalConfig, "BasicWindow", "SnapDistance",
							  10.0);
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"SpacingHelpersEnabled", true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"RecordWhenStreaming", false);
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"KeepRecordingWhenStreamStops", false);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "SysTrayEnabled",
							true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"SysTrayWhenStarted", false);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "SaveProjectors",
							false);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "ShowTransitions",
							true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"ShowListboxToolbars", true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "ShowStatusBar",
							true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "ShowSourceIcons",
							true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"ShowContextToolbars", true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow", "StudioModeLabels",
							true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"VerticalVolControl", false);

	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"MultiviewMouseSwitch", true);

	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"MultiviewDrawNames", true);

	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"MultiviewDrawAreas", true);
	
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"ShowPopupLeft", true);
	config_set_default_bool(m_GlobalConfig, "BasicWindow",
							"MediaControlsCountdownTimer", true);
	// -------------------------------------

    config_set_default_bool(m_GlobalConfig, "BasicWindow",
                            "WarnBeforeStartingStream", true);
    config_set_default_bool(m_GlobalConfig, "BasicWindow",
                            "WarnBeforeStoppingStream", true);
    config_set_default_bool(m_GlobalConfig, "BasicWindow",
                            "WarnBeforeStoppingRecord", false);
    config_set_default_bool(m_GlobalConfig, "BasicWindow", "OverflowHidden",
                            false);
    config_set_default_bool(m_GlobalConfig, "BasicWindow",
                            "OverflowAlwaysVisible", false);
    config_set_default_bool(m_GlobalConfig, "BasicWindow",
                            "OverflowSelectionHidden", false);
    config_set_default_bool(m_GlobalConfig, "BasicWindow", "ShowSafeAreas",
                            false);
    config_set_default_bool(m_GlobalConfig, "General",
                            "AutomaticCollectionSearch", false);
    config_set_default_bool(m_GlobalConfig, "BasicWindow",
                            "TransitionOnDoubleClick", false);
    config_set_default_bool(m_GlobalConfig, "BasicWindow",
                            "StudioPortraitLayout", false);

    config_save_safe(m_GlobalConfig, "tmp", nullptr);
}

void AFConfigManager::ResetProgramConfig() {
    config_remove_value(m_GlobalConfig, "BasicWindow", "PreviewEnabled");
    config_remove_value(m_GlobalConfig, "BasicWindow", "PreviewProgramMode");
    config_remove_value(m_GlobalConfig, "BasicWindow", "SceneDuplicationMode");
    config_remove_value(m_GlobalConfig, "BasicWindow", "SwapScenesMode");
    config_remove_value(m_GlobalConfig, "BasicWindow", "SnappingEnabled");
    config_remove_value(m_GlobalConfig, "BasicWindow", "ScreenSnapping");
    config_remove_value(m_GlobalConfig, "BasicWindow", "SourceSnapping");
    config_remove_value(m_GlobalConfig, "BasicWindow", "CenterSnapping");
    config_remove_value(m_GlobalConfig, "BasicWindow", "SnapDistance");
    config_remove_value(m_GlobalConfig, "BasicWindow", "SpacingHelpersEnabled");
    config_remove_value(m_GlobalConfig, "BasicWindow", "RecordWhenStreaming");
    config_remove_value(m_GlobalConfig, "BasicWindow", "KeepRecordingWhenStreamStops");
    config_remove_value(m_GlobalConfig, "BasicWindow", "SysTrayEnabled");
    config_remove_value(m_GlobalConfig, "BasicWindow", "SysTrayWhenStarted");
    config_remove_value(m_GlobalConfig, "BasicWindow", "SaveProjectors");
    config_remove_value(m_GlobalConfig, "BasicWindow", "ShowTransitions");
    config_remove_value(m_GlobalConfig, "BasicWindow", "ShowListboxToolbars");
    config_remove_value(m_GlobalConfig, "BasicWindow", "ShowStatusBar");
    config_remove_value(m_GlobalConfig, "BasicWindow", "ShowSourceIcons");
    config_remove_value(m_GlobalConfig, "BasicWindow", "ShowContextToolbars");
    config_remove_value(m_GlobalConfig, "BasicWindow", "StudioModeLabels");
    config_remove_value(m_GlobalConfig, "BasicWindow", "VerticalVolControl");

    config_remove_value(m_GlobalConfig, "BasicWindow", "MultiviewMouseSwitch");

    config_remove_value(m_GlobalConfig, "BasicWindow", "MultiviewDrawNames");

    config_remove_value(m_GlobalConfig, "BasicWindow", "MultiviewDrawAreas");

    config_remove_value(m_GlobalConfig, "BasicWindow", "ShowPopupLeft");
    config_remove_value(m_GlobalConfig, "BasicWindow", "MediaControlsCountdownTimer");
	// -------------------------------------

    config_remove_value(m_BasicConfig, "General", "OpenStatsOnStartup");
    config_remove_value(m_GlobalConfig, "BasicWindow",
                        "WarnBeforeStartingStream");
    config_remove_value(m_GlobalConfig, "BasicWindow",
                        "WarnBeforeStoppingStream");
    config_remove_value(m_GlobalConfig, "BasicWindow",
                        "WarnBeforeStoppingRecord");
    config_remove_value(m_GlobalConfig, "BasicWindow", "OverflowHidden");
    config_remove_value(m_GlobalConfig, "BasicWindow", "OverflowAlwaysVisible");
    config_remove_value(m_GlobalConfig, "BasicWindow",
                        "OverflowSelectionHidden");
    config_remove_value(m_GlobalConfig, "BasicWindow", "ShowSafeAreas");
    config_remove_value(m_GlobalConfig, "General", "AutomaticCollectionSearch");
    config_remove_value(m_GlobalConfig, "BasicWindow",
                        "TransitionOnDoubleClick");
    config_remove_value(m_GlobalConfig, "BasicWindow",
                            "StudioPortraitLayout");

    const char* installLang =
        config_get_string(m_GlobalConfig, "General", "LanguageBase");
    if (installLang) { 
        config_set_string(m_GlobalConfig, "General", "Language", installLang);
    }

    SetGlobalProgramConfig();
}

void AFConfigManager::SetBasicStreamConfig() {}

void AFConfigManager::ResetStreamConfig() {}

void AFConfigManager::ResetOutputConfig() {
    // Delete configs
    config_remove_value(m_BasicConfig, "Output", "Mode");
    config_remove_value(m_BasicConfig, "Stream1", "IgnoreRecommended");
    config_remove_value(m_BasicConfig, "SimpleOutput", "FilePath");
    config_remove_value(m_BasicConfig, "SimpleOutput", "RecFormat2");
    config_remove_value(m_BasicConfig, "SimpleOutput", "VBitrate");
    config_remove_value(m_BasicConfig, "SimpleOutput", "ABitrate");
    config_remove_value(m_BasicConfig, "SimpleOutput", "UseAdvanced");
    config_remove_value(m_BasicConfig, "SimpleOutput", "Preset");
    config_remove_value(m_BasicConfig, "SimpleOutput", "NVENCPreset2");
    config_remove_value(m_BasicConfig, "SimpleOutput", "RecQuality");
    config_remove_value(m_BasicConfig, "SimpleOutput", "RecRB");
    config_remove_value(m_BasicConfig, "SimpleOutput", "RecRBTime");
    config_remove_value(m_BasicConfig, "SimpleOutput", "RecRBSize");
    config_remove_value(m_BasicConfig, "SimpleOutput", "RecRBPrefix");
    config_remove_value(m_BasicConfig, "SimpleOutput", "StreamAudioEncoder");
    config_remove_value(m_BasicConfig, "SimpleOutput", "RecAudioEncoder");
    config_remove_value(m_BasicConfig, "SimpleOutput", "RecTracks");

    config_remove_value(m_BasicConfig, "AdvOut", "ApplyServiceSettings");
	config_remove_value(m_BasicConfig, "AdvOut", "FFRescale");
	config_remove_value(m_BasicConfig, "AdvOut", "RecRescale");
    config_remove_value(m_BasicConfig, "AdvOut", "UseRescale");
    config_remove_value(m_BasicConfig, "AdvOut", "TrackIndex");
    config_remove_value(m_BasicConfig, "AdvOut", "VodTrackIndex");
    config_remove_value(m_BasicConfig, "AdvOut", "Encoder");

    config_remove_value(m_BasicConfig, "AdvOut", "RecType");

    config_remove_value(m_BasicConfig, "AdvOut", "RecFilePath");
    config_remove_value(m_BasicConfig, "AdvOut", "RecFormat2");
    config_remove_value(m_BasicConfig, "AdvOut", "RecUseRescale");
    config_remove_value(m_BasicConfig, "AdvOut", "RecTracks");
    config_remove_value(m_BasicConfig, "AdvOut", "RecEncoder");
    config_remove_value(m_BasicConfig, "AdvOut", "FLVTrack");

    config_remove_value(m_BasicConfig, "AdvOut", "FFOutputToFile");
    config_remove_value(m_BasicConfig, "AdvOut", "FFFilePath");
    config_remove_value(m_BasicConfig, "AdvOut", "FFExtension");
    config_remove_value(m_BasicConfig, "AdvOut", "FFVBitrate");
    config_remove_value(m_BasicConfig, "AdvOut", "FFVGOPSize");
    config_remove_value(m_BasicConfig, "AdvOut", "FFUseRescale");
    config_remove_value(m_BasicConfig, "AdvOut", "FFIgnoreCompat");
    config_remove_value(m_BasicConfig, "AdvOut", "FFABitrate");
    config_remove_value(m_BasicConfig, "AdvOut", "FFAudioMixes");

    config_remove_value(m_BasicConfig, "AdvOut", "Track1Bitrate");
    config_remove_value(m_BasicConfig, "AdvOut", "Track2Bitrate");
    config_remove_value(m_BasicConfig, "AdvOut", "Track3Bitrate");
    config_remove_value(m_BasicConfig, "AdvOut", "Track4Bitrate");
    config_remove_value(m_BasicConfig, "AdvOut", "Track5Bitrate");
    config_remove_value(m_BasicConfig, "AdvOut", "Track6Bitrate");

    config_remove_value(m_BasicConfig, "AdvOut", "RecSplitFileTime");
    config_remove_value(m_BasicConfig, "AdvOut", "RecSplitFileSize");

    config_remove_value(m_BasicConfig, "AdvOut", "RecRB");
    config_remove_value(m_BasicConfig, "AdvOut", "RecRBTime");
    config_remove_value(m_BasicConfig, "AdvOut", "RecRBSize");

    config_remove_value(m_BasicConfig, "Output", "FilenameFormatting");

    config_remove_value(m_BasicConfig, "Output", "DelayEnable");
    config_remove_value(m_BasicConfig, "Output", "DelaySec");
    config_remove_value(m_BasicConfig, "Output", "DelayPreserve");

    config_remove_value(m_BasicConfig, "Output", "Reconnect");
    config_remove_value(m_BasicConfig, "Output", "RetryDelay");
    config_remove_value(m_BasicConfig, "Output", "MaxRetries");

    config_remove_value(m_BasicConfig, "Output", "BindIP");
    config_remove_value(m_BasicConfig, "Output", "IPFamily");
    config_remove_value(m_BasicConfig, "Output", "NewSocketLoopEnable");
    config_remove_value(m_BasicConfig, "Output", "LowLatencyEnable");

    // SetDefaultValues2BasicConfig()
    config_remove_value(m_BasicConfig, "SimpleOutput", "StreamEncoder");
    config_remove_value(m_BasicConfig, "SimpleOutput", "RecEncoder");

    config_remove_value(m_BasicConfig, "AdvOut", "AudioEncoder");
    config_remove_value(m_BasicConfig, "AdvOut", "RecAudioEncoder");
    
    // Set default values
    SetBasicOutputConfig();
}

void AFConfigManager::SetBasicOutputConfig() {
    bool changed = false;
	/* ----------------------------------------------------- */
	/* move over old FFmpeg track settings                   */
	if (config_has_user_value(m_BasicConfig, "AdvOut", "FFAudioTrack") &&
		!config_has_user_value(m_BasicConfig, "AdvOut", "Pre22.1Settings")) 
	{

		int track = (int)config_get_int(m_BasicConfig, "AdvOut",
										"FFAudioTrack");
		config_set_int(m_BasicConfig, "AdvOut", "FFAudioMixes",
					   1LL << (track - 1));
		config_set_bool(m_BasicConfig, "AdvOut", "Pre22.1Settings", true);
        changed = true;
	}

	/* ----------------------------------------------------- */
	/* move over mixer values in advanced if older config */
	if (config_has_user_value(m_BasicConfig, "AdvOut", "RecTrackIndex") &&
		!config_has_user_value(m_BasicConfig, "AdvOut", "RecTracks")) 
	{

		uint64_t track =
			config_get_uint(m_BasicConfig, "AdvOut", "RecTrackIndex");
		track = 1ULL << (track - 1);
		config_set_uint(m_BasicConfig, "AdvOut", "RecTracks", track);
		config_remove_value(m_BasicConfig, "AdvOut", "RecTrackIndex");
        changed = true;
	}
	/* ----------------------------------------------------- */
	/* move bitrate enforcement setting to new value         */
	if (config_has_user_value(m_BasicConfig, "SimpleOutput",
							  "EnforceBitrate") &&
		!config_has_user_value(m_BasicConfig, "Stream1",
							   "IgnoreRecommended") &&
		!config_has_user_value(m_BasicConfig, "Stream1", "MovedOldEnforce")) 
	{
		bool enforce = config_get_bool(m_BasicConfig, "SimpleOutput",
									   "EnforceBitrate");
		config_set_bool(m_BasicConfig, "Stream1", "IgnoreRecommended",
						!enforce);
		config_set_bool(m_BasicConfig, "Stream1", "MovedOldEnforce",
						true);
        changed = true;
	}
	/* ----------------------------------------------------- */
	/* enforce minimum retry delay of 1 second prior to 27.1 */
	if (config_has_user_value(m_BasicConfig, "Output", "RetryDelay")) 
	{
		int retryDelay =
			config_get_uint(m_BasicConfig, "Output", "RetryDelay");
		if (retryDelay < 1) 
		{
			config_set_uint(m_BasicConfig, "Output", "RetryDelay", 1);
            changed = true;
		}
	}

	/* ----------------------------------------------------- */
	/* Migrate old container selection (if any) to new key.  */
	auto MigrateFormat = [&](const char* section) {
		bool has_old_key = config_has_user_value(m_BasicConfig, section,
			"RecFormat");
		bool has_new_key = config_has_user_value(m_BasicConfig, section,
			"RecFormat2");
		if (!has_new_key && !has_old_key)
			return;

		std::string old_format = config_get_string(
			m_BasicConfig, section,
			has_new_key ? "RecFormat2" : "RecFormat");
		std::string new_format = old_format;
		if (old_format == "ts")
			new_format = "mpegts";
		else if (old_format == "m3u8")
			new_format = "hls";
		else if (old_format == "fmp4")
			new_format = "fragmented_mp4";
		else if (old_format == "fmov")
			new_format = "fragmented_mov";

		if (new_format != old_format || !has_new_key) {
			config_set_string(m_BasicConfig, section, "RecFormat2",
				new_format.c_str());
            changed = true;
		}
		};

	MigrateFormat("AdvOut");
	MigrateFormat("SimpleOutput");

    config_set_default_string(m_BasicConfig, "Output", "Mode", "Simple");
	config_set_default_bool(m_BasicConfig, "Stream1", "IgnoreRecommended",
							false);
	config_set_default_string(m_BasicConfig, "SimpleOutput", "FilePath",
							  GetDefaultVideoSavePath().c_str());
	config_set_default_string(m_BasicConfig, "SimpleOutput", "RecFormat2",
							  DEFAULT_CONTAINER);
	config_set_default_uint(m_BasicConfig, "SimpleOutput", "VBitrate", 6000);
	config_set_default_uint(m_BasicConfig, "SimpleOutput", "ABitrate", 160);
	config_set_default_bool(m_BasicConfig, "SimpleOutput", "UseAdvanced",
							false);
	config_set_default_string(m_BasicConfig, "SimpleOutput", "Preset",
							  "veryfast");
	config_set_default_string(m_BasicConfig, "SimpleOutput", "NVENCPreset2",
							  "p5");
	config_set_default_string(m_BasicConfig, "SimpleOutput", "RecQuality",
							  "Stream");
	config_set_default_bool(m_BasicConfig, "SimpleOutput", "RecRB", false);
	config_set_default_int(m_BasicConfig, "SimpleOutput", "RecRBTime", 20);
	config_set_default_int(m_BasicConfig, "SimpleOutput", "RecRBSize", 512);
	config_set_default_string(m_BasicConfig, "SimpleOutput", "RecRBPrefix",
							  "Replay");
	config_set_default_string(m_BasicConfig, "SimpleOutput",
							  "StreamAudioEncoder", "aac");
	config_set_default_string(m_BasicConfig, "SimpleOutput",
							  "RecAudioEncoder", "aac");
	config_set_default_uint(m_BasicConfig, "SimpleOutput", "RecTracks",
							(1 << 0));

	config_set_default_bool(m_BasicConfig, "AdvOut", "ApplyServiceSettings",
							true);

	config_set_default_bool(m_BasicConfig, "AdvOut", "FFRescale", false);
	config_set_default_bool(m_BasicConfig, "AdvOut", "RecRescale", false);
	config_set_default_bool(m_BasicConfig, "AdvOut", "UseRescale", false);
	config_set_default_uint(m_BasicConfig, "AdvOut", "TrackIndex", 1);
	config_set_default_uint(m_BasicConfig, "AdvOut", "VodTrackIndex", 2);
	config_set_default_string(m_BasicConfig, "AdvOut", "Encoder", "obs_x264");

	config_set_default_string(m_BasicConfig, "AdvOut", "RecType", "Standard");

	config_set_default_string(m_BasicConfig, "AdvOut", "RecFilePath",
							  GetDefaultVideoSavePath().c_str());
	config_set_default_string(m_BasicConfig, "AdvOut", "RecFormat2",
							  DEFAULT_CONTAINER);
	config_set_default_bool(m_BasicConfig, "AdvOut", "RecUseRescale", false);
	config_set_default_uint(m_BasicConfig, "AdvOut", "RecTracks", (1 << 0));
	config_set_default_string(m_BasicConfig, "AdvOut", "RecEncoder", "none");
	config_set_default_uint(m_BasicConfig, "AdvOut", "FLVTrack", 1);

	config_set_default_bool(m_BasicConfig, "AdvOut", "FFOutputToFile", true);
	config_set_default_string(m_BasicConfig, "AdvOut", "FFFilePath",
							  GetDefaultVideoSavePath().c_str());
	config_set_default_string(m_BasicConfig, "AdvOut", "FFExtension", "mp4");
	config_set_default_uint(m_BasicConfig, "AdvOut", "FFVBitrate", 2500);
	config_set_default_uint(m_BasicConfig, "AdvOut", "FFVGOPSize", 250);
	config_set_default_bool(m_BasicConfig, "AdvOut", "FFUseRescale", false);
	config_set_default_bool(m_BasicConfig, "AdvOut", "FFIgnoreCompat", false);
	config_set_default_uint(m_BasicConfig, "AdvOut", "FFABitrate", 160);
	config_set_default_uint(m_BasicConfig, "AdvOut", "FFAudioMixes", 1);

	config_set_default_uint(m_BasicConfig, "AdvOut", "Track1Bitrate", 160);
	config_set_default_uint(m_BasicConfig, "AdvOut", "Track2Bitrate", 160);
	config_set_default_uint(m_BasicConfig, "AdvOut", "Track3Bitrate", 160);
	config_set_default_uint(m_BasicConfig, "AdvOut", "Track4Bitrate", 160);
	config_set_default_uint(m_BasicConfig, "AdvOut", "Track5Bitrate", 160);
	config_set_default_uint(m_BasicConfig, "AdvOut", "Track6Bitrate", 160);

	config_set_default_uint(m_BasicConfig, "AdvOut", "RecSplitFileTime", 15);
	config_set_default_uint(m_BasicConfig, "AdvOut", "RecSplitFileSize",
							2048);

	config_set_default_bool(m_BasicConfig, "AdvOut", "RecRB", false);
	config_set_default_uint(m_BasicConfig, "AdvOut", "RecRBTime", 20);
	config_set_default_int(m_BasicConfig, "AdvOut", "RecRBSize", 512);

	config_set_default_string(m_BasicConfig, "Output", "FilenameFormatting",
							  "%CCYY-%MM-%DD %hh-%mm-%ss");

	config_set_default_bool(m_BasicConfig, "Output", "DelayEnable", false);
	config_set_default_uint(m_BasicConfig, "Output", "DelaySec", 20);
	config_set_default_bool(m_BasicConfig, "Output", "DelayPreserve", true);

	config_set_default_bool(m_BasicConfig, "Output", "Reconnect", true);
	config_set_default_uint(m_BasicConfig, "Output", "RetryDelay", 2);
	config_set_default_uint(m_BasicConfig, "Output", "MaxRetries", 25);

	config_set_default_string(m_BasicConfig, "Output", "BindIP", "default");
	config_set_default_string(m_BasicConfig, "Output", "IPFamily",
							  "IPv4+IPv6");
	config_set_default_bool(m_BasicConfig, "Output", "NewSocketLoopEnable",
							false);
	config_set_default_bool(m_BasicConfig, "Output", "LowLatencyEnable",
                            false);

    SetDefaultValues2BasicConfig();

    config_save_safe(m_BasicConfig, "tmp", nullptr);
}

void AFConfigManager::SetBasicAudioConfig() {
	config_set_default_string(m_BasicConfig, "Audio", "MonitoringDeviceId",
							  "default");
	config_set_default_string(m_BasicConfig, "Audio", "MonitoringDeviceName",
							  AFLocaleTextManager::GetSingletonInstance().Str(
								"Basic.Settings.Advanced.Audio.MonitoringDevice"
							  	".Default")
							  );
	config_set_default_uint(m_BasicConfig, "Audio", "SampleRate", 48000);
	config_set_default_string(m_BasicConfig, "Audio", "ChannelSetup",
							  "Stereo");
	config_set_default_double(m_BasicConfig, "Audio", "MeterDecayRate",
							  VOLUME_METER_DECAY_FAST);
	config_set_default_uint(m_BasicConfig, "Audio", "PeakMeterType", 0);
    config_save_safe(m_BasicConfig, "tmp", nullptr);
}

void AFConfigManager::SetGlobalAudioConfig() {
#ifdef _WIN32
    config_set_default_bool(m_GlobalConfig, "Audio", "DisableAudioDucking",
                            true);
    config_save_safe(m_GlobalConfig, "tmp", nullptr);
#endif
}

void AFConfigManager::ResetAudioConfig() {
    // Remove exists
    config_remove_value(m_BasicConfig, "Audio", "MonitoringDeviceId");
    config_remove_value(m_BasicConfig, "Audio", "MonitoringDeviceName");
    config_remove_value(m_BasicConfig, "Audio", "SampleRate");
    config_remove_value(m_BasicConfig, "Audio", "ChannelSetup");
    config_remove_value(m_BasicConfig, "Audio", "MeterDecayRate");
    config_remove_value(m_BasicConfig, "Audio", "PeakMeterType");
#ifdef _WIN32
    config_remove_value(m_GlobalConfig, "Audio", "DisableAudioDucking");
#endif
    // Set defaults
    SetBasicAudioConfig();
    SetGlobalAudioConfig();
}

void AFConfigManager::SetBasicVideoConfig(uint32_t cx, uint32_t cy) {
    
	config_set_default_uint(m_BasicConfig, "Video", "BaseCX", cx);
	config_set_default_uint(m_BasicConfig, "Video", "BaseCY", cy);

	/* don't allow BaseCX/BaseCY to be susceptible to defaults changing */
	if (!config_has_user_value(m_BasicConfig, "Video", "BaseCX") ||
		!config_has_user_value(m_BasicConfig, "Video", "BaseCY")) {
		config_set_uint(m_BasicConfig, "Video", "BaseCX", cx);
		config_set_uint(m_BasicConfig, "Video", "BaseCY", cy);
	}

	int i = 0;
	uint32_t scale_cx = cx;
	uint32_t scale_cy = cy;

	///* use a default scaled resolution that has a pixel count no higher
	// * than 1280x720 */
	//while (((scale_cx * scale_cy) > (1280 * 720)) && scaled_vals[i] > 0.0)
	//{
	//	double scale = scaled_vals[i++];
	//	scale_cx = uint32_t(double(cx) / scale);
	//	scale_cy = uint32_t(double(cy) / scale);
	//}

	//config_set_default_uint(m_BasicConfig, "Video", "OutputCX", scale_cx);
	//config_set_default_uint(m_BasicConfig, "Video", "OutputCY", scale_cy);

	///* don't allow OutputCX/OutputCY to be susceptible to defaults
	// * changing */
	//if (!config_has_user_value(m_BasicConfig, "Video", "OutputCX") ||
	//	!config_has_user_value(m_BasicConfig, "Video", "OutputCY"))
	//{
	//	config_set_uint(m_BasicConfig, "Video", "OutputCX", scale_cx);
	//	config_set_uint(m_BasicConfig, "Video", "OutputCY", scale_cy);
	//}

	config_set_default_uint(m_BasicConfig, "Video", "OutputCX", 1920);
	config_set_default_uint(m_BasicConfig, "Video", "OutputCY", 1080);

	config_set_default_uint(m_BasicConfig, "Video", "FPSType", 0);
	config_set_default_string(m_BasicConfig, "Video", "FPSCommon", "30");
	config_set_default_uint(m_BasicConfig, "Video", "FPSInt", 30);
	config_set_default_uint(m_BasicConfig, "Video", "FPSNum", 30);
	config_set_default_uint(m_BasicConfig, "Video", "FPSDen", 1);
	config_set_default_string(m_BasicConfig, "Video", "ScaleType", "bicubic");
	config_set_default_string(m_BasicConfig, "Video", "ColorFormat", "NV12");
	config_set_default_string(m_BasicConfig, "Video", "ColorSpace", "709");
	config_set_default_string(m_BasicConfig, "Video", "ColorRange",
							  "Partial");
	config_set_default_uint(m_BasicConfig, "Video", "SdrWhiteLevel", 300);
	config_set_default_uint(m_BasicConfig, "Video", "HdrNominalPeakLevel",
							1000);
	config_set_default_bool(m_BasicConfig, "AdvOut", "Rescale", false);

    config_save_safe(m_BasicConfig, "tmp", nullptr);
}

void AFConfigManager::SetGlobalVideoConfig() {
#if _WIN32
	config_set_default_string(m_GlobalConfig, "Video", "Renderer",
							  "Direct3D 11");
#else
	config_set_default_string(m_GlobalConfig, "Video", "Renderer", "OpenGL");
	config_set_default_bool(m_GlobalConfig, "Video", "DisableOSXVSync", true);
	config_set_default_bool(m_GlobalConfig, "Video", "ResetOSXVSyncOnExit",
							true);
#endif
    config_save_safe(m_GlobalConfig, "tmp", nullptr);
}

void AFConfigManager::ResetVideoConfig(uint32_t cx, uint32_t cy) {
    config_remove_value(m_BasicConfig, "Video", "BaseCX");
    config_remove_value(m_BasicConfig, "Video", "BaseCY");
	config_remove_value(m_BasicConfig, "Video", "OutputCX");
	config_remove_value(m_BasicConfig, "Video", "OutputCY");
	config_remove_value(m_BasicConfig, "Video", "FPSType");
	config_remove_value(m_BasicConfig, "Video", "FPSCommon");
	config_remove_value(m_BasicConfig, "Video", "FPSInt");
	config_remove_value(m_BasicConfig, "Video", "FPSNum");
	config_remove_value(m_BasicConfig, "Video", "FPSDen");
	config_remove_value(m_BasicConfig, "Video", "ScaleType");
	config_remove_value(m_BasicConfig, "Video", "ColorFormat");
	config_remove_value(m_BasicConfig, "Video", "ColorSpace");
	config_remove_value(m_BasicConfig, "Video", "ColorRange");
	config_remove_value(m_BasicConfig, "Video", "SdrWhiteLevel");
	config_remove_value(m_BasicConfig, "Video", "HdrNominalPeakLevel");
	config_remove_value(m_BasicConfig, "AdvOut", "Rescale");
#if _WIN32
	config_remove_value(m_GlobalConfig, "Video", "Renderer");
#else
	config_remove_value(m_GlobalConfig, "Video", "Renderer");
	config_remove_value(m_GlobalConfig, "Video", "DisableOSXVSync");
	config_remove_value(m_GlobalConfig, "Video", "ResetOSXVSyncOnExit");
#endif
    SetBasicVideoConfig(cx, cy);
    SetGlobalVideoConfig();
}

void AFConfigManager::SetBasicHotkeysConfig() {}

void AFConfigManager::SetGlobalAccessibilityConfig() {
    config_set_default_int(m_GlobalConfig, "Accessibility", "SelectRed",
                           0x01ffd1);
    config_set_default_int(m_GlobalConfig, "Accessibility", "SelectGreen",
                           0x5141c3);
    config_set_default_int(m_GlobalConfig, "Accessibility", "SelectBlue",
                           0xffe000);
    config_set_default_int(m_GlobalConfig, "Accessibility", "MixerGreen",
                           0x2e4821);
    config_set_default_int(m_GlobalConfig, "Accessibility", "MixerYellow",
                           0x275e63);
    config_set_default_int(m_GlobalConfig, "Accessibility", "MixerRed",
                           0x222255);
    config_set_default_int(m_GlobalConfig, "Accessibility", "MixerGreenActive",
                           0x4cff4c);
    config_set_default_int(m_GlobalConfig, "Accessibility", "MixerYellowActive",
                           0x4cffff);
    config_set_default_int(m_GlobalConfig, "Accessibility", "MixerRedActive",
                           0x4c4cff);
    config_set_default_int(m_GlobalConfig, "Accessibility", "ColorPreset",
                           ColorPreset::COLOR_PRESET_DEFAULT);

    config_save_safe(m_GlobalConfig, "tmp", nullptr);
}

void AFConfigManager::ResetAccessibilityConfig() {
    config_remove_value(m_GlobalConfig, "Accessibility", "SelectRed");
    config_remove_value(m_GlobalConfig, "Accessibility", "SelectGreen");
    config_remove_value(m_GlobalConfig, "Accessibility", "SelectBlue");
    config_remove_value(m_GlobalConfig, "Accessibility", "MixerGreen");
    config_remove_value(m_GlobalConfig, "Accessibility", "MixerYellow");
    config_remove_value(m_GlobalConfig, "Accessibility", "MixerRed");
    config_remove_value(m_GlobalConfig, "Accessibility", "MixerGreenActive");
    config_remove_value(m_GlobalConfig, "Accessibility", "MixerYellowActive");
    config_remove_value(m_GlobalConfig, "Accessibility", "MixerRedActive");

    config_remove_value(m_GlobalConfig, "Accessibility", "ColorPreset");

    SetGlobalAccessibilityConfig();
}

void AFConfigManager::SetBasicAdvancedConfig() {
#ifdef _WIN32
	config_set_default_bool(m_BasicConfig, "Output", "NewSocketLoopEnable",
							false);
	config_set_default_bool(m_BasicConfig, "Output", "LowLatencyEnable",
                            false);
#endif
	config_set_default_string(m_BasicConfig, "Output", "FilenameFormatting",
							  "%CCYY-%MM-%DD %hh-%mm-%ss");
    config_set_default_string(m_BasicConfig, "SimpleOutput", "RecRBPrefix",
                            "Replay");

	config_set_default_bool(m_BasicConfig, "Output", "Reconnect", true);
	config_set_default_uint(m_BasicConfig, "Output", "RetryDelay", 2);
	config_set_default_uint(m_BasicConfig, "Output", "MaxRetries", 25);

	config_set_default_string(m_BasicConfig, "Output", "IPFamily",
							  "IPv4+IPv6");
	config_set_default_string(m_BasicConfig, "Output", "BindIP", "default");

    config_save_safe(m_BasicConfig, "tmp", nullptr);
}

void AFConfigManager::SetGlobalAdvancedConfig() {
#ifdef _WIN32
    config_set_default_string(m_GlobalConfig, "General", "ProcessPriority",
                            "Normal");
#endif
	config_set_default_bool(m_GlobalConfig, "General", "ConfirmOnExit", true);
    
    // _WIN32 || __APPLE__
	config_set_default_bool(m_GlobalConfig, "General", "BrowserHWAccel",
							true);

    config_save_safe(m_GlobalConfig, "tmp", nullptr);
}

void AFConfigManager::ResetAdvancedConfig() {
    // Remove basic configs
#ifdef _WIN32
	config_remove_value(m_BasicConfig, "Output", "NewSocketLoopEnable");
	config_remove_value(m_BasicConfig, "Output", "LowLatencyEnable");
#endif
	config_remove_value(m_BasicConfig, "Output", "FilenameFormatting");
    config_remove_value(m_BasicConfig, "SimpleOutput", "RecRBPrefix");

	config_remove_value(m_BasicConfig, "Output", "Reconnect");
	config_remove_value(m_BasicConfig, "Output", "RetryDelay");
	config_remove_value(m_BasicConfig, "Output", "MaxRetries");

	config_remove_value(m_BasicConfig, "Output", "IPFamily");
	config_remove_value(m_BasicConfig, "Output", "BindIP");

    // Remove global configs
#ifdef _WIN32
    config_remove_value(m_GlobalConfig, "General", "ProcessPriority");
#endif
	config_remove_value(m_GlobalConfig, "General", "ConfirmOnExit");
    
    // _WIN32 || __APPLE__
	config_remove_value(m_GlobalConfig, "General", "BrowserHWAccel");

    // Set default
    SetBasicAdvancedConfig();
    SetGlobalAdvancedConfig();
}


void AFConfigManager::UpdateHotkeyFocusSetting(bool resetState, bool appActiveState)
{
    m_StatesApp.SetEnableHotkeysInFocus(true);
    m_StatesApp.SetEnableHotkeysOutOfFocus(true);

    if (resetState) {
        config_remove_value(m_GlobalConfig, "General", "HotkeyFocusType");
        config_set_default_string(m_GlobalConfig, "General",
                                  "HotkeyFocusType", "NeverDisableHotkeys");
        SafeSaveGlobal();
    }

    const char *hotkeyFocusType =
        config_get_string(m_GlobalConfig, "General", "HotkeyFocusType");

    if (astrcmpi(hotkeyFocusType, "DisableHotkeysInFocus") == 0)
        m_StatesApp.SetEnableHotkeysInFocus(false);
    else if (astrcmpi(hotkeyFocusType, "DisableHotkeysOutOfFocus") == 0)
        m_StatesApp.SetEnableHotkeysOutOfFocus(false);

    if (resetState) {
        ResetHotkeyState(appActiveState);
    }

}
void AFConfigManager::DisableHotkeys(bool appActiveState)
{
    m_StatesApp.SetEnableHotkeysInFocus(false);
    m_StatesApp.SetEnableHotkeysOutOfFocus(false);
    
    ResetHotkeyState(appActiveState);
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
bool AFConfigManager::GetUnusedSceneCollectionFile(std::string& name, std::string& file)
{
	char path[512];
	int ret;

	if(!GetFileSafeName(name.c_str(), file)) {
		blog(LOG_WARNING, "Failed to create safe file name for '%s'",
			 name.c_str());
		return false;
	}

	ret = GetConfigPath(path, sizeof(path), "SOOPStudio/basic/scenes/");
	if(ret <= 0) {
		blog(LOG_WARNING, "Failed to get scene collection config path");
		return false;
	}

	file.insert(0, path);

	if(!GetClosestUnusedFileName(file, "json")) {
		blog(LOG_WARNING, "Failed to get closest file name for %s",
			 file.c_str());
		return false;
	}

	file.erase(file.size() - 5, 5);
	file.erase(0, strlen(path));
	return true;
}

void AFConfigManager::_MakeArg()
{
	if (m_pArgOption == nullptr)
		m_pArgOption = new AFArgOption();
}
//
bool AFConfigManager::_do_mkdir(const char* path)
{
	if(os_mkdirs(path) == MKDIR_ERROR) {
		AFErrorBox(NULL, "Failed to create directory %s", path);
		return false;
	}

	return true;
}
bool AFConfigManager::_MakeUserDirs()
{
	char path[512] = {0,};
	if(GetConfigPath(path, sizeof(path), "SOOPStudio/basic") <= 0)
		return false;
	if(!_do_mkdir(path))
		return false;

	if(GetConfigPath(path, sizeof(path), "SOOPStudio/logs") <= 0)
		return false;
	if(!_do_mkdir(path))
		return false;

	if(GetConfigPath(path, sizeof(path), "SOOPStudio/profiler_data") <= 0)
		return false;
	if(!_do_mkdir(path))
		return false;

#ifdef _WIN32
	if(GetConfigPath(path, sizeof(path), "SOOPStudio/crashes") <= 0)
		return false;
	if(!_do_mkdir(path))
		return false;
#endif

#ifdef WHATSNEW_ENABLED
	if(GetConfigPath(path, sizeof(path), "SOOPStudio/updates") <= 0)
		return false;
	if(!_do_mkdir(path))
		return false;
#endif

	if(GetConfigPath(path, sizeof(path), "SOOPStudio/plugin_config") <= 0)
		return false;
	if(!_do_mkdir(path))
		return false;

	return true;
}
bool AFConfigManager::_MakeUserProfileDirs()
{
	char path[512] = {0,};

	if(GetConfigPath(path, sizeof(path), "SOOPStudio/basic/profiles") <= 0)
		return false;
	if(!_do_mkdir(path))
		return false;

	if(GetConfigPath(path, sizeof(path), "SOOPStudio/basic/scenes") <= 0)
		return false;
	if(!_do_mkdir(path))
		return false;

	return true;
}
std::string AFConfigManager::_GetSceneCollectionFileFromName(const char* name)
{
	std::string outputPath;
	os_glob_t* glob;
	char path[512];

	if (GetConfigPath(path, sizeof(path), "SOOPStudio/basic/scenes") <= 0)
		return outputPath;

	strcat(path, "/*.json");

	if (os_glob(path, 0, &glob) != 0)
		return outputPath;

	for (size_t i = 0; i < glob->gl_pathc; i++) 
	{
		struct os_globent ent = glob->gl_pathv[i];
		if (ent.directory)
			continue;

		OBSDataAutoRelease data =
			obs_data_create_from_json_file_safe(ent.path, "bak");
		const char* curName = obs_data_get_string(data, "name");

		if (astrcmpi(name, curName) == 0) 
		{
			outputPath = ent.path;
			break;
		}
	}

	os_globfree(glob);

	if (!outputPath.empty()) 
	{
		outputPath.resize(outputPath.size() - 5);
		replace(outputPath.begin(), outputPath.end(), '\\', '/');
		const char* start = strrchr(outputPath.c_str(), '/');
		if (start)
			outputPath.erase(0, start - outputPath.c_str() + 1);
	}


	return outputPath;
}

std::string AFConfigManager::_GetProfileDirFromName(const char* name)
{
	std::string outputPath;
	os_glob_t* glob = nullptr;
	char path[512] = {0, };

	if (GetConfigPath(path, sizeof(path), "SOOPStudio/basic/profiles") <= 0)
		return outputPath;

	strcat(path, "/*");

	if (os_glob(path, 0, &glob) != 0)
		return outputPath;

	for (size_t i = 0; i < glob->gl_pathc; i++) 
	{
		struct os_globent ent = glob->gl_pathv[i];
		if (!ent.directory)
			continue;

		strcpy(path, ent.path);
		strcat(path, "/basic.ini");

		ConfigFile config;
		if (config.Open(path, CONFIG_OPEN_EXISTING) != 0)
			continue;

		const char* curName =
			config_get_string(config, "General", "Name");
		if (astrcmpi(curName, name) == 0) 
		{
			outputPath = ent.path;
			break;
		}
	}

	os_globfree(glob);

	if (!outputPath.empty()) 
	{
		replace(outputPath.begin(), outputPath.end(), '\\', '/');
		const char* start = strrchr(outputPath.c_str(), '/');
		if (start)
			outputPath.erase(0, start - outputPath.c_str() + 1);
	}

	return outputPath;
}

bool AFConfigManager::_UpdateNvencPresets()
{
	if (config_has_user_value(m_BasicConfig, "SimpleOutput", "NVENCPreset2") ||
		!config_has_user_value(m_BasicConfig, "SimpleOutput", "NVENCPreset"))
		return false;

	const char* streamEncoder =
		config_get_string(m_BasicConfig, "SimpleOutput", "StreamEncoder");
	const char* nvencPreset =
		config_get_string(m_BasicConfig, "SimpleOutput", "NVENCPreset");

	OBSDataAutoRelease data = obs_data_create();
	obs_data_set_string(data, "preset", nvencPreset);

	//if (astrcmpi(streamEncoder, "nvenc_hevc") == 0) {
	//	convert_nvenc_hevc_presets(data);
	//}
	//else {
	//	convert_nvenc_h264_presets(data);
	//}

	config_set_string(m_BasicConfig, "SimpleOutput", "NVENCPreset2",
					  obs_data_get_string(data, "preset2"));

	return true;
}

void AFConfigManager::_SetDefaultValuesGlobalConfig()
{
    SetGlobalProgramConfig();
    SetGlobalAudioConfig();
    SetGlobalVideoConfig();
    SetGlobalAccessibilityConfig();

	config_set_default_uint(m_GlobalConfig, "General", "MaxLogs", 10);
	config_set_default_int(m_GlobalConfig, "General", "InfoIncrement", -1);
	config_set_default_string(m_GlobalConfig, "General", "ProcessPriority",
							  "Normal");
	config_set_default_bool(m_GlobalConfig, "General", "EnableAutoUpdates",
							true);

	config_set_default_bool(m_GlobalConfig, "General", "ConfirmOnExit", true);



	config_set_default_string(m_GlobalConfig, "General", "HotkeyFocusType",
							  "NeverDisableHotkeys");

#ifdef _WIN32
	config_set_default_bool(m_GlobalConfig, "Audio", "DisableAudioDucking", 
							true);
	config_set_default_bool(m_GlobalConfig, "General", "BrowserHWAccel",
							true);
#endif

#ifdef __APPLE__
	config_set_default_bool(m_GlobalConfig, "General", "BrowserHWAccel",
							true);
#endif
	return;
}

bool AFConfigManager::_InitGlobalConfig()
{
	ProfileScope("AFConfigManager::_InitGlobalConfig");


	bool res = false;


	char path[512];
	bool changed = false;

	int len = GetConfigPath(path, sizeof(path), "SOOPStudio/global.ini");
	if (len <= 0)
		return res;
	

	int errorcode = m_GlobalConfig.Open(path, CONFIG_OPEN_ALWAYS);
	if (errorcode != CONFIG_SUCCESS) {
		AFErrorBox(NULL, "Failed to open global.ini: %d", errorcode);
		return res;
	}

	if (!m_strStartingCollection.empty()) 
	{
		std::string path = _GetSceneCollectionFileFromName(m_strStartingCollection.c_str());
		if (!path.empty())
		{
			config_set_string(m_GlobalConfig, "Basic", "SceneCollection",
							  m_strStartingCollection.c_str());
			config_set_string(m_GlobalConfig, "Basic",
							  "SceneCollectionFile", path.c_str());

			changed = true;
		}
	}

	if (!m_strStartingProfile.empty())
	{
		std::string path = _GetProfileDirFromName(m_strStartingProfile.c_str());
		if (!path.empty()) 
		{
			config_set_string(m_GlobalConfig, "Basic", "Profile",
							  m_strStartingProfile.c_str());
			config_set_string(m_GlobalConfig, "Basic", "ProfileDir",
							  path.c_str());

			changed = true;
		}
	}

	uint32_t lastVersion =
		config_get_int(m_GlobalConfig, "General", "LastVersion");

	if (!config_has_user_value(m_GlobalConfig, "General", "Pre19Defaults")) {
		bool useOldDefaults = lastVersion &&
								lastVersion <
								MAKE_SEMANTIC_VERSION(19, 0, 0);

		config_set_bool(m_GlobalConfig, "General", "Pre19Defaults",
						useOldDefaults);


		changed = true;
	}

	if (!config_has_user_value(m_GlobalConfig, "General", "Pre21Defaults")) {
		bool useOldDefaults = lastVersion &&
								lastVersion <
								MAKE_SEMANTIC_VERSION(21, 0, 0);

		config_set_bool(m_GlobalConfig, "General", "Pre21Defaults",
						useOldDefaults);


		changed = true;
	}

	if (!config_has_user_value(m_GlobalConfig, "General", "Pre23Defaults")) {
		bool useOldDefaults = lastVersion &&
								lastVersion <
								MAKE_SEMANTIC_VERSION(23, 0, 0);

		config_set_bool(m_GlobalConfig, "General", "Pre23Defaults",
						useOldDefaults);


		changed = true;
	}

	const char* uuid = config_get_string(m_GlobalConfig, "General", "ModuleUUID");
	if (!uuid)
	{
		m_strModuleUUID = QUuid::createUuid().toString().toStdString();
		config_set_string(m_GlobalConfig, "General", "ModuleUUID",
			m_strModuleUUID.c_str());
	}
	else
		m_strModuleUUID = uuid;

#define PRE_24_1_DEFS "Pre24.1Defaults"
	if (!config_has_user_value(m_GlobalConfig, "General", PRE_24_1_DEFS)) {
		bool useOldDefaults = lastVersion &&
								lastVersion <
								MAKE_SEMANTIC_VERSION(24, 1, 0);

		config_set_bool(m_GlobalConfig, "General", PRE_24_1_DEFS,
						useOldDefaults);


		changed = true;
	}
#undef PRE_24_1_DEFS


	if (lastVersion && lastVersion < MAKE_SEMANTIC_VERSION(24, 0, 0)) {
		bool disableHotkeysInFocus = config_get_bool(
			m_GlobalConfig, "General", "DisableHotkeysInFocus");
		if (disableHotkeysInFocus)
			config_set_string(m_GlobalConfig, "General",
							  "HotkeyFocusType",
							  "DisableHotkeysInFocus");


		changed = true;
	}

	if (changed)
		config_save_safe(m_GlobalConfig, "tmp", nullptr);


	_SetDefaultValuesGlobalConfig();


	res = true;


	return res;
}

bool AFConfigManager::_InitBasicConfig(uint32_t cntAssocScreen,
									   uint32_t cxPrimaryScreen, uint32_t cyPrimaryScreen,
									   float devicePixelRatio)
{
	ProfileScope("AFConfigManager::_InitBasicConfig");

	char configPath[512] = {0,};
	int ret = GetProfilePath(configPath, sizeof(configPath), "");
	if (ret <= 0) {
		AFErrorBox(nullptr, "Failed to get profile path");
		return false;
	}

	if (os_mkdir(configPath) == MKDIR_ERROR) {
		AFErrorBox(nullptr, "Failed to create profile path");
		return false;
	}

	ret = GetProfilePath(configPath, sizeof(configPath), "basic.ini");
	if (ret <= 0) {
		AFErrorBox(nullptr, "Failed to get basic.ini path");
		return false;
	}

	int code = m_BasicConfig.Open(configPath, CONFIG_OPEN_ALWAYS);
	if (code != CONFIG_SUCCESS) {
		AFErrorBox(NULL, "Failed to open basic.ini: %d", code);
		return false;
	}

	if (config_get_string(m_BasicConfig, "General", "Name") == nullptr) {
		const char* curName = config_get_string(m_GlobalConfig,
												"Basic", "Profile");

		config_set_string(m_BasicConfig, "General", "Name", curName);
		m_BasicConfig.SaveSafe("tmp");
	}

	return SetDefaultValuesBasicConfig(cntAssocScreen, cxPrimaryScreen, 
									   cyPrimaryScreen, devicePixelRatio);
}

void AFConfigManager::_move_basic_to_profiles(void)
{
	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();
	//
	char path[512] = {0,};
	char new_path[512] = {0,};
	os_glob_t* glob = NULL;

	/* if not first time use */
	if(GetConfigPath(path, 512, "SOOPStudio/basic") <= 0)
		return;
	if(!os_file_exists(path))
		return;

	/* if the profiles directory doesn't already exist */
	if(GetConfigPath(new_path, 512, "SOOPStudio/basic/profiles") <= 0)
		return;
	if(os_file_exists(new_path))
		return;

	if(os_mkdir(new_path) == MKDIR_ERROR)
		return;

	strcat(new_path, "/");
	strcat(new_path, localeTextManager.Str("Untitled"));
	if(os_mkdir(new_path) == MKDIR_ERROR)
		return;

	strcat(path, "/*.*");
	if(os_glob(path, 0, &glob) != 0)
		return;

	strcpy(path, new_path);

	for(size_t i = 0; i < glob->gl_pathc; i++) {
		struct os_globent ent = glob->gl_pathv[i];
		char* file;

		if(ent.directory)
			continue;

		file = strrchr(ent.path, '/');
		if(!file++)
			continue;

		if(astrcmpi(file, "scenes.json") == 0)
			continue;

		strcpy(new_path, path);
		strcat(new_path, "/");
		strcat(new_path, file);
		os_rename(ent.path, new_path);
	}

	os_globfree(glob);
}
void AFConfigManager::_move_basic_to_scene_collections(void)
{
	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();
	//
	char path[512] = {0,};
	char new_path[512] = {0,};
	if(GetConfigPath(path, 512, "SOOPStudio/basic") <= 0)
		return;
	if(!os_file_exists(path))
		return;

	if(GetConfigPath(new_path, 512, "SOOPStudio/basic/scenes") <= 0)
		return;
	if(os_file_exists(new_path))
		return;

	if(os_mkdir(new_path) == MKDIR_ERROR)
		return;

	strcat(path, "/scenes.json");
	strcat(new_path, "/");
	strcat(new_path, localeTextManager.Str("Untitled"));
	strcat(new_path, ".json");

	os_rename(path, new_path);
}
