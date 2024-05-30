
#include "CService.h"
#include <util/profiler.hpp>

#include "Common/SettingsMiscDef.h"
#include "CoreModel/Config/CConfigManager.h"

AFServiceManager::~AFServiceManager()
{
    service = nullptr;
}
//
bool AFServiceManager::InitService()
{
    ProfileScope("OBSBasic::InitService");

    if(LoadService())
        return true;

    service = obs_service_create("rtmp_common", "default_service", nullptr, nullptr);
    if(!service)
        return false;
    obs_service_release(service);

    return true;
}
void AFServiceManager::SaveService()
{
    if(!service)
        return;

    auto& confManager = AFConfigManager::GetSingletonInstance();
    //
    char serviceJsonPath[512] = {0,};
    int ret = confManager.GetProfilePath(serviceJsonPath, sizeof(serviceJsonPath), SERVICE_PATH);
    if(ret <= 0)
        return;

    OBSDataAutoRelease data = obs_data_create();
    OBSDataAutoRelease settings = obs_service_get_settings(service);

    obs_data_set_string(data, "type", obs_service_get_type(service));
    obs_data_set_obj(data, "settings", settings);

    if(!obs_data_save_json_safe(data, serviceJsonPath, "tmp", "bak"))
        blog(LOG_WARNING, "Failed to save service");
}
bool AFServiceManager::LoadService()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    //
    const char* type = nullptr;
    char serviceJsonPath[512] = {0,};
    int ret = confManager.GetProfilePath(serviceJsonPath, sizeof(serviceJsonPath), SERVICE_PATH);
    if(ret <= 0)
        return false;

    OBSDataAutoRelease data = obs_data_create_from_json_file_safe(serviceJsonPath, "bak");
    if(!data)
        return false;

    obs_data_set_default_string(data, "type", "rtmp_common");
    type = obs_data_get_string(data, "type");

    OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");
    OBSDataAutoRelease hotkey_data = obs_data_get_obj(data, "hotkeys");

    service = obs_service_create(type, "default_service", settings, hotkey_data);
    obs_service_release(service);
    if(!service)
        return false;

    /* Enforce Opus on FTL if needed */
    if(strcmp(obs_service_get_protocol(service), "FTL") == 0 ||
        strcmp(obs_service_get_protocol(service), "WHIP") == 0) {
        const char* option = config_get_string(confManager.GetBasic(), "SimpleOutput", "StreamAudioEncoder");
        if(strcmp(option, "opus") != 0)
            config_set_string(confManager.GetBasic(), "SimpleOutput", "StreamAudioEncoder", "opus");

        option = config_get_string(confManager.GetBasic(), "AdvOut", "AudioEncoder");

        const char* encoder_codec = obs_get_encoder_codec(option);
        if(!encoder_codec || strcmp(encoder_codec, "opus") != 0)
            config_set_string(confManager.GetBasic(), "AdvOut", "AudioEncoder", "ffmpeg_opus");
    }

    return true;
}
obs_service_t* AFServiceManager::GetService()
{
    if(!service) {
        service = obs_service_create("rtmp_common", NULL, NULL, nullptr);
        obs_service_release(service);
    }
    return service;
}
void AFServiceManager::SetService(obs_service_t* newService)
{
    if(newService) {
        service = newService;
    }
}