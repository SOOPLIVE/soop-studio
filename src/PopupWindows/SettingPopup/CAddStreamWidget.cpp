#include "CAddStreamWidget.h"
#include "ui_add-stream-widget.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "UIComponent/CMessageBox.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Service/CService.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Browser/CCefManager.h"

// auth
#include "ViewModel/Auth/CAuth.h"
#include "ViewModel/Auth/COAuthLogin.hpp"
#include "ViewModel/Auth/CAuthListener.hpp"
#include "ViewModel/Auth/Soop/auth-soop.hpp"
#include "ViewModel/Auth/Twitch/auth-twitch.h"
#include "ViewModel/Auth/YouTube/auth-youtube.hpp"
#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"

#include "Common/StudioDefine.h"


/*
     -------           -------          -------
    |       |         |       |        |       |
    |   0   |         |   1   |        |   2   |
    |       |         |       |        |       |
     -------           -------          -------
     Select         Insert Custom       Cef
     Stream         Rtmp Datas          Login Page
*/


AFAddStreamWidget::AFAddStreamWidget(QWidget *parent) :
    AFTTopBaseDialog(parent),
    ui(new Ui::AFAddStreamWidget)
{
    ui->setupUi(this);

#ifdef _WIN32
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowTitle(QTStr("Basic.Settings.Stream.Add.Custom.Channel"));
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif

    ui->pushButton_ThreeDots->hide();

    qslotAuthUsage(false);
    connect(ui->checkBox_RtmpAuth, &QCheckBox::toggled, this, &AFAddStreamWidget::qslotAuthUsage);
    connect(ui->pushButton_ShowStreamKey, &QPushButton::toggled, this, &AFAddStreamWidget::qslotToggleStreamKeyHidden);
    connect(ui->pushButton_ShowPassword, &QPushButton::toggled, this, &AFAddStreamWidget::qslotTogglePasswordHidden);

    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);
}

AFAddStreamWidget::~AFAddStreamWidget()
{
    delete ui;
}

void AFAddStreamWidget::qslotOkTriggered()
{
    bool res = false;

    int indxCurrPage = ui->stackedWidget->currentIndex();
    
    if (indxCurrPage == 1)  // Custom RTMP
    {
        QString tmpKeyCustom = ui->lineEdit_StreamKey->text();
        QString tmpUrlCustom = ui->lineEdit_Url->text();
        QString tmpNameCustom = ui->lineEdit_ChannelName->text();
        QString tmpIDCustom = ui->lineEdit_ID->text();
        QString tmpPWCustom = ui->lineEdit_Password->text();
        
        
        if (tmpKeyCustom.isEmpty() ||
            tmpUrlCustom.isEmpty() ||
            tmpNameCustom.isEmpty())
        {           
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                this, "",
                QTStr("Basic.Settings.Stream.MissingSettingAlert"));

            return;
        }

        if (tmpUrlCustom.startsWith(SOOP_RTMP_URL))
        {
            QString msg = QTStr("Simulcast.InfoMessage_5").arg(PLATFORM_SOOP);

            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
                this, "",
                msg);

            return;
        }

        if (m_editMode)
        {
            res = true;
        }
        else
        {
            do {
                res = _CacheAuth("", "", 0,
                    tmpNameCustom.toStdString(),
                    "",
                    tmpKeyCustom.toStdString(), tmpUrlCustom.toStdString(), "",
                    tmpIDCustom.toStdString(), tmpPWCustom.toStdString());
            } while (false);
        }        
    }
    
    
    if(res)
        accept();
    else
        close();
}

void AFAddStreamWidget::qslotCustomRtmpTriggered()
{
    m_platform = PLATFORM_CUSTOM_RTMP;
    ui->label_WindowTitle->setText(m_platform);
    setWindowTitle(m_platform);
    ui->stackedWidget->setCurrentIndex(1);
    ui->lineEdit_Url->setFocus();
}

void AFAddStreamWidget::qslotAuthButtonTriggered()
{
    QPushButton* senderButton = reinterpret_cast<QPushButton*>(sender());
    m_platform = senderButton->statusTip();

    ui->stackedWidget->setCurrentIndex(2);

    _InitPage();
}

void AFAddStreamWidget::qslotAuthUsage(bool use)
{
    ui->lineEdit_ID->setVisible(use);
    ui->lineEdit_Password->setVisible(use);
    ui->label_ID->setVisible(use);
    ui->label_Password->setVisible(use);
    ui->pushButton_ShowPassword->setVisible(use);
}

void AFAddStreamWidget::qslotToggleStreamKeyHidden(bool show)
{
    if (show)
        ui->lineEdit_StreamKey->setEchoMode(QLineEdit::Normal);
    else
        ui->lineEdit_StreamKey->setEchoMode(QLineEdit::Password);
}

void AFAddStreamWidget::qslotTogglePasswordHidden(bool show)
{
    if(show)
        ui->lineEdit_Password->setEchoMode(QLineEdit::Normal);
    else
        ui->lineEdit_Password->setEchoMode(QLineEdit::Password);
}

void AFAddStreamWidget::qslotCloseTriggered()
{
    setResult(QDialog::Rejected);
    close();
}

void AFAddStreamWidget::qslotGetMessageFromLogin(const QString& msg)
{
    QStringList parts = msg.split(" : ");

    blog(LOG_INFO, "[LOGIN] MessageBox From Login Popup: %s", msg.toUtf8().constData());

    if (parts.size() == 2) {
        QString key = parts[0].trimmed();
        QString booleanValue = parts[1].trimmed();

        if (key == "loginRetainFlag")
        {
            m_loginRetain = (booleanValue.trimmed().toLower() == "true") ? true : false;
        }
        else if (key == "saveIdFlag")
        {
            m_saveId = (booleanValue.trimmed().toLower() == "true") ? true : false;
        }
    }
    else
    {
        if (!msg.contains("사이트 팝업 옵션"))
        {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", msg, false, true);
        }
    }
}

void AFAddStreamWidget::qslotGetChildPopup(const QString& url)
{
    if (url == "close_window")
    {
        close();
        return;
    }

    if (!m_pSnsDialog)
    {
        if (url.contains(QString::fromStdString(SOOP_FIND_SECURITY)) || url.contains(QString::fromStdString(SOOP_MINOR_CHECK)) || url.contains(QString::fromStdString(SOOP_LOGIN_BLOCK))
            || url.contains(QString::fromStdString(SOOP_BLACK_CLEAR)) || url.contains(QString::fromStdString(NEED_CERTIFY_POPUP_URL)) || url.contains(QString::fromStdString(SOOP_DOMESTIC_DORMANT))
            || url.contains(QString::fromStdString(LINK_FIND_PHP)) || url.contains(QString::fromStdString(LINK_JOIN))
            || url.contains(QString::fromStdString(LINK_LOGIN_BLOCK)) || url.contains(QString::fromStdString(LINK_MINOR_CHECK)) || url.contains(QString::fromStdString(LINK_FOREIGN_BLOCK_CHECK)))
        {
            MAINFRAME->NavigateDefaultBrowser(url);
            return;
        }

        if (url.contains(LINK_PASSWORD_CHANGE) || url.contains(LINK_SECOND_PASSWORD))
            return;

        QSize snsSize = _SnsSize(url);
        m_pSnsDialog = new AFQResizeDialog(this);
        m_pSnsDialog->setProperty("url", url);

        QCefWidget* cc = CEFMANAGER.createWidget(this, "first-load-error");
        if (!cc) {
            return;
        }
        cc->allowAllPopups(true);
        cc->m_SoopLogin = true;

        connect(cc, SIGNAL(cefBeforePopup(const QString&)), this, SLOT(qslotGetChildPopup(const QString&)));
        connect(cc, SIGNAL(cefQueryRequest(const QCefQuery&)), this, SLOT(qslotLoginRecieved(const QCefQuery&)));
        connect(cc, SIGNAL(cefMessageBoxMessage(const QString&)), this, SLOT(qslotGetMessageFromLogin(const QString&)));

        cc->setURL(url.toStdString());

        QVBoxLayout* layout = new QVBoxLayout(m_pSnsDialog);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(cc);
        m_pSnsDialog->setLayout(layout);
        m_pSnsDialog->resize(snsSize);
        m_pSnsDialog->exec();

        delete m_pSnsDialog;
        m_pSnsDialog = nullptr;
    }
    else
    {
        QWidget* senderCef = reinterpret_cast<QWidget*>(sender());
        QString propertyUrl = senderCef->property("url").toString();
        QString code = _SnsCode(propertyUrl);
        if (code != "notfound" || code != "sns_code=21" || code != "sns_code=23"
            || code != "sns_code=20" || code != "sns_code=24"
            || code != "sns_code=11" || code != "sns_code=12" || code != "sns_code=17")
        {
            MAINFRAME->NavigateDefaultBrowser(url);
            m_pSnsDialog->close();
        }
    }

}


void AFAddStreamWidget::qslotLoginRecieved(const QCefQuery& query)
{
    std::string err;
}

void AFAddStreamWidget::qslotLoginUrlChanged(const QString& url)
{
    if (url.contains(QString::fromStdString(SOOP_SECONDLOGIN)))
        this->setFixedSize(433, 418);
}

void AFAddStreamWidget::SetAddStreamButtons()
{
    ui->widget_AddSoop->setStatusTip(PLATFORM_SOOP);
    ui->widget_AddTwitch->setStatusTip(PLATFORM_TWITCH);
    ui->widget_AddYoutube->setStatusTip(PLATFORM_YOUTUBE);
    ui->widget_AddRTMP->setStatusTip(PLATFORM_CUSTOM_RTMP);

    // Soop
    if (AUTH_CONTEXT.IsSoopRegistered())
    {
        ui->widget_AddSoop->setProperty("isRegistered", true);
        PolishStyleSheet(ui->widget_AddSoop);
    }
    
    // Youtube
    if (AUTH_CONTEXT.IsYoutubeRegistered())
    {
        ui->widget_AddYoutube->setProperty("isRegistered", true);
        PolishStyleSheet(ui->widget_AddYoutube);
    }
    else
    {
        connect(ui->widget_AddYoutube, &AFQHoverWidget::qsignalMouseClick, this, &AFAddStreamWidget::qslotAuthButtonTriggered);
        ui->label_YoutubeConnectedIcon->setDisabled(true);
    }

    // Twitch
    if (AUTH_CONTEXT.IsTwitchRegistered())
    {
        ui->widget_AddTwitch->setProperty("isRegistered", true);
        PolishStyleSheet(ui->widget_AddTwitch);
    }
    else
    {
        connect(ui->widget_AddTwitch, &AFQHoverWidget::qsignalMouseClick, this, &AFAddStreamWidget::qslotAuthButtonTriggered);
        ui->label_TwitchConnectedIcon->setDisabled(true);
    }

    connect(ui->widget_AddRTMP, &AFQHoverWidget::qsignalMouseClick, this, &AFAddStreamWidget::qslotCustomRtmpTriggered);
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFAddStreamWidget::qslotCloseTriggered);
    connect(ui->pushButton_Accept, &QPushButton::clicked, this, &AFAddStreamWidget::qslotOkTriggered);
    connect(ui->pushButton_Cancel, &QPushButton::clicked, this, &AFAddStreamWidget::close);

    _InitConnectedIconToolTip();
}

void AFAddStreamWidget::AddStreamWidgetInit(QString platform)
{
    if (platform.isEmpty())
    {
        m_startWithoutPlatform = true;
        ui->label_WindowTitle->setText(m_platform);
        setWindowTitle(m_platform);

        ui->stackedWidget->setCurrentIndex(0);
        
        ui->label_AddChannel->style()->unpolish(ui->label_AddChannel);
        ui->label_AddChannel->style()->polish(ui->label_AddChannel);
    }
    else
    {
        m_startWithoutPlatform = false;
        m_platform = platform;
        _InitPage();
    }

    SetAddStreamButtons();
}

void AFAddStreamWidget::EditStreamWidgetInit(QString server, QString streamkey, QString channelName, QString id, QString password)
{
    qslotCustomRtmpTriggered();
    if (!id.isEmpty() || !password.isEmpty())
        ui->checkBox_RtmpAuth->setChecked(true);

    ui->lineEdit_ID->setText(id);
    ui->lineEdit_StreamKey->setText(streamkey);
    ui->lineEdit_Url->setText(server);
    ui->lineEdit_ChannelName->setText(channelName);
    ui->lineEdit_Password->setText(password);

    //Stream Edit Disable

    m_editMode = true;
    if (m_editMode)
    {
        if (AUTH_CONTEXT.IsCachedAuth(channelName.toUtf8().constData()))
        {
            ui->lineEdit_ChannelName->setDisabled(true);
        }
    }

    ui->label_WindowTitle->setText(QTStr("Basic.Settings.Stream.ChannelInfo"));
    setWindowTitle(QTStr("Basic.Settings.Stream.ChannelInfo"));
    SetAddStreamButtons();
}

QCefWidget* AFAddStreamWidget::GetLoginCefWidget(QWidget* parent, const std::string& url)
{
    if(!m_pCefWidget) {
        QCef* cef = CEFMANAGER.GetCef();
        if (!cef)
            return nullptr;

        QCefCookieManager* panel_cookies = CEFMANAGER.GetCefCookieManager();

        if (SOOPLIVE_KR_LOGIN == url || SOOPLIVE_KR_LOGIN_GLOBAL == url)
        {
            if (panel_cookies)
                panel_cookies->DeleteCookies(url, "AuthTicket");
            ui->titleFrame->setFixedHeight(8);
            setFixedSize(433, 640);
        }

        std::string headers = "";
        //
        m_pCefWidget = CEFMANAGER.createWidget(parent, "first-load-error", panel_cookies, headers);
        if(!m_pCefWidget) {
            m_fail = true;
            return nullptr;
        }

        connect(m_pCefWidget, SIGNAL(urlChanged(const QString&)), this, SLOT(qslotLoginUrlChanged(const QString&)));

        m_pCefWidget->allowAllPopups(true);        
        m_pCefWidget->setURL(url);

        ui->page_AuthBrowser->layout()->addWidget(m_pCefWidget);

        //Hiding widget_ButtonBox hides border + margin so make the height 1px 
        //ui->buttonBox->hide();
        ui->pushButton_Accept->hide();
        ui->pushButton_Cancel->hide();
        ui->buttonBoxFrame->setFixedHeight(0);
    } else {
        m_pCefWidget->setURL(url);
    }

    return m_pCefWidget;
}

void AFAddStreamWidget::SetAuthData(std::string accessToken,
                                    std::string refreshToken,
                                    uint64_t expireTime,
                                    std::string channelID,
                                    std::string channelNick,
                                    std::string streamKey,
                                    std::string streamUrl,
                                    bool loginRetain,
                                    std::string clientID)
{
    int res = _CacheAuth(accessToken, refreshToken, expireTime,
                         channelID, channelNick, streamKey, streamUrl, 
                         std::string(), std::string(), std::string(), 
                         loginRetain, clientID); //Check Default Value
    accept();
}

void AFAddStreamWidget::SetAuthCookie(std::string cookie, std::string channelID)
{
    m_rawAuthData.channelID = channelID;
    m_rawAuthData.cookie = cookie;
    m_tempCookie = cookie;
    m_rawAuthData.platform = m_platform.toStdString();
    m_rawAuthData.saveId = m_saveId;
    m_rawAuthData.loginRetain = m_loginRetain;
    ui->lineEdit_ChannelName->setText(QString::fromUtf8(channelID));

    m_rawAuthData.uuid = QUuid::createUuid().toString().toStdString();

    AUTH_CONTEXT.CacheAuth(true, m_rawAuthData);

    accept();
}

QString AFAddStreamWidget::GetStreamKey()
{
    return ui->lineEdit_StreamKey->text();
}

QString AFAddStreamWidget::GetUrl()
{
    return ui->lineEdit_Url->text();
}

QString AFAddStreamWidget::GetID()
{
    QString ret = "";
    if (ui->checkBox_RtmpAuth->isChecked())
        ret = ui->lineEdit_ID->text();
    return ret;
}

QString AFAddStreamWidget::GetChannelName()
{
    return ui->lineEdit_ChannelName->text();
}

QString AFAddStreamWidget::GetPassword()
{
    QString ret = "";
    if (ui->checkBox_RtmpAuth->isChecked())
        ret = ui->lineEdit_Password->text();
    return ret;
}

bool AFAddStreamWidget::CheckChannel(const char* uuid)
{
    auto& authMan = AUTH_CONTEXT;

    if(//authMan.IsAuthed(uuid) ||
        authMan.IsCachedAuth(uuid))
    {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
            this, "",
            QT_UTF8("Exist Account"));

        return true;
    }

    return false;
}

int AFAddStreamWidget::exec()
{
    if (_IsCefPlatform() && !m_pCefWidget) {
        return QDialog::Rejected;
    }

    return QDialog::exec();
}

void AFAddStreamWidget::reject()
{
    if(m_pCefWidget) {
        delete m_pCefWidget;
    }
    QDialog::reject();
}

void AFAddStreamWidget::accept()
{
    if(m_pCefWidget) {
        delete m_pCefWidget;
    }
    QDialog::accept();
}
//

bool AFAddStreamWidget::_CacheAuth(std::string accessToken,
                                   std::string refreshToken,
                                   uint64_t expireTime,
                                   std::string channelID,
                                   std::string channelNick,
                                   std::string streamKey,
                                   std::string streamUrl,
                                   std::string uuid, 
                                   std::string customID, 
                                   std::string customPassword,
                                   bool loginRetain,
                                   std::string clientID)
{
    bool bmain = false;

    m_rawAuthData.accessToken = accessToken;
    m_rawAuthData.refreshToken = refreshToken;
    m_rawAuthData.expireTime = expireTime;
    m_rawAuthData.channelID = channelID;
    m_rawAuthData.channelNick = channelNick;
    m_rawAuthData.keyRTMP = streamKey;
    m_rawAuthData.urlRTMP = streamUrl;
    m_rawAuthData.platform = m_platform.toStdString();
    m_rawAuthData.loginRetain = loginRetain;
    if (uuid == "")
        m_rawAuthData.uuid = QUuid::createUuid().toString().toStdString();
    else
        m_rawAuthData.uuid = uuid;

    if (m_platform == PLATFORM_CUSTOM_RTMP)
    {
        m_rawAuthData.type = AuthType::No_OAuth_RTMP;
        if (ui->checkBox_RtmpAuth->isChecked())
        {
            m_rawAuthData.customID = customID;
            m_rawAuthData.customPassword = customPassword;
        }
    }
    else if (m_platform == PLATFORM_SOOP)
    {
        bmain = true;
        m_rawAuthData.type = AuthType::OAuth_StreamKey;
    }
    else if (m_platform == PLATFORM_YOUTUBE)
    {
        m_rawAuthData.type = AuthType::OAuth_LinkedAccount;
    }
    else if (m_platform == PLATFORM_TWITCH)
    {
        m_rawAuthData.type = AuthType::OAuth_StreamKey;
    }
    else
    {
        return false;
    }

    _ConvertAuthToStreamData();

    return AUTH_CONTEXT.CacheAuth(bmain, m_rawAuthData);
}

void AFAddStreamWidget::_InitPage()
{
    ui->label_WindowTitle->setText(m_platform);
    setWindowTitle(m_platform);

    if (m_platform == PLATFORM_CUSTOM_RTMP)
    {
        ui->stackedWidget->setCurrentIndex(1);

        ui->label_WindowTitle->setText(QTStr("Basic.Settings.Stream.Add.Custom.Channel"));
        setWindowTitle(QTStr("Basic.Settings.Stream.Add.Custom.Channel"));
    }
    else if (m_platform == PLATFORM_YOUTUBE)
    {
        _InitYoutubeLoginPage();
        ui->stackedWidget->setCurrentIndex(2);
    }
    else if (m_platform == PLATFORM_TWITCH)
    {
        _InitTwitchLoginPage();
        ui->stackedWidget->setCurrentIndex(2);
    }
    else if (m_platform == PLATFORM_SOOP)
    {
        _InitSoopLoginPage();
        ui->stackedWidget->setCurrentIndex(2);
    }
}

void AFAddStreamWidget::_ConvertAuthToStreamData()
{
    QString streamKey = m_rawAuthData.keyRTMP.c_str();
    QString streamUrl = m_rawAuthData.urlRTMP.c_str();
    
    QString channelID = m_rawAuthData.channelID.c_str();
    m_channelNick = m_rawAuthData.channelNick.c_str();

    QString customID = m_rawAuthData.customID.c_str();
    QString customPW = m_rawAuthData.customPassword.c_str();

    ui->lineEdit_ChannelName->setText(channelID);
    ui->lineEdit_StreamKey->setText(streamKey);
    ui->lineEdit_Url->setText(streamUrl);
    ui->lineEdit_ID->setText(customID);
    ui->lineEdit_Password->setText(customPW);
}


void AFAddStreamWidget::_InitTwitchLoginPage()
{
    if(m_auth)
        m_auth.reset();

    TwitchAuth* streamAuth = new TwitchAuth(twitchDef, this);
    if(!streamAuth)
        return;

    if(!streamAuth->Login()) {
        delete streamAuth;
        return;
    }
    m_auth.reset(streamAuth);
}
void AFAddStreamWidget::_InitSoopLoginPage()
{
    if(m_auth)
        m_auth.reset();

    SoopAuth* streamAuth = new SoopAuth(soopDef, this);
    if (!streamAuth)
        return;

    std::string id = "";

    AFChannelData* data;
    if (AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, data))
    {
        m_saveId = data->pAuthData->saveId;

        if (m_saveId)
            id = data->pAuthData->channelID;
    }

    int soopWidth = 400;
    int soopHeight = 640;

    if (m_startWithoutPlatform)
    {
        int x = this->x() + (this->width() / 2) - (soopWidth / 2);
        int y = this->y();
        move(x, y);
    }

    setFixedSize(soopWidth, soopHeight);
    ui->page_AuthBrowser->layout()->setContentsMargins(0, 0, 0, 0);
    ui->titleFrame->setFixedHeight(20);
    ui->titleFrame->setStyleSheet("background-color:white; border-bottom:none;");
    ui->label_WindowTitle->hide();
    ui->pushButton_Close->hide();
    ui->pushButton_ThreeDots->hide();

    if (!streamAuth->LoginForCookie(id)) {
        delete streamAuth;
        return;
    }

    m_auth.reset(streamAuth);
}

void AFAddStreamWidget::_InitYoutubeLoginPage()
{
    auto it = std::find_if(youtubeServices.begin(), youtubeServices.end(),
                   [](auto &item) {
                       return "YouTube - RTMP" == item.service;
                   });
    
    if (it == youtubeServices.end())
        return;
    
    YoutubeApiWrappers* streamAuth = new YoutubeApiWrappers(*it, this);
    if(!streamAuth)
        return;
    if(!streamAuth->Login()) {
        delete streamAuth;
        return;
    }
    m_auth.reset(streamAuth);
}

void AFAddStreamWidget::_InitConnectedIconToolTip()
{
    ui->label_SoopConnectedIcon->setToolTip(ui->label_SoopConnectedIcon->isEnabled() ? QTStr("BroadFixed") : "");
    ui->label_TwitchConnectedIcon->setToolTip(ui->label_TwitchConnectedIcon->isEnabled() ? QTStr("BroadAvailable") : "");
    ui->label_YoutubeConnectedIcon->setToolTip(ui->label_YoutubeConnectedIcon->isEnabled() ? QTStr("BroadAvailable") : "");
}

bool AFAddStreamWidget::_IsCefPlatform() {
    
    if (m_platform == PLATFORM_SOOP ||
        m_platform == PLATFORM_YOUTUBE ||
        m_platform == PLATFORM_TWITCH)
        return true;
    
    return false;
}

QSize AFAddStreamWidget::_SnsSize(const QString& url)
{
    QSize size(400, 536);

    QStringList wordsToFind = { 
        "sns_code=21",                      //NAVER
        "sns_code=23",                      //KAKAO
        "sns_code=20",                      //APPLE
        "sns_code=24",                      //TWITCH
        "sns_code=11",                      //FACEBOOK
        "sns_code=12",                      //TWITTER
        "sns_code=17",                      //GOOGLE
        "pop_black_clear",
        "pop_black_info",
        "changeMember",
        "pop_verify_self_minor_none_login",
        "pop_login_block",
        "pop_sleep_info",
        "domestic_dormant_account_restore",
        "pop_ipin",
        "pop_person_check",
        "pop_foreign_login_block"
    };

    QMap<QString, QSize> sizeMap;
    sizeMap["notfound"] = QSize(400, 536);
    sizeMap["sns_code=21"] = QSize(560, 760);
    sizeMap["sns_code=23"] = QSize(560, 840);
    sizeMap["sns_code=20"] = QSize(450, 715);
    sizeMap["sns_code=24"] = QSize(490, 500);
    sizeMap["sns_code=11"] = QSize(560, 400);
    sizeMap["sns_code=12"] = QSize(885, 900);
    sizeMap["sns_code=17"] = QSize(615, 700);
    sizeMap["pop_black_clear"] = QSize(442, 700);
    sizeMap["pop_black_info"] = QSize(525, 555);
    sizeMap["changeMember"] = QSize(435, 455);
    sizeMap["pop_verify_self_minor_none_login"] = QSize(525, 600);
    sizeMap["pop_login_block"] = QSize(525, 555);
    sizeMap["pop_sleep_info"] = QSize(404, 372);
    sizeMap["domestic_dormant_account_restore"] = QSize(645, 624);
    sizeMap["pop_ipin"] = QSize(461, 800);
    sizeMap["pop_person_check"] = QSize(520, 870);
    sizeMap["pop_foreign_login_block"] = QSize(615, 700);

    QString pattern = wordsToFind.join("|");
    QRegularExpression regex(pattern);

    QRegularExpressionMatch match = regex.match(url);

    QString matchedWord = "notfound";
    if (match.hasMatch())
        matchedWord = match.captured(0);

    return sizeMap.value(matchedWord);
}

QString AFAddStreamWidget::_SnsCode(const QString& url)
{
    QStringList wordsToFind = {
    "sns_code=21",                      //NAVER
    "sns_code=23",                      //KAKAO
    "sns_code=20",                      //APPLE
    "sns_code=24",                      //TWITCH
    "sns_code=11",                      //FACEBOOK
    "sns_code=12",                      //TWITTER
    "sns_code=17",                      //GOOGLE
    "pop_black_clear",
    "pop_black_info",         
    "changeMember",
    "pop_verify_self_minor_none_login",
    "pop_login_block",
    "pop_sleep_info",
    "domestic_dormant_account_restore",
    "pop_ipin",
    "pop_person_check",
    "pop_foreign_login_block"
    };

    QMap<QString, QSize> sizeMap;
    sizeMap["notfound"] = QSize(400, 536);
    sizeMap["sns_code=21"] = QSize(560, 760);
    sizeMap["sns_code=23"] = QSize(560, 840);
    sizeMap["sns_code=20"] = QSize(450, 715);
    sizeMap["sns_code=24"] = QSize(490, 435);
    sizeMap["sns_code=11"] = QSize(560, 400);
    sizeMap["sns_code=12"] = QSize(885, 900);
    sizeMap["sns_code=17"] = QSize(615, 700);
    sizeMap["pop_black_clear"] = QSize(442, 700);
    sizeMap["pop_black_info"] = QSize(525, 555);
    sizeMap["changeMember"] = QSize(435, 455);
    sizeMap["pop_verify_self_minor_none_login"] = QSize(525, 600);
    sizeMap["pop_login_block"] = QSize(525, 555);
    sizeMap["pop_sleep_info"] = QSize(404, 372);
    sizeMap["domestic_dormant_account_restore"] = QSize(645, 624);
    sizeMap["pop_ipin"] = QSize(461, 800);
    sizeMap["pop_person_check"] = QSize(520, 870);
    sizeMap["pop_foreign_login_block"] = QSize(615, 700);

    QString pattern = wordsToFind.join("|");
    QRegularExpression regex(pattern);

    QRegularExpressionMatch match = regex.match(url);

    QString matchedWord = "notfound";
    if (match.hasMatch())
        matchedWord = match.captured(0);

    return matchedWord;
}
