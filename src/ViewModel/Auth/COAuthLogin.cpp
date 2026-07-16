#include "COAuthLogin.hpp"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Browser/CCefManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"


#include <unordered_map>

#include <json11.hpp>


using namespace json11;



/* ------------------------------------------------------------------------- */

void AFOAuth::GetAuthedData(std::string& outRefToken,
                            std::string& outRefRefreshToken,
                            uint64_t& outRefExpireTime)
{
    outRefToken = m_token;
    outRefRefreshToken = m_refreshToken;
    outRefExpireTime = m_expireTime;
}

bool AFOAuth::ConnectAuthedAFBase(AFBasicAuth* pValue , bool force /*= false*/)
{
    if (m_pConnectedAFAuth != nullptr)
        return false;
    
    if (AUTH_CONTEXT.IsAuthed(pValue->uuid.c_str()) == false &&
		AUTH_CONTEXT.IsCachedAuth(pValue->uuid.c_str()) == false && !force)
        return false;
    
    m_pConnectedAFAuth = pValue;
    
    m_refreshToken = m_pConnectedAFAuth->refreshToken;
    m_token = m_pConnectedAFAuth->accessToken;
    m_expireTime = m_pConnectedAFAuth->expireTime;
    
    return true;
}

void AFOAuth::GetConnectedAFBasicAuth(AFBasicAuth*& outRefValue)
{
    outRefValue = m_pConnectedAFAuth;
}

void AFOAuth::SaveInternal()
{
	auto activeConfig = ACTIVECONFIG;
	//
	config_set_string(activeConfig, service(), "RefreshToken", m_refreshToken.c_str());
	config_set_string(activeConfig, service(), "Token", m_token.c_str());
	config_set_uint(activeConfig, service(), "ExpireTime", m_expireTime);
	config_set_int(activeConfig, service(), "ScopeVer", m_currentScopeVer);
}

static inline std::string get_config_str(const char *section,
                                         const char *name)
{
	const char *val = config_get_string(ACTIVECONFIG, section, name);
	return val ? val : "";
}

bool AFOAuth::LoadInternal()
{
	m_refreshToken = get_config_str(service(), "RefreshToken");
	m_token = get_config_str(service(), "Token");
	m_expireTime = config_get_uint(ACTIVECONFIG, service(), "ExpireTime");
	m_currentScopeVer = (int)config_get_int(ACTIVECONFIG, service(), "ScopeVer");
	return m_implicit ? !m_token.empty() : !m_refreshToken.empty();
}

bool AFOAuth::TokenExpired()
{
	if (m_token.empty())
		return true;
	if ((uint64_t)time(nullptr) > m_expireTime - 5)
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
	std::string output;
	std::string error;
	std::string desc;

	if (m_currentScopeVer > 0 && m_currentScopeVer < scope_ver) {
		if (RetryLogin()) {
			return true;
		} else {
            QString title = QTStr("Auth.InvalidScope.Title");
			QString text =  QTStr("Auth.InvalidScope.Text").arg(service());
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
		post_data += m_refreshToken;
	}

	bool success = false;

	auto func = [&]() {
		success = GetRemoteFile(url, output, error, nullptr,
					"application/x-www-form-urlencoded", "",
					post_data.c_str(),
					std::vector<std::string>(), nullptr, 5);
	};

	ExecThreadedWithoutBlocking(func, QTStr("Auth.Authing.Title"), QTStr("Auth.Authing.Text").arg(service()));
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

	m_expireTime = (uint64_t)time(nullptr) + json["expires_in"].int_value();
	m_token = json["access_token"].string_value();
	if (m_token.empty())
		throw ErrorInfo("Failed to get token from remote", error);

	if (!auth_code.empty()) {
		m_refreshToken = json["refresh_token"].string_value();
		if (m_refreshToken.empty())
			throw ErrorInfo("Failed to get refresh token from "
					"remote",
					error);

		m_currentScopeVer = scope_ver;
	}

	return true;

} catch (ErrorInfo &info) {

	if (!retry) {
		QString title = QTStr("Auth.AuthFailure.Title");
		QString text = QTStr("Auth.AuthFailure.Text")
			.arg(service(), info.message.c_str(),
				info.error.c_str());
	}

	blog(LOG_WARNING, "%s: %s: %s", __FUNCTION__, info.message.c_str(),
		info.error.c_str());
	return false;
}

void AFOAuthStreamKey::OnStreamConfig()
{
	if (m_key.empty())
		return;
}
