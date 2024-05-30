#include "CLoadSaveManager.h"


#include "Common/SettingsMiscDef.h"
#include "CoreModel/Audio/CAudio.h"
#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Scene/CScene.h"
#include "CoreModel/Scene/CSceneContext.h"


#include "Application/CApplication.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"
#include "Utils/importers/importers.hpp"



void AFLoadSaveManager::EnumProfiles(std::function<bool(const char *, const char *)> &&cb)
{
    char path[512];
    os_glob_t *glob;

    int ret = AFConfigManager::GetSingletonInstance().
                GetConfigPath(path, sizeof(path),
                              "SOOPStudio/basic/profiles/*");
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get profiles config path");
        return;
    }

    if (os_glob(path, 0, &glob) != 0) {
        blog(LOG_WARNING, "Failed to glob profiles");
        return;
    }

    for (size_t i = 0; i < glob->gl_pathc; i++) {
        const char *filePath = glob->gl_pathv[i].path;
        const char *dirName = strrchr(filePath, '/') + 1;

        if (!glob->gl_pathv[i].directory)
            continue;

        if (strcmp(dirName, ".") == 0 || strcmp(dirName, "..") == 0)
            continue;

        std::string file = filePath;
        file += "/basic.ini";

        ConfigFile config;
        int ret = config.Open(file.c_str(), CONFIG_OPEN_EXISTING);
        if (ret != CONFIG_SUCCESS)
            continue;

        const char *name = config_get_string(config, "General", "Name");
        if (!name)
            name = strrchr(filePath, '/') + 1;

        if (!cb(name, filePath))
            break;
    }

    os_globfree(glob);
}

bool AFLoadSaveManager::GetProfileDir(const char* findName, const char*& profileDir)
{
    bool found = false;
    auto func = [&](const char *name, const char *path) {
        if (strcmp(name, findName) == 0) {
            found = true;
            profileDir = strrchr(path, '/') + 1;
            return false;
        }
        return true;
    };

    EnumProfiles(func);
    return found;
}

bool AFLoadSaveManager::ProfileExists(const char* findName)
{
    const char* profileDir = nullptr;
    return GetProfileDir(findName, profileDir);
}

void AFLoadSaveManager::EnumSceneCollections(std::function<bool(const char *, const char *)> &&cb)
{
    char path[512];
    os_glob_t *glob;

    int ret = AFConfigManager::GetSingletonInstance().
                GetConfigPath(path, sizeof(path),
                              "SOOPStudio/basic/scenes/*.json");
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get config path for scene "
                  "collections");
        return;
    }

    if (os_glob(path, 0, &glob) != 0) {
        blog(LOG_WARNING, "Failed to glob scene collections");
        return;
    }

    for (size_t i = 0; i < glob->gl_pathc; i++) {
        const char *filePath = glob->gl_pathv[i].path;

        if (glob->gl_pathv[i].directory)
            continue;

        OBSDataAutoRelease data =
            obs_data_create_from_json_file_safe(filePath, "bak");
        std::string name = obs_data_get_string(data, "name");

        /* if no name found, use the file name as the name
         * (this only happens when switching to the new version) */
        if (name.empty()) {
            name = strrchr(filePath, '/') + 1;
            name.resize(name.size() - 5);
        }

        if (!cb(name.c_str(), filePath))
            break;
    }

    os_globfree(glob);
}

bool AFLoadSaveManager::SceneCollectionExists(const char* findName)
{
    bool found = false;
    auto func = [&](const char *name, const char *) {
        if (strcmp(name, findName) == 0) {
            found = true;
            return false;
        }

        return true;
    };

    EnumSceneCollections(func);
    return found;
}

bool AFLoadSaveManager::Load(const char* file)
{
    IncreaseCheckSaveCnt();

    obs_data_t *data = obs_data_create_from_json_file_safe(file, "bak");
    if (!data) {
        DecreaseCheckSaveCnt();
        blog(LOG_INFO, "No scene file found, creating default scene");
        
        m_pMainUI->GetMainWindow()->CreateDefaultScene(true);
        m_pMainUI->qslotSaveProject();
        //
        return true;
    }

    _LoadData(data, file);
    return false;
}


void AFLoadSaveManager::ForceSaveProjectNow()
{
    long prevDisableVal = m_DisableSaving.load();
    
    m_DisableSaving.store(0);
    
    SaveProjectNow();
    
    m_DisableSaving.store(prevDisableVal);
}

void AFLoadSaveManager::SaveProjectNow()
{
    if (CheckCanSaveProject() == true)
        SaveProjectDeferred();
}

void AFLoadSaveManager::SaveProjectDeferred()
{
    if (m_DisableSaving.load())
        return;

    if (!m_bProjectChanged)
        return;

    m_bProjectChanged = false;

    
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    const char* sceneCollection = config_get_string(confManager.GetGlobal(),
                                                    "Basic", "SceneCollectionFile");

    char savePath[1024];
    char fileName[1024];
    int ret;

    if (!sceneCollection)
        return;

    ret = snprintf(fileName, sizeof(fileName),
                   "SOOPStudio/basic/scenes/%s.json", sceneCollection);
    if (ret <= 0)
        return;

    ret = confManager.GetConfigPath(savePath, sizeof(savePath), fileName);
    if (ret <= 0)
        return;

    _Save(savePath);
}

void AFLoadSaveManager::_LoadTransitions(obs_data_array_t *transitions,
                                          obs_load_source_cb cb, void *private_data)
{
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    
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
            sceneContext.InitTransition(source);
            sceneContext.AddTransition(source.Get());

            if (cb)
                cb(private_data, source);
        }
    }
}

void AFLoadSaveManager::_LoadSceneListOrder(obs_data_array_t *array)
{
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    SceneItemVector& sceneItems = sceneContext.GetSceneItemVector();
    
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
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    
    m_pMainUI->ClearSceneData(true);
    //ClearContextBar();

    /* Exit OBS if clearing scene data failed for some reason. */
//    if (clearingFailed) {
//        OBSMessageBox::critical(this, QTStr("SourceLeak.Title"),
//                    QTStr("SourceLeak.Text"));
//        close();
//        return;
//    }

    sceneContext.InitDefaultTransition();

    m_pMainUI->WaitDevicePropertiesThread();

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

    AFArgOption* tmpArgOption = AFConfigManager::GetSingletonInstance().GetArgOption();
    AFStateAppContext* tmpStateApp = AFConfigManager::GetSingletonInstance().GetStates();
    
    std::string tmpOptStartingScene = tmpArgOption->GetStartingScene();
    if (tmpOptStartingScene.empty() == false)
    {
        programSceneName = tmpOptStartingScene.c_str();
        if (tmpStateApp->IsPreviewProgramMode() == false)
            sceneName = tmpOptStartingScene.c_str();
    }

    int newDuration = (int)obs_data_get_int(data, "transition_duration");
    if (!newDuration)
        newDuration = 300;

    if (!transitionName)
        transitionName = obs_source_get_name(sceneContext.GetFadeTransition());
    
    const char *curSceneCollection = config_get_string(confManager.GetGlobal(),
                                                       "Basic", "SceneCollection");

    obs_data_set_default_string(data, "name", curSceneCollection);

    const char *name = obs_data_get_string(data, "name");
    OBSSourceAutoRelease curScene;
    OBSSourceAutoRelease curProgramScene;
    obs_source_t *curTransition;

    if (!name || !*name)
        name = curSceneCollection;

    LoadAudioDevice(DESKTOP_AUDIO_1, 1, data);
    LoadAudioDevice(DESKTOP_AUDIO_2, 2, data);
    LoadAudioDevice(AUX_AUDIO_1, 3, data);
    LoadAudioDevice(AUX_AUDIO_2, 4, data);
    LoadAudioDevice(AUX_AUDIO_3, 5, data);
    LoadAudioDevice(AUX_AUDIO_4, 6, data);

    if (!sources)
        sources = std::move(groups);
    else
        obs_data_array_push_back_array(sources, groups);
    

    obs_missing_files_t *files = obs_missing_files_create();
    obs_load_sources(sources, AddMissingFiles, files);

    if (transitions)
        _LoadTransitions(transitions, AddMissingFiles, files);
    if (sceneOrder)
        _LoadSceneListOrder(sceneOrder);

    curTransition = sceneContext.FindTransition(transitionName);
    if (!curTransition)
        curTransition = sceneContext.GetFadeTransition();
//
//    ui->transitionDuration->setValue(newDuration);
    
    sceneContext.SetCurTransition(curTransition);
    AFSceneUtil::SetTransition(curTransition);

retryScene:
    curScene = obs_get_source_by_name(sceneName);
    curProgramScene = obs_get_source_by_name(programSceneName);

    /* if the starting scene command line parameter is bad at all,
     * fall back to original settings */
    if (tmpOptStartingScene.empty() == false &&
        (!curScene || !curProgramScene))
    {
        sceneName = obs_data_get_string(data, "current_scene");
        programSceneName = obs_data_get_string(data, "current_program_scene");
        
        tmpOptStartingScene.clear();
        tmpArgOption->SetStartingScene("");
        goto retryScene;
    }
    
    m_pMainUI->GetMainWindow()->SetCurrentScene(curScene.Get(), true);
    
    m_pMainUI->RefreshSceneUI();
    //

    if (!curProgramScene)
        curProgramScene = std::move(curScene);
    if (tmpStateApp->IsPreviewProgramMode())
        AFSceneUtil::TransitionToScene(curProgramScene.Get(), true);

    /* ------------------- */

//    bool projectorSave = config_get_bool(GetGlobalConfig(), "BasicWindow",
//                         "SaveProjectors");

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
    file_base.erase(file_base.size() - 5, 5);

    config_set_string(confManager.GetGlobal(), "Basic", "SceneCollection", name);
    config_set_string(confManager.GetGlobal(), "Basic", "SceneCollectionFile", file_base.c_str());

    OBSDataArrayAutoRelease quickTransitionData =
        obs_data_get_array(data, "quick_transitions");
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

//    if (vcamEnabled) {
//        OBSDataAutoRelease obj =
//            obs_data_get_obj(data, "virtual-camera");
//
//        vcamConfig.type =
//            (VCamOutputType)obs_data_get_int(obj, "type2");
//        if (vcamConfig.type == VCamOutputType::Invalid)
//            vcamConfig.type =
//                (VCamOutputType)obs_data_get_int(obj, "type");
//
//        if (vcamConfig.type == VCamOutputType::Invalid) {
//            VCamInternalType internal =
//                (VCamInternalType)obs_data_get_int(obj,
//                                   "internal");
//
//            switch (internal) {
//            case VCamInternalType::Default:
//                vcamConfig.type = VCamOutputType::ProgramView;
//                break;
//            case VCamInternalType::Preview:
//                vcamConfig.type = VCamOutputType::PreviewOutput;
//                break;
//            }
//        }
//        vcamConfig.scene = obs_data_get_string(obj, "scene");
//        vcamConfig.source = obs_data_get_string(obj, "source");
//    }

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

    if (tmpArgOption->GetDisableMissingFilesCheck() == false)
        m_pMainUI->ShowMissingFilesDialog(files);

    DecreaseCheckSaveCnt();

//    if (vcamEnabled)
//        outputHandler->UpdateVirtualCamOutputSource();
//
//    if (api) {
//        api->on_event(OBS_FRONTEND_EVENT_SCENE_CHANGED);
//        api->on_event(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
//    }
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

    const char* sceneCollection = config_get_string(AFConfigManager::GetSingletonInstance().GetGlobal(),
                                                    "Basic", "SceneCollection");

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

    obs_data_set_string(saveData, "current_transition",
                obs_source_get_name(transition));
    obs_data_set_int(saveData, "transition_duration", transitionDuration);

    return saveData;
}

obs_data_array_t* AFLoadSaveManager::_SaveSceneListOrder()
{
    obs_data_array_t *sceneOrder = obs_data_array_create();

    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    SceneItemVector& sceneItems = sceneContext.GetSceneItemVector();
    
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

    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    std::vector<OBSSource>& tmpTransitions = sceneContext.GetRefTransitions();
    
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
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    
    OBSScene scene = sceneContext.GetCurrOBSScene();
    OBSSource curProgramScene = OBSGetStrongRef(sceneContext.GetProgramOBSScene());
    if (!curProgramScene)
        curProgramScene = obs_scene_get_source(scene);

    OBSDataArrayAutoRelease sceneOrder = _SaveSceneListOrder();
    OBSDataArrayAutoRelease transitions = _SaveTransitions();
    OBSDataArrayAutoRelease quickTrData = _SaveQuickTransitions();
    //OBSDataArrayAutoRelease savedProjectorList = SaveProjectors();
    OBSDataAutoRelease saveData = _GenerateSaveData(
        sceneOrder, quickTrData, 200/*ui->transitionDuration->value()*/,
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

//    if (vcamEnabled) {
//        OBSDataAutoRelease obj = obs_data_create();
//
//        obs_data_set_int(obj, "type2", (int)vcamConfig.type);
//        switch (vcamConfig.type) {
//        case VCamOutputType::Invalid:
//        case VCamOutputType::ProgramView:
//        case VCamOutputType::PreviewOutput:
//            break;
//        case VCamOutputType::SceneOutput:
//            obs_data_set_string(obj, "scene",
//                        vcamConfig.scene.c_str());
//            break;
//        case VCamOutputType::SourceOutput:
//            obs_data_set_string(obj, "source",
//                        vcamConfig.source.c_str());
//            break;
//        }
//
//        obs_data_set_obj(saveData, "virtual-camera", obj);
//    }

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
