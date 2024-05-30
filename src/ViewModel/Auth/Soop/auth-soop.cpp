#include "auth-soop.hpp"

#include <iostream>
#include <QMessageBox>
#include <QThread>
#include <vector>
#include <QAbstractButton>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QUrl>
#include <QRandomGenerator>

#ifdef WIN32
#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "shell32")
#endif




#include "Application/CApplication.h"
#include "qt-wrapper.h"


#include "Utils/OBF/obf.h"

#include "ViewModel/Auth/CAuthListener.hpp"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "src/PopupWindows/SettingPopup/CAddStreamWidget.h"




using namespace json11;

/* ------------------------------------------------------------------------- */
#define SOOP_CLIENTID			""
#define SOOP_SECRETID			""
//
#define SOOP_RTMP_URL			"rtmp://rtmpmanager-freecat.afreeca.tv/app/"
#define SOOP_SCOPE_VERSION		1
#define SECTION_NAME			"afreecaTV"

/* ------------------------------------------------------------------------- */

SoopAuth::SoopAuth(const Def& d, AFAddStreamWidget* widget)
	: AFOAuthStreamKey(d),
	m_widget(widget)
{
}

SoopAuth::~SoopAuth()
{
	if(!m_uiLoaded)
		return;
}

bool SoopAuth::Login()
{
	DeleteCookies();
	//
	if(!m_widget)
		return false;

	QString url_template;
	url_template += "%1";
	url_template += "?client_id=%2";
	url_template += "&callbackCode=";
	QString url = url_template.arg(SOOP_AUTH_URL, SOOP_CLIENTID);

	QCefWidget* cefWidget = m_widget->GetLoginCefWidget(nullptr, url.toStdString());
	if(!cefWidget)
		return false;

	connect(cefWidget, SIGNAL(urlChanged(const QString&)), this, SLOT(qslotUrlChanged(const QString&)));
	//
	return true;
}
void SoopAuth::DeleteCookies()
{
	auto& cefManager = AFCefManager::GetSingletonInstance();
	cefManager.InitPanelCookieManager();
	QCefCookieManager* panel_cookies = cefManager.GetCefCookieManager();
	
	if(panel_cookies) {
		panel_cookies->DeleteCookies("afreecatv.com", std::string());
	}
}

std::string SoopAuth::GetUrlProfileImg()
{
    std::string resUrl;
    resUrl.clear();
    
    
    return resUrl;
}

//
void SoopAuth::qslotUrlChanged(const QString& url)
{
	return;
}

//
bool SoopAuth::RetryLogin()
{
	if(!m_widget)
		return false;

	QCefWidget* cefWidget = m_widget->GetLoginCefWidget(nullptr, SOOP_AUTH_URL);
	if(!cefWidget)
		return false;

	if(m_widget->exec() == QDialog::Rejected) {
		return false;
	}

	std::string client_id = SOOP_CLIENTID;
	//deobfuscate_str(&client_id[0], SOOP_HASH);

	return GetToken(SOOP_TOKEN_URL, client_id, SOOP_SCOPE_VERSION,
			QT_TO_UTF8(m_code), true);
}

void SoopAuth::LoadUI()
{
	if(m_uiLoaded)
		return;


	m_uiLoaded = true;
}

QString SoopAuth::GetParseKey(const QString& data, const std::string& token)
{
	QString parseKey;
	int code_idx = data.indexOf(token.c_str());
	if(code_idx == -1)
		return parseKey;

	code_idx += (int)token.size();
	int next_idx = data.indexOf("&", code_idx);
	if(next_idx != -1)
		parseKey = data.mid(code_idx, next_idx - code_idx);
	else
		parseKey = data.right(data.size() - code_idx);
	return parseKey;
}
