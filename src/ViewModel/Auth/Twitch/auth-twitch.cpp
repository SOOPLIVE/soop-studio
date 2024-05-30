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



#include "Application/CApplication.h"
#include "qt-wrapper.h"


#include "Utils/OBF/obf.h"

#include "ViewModel/Auth/CAuthListener.hpp"

#include "CoreModel/Auth/SBaseAuth.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Browser/CCefManager.h"

#include "src/PopupWindows/SettingPopup/CAddStreamWidget.h"




using namespace json11;

/* ------------------------------------------------------------------------- */
#define TWITCH_CLIENTID				""
#define TWITCH_CLIENT_SECRET        ""
//
#define TWITCH_HASH					0x0
#define TWITCH_SCOPE_VERSION		1

#define TWITCH_RTM_URL				"rtmp://live.twitch.tv/app"

#define TWITCH_CHAT_DOCK_NAME		"twitchChat"
#define TWITCH_INFO_DOCK_NAME		"twitchInfo"
#define TWITCH_STATS_DOCK_NAME		"twitchStats"
#define TWITCH_FEED_DOCK_NAME		"twitchFeed"

TwitchAuth::TwitchAuth(const Def& d, AFAddStreamWidget* widget)
	: AFOAuthStreamKey(d),
	m_widget(widget)
{
	QCef* cef = AFCefManager::GetSingletonInstance().GetCef();
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
	DeleteCookies();
	//
	if(!m_widget)
		return false;

	QString url_template;
	url_template += "%1";
	url_template += "?response_type=code";
	url_template += "&client_id=%2";
	url_template += "&redirect_uri=%3";
	url_template += "&scope=channel:manage:polls+channel:read:polls+channel:read:stream_key";
	QString url = url_template.arg(TWITCH_AUTH_URL, TWITCH_CLIENTID, TWITCH_REDIRECT_URL);

	QCefWidget* cefWidget = m_widget->GetLoginCefWidget(nullptr, url.toStdString());
	if(!cefWidget)
		return false;

	connect(cefWidget, SIGNAL(urlChanged(const QString&)), this, SLOT(qslotUrlChanged(const QString&)));
	//
	return true;
}
void TwitchAuth::DeleteCookies()
{
	auto& cefManager = AFCefManager::GetSingletonInstance();
	cefManager.InitPanelCookieManager();
	QCefCookieManager* panel_cookies = cefManager.GetCefCookieManager();

	if(panel_cookies) {
		panel_cookies->DeleteCookies("twitch.tv", std::string());
	}
}

//
void TwitchAuth::qslotUrlChanged(const QString& url)
{
	if(!m_widget)
		return;

	std::string access_token;
	std::string refresh_token;
	uint64_t expire_time = 0;
	std::string id;
	std::string channel_id;
	std::string nick;
	std::string stream_url;

	// access_token
	QString parseKey = GetParseKey(url, std::string("access_token="));
	if(parseKey.isEmpty())
		return;

	access_token = parseKey.toStdString();

	// refresh_token
	parseKey = GetParseKey(url, std::string("refresh_token="));
	if(parseKey.isEmpty())
		return;

	refresh_token = parseKey.toStdString();

	m_code = access_token.c_str();


	std::string output;
	std::string error;
	std::string desc;

	bool success = false;


	std::vector<std::string> headers;
	headers.push_back(std::string("Client-ID: ") + TWITCH_CLIENTID);
	headers.push_back(std::string("Authorization: Bearer ") + m_code.toStdString());

	// user info
	success = GetRemoteFile("https://api.twitch.tv/helix/users", output, error, nullptr,
							"application/json", "",
							nullptr,
							headers, nullptr, 10, true, 0);

	json11::Json json = json11::Json::parse(output, error);
	if(!error.empty())
		throw ErrorInfo("Failed to parse json", error);

	if(!json["data"].is_array() ||
	   !json["data"][0]["id"].is_string()) {
		// error
		return;
	}

	channel_id = json["data"][0]["id"].string_value();
	id = json["data"][0]["login"].string_value();
	nick = json["data"][0]["display_name"].string_value();

	output.clear();
	error.clear();

	// stream key
	std::string strUrl = "https://api.twitch.tv/helix/streams/key?broadcaster_id=";
	strUrl += channel_id;

	success = GetRemoteFile(strUrl.c_str(), output, error, nullptr,
							"application/json", "",
							nullptr,
							headers, nullptr, 10, true, 0);

	json = json11::Json::parse(output, error);
	output.clear();
	error.clear();

	if(!json["data"].is_array() ||
	   !json["data"][0]["stream_key"].is_string()) {
		// error
		return;
	}

	key_ = json["data"][0]["stream_key"].string_value();

	m_widget->SetAuthData(access_token, refresh_token,
						  0, id, nick,
						  key_,
						  TWITCH_RTM_URL);
}

std::string TwitchAuth::GetUrlProfileImg()
{
    std::string resUrl;
    resUrl.clear();
    
    AFBasicAuth* pConnectedAuthData = nullptr;
    GetConnectedAFBasicAuth(pConnectedAuthData);
    
    // Refresh Token
    if (pConnectedAuthData != nullptr)
    {
        std::string _body;
        std::string _output;
        std::string _error;
        
        
        _body = "grant_type=refresh_token&refresh_token=";
        _body += pConnectedAuthData->strRefreshToken;
        _body += "&client_id=";
        _body += TWITCH_CLIENTID;
        _body += "&client_secret=";
        _body += TWITCH_CLIENT_SECRET;
        
        
        std::vector<std::string> _headers;
        bool _success = GetRemoteFile(TWITCH_TOKEN_URL, _output, _error, nullptr,
                                      "application/x-www-form-urlencoded", "",
                                      _body.c_str(),
                                      _headers, nullptr, 10, true, _body.length());

        json11::Json _json = json11::Json::parse(_output, _error);
        
        if (_success)
        {
            pConnectedAuthData->strAccessToken = _json["access_token"].string_value();
            pConnectedAuthData->strRefreshToken = _json["refresh_token"].string_value();
            token = pConnectedAuthData->strAccessToken;
        }
    }
    
    
    
    bool success = false;
    std::string output;
    std::string error;
    
    std::vector<std::string> headers;
    headers.push_back(std::string("Client-ID: ") + TWITCH_CLIENTID);
    headers.push_back(std::string("Authorization: Bearer ") + token);


    // token
    success = GetRemoteFile("https://api.twitch.tv/helix/users", output, error, nullptr,
                            "application/json", "",
                            nullptr,
                            headers, nullptr, 10, true, 0);

    if (!success)
        return resUrl;

    json11::Json json = json11::Json::parse(output, error);

    if(!error.empty())
        throw ErrorInfo("Failed to parse json", error);

    if(!json["data"].is_array() ||
       !json["data"][0]["id"].is_string()) {
        // error
        return resUrl;
    }
    

    resUrl = json["data"][0]["profile_image_url"].string_value();
    
    
    return resUrl;
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

	QCefCookieManager* panel_cookies = AFCefManager::GetSingletonInstance().GetCefCookieManager();
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
	//	uint32_t lastVersion = config_get_int(App()->GlobalConfig(),
	//						  "General", "LastVersion");

	//	if(lastVersion <= MAKE_SEMANTIC_VERSION(23, 0, 2)) {
	//		feed->setVisible(false);
	//	}

	//	const char* dockStateStr = config_get_string(
	//		main->Config(), service(), "DockState");
	//	QByteArray dockState =
	//		QByteArray::fromBase64(QByteArray(dockStateStr));

	//	if(main->isVisible() || !main->isMaximized())
	//		main->restoreState(dockState);
	//}
}

//
bool TwitchAuth::RetryLogin()
{
	if(!m_widget)
		return false;

	QCefWidget* cefWidget = m_widget->GetLoginCefWidget(nullptr, TWITCH_AUTH_URL);
	if(!cefWidget)
		return false;

	if(m_widget->exec() == QDialog::Rejected) {
		return false;
	}

	std::string client_id = TWITCH_CLIENTID;
	//deobfuscate_str(&client_id[0], TWITCH_HASH);

	return GetToken(TWITCH_TOKEN_URL, client_id, TWITCH_SCOPE_VERSION,
			QT_TO_UTF8(m_code), true);
}

void TwitchAuth::SaveInternal()
{
	AFMainDynamicComposit* composit = App()->GetMainView()->GetMainWindow();
	config_set_string(GetBasicConfig(), service(), "Name", m_name.c_str());
	config_set_string(GetBasicConfig(), service(), "UUID", m_uuid.c_str());

	if(m_uiLoaded) {
		config_set_string(GetBasicConfig(), service(), "DockState",
				  composit->saveState().toBase64().constData());
	}
	AFOAuthStreamKey::SaveInternal();
}
bool TwitchAuth::LoadInternal()
{
	QCef* cef = AFCefManager::GetSingletonInstance().GetCef();
	if(!cef)
		return false;

	m_name = config_get_string(GetBasicConfig(), service(), "Name");
	m_uuid = config_get_string(GetBasicConfig(), service(), "UUID");

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
	headers.push_back(std::string("Authorization: Bearer ") + token);

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
		blog(LOG_WARNING, "%s: %s", __FUNCTION__,
			 "Got 403 from Twitch, user probably does not "
			 "have two-factor authentication enabled on "
			 "their account");
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
	if(token.empty())
		return false;
	if(!key_.empty())
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

	key_ = json["data"][0]["stream_key"].string_value();

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

//#ifdef BROWSER_AVAILABLE
//void YoutubeChatDock::SetWidget(QCefWidget *widget_)
//{
//	lineEdit = new LineEditAutoResize();
//	lineEdit->setVisible(false);
//	lineEdit->setMaxLength(200);
//	lineEdit->setPlaceholderText(QTStr("YouTube.Chat.Input.Placeholder"));
//	sendButton = new QPushButton(QTStr("YouTube.Chat.Input.Send"));
//	sendButton->setVisible(false);
//
//	chatLayout = new QHBoxLayout();
//	chatLayout->setContentsMargins(0, 0, 0, 0);
//	chatLayout->addWidget(lineEdit, 1);
//	chatLayout->addWidget(sendButton);
//
//	QVBoxLayout *layout = new QVBoxLayout();
//	layout->setContentsMargins(0, 0, 0, 0);
//	layout->addWidget(widget_, 1);
//	layout->addLayout(chatLayout);
//
//	QWidget *widget = new QWidget();
//	widget->setLayout(layout);
//	setWidget(widget);
//
//	QWidget::connect(lineEdit, &LineEditAutoResize::returnPressed, this,
//			 &YoutubeChatDock::SendChatMessage);
//	QWidget::connect(sendButton, &QPushButton::pressed, this,
//			 &YoutubeChatDock::SendChatMessage);
//
//	cefWidget.reset(widget_);
//}
//
//void YoutubeChatDock::SetApiChatId(const std::string &id)
//{
//	this->apiChatId = id;
//	QMetaObject::invokeMethod(this, "EnableChatInput",
//				  Qt::QueuedConnection);
//}
//
//void YoutubeChatDock::SendChatMessage()
//{
//	const QString message = lineEdit->text();
//	if (message == "")
//		return;
//
//	OBSBasic *main = OBSBasic::Get();
//	YoutubeApiWrappers *apiYouTube(
//		dynamic_cast<YoutubeApiWrappers *>(main->GetAuth()));
//
//	ExecuteFuncSafeBlock([&]() {
//		lineEdit->setText("");
//		lineEdit->setPlaceholderText(
//			QTStr("YouTube.Chat.Input.Sending"));
//		if (apiYouTube->SendChatMessage(apiChatId, message)) {
//			os_sleep_ms(3000);
//		} else {
//			QString error = apiYouTube->GetLastError();
//			apiYouTube->GetTranslatedError(error);
//			QMetaObject::invokeMethod(
//				this, "ShowErrorMessage", Qt::QueuedConnection,
//				Q_ARG(const QString &, error));
//		}
//		lineEdit->setPlaceholderText(
//			QTStr("YouTube.Chat.Input.Placeholder"));
//	});
//}
//
//void YoutubeChatDock::ShowErrorMessage(const QString &error)
//{
//	QMessageBox::warning(this, QTStr("YouTube.Chat.Error.Title"),
//			     QTStr("YouTube.Chat.Error.Text").arg(error));
//}
//
//void YoutubeChatDock::EnableChatInput()
//{
//	lineEdit->setVisible(true);
//	sendButton->setVisible(true);
//}
//#endif
