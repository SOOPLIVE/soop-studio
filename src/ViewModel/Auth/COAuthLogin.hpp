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
    AFBasicAuth* m_pConnectedAFAuth = nullptr;
	std::string m_refreshToken;
	std::string m_token;
	bool m_implicit = false;
	uint64_t m_expireTime = 0;
	int m_currentScopeVer = 0;

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
	std::string m_key;

public:
	inline AFOAuthStreamKey(const Def &d) : AFOAuth(d) {}

	inline const std::string &key() const { return m_key; }

	virtual void OnStreamConfig() override;
};
