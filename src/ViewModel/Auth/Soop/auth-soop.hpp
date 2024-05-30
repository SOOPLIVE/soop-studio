#pragma once

#include <qtimer.h>
#include <json11.hpp>

#include "ViewModel/Auth/COAuthLogin.hpp"

class AFAddStreamWidget;

static AFAuth::Def soopDef = {"afreecaTV", AFAuth::Type::OAuth_StreamKey};

#define SOOP_AUTH_URL			""
#define SOOP_TOKEN_URL			""
#define SOOP_USER_INFO_URL		""
#define SOOP_STREAMKEY_URL		""

#define SOOP_DASHBOARD_URL		""

class SoopAuth : public AFOAuthStreamKey {
	Q_OBJECT

public:
	SoopAuth(const Def& d, AFAddStreamWidget* widget);
	~SoopAuth();

	virtual bool Login() override;
	virtual void DeleteCookies() override;
    
    std::string GetUrlProfileImg();

public slots:
	void qslotUrlChanged(const QString& url);

private:
	virtual bool RetryLogin() override;

	virtual void LoadUI() override;

	QString GetParseKey(const QString& data, const std::string& token);

private:
	AFAddStreamWidget* m_widget = nullptr;
	QTimer m_uiLoadTimer;

	bool m_uiLoaded = false;

	QString m_code;
};
