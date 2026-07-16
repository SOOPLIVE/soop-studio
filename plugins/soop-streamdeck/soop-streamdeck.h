#include <obs-frontend-api.h>
#include <obs-module.h>
#include <nlohmann/json.hpp>

class SOOPWebSocketServer;
typedef std::shared_ptr<SOOPWebSocketServer> WebSocketServerPtr;

class EventHandler;
typedef std::shared_ptr<EventHandler> EventHandlerPtr;

EventHandlerPtr GetEventHandler();

WebSocketServerPtr GetWebSocketServer();