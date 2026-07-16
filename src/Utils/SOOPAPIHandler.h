#pragma once

#include <unordered_map>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <QObject>
#include <QList>
#include <QPointer>

#include <json11.hpp>
#include <curl/curl.h>

class AFQGLBroadInfo;

enum SOOP_API_KEY {
	API_FIRST,
	GET_CATEGORY_LIST,
	GET_VOD_CONTENT_LIST,
	GET_VOD_LIST,
	GET_TVBROAD_ONETIME_URL,
	GET_DIRECTBROAD_DATA,
	GET_DIRECTBROAD_ONETIME_URL,
	GET_STREAMER_INFO,
	GET_UP_INFO,
	GET_USER_INFO,
	GET_GIFT_INFO,
	GET_SAVVY_REACTION_ABLE,
	GET_BROADINFO,
	GET_ITEMINFO,
	GET_USE_BURNNINGTEN_ITEM,
	GET_REMAIN_BURNNINGTEN_ITEM,
	GET_OPENAPI_DAMAIN,
	GET_KR_BROADING_STATUS,
	POST_BROADINFO_SETTING,
	POST_BROADINFO_SETTING_WITH_SUB,
	POST_VOD_SAVE,
	POST_VOD_SAVE_M,
	POST_AQUA_REMOTE_CONTROL,
	POST_BROAD_START,
	GET_FREECSHOTPLUS_UPDATELOG_VERSION,
	GET_BROADTITLE_ENABLE,
	POST_KR_CLOSE_REQUEST,
	BREAKTIME_START,
	BREAKTIME_STOP,
	BREAKTIME_AD_COUNT,
	GET_BROAD_GEO_BLOCK,
	GET_AI_MANAGER_TOKEN,
	API_NONE,
};

enum REQUEST_TYPE {
	TYPE_NONE,
	GET,
	POST,
	DELETETYPE
};

struct SOOP_API_DATA {
	std::string apiName;
	std::string url;
	QList<QString> paramKeys;
};

struct CurlResult {
	CURLcode code;
	std::string response;
};

struct CurlRequest {
	bool isPostMethod = false;
	QString apiName;
	QString url;
	QByteArray postData; // GET is empty
	QString contentType; // only use POST
	QString cookie;
	QString token;
	QPointer<QObject> receiver;
	QString slot;
	QList<int> appendInts;
	bool invokeWhileError = false;

	bool isDelete  = false;
};

class SOOPApiHandler : public QObject
{
	Q_OBJECT

public:
	SOOPApiHandler();
	~SOOPApiHandler();

public:
	void initAPIList();
	void setAPIInfo(SOOP_API_KEY key, REQUEST_TYPE type, std::string apiName, std::string url = "", const QList<QString> postParamKeys = QList<QString>());

	SOOP_API_DATA getAPIInfofromId(SOOP_API_KEY api);

	CurlResult performCurl(const std::string& url,
							const std::string& cookie,
							const std::string& postData = "",
							const std::string& token = "",
							const std::string& contentType = "",
							bool isPost = false,
							bool isDelete = false);

	void getAPI(const char* apiName, const char* _url, QObject* receiver, const char* slot, 
		const QList<int>& appendInts = QList<int>(), std::string tempcookie = "", bool invokeWhileError = false);
	bool getAPIfromId(SOOP_API_KEY key, const QList<QVariant>& queryValues, QObject* receiver,
		const char* slot, const QList<int>& appendInts = QList<int>(), std::string tempcookie = "", bool invokeWhileError = false);

	std::string getAPIfromId(SOOP_API_KEY key, const QList<QVariant>& queryValues);

	void postAPI(const char* apiName, const char* _url, const QByteArray& postData, QObject* receiver, const char* slot, const QList<int>& appendInts = QList<int>(), const char* contentType = nullptr, std::string token = "", bool invokeWhileError = false);
	bool postAPIfromId(SOOP_API_KEY key, const QList<QVariant>& postDataValues, QObject* receiver, const char* slot, const QList<int>& appendInts = QList<int>(), bool invokeWhileError = false);
	
	std::string postAPIfromId(SOOP_API_KEY key, const QList<QVariant>& postDataValues);

	void deleteAPI(const char* _url, QObject* receiver, const char* slot,
		const QList<int>& appendInts = QList<int>(), std::string tempcookie = "");
	void deleteAPIfromId(SOOP_API_KEY key, const QList<QVariant>& queryValues, QObject* receiver,
		const char* slot, const QList<int>& appendInts = QList<int>(), std::string tempcookie = "");

	void downloadImage(const char* url, QObject* receiver, const char* slot);

private:
	void _workerLoop();

private:
	std::unordered_map<SOOP_API_KEY, SOOP_API_DATA> m_mapAPI;

	std::queue<CurlRequest> m_requestQueue;
	std::mutex m_queueMutex;
	std::condition_variable m_cv;
	std::atomic<bool> m_workerRunning = false;
	std::thread m_workerThread;

	std::string m_lang;
};