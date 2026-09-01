#include "CLoadSaveManager.h"

#include <util/profiler.hpp>

#include "Common/SettingsMiscDef.h"
#include "Common/StudioDefine.h"

#include "Application/CApplication.h"

#include "CoreModel/Audio/CAudio.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"
#include "CoreModel/Profile/CProfile.h"
#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"
#include "Utils/importers/importers.hpp"

#include "MainFrame/Profile/CMainProfile.h"
#include "MainFrame/SceneCollection/CMainSceneCollection.h"
#include "MainFrame/AudioSource/CAudioSource.h"
#include "MainFrame/SceneSource/CMainSceneSource.h"


bool AFLoadSaveManager::InitLoadSave()
{
	bool firstOpen = false;

	const char* sceneCollectionFile = config_get_string(USERCONFIG, "Basic", "SceneCollectionFile");
	char savePath[1024];
	char fileName[1024];
	int ret;

	if (!sceneCollectionFile)
		throw "Failed to get scene collection name";

    ret = snprintf(fileName, sizeof(fileName), (LOCAL_FOLDER_NAME + "/basic/scenes/%s").c_str(), sceneCollectionFile);

	if (ret <= 0)
		throw "Failed to create scene collection file name";

	ret = GetAppConfigPath(savePath, sizeof(savePath), fileName);
	if (ret <= 0)
		throw "Failed to get scene collection json file path";
	//

	ProfileScope("AFLoadSaveManager::Load");

    const char* sceneCollectionRaw = config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection");
    const std::string sceneCollectionName{ sceneCollectionRaw ? sceneCollectionRaw : "" };        

    const std::optional<OBSSceneCollection> configuredCollection =
        MAIN_SCENECOLLECTION->GetSceneCollectionByName(sceneCollectionName);

    if (configuredCollection) {
        MAIN_SCENECOLLECTION->ActivateSceneCollection(configuredCollection.value());
    }
    else {
        DecreaseCheckSaveCnt();
        MAIN_SCENECOLLECTION->SetupNewSceneCollection(sceneCollectionName);
        IncreaseCheckSaveCnt();
        config_save_safe(USERCONFIG, "tmp", nullptr);
        firstOpen = true;
    }

    if (configuredCollection) {
        DecreaseCheckSaveCnt();
        MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
        MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
        MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
        MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
        IncreaseCheckSaveCnt();
    }

    MAIN_SCENECOLLECTION->RefreshSceneCollections();
    MAIN_PROFILE->RefreshProfiles();
	DecreaseCheckSaveCnt();

    return firstOpen;
}

bool AFLoadSaveManager::Load(const char* file, bool remigrate)
{
    IncreaseCheckSaveCnt();

    obs_data_t *data = obs_data_create_from_json_file_safe(file, "bak");
    if (!data) {
        DecreaseCheckSaveCnt();
        blog(LOG_INFO, "No scene file found, creating default scene");
        

        MAIN_SCENESOURCE->CreateDefaultScene(true);
        MAINFRAME->qslotSaveProject();
        MAINFRAME->RefreshSceneUI();
        //
        return true;
    }

    sceneCollectionBackup(file, data);

    _LoadData(data, file);
    return false;
}

bool AFLoadSaveManager::CheckCanSaveProject()
{
    if(m_disableSaving.load())
        return false;

    m_projectChanged = true;

    return true;
};

void AFLoadSaveManager::ForceSaveProjectNow()
{
    long prevDisableVal = m_disableSaving.load();
    
    m_disableSaving.store(0);
    
    SaveProjectNow();
    
    m_disableSaving.store(prevDisableVal);
}

void AFLoadSaveManager::SaveProjectNow()
{
    if (CheckCanSaveProject() == true)
        SaveProjectDeferred();
}

void AFLoadSaveManager::SaveProjectDeferred()
{
    if (m_disableSaving.load())
        return;

    if (!m_projectChanged)
        return;

    m_projectChanged = false;
        

    try {
        const OBSSceneCollection& currentCollection = MAIN_SCENECOLLECTION->GetCurrentSceneCollection();

        _Save(currentCollection.collectionFile.u8string().c_str());
    }
    catch (const std::invalid_argument& error) {
        blog(LOG_ERROR, "%s", error.what());
    }

}

//Need Change if backup has same name
void AFLoadSaveManager::MoveProfileToBackup(std::string remainID)
{
    _CheckBackupDir(remainID);

    char sceneDir[1024];
    int ret;

    std::string profilePath = LOCAL_FOLDER_NAME + "/backup/" + remainID + "/profiles/";

    //Backup Folder/account/scene Check
    char backupDir[1024];
    ret = GetAppConfigPath(backupDir, sizeof(backupDir), profilePath.c_str());
    if (!std::filesystem::exists(backupDir))
        std::filesystem::create_directory(backupDir);

    //Move SceneCollection
    ret = GetAppConfigPath(sceneDir, sizeof(sceneDir), (LOCAL_FOLDER_NAME + "/basic/profiles").c_str());
    std::string strBackupDir(backupDir);

    for (const auto& entry : std::filesystem::directory_iterator(sceneDir))
    {
        const std::filesystem::path filePath(strBackupDir + "/" + entry.path().filename().string());
        std::filesystem::rename(entry.path(), filePath);
    }
}

void AFLoadSaveManager::MoveSceneCollectionToBackup(std::string remainID)
{
    _CheckBackupDir(remainID);

    char sceneDir[1024];
    int ret;

    std::string scenePath = LOCAL_FOLDER_NAME + "/backup/" + remainID + "/scenes/";

    //Backup Folder/account/scene Check
    char backupDir[1024];
    ret = GetAppConfigPath(backupDir, sizeof(backupDir), scenePath.c_str());

    
    if (!std::filesystem::exists(backupDir))
        std::filesystem::create_directory(backupDir);

    //Move SceneCollection
    ret = GetAppConfigPath(sceneDir, sizeof(sceneDir), (LOCAL_FOLDER_NAME + "/basic/scenes").c_str());
    std::string strBackupDir(backupDir);

    for (const auto& entry : std::filesystem::directory_iterator(sceneDir))
    {
        if (std::filesystem::is_regular_file(entry.path()))
        {
            const std::filesystem::path filePath(strBackupDir + "/" + entry.path().filename().string());

            if (std::filesystem::exists(filePath))
            {
                std::filesystem::remove(filePath);
            }

            std::filesystem::rename(entry.path(), filePath);
        }
    }

    //Set SceneCollection to default
    config_t* userConfig = USERCONFIG;
    config_set_string(userConfig, "Basic", "SceneCollection", Str("Untitled"));
    config_set_string(userConfig, "Basic", "SceneCollectionFile", Str("Untitled"));
}

void AFLoadSaveManager::_LoadTransitions(obs_data_array_t *transitions, obs_load_source_cb cb, void *private_data)
{
    size_t count = obs_data_array_count(transitions);

    for (size_t i = 0; i < count; i++) {
        OBSDataAutoRelease item = obs_data_array_item(transitions, i);
        const char *name = obs_data_get_string(item, "name");
        const char *id = obs_data_get_string(item, "id");
        OBSDataAutoRelease settings =
            obs_data_get_obj(item, "settings");

        OBSSourceAutoRelease source =
            obs_source_create_private(id, name, settings);
        if (!obs_obj_invalid(source)) {
            SCENE_CONTEXT.InitTransition(source);
            SCENE_CONTEXT.AddTransition(source.Get());

            if (cb)
                cb(private_data, source);
        }
    }
}

void AFLoadSaveManager::_LoadSceneListOrder(obs_data_array_t *array)
{
    SceneItemVector& sceneItems = SCENE_CONTEXT.GetSceneItemVector();
    
    size_t num = obs_data_array_count(array);

    for (size_t i = 0; i < num; i++) {
        OBSDataAutoRelease data = obs_data_array_item(array, i);
        const char *name = obs_data_get_string(data, "name");

        { //ReorderItemByName(ui->scenes, name, (int)i)
            for (int indx = 0; indx < sceneItems.size(); indx++)
            {
                AFQSceneListItem* item = sceneItems[indx];
                if (strcmp(name, item->GetSceneName()) == 0)
                {
                    if (i != indx)
                    {
                        item = sceneItems[indx];
                        sceneItems.erase(sceneItems.begin() + indx);
                        sceneItems.insert(sceneItems.begin() + i, item);
                    }
                    
                    break;
                }
            }
        } //
    }
}

void AFLoadSaveManager::_LoadData(obs_data_t* data, const char* file)
{
    MAINFRAME->ClearSceneData(true);
    //ClearContextBar();

    /* Exit OBS if clearing scene data failed for some reason. */
//    if (clearingFailed) {
//        OBSMessageBox::critical(this, QTStr("SourceLeak.Title"),
//                    QTStr("SourceLeak.Text"));
//        close();
//        return;
//    }

    auto& sceneContext = SCENE_CONTEXT;
    sceneContext.InitDefaultTransition();

    MAIN_PROFILE->WaitDevicePropertiesThread();

//    OBSDataAutoRelease modulesObj = obs_data_get_obj(data, "modules");
//    if (api)
//        api->on_preload(modulesObj);
//
//    if (safe_mode || disable_3p_plugins) {
//        /* Keep a reference to "modules" data so plugins that are not
//         * loaded do not have their collection specific data lost. */
//        safeModeModuleData = obs_data_get_obj(data, "modules");
//    }

    OBSDataArrayAutoRelease sceneOrder = obs_data_get_array(data, "scene_order");
    OBSDataArrayAutoRelease sources = obs_data_get_array(data, "sources");
    OBSDataArrayAutoRelease groups = obs_data_get_array(data, "groups");
    OBSDataArrayAutoRelease transitions = obs_data_get_array(data, "transitions");
    const char *sceneName = obs_data_get_string(data, "current_scene");
    const char *programSceneName = obs_data_get_string(data, "current_program_scene");
    const char *transitionName = obs_data_get_string(data, "current_transition");

    auto& optStartingScene = ARGOPTION.startingScene();
    if (optStartingScene.empty() == false)
    {
        programSceneName = optStartingScene.c_str();
        if (STATEAPP.IsPreviewProgramMode() == false)
            sceneName = optStartingScene.c_str();
    }

    int newDuration = (int)obs_data_get_int(data, "transition_duration");
    if (!newDuration)
        newDuration = 300;

    if (!transitionName)
        transitionName = obs_source_get_name(sceneContext.GetFadeTransition());
    
    const char *curSceneCollection = config_get_string(USERCONFIG, "Basic", "SceneCollection");
    obs_data_set_default_string(data, "name", curSceneCollection);

    const char *name = obs_data_get_string(data, "name");
    OBSSourceAutoRelease curScene;
    OBSSourceAutoRelease curProgramScene;
    obs_source_t *curTransition;

    if (!name || !*name)
        name = curSceneCollection;

    AFAudioUtil::LoadAudioDevice(DESKTOP_AUDIO_1, 1, data);
    AFAudioUtil::LoadAudioDevice(DESKTOP_AUDIO_2, 2, data);
    AFAudioUtil::LoadAudioDevice(AUX_AUDIO_1, 3, data);
    AFAudioUtil::LoadAudioDevice(AUX_AUDIO_2, 4, data);
    AFAudioUtil::LoadAudioDevice(AUX_AUDIO_3, 5, data);
    AFAudioUtil::LoadAudioDevice(AUX_AUDIO_4, 6, data);

    if (!sources)
        sources = std::move(groups);
    else
        obs_data_array_push_back_array(sources, groups);
    

    obs_missing_files_t *files = obs_missing_files_create();
    obs_load_sources(sources, AFProfileUtil::AddMissingFiles, files);

    if (transitions)
        _LoadTransitions(transitions, AFProfileUtil::AddMissingFiles, files);
    if (sceneOrder)
        _LoadSceneListOrder(sceneOrder);

    curTransition = sceneContext.FindTransition(transitionName);
    if (!curTransition)
        curTransition = sceneContext.GetFadeTransition();
 
    sceneContext.SetCurTransition(curTransition);
    sceneContext.SetCurDuration(newDuration);
    sceneContext.SetTransition(curTransition);

retryScene:
    curScene = obs_get_source_by_name(sceneName);
    curProgramScene = obs_get_source_by_name(programSceneName);

    /* if the starting scene command line parameter is bad at all,
     * fall back to original settings */
    if (optStartingScene.empty() == false &&
        (!curScene || !curProgramScene))
    {
        sceneName = obs_data_get_string(data, "current_scene");
        programSceneName = obs_data_get_string(data, "current_program_scene");
        
        optStartingScene.clear();
        ARGOPTION.startingScene("");
        goto retryScene;
    }

    /* if current_scene& current_program_scene is not matched */ 
    if (!curScene || !curProgramScene)
    {
        obs_frontend_source_list scenes = {};
        obs_frontend_get_scenes(&scenes);

        if (scenes.sources.num > 0 && scenes.sources.array[0]) {
            obs_source_t* firstScene = scenes.sources.array[0];
            const char* firstName = obs_source_get_name(firstScene);

            sceneName = firstName;
            programSceneName = firstName;

            obs_frontend_source_list_free(&scenes);
            goto retryScene;
        }

        obs_frontend_source_list_free(&scenes);
    }

    DYNAMIC_COMPOSIT->SetCurrentScene(curScene.Get(), true);    
    MAINFRAME->RefreshSceneUI();
    //

    if (!curProgramScene)
        curProgramScene = std::move(curScene);
    if (STATEAPP.IsPreviewProgramMode())
        sceneContext.TransitionToScene(curProgramScene.Get(), true);


    // Attach SOOP Media Source
    OBSScene changeScene = obs_scene_from_source(curScene.Get());
    obs_scene_enum_items(changeScene, [](obs_scene_t* scene, obs_sceneitem_t* item, void* param) {
        obs_source_t* source = obs_sceneitem_get_source(item);
        if (source) {
            if (AFSourceUtil::IsSoopMediaSource(source)) {
                SOOP_SRC_MANAGER.SetSoopMediaSource(source);
                return false;
            }
        }
        return true;
        }, NULL);

    /* ------------------- */

//    bool projectorSave = config_get_bool(USERCONFIG, "BasicWindow", "SaveProjectors");

//    if (projectorSave) {
//        OBSDataArrayAutoRelease savedProjectors =
//            obs_data_get_array(data, "saved_projectors");
//
//        if (savedProjectors) {
//            LoadSavedProjectors(savedProjectors);
//            OpenSavedProjectors();
//            activateWindow();
//        }
//    }

    /* ------------------- */

    std::string file_base = strrchr(file, '/') + 1;
    
    config_t* userConfig = USERCONFIG;
    config_set_string(userConfig, "Basic", "SceneCollection", name);
    config_set_string(userConfig, "Basic", "SceneCollectionFile", file_base.c_str());

    OBSDataArrayAutoRelease quickTransitionData = obs_data_get_array(data, "quick_transitions");
    //LoadQuickTransitions(quickTransitionData);

    //RefreshQuickTransitions();

//    bool previewLocked = obs_data_get_bool(data, "preview_locked");
//    ui->preview->SetLocked(previewLocked);
//    ui->actionLockPreview->setChecked(previewLocked);

    /* ---------------------- */

//    bool fixedScaling = obs_data_get_bool(data, "scaling_enabled");
//    int scalingLevel = (int)obs_data_get_int(data, "scaling_level");
//    float scrollOffX = (float)obs_data_get_double(data, "scaling_off_x");
//    float scrollOffY = (float)obs_data_get_double(data, "scaling_off_y");
//
//    if (fixedScaling) {
//        ui->preview->SetScalingLevel(scalingLevel);
//        ui->preview->SetScrollingOffset(scrollOffX, scrollOffY);
//    }
    //ui->preview->SetFixedScaling(fixedScaling);
    //emit ui->preview->DisplayResized();

    bool vcamEnabled = MAINFRAME->VirtualCamEnabled();
    VCamConfig& config = MAINFRAME->VirtualCamConfig();
    if (vcamEnabled) {
        OBSDataAutoRelease obj = obs_data_get_obj(data, "virtual-camera");
        config.type = (VCamOutputType)obs_data_get_int(obj, "type2");
        if (config.type == VCamOutputType::Invalid)
            config.type = (VCamOutputType)obs_data_get_int(obj, "type");
        //
        if (config.type == VCamOutputType::Invalid) {
            VCamInternalType internal = (VCamInternalType)obs_data_get_int(obj, "internal");

            switch (internal) {
            case VCamInternalType::Default:
                config.type = VCamOutputType::ProgramView;
                break;
            case VCamInternalType::Preview:
                config.type = VCamOutputType::PreviewOutput;
                break;
            }
        }
        config.scene = obs_data_get_string(obj, "scene");
        config.source = obs_data_get_string(obj, "source");
    }

    /* ---------------------- */

//    if (api)
//        api->on_load(modulesObj);

    obs_data_release(data);

//    if (!opt_starting_scene.empty())
//        opt_starting_scene.clear();
//
//    if (opt_start_streaming && !safe_mode) {
//        blog(LOG_INFO, "Starting stream due to command line parameter");
//        QMetaObject::invokeMethod(this, "StartStreaming",
//                      Qt::QueuedConnection);
//        opt_start_streaming = false;
//    }
//
//    if (opt_start_recording && !safe_mode) {
//        blog(LOG_INFO,
//             "Starting recording due to command line parameter");
//        QMetaObject::invokeMethod(this, "StartRecording",
//                      Qt::QueuedConnection);
//        opt_start_recording = false;
//    }
//
//    if (opt_start_replaybuffer && !safe_mode) {
//        QMetaObject::invokeMethod(this, "StartReplayBuffer",
//                      Qt::QueuedConnection);
//        opt_start_replaybuffer = false;
//    }
//
//    if (opt_start_virtualcam && !safe_mode) {
//        QMetaObject::invokeMethod(this, "StartVirtualCam",
//                      Qt::QueuedConnection);
//        opt_start_virtualcam = false;
//    }

    //LogScenes();

    if (ARGOPTION.GetDisableMissingFilesCheck() == false)
        MAINFRAME->ShowMissingFilesDialog(files);

    DecreaseCheckSaveCnt();

    if (vcamEnabled) {
        MAINFRAME->UpdateVirtualCamConfig(config);
    }
//
    MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
    MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
}

void AFLoadSaveManager::_SaveAudioDevice(const char *name, int channel, obs_data_t *parent,
                                         std::vector<OBSSource> &audioSources)
{
    OBSSourceAutoRelease source = obs_get_output_source(channel);
    if (!source)
        return;

    audioSources.push_back(source.Get());

    OBSDataAutoRelease data = obs_save_source(source);

    obs_data_set_obj(parent, name, data);
}

obs_data_t* AFLoadSaveManager::_GenerateSaveData(obs_data_array_t* sceneOrder,
                                                 obs_data_array_t* quickTransitionData,
                                                 int transitionDuration,
                                                 obs_data_array_t* transitions,
                                                 OBSScene& scene, OBSSource& curProgramScene,
                                                 obs_data_array_t* savedProjectorList)
{
    obs_data_t *saveData = obs_data_create();
    obs_data_set_string(saveData, "importer_type", STR_IMPORTER_TYPE_SOOP);

    std::vector<OBSSource> audioSources;
    audioSources.reserve(6);

    _SaveAudioDevice(DESKTOP_AUDIO_1, 1, saveData, audioSources);
    _SaveAudioDevice(DESKTOP_AUDIO_2, 2, saveData, audioSources);
    _SaveAudioDevice(AUX_AUDIO_1, 3, saveData, audioSources);
    _SaveAudioDevice(AUX_AUDIO_2, 4, saveData, audioSources);
    _SaveAudioDevice(AUX_AUDIO_3, 5, saveData, audioSources);
    _SaveAudioDevice(AUX_AUDIO_4, 6, saveData, audioSources);

    /* -------------------------------- */
    /* save non-group sources           */

    auto FilterAudioSources = [&](obs_source_t *source) {
        if (obs_source_is_group(source))
            return false;

        return find(begin(audioSources), end(audioSources), source) ==
               end(audioSources);
    };
    using FilterAudioSources_t = decltype(FilterAudioSources);

    obs_data_array_t *sourcesArray = obs_save_sources_filtered(
        [](void *data, obs_source_t *source) {
            auto &func = *static_cast<FilterAudioSources_t *>(data);
            return func(source);
        },
        static_cast<void *>(&FilterAudioSources));

    /* reset soop media source */
    const size_t count = obs_data_array_count(sourcesArray);
    for (size_t i = 0; i < count; i++) {
        OBSDataAutoRelease sourceData = obs_data_array_item(sourcesArray, i);     
        const char* id = obs_data_get_string(sourceData, "id");
        if (id && AFSourceUtil::IsSoopMediaSource(id)) {
            obs_data_set_string(sourceData, "name", obs_source_get_display_name(id));

            OBSDataAutoRelease settings = obs_data_get_obj(sourceData, "settings");
            obs_data_set_string(settings, "input", "");
            obs_data_set_int(settings, "cpNo", 0);
            obs_data_set_int(settings, "idx", 0);
        }
    }

    /* -------------------------------- */
    /* save group sources separately    */

    /* saving separately ensures they won't be loaded in older versions */
    obs_data_array_t *groupsArray = obs_save_sources_filtered(
        [](void *, obs_source_t *source) {
            return obs_source_is_group(source);
        },
        nullptr);

    /* -------------------------------- */

    OBSSourceAutoRelease transition = obs_get_output_source(0);
    obs_source_t* currentScene = obs_scene_get_source(scene);
    const char* sceneName = obs_source_get_name(currentScene);
    const char* programName = obs_source_get_name(curProgramScene);

    const char* sceneCollection = config_get_string(USERCONFIG, "Basic", "SceneCollection");

    obs_data_set_string(saveData, "current_scene", sceneName);
    obs_data_set_string(saveData, "current_program_scene", programName);
    obs_data_set_array(saveData, "scene_order", sceneOrder);
    obs_data_set_string(saveData, "name", sceneCollection);
    obs_data_set_array(saveData, "sources", sourcesArray);
    obs_data_set_array(saveData, "groups", groupsArray);
    obs_data_set_array(saveData, "quick_transitions", quickTransitionData);
    obs_data_set_array(saveData, "transitions", transitions);
    obs_data_set_array(saveData, "saved_projectors", savedProjectorList);
    obs_data_array_release(sourcesArray);
    obs_data_array_release(groupsArray);

    obs_data_set_string(saveData, "current_transition", obs_source_get_name(transition));
    obs_data_set_int(saveData, "transition_duration", transitionDuration);

    return saveData;
}

obs_data_array_t* AFLoadSaveManager::_SaveSceneListOrder()
{
    obs_data_array_t *sceneOrder = obs_data_array_create();

    SceneItemVector& sceneItems = SCENE_CONTEXT.GetSceneItemVector();
    
    if (sceneItems.empty() == false)
    {
        size_t sceneCount = sceneItems.size();
        for (size_t i = 0; i < sceneCount; i++)
        {
            OBSDataAutoRelease data = obs_data_create();
            obs_data_set_string(data, "name",
                                sceneItems.at(i)->GetSceneName());
            obs_data_array_push_back(sceneOrder, data);
        }
    }
    
    return sceneOrder;
}

obs_data_array_t* AFLoadSaveManager::_SaveTransitions()
{
    obs_data_array_t *transitions = obs_data_array_create();

    std::vector<OBSSource>& tmpTransitions = SCENE_CONTEXT.GetRefTransitions();
    
    for (int i = 0; i < tmpTransitions.size(); i++)
    {
        OBSSource tr = tmpTransitions[i];
        if (!tr || !obs_source_configurable(tr))
            continue;

        OBSDataAutoRelease sourceData = obs_data_create();
        OBSDataAutoRelease settings = obs_source_get_settings(tr);

        obs_data_set_string(sourceData, "name", obs_source_get_name(tr));
        obs_data_set_string(sourceData, "id", obs_obj_get_id(tr));
        obs_data_set_obj(sourceData, "settings", settings);

        obs_data_array_push_back(transitions, sourceData);
    }

    return transitions;
}

obs_data_array_t* AFLoadSaveManager::_SaveQuickTransitions()
{

    obs_data_array_t *array = obs_data_array_create();

//    for (QuickTransition &qt : quickTransitions) {
//        OBSDataAutoRelease data = obs_data_create();
//        OBSDataArrayAutoRelease hotkeys = obs_hotkey_save(qt.hotkey);
//
//        obs_data_set_string(data, "name",
//                    obs_source_get_name(qt.source));
//        obs_data_set_int(data, "duration", qt.duration);
//        obs_data_set_array(data, "hotkeys", hotkeys);
//        obs_data_set_int(data, "id", qt.id);
//        obs_data_set_bool(data, "fade_to_black", qt.fadeToBlack);
//
//        obs_data_array_push_back(array, data);
//    }

    return array;
}

void AFLoadSaveManager::_Save(const char* file)
{
    auto& sceneContext = SCENE_CONTEXT;
    //
    OBSScene scene = sceneContext.GetCurrentScene();
    OBSSource curProgramScene = sceneContext.GetProgramSource();
    if (!curProgramScene)
        curProgramScene = obs_scene_get_source(scene);

    int transDuration = sceneContext.GetCurDuraition();

    OBSDataArrayAutoRelease sceneOrder = _SaveSceneListOrder();
    OBSDataArrayAutoRelease transitions = _SaveTransitions();
    OBSDataArrayAutoRelease quickTrData = _SaveQuickTransitions();

    //OBSDataArrayAutoRelease savedProjectorList = SaveProjectors();
    OBSDataAutoRelease saveData = _GenerateSaveData(
        sceneOrder, quickTrData, transDuration,
        transitions, scene, curProgramScene, nullptr/*savedProjectorList*/);


//    obs_data_set_bool(saveData, "preview_locked", ui->preview->Locked());
//    obs_data_set_bool(saveData, "scaling_enabled",
//              ui->preview->IsFixedScaling());
//    obs_data_set_int(saveData, "scaling_level",
//             ui->preview->GetScalingLevel());
//    obs_data_set_double(saveData, "scaling_off_x",
//                ui->preview->GetScrollX());
//    obs_data_set_double(saveData, "scaling_off_y",
//                ui->preview->GetScrollY());

    if (MAINFRAME->VirtualCamEnabled()) {
        OBSDataAutoRelease obj = obs_data_create();
        VCamConfig& config = MAINFRAME->VirtualCamConfig();
        obs_data_set_int(obj, "type2", (int)config.type);
        switch (config.type) {
        case VCamOutputType::Invalid:
        case VCamOutputType::ProgramView:
        case VCamOutputType::PreviewOutput:
            break;
        case VCamOutputType::SceneOutput:
            obs_data_set_string(obj, "scene", config.scene.c_str());
            break;
        case VCamOutputType::SourceOutput:
            obs_data_set_string(obj, "source", config.source.c_str());
            break;
        }
        //
        obs_data_set_obj(saveData, "virtual-camera", obj);
    }


//    if (api) {
//        if (safeModeModuleData) {
//            /* If we're in Safe Mode and have retained unloaded
//             * plugin data, update the existing data object instead
//             * of creating a new one. */
//            api->on_save(safeModeModuleData);
//            obs_data_set_obj(saveData, "modules",
//                     safeModeModuleData);
//        } else {
//            OBSDataAutoRelease moduleObj = obs_data_create();
//            api->on_save(moduleObj);
//            obs_data_set_obj(saveData, "modules", moduleObj);
//        }
//    }
    //

    if (!obs_data_save_json_safe(saveData, file, "tmp", "bak"))
        blog(LOG_ERROR, "Could not save scene data to %s", file);
}

void AFLoadSaveManager::_CheckBackupDir(std::string remainID)
{
    char sceneDir[1024];
    int ret;

    std::string backupPath = LOCAL_FOLDER_NAME + "/backup/";
    std::string accountPath = LOCAL_FOLDER_NAME + "/backup/" + remainID;

    //Backup Folder Check
    char backupDir[1024];
    ret = GetAppConfigPath(backupDir, sizeof(backupDir), backupPath.c_str());
    if (!std::filesystem::exists(backupDir))
        std::filesystem::create_directory(backupDir);

    //Backup Folder/account Check
    ret = GetAppConfigPath(backupDir, sizeof(backupDir), accountPath.c_str());

    if (!std::filesystem::exists(backupDir))
        std::filesystem::create_directory(backupDir);
}

constexpr int maxBackupFile = 30;
void AFLoadSaveManager::sceneCollectionBackup(const char* file, obs_data_t* data)
{
    if (!file || 0 == strlen(file) || !data)
        return;

    QFileInfo fileInfo(QString::fromUtf8(file));
    if (!fileInfo.exists())
        return;

    // backup directory
    std::string backupPath = LOCAL_FOLDER_NAME + "/backup/scenes/";
    // directory check & make
    char backupDir[MAX_PATH] = { 0, };
    int ret = GetAppConfigPath(backupDir, sizeof(backupDir), backupPath.c_str());
    if (!os_file_exists(backupDir))
        os_mkdirs(backupDir);

    // backup file
    const QString backupFile = QString("%1/%2_%3.json")
        .arg(backupDir)
        .arg(fileInfo.completeBaseName())
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    // save
    obs_data_save_json(data, backupFile.toUtf8().constData());

    // check max file count
    QDir dir(backupDir);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot); // set filter : file & remove parent folder 
    dir.setSorting(QDir::Time | QDir::Reversed); // sort

    if (dir.count() > maxBackupFile) {
        int removeCount = dir.count() - maxBackupFile;
        //
        QFileInfoList list = dir.entryInfoList();
        for (auto item : list) {
            std::string removeFile = item.absoluteFilePath().toUtf8().constData();
            os_unlink(removeFile.c_str());
            if (--removeCount <= 0)
                break;
        }
    }
}