#include "auth-youtube.hpp"

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

#include "CoreModel/Locale/CLocaleTextManager.h"

#include "youtube-api-wrappers.hpp"

#include "src/PopupWindows/SettingPopup/CAddStreamWidget.h"




using namespace json11;

/* ------------------------------------------------------------------------- */
#define YOUTUBE_AUTH_URL			"https://accounts.google.com/o/oauth2/v2/auth"
#define YOUTUBE_TOKEN_URL			"https://www.googleapis.com/oauth2/v4/token"
#define YOUTUBE_SCOPE_VERSION		1
#define YOUTUBE_API_STATE_LENGTH	32
#define SECTION_NAME				"YouTube"

#define YOUTUBE_CHAT_PLACEHOLDER_URL	"https://obsproject.com/placeholders/youtube-chat"
//#define YOUTUBE_CHAT_POPOUT_URL			"https://www.youtube.com/live_chat?is_popout=1&dark_theme=1&v=%1"



#define YOUTUBE_CHAT_DOCK_NAME "ytChat"

static const char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
static const int allowedCount = static_cast<int>(sizeof(allowedChars) - 1);
/* ------------------------------------------------------------------------- */

YoutubeAuth::YoutubeAuth(const Def &d, AFAddStreamWidget* widget)
	: AFOAuthStreamKey(d),
	m_pWidget(widget),
	m_section(SECTION_NAME)
{
}

YoutubeAuth::~YoutubeAuth()
{
	if (!m_uiLoaded)
		return;

	if(m_pAuthListner) {
		delete m_pAuthListner;
		m_pAuthListner = nullptr;
	}

//#ifdef BROWSER_AVAILABLE
//	OBSBasic *main = OBSBasic::Get();
//
//	main->RemoveDockWidget(YOUTUBE_CHAT_DOCK_NAME);
//	chat = nullptr;
//#endif
}

bool YoutubeAuth::Login()
{
	//DeleteCookies();
	//
	if(!m_pWidget)
		return false;

	m_pAuthListner = new AFAuthListener;
	if(!m_pAuthListner)
		return false;

	m_redirectUri = QString("http://127.0.0.1:%1").arg(m_pAuthListner->GetPort());

	QString state = GenerateState();
	m_pAuthListner->SetState(state);

	connect(m_pAuthListner, SIGNAL(ok(const QString&)), this, SLOT(qslotRedirect(const QString&)));
	connect(m_pAuthListner, &AFAuthListener::fail, this, &YoutubeAuth::qslotClose);

	QString url_template;
	url_template += "%1";
	url_template += "?response_type=code";
	url_template += "&client_id=%2";
	url_template += "&redirect_uri=%3";
	url_template += "&state=%4";
	url_template += "&scope=https://www.googleapis.com/auth/youtube";
	QString url = url_template.arg(YOUTUBE_AUTH_URL, YOUTUBE_CLIENTID, m_redirectUri, state);

	QCefWidget* cefWidget = m_pWidget->GetLoginCefWidget(nullptr, url.toStdString());
	if(!cefWidget)
		return false;
	//
	return true;
}
void YoutubeAuth::DeleteCookies()
{
	CEFMANAGER.InitPanelCookieManager();
	QCefCookieManager* panel_cookies = CEFMANAGER.GetCefCookieManager();
	if(panel_cookies) {
		panel_cookies->DeleteCookies(service(), std::string());
	}
}

void YoutubeAuth::SetChatId(const QString &chat_id)
{
	m_youtubeChatUrl = QString(YOUTUBE_CHAT_POPOUT_URL).arg(chat_id);

//#ifdef BROWSER_AVAILABLE
//	QString chat_url = QString(YOUTUBE_CHAT_POPOUT_URL).arg(chat_id);
//
//	if (chat && chat->cefWidget) {
//		chat->cefWidget->setURL(chat_url.toStdString());
//	}
//#else
//	UNUSED_PARAMETER(chat_id);
//#endif
}

void YoutubeAuth::ResetChat()
{
//#ifdef BROWSER_AVAILABLE
//	if (chat && chat->cefWidget) {
//		chat->cefWidget->setURL(YOUTUBE_CHAT_PLACEHOLDER_URL);
//	}
//#endif
}

void YoutubeAuth::ReloadChat()
{
//#ifdef BROWSER_AVAILABLE
//	if(chat && chat->cefWidget) {
//		chat->cefWidget->reloadPage();
//	}
//#endif
}

QString YoutubeAuth::GenerateState()
{
	char state[YOUTUBE_API_STATE_LENGTH + 1];
	QRandomGenerator *rng = QRandomGenerator::system();
	int i;

	for (i = 0; i < YOUTUBE_API_STATE_LENGTH; i++)
		state[i] = allowedChars[rng->bounded(0, allowedCount)];
	state[i] = 0;

	return state;
}

bool YoutubeAuth::RetryLogin()
{
	return true;
}

void YoutubeAuth::SaveInternal()
{
	auto activeConfig = ACTIVECONFIG;

	const char* section_name = m_section.c_str();
	config_set_string(activeConfig, section_name, "RefreshToken", m_refreshToken.c_str());
	config_set_string(activeConfig, section_name, "Token", m_token.c_str());
	config_set_uint(activeConfig, section_name, "ExpireTime", m_expireTime);
	config_set_int(activeConfig, section_name, "ScopeVer", m_currentScopeVer);
}

static inline std::string get_config_str(const char* section,
										 const char* name)
{
	const char* val = config_get_string(ACTIVECONFIG, section, name);
	return val ? val : "";
}

bool YoutubeAuth::LoadInternal()
{
	const char* section_name = m_section.c_str();
    m_refreshToken = get_config_str(section_name, "RefreshToken");
    m_token = get_config_str(section_name, "Token");
    m_expireTime = config_get_uint(ACTIVECONFIG, section_name, "ExpireTime");
    m_currentScopeVer = (int)config_get_int(ACTIVECONFIG, section_name, "ScopeVer");
	m_firstLoad = false;
	return m_implicit ? !m_token.empty() : !m_refreshToken.empty();
}

void YoutubeAuth::LoadUI()
{
	if(m_uiLoaded)
		return;

	//#ifdef BROWSER_AVAILABLE
	//	if (!cef)
	//		return;
	//
	//	OBSBasic::InitBrowserPanelSafeBlock();
	//	OBSBasic *main = OBSBasic::Get();
	//
	//	QCefWidget *browser;
	//
	//	QSize size = main->frameSize();
	//	QPoint pos = main->pos();
	//
	//	chat = new YoutubeChatDock(QTStr("Auth.Chat"));
	//	chat->setObjectName(YOUTUBE_CHAT_DOCK_NAME);
	//	chat->resize(300, 600);
	//	chat->setMinimumSize(200, 300);
	//	chat->setAllowedAreas(Qt::AllDockWidgetAreas);
	//
	//	browser = cef->create_widget(chat, YOUTUBE_CHAT_PLACEHOLDER_URL, panel_cookies);
	//
	//	chat->SetWidget(browser);
	//	main->AddDockWidget(chat, Qt::RightDockWidgetArea);
	//
	//	chat->setFloating(true);
	//	chat->move(pos.x() + size.width() - chat->width() - 50, pos.y() + 50);
	//
	//	if (firstLoad) {
	//		chat->setVisible(true);
	//	}
	//#endif

	//	main->NewYouTubeAppDock();

	//	if (!firstLoad) {
	//		const char *dockStateStr = config_get_string(ACTIVECONFIG, service(), "DockState");
	//		QByteArray dockState = QByteArray::fromBase64(QByteArray(dockStateStr));
	//
	//		if (main->isVisible() || !main->isMaximized())
	//			main->restoreState(dockState);
	//	}

	m_uiLoaded = true;
}
void YoutubeAuth::qslotRedirect(QString code)
{
	if(!m_pWidget)
		return;

	bool res = GetToken(YOUTUBE_TOKEN_URL, YOUTUBE_CLIENTID, YOUTUBE_SECRETID,
						QT_TO_UTF8(m_redirectUri), YOUTUBE_SCOPE_VERSION,
						QT_TO_UTF8(code), true);
	if(res) {
		GetAuthInfo(m_pWidget);
	} else {
		m_pWidget->close();
	}
}
void YoutubeAuth::qslotClose()
{
	if(!m_pWidget)
		return;

	m_pWidget->close();
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
