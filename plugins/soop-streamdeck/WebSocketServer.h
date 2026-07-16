#pragma once

#include <thread>
#include <set>
#include <queue>

#include <string>
#include <nlohmann/json.hpp>

#include <obs.hpp>
#include <asio.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;
typedef	websocketpp::connection_hdl conn_hdl;
typedef std::set<conn_hdl, std::owner_less<conn_hdl>> connections;

typedef server::message_ptr message_ptr;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

class SOOPWebSocketServer
{
public:
	SOOPWebSocketServer();
	~SOOPWebSocketServer();

	void Start();
	void Stop();
	
	void BroadcastEvent(uint64_t requiredIntent, const std::string& eventType, const nlohmann::json& eventData = nullptr,
		uint8_t rpcVersion = 0);

private:
	void ServerRunThread();
	void SendThreadLoop();
	void EnqueueMessage(const std::string& msg);
	void Send(const std::string& msg);

	bool on_validate(websocketpp::connection_hdl hdl);
	void on_open(websocketpp::connection_hdl hdl);
	void on_close(websocketpp::connection_hdl hdl);
	void on_message(websocketpp::connection_hdl hdl, websocketpp::server<websocketpp::config::asio>::message_ptr message);

	void ParseStreamDeckEventJson(std::string json);

private:
	server		_streamdeck_server;
	connections _connections;
	std::mutex  _connectionMutex;
	
	//
	std::queue<std::string> _sendQueue;
	std::mutex _queueMutex;
	std::condition_variable _queueCV;

	//
	std::thread _server_thread;
	std::thread _send_thread;
	bool _running = false;
};