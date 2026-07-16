#include "SOOPAPIHandler.h"

#include <QObject>
#include <QMetaObject>
#include <QThread>
#include <QString>
#include <QByteArray>
#include <QNetworkRequest>

#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Auth/SBaseAuth.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "obs.hpp"
#include "Common/StudioDefine.h"

SOOPApiHandler::SOOPApiHandler() 
{
	m_mapAPI.clear();

	m_lang = LOCALE_CONTEXT.GetCurrentLocaleStr().c_str();

	initAPIList();

	m_workerRunning = true;
	m_workerThread = std::thread(&SOOPApiHandler::_workerLoop, this);
}

SOOPApiHandler::~SOOPApiHandler()
{
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_workerRunning = false;

		std::queue<CurlRequest> empty;
		std::swap(m_requestQueue, empty);
	}

	m_cv.notify_one();

	if (m_workerThread.joinable())
		m_workerThread.join();

	m_mapAPI.clear();
}

void SOOPApiHandler::initAPIList() {
	setAPIInfo(API_NONE, REQUEST_TYPE::TYPE_NONE, "", "");
};

void SOOPApiHandler::setAPIInfo(SOOP_API_KEY key, REQUEST_TYPE type, std::string apiName, std::string url , const QList<QString> postParamKeys)
{
	m_mapAPI[key].apiName = apiName;
	m_mapAPI[key].url = url;
	m_mapAPI[key].paramKeys = postParamKeys;
}

SOOP_API_DATA SOOPApiHandler::getAPIInfofromId(SOOP_API_KEY api) {

	if(m_mapAPI.empty())
		return { "", "" };

	auto it = m_mapAPI.find(api);
	if (it != m_mapAPI.end()) {
		return it->second;
	}
	return { "", "" };
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

CurlResult SOOPApiHandler::performCurl(const std::string& url,
										const std::string& cookie,
										const std::string& postData,
										const std::string& token,
										const std::string& contentType,
										bool isPost,
										bool isDelete)
{
	CurlResult result{ CURLE_FAILED_INIT, "" };

	CURL* curl = curl_easy_init();
	if (!curl)
		return result;

	std::string readBuffer;
	struct curl_slist* header = nullptr;

	header = curl_slist_append(header, "");

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	//curl_easy_setopt(curl, CURLOPT_PROXY, "");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	std::string finalCookie;

	if (!cookie.empty())
	{
		finalCookie = cookie;

		if (!finalCookie.empty() && finalCookie.back() != ';')
			finalCookie += ";";
	}

	if (!m_lang.empty())
	{
		finalCookie += "_lang=" + m_lang + ";";
	}

	if (!finalCookie.empty())
	{
		curl_easy_setopt(curl, CURLOPT_COOKIE, finalCookie.c_str());
	}

	if (isDelete) {
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	}
	if (isPost) {
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)postData.size());
	}

	// SSL & timeout
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 8L);

	//
	CURLcode res = curl_easy_perform(curl);
	result.code = res;
	if (res == CURLE_OK) {
		result.response = readBuffer;
	}

	curl_easy_cleanup(curl);
	curl_slist_free_all(header);

	return result;
}

void SOOPApiHandler::getAPI(const char* apiName, const char* url, QObject* receiver, const char* slot, const QList<int>& appendInts, std::string tempcookie, bool invokeWhileError)
{
	CurlRequest req;
	req.isPostMethod = false;
	req.apiName = apiName;
	req.url = url;
	req.receiver = receiver;
	req.slot = slot;
	req.postData.clear();
	req.token = "";
	req.appendInts = appendInts;
	req.invokeWhileError = invokeWhileError;

	req.cookie = tempcookie.empty()
					  ? AUTH_CONTEXT.SoopCookie().c_str() 
					  : QString::fromStdString(tempcookie);

	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_requestQueue.push(req);
	}
	m_cv.notify_one();
}

bool SOOPApiHandler::getAPIfromId(SOOP_API_KEY key, const QList<QVariant>& queryValues, QObject* receiver, const char* slot, const QList<int>& appendInts, std::string tempcookie, bool invokeWhileError) {
	SOOP_API_DATA apiInfo = getAPIInfofromId(key);
	if (0 == apiInfo.url.compare(""))
		return false;

	if (0 == apiInfo.apiName.compare(""))
		return false;

	QString result = QString::fromStdString(apiInfo.url);

	if (!queryValues.isEmpty())
	{
		QString body;
		for (int i = 0; i < queryValues.size(); i++) {
			if (!body.isEmpty())
				body += '&';

			const QString key = apiInfo.paramKeys[i];
			const QString value = queryValues[i].toString();

			body += QUrl::toPercentEncoding(key);
			body += '=';
			body += QUrl::toPercentEncoding(value);
		}

		result += "?";
		result += body;
	}

	getAPI(apiInfo.apiName.c_str(), result.toStdString().c_str(), receiver, slot, appendInts, tempcookie, invokeWhileError);
	return true;
}

std::string SOOPApiHandler::getAPIfromId(SOOP_API_KEY key, const QList<QVariant>& queryValues)
{
	SOOP_API_DATA apiInfo = getAPIInfofromId(key);
	if (apiInfo.url.empty() || apiInfo.apiName.empty())
		return "";

	QString result = QString::fromStdString(apiInfo.url);

	if (!queryValues.isEmpty())
	{
		QString body;
		for (int i = 0; i < queryValues.size(); i++) {
			if (!body.isEmpty())
				body += '&';

			const QString key = apiInfo.paramKeys[i];
			const QString value = queryValues[i].toString();

			body += QUrl::toPercentEncoding(key);
			body += '=';
			body += QUrl::toPercentEncoding(value);
		}

		result += "?";
		result += body;
	}

	blog_api(LOG_INFO, std::string("[api call] - " + apiInfo.apiName).c_str());

	CurlResult res = performCurl(result.toStdString(), AUTH_CONTEXT.SoopCookie());

	if (res.code == CURLE_OK) {
		blog_api(LOG_INFO, std::string("[api response] - " + apiInfo.apiName).c_str());
	}
	else {
		blog_api(LOG_INFO, std::string("[api fail] - " + apiInfo.apiName + " : " + curl_easy_strerror(res.code)).c_str());
	}

	return (res.code == CURLE_OK) ? res.response : "";
}

void SOOPApiHandler::postAPI(const char* apiName, const char* url, const QByteArray& postData, QObject* receiver,
	const char* slot, const QList<int>& appendInts, const char* contentType, std::string token, bool invokeWhileError)
{

	CurlRequest req;
	req.isPostMethod = true;
	req.apiName = apiName;
	req.url = url;
	req.receiver = receiver;
	req.slot = slot;
	req.postData = postData;
	req.contentType = contentType ? QString(contentType) : "";
	req.token = QString::fromStdString(token);
	req.appendInts = appendInts;
	req.cookie = AUTH_CONTEXT.SoopCookie().c_str();
	req.invokeWhileError = invokeWhileError;

	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_requestQueue.push(req);
	}
	m_cv.notify_one();
}

bool SOOPApiHandler::postAPIfromId(SOOP_API_KEY apikey, const QList<QVariant>& postDataValues,
								   QObject* receiver, const char* slot, const QList<int>& appendInts, bool invokeWhileError) {
	SOOP_API_DATA apiInfo = getAPIInfofromId(apikey);

	if (0 == apiInfo.url.compare(""))
		return false;

	if (apiInfo.paramKeys.isEmpty())
		return false;

	if (0 == apiInfo.apiName.compare(""))
		return false;

	QString postBody;
	for (int i = 0; i < postDataValues.size(); ++i) {
		if (!postBody.isEmpty())
			postBody += '&';

		const QString key = apiInfo.paramKeys[i];
		const QString value = postDataValues[i].toString();

		postBody += QUrl::toPercentEncoding(key);
		postBody += '=';
		postBody += QUrl::toPercentEncoding(value);
	}

	postAPI(apiInfo.apiName.c_str(), apiInfo.url.c_str(), postBody.toStdString().c_str(),
		receiver, slot, appendInts, "", {}, invokeWhileError);

	return true;
}

std::string SOOPApiHandler::postAPIfromId(SOOP_API_KEY key, const QList<QVariant>& postDataValues)
{
	SOOP_API_DATA apiInfo = getAPIInfofromId(key);
	if (apiInfo.url.empty() || apiInfo.paramKeys.empty() || apiInfo.apiName.empty())
		return "";

	QString postBody;
	for (int i = 0; i < postDataValues.size(); ++i) {
		if (!postBody.isEmpty())
			postBody += '&';

		const QString key = apiInfo.paramKeys[i];
		const QString value = postDataValues[i].toString();

		postBody += QUrl::toPercentEncoding(key);
		postBody += '=';
		postBody += QUrl::toPercentEncoding(value);
	}

	blog_api(LOG_INFO, std::string("[api call] - " + apiInfo.apiName).c_str());

	CurlResult res = performCurl(apiInfo.url,
								 AUTH_CONTEXT.SoopCookie(),
								 postBody.toStdString(),
								 "", "", true /* isPost */);

	if (res.code == CURLE_OK) {
		blog_api(LOG_INFO, std::string("[api response] - " + apiInfo.apiName).c_str());
	}
	else {
		blog_api(LOG_INFO, std::string("[api fail] - " + apiInfo.apiName + " : " + curl_easy_strerror(res.code)).c_str());
	}

	return (res.code == CURLE_OK) ? res.response : "";
}

void SOOPApiHandler::deleteAPI(const char* _url, QObject* receiver, const char* slot, const QList<int>& appendInts, std::string tempcookie)
{

	CurlRequest req;
	req.isPostMethod = false;
	req.isDelete = true;
	req.apiName = "DELETE";
	req.url = _url;
	req.receiver = receiver;
	req.slot = slot;
	req.postData.clear();
	req.token.clear();
	req.appendInts = appendInts;
	req.cookie = tempcookie.empty()
				 ? AUTH_CONTEXT.SoopCookie().c_str()
				 : QString::fromStdString(tempcookie);

	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_requestQueue.push(req);
	}
	m_cv.notify_one();
}

void SOOPApiHandler::deleteAPIfromId(SOOP_API_KEY key, const QList<QVariant>& queryValues, QObject* receiver, const char* slot, const QList<int>& appendInts, std::string tempcookie)
{
	SOOP_API_DATA apiInfo = getAPIInfofromId(key);
	if (0 == apiInfo.url.compare(""))
		return;

	QString result = apiInfo.url.c_str();

	for (int i = 0; i < queryValues.size(); ++i) {
		QString placeholder = "%" + QString::number(i + 1);

		QString value;
		if (queryValues[i].type() == QVariant::Bool) {
			value = queryValues[i].toBool() ? "true" : "false";
		}
		else {
			value = queryValues[i].toString();
		}

		result.replace(placeholder, value);
	}

	deleteAPI(result.toStdString().c_str(), receiver, slot, appendInts, tempcookie);
}

void SOOPApiHandler::downloadImage(const char* url, QObject* receiver, const char* slot) {
	getAPI("", url, receiver, slot);
}

void SOOPApiHandler::_workerLoop()
{
	while (m_workerRunning) {
		CurlRequest req;

		{
			std::unique_lock<std::mutex> lock(m_queueMutex);
			m_cv.wait(lock, [&]() { return !m_requestQueue.empty() || !m_workerRunning; });

			if (!m_workerRunning) break;
			req = m_requestQueue.front();
			m_requestQueue.pop();
		}

		if (!req.receiver) continue;

		if (false == req.apiName.isEmpty())
		{
			if (req.apiName != "check_broading" && req.apiName != "check_simulcast_broading" &&
				req.apiName != "get_user_info" && req.apiName != "get_broadinfo")
			{
				blog_api(LOG_INFO, QString("[api call] - %1")
					.arg(req.apiName).toStdString().c_str());
			}
		}

		CurlResult res = performCurl(req.url.toStdString(),
			req.cookie.toStdString(),
			req.postData.toStdString(),
			req.token.toStdString(),
			req.contentType.toStdString(),
			req.isPostMethod,
			req.isDelete);

		if (res.code == CURLE_OK || req.invokeWhileError) {
			QByteArray responseData = QByteArray::fromStdString(res.response);

				if (res.code == CURLE_OK)
					blog_api(LOG_INFO, std::string("[api response] - " + req.apiName.toStdString()).c_str());
				else
					blog_api(LOG_WARNING, QString("[api fail] - %1 : %2")
						.arg(req.apiName)
						.arg(curl_easy_strerror(res.code))
						.toStdString().c_str());

			if (req.receiver)
			{
				switch (req.appendInts.size()) {
				case 0:
					QMetaObject::invokeMethod(req.receiver, req.slot.toStdString().c_str(),
						Qt::QueuedConnection,
						Q_ARG(QByteArray, responseData));
					break;
				case 1:
					QMetaObject::invokeMethod(req.receiver, req.slot.toStdString().c_str(),
						Qt::QueuedConnection,
						Q_ARG(QByteArray, responseData),
						Q_ARG(int, req.appendInts[0]));
					break;
				case 2:
					QMetaObject::invokeMethod(req.receiver, req.slot.toStdString().c_str(),
						Qt::QueuedConnection,
						Q_ARG(QByteArray, responseData),
						Q_ARG(int, req.appendInts[0]),
						Q_ARG(int, req.appendInts[1]));
					break;
				case 3:
					QMetaObject::invokeMethod(req.receiver, req.slot.toStdString().c_str(),
						Qt::QueuedConnection,
						Q_ARG(QByteArray, responseData),
						Q_ARG(int, req.appendInts[0]),
						Q_ARG(int, req.appendInts[1]),
						Q_ARG(int, req.appendInts[2]));
					break;
				default:
					break;
				}
			}
		}
		else {
			blog_api(LOG_WARNING, QString("[api fail] - %1 : %2")
				.arg(req.apiName)
				.arg(curl_easy_strerror(res.code))
				.toStdString().c_str());
		}
	}
}
