#include <obs-frontend-api.h>
#include <obs-module.h>
#include <nlohmann/json.hpp>

#include "soop-streamdeck.h"

#include "WebSocketServer.h"
#include "EventHandler.h"

const char* SOOP_STREAMDECK_PLUGIN_VERSION = "1.0.0";

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
OBS_MODULE_AUTHOR("SOOPStudio")

std::shared_ptr<SOOPWebSocketServer> _WebserverStreamDeck;
std::shared_ptr<EventHandler>		 _EventHandler;

bool CheckSoopFrontendApiVersion()
{
	HMODULE h = GetModuleHandleA("obs-frontend-api.dll");
	if (!h) return false;

	auto get_version = (const char* (*)())GetProcAddress(h, "soop_frontend_api_get_version");
	if (!get_version) return false;

	const char* ver = get_version();
	return strcmp(ver, "1.0.0") >= 0;
}

EventHandlerPtr GetEventHandler()
{
	return _EventHandler;
}

WebSocketServerPtr GetWebSocketServer()
{
	return _WebserverStreamDeck;
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "[soop-streamdeck]: Version %s", SOOP_STREAMDECK_PLUGIN_VERSION);

	if (!CheckSoopFrontendApiVersion()) {
		blog(LOG_INFO, "[soop-frontend-api]: Version is lower");
		return false;
	}

	_EventHandler = std::shared_ptr<EventHandler>(new EventHandler());

	_WebserverStreamDeck = std::shared_ptr<SOOPWebSocketServer>(new SOOPWebSocketServer());
	_WebserverStreamDeck->Start();

	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[soop-streamdeck] unload");

	_WebserverStreamDeck->Stop();
}

const char* obs_module_name(void)
{
	return "soop-streamdeck";
}

MODULE_EXPORT const char* obs_module_description(void)
{
	return "soop streamdeck plugin";
}
