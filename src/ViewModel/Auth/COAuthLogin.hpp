#pragma once

#include <QDialog>
#include <string>
#include <memory>

#include "CAuth.h"


class AFBasicAuth;
class QCefWidget;

class AFOAuth : public AFAuth {
	Q_OBJECT

public:
	inline AFOAuth(const Def &d) : AFAuth(d) {}

	virtual bool Login() { return false; }
	virtual void DeleteCookies() {}
    
    void GetAuthedData(std::string& outRefToken,
                       std::string& outRefRefreshToken,
                       uint64_t& outRefExpireTime);
    bool ConnectAuthedAFBase(AFBasicAuth* pValue, bool force = false);
    void GetConnectedAFBasicAuth(AFBasicAuth*& outRefValue);

protected:
    AFBasicAuth* pConnectedAFAuth = nullptr;
	std::string refresh_token;
	std::string token;
	bool implicit = false;
	uint64_t expire_time = 0;
	int currentScopeVer = 0;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	virtual bool RetryLogin() = 0;
	bool TokenExpired();
	bool GetToken(const char *url, const std::string &client_id,
		      int scope_ver,
		      const std::string &auth_code = std::string(),
		      bool retry = false);
	bool GetToken(const char *url, const std::string &client_id,
		      const std::string &secret,
		      const std::string &redirect_uri, int scope_ver,
		      const std::string &auth_code, bool retry);

private:
	bool GetTokenInternal(const char *url, const std::string &client_id,
			      const std::string &secret,
			      const std::string &redirect_uri, int scope_ver,
			      const std::string &auth_code, bool retry);
};

class AFOAuthStreamKey : public AFOAuth {
	Q_OBJECT

protected:
	std::string key_;

public:
	inline AFOAuthStreamKey(const Def &d) : AFOAuth(d) {}

	inline const std::string &key() const { return key_; }

	virtual void OnStreamConfig() override;
};


bool GetRemoteFile(const char *url, std::string &str, std::string &error,
                   long *responseCode = nullptr, const char *contentType = nullptr,
                   std::string request_type = "", const char *postData = nullptr,
                   std::vector<std::string> extraHeaders = std::vector<std::string>(),
                   std::string *signature = nullptr, int timeoutSec = 0,
                   bool fail_on_error = true, int postDataSize = 0);
