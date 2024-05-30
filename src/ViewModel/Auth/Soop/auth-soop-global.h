#pragma once

#include <string>

#include "ViewModel/Auth/COAuthLogin.hpp"

class AFAddStreamWidget;
class AFAuthListener;


static AFAuth::Def soopGlobalDef = { "SOOP Global", AFAuth::Type::OAuth_StreamKey };

/* ------------------------------------------------------------------------- */
#define SOOP_GLOBAL_AUTH_URL		""
#define SOOP_GLOBAL_STREAM_KEY_URL ""
#define SOOP_GLOBAL_RTMP_URL			"rtmp://global-stream.sooplive.com/app"
#define SOOP_GLOBAL_INFO_CHANNEL   ""
#define SOOP_GLOBAL_REFRESH_TOKEN  ""
#define SOOP_GLOBAL_CHAT_URL					""
#define SOOP_GLOBAL_DASHBOARD_URL			""
#define SOOP_GLOBAL_NEWSFEED_URL			""


class SoopGlobalAuth : public AFOAuthStreamKey {
	Q_OBJECT

public:
	SoopGlobalAuth(const Def& d, AFAddStreamWidget* widget);
	~SoopGlobalAuth();

	virtual bool Login() override;
	virtual void DeleteCookies() override;
	
	QString GenerateState();
    
    std::string GetUrlProfileImg();

private:
	virtual bool RetryLogin() override;
	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;
	virtual void LoadUI() override;

private slots:
	void qslotRedirect(QString code);
	void qslotClose();

private:
	AFAddStreamWidget* m_widget = nullptr;
	bool uiLoaded = false;
	std::string section;

	QString m_redirect_uri;

	AFAuthListener* m_authListner = nullptr;
};
