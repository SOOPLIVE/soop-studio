#include "auth-twitch.h"

#include <iostream>
#include <QMessageBox>
#include <QThread>
#include <vector>
#include <QAbstractButton>
#include <QDesktopServices>
#include <QUrl>
#include <QRandomGenerator>
#include <json11.hpp>

#ifdef WIN32
#include <windows.h>
#include <shellapi.h>

#pragma comment(lib, "shell32")
#endif

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "Utils/OBF/obf.h"

#include "ViewModel/Auth/CAuthListener.hpp"

#include "CoreModel/Auth/SBaseAuth.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Browser/CCefManager.h"

#include "src/PopupWindows/SettingPopup/CAddStreamWidget.h"




using namespace json11;


TwitchAuth::TwitchAuth(const Def& d, AFAddStreamWidget* widget)
	: AFOAuthStreamKey(d),
	m_pWidget(widget)
{
	QCef* cef = CEFMANAGER.GetCef();
	if(!cef)
		return;

	cef->add_popup_whitelist_url(
		"https://twitch.tv/popout/frankerfacez/chat?ffz-settings",
		this);

	/* enables javascript-based popups.  basically bttv popups */
	cef->add_popup_whitelist_url("about:blank#blocked", this);

	m_uiLoadTimer.setSingleShot(true);
	m_uiLoadTimer.setInterval(500);
	connect(&m_uiLoadTimer, &QTimer::timeout, this, &TwitchAuth::TryLoadSecondaryUIPanes);
}

TwitchAuth::~TwitchAuth()
{
	if (!m_uiLoaded)
		return;
}

bool TwitchAuth::Login()
{
	//DeleteCookies();
	//
	if(!m_pWidget)
		return false;

	QString url_template;
	url_template += "%1";
	url_template += "?response_type=code";
	url_template += "&client_id=%2";
	url_template += "&redirect_uri=%3";
	url_template += "&scope=channel:manage:broadcast+chat:edit+channel:read:stream_key";
	
	QString url = url_template.arg(TWITCH_AUTH_URL, TWITCH_CLIENTID, TWITCH_REDIRECT_URL);

	QCefWidget* cefWidget = m_pWidget->GetLoginCefWidget(nullptr, url.toStdString());
	if(!cefWidget)
		return false;
	
	connect(cefWidget, SIGNAL(urlChanged(const QString&)), this, SLOT(qslotUrlChanged(const QString&)));

	cefWidget->setFocusPolicy(Qt::StrongFocus);
	cefWidget->raise();
	cefWidget->setFocus(Qt::ActiveWindowFocusReason);
	//
	return true;
}
void TwitchAuth::DeleteCookies()
{
	CEFMANAGER.InitPanelCookieManager();
	QCefCookieManager* panel_cookies = CEFMANAGER.GetCefCookieManager();
	if(panel_cookies) {
		panel_cookies->DeleteCookies("twitch.tv", std::string());
	}
}

//
void TwitchAuth::qslotUrlChanged(const QString& url)
{
	if (!m_pWidget)
		return;

	if (url.contains("error=access_denied") || !GetParseKey(url, std::string("error=")).isEmpty()) {
		m_pWidget->reject();
		return;
	}

	std::string access_token;
	std::string refresh_token;
	std::string channel_id;
	std::string id;
	std::string nick;

	QString parseCode = GetParseKey(url, std::string("code="));
	if (parseCode.isEmpty())
		return;

	std::string auth_code = parseCode.toStdString();

	std::string token_body;
	token_body = "grant_type=authorization_code";
	token_body += "&code=" + auth_code;
	token_body += "&client_id=" + std::string(TWITCH_CLIENTID);
	token_body += "&client_secret=" + std::string(TWITCH_CLIENT_SECRET);
	token_body += "&redirect_uri=" + std::string(TWITCH_REDIRECT_URL);

	std::string token_output;
	std::string token_error;
	std::vector<std::string> token_headers;

	bool token_success = GetRemoteFile(TWITCH_TOKEN_URL, token_output, token_error, nullptr,
		"application/x-www-form-urlencoded", "",
		token_body.c_str(),
		token_headers, nullptr, 10, true, token_body.length());

	if (!token_success) {
		blog(LOG_WARNING, "Twitch token exchange failed: %s", token_error.c_str());
		return;
	}

	m_pWidget->SetAuthData(access_token, refresh_token,
						   0, id, nick,
                           m_key,
						   TWITCH_RTM_URL);
}

std::string TwitchAuth::GetUrlProfileImg()
{
	GetChannelInfo();

    std::string resUrl;
    resUrl.clear();
    
    AFBasicAuth* pConnectedAuthData = nullptr;
    GetConnectedAFBasicAuth(pConnectedAuthData);
    
    return resUrl;
}

bool TwitchAuth::SendChatMessage(std::string message)
{
	std::string client_id = TWITCH_CLIENTID;
	//deobfuscate_str(&client_id[0], TWITCH_HASH);

	if (!GetToken(TWITCH_TOKEN_URL, client_id, TWITCH_SCOPE_VERSION))
		return false;
	if (m_token.empty())
		return false;
	
	Json json;
	bool success = MakeApiRequest("users", json);
	if (!success)
		return false;

	std::string _body;
	std::string _output;
	std::string _error;

	_body = "broadcaster_id=" + json["data"][0]["id"].string_value();
	_body += "&sender_id=" + json["data"][0]["id"].string_value();
	_body += "&message=" + message;

	std::vector<std::string> _headers;
	_headers.push_back(std::string("Client-ID: ") + client_id);
	_headers.push_back(std::string("Authorization: Bearer ") + m_token);
	//_headers.push_back(std::string("Content-Type: application/json"));

	bool _success = GetRemoteFile("https://api.twitch.tv/helix/chat/messages", _output, _error, nullptr,
		"application/x-www-form-urlencoded", "",
		_body.c_str(),
		_headers, nullptr, 10, true, _body.length());

	if (!success)
		return false;

	return true;
}

//
/* Twitch.tv has an OAuth for itself.  If we try to load multiple panel pages
 * at once before it's OAuth'ed itself, they will all try to perform the auth
 * process at the same time, get their own request codes, and only the last
 * code will be valid -- so one or more panels are guaranteed to fail.
 *
 * To solve this, we want to load just one panel first (the chat), and then all
 * subsequent panels should only be loaded once we know that Twitch has auth'ed
 * itself (if the cookie "auth-token" exists for twitch.tv).
 *
 * This is annoying to deal with. */
void TwitchAuth::TryLoadSecondaryUIPanes()
{
	QPointer<TwitchAuth> this_ = this;

	auto cb = [this_](bool found) {
		if(!this_) {
			return;
		}

		if(!found) {
			QMetaObject::invokeMethod(&this_->m_uiLoadTimer, "start");
		} else {
			QMetaObject::invokeMethod(this_,
						  "LoadSecondaryUIPanes");
		}
	};

	QCefCookieManager* panel_cookies = CEFMANAGER.GetCefCookieManager();
	panel_cookies->CheckForCookie("https://www.twitch.tv", "auth-token", cb);
}
void TwitchAuth::LoadSecondaryUIPanes()
{
	//OBSBasic* main = OBSBasic::Get();

	//QCefWidget* browser;
	//std::string url;
	//std::string script;

	//QSize size = main->frameSize();
	//QPoint pos = main->pos();

	//if(App()->IsThemeDark()) {
	//	script = "localStorage.setItem('twilight.theme', 1);";
	//} else {
	//	script = "localStorage.setItem('twilight.theme', 0);";
	//}
	//script += referrer_script1;
	//script += "https://www.twitch.tv/";
	//script += name;
	//script += "/dashboard/live";
	//script += referrer_script2;

	//const int twAddonChoice =
	//	config_get_int(main->Config(), service(), "AddonChoice");
	//if(twAddonChoice) {
	//	if(twAddonChoice & 0x1)
	//		script += bttv_script;
	//	if(twAddonChoice & 0x2)
	//		script += ffz_script;
	//}

	///* ----------------------------------- */

	//url = "https://dashboard.twitch.tv/popout/u/";
	//url += name;
	//url += "/stream-manager/edit-stream-info";

	//BrowserDock* info = new BrowserDock(QTStr("Auth.StreamInfo"));
	//info->setObjectName(TWITCH_INFO_DOCK_NAME);
	//info->resize(300, 650);
	//info->setMinimumSize(200, 300);
	//info->setWindowTitle(QTStr("Auth.StreamInfo"));
	//info->setAllowedAreas(Qt::AllDockWidgetAreas);

	//browser = cef->create_widget(info, url, panel_cookies);
	//info->SetWidget(browser);
	//browser->setStartupScript(script);

	//main->AddDockWidget(info, Qt::RightDockWidgetArea);

	///* ----------------------------------- */

	//url = "https://www.twitch.tv/popout/";
	//url += name;
	//url += "/dashboard/live/stats";

	//BrowserDock* stats = new BrowserDock(QTStr("TwitchAuth.Stats"));
	//stats->setObjectName(TWITCH_STATS_DOCK_NAME);
	//stats->resize(200, 250);
	//stats->setMinimumSize(200, 150);
	//stats->setWindowTitle(QTStr("TwitchAuth.Stats"));
	//stats->setAllowedAreas(Qt::AllDockWidgetAreas);

	//browser = cef->create_widget(stats, url, panel_cookies);
	//stats->SetWidget(browser);
	//browser->setStartupScript(script);

	//main->AddDockWidget(stats, Qt::RightDockWidgetArea);

	///* ----------------------------------- */

	//url = "https://dashboard.twitch.tv/popout/u/";
	//url += name;
	//url += "/stream-manager/activity-feed";
	//url += "?uuid=" + uuid;

	//BrowserDock* feed = new BrowserDock(QTStr("TwitchAuth.Feed"));
	//feed->setObjectName(TWITCH_FEED_DOCK_NAME);
	//feed->resize(300, 650);
	//feed->setMinimumSize(200, 300);
	//feed->setWindowTitle(QTStr("TwitchAuth.Feed"));
	//feed->setAllowedAreas(Qt::AllDockWidgetAreas);

	//browser = cef->create_widget(feed, url, panel_cookies);
	//feed->SetWidget(browser);
	//browser->setStartupScript(script);

	//main->AddDockWidget(feed, Qt::RightDockWidgetArea);

	///* ----------------------------------- */

	//info->setFloating(true);
	//stats->setFloating(true);
	//feed->setFloating(true);

	//QSize statSize = stats->frameSize();

	//info->move(pos.x() + 50, pos.y() + 50);
	//stats->move(pos.x() + size.width() / 2 - statSize.width() / 2,
	//		pos.y() + size.height() / 2 - statSize.height() / 2);
	//feed->move(pos.x() + 100, pos.y() + 100);

	//if(firstLoad) {
	//	info->setVisible(true);
	//	stats->setVisible(false);
	//	feed->setVisible(false);
	//} else {
	//	uint32_t lastVersion = config_get_int(APPCONFIG, "General", "LastVersion");
	//	if(lastVersion <= MAKE_SEMANTIC_VERSION(23, 0, 2)) {
	//		feed->setVisible(false);
	//	}

	//	const char* dockStateStr = config_get_string(USERCONFIG, service(), "DockState");
	//	QByteArray dockState = QByteArray::fromBase64(QByteArray(dockStateStr));

	//	if(main->isVisible() || !main->isMaximized())
	//		main->restoreState(dockState);
	//}
}

//
bool TwitchAuth::RetryLogin()
{
	if(!m_pWidget)
		return false;

	QCefWidget* cefWidget = m_pWidget->GetLoginCefWidget(nullptr, TWITCH_AUTH_URL);
	if(!cefWidget)
		return false;

	if(m_pWidget->exec() == QDialog::Rejected) {
		return false;
	}

	std::string client_id = TWITCH_CLIENTID;
	//deobfuscate_str(&client_id[0], TWITCH_HASH);

	return GetToken(TWITCH_TOKEN_URL, client_id, TWITCH_SCOPE_VERSION,
			QT_TO_UTF8(m_code), true);
}

void TwitchAuth::SaveInternal()
{
	auto activeConfig = ACTIVECONFIG;
	//
	config_set_string(activeConfig, service(), "Name", m_name.c_str());
	config_set_string(activeConfig, service(), "UUID", m_uuid.c_str());

	if(m_uiLoaded) {
		config_set_string(activeConfig, service(), "DockState",
						  DYNAMIC_COMPOSIT->saveState().toBase64().constData());
	}
	AFOAuthStreamKey::SaveInternal();
}
bool TwitchAuth::LoadInternal()
{
	QCef* cef = CEFMANAGER.GetCef();
	if(!cef)
		return false;

	m_name = config_get_string(ACTIVECONFIG, service(), "Name");
	m_uuid = config_get_string(ACTIVECONFIG, service(), "UUID");

	m_firstLoad = false;
	return AFOAuthStreamKey::LoadInternal();
}

void TwitchAuth::LoadUI()
{
	if (m_uiLoaded)
		return;


	m_uiLoaded = true;
}

bool TwitchAuth::MakeApiRequest(const char* path, json11::Json& json_out)
{
	std::string client_id = TWITCH_CLIENTID;
	//deobfuscate_str(&client_id[0], TWITCH_HASH);

	std::string url = "https://api.twitch.tv/helix/";
	url += std::string(path);

	std::vector<std::string> headers;
	headers.push_back(std::string("Client-ID: ") + client_id);
	headers.push_back(std::string("Authorization: Bearer ") + m_token);

	std::string output;
	std::string error;
	long error_code = 0;

	bool success = false;

	auto func = [&]() {
		success = GetRemoteFile(url.c_str(), output, error, &error_code,
					"application/json", "", nullptr,
					headers, nullptr, 5);
	};

	ExecThreadedWithoutBlocking(
		func, QTStr("Auth.LoadingChannel.Title"),
		QTStr("Auth.LoadingChannel.Text").arg(service()));
	if(error_code == 403) {
		/*OBSMessageBox::warning(OBSBasic::Get(),
					   Str("TwitchAuth.TwoFactorFail.Title"),
					   Str("TwitchAuth.TwoFactorFail.Text"),
					   true);*/
		blog(LOG_WARNING, "%s: %s. API response: %s", __FUNCTION__,
			 "Got 403 from Twitch, user probably does not "
			 "have two-factor authentication enabled on "
			 "their account",
			 output.empty() ? "<none>" : output.c_str());
		return false;
	}

	if(!success || output.empty())
		throw ErrorInfo("Failed to get text from remote", error);

	json_out = Json::parse(output, error);
	if(!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	error = json_out["error"].string_value();
	if(!error.empty())
		throw ErrorInfo(error, json_out["message"].string_value());

	return true;
}

bool TwitchAuth::GetChannelInfo()
try {
	std::string client_id = TWITCH_CLIENTID;
	//deobfuscate_str(&client_id[0], TWITCH_HASH);

	if(!GetToken(TWITCH_TOKEN_URL, client_id, TWITCH_SCOPE_VERSION))
		return false;
	if(m_token.empty())
		return false;
	if(!m_key.empty())
		return true;

	Json json;
	bool success = MakeApiRequest("users", json);

	if(!success)
		return false;

	m_name = json["data"][0]["login"].string_value();

	std::string path = "streams/key?broadcaster_id=" +
		json["data"][0]["id"].string_value();
	success = MakeApiRequest(path.c_str(), json);
	if(!success)
		return false;

    m_key = json["data"][0]["stream_key"].string_value();

	return true;

} catch(ErrorInfo info) {
	QString title = QTStr("Auth.ChannelFailure.Title");
	QString text = QTStr("Auth.ChannelFailure.Text")
		.arg(service(), info.message.c_str(),
		 info.error.c_str());

	//QMessageBox::warning(OBSBasic::Get(), title, text);

	blog(LOG_WARNING, "%s: %s: %s", __FUNCTION__, info.message.c_str(),
		 info.error.c_str());
	return false;
}

QString TwitchAuth::GetParseKey(const QString &data, const std::string &token)
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
