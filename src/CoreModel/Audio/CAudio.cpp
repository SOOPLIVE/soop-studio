
#include "CAudio.h"

#include <util/profiler.hpp>

#include "Common/StringMiscUtils.h"

#include "Application/CApplication.h"

#include "CoreModel/Log/CLogManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

namespace AFAudioUtil {
    void CreateFirstRunSources()
    {
        bool hasDesktopAudio = HasAudioDevices(App()->OutputAudioSource());
        bool hasInputAudio = HasAudioDevices(App()->InputAudioSource());

        if(hasDesktopAudio)
            ResetAudioDevice(App()->OutputAudioSource(), "default", Str("Basic.DesktopDevice1"), 1);
        if(hasInputAudio)
            ResetAudioDevice(App()->InputAudioSource(), "default", Str("Basic.AuxDevice1"), 3);
    }

    int ResetAudio()
    {
        ProfileScope("ResetAudio");

        struct obs_audio_info2 ai = {};
        ai.samples_per_sec = config_get_uint(ACTIVECONFIG, "Audio", "SampleRate");

        const char* channelSetupStr = config_get_string(ACTIVECONFIG, "Audio", "ChannelSetup");
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

        bool lowLatencyAudioBuffering = config_get_bool(ACTIVECONFIG, "Audio", "LowLatencyAudioBuffering");
        if(lowLatencyAudioBuffering) {
            ai.max_buffering_ms = 20;
            ai.fixed_buffering = true;
        }

        return obs_reset_audio2(&ai);
    }
    void ResetAudioDevice(const char* sourceId, const char* deviceId, const char* deviceDesc, int channel)
    {
        if(!sourceId || !deviceId || !deviceDesc || !channel)
            return;

        bool disable = deviceId && strcmp(deviceId, "disabled") == 0;
        OBSSourceAutoRelease source;
        OBSDataAutoRelease settings;

        source = obs_get_output_source(channel);
        if(source) {
            if(disable) {
                obs_set_output_source(channel, nullptr);
            } else {
                settings = obs_source_get_settings(source);
                const char* oldId = obs_data_get_string(settings, "device_id");
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
    void LoadAudioMonitoring()
    {
        if(obs_audio_monitoring_available()) {
            config_t* activeConfig = ACTIVECONFIG;
            //
            const char* device_name = config_get_string(activeConfig, "Audio", "MonitoringDeviceName");
            const char* device_id = config_get_string(activeConfig, "Audio", "MonitoringDeviceId");

            obs_set_audio_monitoring_device(device_name, device_id);

            blog(LOG_INFO, "Audio monitoring device:\n\tname: %s\n\tid: %s", device_name, device_id);
        }
    }
    //
    void LogFilter(obs_source_t*, obs_source_t* filter, void* v_val)
    {
        const char* name = obs_source_get_name(filter);
        const char* id = obs_source_get_id(filter);
        int val = (int)(intptr_t)v_val;
        std::string indent;
        for(int i = 0; i < val; i++) {
            indent += "    ";
        }
        blog(LOG_INFO, "%s- filter: '%s' (%s)", indent.c_str(), name, id);
    };

    void LoadAudioDevice(const char* name, int channel, obs_data_t* parent)
    {
        OBSDataAutoRelease data = obs_data_get_obj(parent, name);
        if(!data) {
            return;
        }

        OBSSourceAutoRelease source = obs_load_source(data);
        if(!source) {
            return;
        }

        obs_set_output_source(channel, source);

        const char* source_name = obs_source_get_name(source);
        blog(LOG_INFO, "[Loaded global audio device]: '%s'", source_name);
        obs_source_enum_filters(source, LogFilter, (void*)(intptr_t)1);
        obs_monitoring_type monitoring_type =
            obs_source_get_monitoring_type(source);
        if(monitoring_type != OBS_MONITORING_TYPE_NONE) {
            const char* type =
                (monitoring_type == OBS_MONITORING_TYPE_MONITOR_ONLY)
                ? "monitor only"
                : "monitor and output";

            blog(LOG_INFO, "    - monitoring: %s", type);
        }
    };
    inline bool HasAudioDevices(const char* source_id)
    {
        const char* output_id = source_id;
        obs_properties_t* props = obs_get_source_properties(output_id);
        if(!props) {
            return false;
        }

        size_t count = 0;
        obs_property_t* devices = obs_properties_get(props, "device_id");
        if(devices) {
            count = obs_property_list_item_count(devices);
        }
        obs_properties_destroy(props);

        return count != 0;
    };
};

char* get_new_source_name(const char* name, const char* format)
{
    struct dstr new_name = {0};
    int inc = 0;

    dstr_copy(&new_name, name);

    for(;;) {
        OBSSourceAutoRelease existing_source =
            obs_get_source_by_name(new_name.array);
        if(!existing_source)
            break;

        dstr_printf(&new_name, format, name, ++inc + 1);
    }

    return new_name.array;
}