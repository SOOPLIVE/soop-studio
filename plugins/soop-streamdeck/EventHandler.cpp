#pragma once

#include <obs-module.h>

#include "EventHandler.h"
#include "../plugins/obs-websocket/src/eventhandler/types/EventSubscription.h"

struct LNBMenuInfo {
	std::string platform;
	int menuId;			// 
	int menuType;			// popup : 1, action : 0
	std::string menuLocale;
};

static const LNBMenuInfo kLNBMenuList[] = {
	{"soop_kr", 3,   1, "Chat"},					// soop kr chat
	{"soop_kr", 13,  1, "Mission" },				// soop kr mission
	{"soop_kr", 14,  1, "Vote" },					// soop kr vote
	{"soop_kr", 15,  1, "Extension" },				// soop kr extension
	{"soop_kr", 17,  1, "AquaRemoteControl" },		// soop kr aqua remote control
	{"soop_kr", 99,  0, "SaveVod" },				// soop kr save vod
};

template<typename T> T* GetCalldataPointer(const calldata_t* data, const char* name)
{
	void* ptr = nullptr;
	calldata_get_ptr(data, name, &ptr);
	return static_cast<T*>(ptr);
}

void EventHandler::OnFrontendEvent(enum obs_frontend_event event, void* ptr)
{
	auto eventHandler = static_cast<EventHandler*>(ptr);
	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTING:
	{
		eventHandler->HandleStreamStateChanged(OBS_WEBSOCKET_OUTPUT_STARTING);
	}
	break;
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		eventHandler->HandleStreamStateChanged(OBS_WEBSOCKET_OUTPUT_STARTED);
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
	{
		eventHandler->HandleStreamStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPING);
	}
	break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		eventHandler->HandleStreamStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPED);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STARTING:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_STARTING);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_STARTED);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPING:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPING);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_STOPPED);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_PAUSED:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_PAUSED);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED:
		eventHandler->HandleRecordStateChanged(OBS_WEBSOCKET_OUTPUT_RESUMED);
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING:
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED:
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED:
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPING:
		break;
	case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED:
		break;
	case OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED:
		break;
	case OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED:
		break;
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
		eventHandler->HandleCurrentPreviewSceneChanged();
		break;
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		eventHandler->HandleCurrentProgramSceneChanged();
		break;
	case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
		eventHandler->HandleSceneListChanged();
		break;
	case OBS_FRONTEND_EVENT_TRANSITION_CHANGED:
		break;
	case OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED:
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		break;

	case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
		eventHandler->HandleStudioModeStateChanged(true);
		break;
	case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
		eventHandler->HandleStudioModeStateChanged(false);
		break;

	default:
		break;
	}
}


void EventHandler::OnSoopFrontendEvent(enum soop_frontend_type event, void* data, void* ptr)
{
	EventHandler* eventHandler = static_cast<EventHandler*>(ptr);

	switch (event) {
	case SOOP_FRONTEND_EVENT_TOOGLE_MAIN_MIC_ON:
		eventHandler->HandleStudioMainMicStateChanged(true);
		break;
	case SOOP_FRONTEND_EVENT_TOOGLE_MAIN_MIC_OFF:
		eventHandler->HandleStudioMainMicStateChanged(false);
		break;
	case SOOP_FRONTEND_EVENT_TOOGLE_MAIN_VOLUME_ON:
		eventHandler->HandleStudioMainVolumeStateChanged(true);
		break;
	case SOOP_FRONTEND_EVENT_TOOGLE_MAIN_VOLUME_OFF:
		eventHandler->HandleStudioMainVolumeStateChanged(false);
		break;

	case SOOP_FRONTEND_EVENT_STATE_SIDEBAR_ON:
		eventHandler->HandleStudioSOOPChannelSidebarStateChanged(true);
		break;

	case SOOP_FRONTEND_EVENT_STATE_SIDEBAR_OFF:
		eventHandler->HandleStudioSOOPChannelSidebarStateChanged(false);
		break;

	case SOOP_FRONTEND_EVENT_STATE_LNBMENU_ON:
	{
		int popuId = *static_cast<int*>(data);
		eventHandler->HandleStudioSOOPLNBMenuStateChanged(popuId, true);
	}
	break;

	case SOOP_FRONTEND_EVENT_STATE_LNBMENU_OFF:
	{
		int menuType = *static_cast<int*>(data);
		eventHandler->HandleStudioSOOPLNBMenuStateChanged(menuType, false);
	}
	break;

	default:
		break;
	}
}

EventHandler::EventHandler()
{
	obs_frontend_add_event_callback(OnFrontendEvent, this);
	soop_frontend_add_callback(OnSoopFrontendEvent, this);

	signal_handler_t* coreSignalHandler = obs_get_signal_handler();
	if (coreSignalHandler) {
		signal_handler_connect(coreSignalHandler, "source_create", SourceCreatedMultiHandler, this);
		signal_handler_connect(coreSignalHandler, "source_destroy", SourceDestroyedMultiHandler, this);
		//signal_handler_connect(coreSignalHandler, "source_remove", SourceRemovedMultiHandler, this);
		//signal_handler_connect(coreSignalHandler, "source_rename", SourceRenamedMultiHandler, this);
	}
	else {
		blog(LOG_ERROR, "[EventHandler::EventHandler] Unable to get libobs signal handler!");
	}
}

EventHandler::~EventHandler()
{
}

void EventHandler::SetWebSocketCallback(EventHandler::BroadcastCallback cb)
{
	_webSocketCallback = cb;
}

void EventHandler::SendEventToStreamDeck(uint64_t requiredIntent, std::string eventType, nlohmann::json eventData, uint8_t rpcVersion)
{
	if (!_webSocketCallback)
		return;

	_webSocketCallback(requiredIntent, eventType, eventData, rpcVersion);
}

void EventHandler::ParseStreamDeckEventJson(std::string json)
{
	nlohmann::json jsonMessage;
	try {
		jsonMessage = nlohmann::json::parse(json);
	}
	catch (const std::exception& e) {
		blog(LOG_ERROR, "[StreamDeck Plugin] Failed to parse JSON: %s", e.what());
		return;
	}

	int opCode = -1;
	nlohmann::json data;

	try {
		opCode = jsonMessage.at("op").get<int>();
		data = jsonMessage.at("d");
	}
	catch (const std::exception& e) {
		blog(LOG_ERROR, "[StreamDeck Plugin] Missing 'op' or 'd': %s", e.what());
		return;
	}

	if (opCode != 6) {
		blog(LOG_WARNING, "[StreamDeck Plugin] Unsupported op code: %d", opCode);
		return;
	}

	if (!data.is_object()) {
		blog(LOG_ERROR, "[StreamDeck Plugin] 'd' is not a valid object");
		return;
	}

	std::string requestId;
	std::string requiredIntent;
	std::string eventType;

	try {
		requestId = data.at("requestId").get<std::string>();
		requiredIntent = data.at("requiredIntent").get<std::string>();
		eventType = data.at("eventType").get<std::string>();
	}
	catch (const std::exception& e) {
		blog(LOG_ERROR, "[StreamDeck Plugin] Missing requiredIntent or eventType: %s", e.what());
		return;
	}

	if (0 == requiredIntent.compare("general"))
	{

	}
	else if (0 == requiredIntent.compare("config"))
	{

	}
	else if (0 == requiredIntent.compare("scenes"))
	{
		if (0 == eventType.compare("requestSceneList"))
		{
			HandleSceneListChanged();
		}
		else if (0 == eventType.compare("changeScene"))
		{
			std::string scene;
			try {
				scene = data.at("params").at("scene").get<std::string>();
			}
			catch (const std::exception& e) {
				blog(LOG_ERROR, "[StreamDeck Plugin]  Missing 'params.scene': %s", e.what());
				return;
			}

			OBSSourceAutoRelease source =
				obs_get_source_by_name(scene.c_str());

			if (!source) {
				blog(LOG_WARNING, "[StreamDeck Plugin] scene not found: %s", scene.c_str());
				HandleCurrentProgramSceneChanged();
				return;
			}

			obs_frontend_set_current_scene(source);
		}
		else if (0 == eventType.compare("requestCurrentScene")) {
			HandleCurrentProgramSceneChanged();
		}
	}
	else if (0 == requiredIntent.compare("inputs"))
	{
		if (0 == eventType.compare("toggleMicAcitve"))
		{
			soop_frontend_toogle_main_mic();
		}
		else if (0 == eventType.compare("toggleVolAcitve"))
		{
			soop_frontend_toogle_main_vol();
		}
		else if (0 == eventType.compare("requestAudioState"))
		{
			HandleStudioMainMicStateChanged(soop_frontend_main_mic_state());
			HandleStudioMainVolumeStateChanged(soop_frontend_main_vol_state());
		}
	}
	else if (0 == requiredIntent.compare("transitions"))
	{

	}
	else if (0 == requiredIntent.compare("filters"))
	{

	}
	else if (0 == requiredIntent.compare("outputs"))
	{
		if (0 == eventType.compare("toggleStream"))
		{
			if (obs_frontend_streaming_active())
			{
				obs_frontend_streaming_stop();
			}
			else {
				obs_frontend_streaming_start();
			}
		}
		else if (0 == eventType.compare("toggleRecord"))
		{
			if (obs_frontend_recording_active())
			{
				obs_frontend_recording_stop();
			}
			else {
				obs_frontend_recording_start();
			}
		}
		else if (0 == eventType.compare("requestOutputState")) 
		{
			HandleRecordStateChanged(obs_frontend_recording_active() ? OBS_WEBSOCKET_OUTPUT_STARTED : OBS_WEBSOCKET_OUTPUT_STOPPED);
			HandleStreamStateChanged(obs_frontend_streaming_active() ? OBS_WEBSOCKET_OUTPUT_STARTED : OBS_WEBSOCKET_OUTPUT_STOPPED);
		}
	}
	else if (0 == requiredIntent.compare("sceneItems"))
	{
		if (0 == eventType.compare("toogleSourceVisible")) {

			std::string source;
			try {
				source = data.at("params").at("source").get<std::string>();
			}
			catch (const std::exception& e) {
				blog(LOG_ERROR, "[StreamDeck Plugin] Missing 'params.source': %s", e.what());
				return;
			}

			obs_frontend_source_list scenes = { 0 };
			obs_frontend_get_scenes(&scenes);

			for (size_t i = 0; i < scenes.sources.num; ++i) {
				obs_source_t* sceneSource = scenes.sources.array[i];
				if (!sceneSource)
					continue;

				obs_scene_t* scene = obs_scene_from_source(sceneSource);
				if (!scene) {
					continue;
				}

				obs_sceneitem_t* item = obs_scene_find_source(scene, source.c_str()); 
				if (item) {
					bool visible = obs_sceneitem_visible(item);
					obs_sceneitem_set_visible(item, !visible);
					break;
				}
			}
			obs_frontend_source_list_free(&scenes);
		}
		else if (0 == eventType.compare("GetSourceList"))
		{
			std::string sceneName;
			try {
				sceneName = data.at("params").at("scene").get<std::string>();
			}
			catch (const std::exception& e) {
				blog(LOG_ERROR, "[StreamDeck Plugin]  Missing 'params.scene': %s", e.what());
				return;
			}

			OBSSceneAutoRelease scene = obs_get_scene_by_name(sceneName.c_str());
			HandleSourceListChanged(scene, false);
		}
		else if (0 == eventType.compare("GetSourceVisibleState"))
		{
			std::string sceneName;
			std::string sourceName;
			try {
				sceneName = data.at("params").at("scene").get<std::string>();
				sourceName = data.at("params").at("source").get<std::string>();

			}
			catch (const std::exception& e) {
				blog(LOG_ERROR, "[StreamDeck Plugin]  Missing 'params.scene': %s", e.what());
				return;
			}

			nlohmann::json eventData;
			eventData["sceneName"] = sceneName;
			eventData["sourceName"] = sourceName;
			eventData["sceneItemEnabled"] = false;

			OBSSceneAutoRelease scene = obs_get_scene_by_name(sceneName.c_str());
			if (!scene) {
				blog(LOG_ERROR, "[StreamDeck Plugin] Scene not found: %s", sceneName.c_str());
				SendEventToStreamDeck(EventSubscription::SceneItems, "SceneItemEnableStateChanged", eventData);
				return;
			}

			OBSSourceAutoRelease source = obs_get_source_by_name(sourceName.c_str());
			if (!source) {
				blog(LOG_ERROR, "[StreamDeck Plugin] Source not found: %s", sourceName.c_str());
				SendEventToStreamDeck(EventSubscription::SceneItems, "SceneItemEnableStateChanged", eventData);
				return;
			}

			obs_sceneitem_t* sceneItem = obs_scene_sceneitem_from_source(scene, source);
			if (!sceneItem) {
				blog(LOG_ERROR, "[StreamDeck Plugin] Scene item not found for source '%s' in scene '%s'", sourceName.c_str(), sceneName.c_str());
				SendEventToStreamDeck(EventSubscription::SceneItems, "SceneItemEnableStateChanged", eventData);
				return;
			}

			eventData["sceneName"] = obs_source_get_name(obs_scene_get_source(scene));
			eventData["sourceName"] = obs_source_get_name(source);
			eventData["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
			eventData["sceneItemEnabled"] = obs_sceneitem_visible(sceneItem);
			SendEventToStreamDeck(EventSubscription::SceneItems, "SceneItemEnableStateChanged", eventData);

		}
	}
	else if (0 == requiredIntent.compare("mediaInputs"))
	{

	}
	else if (0 == requiredIntent.compare("vendors"))
	{

	}
	else if (0 == requiredIntent.compare("ui"))
	{
		if (0 == eventType.compare("toggleStudioMode")) {
			bool isActiveStudioMode = obs_frontend_preview_program_mode_active();
			obs_frontend_set_preview_program_mode(!isActiveStudioMode);
		}
		else if (0 == eventType.compare("TransStudioModeScene")) {
			obs_frontend_preview_program_trigger_transition();
		}
		else if (0 == eventType.compare("toggleSidebar")) {
			soop_frontend_toogle_channel_sidebar();
		}
		else if (0 == eventType.compare("toggleLNBMenu")) {

			int menuType;
			int menuId;
			try {
				std::string menuString = data.at("params").at("menuType").get<std::string>();
				menuType = std::stoi(menuString.c_str());

				std::string popupString = data.at("params").at("menuId").get<std::string>();
				menuId = std::stoi(popupString.c_str());

			}
			catch (const std::exception& e) {
				blog(LOG_ERROR, "[StreamDeck Plugin]  Missing 'params.scene': %s", e.what());
				return;
			}

			soop_frontend_toogle_lnbmenu(menuId);
		}
		else if (0 == eventType.compare("requestLNBMenuList")) {

			nlohmann::json eventData;
			nlohmann::json menuArray = nlohmann::json::array();
			for (const auto& item : kLNBMenuList) {
				nlohmann::json obj;
				obj["platform"] = item.platform;
				obj["menuId"] = item.menuId;
				obj["menuType"] = item.menuType;
				obj["menuName"] = obs_module_text(item.menuLocale.c_str());

				menuArray.push_back(obj);
			}
			eventData["menus"] = menuArray;

			SendEventToStreamDeck(EventSubscription::Ui, "LNBMenuListChanged", eventData);
		}
		else if (0 == eventType.compare("requestLNBMenuState")) {
			int menuType;
			int menuId;
			std::string platform;
			try {
				std::string menuString = data.at("params").at("menuType").get<std::string>();
				menuType = std::stoi(menuString.c_str());

				std::string popupString = data.at("params").at("menuId").get<std::string>();
				menuId = std::stoi(popupString.c_str());

				platform = data.at("params").at("platform").get<std::string>();
			}
			catch (const std::exception& e) {
				blog(LOG_ERROR, "[StreamDeck Plugin]  Missing 'params.scene': %s", e.what());
				return;
			}

			if (1 == menuType)
			{
				if (0 == menuId)
				{
					int count = sizeof(kLNBMenuList) / sizeof(kLNBMenuList[0]);

					for (int i = 0; i < count; ++i) {
						const LNBMenuInfo& item = kLNBMenuList[i];
						if (0 == item.platform.compare("soop_kr")) {
							bool state = soop_frontend_get_lnbmenu_state(item.menuId);
							HandleStudioSOOPLNBMenuStateChanged(item.menuId, state);
						}
					}
				}
				else
				{
					bool state = soop_frontend_get_lnbmenu_state(menuId);
					HandleStudioSOOPLNBMenuStateChanged(menuId, state);
				}
			}
		}
		else if (0 == eventType.compare("requestStudioModeState")) {
			bool isActiveStudioMode = obs_frontend_preview_program_mode_active();
			HandleStudioModeStateChanged(isActiveStudioMode);
		}
	}
	else if (0 == requiredIntent.compare("inputVolumeMeters"))
	{

	}
	else if (0 == requiredIntent.compare("inputActiveStateChanged"))
	{

	}
	else if (0 == requiredIntent.compare("inputshowStateChanged"))
	{

	}
	else if (0 == requiredIntent.compare("sceneItemTransfromChanged"))
	{

	}
}

void EventHandler::ConnectSourceSignals(obs_source_t* source)
{
	if (!source || obs_source_removed(source))
		return;

	DisconnectSourceSignals(source);

	signal_handler_t* sh = obs_source_get_signal_handler(source);

	obs_source_type sourceType = obs_source_get_type(source);

	if (sourceType == OBS_SOURCE_TYPE_SCENE) {
		signal_handler_connect(sh, "item_add", HandleSceneItemCreated, this);
		signal_handler_connect(sh, "item_remove", HandleSceneItemRemoved, this);
		signal_handler_connect(sh, "reorder", HandleSceneItemListReindexed, this);
		signal_handler_connect(sh, "item_visible", HandleSceneItemEnableStateChanged, this);
		//signal_handler_connect(sh, "item_locked", HandleSceneItemLockStateChanged, this);
		//signal_handler_connect(sh, "item_select", HandleSceneItemSelected, this);
		//signal_handler_connect(sh, "item_transform", HandleSceneItemTransformChanged, this);
	}

}

void EventHandler::DisconnectSourceSignals(obs_source_t* source)
{
	if (!source)
		return;

	signal_handler_t* sh = obs_source_get_signal_handler(source);

	obs_source_type sourceType = obs_source_get_type(source);

	if (sourceType == OBS_SOURCE_TYPE_SCENE) {
		signal_handler_disconnect(sh, "item_add", HandleSceneItemCreated, this);
		signal_handler_disconnect(sh, "item_remove", HandleSceneItemRemoved, this);
		signal_handler_disconnect(sh, "reorder", HandleSceneItemListReindexed, this);
		signal_handler_disconnect(sh, "item_visible", HandleSceneItemEnableStateChanged, this);
		//signal_handler_disconnect(sh, "item_locked", HandleSceneItemLockStateChanged, this);
		//signal_handler_disconnect(sh, "item_select", HandleSceneItemSelected, this);
		//signal_handler_disconnect(sh, "item_transform", HandleSceneItemTransformChanged, this);
	}
}

void EventHandler::SourceCreatedMultiHandler(void* param, calldata_t* data)
{
	auto eventHandler = static_cast<EventHandler*>(param);

	obs_source_t* source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	eventHandler->ConnectSourceSignals(source);
}

void EventHandler::SourceDestroyedMultiHandler(void* param, calldata_t* data)
{
	auto eventHandler = static_cast<EventHandler*>(param);

	obs_source_t* source = GetCalldataPointer<obs_source_t>(data, "source");
	if (!source)
		return;

	eventHandler->DisconnectSourceSignals(source);
}

void EventHandler::HandleSceneItemCreated(void* param, calldata_t* data)
{
	auto eventHandler = static_cast<EventHandler*>(param);
	eventHandler->HandleSceneListChanged();
}

void EventHandler::HandleSceneItemRemoved(void* param, calldata_t* data)
{
	obs_scene_t* scene = GetCalldataPointer<obs_scene_t>(data, "scene");
	if (!scene)
		return;

	obs_sceneitem_t* sceneItem = GetCalldataPointer<obs_sceneitem_t>(data, "item");
	if (!sceneItem)
		return;

	std::string removeSourceName = obs_source_get_name(obs_sceneitem_get_source(sceneItem));

	auto eventHandler = static_cast<EventHandler*>(param);
	eventHandler->HandleSceneListChanged(removeSourceName);
}

void EventHandler::HandleSceneItemListReindexed(void* param, calldata_t* data)
{
	auto eventHandler = static_cast<EventHandler*>(param);

	eventHandler->HandleSceneListChanged();
}

void EventHandler::HandleSceneItemEnableStateChanged(void* param, calldata_t* data)
{
	auto eventHandler = static_cast<EventHandler*>(param);

	obs_scene_t* scene = GetCalldataPointer<obs_scene_t>(data, "scene");
	if (!scene)
		return;

	obs_sceneitem_t* sceneItem = GetCalldataPointer<obs_sceneitem_t>(data, "item");
	if (!sceneItem)
		return;

	bool sceneItemEnabled = calldata_bool(data, "visible");

	nlohmann::json eventData;
	eventData["sceneName"] = obs_source_get_name(obs_scene_get_source(scene));
	eventData["sourceName"] = obs_source_get_name(obs_sceneitem_get_source(sceneItem));
	eventData["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
	eventData["sceneItemEnabled"] = sceneItemEnabled;
	eventHandler->SendEventToStreamDeck(EventSubscription::SceneItems, "SceneItemEnableStateChanged", eventData);
}

void EventHandler::HandleSourceListChanged(obs_scene_t* scene, bool basic)
{
	if (!scene) {
		return;
	}

	nlohmann::json eventData;
	eventData["sceneName"] = obs_source_get_name(obs_scene_get_source(scene));
	eventData["sceneItems"] = GetSceneItemList(scene, basic);
	SendEventToStreamDeck(EventSubscription::SceneItems, "SceneListChanged", eventData);
}

void EventHandler::HandleCurrentProgramSceneChanged()
{
	OBSSourceAutoRelease currentScene = obs_frontend_get_current_scene();
	if (!currentScene)
		return;

	nlohmann::json eventData;
	eventData["sceneName"] = obs_source_get_name(currentScene);
	SendEventToStreamDeck(EventSubscription::Scenes, "CurrentProgramSceneChanged", eventData);

}

void EventHandler::HandleCurrentPreviewSceneChanged()
{
	OBSSourceAutoRelease currentPreviewScene = obs_frontend_get_current_preview_scene();

	// This event may be called when OBS is not in studio mode, however retreiving the source while not in studio mode will return null.
	if (!currentPreviewScene)
		return;

	nlohmann::json eventData;
	eventData["sceneName"] = obs_source_get_name(currentPreviewScene);
	SendEventToStreamDeck(EventSubscription::Scenes, "CurrentPreviewSceneChanged", eventData);
}

void EventHandler::HandleSceneListChanged(std::string removeSourceName)
{
	nlohmann::json eventData;
	eventData["scenes"] = GetSceneListWithSceneItemList(removeSourceName);// GetSceneList();

	SendEventToStreamDeck(EventSubscription::Scenes, "SceneListChanged", eventData);
}

static bool GetOutputStateActive(ObsOutputState state)
{
	switch (state) {
	case OBS_WEBSOCKET_OUTPUT_STARTED:
	case OBS_WEBSOCKET_OUTPUT_RESUMED:
	case OBS_WEBSOCKET_OUTPUT_RECONNECTED:
		return true;
	case OBS_WEBSOCKET_OUTPUT_STARTING:
	case OBS_WEBSOCKET_OUTPUT_STOPPING:
	case OBS_WEBSOCKET_OUTPUT_STOPPED:
	case OBS_WEBSOCKET_OUTPUT_RECONNECTING:
	case OBS_WEBSOCKET_OUTPUT_PAUSED:
		return false;
	default:
		return false;
	}
}

void EventHandler::HandleStreamStateChanged(ObsOutputState state)
{
	nlohmann::json eventData;
	eventData["outputActive"] = GetOutputStateActive(state);
	eventData["outputState"] = state;
	SendEventToStreamDeck(EventSubscription::Outputs, "StreamStateChanged", eventData);
}

void EventHandler::HandleRecordStateChanged(ObsOutputState state)
{
	nlohmann::json eventData;
	eventData["outputActive"] = GetOutputStateActive(state);
	//eventData["outputState"] = state;
	//if (state == OBS_WEBSOCKET_OUTPUT_STOPPED || state == OBS_WEBSOCKET_OUTPUT_STARTED) {
	//	eventData["outputPath"] = Utils::Obs::StringHelper::GetLastRecordFileName();
	//}
	//else {
	//	eventData["outputPath"] = nullptr;
	//}
	SendEventToStreamDeck(EventSubscription::Outputs, "RecordStateChanged", eventData);
}

void EventHandler::HandleStudioModeStateChanged(bool enabled)
{
	nlohmann::json eventData;
	eventData["studioModeEnabled"] = enabled;
	SendEventToStreamDeck(EventSubscription::Ui, "StudioModeStateChanged", eventData);
}

void EventHandler::HandleStudioSOOPChannelSidebarStateChanged(bool active)
{
	nlohmann::json eventData;
	eventData["studioSOOPChannelSidebarActive"] = active;
	SendEventToStreamDeck(EventSubscription::Ui, "StudioSOOPChannelSidebarStateChanged", eventData);
}

void EventHandler::HandleStudioSOOPLNBMenuStateChanged(int menuId, bool active)
{
	nlohmann::json eventData;
	eventData["menuId"] = menuId;
	eventData["SOOPLNBMenuActive"] = active;
	SendEventToStreamDeck(EventSubscription::Ui, "StudioSOOPLNBMenuStateChanged", eventData);
}

void EventHandler::HandleStudioMainMicStateChanged(bool active)
{
	nlohmann::json eventData;
	eventData["studioMainMicState"] = active;
	SendEventToStreamDeck(EventSubscription::Inputs, "StudioMainMicStateChanged", eventData);
}

void EventHandler::HandleStudioMainVolumeStateChanged(bool active)
{
	nlohmann::json eventData;
	eventData["studioMainVolumeState"] = active;
	SendEventToStreamDeck(EventSubscription::Inputs, "StudioMainVolumeStateChanged", eventData);
}

std::vector<nlohmann::json> EventHandler::GetSceneList()
{
	obs_frontend_source_list sceneList = {};
	obs_frontend_get_scenes(&sceneList);

	std::vector<nlohmann::json> ret;
	ret.reserve(sceneList.sources.num);
	for (size_t i = 0; i < sceneList.sources.num; i++) {
		obs_source_t* scene = sceneList.sources.array[i];

		nlohmann::json sceneJson;
		sceneJson["sceneName"] = obs_source_get_name(scene);
		sceneJson["sceneIndex"] = sceneList.sources.num - i - 1;

		ret.push_back(sceneJson);
	}

	obs_frontend_source_list_free(&sceneList);

	return ret;
}

std::vector<nlohmann::json> EventHandler::GetSceneListWithSceneItemList(std::string& removeSourceName)
{
	obs_frontend_source_list sceneList = {};
	obs_frontend_get_scenes(&sceneList);

	std::vector<nlohmann::json> ret;
	ret.reserve(sceneList.sources.num);

	for (size_t i = 0; i < sceneList.sources.num; i++) {
		obs_source_t* sceneSource = sceneList.sources.array[i];
		if (!sceneSource)
			continue;

		obs_scene_t* scene = obs_scene_from_source(sceneSource);
		if (!scene)
			continue;

		nlohmann::json sceneJson;
		sceneJson["sceneName"] = obs_source_get_name(sceneSource);
		sceneJson["sceneIndex"] = static_cast<int>(sceneList.sources.num - i - 1);
		//sceneJson["sceneItems"] = GetSceneItemList(scene, false);

		if (scene) {
			std::vector<nlohmann::json> items = GetSceneItemList(scene, false);

			if (!removeSourceName.empty()) {
				items.erase(
					std::remove_if(items.begin(), items.end(), [&](const nlohmann::json& item) {
						return item.contains("sourceName") && item["sourceName"] == removeSourceName;
						}),
					items.end()
				);
			}
			sceneJson["sceneItems"] = std::move(items);
		}

		ret.push_back(sceneJson);
	}

	obs_frontend_source_list_free(&sceneList);

	return ret;
}

bool SceneItemEnumCallback(obs_scene_t*, obs_sceneitem_t* sceneItem, void* param)
{
	auto enumData = static_cast<std::pair<std::vector<nlohmann::json>*, std::string>*>(param);
	auto& list = *enumData->first;
	std::string groupName = enumData->second;

	nlohmann::json item;
	//item["sceneItemId"] = obs_sceneitem_get_id(sceneItem);
	//item["sceneItemIndex"] = list.size();

	item["sceneItemEnabled"] = obs_sceneitem_visible(sceneItem);
	//item["sceneItemLocked"] = obs_sceneitem_locked(sceneItem);
	//item["sceneItemTransform"] = ObjectHelper::GetSceneItemTransform(sceneItem);
	//item["sceneItemBlendMode"] = obs_sceneitem_get_blending_mode(sceneItem);

	OBSSource itemSource = obs_sceneitem_get_source(sceneItem);
	item["sourceName"] = obs_source_get_name(itemSource);
	item["sourceType"] = obs_source_get_type(itemSource);

	if (obs_source_get_type(itemSource) == OBS_SOURCE_TYPE_INPUT)
		item["inputKind"] = obs_source_get_id(itemSource);
	else
		item["inputKind"] = nullptr;

	bool isGroup = obs_source_get_type(itemSource) == OBS_SOURCE_TYPE_SCENE && obs_source_is_group(itemSource);
	item["isGroup"] = isGroup;
	item["groupName"] = groupName;

	if (isGroup) {

		OBSDataAutoRelease data =
			obs_sceneitem_get_private_settings(sceneItem);

		obs_scene_t* scene =
			obs_sceneitem_group_get_scene(sceneItem);

		groupName = obs_source_get_name(itemSource);
		std::vector<nlohmann::json> groupChildren;
		std::pair<std::vector<nlohmann::json>*, std::string> groupParam{ &groupChildren, groupName };
		obs_scene_enum_items(scene, SceneItemEnumCallback, &groupParam);

		list.insert(list.end(), groupChildren.begin(), groupChildren.end());
		list.push_back(item);
	}
	else {
		list.push_back(item);
	}

	return true;
}

std::vector<nlohmann::json> EventHandler::GetSceneItemList(obs_scene_t* scene, bool basic)
{
	std::vector<nlohmann::json> result;
	std::pair<std::vector<nlohmann::json>*, std::string> enumData{ &result, "" };

	obs_scene_enum_items(scene, SceneItemEnumCallback, &enumData);

	std::reverse(result.begin(), result.end());

	return result;
}