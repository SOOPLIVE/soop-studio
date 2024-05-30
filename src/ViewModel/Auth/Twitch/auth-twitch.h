#pragma once

#include <qtimer.h>
#include <json11.hpp>

#include "ViewModel/Auth/COAuthLogin.hpp"

class AFAddStreamWidget;

static AFAuth::Def twitchDef = {"Twitch", AFAuth::Type::OAuth_StreamKey};

#define TWITCH_AUTH_URL				"https://id.twitch.tv/oauth2/authorize"
#define TWITCH_REDIRECT_URL			""
#define TWITCH_TOKEN_URL			"https://id.twitch.tv/oauth2/token"

#define TWITCH_DASHBOARD_URL	"https://dashboard.twitch.tv/u/" // ex) https://dashboard.twitch.tv/u/twitchid/home

class TwitchAuth : public AFOAuthStreamKey {
	Q_OBJECT

public:
	TwitchAuth(const Def& d, AFAddStreamWidget* widget);
	~TwitchAuth();

	virtual bool Login() override;
	virtual void DeleteCookies() override;

    std::string GetUrlProfileImg();
    
public slots:
	void qslotUrlChanged(const QString& url);

	void TryLoadSecondaryUIPanes();
	void LoadSecondaryUIPanes();

private:
	virtual bool RetryLogin() override;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	virtual void LoadUI() override;

	bool MakeApiRequest(const char* path, json11::Json& json_out);
	bool GetChannelInfo();

	QString GetParseKey(const QString& data, const std::string& token);

private:
	AFAddStreamWidget* m_widget = nullptr;
	QTimer m_uiLoadTimer;

	bool m_uiLoaded = false;

	QString m_code;

	std::string m_name;
	std::string m_uuid;
};
