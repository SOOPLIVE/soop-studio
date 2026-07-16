#pragma once

#include <qtimer.h>
#include <json11.hpp>

#include "Common/StudioDefine.h"

#include "CoreModel/Auth/SBaseAuth.h"
#include "CoreModel/Auth/SBroadInfo.h"
#include "CoreModel/Browser/CCefManager.h"
#include "CoreModel/Auth/CAuthManager.h"

#include "ViewModel/Auth/COAuthLogin.hpp"

class AFAddStreamWidget;

static AFAuth::Def soopDef = { PLATFORM_SOOP, AFAuth::Type::OAuth_StreamKey};

class SoopAuth : public AFOAuthStreamKey {
	Q_OBJECT

public:
	SoopAuth(const Def& d, AFAddStreamWidget* widget);
	~SoopAuth();

	virtual bool Login() override;
	virtual void DeleteCookies() override;
	bool LoginForCookie(std::string id = "");
    
    std::string GetUrlProfileImg();
	bool RefreshAccessToken(std::string refresh_token, 
		std::string& out_access_token,
		int64_t& out_exires_in,
		std::string& out_refresh_token);
	std::string RefreshCookie(std::string cookie);
	std::string VodSaveAvailable(std::string cookie);
	std::string VodSaveRequest(std::string cookie, std::string title, std::string hashtags);

public slots:
	void qslotUrlChanged(const QString& url);

private:
	virtual bool RetryLogin() override;

	virtual void LoadUI() override;

	QString GetParseKey(const QString& data, const std::string& token);

private:
	AFAddStreamWidget* m_pWidget = nullptr;
	QTimer m_uiLoadTimer;

	bool m_uiLoaded = false;

	QString m_code;
};
