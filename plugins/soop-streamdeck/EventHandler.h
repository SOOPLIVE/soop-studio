#pragma once

#include <functional>
#include <thread>
#include <set>
#include <string>
#include <nlohmann/json.hpp>

#include <obs.hpp>
#include <obs-frontend-api.h>

// from obs-websocket
enum ObsOutputState {
	OBS_WEBSOCKET_OUTPUT_UNKNOWN,
	OBS_WEBSOCKET_OUTPUT_STARTING,
	OBS_WEBSOCKET_OUTPUT_STARTED,
	OBS_WEBSOCKET_OUTPUT_STOPPING,
	OBS_WEBSOCKET_OUTPUT_STOPPED,
	OBS_WEBSOCKET_OUTPUT_RECONNECTING,
	OBS_WEBSOCKET_OUTPUT_RECONNECTED,
	OBS_WEBSOCKET_OUTPUT_PAUSED,
	OBS_WEBSOCKET_OUTPUT_RESUMED,
};

class EventHandler
{
public:
	EventHandler();
	~EventHandler();
	
	typedef std::function<void(uint64_t, std::string, nlohmann::json, uint8_t)>
		BroadcastCallback; // uint64_t requiredIntent, std::string eventType, json eventData, uint8_t rpcVersion

	void SetWebSocketCallback(BroadcastCallback cb);
	void SendEventToStreamDeck(uint64_t requiredIntent, std::string eventType, nlohmann::json eventData = nullptr, uint8_t rpcVersion = 0);

	void ParseStreamDeckEventJson(std::string json);

private:

	static void OnFrontendEvent(enum obs_frontend_event event, void* ptr);
	static void OnSoopFrontendEvent(enum soop_frontend_type event, void* data, void* ptr);

	void ConnectSourceSignals(obs_source_t* source);
	void DisconnectSourceSignals(obs_source_t* source);

	// Signal handler: libobs
	static void SourceCreatedMultiHandler(void* param, calldata_t* data);
	static void SourceDestroyedMultiHandler(void* param, calldata_t* data);

	// Signal handler: source
	static void HandleSceneItemCreated(void* param, calldata_t* data);
	static void HandleSceneItemRemoved(void* param, calldata_t* data);
	static void HandleSceneItemListReindexed(void* param, calldata_t* data);
	static void HandleSceneItemEnableStateChanged(void* param, calldata_t* data);

	void HandleSourceListChanged(obs_scene_t* scene, bool basic);

	// Scenes
	void HandleCurrentProgramSceneChanged();
	void HandleCurrentPreviewSceneChanged();
	void HandleSceneListChanged(std::string removeSourceName = "");

	// Output
	void HandleStreamStateChanged(ObsOutputState state);
	void HandleRecordStateChanged(ObsOutputState state);

	// UI
	void HandleStudioModeStateChanged(bool enabled); 
	void HandleStudioSOOPChannelSidebarStateChanged(bool active);
	void HandleStudioSOOPLNBMenuStateChanged(int menuId, bool active);


	// Audio
	void HandleStudioMainMicStateChanged(bool active);
	void HandleStudioMainVolumeStateChanged(bool active);

	//
	std::vector<nlohmann::json> GetSceneList();
	std::vector<nlohmann::json> GetSceneListWithSceneItemList(std::string& removeSourceName);
	std::vector<nlohmann::json> GetSceneItemList(obs_scene_t* scene, bool basic);

private:
	BroadcastCallback _webSocketCallback;
};