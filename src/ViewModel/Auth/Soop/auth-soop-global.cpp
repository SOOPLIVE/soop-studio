#include "auth-soop-global.h"

#include <iostream>
#include <QMessageBox>
#include <QThread>
#include <vector>
#include <QAbstractButton>
#include <QDesktopServices>
#include <QUrl>
#include <QRandomGenerator>
#include <json11.hpp>
#include <Application/CApplication.h>

#ifdef WIN32
#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "shell32")
#endif




#include "qt-wrapper.h"


#include "Utils/OBF/obf.h"

#include "ViewModel/Auth/CAuthListener.hpp"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Browser/CCefManager.h"

#include "src/PopupWindows/SettingPopup/CAddStreamWidget.h"




using namespace json11;


static const char allowedChars[] =
"null";
static const int allowedCount = static_cast<int>(sizeof(allowedChars) - 1);

/* ------------------------------------------------------------------------- */


SoopGlobalAuth::SoopGlobalAuth(const Def& d, AFAddStreamWidget* widget)
	: AFOAuthStreamKey(d),
	m_widget(widget)
{
}

SoopGlobalAuth::~SoopGlobalAuth()
{
	if (!uiLoaded)
		return;

	if (m_authListner) {
		delete m_authListner;
		m_authListner = nullptr;
	}
}

bool SoopGlobalAuth::Login()
{
	DeleteCookies();
	
	if (!m_widget)
		return false;

	m_authListner = new AFAuthListener;
	if (!m_authListner)
		return false;
	m_authListner->SetGlobalSoop();
	m_redirect_uri = QString("http://127.0.0.1:%1").arg(m_authListner->GetPort());

	QString state = GenerateState();
	m_authListner->SetState(state);

	connect(m_authListner, SIGNAL(ok(const QString&)), this, SLOT(qslotRedirect(const QString&)));
	connect(m_authListner, &AFAuthListener::fail, this, &SoopGlobalAuth::qslotClose);

	QString url_template;
	url_template += "%1";
	url_template += "&redirect_uri=%2";
	QString url = url_template.arg(SOOP_GLOBAL_AUTH_URL, m_redirect_uri);
	QCefWidget* cefWidget = m_widget->GetLoginCefWidget(nullptr, url.toStdString());
	if (!cefWidget)
		return false;
	//
	return true;
}

void SoopGlobalAuth::DeleteCookies()
{
	auto& cefManager = AFCefManager::GetSingletonInstance();
	cefManager.InitPanelCookieManager();
	QCefCookieManager* panel_cookies = cefManager.GetCefCookieManager();

	if (panel_cookies) {
		panel_cookies->DeleteCookies(service(), std::string());
	}
}

std::string SoopGlobalAuth::GetUrlProfileImg()
{
    std::string resUrl;
    resUrl.clear();
   
    
    return resUrl;
}

void SoopGlobalAuth::qslotClose()
{
	if (!m_widget)
		return;

	m_widget->close();
}

void SoopGlobalAuth::qslotRedirect(QString code)
{
	return;
}

bool SoopGlobalAuth::RetryLogin()
{
	return true;
}

void SoopGlobalAuth::SaveInternal()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
}

static inline std::string get_config_str(const char *section,
                                         const char *name)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
	const char *val = config_get_string(confManager.GetBasic(), section, name);
	return val ? val : "";
}

bool SoopGlobalAuth::LoadInternal()
{
	m_firstLoad = false;
	return AFOAuthStreamKey::LoadInternal();
}

void SoopGlobalAuth::LoadUI()
{
	if (uiLoaded)
		return;


	uiLoaded = true;
}

QString SoopGlobalAuth::GenerateState()
{
	char state[32 + 1];
	QRandomGenerator* rng = QRandomGenerator::system();
	int i;

	for (i = 0; i < 32; i++)
		state[i] = allowedChars[rng->bounded(0, allowedCount)];
	state[i] = 0;

	return state;
}