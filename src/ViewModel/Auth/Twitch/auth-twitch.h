#pragma once

#include <qtimer.h>
#include <json11.hpp>

#include "Common/StudioDefine.h"

#include "CoreModel/Auth/SBaseAuth.h"
#include "CoreModel/Auth/CAuthManager.h"

#include "ViewModel/Auth/COAuthLogin.hpp"

class AFAddStreamWidget;

static AFAuth::Def twitchDef = { PLATFORM_TWITCH, AFAuth::Type::OAuth_StreamKey};


class TwitchAuth : public AFOAuthStreamKey {
	Q_OBJECT

public:
	TwitchAuth(const Def& d, AFAddStreamWidget* widget);
	~TwitchAuth();

	virtual bool Login() override;
	virtual void DeleteCookies() override;

    std::string GetUrlProfileImg();
	bool SendChatMessage(std::string message);
    
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
	AFAddStreamWidget* m_pWidget = nullptr;
	QTimer m_uiLoadTimer;

	bool m_uiLoaded = false;

	QString m_code;

	std::string m_name;
	std::string m_uuid;
};
