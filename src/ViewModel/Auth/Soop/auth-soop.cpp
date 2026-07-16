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

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "Utils/OBF/obf.h"

#include "ViewModel/Auth/CAuthListener.hpp"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "src/PopupWindows/SettingPopup/CAddStreamWidget.h"


using namespace json11;

/* ------------------------------------------------------------------------- */
#define SOOP_SCOPE_VERSION		1
#define SECTION_NAME			"SOOP"
/* ------------------------------------------------------------------------- */

SoopAuth::SoopAuth(const Def& d, AFAddStreamWidget* widget)
	: AFOAuthStreamKey(d),
	m_pWidget(widget)
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
	if(!m_pWidget)
		return false;

	QString url_template;
	url_template += SOOP_AUTH_URL;

	QCefWidget* cefWidget = m_pWidget->GetLoginCefWidget(nullptr, url_template.toStdString());
	if(!cefWidget)
		return false;

	connect(cefWidget, SIGNAL(urlChanged(const QString&)), this, SLOT(qslotUrlChanged(const QString&)));
	//
	return true;
}

void SoopAuth::DeleteCookies()
{
	CEFMANAGER.InitPanelCookieManager();
	QCefCookieManager* panel_cookies = CEFMANAGER.GetCefCookieManager();
	
	if(panel_cookies) {
		panel_cookies->DeleteCookies(SOOPLIVE_DOMAIN, std::string());
	}
}

bool SoopAuth::LoginForCookie(std::string id)
{
	if (!m_pWidget)
		return false;

	std::string url = SOOPLIVE_KR_LOGIN;

	QCefWidget* cefWidget = m_pWidget->GetLoginCefWidget(nullptr, url);

	if (!cefWidget)
		return false;

	connect(cefWidget, SIGNAL(cefQueryRequest(const QCefQuery&)), m_pWidget, SLOT(qslotLoginRecieved(const QCefQuery&)));
	connect(cefWidget, SIGNAL(cefMessageBoxMessage(const QString&)), m_pWidget, SLOT(qslotGetMessageFromLogin(const QString&)));
	connect(cefWidget, SIGNAL(cefBeforePopup(const QString&)), m_pWidget, SLOT(qslotGetChildPopup(const QString&)));

	cefWidget->m_SoopLogin = true;

	return true;
}

std::string SoopAuth::GetUrlProfileImg()
{
    std::string resUrl;
    resUrl.clear();
    
    
    bool success = false;
    std::string body;
    std::string output;
    std::string error;
    std::vector<std::string> headers;

    body.clear();
    output.clear();
    error.clear();

    success = GetRemoteFile(SOOP_USER_INFO_URL, output, error, nullptr,
        "application/x-www-form-urlencoded", "",
        body.c_str(),
        headers, nullptr, 10, true, body.length());
    if (!success)
        return resUrl;

    body.clear();
    output.clear();
    error.clear();   
    
    return resUrl;
}

bool SoopAuth::RefreshAccessToken(std::string refresh_token, std::string& out_access_token, int64_t& out_exires_in, std::string& out_refresh_token)
{
	std::string resUrl;
	resUrl.clear();

	bool success = false;
	std::string body;
	std::string output;
	std::string error;
	std::vector<std::string> headers;

	body.clear();
	output.clear();
	error.clear();

	success = GetRemoteFile(SOOP_TOKEN_URL, output, error, nullptr,
		"application/x-www-form-urlencoded", "",
		body.c_str(),
		headers, nullptr, 10, true, body.length());
	if (!success)
		return false;

	body.clear();
	output.clear();
	error.clear();

	return true;
}

std::string SoopAuth::RefreshCookie(std::string cookie)
{
	return "";
}

std::string SoopAuth::VodSaveAvailable(std::string cookie)
{
	std::string body;
	std::string output;
	std::string error;
	std::vector<std::string> headers;

	body.clear();
	output.clear();
	error.clear();

	bool success = GetRemoteFile(SOOP_DASHBOARD_API_URL, output, error, nullptr,
		"application/x-www-form-urlencoded", "GET",
		body.c_str(),
		headers, nullptr, 10, true, body.length());
	if (!success)
		return "";

	return output;
}

std::string SoopAuth::VodSaveRequest(std::string cookie, std::string title, std::string hashtags)
{
	QList<QVariant> values = { };
	std::string responseData = SOOP_API_HANDLER->postAPIfromId(POST_VOD_SAVE, values);

	return responseData;
}

//
void SoopAuth::qslotUrlChanged(const QString& url)
{
	return;
}

//
bool SoopAuth::RetryLogin()
{
	if(!m_pWidget)
		return false;

	QCefWidget* cefWidget = m_pWidget->GetLoginCefWidget(nullptr, SOOP_AUTH_URL);
	if(!cefWidget)
		return false;

	if(m_pWidget->exec() == QDialog::Rejected) {
		return false;
	}
	
	std::string client_id = "";
	return GetToken(SOOP_TOKEN_URL, client_id, SOOP_SCOPE_VERSION, QT_TO_UTF8(m_code), true);
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
