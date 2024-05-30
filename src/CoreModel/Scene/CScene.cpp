#include "CScene.h"
#include "CSceneContext.h"


#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Config/CConfigManager.h"


bool AFSceneUtil::SceneItemHasVideo(obs_sceneitem_t* item)
{
	obs_source_t* source = obs_sceneitem_get_source(item);
	uint32_t flags = obs_source_get_output_flags(source);
	return (flags & OBS_SOURCE_VIDEO) != 0;
}

obs_source_t* AFSceneUtil::CreateOBSScene(const char* name)
{
	OBSSceneAutoRelease scene = obs_scene_create(name);
	obs_source_t* source = obs_scene_get_source(scene);
	
	return source;
}

void AFSceneUtil::TransitionToScene(OBSSource source, bool force,
									bool quickTransition , int quickDuration, 
									bool black, bool manual)
{
    
	auto& sceneContext = AFSceneContext::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    AFArgOption* tmpArgs = confManager.GetArgOption();
    AFStateAppContext* tmpStateApp = confManager.GetStates();


	obs_scene_t* scene = obs_scene_from_source(source);
	bool usingPreviewProgram = tmpStateApp->IsPreviewProgramMode();
    bool sceneDuplicationMode = tmpStateApp->GetSceneDuplicationMode();
    bool editPropertiesMode = tmpStateApp->GetEditPropertiesMode();
    bool swapScenesMode = tmpStateApp->GetSwapScenesMode();

	if (!scene)
		return;

	if (usingPreviewProgram) {
        sceneContext.SetLastProgramOBSScene(sceneContext.GetProgramOBSScene());
        sceneContext.SetProgramOBSScene(source);

		if (!force && !black) {
			OBSSource lastScene = OBSGetStrongRef(sceneContext.GetLastProgramOBSScene());

			if (!sceneDuplicationMode && lastScene == source)
				return;

            if (swapScenesMode) {
                sceneContext.SetSwapOBSScene(sceneContext.GetLastProgramOBSScene());
            }
			if (swapScenesMode && lastScene &&
                lastScene != CnvtToOBSSource(sceneContext.GetCurrOBSScene()))
                sceneContext.SetSwapOBSScene(sceneContext.GetLastProgramOBSScene());
		}
	}

	if (usingPreviewProgram && sceneDuplicationMode) {
		scene = obs_scene_duplicate(
			scene, obs_source_get_name(obs_scene_get_source(scene)),
			editPropertiesMode ? OBS_SCENE_DUP_PRIVATE_COPY
			: OBS_SCENE_DUP_PRIVATE_REFS);
		source = obs_scene_get_source(scene);
	}

	OBSSourceAutoRelease transition = obs_get_output_source(0);
	if (!transition) {
		if (usingPreviewProgram && sceneDuplicationMode)
			obs_scene_release(scene);
		return;
	}

	float t = obs_transition_get_time(transition);
	bool stillTransitioning = t < 1.0f && t > 0.0f;

	// If actively transitioning, block new transitions from starting
	if (usingPreviewProgram && stillTransitioning)
		goto cleanup;

	//if (usingPreviewProgram) {
	//	if (!black && !manual) {
	//		const char* sceneName = obs_source_get_name(source);
	//		blog(LOG_INFO, "User switched Program to scene '%s'",
	//			sceneName);

	//	}
	//	else if (black && !prevFTBSource) {
	//		OBSSourceAutoRelease target =
	//			obs_transition_get_active_source(transition);
	//		const char* sceneName = obs_source_get_name(target);
	//		blog(LOG_INFO, "User faded from scene '%s' to black",
	//			sceneName);

	//	}
	//	else if (black && prevFTBSource) {
	//		const char* sceneName =
	//			obs_source_get_name(prevFTBSource);
	//		blog(LOG_INFO, "User faded from black to scene '%s'",
	//			sceneName);

	//	}
	//	else if (manual) {
	//		const char* sceneName = obs_source_get_name(source);
	//		blog(LOG_INFO,
	//			"User started manual transition to scene '%s'",
	//			sceneName);
	//	}
	//}

	if (force) {
		obs_transition_set(transition, source);
		//if (api)
		//	api->on_event(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	}
	else {

		int duration = 200;

		/* check for scene override */
		//OBSSource trOverride = GetOverrideTransition(source);

		//if (trOverride && !overridingTransition && !quickTransition) {
		//	transition = std::move(trOverride);
			duration = GetOverrideTransitionDuration(source);
			OverrideTransition(transition.Get());
		//	overridingTransition = true;
		//}

		//if (black && !prevFTBSource) {
		//	prevFTBSource = source;
		//	source = nullptr;
		//}
		//else if (black && prevFTBSource) {
		//	source = prevFTBSource;
		//	prevFTBSource = nullptr;
		//}
		//else if (!black) {
		//	prevFTBSource = nullptr;
		//}

		if (quickTransition)
			duration = quickDuration;
		else
			duration = sceneContext.GetCurDuraition();

		enum obs_transition_mode mode =
					manual ? OBS_TRANSITION_MODE_MANUAL
						   : OBS_TRANSITION_MODE_AUTO;

		//EnableTransitionWidgets(false);

		bool success = obs_transition_start(transition, mode, duration, source);

		//if (!success)
		//	TransitionFullyStopped();
	}

cleanup:
	if (usingPreviewProgram && sceneDuplicationMode)
		obs_scene_release(scene);
}

int AFSceneUtil::GetOverrideTransitionDuration(OBSSource source)
{
	if (!source)
		return 300;

	OBSDataAutoRelease data = obs_source_get_private_settings(source);
	obs_data_set_default_int(data, "transition_duration", 300);

	return (int)obs_data_get_int(data, "transition_duration");
}

void AFSceneUtil::OverrideTransition(OBSSource transition)
{
	OBSSourceAutoRelease oldTransition = obs_get_output_source(0);

	if (transition != oldTransition) {
		obs_transition_swap_begin(transition, oldTransition);
		obs_set_output_source(0, transition);
		obs_transition_swap_end(transition, oldTransition);
	}
}

void AFSceneUtil::SetTransition(OBSSource transition)
{
	OBSSourceAutoRelease oldTransition = obs_get_output_source(0);

	if (oldTransition && transition) {
		obs_transition_swap_begin(transition, oldTransition);
		obs_set_output_source(0, transition);
		obs_transition_swap_end(transition, oldTransition);
	}
	else {
		obs_set_output_source(0, transition);
	}
}