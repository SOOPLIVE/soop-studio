#include "CSceneContext.h"

#include <util/profiler.hpp>
#include "Application/CApplication.h"

#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Source/CSource.h"

#include "Blocks/SceneSourceDock/CSourceListView.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"
#include "MainFrame/SceneSource/CMainSceneSource.h"


void AFSceneContext::AddSource(void* _data, obs_scene* scene)
{
	AddSourceData* data = (AddSourceData*)_data;
	obs_sceneitem_t* sceneitem;

	sceneitem = obs_scene_add(scene, data->source);

	if(data->transform != nullptr)
		obs_sceneitem_set_info(sceneitem, data->transform);
	if(data->crop != nullptr)
		obs_sceneitem_set_crop(sceneitem, data->crop);
	if(data->blend_method != nullptr)
		obs_sceneitem_set_blending_method(sceneitem, *data->blend_method);
	if(data->blend_mode != nullptr)
		obs_sceneitem_set_blending_mode(sceneitem, *data->blend_mode);

	obs_sceneitem_set_visible(sceneitem, data->visible);
}

void AFSceneContext::InitContext()
{
	ClearContext();
}

void AFSceneContext::InitSourceSignalCallback()
{
	ProfileScope("AFSceneContext::InitSourceSignalCallback");
	//
	m_signalHandlers.reserve(m_signalHandlers.size() + 9);
	m_signalHandlers.emplace_back(obs_get_signal_handler(), "source_create", AFSourceUtil::SourceCreated, MAINFRAME);
	m_signalHandlers.emplace_back(obs_get_signal_handler(), "source_remove", AFSourceUtil::SourceRemoved, MAINFRAME);
	m_signalHandlers.emplace_back(obs_get_signal_handler(), "source_activate", AFSourceUtil::SourceActivated, MAINFRAME);
	m_signalHandlers.emplace_back(obs_get_signal_handler(), "source_deactivate", AFSourceUtil::SourceDeactivated, MAINFRAME);
	m_signalHandlers.emplace_back(obs_get_signal_handler(), "source_audio_activate", AFSourceUtil::SourceAudioActivated, MAINFRAME);
	m_signalHandlers.emplace_back(obs_get_signal_handler(), "source_audio_deactivate", AFSourceUtil::SourceAudioDeactivated, MAINFRAME);
	m_signalHandlers.emplace_back(obs_get_signal_handler(), "source_rename", AFSourceUtil::SourceRenamed, MAINFRAME);
	//
	m_signalHandlers.emplace_back(
		obs_get_signal_handler(), "source_filter_add",
		[](void* data, calldata_t*) {
			QMetaObject::invokeMethod(static_cast<CMainSceneSource*>(data), "UpdateEditMenu", Qt::QueuedConnection);
	},
		MAIN_SCENESOURCE);
	m_signalHandlers.emplace_back(
		obs_get_signal_handler(), "source_filter_remove",
		[](void* data, calldata_t*) {
			QMetaObject::invokeMethod(static_cast<CMainSceneSource*>(data), "UpdateEditMenu", Qt::QueuedConnection);
	},
		MAIN_SCENESOURCE);
}

void AFSceneContext::ClearSourceSignalCallback()
{
	m_signalHandlers.clear();
}

void AFSceneContext::ClearSceneListButtonList()
{
	auto iterScene = m_vecSceneItem.begin();
	for(; iterScene != m_vecSceneItem.end(); ++iterScene) {
		AFQSceneListItem* item = (*iterScene);
		if(!item)
			continue;
		item->close();
		delete item;
	}
	m_vecSceneItem.clear();
	currentScene = nullptr;

	if(m_pSourceListViewPtr)
		m_pSourceListViewPtr->Clear();
}

void AFSceneContext::ClearSceneTransitionList()
{
	m_obsTransitions.clear();
	SetCurTransition(nullptr);
}

void AFSceneContext::ClearVolControlList()
{
	for(auto iter = m_vecVolControl.begin(); iter != m_vecVolControl.end(); ++iter) {
		AFQVolControl* vol = (*iter);
		if(!vol)
			continue;
		vol->close();
		delete vol;
	}
	m_vecVolControl.clear();
}

void AFSceneContext::ClearContext()
{
	_Clear();

	// !! locate obs - window-basic-main.cpp 4959 line - [34ef67e]
	obs_enum_scenes(AFSourceUtil::RemoveSimpleCallback, nullptr);
	obs_enum_sources(AFSourceUtil::RemoveSimpleCallback, nullptr);
	// !!
}

OBSSource AFSceneContext::GetProgramSource()
{
	return OBSGetStrongRef(programScene);
}
OBSScene AFSceneContext::GetCurrentScene()
{
	return currentScene.load();
}

OBSWeakSource AFSceneContext::GetProgramScene()
{
	return programScene;
}
OBSWeakSource AFSceneContext::GetLastScene()
{
	return lastScene;
}
OBSWeakSource AFSceneContext::GetSwapScene()
{
	return swapScene;
}

void AFSceneContext::SetProgramScene(obs_source_t* obsSource)
{
	programScene = OBSGetWeakRef(obsSource);
}
void AFSceneContext::SetCurrentScene(obs_scene_t* scene)
{
	currentScene.store(scene);
}
void AFSceneContext::SetLastScene(obs_source_t* obsSource)
{
	lastScene = OBSGetWeakRef(obsSource);
}
void AFSceneContext::SetSwapScene(OBSWeakSource obsSource)
{
	swapScene = obsSource;
}

bool AFSceneContext::IsMustInSizePreview(obs_source_t* source)
{
	if(!source)
		return false;

	std::string id = obs_source_get_id(source);

	if (AFSourceUtil::IsSoopMediaSource(source))
		return true;

	if (0 == id.compare("soop_aimanager_source"))
		return true;

	if (0 == id.compare("painter_source"))
		return true;

	return false;
}

void AFSceneContext::SetCurTransition(OBSSource source)
{
	m_curTransition = source;
}

void AFSceneContext::SetCurDuration(int duration)
{
	m_transDuration = duration;
}

void AFSceneContext::AddTransition(OBSSource source)
{
	m_obsTransitions.emplace_back(source);
}

void AFSceneContext::RemoveTransition(OBSSource source)
{
	std::vector<OBSSource>::iterator it = m_obsTransitions.begin();
	for(; it != m_obsTransitions.end(); ++it) {
		if((*it) == source)
			m_obsTransitions.erase(it);
	}
}

void AFSceneContext::InitDefaultTransition()
{
	size_t idx = 0;
	const char* id;

	/* automatically add transitions that have no configuration (things
	* such as cut/fade/etc) */
	while(obs_enum_transition_types(idx++, &id)) {
		if(!obs_is_source_configurable(id)) {
			const char* name = obs_source_get_display_name(id);

			OBSSourceAutoRelease tr = obs_source_create_private(id, name, NULL);
			InitTransition(tr);
			m_obsTransitions.emplace_back(tr);

			if(strcmp(id, "fade_transition") == 0)
				m_pFadeTransition = tr;
			else if(strcmp(id, "cut_transition") == 0)
				m_pCutTransition = tr;
		}
	}

	m_curTransition = m_obsTransitions.at(0);
}
void AFSceneContext::TransitionToScene(OBSSource source, bool force,
									   bool quickTransition,
									   int quickDuration, bool black,
									   bool manual)
{
	obs_scene_t* scene = obs_scene_from_source(source);
	if(!scene)
		return;

	auto& stateApp = STATEAPP;
	//
	bool usingPreviewProgram = stateApp.IsPreviewProgramMode();
	bool sceneDuplicationMode = stateApp.GetSceneDuplicationMode();
	bool editPropertiesMode = stateApp.GetEditPropertiesMode();
	bool swapScenesMode = stateApp.GetSwapScenesMode();

	if(usingPreviewProgram) {
		//	if (!tBarActive){
			lastProgramScene = programScene;
		//}
		programScene = OBSGetWeakRef(source);

		if(!force && !black) {
			OBSSource lastScene = OBSGetStrongRef(lastProgramScene);

			if(!sceneDuplicationMode && lastScene == source)
				return;

            if(swapScenesMode && lastScene && lastScene != GetCurrentSceneSource())
                swapScene = lastProgramScene;
		}
	}

	if(usingPreviewProgram && sceneDuplicationMode) {
		scene = obs_scene_duplicate(scene, obs_source_get_name(obs_scene_get_source(scene)),
									editPropertiesMode ? OBS_SCENE_DUP_PRIVATE_COPY
													   : OBS_SCENE_DUP_PRIVATE_REFS);
		source = obs_scene_get_source(scene);
	}

	OBSSourceAutoRelease transition = obs_get_output_source(0);
	if(!transition) {
		if(usingPreviewProgram && sceneDuplicationMode)
			obs_scene_release(scene);
		return;
	}

	float t = obs_transition_get_time(transition);
	bool stillTransitioning = t < 1.0f && t > 0.0f;

	// If actively transitioning, block new transitions from starting
	if(usingPreviewProgram && stillTransitioning)
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

	if(force) {
		obs_transition_set(transition, source);
		MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	} else {

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

		if(quickTransition)
			duration = quickDuration;
		else
			duration = GetCurDuraition();

		enum obs_transition_mode mode = manual ? OBS_TRANSITION_MODE_MANUAL : OBS_TRANSITION_MODE_AUTO;

		MAINFRAME->EnableTransitionState(false);
		//EnableTransitionWidgets(false);

		bool success = obs_transition_start(transition, mode, duration, source);

		//if (!success)
		//	TransitionFullyStopped();
	}

cleanup:
	if(usingPreviewProgram && sceneDuplicationMode)
		obs_scene_release(scene);
}

int AFSceneContext::GetOverrideTransitionDuration(OBSSource source)
{
	if(!source)
		return 300;

	OBSDataAutoRelease data = obs_source_get_private_settings(source);
	obs_data_set_default_int(data, "transition_duration", 300);

	return (int)obs_data_get_int(data, "transition_duration");
}
void AFSceneContext::OverrideTransition(OBSSource transition)
{
	OBSSourceAutoRelease oldTransition = obs_get_output_source(0);

	if(transition != oldTransition) {
		obs_transition_swap_begin(transition, oldTransition);
		obs_set_output_source(0, transition);
		obs_transition_swap_end(transition, oldTransition);
	}
}

void AFSceneContext::SetTransition(OBSSource transition)
{
	OBSSourceAutoRelease oldTransition = obs_get_output_source(0);

	if(oldTransition && transition) {
		obs_transition_swap_begin(transition, oldTransition);
		obs_set_output_source(0, transition);
		obs_transition_swap_end(transition, oldTransition);
	} else {
		obs_set_output_source(0, transition);
	}
}

void AFSceneContext::InitTransition(obs_source_t* transition)
{
	auto onTransitionStop = [](void* data, calldata_t*) {
		if(auto main = DYNAMIC_COMPOSIT) {
			QMetaObject::invokeMethod(main, "TransitionStopped", Qt::QueuedConnection);
		}
	};

	auto onTransitionFullStop = [](void* data, calldata_t*) {
		//OBSBasic* window = (OBSBasic*)data;
		//QMetaObject::invokeMethod(window, "TransitionFullyStopped",
		//	Qt::QueuedConnection);
	};

	signal_handler_t* handler = obs_source_get_signal_handler(transition);
	signal_handler_connect(handler, "transition_video_stop", onTransitionStop, this);
	signal_handler_connect(handler, "transition_stop", onTransitionFullStop, this);
}

obs_source_t* AFSceneContext::FindTransition(const char* name)
{
	std::vector<OBSSource> transitions = GetTransitions();

	for(int i = 0; i < transitions.size(); i++) {
		OBSSource tr = transitions.at(i);
		if(!tr)
			continue;

		const char* trName = obs_source_get_name(tr);
		if(strcmp(trName, name) == 0)
			return tr;
	}

	return nullptr;
}

AFQSourceListView* AFSceneContext::GetSourceListViewPtr()
{
	return m_pSourceListViewPtr;
}

void AFSceneContext::SetSourceListViewPtr(AFQSourceListView* listview)
{
	m_pSourceListViewPtr = listview;
}

SceneItemVector& AFSceneContext::GetSceneItemVector()
{
	return m_vecSceneItem;
}

size_t AFSceneContext::GetSceneItemSize()
{
	return m_vecSceneItem.size();
}

AFQSceneListItem* AFSceneContext::GetCurSelectedSceneItem()
{
	return m_clickedSceneItem.load();
}

OBSSceneItem AFSceneContext::GetCurrentOBSSceneItem(int idx_)
{
	if(!m_pSourceListViewPtr)
		return nullptr;

	int idx = idx_;
	if(idx_ == -1)
		idx = m_pSourceListViewPtr->GetTopSelectedSourceItem();

	return m_pSourceListViewPtr->Get(idx);
}

void AFSceneContext::SetCurSelectedSceneItem(AFQSceneListItem* sceneItem)
{
	m_clickedSceneItem.store(sceneItem);
}

void AFSceneContext::AddSceneItem(AFQSceneListItem* sceneItem)
{
	m_vecSceneItem.emplace_back(sceneItem);
}

void AFSceneContext::SwapSceneItem(int from, int dest)
{
	if(from == dest)
		return;

	int vecSize = m_vecSceneItem.size();

	if(from >= 0 && from < vecSize && dest >= 0 && dest < vecSize) {
		if(from < dest) {
			std::rotate(m_vecSceneItem.begin() + from, m_vecSceneItem.begin() + from + 1, m_vecSceneItem.begin() + dest + 1);
		} else if(from > dest) {
			std::rotate(m_vecSceneItem.begin() + dest, m_vecSceneItem.begin() + from, m_vecSceneItem.begin() + from + 1);
		}
	} else {
		return;
	}
}

int AFSceneContext::GetFavoriteSceneCount()
{
	int nFavoriteSceneCount = 0;
	SceneItemVector& sceneItems = GetSceneItemVector();
	if(sceneItems.empty() == false) {
		size_t sceneCount = sceneItems.size();
		for(size_t i = 0; i < sceneCount; i++) {
			OBSScene scene = sceneItems.at(i)->GetScene();

			obs_source_t* source = obs_scene_get_source(scene);
			obs_data_t* scene_data = obs_source_get_settings(source);

			int favorite_scene = obs_data_get_int(scene_data, "favorite_scene");
			if(1 == favorite_scene)
				nFavoriteSceneCount++;
		}
	}

	return nFavoriteSceneCount;
}


const int AFSceneContext::GetFavoriteSceneMaxCount()
{
	const int maxFavoriteSceneCount = 5;
	return maxFavoriteSceneCount;
}

VolControlVector& AFSceneContext::GetVolControlVector()
{
	return m_vecVolControl;
}

void AFSceneContext::SetMixerCopyFilter(obs_source_t* source)
{
	m_mixerCopyFiltersSource = obs_source_get_weak_source(source);
}

OBSSourceAutoRelease AFSceneContext::GetMixerCopyFilter()
{
	return obs_weak_source_get_source(m_mixerCopyFiltersSource);
}

static bool resize_soop_source(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
	if(!item) return true;

	obs_source_t* source = obs_sceneitem_get_source(item);

	if(!source) {
		return true;
	}

	auto* p = reinterpret_cast<vec2*>(param);

	std::string id = obs_source_get_id(source);


	obs_video_info ovi;
	obs_get_video_info(&ovi);

	if (0 == id.compare("painter_source")) {

		OBSDataAutoRelease settings = obs_source_get_settings(source);

		obs_data_set_int(settings, "width", ovi.base_width);
		obs_data_set_int(settings, "height", ovi.base_height);
		obs_source_update(source, settings);

		obs_transform_info itemInfo;
		obs_sceneitem_get_info(item, &itemInfo);
		vec2_set(&itemInfo.bounds, p->x, p->y);
		obs_sceneitem_set_info(item, &itemInfo);
	}
	
	if (AFSourceUtil::IsSoopMediaSource(source) || 0 == id.compare("soop_aimanager_source"))
	{
		obs_transform_info itemInfo;
		obs_sceneitem_get_info(item, &itemInfo);

		OBSDataAutoRelease settings = obs_source_get_settings(source);
		const float originW = obs_data_get_int(settings, "origin_width");
		const float originH = obs_data_get_int(settings, "origin_height");

		if (originW > 0.0f && originH > 0.0f) {
			const float availableW = (float)ovi.base_width - itemInfo.pos.x;
			const float availableH = (float)ovi.base_height - itemInfo.pos.y;

			const float scaledW = itemInfo.scale.x * originW;
			const float scaledH = itemInfo.scale.y * originH;

			float clampScale = 1.0f;

			if (scaledW > availableW && scaledW > 0.0f)
				clampScale = std::min(clampScale, availableW / scaledW);

			if (scaledH > availableH && scaledH > 0.0f)
				clampScale = std::min(clampScale, availableH / scaledH);

			if (clampScale < 1.0f) {
				itemInfo.scale.x *= clampScale;
				itemInfo.scale.y *= clampScale;
				obs_sceneitem_set_info(item, &itemInfo);
			}
		}
	}

	return true;
};

static bool resize_soop_source_in_scene(void* param, obs_source_t* source)
{
	vec2* size = static_cast<vec2*>(param);
	if (!size)
		return true;

	obs_scene_t* scene = obs_scene_from_source(source);
	if (!scene)
		return true;

	obs_scene_enum_items(scene, resize_soop_source, size);
	return true;
}

void AFSceneContext::UpdateVideoSize(int width, int height)
{
	vec2 size;
	vec2_set(&size, (float)width, (float)height);

	obs_enum_scenes(resize_soop_source_in_scene, &size);
}
//
void AFSceneContext::_Clear()
{
	lastScene = nullptr;
	swapScene = nullptr;
	programScene = nullptr;
	lastProgramScene = nullptr;
	m_prevFTBSource = nullptr;

	m_obsCopyFiltersSource = nullptr;
	m_copyFilter = nullptr;

	m_curTransition = nullptr;
	m_transDuration = 300;

	//m_pSourceListViewPtr = nullptr;
}

namespace AFSceneUtil
{
	OBSSource CnvtToOBSSource(OBSScene scene)
	{
		return OBSSource(obs_scene_get_source(scene));
	}
	OBSScene CnvtToOBSScene(OBSSource source)
	{
		return OBSScene(obs_scene_from_source(source));
	}

	bool SceneItemHasVideo(obs_sceneitem_t* item)
	{
		obs_source_t* source = obs_sceneitem_get_source(item);
		uint32_t flags = obs_source_get_output_flags(source);
		return (flags & OBS_SOURCE_VIDEO) != 0;
	}
	bool IsCropEnabled(const obs_sceneitem_crop* crop)
	{
		return crop->left > 0 || crop->top > 0 ||
			crop->right > 0 || crop->bottom > 0;
	}

	obs_source_t* CreateOBSScene(const char* name)
	{
		OBSSceneAutoRelease scene = obs_scene_create(name);
		obs_source_t* source = obs_scene_get_source(scene);

		return source;
	}
}