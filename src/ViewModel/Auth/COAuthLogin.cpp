#include "COAuthLogin.hpp"
#include "Application/CApplication.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <util/curl/curl-helper.h>

#include "qt-wrapper.h"


#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Browser/CCefManager.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"


#include <unordered_map>

#include <json11.hpp>


using namespace json11;



// Utils Func
static auto curl_deleter = [](CURL *curl) {
    curl_easy_cleanup(curl);
};
using Curl = std::unique_ptr<CURL, decltype(curl_deleter)>;

static size_t string_write(char *ptr, size_t size, size_t nmemb, std::string &str)
{
    size_t total = size * nmemb;
    if (total)
        str.append(ptr, total);

    return total;
}
static size_t header_write(char *ptr, size_t size, size_t nmemb,
               std::vector<std::string> &list)
{
    std::string str;

    size_t total = size * nmemb;
    if (total)
        str.append(ptr, total);

    if (str.back() == '\n')
        str.resize(str.size() - 1);
    if (str.back() == '\r')
        str.resize(str.size() - 1);

    list.push_back(std::move(str));
    return total;
}

bool GetRemoteFile(const char *url, std::string &str, std::string &error,
                   long *responseCode, const char *contentType,
                   std::string request_type, const char *postData,
                   std::vector<std::string> extraHeaders,
                   std::string *signature, int timeoutSec,
                   bool fail_on_error, int postDataSize)
{
    std::vector<std::string> header_in_list;
    char error_in[CURL_ERROR_SIZE];
    CURLcode code = CURLE_FAILED_INIT;

    error_in[0] = 0;

    std::string versionString("User-Agent: SOOP Studio 0.0.1");
    //versionString += App()->GetVersionString();
    std::string contentTypeString;
    if (contentType) {
        contentTypeString += "Content-Type: ";
        contentTypeString += contentType;
    }

    Curl curl{curl_easy_init(), curl_deleter};
    if (curl) {
        struct curl_slist *header = nullptr;

        header = curl_slist_append(header, versionString.c_str());

        if (!contentTypeString.empty()) {
            header = curl_slist_append(header,
                           contentTypeString.c_str());
        }

        for (std::string &h : extraHeaders)
            header = curl_slist_append(header, h.c_str());

        curl_easy_setopt(curl.get(), CURLOPT_URL, url);
        curl_easy_setopt(curl.get(), CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, header);
        curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_in);
        if (fail_on_error)
            curl_easy_setopt(curl.get(), CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION,
                 string_write);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &str);
        curl_obs_set_revoke_setting(curl.get());

        if (signature) {
            curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION,
                     header_write);
            curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA,
                     &header_in_list);
        }

        if (timeoutSec)
            curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT,
                     timeoutSec);

        if (!request_type.empty()) {
            if (request_type != "GET")
                curl_easy_setopt(curl.get(),
                         CURLOPT_CUSTOMREQUEST,
                         request_type.c_str());

            // Special case of "POST"
            if (request_type == "POST") {
                curl_easy_setopt(curl.get(), CURLOPT_POST, 1);
                if (!postData)
                    curl_easy_setopt(curl.get(),
                             CURLOPT_POSTFIELDS,
                             "{}");
            }
        }
        if (postData) {
            if (postDataSize > 0) {
                curl_easy_setopt(curl.get(),
                         CURLOPT_POSTFIELDSIZE,
                         (long)postDataSize);
            }
            curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,
                     postData);
        }

        code = curl_easy_perform(curl.get());
        if (responseCode)
            curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE,
                      responseCode);

        if (code != CURLE_OK) {
            error = strlen(error_in) ? error_in
                         : curl_easy_strerror(code);
        } else if (signature) {
            for (std::string &h : header_in_list) {
                std::string name = h.substr(0, 13);
                // HTTP headers are technically case-insensitive
                if (name == "X-Signature: " ||
                    name == "x-signature: ") {
                    *signature = h.substr(13);
                    break;
                }
            }
        }

        curl_slist_free_all(header);
    }

    return code == CURLE_OK;
}
// Utils Func


/* ------------------------------------------------------------------------- */

void AFOAuth::GetAuthedData(std::string& outRefToken,
                            std::string& outRefRefreshToken,
                            uint64_t& outRefExpireTime)
{
    outRefToken = token;
    outRefRefreshToken = refresh_token;
    outRefExpireTime = expire_time;
}

bool AFOAuth::ConnectAuthedAFBase(AFBasicAuth* pValue , bool force /*= false*/)
{
    if (pConnectedAFAuth != nullptr)
        return false;
    
    auto& authManager = AFAuthManager::GetSingletonInstance();
    
    if (authManager.IsAuthed(pValue->strUuid.c_str()) == false &&
        authManager.IsCachedAuth(pValue->strUuid.c_str()) == false && !force)
        return false;
    
    pConnectedAFAuth = pValue;
    
    refresh_token = pConnectedAFAuth->strRefreshToken;
    token = pConnectedAFAuth->strAccessToken;
    expire_time = pConnectedAFAuth->uiExpireTime;
    
    return true;
}

void AFOAuth::GetConnectedAFBasicAuth(AFBasicAuth*& outRefValue)
{
    outRefValue = pConnectedAFAuth;
}

void AFOAuth::SaveInternal()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
	config_set_string(confManager.GetBasic(), service(), "RefreshToken",
                      refresh_token.c_str());
	config_set_string(confManager.GetBasic(), service(), "Token", token.c_str());
	config_set_uint(confManager.GetBasic(), service(), "ExpireTime", expire_time);
	config_set_int(confManager.GetBasic(), service(), "ScopeVer", currentScopeVer);
}

static inline std::string get_config_str(const char *section,
                                         const char *name)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
	const char *val = config_get_string(confManager.GetBasic(), section, name);
	return val ? val : "";
}

bool AFOAuth::LoadInternal()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
	refresh_token = get_config_str(service(), "RefreshToken");
	token = get_config_str(service(), "Token");
	expire_time = config_get_uint(confManager.GetBasic(), service(), "ExpireTime");
	currentScopeVer =
		(int)config_get_int(confManager.GetBasic(), service(), "ScopeVer");
	return implicit ? !token.empty() : !refresh_token.empty();
}

bool AFOAuth::TokenExpired()
{
	if (token.empty())
		return true;
	if ((uint64_t)time(nullptr) > expire_time - 5)
		return true;
	return false;
}

bool AFOAuth::GetToken(const char *url, const std::string &client_id,
		     const std::string &secret, const std::string &redirect_uri,
		     int scope_ver, const std::string &auth_code, bool retry)
{
	return GetTokenInternal(url, client_id, secret, redirect_uri, scope_ver,
				auth_code, retry);
}

bool AFOAuth::GetToken(const char *url, const std::string &client_id,
		     int scope_ver, const std::string &auth_code, bool retry)
{
	return GetTokenInternal(url, client_id, {}, {}, scope_ver, auth_code,
				retry);
}

bool AFOAuth::GetTokenInternal(const char *url, const std::string &client_id,
			     const std::string &secret,
			     const std::string &redirect_uri, int scope_ver,
			     const std::string &auth_code, bool retry)
try {
    auto& localeManaer = AFLocaleTextManager::GetSingletonInstance();
    
	std::string output;
	std::string error;
	std::string desc;

	if (currentScopeVer > 0 && currentScopeVer < scope_ver) {
		if (RetryLogin()) {
			return true;
		} else {
            QString title = QT_UTF8(localeManaer.Str("Auth.InvalidScope.Title"));
			QString text =
                QT_UTF8(localeManaer.Str("Auth.InvalidScope.Text")).arg(service());

			//QMessageBox::warning(OBSBasic::Get(), title, text);
		}
	}

	if (auth_code.empty() && !TokenExpired()) {
		return true;
	}

	std::string post_data;
	post_data += "action=redirect&client_id=";
	post_data += client_id;
	if (!secret.empty()) {
		post_data += "&client_secret=";
		post_data += secret;
	}
	if (!redirect_uri.empty()) {
		post_data += "&redirect_uri=";
		post_data += redirect_uri;
	}

	if (!auth_code.empty()) {
		post_data += "&grant_type=authorization_code&code=";
		post_data += auth_code;
	} else {
		post_data += "&grant_type=refresh_token&refresh_token=";
		post_data += refresh_token;
	}

	bool success = false;

	auto func = [&]() {
		success = GetRemoteFile(url, output, error, nullptr,
					"application/x-www-form-urlencoded", "",
					post_data.c_str(),
					std::vector<std::string>(), nullptr, 5);
	};

	ExecThreadedWithoutBlocking(func, QT_UTF8(localeManaer.Str("Auth.Authing.Title")),
                                QT_UTF8(localeManaer.Str("Auth.Authing.Text")).arg(service()));
	if (!success || output.empty())
		throw ErrorInfo("Failed to get token from remote", error);

	Json json = Json::parse(output, error);
	if (!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	/* -------------------------- */
	/* error handling             */

	error = json["error"].string_value();
	if (!retry && error == "invalid_grant") {
		if (RetryLogin()) {
			return true;
		}
	}
	if (!error.empty())
		throw ErrorInfo(error,
				json["error_description"].string_value());

	/* -------------------------- */
	/* success!                   */

	expire_time = (uint64_t)time(nullptr) + json["expires_in"].int_value();
	token = json["access_token"].string_value();
	if (token.empty())
		throw ErrorInfo("Failed to get token from remote", error);

	if (!auth_code.empty()) {
		refresh_token = json["refresh_token"].string_value();
		if (refresh_token.empty())
			throw ErrorInfo("Failed to get refresh token from "
					"remote",
					error);

		currentScopeVer = scope_ver;
	}

	return true;

} catch (ErrorInfo &info) {
    auto& localeManaer = AFLocaleTextManager::GetSingletonInstance();
    
	if (!retry) {
		QString title = QT_UTF8(localeManaer.Str("Auth.AuthFailure.Title"));
		QString text = QT_UTF8(localeManaer.Str("Auth.AuthFailure.Text"))
				       .arg(service(), info.message.c_str(),
					    info.error.c_str());

		//QMessageBox::warning(OBSBasic::Get(), title, text);
	}

	blog(LOG_WARNING, "%s: %s: %s", __FUNCTION__, info.message.c_str(),
	     info.error.c_str());
	return false;
}

void AFOAuthStreamKey::OnStreamConfig()
{
	if (key_.empty())
		return;

//	OBSBasic *main = OBSBasic::Get();
//	obs_service_t *service = main->GetService();
//
//	OBSDataAutoRelease settings = obs_service_get_settings(service);
//
//	bool bwtest = obs_data_get_bool(settings, "bwtest");
//
//	if (bwtest && strcmp(this->service(), "Twitch") == 0)
//		obs_data_set_string(settings, "key",
//				    (key_ + "?bandwidthtest=true").c_str());
//	else
//		obs_data_set_string(settings, "key", key_.c_str());
//
//	obs_service_update(service, settings);
}
