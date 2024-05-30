#include "CSceneContext.h"

#include "CoreModel/Source/CSource.h"
#include "CoreModel/Scene/CScene.h"

#include "Blocks/SceneSourceDock/CSourceListView.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"
#include "Application/CApplication.h"

AFSceneContext::~AFSceneContext()
{
	m_obsCurrScene = nullptr;
}

void AFSceneContext::AddSource(void* _data, obs_scene* scene)
{
    AddSourceData* data = (AddSourceData*)_data;
    obs_sceneitem_t* sceneitem;

    sceneitem = obs_scene_add(scene, data->source);

    if (data->transform != nullptr)
        obs_sceneitem_set_info(sceneitem, data->transform);
    if (data->crop != nullptr)
        obs_sceneitem_set_crop(sceneitem, data->crop);
    if (data->blend_method != nullptr)
        obs_sceneitem_set_blending_method(sceneitem,
            *data->blend_method);
    if (data->blend_mode != nullptr)
        obs_sceneitem_set_blending_mode(sceneitem, *data->blend_mode);

    obs_sceneitem_set_visible(sceneitem, data->visible);
}

void AFSceneContext::InitContext()
{    
	ClearContext();
}

void AFSceneContext::ClearContext()
{
	_Clear();

	obs_enum_scenes(AFSourceUtil::RemoveSimpleCallback, nullptr);
	obs_enum_sources(AFSourceUtil::RemoveSimpleCallback, nullptr);
}

void AFSceneContext::SetCurrOBSScene(obs_scene_t* scene)
{
    m_obsCurrScene.store(scene);
}

void AFSceneContext::SetLastOBSScene(obs_source_t* obsSource)
{
    m_obsLastScene = OBSGetWeakRef(obsSource);
}

void AFSceneContext::SetSwapOBSScene(obs_source_t* obsSource)
{
    m_obsSwapScene = OBSGetWeakRef(obsSource);
}

void AFSceneContext::SetSwapOBSScene(OBSWeakSource obsSource)
{
    m_obsSwapScene = obsSource;
}

void AFSceneContext::SetProgramOBSScene(obs_source_t* obsSource)
{
    m_obsProgramScene = OBSGetWeakRef(obsSource);
}

void AFSceneContext::SetLastProgramOBSScene(obs_source_t* obsSource)
{
    m_obsLastProgramScene = OBSGetWeakRef(obsSource);
}

void AFSceneContext::SetLastProgramOBSScene(OBSWeakSource obsSource)
{
    m_obsLastProgramScene = obsSource;
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
	for (; it != m_obsTransitions.end(); ++it) {
		if ((*it) == source)
			m_obsTransitions.erase(it);
	}
}

void AFSceneContext::_Clear()
{
	m_obsLastScene = nullptr;
	m_obsSwapScene = nullptr;
	m_obsProgramScene = nullptr;
	m_obsLastProgramScene = nullptr;
    m_prevFTBSource = nullptr;

    m_obsCopyFiltersSource = nullptr;
    m_copyFilter = nullptr;

	m_curTransition = nullptr;
	m_transDuration = 300;

	//m_sourceListViewPtr = nullptr;
}

void AFSceneContext::InitDefaultTransition()
{
    size_t idx = 0;
    const char* id;

	/* automatically add transitions that have no configuration (things
	* such as cut/fade/etc) */
	while (obs_enum_transition_types(idx++, &id)) {
		if (!obs_is_source_configurable(id)) {
			const char* name = obs_source_get_display_name(id);

			OBSSourceAutoRelease tr =
				obs_source_create_private(id, name, NULL);
			InitTransition(tr);
			m_obsTransitions.emplace_back(tr);

			if (strcmp(id, "fade_transition") == 0)
				m_fadeTransition = tr;
			else if (strcmp(id, "cut_transition") == 0)
				m_cutTransition = tr;
		}
	}

	m_curTransition = m_obsTransitions.at(0);
}

void AFSceneContext::InitTransition(obs_source_t* transition)
{
	auto onTransitionStop = [](void* data, calldata_t*) {
        auto mainView = App()->GetMainView();
        if (auto main = mainView->GetMainWindow()) {
            QMetaObject::invokeMethod(main, "TransitionStopped",
                                      Qt::QueuedConnection);
        }
	};

	auto onTransitionFullStop = [](void* data, calldata_t*) {
		//OBSBasic* window = (OBSBasic*)data;
		//QMetaObject::invokeMethod(window, "TransitionFullyStopped",
		//	Qt::QueuedConnection);
	};

	signal_handler_t* handler = obs_source_get_signal_handler(transition);
	signal_handler_connect(handler, "transition_video_stop",
		onTransitionStop, this);
	signal_handler_connect(handler, "transition_stop", onTransitionFullStop,
		this);
}


obs_source_t* AFSceneContext::FindTransition(const char* name)
{
	std::vector<OBSSource> transitions = GetTransitions();

	for (int i = 0; i < transitions.size(); i++) {
		OBSSource tr = transitions.at(i);
		if (!tr)
			continue;

		const char* trName = obs_source_get_name(tr);
		if (strcmp(trName, name) == 0)
			return tr;
	}

	return nullptr;
}

AFQSourceListView* AFSceneContext::GetSourceListViewPtr()
{
	return m_sourceListViewPtr;
}

void AFSceneContext::SetSourceListViewPtr(AFQSourceListView* listview)
{
	m_sourceListViewPtr = listview;
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
	if (from == dest)
		return;

	int vecSize = m_vecSceneItem.size();

	if (from >= 0 && from < vecSize && dest >= 0 && dest < vecSize) {
		if (from < dest) {
			std::rotate(m_vecSceneItem.begin() + from, m_vecSceneItem.begin() + from + 1, m_vecSceneItem.begin() + dest + 1);
		}
		else if (from > dest) {
			std::rotate(m_vecSceneItem.begin() + dest, m_vecSceneItem.begin() + from, m_vecSceneItem.begin() + from + 1);
		}
	}
	else {
		return;
	}
}