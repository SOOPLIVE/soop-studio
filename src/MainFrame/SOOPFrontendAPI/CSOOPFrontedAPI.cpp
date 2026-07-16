#include "CSOOPFrontendAPI.h"

#include "qt-wrappers.hpp"

#include "Application/CApplication.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Source/CSource.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Service/CService.h"
#include "CoreModel/Video/CVideo.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/AudioSource/CAudioSource.h"
#include "MainFrame/SceneCollection/CMainSceneCollection.h"
#include "MainFrame/Profile/CMainProfile.h"

soop_frontend_callbacks* InitializeAPIInterface(AFMainFrame* main)
{
    soop_frontend_callbacks* api = new SOOPStudioAPI(main);
    soop_frontend_set_callbacks_internal(api);
    return api;
}

void SOOPStudioAPI::obs_frontend_get_scenes(struct obs_frontend_source_list* sources)
{

    SceneItemVector& scene_vector = SCENE_CONTEXT.GetSceneItemVector();
    for (auto itr = scene_vector.begin(); itr < scene_vector.end(); itr++) {

        OBSScene scene = (*itr)->GetScene();
        obs_source_t* source = obs_scene_get_source(scene);

        if (obs_source_get_ref(source) != nullptr)
            da_push_back(sources->sources, &source);
    }
}

obs_source_t* SOOPStudioAPI::obs_frontend_get_current_scene(void)
{
    OBSSource source = SCENE_CONTEXT.GetCurrentSceneSource();
    return obs_source_get_ref(source);
}

void SOOPStudioAPI::obs_frontend_set_current_scene(obs_source_t* scene) 
{
    if (main && main->IsCompleteInit()) {
        QMetaObject::invokeMethod(main, "qslotSetCurrentSceneFrontendAPI", WaitConnection(),
                                  Q_ARG(OBSSource, OBSSource(scene)),
                                  Q_ARG(bool, false));
    }
}

void SOOPStudioAPI::obs_frontend_get_transitions(struct obs_frontend_source_list* sources) {
}

obs_source_t* SOOPStudioAPI::obs_frontend_get_current_transition(void) {
    return nullptr;
}

void SOOPStudioAPI::obs_frontend_set_current_transition(obs_source_t* transition) {
}

int SOOPStudioAPI::obs_frontend_get_transition_duration(void) {
    return 200;
}

void SOOPStudioAPI::obs_frontend_set_transition_duration(int duration) {
}

void SOOPStudioAPI::obs_frontend_release_tbar(void) {
}

void SOOPStudioAPI::obs_frontend_set_tbar_position(int position) {
}

int SOOPStudioAPI::obs_frontend_get_tbar_position(void) {
    return 0;
}

void SOOPStudioAPI::obs_frontend_get_scene_collections(std::vector<std::string>& strings)
{
    for (auto &[collectionName, collection] : MAIN_SCENECOLLECTION->GetSceneCollectionCache()) {
        strings.emplace_back(collectionName);
    }
}

char* SOOPStudioAPI::obs_frontend_get_current_scene_collection(void)
{
    const OBSSceneCollection& currentCollection = MAIN_SCENECOLLECTION->GetCurrentSceneCollection();
    return bstrdup(currentCollection.name.c_str());
}

void SOOPStudioAPI::obs_frontend_set_current_scene_collection(const char* collection)
{
    /*
    QList<QAction*> menuActions = ui->sceneCollectionMenu->actions();
    QString qstrCollection = QT_UTF8(collection);

    for(int i = 0; i < menuActions.count(); i++) {
        QAction* action = menuActions[i];
        QVariant v = action->property("file_name");

        if(v.typeName() != nullptr) {
            if(action->text() == qstrCollection) {
                action->trigger();
                break;
            }
        }
    }
    */
}

bool SOOPStudioAPI::obs_frontend_add_scene_collection(const char* name)
{
    bool success = false;
    QMetaObject::invokeMethod(main, "CreateNewSceneCollection", WaitConnection(),
                              Q_RETURN_ARG(bool, success), Q_ARG(QString, QT_UTF8(name)));
    return success;
}

void SOOPStudioAPI::obs_frontend_get_profiles(std::vector<std::string>& strings)
{
    const OBSProfileCache& profiles = MAIN_PROFILE->GetProfileCache();

    for(auto& [profileName, profile] : profiles) {
        strings.emplace_back(profileName);
    }
}

char* SOOPStudioAPI::obs_frontend_get_current_profile(void)
{
    const OBSProfile& profile = MAIN_PROFILE->GetCurrentProfile();
    return bstrdup(profile.name.c_str());
}

char* SOOPStudioAPI::obs_frontend_get_current_profile_path(void)
{
    const OBSProfile& profile = MAIN_PROFILE->GetCurrentProfile();
    return bstrdup(profile.path.u8string().c_str());
}

void SOOPStudioAPI::obs_frontend_set_current_profile(const char* profile)
{
    /*
    QList<QAction*> menuActions = main->ui->profileMenu->actions();
    QString qstrProfile = QT_UTF8(profile);

    for(int i = 0; i < menuActions.count(); i++) {
        QAction* action = menuActions[i];
        QVariant v = action->property("file_name");

        if(v.typeName() != nullptr) {
            if(action->text() == qstrProfile) {
                action->trigger();
                break;
            }
        }
    }
    */
}

void SOOPStudioAPI::obs_frontend_create_profile(const char* name)
{
    QMetaObject::invokeMethod(main, "CreateNewProfile", Q_ARG(QString, name));
}

void SOOPStudioAPI::obs_frontend_duplicate_profile(const char* name)
{
    QMetaObject::invokeMethod(main, "DuplicateProfile", Q_ARG(QString, name));
}

void SOOPStudioAPI::obs_frontend_delete_profile(const char* profile)
{
    QMetaObject::invokeMethod(main, "DeleteProfile", Q_ARG(QString, profile));
}

void SOOPStudioAPI::obs_frontend_streaming_start(void) 
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "qslotStartStreamingFrontendAPI");
}

void SOOPStudioAPI::obs_frontend_streaming_stop(void) 
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "qslotStopStreamingFrontendAPI");
}

bool SOOPStudioAPI::obs_frontend_streaming_active(void) 
{
    return AFOutputUtil::GetStreamingCheck();
}

void SOOPStudioAPI::obs_frontend_recording_start(void) 
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "qslotStartRecordingFrontendAPI");
}

void SOOPStudioAPI::obs_frontend_recording_stop(void) 
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "qslotStopRecordingFrontendAPI");
}

bool SOOPStudioAPI::obs_frontend_recording_active(void) 
{
    return AFOutputUtil::GetRecordingCheck();
}

void SOOPStudioAPI::obs_frontend_recording_pause(bool pause)
{
    QMetaObject::invokeMethod(main, pause ? "PauseRecording" : "UnpauseRecording");
}

bool SOOPStudioAPI::obs_frontend_recording_paused(void)
{
    return false;
    //return AFOutputUtil::GetRecordingPauseCheck();
}

bool SOOPStudioAPI::obs_frontend_recording_split_file(void)
{
    if(AFOutputUtil::GetRecordingCheck() && !AFOutputUtil::GetRecordingPauseCheck()) {
        obs_output_t* fileOutput = AFOutputUtil::GetRecordingFileOutput();
        proc_handler_t* ph = obs_output_get_proc_handler(fileOutput);
        uint8_t stack[128];
        calldata cd;
        calldata_init_fixed(&cd, stack, sizeof(stack));
        proc_handler_call(ph, "split_file", &cd);
        bool result = calldata_bool(&cd, "split_file_enabled");
        return result;
    } else {
        return false;
    }
}
bool SOOPStudioAPI::obs_frontend_recording_add_chapter(const char* name)
{
    if(!AFOutputUtil::GetRecordingCheck() || AFOutputUtil::GetRecordingPauseCheck())
        return false;

    obs_output_t* fileOutput = AFOutputUtil::GetRecordingFileOutput();
    proc_handler_t* ph = obs_output_get_proc_handler(fileOutput);

    calldata cd;
    calldata_init(&cd);
    calldata_set_string(&cd, "chapter_name", name);
    bool result = proc_handler_call(ph, "add_chapter", &cd);
    calldata_free(&cd);
    return result;
}

void SOOPStudioAPI::obs_frontend_replay_buffer_start(void) 
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "StartReplayBuffer");
}

void SOOPStudioAPI::obs_frontend_replay_buffer_save(void) 
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "ReplayBufferSave");
}

void SOOPStudioAPI::obs_frontend_replay_buffer_stop(void) 
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "StopReplayBuffer");
}

bool SOOPStudioAPI::obs_frontend_replay_buffer_active(void) {
    return false;
    //return AFOutputUtil::GetReplayBufferCheck();
}

void* SOOPStudioAPI::obs_frontend_add_tools_menu_qaction(const char* name)
{
    return nullptr;
    /*
    main->ui->menuTools->setEnabled(true);
    QAction* action = main->ui->menuTools->addAction(QT_UTF8(name));
    action->setMenuRole(QAction::NoRole);
    return static_cast<void*>(action);
    */
}

void SOOPStudioAPI::obs_frontend_add_tools_menu_item(const char* name,
                                                     obs_frontend_cb callback,
                                                     void* private_data)
{
    /*
    main->ui->menuTools->setEnabled(true);

    auto func = [private_data, callback]() {
        callback(private_data);
    };

    QAction* action = main->ui->menuTools->addAction(QT_UTF8(name));
    action->setMenuRole(QAction::NoRole);
    QObject::connect(action, &QAction::triggered, func);
    */
}

void* SOOPStudioAPI::obs_frontend_add_dock(void* dock)
{
    return nullptr;
    /*
    QDockWidget* d = reinterpret_cast<QDockWidget*>(dock);

    QString name = d->objectName();
    if(name.isEmpty() || main->IsDockObjectNameUsed(name)) {
        blog(LOG_WARNING, "The object name of the added dock is empty or already used,"
                  " a temporary one will be set to avoid conflicts");

        char* uuid = os_generate_uuid();
        name = QT_UTF8(uuid);
        bfree(uuid);
        name.append("_oldExtraDock");

        d->setObjectName(name);
    }

    return (void*)main->AddDockWidget(d);
    */
}

bool SOOPStudioAPI::obs_frontend_add_dock_by_id(const char* id, const char* title, void* widget)
{
    return true;
    /*
    if(main->IsDockObjectNameUsed(QT_UTF8(id))) {
        blog(LOG_WARNING,
             "Dock id '%s' already used!  "
             "Duplicate library?",
             id);
        return false;
    }

    OBSDock* dock = new OBSDock(main);
    dock->setWidget((QWidget*)widget);
    dock->setWindowTitle(QT_UTF8(title));
    dock->setObjectName(QT_UTF8(id));

    main->AddDockWidget(dock, Qt::RightDockWidgetArea);

    dock->setVisible(false);
    dock->setFloating(true);

    return true;
    */
}

void SOOPStudioAPI::obs_frontend_remove_dock(const char* id)
{
    //main->RemoveDockWidget(QT_UTF8(id));
}

bool SOOPStudioAPI::obs_frontend_add_custom_qdock(const char* id, void* dock)
{
    return true;
    /*
    if(main->IsDockObjectNameUsed(QT_UTF8(id))) {
        blog(LOG_WARNING,
             "Dock id '%s' already used!  "
             "Duplicate library?",
             id);
        return false;
    }

    QDockWidget* d = reinterpret_cast<QDockWidget*>(dock);
    d->setObjectName(QT_UTF8(id));

    main->AddCustomDockWidget(d);
    */

    return true;
}

void SOOPStudioAPI::obs_frontend_add_event_callback(obs_frontend_event_cb callback,
                                                    void* private_data) {
    size_t idx = GetCallbackIdx(callbacks, callback, private_data);
    if (idx == (size_t)-1)
        callbacks.emplace_back(callback, private_data);
}

void SOOPStudioAPI::obs_frontend_remove_event_callback(obs_frontend_event_cb callback,
                                                       void* private_data) {
    size_t idx = GetCallbackIdx(callbacks, callback, private_data);
    if (idx == (size_t)-1)
        return;

    callbacks.erase(callbacks.begin() + idx);
}

obs_output_t* SOOPStudioAPI::obs_frontend_get_streaming_output(void)
{
    return nullptr;
    /*
    auto multitrackVideo = main->outputHandler->multitrackVideo.get();
    auto mtvOutput = multitrackVideo ? obs_output_get_ref(multitrackVideo->StreamingOutput()) : nullptr;
    if(mtvOutput)
        return mtvOutput;

    OBSOutput output = main->outputHandler->streamOutput.Get();
    return obs_output_get_ref(output);
    */
}

obs_output_t* SOOPStudioAPI::obs_frontend_get_recording_output(void)
{
    return nullptr;
    /*
    OBSOutput out = AFOutputUtil::GetRecordingFileOutput();
    return obs_output_get_ref(out);
    */
}

obs_output_t* SOOPStudioAPI::obs_frontend_get_replay_buffer_output(void)
{
    return nullptr;
    /*
    OBSOutput out = main->outputHandler->replayBuffer.Get();
    return obs_output_get_ref(out);
    */
}

config_t* SOOPStudioAPI::obs_frontend_get_profile_config(void)
{
    return ACTIVECONFIG;
}

config_t* SOOPStudioAPI::obs_frontend_get_global_config(void) {
    blog(LOG_WARNING,
         "DEPRECATION: obs_frontend_get_global_config is deprecated. "
         "Read from global or user configuration explicitly instead.");

    return APPCONFIG;
}

config_t* SOOPStudioAPI::obs_frontend_get_app_config()
{
    return APPCONFIG;
}
config_t* SOOPStudioAPI::obs_frontend_get_user_config()
{
    return USERCONFIG;
}

void SOOPStudioAPI::obs_frontend_open_projector(const char* type, int monitor,
                                                const char* geometry,
                                                const char* name)
{
    /*
    SavedProjectorInfo proj = {
        ProjectorType::Preview,
        monitor,
        geometry ? geometry : "",
        name ? name : "",
    };
    if(type) {
        if(astrcmpi(type, "Source") == 0)
            proj.type = ProjectorType::Source;
        else if(astrcmpi(type, "Scene") == 0)
            proj.type = ProjectorType::Scene;
        else if(astrcmpi(type, "StudioProgram") == 0)
            proj.type = ProjectorType::StudioProgram;
        else if(astrcmpi(type, "Multiview") == 0)
            proj.type = ProjectorType::Multiview;
    }
    QMetaObject::invokeMethod(main, "OpenSavedProjector", WaitConnection(),
                  Q_ARG(SavedProjectorInfo*, &proj));
    */
}

void SOOPStudioAPI::obs_frontend_save(void)
{
    main->qslotSaveProject();
}

void SOOPStudioAPI::obs_frontend_defer_save_begin(void)
{
    QMetaObject::invokeMethod(main, "DeferSaveBegin");
}

void SOOPStudioAPI::obs_frontend_defer_save_end(void)
{
    QMetaObject::invokeMethod(main, "DeferSaveEnd");
}

void SOOPStudioAPI::obs_frontend_add_save_callback(obs_frontend_save_cb callback, void* private_data) 
{
    size_t idx = GetCallbackIdx(saveCallbacks, callback, private_data);
    if (idx == (size_t)-1)
        saveCallbacks.emplace_back(callback, private_data);
}

void SOOPStudioAPI::obs_frontend_remove_save_callback(obs_frontend_save_cb callback, void* private_data)
{
    size_t idx = GetCallbackIdx(saveCallbacks, callback, private_data);
    if (idx == (size_t)-1)
        return;

    saveCallbacks.erase(saveCallbacks.begin() + idx);
}

void SOOPStudioAPI::obs_frontend_add_preload_callback(obs_frontend_save_cb callback, void* private_data)
{
    size_t idx = GetCallbackIdx(preloadCallbacks, callback, private_data);
    if (idx == (size_t)-1)
        preloadCallbacks.emplace_back(callback, private_data);
}

void SOOPStudioAPI::obs_frontend_remove_preload_callback(obs_frontend_save_cb callback, void* private_data)
{
    size_t idx = GetCallbackIdx(preloadCallbacks, callback, private_data);
    if (idx == (size_t)-1)
        return;

    preloadCallbacks.erase(preloadCallbacks.begin() + idx);
}

void SOOPStudioAPI::obs_frontend_push_ui_translation(obs_frontend_translate_ui_cb translate)
{
    //App()->PushUITranslation(translate);
}

void SOOPStudioAPI::obs_frontend_pop_ui_translation(void)
{
    //App()->PopUITranslation();
}

void SOOPStudioAPI::obs_frontend_set_streaming_service(obs_service_t* service)
{
    //SERVICE_MANAGER.SetService(service);
}

obs_service_t* SOOPStudioAPI::obs_frontend_get_streaming_service(void)
{
    return nullptr;
    //return SERVICE_MANAGER.GetService();
}

void SOOPStudioAPI::obs_frontend_save_streaming_service(void)
{
    //SERVICE_MANAGER.SaveService();
}

bool SOOPStudioAPI::obs_frontend_preview_program_mode_active(void)
{
    return main->IsPreviewProgramMode();
}

void SOOPStudioAPI::obs_frontend_set_preview_program_mode(bool enable)
{
    if (main && main->IsCompleteInit()) {
        QMetaObject::invokeMethod(main->GetMainWindow(), "ToggleStudioModeBlock", Q_ARG(bool, enable));
    }
}

void SOOPStudioAPI::obs_frontend_preview_program_trigger_transition(void)
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "qslotTransitionStudioModeScene");
}

bool SOOPStudioAPI::obs_frontend_preview_enabled(void)
{
    return main->GetPreviewEnable();
}

void SOOPStudioAPI::obs_frontend_set_preview_enabled(bool enable)
{
    if(main->GetPreviewEnable() != enable)
        main->EnablePreviewDisplay(enable);
}

obs_source_t* SOOPStudioAPI::obs_frontend_get_current_preview_scene(void)
{
    /*
    if(main->IsPreviewProgramMode()) {
        OBSSource source = SCENE_CONTEXT.GetCurrOBSSceneSource();
        return obs_source_get_ref(source);
    }
    */
    return nullptr;
}

void SOOPStudioAPI::obs_frontend_set_current_preview_scene(obs_source_t* scene)
{
    if (main && main->IsCompleteInit()) {
        if (main->IsPreviewProgramMode()) {
            QMetaObject::invokeMethod(main, "qslotSetCurrentSceneFrontendAPI",
                                      Q_ARG(OBSSource, OBSSource(scene)),
                                      Q_ARG(bool, false));
        }
    }
}

void SOOPStudioAPI::obs_frontend_take_screenshot(void)
{
    QMetaObject::invokeMethod(main, "Screenshot");
}

void SOOPStudioAPI::obs_frontend_take_source_screenshot(obs_source_t* source)
{
    QMetaObject::invokeMethod(main, "Screenshot", Q_ARG(OBSSource, OBSSource(source)));
}

obs_output_t* SOOPStudioAPI::obs_frontend_get_virtualcam_output(void)
{
    return main->GetVirtualCamOutput();
}

void SOOPStudioAPI::obs_frontend_start_virtualcam(void)
{
    QMetaObject::invokeMethod(main, "qslotStartVCam");
}

void SOOPStudioAPI::obs_frontend_stop_virtualcam(void)
{
    QMetaObject::invokeMethod(main, "qslotStopVCam");
}

bool SOOPStudioAPI::obs_frontend_virtualcam_active(void)
{
    return false;
}

void SOOPStudioAPI::obs_frontend_reset_video(void)
{
    AFVideoUtil::ResetVideo();
}

void SOOPStudioAPI::obs_frontend_open_source_properties(obs_source_t* source)
{
    //QMetaObject::invokeMethod(main, "OpenProperties", Q_ARG(OBSSource, OBSSource(source)));
}

void SOOPStudioAPI::obs_frontend_open_source_filters(obs_source_t* source)
{
    //QMetaObject::invokeMethod(main, "OpenFilters", Q_ARG(OBSSource, OBSSource(source)));
}

void SOOPStudioAPI::obs_frontend_open_source_interaction(obs_source_t* source)
{
    //QMetaObject::invokeMethod(main, "OpenInteraction", Q_ARG(OBSSource, OBSSource(source)));
}

void SOOPStudioAPI::obs_frontend_open_sceneitem_edit_transform(obs_sceneitem_t* item)
{
    //QMetaObject::invokeMethod(main, "OpenEditTransform", Q_ARG(OBSSceneItem, OBSSceneItem(item)));
}

char* SOOPStudioAPI::obs_frontend_get_current_record_output_path(void)
{
    const char* recordOutputPath = AFOutputUtil::GetCurrentOutputPath();
    return bstrdup(recordOutputPath);
}

const char* SOOPStudioAPI::obs_frontend_get_locale_string(const char* string)
{
    return Str(string);
}

bool SOOPStudioAPI::obs_frontend_is_theme_dark(void) {
    return true;
}

char* SOOPStudioAPI::obs_frontend_get_last_recording(void) {
    return bstrdup("");
}

char* SOOPStudioAPI::obs_frontend_get_last_screenshot(void) {
    return bstrdup("");
}

char* SOOPStudioAPI::obs_frontend_get_last_replay(void) {
    return bstrdup("");
}

void SOOPStudioAPI::obs_frontend_add_undo_redo_action(const char* name,
                                                      const undo_redo_cb undo,
                                                      const undo_redo_cb redo,
                                                      const char* undo_data,
                                                      const char* redo_data,
                                                      bool repeatable) {
}

void SOOPStudioAPI::on_load(obs_data_t* settings)
{
    for (size_t i = saveCallbacks.size(); i > 0; i--) {
        auto cb = saveCallbacks[i - 1];
        cb.callback(settings, false, cb.private_data);
    }
}

void SOOPStudioAPI::on_preload(obs_data_t* settings)
{
    for (size_t i = preloadCallbacks.size(); i > 0; i--) {
        auto cb = preloadCallbacks[i - 1];
        cb.callback(settings, false, cb.private_data);
    }
}

void SOOPStudioAPI::on_save(obs_data_t* settings)
{
    for (size_t i = saveCallbacks.size(); i > 0; i--) {
        auto cb = saveCallbacks[i - 1];
        cb.callback(settings, true, cb.private_data);
    }
}

void SOOPStudioAPI::on_event(enum obs_frontend_event event)
{
    //if (main->disableSaving &&
    //    event != OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP &&
    //    event != OBS_FRONTEND_EVENT_EXIT)
    //    return;

    for (size_t i = callbacks.size(); i > 0; i--) {
        auto cb = callbacks[i - 1];
        cb.callback(event, cb.private_data);
    }
}

void SOOPStudioAPI::soop_frontend_add_callback(soop_frontend_cb callback, void* private_data)
{
    size_t idx = GetCallbackIdx(soop_callbacks, callback, private_data);
    if (idx == (size_t)-1)
        soop_callbacks.emplace_back(callback, private_data);
}

void SOOPStudioAPI::soop_frontend_remove_callback(soop_frontend_cb callback, void* private_data)
{
    size_t idx = GetCallbackIdx(soop_callbacks, callback, private_data);
    if (idx == (size_t)-1)
        return;

    soop_callbacks.erase(soop_callbacks.begin() + idx);
}

void SOOPStudioAPI::on_soop_event(enum soop_frontend_type frontend_type, void* data)
{
    for (size_t i = soop_callbacks.size(); i > 0; i--) {
        auto cb = soop_callbacks[i - 1];
        cb.callback(frontend_type, data, cb.private_data);
    }
}

void SOOPStudioAPI::soop_frontend_send_data(void* data) {
}

void SOOPStudioAPI::soop_frontend_toogle_main_mic() 
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "qslotToggleMainMicFrontendAPI");
}

void SOOPStudioAPI::soop_frontend_toogle_main_vol(void)
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "qslotToggleMainVolFrontendAPI");
}

bool SOOPStudioAPI::soop_frontend_main_mic_state()
{
   return !MAIN_AUDIOSOURCE->IsMicMuted();
}

bool SOOPStudioAPI::soop_frontend_main_vol_state()
{
    return !MAIN_AUDIOSOURCE->IsAudioMuted();
}

void SOOPStudioAPI::soop_frontend_toogle_channel_sidebar(void)
{
    if (main && main->IsCompleteInit())
        QMetaObject::invokeMethod(main, "qslotToggleSOOPChannelSidebarFrontendAPI");
}

void SOOPStudioAPI::soop_frontend_toogle_lnbmenu(int menuType)
{
    if (main && main->IsCompleteInit()) {
        QMetaObject::invokeMethod(main, "qslotToggleSOOPLnbMenuFrontendAPI", Q_ARG(int, menuType));
    }
}

bool SOOPStudioAPI::soop_frontend_get_lnbmenu_state(int menuType)
{
    if (main && main->IsCompleteInit()) {
        AFQBorderPopupBaseWidget* popup = nullptr;
        if (MAIN_BLOCKMANAGER && MAIN_BLOCKMANAGER->GetPopup((ENUM_WINDOW_TYPE)menuType, popup))
            return true;
        else
            return false;
    }
    return false;
}

bool SOOPStudioAPI::soop_frontend_add_imageprinter(const char* file, const char* path, int width, int height)
{
    if (main && main->IsCompleteInit()) {

        const char* id = "image_source";

        obs_transform_info transInfo;
        vec2_set(&transInfo.pos, 0, 0);
        vec2_set(&transInfo.scale, 0.615f, 0.615f);
        vec2_set(&transInfo.bounds, width, height);

        transInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
        transInfo.rot = 0.0f;
        transInfo.bounds_type = OBS_BOUNDS_NONE;
        transInfo.bounds_alignment = OBS_ALIGN_CENTER;

        OBSDataAutoRelease settings = obs_data_create();
        obs_data_set_string(settings, "file", path);

        OBSSource newSource;
        if (!AFSourceUtil::AddNewSource(main, id, file, true, newSource, &transInfo, settings))
            return false;

        AFSourceUtil::SetUndoRedoAddSource(id, file, true);

        return true;
    }

    return false;
}