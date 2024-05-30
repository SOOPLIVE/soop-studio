
#include "CAudio.h"
#include <util/dstr.hpp>
#include <util/profiler.hpp>

#include "Common/StringMiscUtils.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Log/CLogManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "Application/CApplication.h"

AFAudioUtil::AFAudioUtil()
{
}
AFAudioUtil::~AFAudioUtil()
{
}
//
void AFAudioUtil::CreateFirstRunSources()
{
	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();
	//
	bool hasDesktopAudio = HasAudioDevices(App()->OutputAudioSource());
	bool hasInputAudio = HasAudioDevices(App()->InputAudioSource());

	if(hasDesktopAudio)
		ResetAudioDevice(App()->OutputAudioSource(), "default", localeTextManager.Str("Basic.DesktopDevice1"), 1);
	if(hasInputAudio)
		ResetAudioDevice(App()->InputAudioSource(), "default", localeTextManager.Str("Basic.AuxDevice1"), 3);
}
int AFAudioUtil::ResetAudio()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	ProfileScope("AFAudioUtil::ResetAudio");

	struct obs_audio_info2 ai = {};
	ai.samples_per_sec = config_get_uint(confManager.GetBasic(), "Audio", "SampleRate");

	const char* channelSetupStr = config_get_string(confManager.GetBasic(), "Audio", "ChannelSetup");
	if(strcmp(channelSetupStr, "Mono") == 0)
		ai.speakers = SPEAKERS_MONO;
	else if(strcmp(channelSetupStr, "2.1") == 0)
		ai.speakers = SPEAKERS_2POINT1;
	else if(strcmp(channelSetupStr, "4.0") == 0)
		ai.speakers = SPEAKERS_4POINT0;
	else if(strcmp(channelSetupStr, "4.1") == 0)
		ai.speakers = SPEAKERS_4POINT1;
	else if(strcmp(channelSetupStr, "5.1") == 0)
		ai.speakers = SPEAKERS_5POINT1;
	else if(strcmp(channelSetupStr, "7.1") == 0)
		ai.speakers = SPEAKERS_7POINT1;
	else
		ai.speakers = SPEAKERS_STEREO;

	bool lowLatencyAudioBuffering = config_get_bool(confManager.GetGlobal(), "Audio", "LowLatencyAudioBuffering");
	if(lowLatencyAudioBuffering) {
		ai.max_buffering_ms = 20;
		ai.fixed_buffering = true;
	}

	return obs_reset_audio2(&ai);
}
void AFAudioUtil::ResetAudioDevice(const char* sourceId, const char* deviceId, const char* deviceDesc, int channel)
{
	bool disable = deviceId && strcmp(deviceId, "disabled") == 0;
	OBSSourceAutoRelease source;
	OBSDataAutoRelease settings;

	source = obs_get_output_source(channel);
	if(source) {
		if(disable) {
			obs_set_output_source(channel, nullptr);
		} else {
			settings = obs_source_get_settings(source);
			const char* oldId =
				obs_data_get_string(settings, "device_id");
			if(strcmp(oldId, deviceId) != 0) {
				obs_data_set_string(settings, "device_id", deviceId);
				obs_source_update(source, settings);
			}
		}

	} else if(!disable) {
		BPtr<char> name = get_new_source_name(deviceDesc, "%s (%d)");
        //

		settings = obs_data_create();
		obs_data_set_string(settings, "device_id", deviceId);
		source = obs_source_create(sourceId, name, settings, nullptr);

		obs_set_output_source(channel, source);
	}
}
void AFAudioUtil::LoadAudioMonitoring()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	if(obs_audio_monitoring_available()) {
		const char* device_name = config_get_string(confManager.GetBasic(), "Audio", "MonitoringDeviceName");
		const char* device_id = config_get_string(confManager.GetBasic(), "Audio", "MonitoringDeviceId");

		obs_set_audio_monitoring_device(device_name, device_id);

		blog(LOG_INFO, "Audio monitoring device:\n\tname: %s\n\tid: %s", device_name, device_id);
	}
}

char* get_new_source_name(const char* name, const char* format)
{
    struct dstr new_name = { 0 };
    int inc = 0;

    dstr_copy(&new_name, name);

    for (;;) {
        OBSSourceAutoRelease existing_source =
            obs_get_source_by_name(new_name.array);
        if (!existing_source)
            break;

        dstr_printf(&new_name, format, name, ++inc + 1);
    }

    return new_name.array;
}
//
