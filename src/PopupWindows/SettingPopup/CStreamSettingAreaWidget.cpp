#include "CStreamSettingAreaWidget.h"
#include "ui_stream-setting-area.h"


#include "Application/CApplication.h"
#include "UIComponent/CStreamAccount.h"
#include "include/qt-wrapper.h"
#include "CSettingAreaUtils.h"
#include "CAddStreamWidget.h"
#include "UIComponent/CMessageBox.h"
#include "platform/platform.hpp"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Service/CService.h"

#include "src/ViewModel/Auth/Soop/auth-soop-global.h"
#include "src/ViewModel/Auth/Soop/auth-soop.hpp"
#include "src/ViewModel/Auth/Twitch/auth-twitch.h"

#include "ViewModel/Auth/CAuth.h"
#include "ViewModel/Auth/COAuthLogin.hpp"
#include "ViewModel/Auth/CAuthListener.hpp"
#include "ViewModel/Auth/YouTube/auth-youtube.hpp"
#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"
const QString SchedulDateAndTimeFormat = "yyyy-MM-dd'T'hh:mm:ss'Z'";

#define STREAM_CHANGED     &AFQStreamSettingAreaWidget::qslotStreamDataChanged


bool AFQStreamSettingAreaWidget::s_FirstAddAccountUI = true;

AFQStreamSettingAreaWidget::AFQStreamSettingAreaWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::AFQStreamSettingAreaWidget)
{
    ui->setupUi(this);
    
    std::string absPath;
    bool foundIcon = GetDataFilePath("assets/setting-dialog/Program/profile.png", absPath);
    if (foundIcon)
    {
        QString sts = QString("QLabel { image:url(%1);}").arg(absPath.c_str());
        ui->label_ProfilePicture->setStyleSheet(sts);
    }

    connect(this, &AFQStreamSettingAreaWidget::qsignalModified, this, &AFQStreamSettingAreaWidget::qslotChangeLabel);
}

AFQStreamSettingAreaWidget::~AFQStreamSettingAreaWidget()
{
    delete ui;
}

void AFQStreamSettingAreaWidget::qslotAddAccountTriggered()
{
    int buttoncount = m_AccountButtonList.count();
    if (m_MainAccountButton)
        buttoncount++;

    if (buttoncount >= 10)
    {
        AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
            this->parentWidget(),
            "",
            locale.Str("Basic.Settings.Stream.Account.Count.Warning"),
            true, false);
        return;
    }

    AFAddStreamWidget* addStream = new AFAddStreamWidget(this);
    addStream->AddStreamWidgetInit();

    if (addStream->exec() == QDialog::Accepted)
    {
        AFBasicAuth& resAuth = addStream->GetRawAuth();
        AFQStreamAccount* newAccount = _CreateStreamAccount(addStream, &resAuth, resAuth.strUuid.c_str());
        _CreateProfileImgObj(&resAuth, newAccount);
    }
}

void AFQStreamSettingAreaWidget::qslotAuthTriggered()
{
    QPushButton* platformButton = reinterpret_cast<QPushButton*>(sender());

    QString platform = platformButton->statusTip();

    AFAddStreamWidget* addStream = new AFAddStreamWidget(this);
    addStream->AddStreamWidgetInit(platform);
    
    if (addStream->exec() == QDialog::Accepted)
    {
        AFBasicAuth& resAuth = addStream->GetRawAuth();
        AFQStreamAccount* newAccount = _CreateStreamAccount(addStream, &resAuth, resAuth.strUuid.c_str());
        
        _CreateProfileImgObj(&resAuth, newAccount);
    }

    addStream->close();
    delete addStream;
    addStream = nullptr;
}

void AFQStreamSettingAreaWidget::qslotEditTriggred()
{
    if (m_CurrentAccountButton != nullptr)
    {
        QString platform = m_CurrentAccountButton->GetStreamAccountPlatform();
        if (platform == "Youtube")
        {
            int currIndx = 0;
            foreach(AFQStreamAccount * button, m_AccountButtonList)
            {
                if (button == m_CurrentAccountButton)
                    break;
                else
                    ++currIndx;
            }
            
            AFChannelData* channelData = nullptr;
            auto& authManager = AFAuthManager::GetSingletonInstance();
            authManager.GetChannelData(currIndx, channelData);
                        
            if (channelData != nullptr)
                App()->GetMainView()->ShowPopupPageYoutubeChannel(channelData->pAuthData);
        }
        else if (platform == "SOOP Global") {
            QString dashboard_url;
            dashboard_url = SOOP_GLOBAL_DASHBOARD_URL;
            QDesktopServices::openUrl(QUrl(dashboard_url));
        }
        else if (platform == "afreecaTV") {
            QString dashboard_url;
            dashboard_url = SOOP_DASHBOARD_URL;
            QDesktopServices::openUrl(QUrl(dashboard_url));
        }
        else if (platform == "Twitch") {
            QString dashboard_url;
            dashboard_url = TWITCH_DASHBOARD_URL +
                            m_CurrentAccountButton->GetChannelName() + "/home";
            QDesktopServices::openUrl(QUrl(dashboard_url));
        }
        else
        {
            AFAddStreamWidget* addStream = new AFAddStreamWidget(this);
            addStream->EditStreamWidgetInit(m_CurrentAccountButton->GetServer(), m_CurrentAccountButton->GetStreamKey(),
                m_CurrentAccountButton->GetChannelName(), m_CurrentAccountButton->GetID(), m_CurrentAccountButton->GetPassword());

            if (addStream->exec() == QDialog::Accepted)
            {
                _ModifyStreamAccount(addStream->GetUrl(), addStream->GetStreamKey(),
                    addStream->GetChannelName(), addStream->GetID(), addStream->GetPassword());
                emit qsignalModified(addStream->GetChannelName());
            }
        }
    }
}

void AFQStreamSettingAreaWidget::qslotSimulcastToggled(bool toggled)
{
    if (m_CurrentAccountButton != nullptr)
    {
        if (toggled)
        {
            if (App()->GetMainView()->IsStreamActive())
            {
                ui->pushButton_ToggleSimulcast->setChecked(false);
                ui->pushButton_ToggleSimulcast->ChangeState(false);
                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                    "", QTStr("Simulcast.Start.Denied"), false, true);
                return;
            }
        }

        int cntToggled = 0;
        foreach(AFQStreamAccount * button, m_AccountButtonList)
        {
            auto name = button->GetStreamAccountPlatform();
            auto live = button->GetOnLive();
            if (button->GetOnLive())
                if(button->GetStreamAccountPlatform() != "SOOP Global")
                    cntToggled++;
        }


        if (toggled)
        {
            if (m_CurrentAccountButton->GetStreamAccountPlatform() != "SOOP Global")
            {
                if (cntToggled < 4)
                    m_CurrentAccountButton->SetOnLive(toggled);
                else
                {
                    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                        "",
                        QTStr("Simulcast.Max.Start"));
                    ui->pushButton_ToggleSimulcast->ChangeState(false);
                    ui->pushButton_ToggleSimulcast->setChecked(false);
                }
            }
            else
            {
                m_CurrentAccountButton->SetOnLive(toggled);
                m_CurrentAccountButton->SetModified(true);
            }

            
        }
        else
        {
            if (m_MainAccountButton)
                if (m_MainAccountButton->GetOnLive())
                    cntToggled++;

            if (App()->GetMainView()->IsStreamActive() && cntToggled <= 1)
            {
                ui->pushButton_ToggleSimulcast->setChecked(true);
                ui->pushButton_ToggleSimulcast->ChangeState(true);
                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                    "", QTStr("Simulcast.Stop.Denied"), false, true);

                return;
            }
            m_CurrentAccountButton->SetOnLive(toggled);
            m_CurrentAccountButton->SetModified(true);
        }
    }
}

void AFQStreamSettingAreaWidget::qslotAccountButtonTriggered(bool selected)
{
    AFQStreamAccount* senderButton = reinterpret_cast<AFQStreamAccount*>(sender());

    if(m_MainAccountButton)
        m_MainAccountButton->setChecked(false);

    foreach(AFQStreamAccount * button, m_AccountButtonList)
    {
        button->setChecked(false);
    }

    senderButton->setChecked(true);
    m_CurrentAccountButton = senderButton;

    QPixmap* savedPixmap = m_CurrentAccountButton->GetPixmapProfileImgObj();
    
    if (savedPixmap != nullptr)
        ui->label_ProfilePicture->setPixmap(*savedPixmap);
    else
    {
        std::string absPath;
        bool foundIcon = GetDataFilePath("assets/setting-dialog/Program/profile.png", absPath);
        if (foundIcon)
        {
            QSize iconSize = ui->label_ProfilePicture->size();
            QPixmap scaled = QPixmap(absPath.c_str()).scaled(iconSize,
                                                             Qt::IgnoreAspectRatio,
                                                             Qt::SmoothTransformation);
            ui->label_ProfilePicture->setPixmap(scaled);
        }
    }
    
    ui->label_Nickname->setText(m_CurrentAccountButton->GetChannelName());
    ui->label_Nickname->setToolTip(m_CurrentAccountButton->GetChannelName());
    ui->label_IdNumber->setText(m_CurrentAccountButton->GetChannelNick());
    ui->label_IdNumber->setToolTip(m_CurrentAccountButton->GetChannelNick());


    std::string absPath;
    bool foundIcon = false;
    if (senderButton->GetStreamAccountPlatform() == "SOOP Global")
        foundIcon = GetDataFilePath("assets/platform/default/soopglobal.png", absPath);
    else if (senderButton->GetStreamAccountPlatform() == "afreecaTV")
        foundIcon = GetDataFilePath("assets/platform/default/soop.png", absPath);
    else if (senderButton->GetStreamAccountPlatform() == "Twitch")
        foundIcon = GetDataFilePath("assets/platform/default/twitch.png", absPath);
    else if (senderButton->GetStreamAccountPlatform() == "Youtube")
        foundIcon = GetDataFilePath("assets/platform/default/youtube.png", absPath);
    else if (senderButton->GetStreamAccountPlatform() == "Custom RTMP")
        foundIcon = GetDataFilePath("assets/platform/default/rtmp.png", absPath);
    

    if (foundIcon)
    {
        QString sts = QString("QLabel { image:url(%1); }").arg(absPath.c_str());
        ui->label_Platform->setStyleSheet(sts);
    }
    
    
    if (m_CurrentAccountButton->GetStreamAccountPlatform() == "Custom RTMP")
    {
        ui->label_IdNumber->setText("");
    }

    bool onliveCurrAccount = m_CurrentAccountButton->GetStateLive();
    
    if (ui->pushButton_ToggleSimulcast->isChecked() != onliveCurrAccount)
    {
        ui->pushButton_ToggleSimulcast->setChecked(onliveCurrAccount);
        ui->pushButton_ToggleSimulcast->ChangeState(onliveCurrAccount);
    }

    if (senderButton->GetStreamAccountPlatform() == "Youtube") {
        AFAuthManager& authManager = AFAuthManager::GetSingletonInstance();
        if (-1 == authManager.IsRegisterChannel(
                      senderButton->GetUuid().toStdString().c_str())) {
            ui->widget_ChannelInfo->setEnabled(false);
        }
        else {
            ui->widget_ChannelInfo->setEnabled(true);
        }
    }
    else {
        ui->widget_ChannelInfo->setEnabled(true);
    }
}

void AFQStreamSettingAreaWidget::qslotPlatformSelectTriggered()
{

}

void AFQStreamSettingAreaWidget::qslotReleaseAccount()
{
    auto& localeMan = AFLocaleTextManager::GetSingletonInstance();
    int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this, "",
        QT_UTF8(localeMan.Str("Basic.Settings.Stream.DeleteAccountAlert")));

    if (result ==QDialog::Rejected)
    {
        return;
    }
    else
    {
        _ReleaseAccount();
    }
}

void AFQStreamSettingAreaWidget::qslotResetStreamSettingUi()
{
}

void AFQStreamSettingAreaWidget::qslotStreamDataChanged()
{
    if (!m_bLoading)
    {
        m_bStreamDataChanged = true;
        sender()->setProperty("changed", QVariant(true));
        emit qsignalStreamDataChanged();
    }
}

void AFQStreamSettingAreaWidget::qslotChangeLabel(QString channelName)
{
    m_CurrentAccountButton->SetChannelName(channelName);
}


void AFQStreamSettingAreaWidget::StreamSettingAreaInit()
{    
    _SetStreamSettings();
    _SetStreamLanguage(); 
    LoadStreamAccountSaved();
    _LoadStreamAccountSettings();
    ToggleOnStreaming(false);
}

void AFQStreamSettingAreaWidget::FindAccountButtonWithID(QString id)
{
    for (int index = 0; index < m_AccountButtonList.count(); index++)
    {
        if (m_AccountButtonList[index]->GetChannelName() == id)
        {
            m_AccountButtonList[index]->clicked();
            int i = ui->scrollArea_AccountList->verticalScrollBar()->maximum();
            int val = (m_AccountButtonList.count() - index - 1) * 70;
            ui->scrollArea_AccountList->verticalScrollBar()->setValue(val);
            break;
        }
    }
}

void AFQStreamSettingAreaWidget::SaveStreamSettings()
{
    auto& authManager = AFAuthManager::GetSingletonInstance();

    if (App()->GetMainView()->GetStreamingCheck())
    {
        if (m_MainAccountButton && m_MainAccountButton->GetModified())
        {
            AFChannelData* channeldata = nullptr;
            authManager.GetMainChannelData(channeldata);
            channeldata->bIsStreaming = m_MainAccountButton->GetOnLive();
        }

        int index = 0;
        for (index; index < m_AccountButtonList.count(); index++)
        {
            if (m_AccountButtonList[index]->GetOnLive())
                continue;

            AFChannelData* channeldata = nullptr;
            if (m_AccountButtonList[index]->GetModified())
            {
                AFChannelData* channeldata = nullptr;
                authManager.GetChannelData(index, channeldata);
                channeldata->bIsStreaming = m_AccountButtonList[index]->GetOnLive();
            }
        }
        return;
    }

    authManager.ClearRegisterChannel();

    bool GlobalNotRegistered = false;

    int index = 0;
    for (index; index < m_AccountButtonList.count(); index++)
    {
        AFChannelData* channeldata = nullptr;
        channeldata = new AFChannelData();

        QString uuidTemp = m_AccountButtonList[index]->GetUuid();
        std::string strUuid = uuidTemp.toStdString();
        channeldata->bIsStreaming = m_AccountButtonList[index]->GetOnLive();
        int chindex = authManager.GetCntChannel();
        authManager.RegisterChannel(strUuid.c_str(), channeldata);
        
        if (m_AccountButtonList[index]->GetModified())
        {
            channeldata->pAuthData->strChannelID        = m_AccountButtonList[index]->GetChannelName().
                                                                            toStdString().c_str();
            channeldata->pAuthData->strUrlRTMP          = m_AccountButtonList[index]->GetServer().
                                                                            toStdString().c_str();
            channeldata->pAuthData->strKeyRTMP          = m_AccountButtonList[index]->GetStreamKey().
                                                                            toStdString().c_str();
            channeldata->pAuthData->strCustomID         = m_AccountButtonList[index]->GetID().
                                                                            toStdString().c_str();
            channeldata->pAuthData->strCustomPassword   = m_AccountButtonList[index]->GetPassword().
                                                                            toStdString().c_str();
            channeldata->pAuthData->strPlatform         = m_AccountButtonList[index]->GetStreamAccountPlatform().
                                                                            toStdString().c_str();
        }

        m_AccountButtonList[index]->CompleteRegist();
        channeldata->pObjQtPixmap = (void*)m_AccountButtonList[index]->GetPixmapProfileImgObj();
        
    }

    if (m_MainAccountButton)
    {
        AFChannelData* channeldata = nullptr;
        channeldata = new AFChannelData();
        QString uuidTemp = m_MainAccountButton->GetUuid();
        std::string strUuid = uuidTemp.toStdString();
        channeldata->bIsStreaming = m_MainAccountButton->GetOnLive();
        authManager.RegisterChannel(strUuid.c_str(), channeldata);
        m_MainAccountButton->CompleteRegist();
        channeldata->pObjQtPixmap = (void*)m_MainAccountButton->GetPixmapProfileImgObj();
        
        
    }

    authManager.SetSoopRegistered(authManager.IsSoopPreregistered());
    authManager.SetSoopGlobalRegistered(authManager.IsSoopGlobalPreregistered());
    authManager.SetTwitchRegistered(authManager.IsTwitchPreregistered());
    authManager.SetYoutubeRegistered(authManager.IsYoutubePreregistered());
    return;
}

void AFQStreamSettingAreaWidget::ToggleOnStreaming(bool streaming)
{
    bool useVideo = obs_video_active() ? false : true;
    ui->widget_DisconnectChannel->setEnabled(useVideo);
    ui->pushButton_AddPlatform->setEnabled(useVideo);

    std::string absPath;
    if (ui->pushButton_AddPlatform->isEnabled()) {
        GetDataFilePath("assets/setting-dialog/stream/bt_add.svg", absPath);
    } else {
        GetDataFilePath("assets/setting-dialog/stream/bt_add-disabled.svg",
                        absPath);
    }
    QIcon buttonIcon(absPath.c_str());
    ui->pushButton_AddPlatform->setIcon(buttonIcon);
}

void AFQStreamSettingAreaWidget::LoadStreamAccountSaved()
{
    auto& authManager = AFAuthManager::GetSingletonInstance();

    m_AccountButtonList.clear();

    QList<AFQStreamAccount*> mainbuttons = ui->widget_Main->findChildren<AFQStreamAccount*>();
    foreach(AFQStreamAccount * button, mainbuttons)
    {
        ui->widget_Main->layout()->removeWidget(button);
        button->close();
        delete button;
    }

    m_MainAccountButton = nullptr;

    QList<AFQStreamAccount*> buttons = ui->widget_OtherAccount->findChildren<AFQStreamAccount*>();
    foreach(AFQStreamAccount * button, buttons)
    {
        ui->widget_OtherAccount->layout()->removeWidget(button);
        button->close();
        delete button;
    }

    bool haveChannel = false;

    int cntList = authManager.GetCntChannel();

    for (int idx = 0; idx < cntList; idx++)
    {
        AFChannelData* tmpChannelNode = nullptr;
        AFBasicAuth* tmpAuthNode = nullptr;
        authManager.GetChannelData(idx, tmpChannelNode);

        if (tmpChannelNode == nullptr)
            continue;

        tmpAuthNode = tmpChannelNode->pAuthData;

        s_FirstAddAccountUI = false;

        if (tmpAuthNode->strPlatform == "SOOP Global")
            authManager.SetSoopGlobalRegistered(true);
        else if (tmpAuthNode->strPlatform == "afreecaTV")
            authManager.SetSoopRegistered(true);
        else if (tmpAuthNode->strPlatform == "Youtube")
            authManager.SetYoutubeRegistered(true);
        else if (tmpAuthNode->strPlatform == "Twitch")
            authManager.SetTwitchRegistered(true);

        AFQStreamAccount* newAccount = _CreateStreamAccount(tmpAuthNode->strPlatform.c_str(),
                                                            tmpAuthNode->strChannelID.c_str(),
                                                            tmpAuthNode->strChannelNick.c_str(),
                                                            tmpChannelNode->bIsStreaming,
                                                            tmpAuthNode->strUrlRTMP.c_str(),
                                                            tmpAuthNode->strKeyRTMP.c_str(),
                                                            tmpAuthNode->strCustomID.c_str(), 
                                                            tmpAuthNode->strCustomPassword.c_str(),
                                                            tmpAuthNode->strUuid.c_str(),
                                                            true);
        
        if (tmpChannelNode->pObjQtPixmap != nullptr)
        {
            QPixmap* savedPixmap = (QPixmap*)tmpChannelNode->pObjQtPixmap;
            newAccount->CompleteRegist();
            newAccount->SetPixmapProfileImgObj(savedPixmap);
            ui->label_ProfilePicture->setPixmap(*savedPixmap);
        }
        haveChannel |= true;
    }

    haveChannel |= LoadMainAccount();

    if (haveChannel)
        ui->stackedWidget->setCurrentIndex(0);
    else
        ui->stackedWidget->setCurrentIndex(1);
}

bool AFQStreamSettingAreaWidget::LoadMainAccount()
{
    auto& authManager = AFAuthManager::GetSingletonInstance();

    AFChannelData* mainChannelNode = nullptr;
    if (authManager.GetMainChannelData(mainChannelNode))
    {
        if (m_MainAccountButton == nullptr)
        {
            AFBasicAuth* mainAuthNode = mainChannelNode->pAuthData;
            AFQStreamAccount* newAccount = _CreateStreamAccount(mainAuthNode->strPlatform.c_str(),
                                                                mainAuthNode->strChannelID.c_str(),
                                                                mainAuthNode->strChannelNick.c_str(),
                                                                mainChannelNode->bIsStreaming,
                                                                mainAuthNode->strUrlRTMP.c_str(),
                                                                mainAuthNode->strKeyRTMP.c_str(),
                                                                mainAuthNode->strCustomID.c_str(),
                                                                mainAuthNode->strCustomPassword.c_str(),
                                                                mainAuthNode->strUuid.c_str(),
                                                                true);
            
            if (mainChannelNode->pObjQtPixmap != nullptr)
            {
                QPixmap* savedPixmap = (QPixmap*)mainChannelNode->pObjQtPixmap;
                if (savedPixmap) {
                    newAccount->CompleteRegist();
                    newAccount->SetPixmapProfileImgObj(savedPixmap);
                    ui->label_ProfilePicture->setPixmap(*savedPixmap);
                }
            }
        }
        return true;
    }
    else
    {
        if (m_MainAccountButton)
        {
            m_MainAccountButton = nullptr;
            s_FirstAddAccountUI &= true;
        }

        return false;
    }    
}

void AFQStreamSettingAreaWidget::_SetStreamSettings()
{
    m_CurrentAccountButton = nullptr;

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

    QString styleWithDisabled =
        "QLabel { image: url(%1); } QLabel:disabled { image: url(%2);}";
    std::string absPathDisabled;
    GetDataFilePath("assets/setting-dialog/stream/channel-disconnect.svg",
                    absPath);
    GetDataFilePath(
        "assets/setting-dialog/stream/channel-disconnect-disabled.svg",
        absPathDisabled);
    ui->label_DisconnectChannelIcon->setStyleSheet(
        styleWithDisabled.arg(absPath.c_str()).arg(absPathDisabled.c_str()));

    GetDataFilePath("assets/setting-dialog/stream/channel-info.svg", absPath);
    GetDataFilePath("assets/setting-dialog/stream/channel-info-disabled.svg", absPathDisabled);
    ui->label_ChannelInfoIcon->setStyleSheet(
        styleWithDisabled.arg(absPath.c_str()).arg(absPathDisabled.c_str()));

    connect(ui->pushButton_AddPlatform, &QPushButton::clicked,
        this, &AFQStreamSettingAreaWidget::qslotAddAccountTriggered);

    connect(ui->pushButton_ToggleSimulcast, &QPushButton::clicked, 
        this, &AFQStreamSettingAreaWidget::qslotSimulcastToggled);
    connect(ui->widget_ChannelInfo, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotEditTriggred);
    connect(ui->widget_DisconnectChannel, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotReleaseAccount);

    ui->widget_AddSoopGlobal->setStatusTip("SOOP Global");
    ui->widget_AddSoop->setStatusTip("afreecaTV");
    ui->widget_AddTwitch->setStatusTip("Twitch");
    ui->widget_AddYoutube->setStatusTip("Youtube");
    ui->widget_AddRTMP->setStatusTip("Custom RTMP");

    connect(ui->widget_AddSoopGlobal, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotAuthTriggered);
    connect(ui->widget_AddSoop, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotAuthTriggered);
    connect(ui->widget_AddTwitch, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotAuthTriggered);
    connect(ui->widget_AddYoutube, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotAuthTriggered);
    connect(ui->widget_AddRTMP, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotAuthTriggered);
    
    AFSettingUtils::HookWidget(ui->pushButton_ToggleSimulcast, this, BUTTON_CLICKED, STREAM_CHANGED);

    ui->label_SoopGlobalConnected->setVisible(false);
    ui->label_SoopConnected->setVisible(false);
    ui->label_TwitchConnected->setVisible(false);
    ui->label_RTMPConnected->setVisible(false);
    ui->label_YoutubeConnected->setVisible(false);
}

void AFQStreamSettingAreaWidget::_SetStreamLanguage()
{
}


void AFQStreamSettingAreaWidget::_LoadStreamAccountSettings()
{
    if (m_AccountButtonList.isEmpty())
    {
        ui->stackedWidget->setCurrentIndex(1);
    }
    else
    {
        ui->stackedWidget->setCurrentIndex(0);
    }
}

void AFQStreamSettingAreaWidget::_LoadStreamAccountSettings(AFQStreamAccount* account)
{
}

void AFQStreamSettingAreaWidget::_ReleaseAccount()
{
    auto& authManager = AFAuthManager::GetSingletonInstance();

    ui->widget_OtherAccount->layout()->removeWidget(m_CurrentAccountButton);

    if (m_CurrentAccountButton == m_MainAccountButton)
    {
        authManager.SetSoopGlobalPreregistered(false);
        authManager.SetSoopGlobalPreregistered(false);
        authManager.RemoveMainChannel();
        App()->GetMainView()->qslotLogoutGlobalPage();
        m_MainAccountButton->deleteLater();
        m_MainAccountButton = nullptr;
        m_CurrentAccountButton = nullptr;
    }
    else
    {
        int deleteindex = 0;
        for (deleteindex; deleteindex < m_AccountButtonList.count(); deleteindex++)
        {
            if (m_AccountButtonList[deleteindex] == m_CurrentAccountButton)
            {
                QString platform = m_AccountButtonList[deleteindex]->GetStreamAccountPlatform();
                if (platform == "afreecaTV")
                    authManager.SetSoopRegistered(false);
                else if (platform == "Youtube")
                    authManager.SetYoutubePreregistered(false);
                else if (platform == "Twitch")
                    authManager.SetTwitchPreregistered(false);
                std::string strUuid = m_AccountButtonList[deleteindex]->GetUuid().toStdString();
                authManager.RemoveCachedAuth(strUuid.c_str());
                break;
            }
        }
        m_AccountButtonList.remove(deleteindex);

        m_CurrentAccountButton->deleteLater();
        m_CurrentAccountButton = nullptr;
    }

    if (m_AccountButtonList.count() == 0 && !m_MainAccountButton)
    {
        s_FirstAddAccountUI = true;
        ui->stackedWidget->setCurrentIndex(1);
    }
    else
    {
        if (m_MainAccountButton)
            m_MainAccountButton->clicked();
        else
            m_AccountButtonList[0]->click();
    }

    qslotStreamDataChanged();
}


void AFQStreamSettingAreaWidget::_ShowSimulcastWithLogin()
{
    ui->pushButton_ToggleSimulcast->setChecked(true);
    ui->pushButton_ToggleSimulcast->ChangeState(true);
}

void AFQStreamSettingAreaWidget::_ShowSimulcastWithoutLogin()
{
    ui->pushButton_ToggleSimulcast->setChecked(true);
    ui->pushButton_ToggleSimulcast->ChangeState(true);
}

void AFQStreamSettingAreaWidget::_ShowLoginWithoutSimulcast()
{
    ui->pushButton_ToggleSimulcast->setChecked(false);
    ui->pushButton_ToggleSimulcast->ChangeState(false);
}

void AFQStreamSettingAreaWidget::_ShowDefaultSetting()
{
    ui->pushButton_ToggleSimulcast->setChecked(false);
    ui->pushButton_ToggleSimulcast->ChangeState(false);
}

QPixmap* AFQStreamSettingAreaWidget::_CreateProfileImgObj(AFBasicAuth* pData, AFQStreamAccount*& outRefAccount)
{
    if (pData == nullptr)
        return nullptr;
    
    auto& authManager = AFAuthManager::GetSingletonInstance();
    
    
    if (pData->strPlatform == "")
        pData->strPlatform = outRefAccount->GetStreamAccountPlatform().toStdString();
    
    
    QPixmap* resPixmap = App()->GetMainView()->MakePixmapFromAuthData(pData);
    
    if (resPixmap != nullptr)
    {
        if (outRefAccount->GetPixmapProfileImgObj() == nullptr)
            outRefAccount->SetPixmapProfileImgObj(resPixmap);
        ui->label_ProfilePicture->setPixmap(*resPixmap);
    }
    
    return resPixmap;
}

AFQStreamAccount* AFQStreamSettingAreaWidget::_CreateStreamAccount(AFAddStreamWidget* pUIObjAddStream,
                                                                   AFBasicAuth* pAuthData,
                                                                   QString uuid)
{
    return _CreateStreamAccount(pUIObjAddStream->GetPlatform(), pUIObjAddStream->GetChannelName(),
                                pUIObjAddStream->GetChannelNick(),
                                false,
                                pAuthData->strUrlRTMP.c_str(), pAuthData->strKeyRTMP.c_str(),
                                pUIObjAddStream->GetID(),
                                pUIObjAddStream->GetPassword(),
                                uuid);
}

AFQStreamAccount* AFQStreamSettingAreaWidget::_CreateStreamAccount(QString platformName, QString channelName,
                                                                   QString channelNickName, bool onLive,
                                                                   QString server, QString streamKey,
                                                                   QString id, QString password,  
                                                                   QString uuid, bool bFromSavedFile /*= false*/)
{
    if (s_FirstAddAccountUI)
    {
        s_FirstAddAccountUI = false;
        
        onLive = true;
    }

    auto& authManager = AFAuthManager::GetSingletonInstance();
    if(platformName == "SOOP Global")
        authManager.SetSoopGlobalPreregistered(true);
    else if (platformName == "afreecaTV")
        authManager.SetSoopPreregistered(true);
    else if (platformName == "Youtube")
        authManager.SetYoutubePreregistered(true);
    else if (platformName == "Twitch")
        authManager.SetTwitchPreregistered(true);
        
    AFQStreamAccount* account = new AFQStreamAccount(this);
    account->StreamAccountAreaInit(platformName, channelName, channelNickName, id, password, onLive, server, streamKey, uuid);
    connect(account, &QPushButton::clicked, this, &AFQStreamSettingAreaWidget::qslotAccountButtonTriggered);
    QVBoxLayout* layout = reinterpret_cast<QVBoxLayout*>(ui->widget_OtherAccount->layout());


    
    int index = 0;

    if (platformName == "SOOP Global")
    {
        m_MainAccountButton = account;
        ui->widget_Main->layout()->addWidget(m_MainAccountButton);
    }
    else
    {
        layout->insertWidget(0, account);
        m_AccountButtonList.append(account);
    }

    ui->stackedWidget->setCurrentIndex(0);
    account->clicked();

    
    if (bFromSavedFile == false)
        qslotStreamDataChanged();
    
    return account;
}

void AFQStreamSettingAreaWidget::_ModifyStreamAccount(QString server, QString streamkey, QString channelName, QString id, QString password)
{
    if (m_CurrentAccountButton != nullptr)
    {
        m_CurrentAccountButton->SetServer(server);
        m_CurrentAccountButton->SetStreamKey(streamkey);
        m_CurrentAccountButton->SetChannelName(channelName);
        m_CurrentAccountButton->SetStreamAccountID(id);
        m_CurrentAccountButton->SetPassword(password);
        m_CurrentAccountButton->SetModified(true);

        ui->label_Nickname->setText(channelName);
        ui->label_Nickname->setToolTip(channelName);
        m_CurrentAccountButton->repaint();

        qslotStreamDataChanged();
    }
}
