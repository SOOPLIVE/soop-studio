#pragma once

#include "WebSocketServer.h"

#include <vector>

#include <obs.hpp>
#include <obs-frontend-api.h>

#include "soop-streamdeck.h"
#include "EventHandler.h"

#define STREAMDECK_PLUGIN_PORT	11521
#define MAX_MESSAGE_SIZE 256 * 1024

SOOPWebSocketServer::SOOPWebSocketServer()
{
	_streamdeck_server.set_access_channels(websocketpp::log::alevel::all);
	_streamdeck_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
	_streamdeck_server.init_asio();

	_streamdeck_server.set_validate_handler(
		websocketpp::lib::bind(&SOOPWebSocketServer::on_validate, this, websocketpp::lib::placeholders::_1));
	_streamdeck_server.set_open_handler(websocketpp::lib::bind(&SOOPWebSocketServer::on_open, this, websocketpp::lib::placeholders::_1));
	_streamdeck_server.set_close_handler(websocketpp::lib::bind(&SOOPWebSocketServer::on_close, this, websocketpp::lib::placeholders::_1));
	_streamdeck_server.set_message_handler(websocketpp::lib::bind(&SOOPWebSocketServer::on_message, this, websocketpp::lib::placeholders::_1,
		websocketpp::lib::placeholders::_2));

	auto eventHandler = GetEventHandler();
	if (eventHandler) {
		eventHandler->SetWebSocketCallback(std::bind(&SOOPWebSocketServer::BroadcastEvent, this, std::placeholders::_1,
			std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}
}

SOOPWebSocketServer::~SOOPWebSocketServer()
{
	Stop();
}

std::string ConvertToUtf8(const char* localStr)
{
	int wlen = MultiByteToWideChar(CP_ACP, 0, localStr, -1, nullptr, 0);
	if (wlen <= 0) return "Failed to decode";

	std::wstring wstr(wlen, 0);
	MultiByteToWideChar(CP_ACP, 0, localStr, -1, &wstr[0], wlen);

	int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (u8len <= 0) return "Failed to encode";

	std::string utf8str(u8len, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8str[0], u8len, nullptr, nullptr);

	return utf8str;
}

void SOOPWebSocketServer::Start()
{
	try {
		_streamdeck_server.listen(websocketpp::lib::asio::ip::tcp::endpoint(
			websocketpp::lib::asio::ip::address::from_string("127.0.0.1"),
			STREAMDECK_PLUGIN_PORT
		));

		_streamdeck_server.set_max_message_size(MAX_MESSAGE_SIZE);
		_streamdeck_server.start_accept();

		_running = true;

		_server_thread = std::thread(&SOOPWebSocketServer::ServerRunThread, this);
		_send_thread = std::thread(&SOOPWebSocketServer::SendThreadLoop, this);
	}
	catch (const std::exception& e) {
		std::string converted = ConvertToUtf8(e.what());
		blog(LOG_ERROR, "[soop-ws] Exception in Start: %s", converted.c_str());
	}
	catch (...) {
		blog(LOG_ERROR, "[soop-ws] Unknown exception in Start()");
	}
}

void SOOPWebSocketServer::Stop()
{
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		_running = false;
	}
	_queueCV.notify_all();

	if (_streamdeck_server.is_listening()) {
		_streamdeck_server.stop_listening();
	}
	_streamdeck_server.get_io_service().stop();

	if (_server_thread.joinable()) 
		_server_thread.join();

	if (_send_thread.joinable()) 
		_send_thread.join();

	std::lock_guard<std::mutex> lock(_connectionMutex);
	for (auto& conn : _connections) {
		_streamdeck_server.close(conn, websocketpp::close::status::going_away, "Server shutting down");
	}
	_connections.clear();
}

void SOOPWebSocketServer::BroadcastEvent(uint64_t requiredIntent, const std::string& eventType, 
										 const nlohmann::json& eventData, uint8_t rpcVersion)
{
	nlohmann::json eventMessage;
	eventMessage["op"] = 5;
	eventMessage["d"]["eventType"] = eventType;
	eventMessage["d"]["eventIntent"] = requiredIntent;
	if (eventData.is_object())
		eventMessage["d"]["eventData"] = eventData;

	std::string messageJson = eventMessage.dump();
	EnqueueMessage(messageJson);
}

void SOOPWebSocketServer::ServerRunThread()
{
	_streamdeck_server.run();
}

void SOOPWebSocketServer::SendThreadLoop()
{
	while (true) {
		std::unique_lock<std::mutex> lock(_queueMutex);
		_queueCV.wait(lock, [this] { 
			return !_sendQueue.empty() || !_running; 
			});

		if (!_running && _sendQueue.empty()) 
			break;

		std::string msg = std::move(_sendQueue.front());
		_sendQueue.pop();
		lock.unlock();

		Send(msg);
	}
}

void SOOPWebSocketServer::EnqueueMessage(const std::string& msg)
{
	{
		std::lock_guard<std::mutex> lock(_queueMutex);
		_sendQueue.push(msg);
	}
	_queueCV.notify_one();
}

void SOOPWebSocketServer::Send(const std::string& msg)
{
	websocketpp::lib::error_code errorCode;

	std::lock_guard<std::mutex> lock(_connectionMutex);
	for (auto& conn : _connections) {
		_streamdeck_server.send(conn, msg, websocketpp::frame::opcode::text, errorCode);

		if (errorCode)
			blog(LOG_ERROR, "[WebSocketServer::BroadcastEvent] Error sending event message: %s",
				errorCode.message().c_str());
	}
}

bool SOOPWebSocketServer::on_validate(websocketpp::connection_hdl hdl)
{
	server::connection_ptr con = _streamdeck_server.get_con_from_hdl(hdl);
	std::vector<std::string> protocols = con->get_requested_subprotocols();

	for (const auto& protocol : protocols) {
		if (protocol == "streamdeck-freecshotplus") {
			con->select_subprotocol(protocol);
			return true;
		}
	}

	return false;
}

void SOOPWebSocketServer::on_open(websocketpp::connection_hdl hdl)
{
	std::lock_guard<std::mutex> lock(_connectionMutex);
	if(_connections.empty())
		_connections.insert(hdl);
	else
		_streamdeck_server.close(hdl,websocketpp::close::status::policy_violation, "Only one connection allowed");
}

void SOOPWebSocketServer::on_close(websocketpp::connection_hdl hdl)
{
	std::lock_guard<std::mutex> lock(_connectionMutex);
	_connections.erase(hdl);
}

void SOOPWebSocketServer::on_message(websocketpp::connection_hdl hdl,
	websocketpp::server<websocketpp::config::asio>::message_ptr message)
{
	const std::string& payload = message->get_payload();

	try {
		nlohmann::json jsonMessage = nlohmann::json::parse(payload);

		if (!jsonMessage.contains("op")) {
			blog(LOG_WARNING, "[WebSocketServer] Received message without 'op': %s", payload.c_str());
			return;
		}

		int opCode = jsonMessage.at("op").get<int>();

		// Receive Ping
		if (opCode == 9) {
			//blog(LOG_INFO, "[WebSocketServer] Received Ping. Sending Pong.");

			nlohmann::json pongMessage;
			pongMessage["op"] = 10;
			pongMessage["d"] = nullptr;

			std::string pongStr = pongMessage.dump();
			websocketpp::lib::error_code ec;
			_streamdeck_server.send(hdl, pongStr, websocketpp::frame::opcode::text, ec);

			if (ec) {
				blog(LOG_ERROR, "[WebSocketServer] Failed to send Pong: %s", ec.message().c_str());
			}
			return;
		}

		// Receive Pong
		if (opCode == 10) {
			//blog(LOG_INFO, "[WebSocketServer] Received Pong.");
			return;
		}

		ParseStreamDeckEventJson(message->get_payload());
	}
	catch (const std::exception& e) {
		blog(LOG_ERROR, "[WebSocketServer] Failed to parse message: %s", e.what());
	}
}

void SOOPWebSocketServer::ParseStreamDeckEventJson(std::string json)
{
	auto eventHandler = GetEventHandler();
	if (eventHandler) {
		eventHandler->ParseStreamDeckEventJson(json);
	}
}