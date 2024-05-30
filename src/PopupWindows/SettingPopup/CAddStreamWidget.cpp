#include "CAddStreamWidget.h"
#include "ui_add-stream-widget.h"

#include "UIComponent/CMessageBox.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "qt-wrapper.h"
#include "platform/platform.hpp"
#include "CoreModel/Service/CService.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Browser/CCefManager.h"

#include "Application/CApplication.h"

// auth
#include "ViewModel/Auth/CAuth.h"
#include "ViewModel/Auth/COAuthLogin.hpp"
#include "ViewModel/Auth/CAuthListener.hpp"
#include "ViewModel/Auth/Soop/auth-soop-global.h"
#include "ViewModel/Auth/Soop/auth-soop.hpp"
#include "ViewModel/Auth/Twitch/auth-twitch.h"
#include "ViewModel/Auth/YouTube/auth-youtube.hpp"
#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"



#define YOUTUBE_AUTH_URL ""





AFAddStreamWidget::AFAddStreamWidget(QWidget *parent) :
    AFQRoundedDialogBase(parent),
    ui(new Ui::AFAddStreamWidget)
{
    ui->setupUi(this);

    ui->pushButton_ThreeDots->hide();

    this->setAttribute(Qt::WA_TranslucentBackground, false);

    qslotAuthUsage(false);
    connect(ui->checkBox_RtmpAuth, &QCheckBox::toggled,
        this, &AFAddStreamWidget::qslotAuthUsage);
    connect(ui->pushButton_ShowStreamKey, &QPushButton::toggled,
        this, &AFAddStreamWidget::qslotToggleStreamKeyHidden);
    connect(ui->pushButton_ShowPassword, &QPushButton::toggled,
        this, &AFAddStreamWidget::qslotTogglePasswordHidden);

    this->SetHeightFixed(true);
    this->SetWidthFixed(true);
}

AFAddStreamWidget::~AFAddStreamWidget()
{
    delete ui;
}

void AFAddStreamWidget::qslotOkTriggered()
{
    bool res = false;
    
    auto& localeMan = AFLocaleTextManager::GetSingletonInstance();
    
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
                QT_UTF8(localeMan.Str("Basic.Settings.Stream.MissingSettingAlert")));

            return;
        }

        if (m_bEditMode)
        {
            res = true;
        }
        else
        {
            do {
                /*
                if (CheckChannel(uuid))
                {
                    return;
                }
                */

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
    m_sPlatform = "Custom RTMP";
    ui->label_WindowTitle->setText(m_sPlatform);
    setWindowTitle(m_sPlatform);
    ui->stackedWidget->setCurrentIndex(1);
    ui->lineEdit_Url->setFocus();
}

void AFAddStreamWidget::qslotAuthButtonTriggered()
{
    ui->stackedWidget->setCurrentIndex(2);
    QPushButton* senderButton = reinterpret_cast<QPushButton*>(sender());

    m_sPlatform = senderButton->statusTip();

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

void AFAddStreamWidget::SetAddStreamButtons()
{
    std::string absPath;
    QString style = "QLabel {image: url(%1);}";
    GetDataFilePath("assets/setting-dialog/stream/soop-global.svg", absPath);
    ui->label_SoopGlobalIcon->setStyleSheet(style.arg(absPath.c_str()));
    GetDataFilePath("assets/setting-dialog/stream/global-icon.svg", absPath);
    ui->label_GlobalIcon->setStyleSheet(style.arg(absPath.c_str()));
    GetDataFilePath("assets/setting-dialog/stream/soop.svg", absPath);
    ui->label_SoopIcon->setStyleSheet(style.arg(absPath.c_str()));
    GetDataFilePath("assets/setting-dialog/stream/twitch.svg", absPath);
    ui->label_TwitchIcon->setStyleSheet(style.arg(absPath.c_str()));
    GetDataFilePath("assets/setting-dialog/stream/youtube.svg", absPath);
    ui->label_YoutubeIcon->setStyleSheet(style.arg(absPath.c_str()));

    ui->widget_AddSoopGlobal->setStatusTip("SOOP Global");
    ui->widget_AddSoop->setStatusTip("afreecaTV");
    ui->widget_AddTwitch->setStatusTip("Twitch");
    ui->widget_AddYoutube->setStatusTip("Youtube");
    ui->widget_AddRTMP->setStatusTip("Custom RTMP");

    auto& auth = AFAuthManager::GetSingletonInstance();

    if (auth.IsSoopGlobalPreregistered())
    {
        ui->label_SoopGlobalConnected->setVisible(true);
        ui->widget_AddSoopGlobal->setDisabled(true);
    }
    else
    {
        connect(ui->widget_AddSoopGlobal, &AFQHoverWidget::qsignalMouseClick,
            this, &AFAddStreamWidget::qslotAuthButtonTriggered);
        ui->label_SoopGlobalConnected->setVisible(false);
    }

    if (auth.IsSoopPreregistered())
    {
        ui->label_SoopConnected->setVisible(true);
        ui->widget_AddSoop->setDisabled(true);

    }
    else
    {
        connect(ui->widget_AddSoop, &AFQHoverWidget::qsignalMouseClick,
            this, &AFAddStreamWidget::qslotAuthButtonTriggered);
        ui->label_SoopConnected->setVisible(false);

    }
    if (auth.IsYoutubePreregistered())
    {
        ui->label_YoutubeConnected->setVisible(true);
        ui->widget_AddYoutube->setDisabled(true);
    }
    else
    {
        connect(ui->widget_AddYoutube, &AFQHoverWidget::qsignalMouseClick,
            this, &AFAddStreamWidget::qslotAuthButtonTriggered);
        ui->label_YoutubeConnected->setVisible(false);
    }
    if (auth.IsTwitchPreregistered())
    {
        ui->label_TwitchConnected->setVisible(true);
        ui->widget_AddTwitch->setDisabled(true);
    }
    else
    {
        connect(ui->widget_AddTwitch, &AFQHoverWidget::qsignalMouseClick,
            this, &AFAddStreamWidget::qslotAuthButtonTriggered);

        ui->label_TwitchConnected->setVisible(false);
    }


    connect(ui->widget_AddRTMP, &AFQHoverWidget::qsignalMouseClick,
        this, &AFAddStreamWidget::qslotCustomRtmpTriggered);
    connect(ui->pushButton_Close, &QPushButton::clicked, 
        this, &AFAddStreamWidget::close);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setFixedSize(128, 40);


    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this,
        &AFAddStreamWidget::qslotOkTriggered);

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setFixedSize(128, 40);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this,
        &AFAddStreamWidget::close);




    ui->label_RTMPConnected->setVisible(false);
}

void AFAddStreamWidget::AddStreamWidgetInit()
{
    ui->label_WindowTitle->setText(m_sPlatform);
    setWindowTitle(m_sPlatform);

    ui->stackedWidget->setCurrentIndex(0);
    SetAddStreamButtons();
}

void AFAddStreamWidget::AddStreamWidgetInit(QString platform)
{
    m_sPlatform = platform;

    _InitPage();
    
    SetAddStreamButtons();
}

void AFAddStreamWidget::EditStreamWidgetInit(QString server, QString streamkey, 
    QString channelName, QString id, QString password)
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

    m_bEditMode = true;
    if (m_bEditMode)
    {
        auto& authManager = AFAuthManager::GetSingletonInstance();
        if (authManager.IsCachedAuth(channelName.toUtf8().constData()))
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
    if(!m_cefWidget) {
        auto& cefManager = AFCefManager::GetSingletonInstance();
        QCef* cef = cefManager.GetCef();
        if (!cef)
            return nullptr;
        QCefCookieManager* panel_cookies = cefManager.GetCefCookieManager();

        m_cefWidget = cef->create_widget(parent, "first-load-error", panel_cookies);
        if(!m_cefWidget) {
            m_fail = true;
            return nullptr;
        }

        m_cefWidget->setURL(url);
        
        ui->page_AuthBrowser->setLayout(new QHBoxLayout());
        ui->page_AuthBrowser->layout()->addWidget(m_cefWidget);

        //Hiding widget_ButtonBox hides border + margin so make the height 1px 
        ui->buttonBox->hide();
        ui->widget_ButtonBox->setFixedHeight(1);
    } else {
        m_cefWidget->setURL(url);
    }
    return m_cefWidget;
}
void AFAddStreamWidget::SetAuthData(std::string accessToken,
                                    std::string refreshToken,
                                    uint64_t expireTime,
                                    std::string channelID,
                                    std::string channelNick,
                                    std::string streamKey,
                                    std::string streamUrl)
{
    int res = _CacheAuth(accessToken, refreshToken, expireTime,
                         channelID, channelNick, streamKey, streamUrl);
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
    auto& authMan = AFAuthManager::GetSingletonInstance();

    if(
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
    if (_IsCefPlatform() && !m_cefWidget) {
        return QDialog::Rejected;
    }

    return QDialog::exec();
}

void AFAddStreamWidget::reject()
{
    if(m_cefWidget) {
        delete m_cefWidget;
    }
    QDialog::reject();
}
void AFAddStreamWidget::accept()
{
    if(m_cefWidget) {
        delete m_cefWidget;
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
                                   std::string customPassword)
{
    bool bmain = false;

    m_RawAuthData.strAccessToken = accessToken;
    m_RawAuthData.strRefreshToken = refreshToken;
    m_RawAuthData.uiExpireTime = expireTime;
    m_RawAuthData.strChannelID = channelID;
    m_RawAuthData.strChannelNick = channelNick;
    m_RawAuthData.strKeyRTMP = streamKey;
    m_RawAuthData.strUrlRTMP = streamUrl;
    m_RawAuthData.strPlatform = m_sPlatform.toStdString();
    if (uuid == "")
        m_RawAuthData.strUuid = QUuid::createUuid().toString().toStdString();
    else
        m_RawAuthData.strUuid = uuid;

    if (m_sPlatform == "SOOP Global")
    {
        m_RawAuthData.eType = AuthType::OAuth_StreamKey;
        bmain = true;
    }
    else
    {
        if (m_sPlatform == "Custom RTMP")
        {
            m_RawAuthData.eType = AuthType::No_OAuth_RTMP;
            if (ui->checkBox_RtmpAuth->isChecked())
            {
                m_RawAuthData.strCustomID = customID;
                m_RawAuthData.strCustomPassword = customPassword;
            }
        }
        else if (m_sPlatform == "afreecaTV")
        {
            m_RawAuthData.eType = AuthType::OAuth_StreamKey;
        }
        else if (m_sPlatform == "Youtube")
        {
            m_RawAuthData.eType = AuthType::OAuth_LinkedAccount;
        }
        else if (m_sPlatform == "Twitch")
        {
            m_RawAuthData.eType = AuthType::OAuth_StreamKey;
        }
        else
        {
            return false;
        }
    }    

    _ConvertAuthToStreamData();

    return AFAuthManager::GetSingletonInstance().CacheAuth(bmain, m_RawAuthData);;
}

void AFAddStreamWidget::_InitPage()
{
    ui->label_WindowTitle->setText(m_sPlatform);
    setWindowTitle(m_sPlatform);

    if (m_sPlatform == "Custom RTMP")
    {
        ui->stackedWidget->setCurrentIndex(1);

        ui->label_WindowTitle->setText(QTStr("Basic.Settings.Stream.Add.Custom.Channel"));
        setWindowTitle(QTStr("Basic.Settings.Stream.Add.Custom.Channel"));
    }
    else if (m_sPlatform == "Youtube")
    {
        _InitYoutubeLoginPage();
        ui->stackedWidget->setCurrentIndex(2);
    }
    else if(m_sPlatform == "Twitch")
    {
        _InitTwitchLoginPage();
        ui->stackedWidget->setCurrentIndex(2);
    }
    else if (m_sPlatform == "SOOP Global")
    {
        _InitSoopGlobalLoginPage();
        ui->stackedWidget->setCurrentIndex(2);
    }
    else if (m_sPlatform == "afreecaTV")
    {
        _InitSoopLoginPage();
        ui->stackedWidget->setCurrentIndex(2);
    }
    else
    {
        ui->stackedWidget->setCurrentIndex(2);

        QLabel* NotYet = new QLabel(this);
        NotYet->setText("Coming Soon....");
        NotYet->setAlignment(Qt::AlignCenter);

        ui->page_AuthBrowser->layout()->addWidget(NotYet);

        ui->label_WindowTitle->setText(QTStr("Basic.Settings.Stream.ChannelInfo"));
        setWindowTitle(QTStr("Basic.Settings.Stream.ChannelInfo"));
    }
}



void AFAddStreamWidget::_ConvertAuthToStreamData()
{
    QString streamKey = m_RawAuthData.strKeyRTMP.c_str();
    QString streamUrl = m_RawAuthData.strUrlRTMP.c_str();
    
    QString channelID = m_RawAuthData.strChannelID.c_str();
    m_ChannelNick = m_RawAuthData.strChannelNick.c_str();

    QString customID = m_RawAuthData.strCustomID.c_str();
    QString customPW = m_RawAuthData.strCustomPassword.c_str();

    ui->lineEdit_ChannelName->setText(channelID);
    ui->lineEdit_StreamKey->setText(streamKey);
    ui->lineEdit_Url->setText(streamUrl);
    ui->lineEdit_ID->setText(customID);
    ui->lineEdit_Password->setText(customPW);
}

void AFAddStreamWidget::_InitSoopGlobalLoginPage()
{
    if (m_auth)
        m_auth.reset();

    SoopGlobalAuth* streamAuth = new SoopGlobalAuth(soopGlobalDef, this);
    if (!streamAuth)
        return;

    if (!streamAuth->Login()) {
        delete streamAuth;
        return;
    }
    m_auth.reset(streamAuth);

    return;
    //std::string service = QT_TO_UTF8(m_sPlatform);
    //AFOAuth::DeleteCookies(service);
    //m_auth = AFOAuthStreamKey::Login(this, service);
    //if(!!m_auth) {
    //    //......
    //}
    //return;

    /*m_pIDEditorTestGlobalSoop = new QLineEdit(this);
    m_pPWEditorTestGlobalSoop = new QLineEdit(this);

    m_pPWEditorTestGlobalSoop->setEchoMode(QLineEdit::Password);

    QWidget* idArea = new QWidget(this);
    QWidget* pwArea = new QWidget(this);
    QHBoxLayout* layoutIdArea = new QHBoxLayout(this);
    QHBoxLayout* layoutPWArea = new QHBoxLayout(this);
    QLabel* labelId = new QLabel(this);
    QLabel* labelPW = new QLabel(this);

    labelId->setText("e-Mail : ");
    labelPW->setText("password : ");

    layoutIdArea->addWidget(labelId);
    layoutIdArea->addWidget(m_pIDEditorTestGlobalSoop);

    layoutPWArea->addWidget(labelPW);
    layoutPWArea->addWidget(m_pPWEditorTestGlobalSoop);

    idArea->setLayout(layoutIdArea);
    pwArea->setLayout(layoutPWArea);

    ui->page_AuthBrowser->layout()->addWidget(idArea);
    ui->page_AuthBrowser->layout()->addWidget(pwArea);*/
    //
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
    if(!streamAuth)
        return;

    if(!streamAuth->Login()) {
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

bool AFAddStreamWidget::_IsCefPlatform() {
    if (m_sPlatform == "afreecaTV" || m_sPlatform == "Youtube" ||
        m_sPlatform == "Twitch") {
        return true;
    }
    return false;
}
