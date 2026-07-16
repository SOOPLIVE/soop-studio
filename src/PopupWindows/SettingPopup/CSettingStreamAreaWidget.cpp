#include "CSettingStreamAreaWidget.h"
#include "ui_setting-stream-area.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "UIComponent/CStreamAccount.h"
#include "CSettingUtils.h"
#include "CAddStreamWidget.h"
#include "UIComponent/CMessageBox.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Service/CService.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Source/CSource.h"

#include "MainFrame/Output/COutput.h"

#include "src/ViewModel/Auth/Soop/auth-soop.hpp"
#include "src/ViewModel/Auth/Twitch/auth-twitch.h"


#include "ViewModel/Auth/CAuth.h"
#include "ViewModel/Auth/COAuthLogin.hpp"
#include "ViewModel/Auth/CAuthListener.hpp"
#include "ViewModel/Auth/YouTube/auth-youtube.hpp"
#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"

const QString SchedulDateAndTimeFormat = "yyyy-MM-dd'T'hh:mm:ss'Z'";
#define STREAM_CHANGED     &AFQStreamSettingAreaWidget::qslotStreamDataChanged


bool AFQStreamSettingAreaWidget::s_firstAddAccountUI = true;

AFQStreamSettingAreaWidget::AFQStreamSettingAreaWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::AFQStreamSettingAreaWidget)
{
    ui->setupUi(this);
 
    std::string absPath;
    bool foundIcon = GetDataFilePath("assets/setting-dialog/Program/profile.png", absPath);
    if (foundIcon)
    {
        //Image QSS higher qulality on png
        QString sts = QString("QLabel { image:url(%1);}").arg(absPath.c_str());
        ui->label_ProfilePicture->setStyleSheet(sts);
    }

    connect(this, &AFQStreamSettingAreaWidget::qsignalModified, this, &AFQStreamSettingAreaWidget::qslotChangeLabel);

    ui->scrollArea_AccountList->setWidgetResizable(true);
    ui->pushButton_ChannelSetting->setProperty("buttonType", "settings");
    PolishStyleSheet(ui->pushButton_ChannelSetting);
}

AFQStreamSettingAreaWidget::~AFQStreamSettingAreaWidget()
{
    if(m_broadInfoWidget)
        m_broadInfoWidget->deleteLater();

    delete ui;
}

void AFQStreamSettingAreaWidget::qslotShowAddAccount()
{
    if (AFOutputUtil::IsStreamActive())
        return;

    int buttoncount = m_accountButtonList.count();
    if (m_pMainAccountButton)
        buttoncount++;

    if (buttoncount >= 10)
    {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
            this->parentWidget(),
            "",
            Str("Basic.Settings.Stream.Account.Count.Warning"),
            true, false);
        return;
    }

    AFAddStreamWidget* addStream = new AFAddStreamWidget(this);
    addStream->AddStreamWidgetInit();

    auto& authManager = AUTH_CONTEXT;
    //
    if (addStream->exec() == QDialog::Accepted)
    {
        AFBasicAuth& resAuth = addStream->GetRawAuth();

        AFQStreamAccount* newAccount = _CreateStreamAccount(addStream, &resAuth, resAuth.uuid.c_str());

        if (addStream->GetPlatform() == PLATFORM_SOOP)
        {
            newAccount->SetCookie(addStream->GetCookie());
            AFChannelData* channeldata = nullptr;
            channeldata = new AFChannelData();
            QString uuidTemp = newAccount->GetUuid();
            std::string strUuid = uuidTemp.toStdString();
            authManager.RegisterChannel(strUuid.c_str(), channeldata);
            channeldata->imgUrlUserThumb = addStream->GetThumbnailPath();

            channeldata->pAuthData->cookie = addStream->GetCookie();

            authManager.GetSoopBroadInfo()->ReceiveSoopStreamerInfoWithTempCookie(this, "qslotGetSoopStreamerInfo", newAccount->GetCookie());
            authManager.RequestBroadInfoAPI();
        }
        else
            _CreateProfileImgObj(&resAuth, newAccount);

        RefreshAccountButtons();

        //Layout update -> Size update : need time for size update to maximum scroll
        QTimer::singleShot(10, [=]() {
            FindAccountButtonWithID(newAccount->GetChannelName(), newAccount->GetStreamAccountPlatform());
            });
    }

    if (addStream)
        addStream->deleteLater();
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
        AFQStreamAccount* newAccount = _CreateStreamAccount(addStream, &resAuth, resAuth.uuid.c_str());

        if (resAuth.platform == PLATFORM_SOOP)
            newAccount->SetCookie(resAuth.cookie);

        _CreateProfileImgObj(&resAuth, newAccount);

        RefreshAccountButtons();
    }

    addStream->close();
    delete addStream;
    addStream = nullptr;
}

void AFQStreamSettingAreaWidget::qslotEditTriggred()
{
    if (m_pCurrentAccountButton != nullptr)
    {
        QString platform = m_pCurrentAccountButton->GetStreamAccountPlatform();
        if (platform == PLATFORM_YOUTUBE)
        {
            AFChannelData* channelData = nullptr;
            AUTH_CONTEXT.GetChannelData(PLATFORM_YOUTUBE, channelData);                        
            if (channelData != nullptr)
                MAINFRAME->ShowPopupPageYoutubeChannel(channelData->pAuthData);
        }
        else if (platform == PLATFORM_SOOP) {
            QString dashboard_url;
            dashboard_url = QString::fromStdString(SOOP_DASHBOARD_URL);
            MAINFRAME->NavigateDefaultBrowser(dashboard_url);
        }
        else if (platform == PLATFORM_TWITCH) {
            QString dashboard_url;
            dashboard_url = TWITCH_DASHBOARD_URL +
                            m_pCurrentAccountButton->GetChannelName() + "/home";
            MAINFRAME->NavigateDefaultBrowser(dashboard_url);
        }
        else
        {
            AFAddStreamWidget* addStream = new AFAddStreamWidget(this);
            addStream->EditStreamWidgetInit(m_pCurrentAccountButton->GetServer(), m_pCurrentAccountButton->GetStreamKey(),
                m_pCurrentAccountButton->GetChannelName(), m_pCurrentAccountButton->GetID(), m_pCurrentAccountButton->GetPassword());

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
    QString mainPlatform;
    if (m_pCurrentAccountButton != nullptr)
    {
        if (toggled)
        {
            if (AFOutputUtil::IsStreamActive())
            {
                do
                {
                    AFChannelData * soopData = nullptr;
                    AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, soopData);
                    if (soopData)
                    {
                        bool soopIsLive = soopData->isStreaming;
                        if (soopIsLive)
                        {
                            AFQBroadInfo* info = AUTH_CONTEXT.GetSoopBroadInfo();
                            if (info)
                            {
                                int subscribeLive = info->SubscribeBroad();
                                if (subscribeLive > 0)
                                {
                                    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                        "", QTStr("Simulcast.Reject.Subscribe.Start.Broad"), false, true);
                                    break;
                                }
                            }
                        }
                    }

                    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                        "", QTStr("Simulcast.Toggle.Refuse"), false, true);

                } while (false);

                ui->pushButton_ToggleSimulcast->setChecked(false);
                ui->pushButton_ToggleSimulcast->ChangeState(false);

                return;
            }
        }

        int cntToggled = 0;
        foreach(AFQStreamAccount * button, m_accountButtonList)
        {
            auto name = button->GetStreamAccountPlatform();
            auto live = button->GetOnLive();
            if (button->GetOnLive())
                if(button->GetStreamAccountPlatform() != mainPlatform)
                    cntToggled++;
        }


        if (toggled)
        {
            QString currentPlatform = m_pCurrentAccountButton->GetStreamAccountPlatform();
            if (currentPlatform != mainPlatform)
            {
                if (cntToggled < 4)
                {
                    m_pCurrentAccountButton->SetOnLive(toggled);

                    AFChannelData* data = nullptr;
                    std::string platform = m_pCurrentAccountButton->GetStreamAccountPlatform().toStdString();
                    AUTH_CONTEXT.GetChannelData(platform, data);
                    if (data)
                        data->isStreaming = toggled;
                    m_pCurrentAccountButton->SetModified(true);
                }
                else
                {
                    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                               "", QTStr("Simulcast.Max.Start"));
                    ui->pushButton_ToggleSimulcast->ChangeState(false);
                    ui->pushButton_ToggleSimulcast->setChecked(false);
                }
            }
            else
            {
                m_pCurrentAccountButton->SetOnLive(toggled);
                m_pCurrentAccountButton->SetModified(true);

                AFChannelData* data = nullptr;
                std::string platform = m_pCurrentAccountButton->GetStreamAccountPlatform().toStdString();
                AUTH_CONTEXT.GetChannelData(platform, data);
                data->isStreaming = true;
            }
            AFSourceUtil::CheckAddSoopAI("soop_aimanager_source", true);
        }
        else
        {
            if (m_pMainAccountButton)
                if (m_pMainAccountButton->GetOnLive())
                    cntToggled++;

            m_pCurrentAccountButton->SetOnLive(toggled);

            AFChannelData* data = nullptr;
            std::string platform = m_pCurrentAccountButton->GetStreamAccountPlatform().toStdString();
            AUTH_CONTEXT.GetChannelData(platform, data);
            if(data)
                data->isStreaming = toggled;
            m_pCurrentAccountButton->SetModified(true);
        }
    }

    MAIN_OUTPUT->SetStreamingOutput();
}

void AFQStreamSettingAreaWidget::qslotAccountButtonTriggered(bool selected)
{
    AFQStreamAccount* senderButton = reinterpret_cast<AFQStreamAccount*>(sender());

    ui->pushButton_Disconnect->setEnabled(true);
    //Set All Button False
    if(m_pMainAccountButton)
        m_pMainAccountButton->setChecked(false);

    foreach(AFQStreamAccount * button, m_accountButtonList)
        button->setChecked(false);

    //Clicked Button Set True
    senderButton->setChecked(true);
    m_pCurrentAccountButton = senderButton;

    //ProfilePic
    QPixmap* savedPixmap = m_pCurrentAccountButton->GetPixmapProfileImgObj();
    
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

    //Channel Info
    ui->label_Nickname->setText(m_pCurrentAccountButton->GetChannelName());
    ui->label_Nickname->setToolTip(m_pCurrentAccountButton->GetChannelName());
    ui->label_IdNumber->setText(m_pCurrentAccountButton->GetChannelNick());
    ui->label_IdNumber->setToolTip(m_pCurrentAccountButton->GetChannelNick());

    std::string absPath;
    bool foundIcon = false;

    if (senderButton->GetStreamAccountPlatform() == PLATFORM_SOOP)
        foundIcon = GetDataFilePath("assets/platform/default/soop.png", absPath);
    else if (senderButton->GetStreamAccountPlatform() == PLATFORM_TWITCH)
        foundIcon = GetDataFilePath("assets/platform/default/twitch.svg", absPath);
    else if (senderButton->GetStreamAccountPlatform() == PLATFORM_YOUTUBE)
        foundIcon = GetDataFilePath("assets/platform/default/youtube.svg", absPath);
    else if (senderButton->GetStreamAccountPlatform() == PLATFORM_CUSTOM_RTMP)
        foundIcon = GetDataFilePath("assets/platform/default/customrtmp.svg", absPath);
    

    if (foundIcon)
    {
        //Image QSS higher qulality on png
        /*QSize testIconSize = ui->label_Platform->size();
        testIconSize.setWidth(testIconSize.width() - 2);
        testIconSize.setHeight(testIconSize.height() - 2);
        QPixmap scaled = QPixmap(absPath.c_str()).scaled(testIconSize,
                                                         Qt::KeepAspectRatio,
                                                         Qt::SmoothTransformation);
        ui->label_Platform->setPixmap(scaled);*/
        QString sts = QString("QLabel { image:url(%1); }").arg(absPath.c_str());
        ui->label_Platform->setStyleSheet(sts);
    }
    
    
    if (m_pCurrentAccountButton->GetStreamAccountPlatform() == PLATFORM_CUSTOM_RTMP)
    {
        ui->label_IdNumber->setText("");
    }

    bool onliveCurrAccount = m_pCurrentAccountButton->GetStateLive();
    //
    
    if (ui->pushButton_ToggleSimulcast->isChecked() != onliveCurrAccount)
    {
        ui->pushButton_ToggleSimulcast->setChecked(onliveCurrAccount);
        ui->pushButton_ToggleSimulcast->ChangeState(onliveCurrAccount);
    }

    if (senderButton->GetStreamAccountPlatform() == PLATFORM_YOUTUBE) {
        if (-1 == AUTH_CONTEXT.IsRegisterChannel(senderButton->GetUuid().toStdString().c_str())) {
            ui->pushButton_ChannelSetting->setEnabled(false);
        }
        else {
            ui->pushButton_ChannelSetting->setEnabled(true);
        }
    }
    else {
        ui->pushButton_ChannelSetting->setEnabled(true);
    }

    //If Main - Check Sub - No Sub Simulcast Button Hide(true), Yes Sub Simulcast Button Show
    //Not Main Simulcast Button Always Show

    ui->widget_SimulcastChannel->setVisible(true);
    ui->pushButton_Disconnect->setVisible(true);
    ui->pushButton_ChannelSetting->setVisible(true);
    ui->frame_Line1->setVisible(true);
    ui->frame_Line2->setVisible(true);

    if (m_broadInfoWidget)
        m_broadInfoWidget->hide();

    if (senderButton->GetStreamAccountPlatform() == PLATFORM_SOOP)
    {
        if (m_broadInfoWidget)
        {
            m_broadInfoWidget->show();
            m_broadInfoWidget->TabButtonClick(AFQSettingBroadInfoWidget::BroadInfoPage::BroadInfo_Page);
        }

        ui->pushButton_Disconnect->hide();
        ui->frame_Line2->hide();

        ui->frame_Line1->hide();
        ui->widget_SimulcastChannel->hide();

        AFChannelData* data = nullptr;
        AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, data);
        m_pMainAccountButton->SetOnLive(true);
        ui->pushButton_ToggleSimulcast->SetChecked(true);
        if (data)
            data->isStreaming = true;
    }
    else
    {
        ui->pushButton_Disconnect->setDisabled(AFOutputUtil::IsStreamActive());
        ui->pushButton_Disconnect->show();
        ui->frame_Line2->show();
    }
}

void AFQStreamSettingAreaWidget::qslotReleaseAccount()
{
    bool isStreaming = AFOutputUtil::IsStreamActive();
    if (isStreaming)
    {
        if (m_pCurrentAccountButton->GetOnLive())
        {
            int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
                this->parentWidget(), "",
                QTStr("Stream.Delete.OnStream.Refuse"));
        return;

        }
    }

    int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this->parentWidget(), "",
        QTStr("Basic.Settings.Stream.DeleteAccountAlert"));

    if (result == QDialog::Rejected)
        return;
    else
        _ReleaseAccount();
}

void AFQStreamSettingAreaWidget::qslotResetStreamSettingUi()
{
}

void AFQStreamSettingAreaWidget::qslotStreamDataChanged()
{
    if (!m_loading)
    {
        m_streamDataChanged = true;
        sender()->setProperty("changed", QVariant(true));
        //emit qsignalStreamDataChanged();
    }
}

void AFQStreamSettingAreaWidget::qslotChangeLabel(QString channelName)
{
    m_pCurrentAccountButton->SetChannelName(channelName);
}

void AFQStreamSettingAreaWidget::qslotProfilePictureClicked()
{
    if (!m_pCurrentAccountButton)
        return;

    QString channelName = m_pCurrentAccountButton->GetChannelName();
    QString nickName = m_pCurrentAccountButton->GetChannelNick();
    QString platform = m_pCurrentAccountButton->GetStreamAccountPlatform();
    QString dashboardUrl;

    if (channelName == "" || platform == "")
        return;

    if (platform == PLATFORM_SOOP)
        dashboardUrl = QString::fromStdString(SOOP_CHANNEL_URL) + channelName;
    else if (platform == PLATFORM_TWITCH)
        dashboardUrl = TWITCH_URL + channelName;
    else if (platform == PLATFORM_YOUTUBE)
        dashboardUrl = YOUTUBE_URL + nickName;
    else
        return;

    MAINFRAME->NavigateDefaultBrowser(dashboardUrl);
}

void AFQStreamSettingAreaWidget::qslotGetSoopStreamerInfo(const QByteArray& responseData)
{
    std::string jsonString = responseData.toStdString();
    std::string err;
    err.clear();

    QString msg = "";
    int code = 1;
}

void AFQStreamSettingAreaWidget::qslotRefreshProfilePicture(int code, QString message)
{
    UNUSED_PARAMETER(code);
    UNUSED_PARAMETER(message);

    AFChannelData* data = nullptr;
    QPixmap* savedPixmap = nullptr;

    AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, data);
    if (data)
    {
        savedPixmap = (QPixmap*)data->pObjQtPixmap;
        for (int index = 0; index < m_accountButtonList.count(); index++)
        {
            if (m_accountButtonList[index]->GetStreamAccountPlatform() == PLATFORM_SOOP)
            {
                m_accountButtonList[index]->SetPixmapProfileImgObj(savedPixmap);
                break;
            }
        }
    }

    if (m_pCurrentAccountButton)
    {
        if (m_pCurrentAccountButton->GetStreamAccountPlatform() == PLATFORM_SOOP)
        {
            savedPixmap = m_pCurrentAccountButton->GetPixmapProfileImgObj();
            if (savedPixmap != nullptr)
                ui->label_ProfilePicture->setPixmap(*savedPixmap);
        }
    }
}

void AFQStreamSettingAreaWidget::qslotClickMainAccount(bool checked)
{
    Q_UNUSED(checked);

    if (m_pMainAccountButton)
        m_pMainAccountButton->clicked();
}

void AFQStreamSettingAreaWidget::qslotScrollEnter()
{
    ui->scrollArea_AccountList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void AFQStreamSettingAreaWidget::qslotScrollLeave()
{
    ui->scrollArea_AccountList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void AFQStreamSettingAreaWidget::qslotAddAccountEnter()
{
    std::string absPath;
    if (ui->pushButton_AddPlatform->isEnabled()) {
        GetDataFilePath("assets/scene-source-block/source-control/bt_add_source_hover.svg", absPath);

        ui->pushButton_AddPlatform->setIconSize(QSize(24, 24));

        QIcon buttonIcon(absPath.c_str());
        ui->pushButton_AddPlatform->setIcon(buttonIcon);
    }
}

void AFQStreamSettingAreaWidget::qslotAddAccountLeave()
{
    std::string absPath;
    if (ui->pushButton_AddPlatform->isEnabled()) {
        GetDataFilePath("assets/scene-source-block/source-control/bt_add_source.svg", absPath);

        ui->pushButton_AddPlatform->setIconSize(QSize(24, 24));

        QIcon buttonIcon(absPath.c_str());
        ui->pushButton_AddPlatform->setIcon(buttonIcon);
    }
}

void AFQStreamSettingAreaWidget::StreamSettingAreaInit()
{    
    if (MAINFRAME->IsSmallResolution())
    {
        ui->scrollArea->setFixedWidth(796);
        ui->scrollAreaWidgetContents->setFixedWidth(796);
    }

    _SetStreamSettings();
    LoadStreamAccountSaved();
    _LoadStreamAccountSettings();
    ToggleOnStreaming(false);
}

void AFQStreamSettingAreaWidget::FindAccountButtonWithID(QString id, QString platform)
{
    bool noButton = true;
    for (int index = 0; index < m_accountButtonList.count(); index++)
    {
        if (id == "" && platform != PLATFORM_CUSTOM_RTMP)
        {
            if (m_accountButtonList[index]->GetStreamAccountPlatform() == platform)
            {
                m_accountButtonList[index]->clicked();
                int val = (index + 1) * 78;
                ui->scrollArea_AccountList->verticalScrollBar()->setValue(val);
                noButton = false;
                break;
            }
        }
        else
        {
            if (m_accountButtonList[index]->GetChannelName() == id && m_accountButtonList[index]->GetStreamAccountPlatform() == platform)
            {
                m_accountButtonList[index]->clicked();
                int val = (index + 1) * 78;
                ui->scrollArea_AccountList->verticalScrollBar()->setValue(val);
                noButton = false;
                break;
            }
        }
    }

    if (noButton) {
        m_pMainAccountButton->clicked();
        ui->scrollArea_AccountList->verticalScrollBar()->setValue(0);
    }
}

void AFQStreamSettingAreaWidget::SetBroadDataChangedVal(bool changed)
{
    if (!m_broadInfoWidget)
        return;

    m_broadInfoWidget->SetDataChanged(changed);
}

bool AFQStreamSettingAreaWidget::BroadDataChanged()
{
    if (!m_broadInfoWidget)
        return false;

    return m_broadInfoWidget->IsDataChanged();
}

void AFQStreamSettingAreaWidget::SaveStreamSettings()
{
    auto& authManager = AUTH_CONTEXT;
    if (AFOutputUtil::GetStreamingCheck())
    {
        if (m_pMainAccountButton && m_pMainAccountButton->GetModified())
        {
            AFChannelData* channeldata = nullptr;
            authManager.GetChannelData(m_pMainAccountButton->GetStreamAccountPlatform().toStdString(), channeldata);
            if(channeldata)
                channeldata->isStreaming = m_pMainAccountButton->GetOnLive();
        }

        int index = 0;
        for (index; index < m_accountButtonList.count(); index++)
        {
            if (m_accountButtonList[index]->GetOnLive())
                continue;

            if (m_accountButtonList[index]->GetModified())
            {
                AFChannelData* channeldata = nullptr;
                authManager.GetChannelData(index, channeldata);
                if(channeldata)
                    channeldata->isStreaming = m_accountButtonList[index]->GetOnLive();
            }
        }
        return;
    }

    //-----------------------------------------------------------------------
    authManager.ClearRegisterChannel();

    foreach(QPointer<AFQStreamAccount> button, m_releasedAccountButtonList)
    {
        MAIN_BLOCKMANAGER->DeletePlatformPage(button->GetStreamAccountPlatform().toStdString());
        button->deleteLater();
        button = nullptr;
    }

    m_releasedAccountButtonList.clear();

    if (m_pMainAccountButton)
    {
        AFChannelData* channeldata = nullptr;
        channeldata = new AFChannelData();
        QString uuidTemp = m_pMainAccountButton->GetUuid();
        std::string strUuid = uuidTemp.toStdString();
        channeldata->isStreaming = m_pMainAccountButton->GetOnLive();
        authManager.RegisterChannel(strUuid.c_str(), channeldata);
        m_pMainAccountButton->CompleteRegist();
        channeldata->pObjQtPixmap = (void*)m_pMainAccountButton->GetPixmapProfileImgObj();

        if (channeldata->pAuthData->platform == PLATFORM_SOOP)
            channeldata->pAuthData->cookie = m_pMainAccountButton->GetCookie();
    }

    int index = 0;
    for (index; index < m_accountButtonList.count(); index++)
    {
        AFChannelData* channeldata = nullptr;
        channeldata = new AFChannelData();

        QString uuidTemp = m_accountButtonList[index]->GetUuid();
        std::string strUuid = uuidTemp.toStdString();
        channeldata->isStreaming = m_accountButtonList[index]->GetOnLive();
        int chindex = authManager.GetCntChannel();
        authManager.RegisterChannel(strUuid.c_str(), channeldata);
        
        if (m_accountButtonList[index]->GetModified())
        {
            channeldata->pAuthData->channelID        = m_accountButtonList[index]->GetChannelName().
                                                                            toStdString().c_str();
            channeldata->pAuthData->urlRTMP          = m_accountButtonList[index]->GetServer().
                                                                            toStdString().c_str();
            channeldata->pAuthData->keyRTMP          = m_accountButtonList[index]->GetStreamKey().
                                                                            toStdString().c_str();
            channeldata->pAuthData->customID         = m_accountButtonList[index]->GetID().
                                                                            toStdString().c_str();
            channeldata->pAuthData->customPassword   = m_accountButtonList[index]->GetPassword().
                                                                            toStdString().c_str();
            channeldata->pAuthData->platform         = m_accountButtonList[index]->GetStreamAccountPlatform().
                                                                            toStdString().c_str();

        }

        MAIN_BLOCKMANAGER->CreatePlatformPage(channeldata->pAuthData->platform,
                                              m_accountButtonList[index]->GetModified());

        m_accountButtonList[index]->CompleteRegist();
        channeldata->pObjQtPixmap = (void*)m_accountButtonList[index]->GetPixmapProfileImgObj();

        authManager.SetSoopCookie(m_accountButtonList[index]->GetCookie());
    }
}


bool AFQStreamSettingAreaWidget::SaveBroadInfoSettings()
{
    bool retVal = true;
    AFQStreamAccount* StreamAccount = nullptr;
    if (_FindAccountButtonWithPlatform(PLATFORM_SOOP, StreamAccount))
    {
        if (m_broadInfoWidget) {
            if (m_broadInfoWidget->IsDataChanged()) {
                retVal = m_broadInfoWidget->SaveSettings();
            }
        }
    }

    return retVal;
}

void AFQStreamSettingAreaWidget::ToggleOnStreaming(bool streaming)
{
    bool isStreaming = AFOutputUtil::IsStreamActive();
    bool isRecording = AFOutputUtil::IsRecordingActive();
    //bool useVideo = obs_video_active() ? false : true;
    bool useVideo = (isStreaming || isRecording) ? false : true; // Check Except Replay Buffer

    ui->pushButton_AddPlatform->setEnabled(useVideo);

    std::string absPath;
    if (ui->pushButton_AddPlatform->isEnabled()) {
        GetDataFilePath("assets/scene-source-block/source-control/bt_add_source.svg", absPath);
        ui->pushButton_AddPlatform->setIconSize(QSize(24, 24));
    } else {
        GetDataFilePath("assets/setting-dialog/stream/bt_add-disabled.svg",
                        absPath);
        ui->pushButton_AddPlatform->setIconSize(QSize(12, 12));
    }
    QIcon buttonIcon(absPath.c_str());
    ui->pushButton_AddPlatform->setIcon(buttonIcon);
}

void AFQStreamSettingAreaWidget::LoadStreamAccountSaved()
{
    m_releasedAccountButtonList.clear();
    m_accountButtonList.clear();

    QList<AFQStreamAccount*> mainbuttons = ui->widget_Main->findChildren<AFQStreamAccount*>();
    foreach(AFQStreamAccount * button, mainbuttons)
    {
        ui->widget_Main->layout()->removeWidget(button);
        button->close();
        delete button;
    }

    m_pMainAccountButton = nullptr;

    QList<AFQStreamAccount*> buttons = ui->widget_OtherAccount->findChildren<AFQStreamAccount*>();
    foreach(AFQStreamAccount * button, buttons)
    {
        ui->widget_OtherAccount->layout()->removeWidget(button);
        button->close();
        delete button;
    }

    bool haveChannel = false;

    AFChannelData* checkMain = nullptr;
    if (AUTH_CONTEXT.GetMainChannelData(checkMain))
    {
        if (!checkMain)
            return;
    }

    int cntList = AUTH_CONTEXT.GetCntChannel();

    for (int idx = 0; idx < cntList; idx++)
    {
        AFChannelData* tmpChannelNode = nullptr;
        AFBasicAuth* tmpAuthNode = nullptr;
        AUTH_CONTEXT.GetChannelData(idx, tmpChannelNode);

        if (tmpChannelNode == nullptr)
            continue;

        tmpAuthNode = tmpChannelNode->pAuthData;

        s_firstAddAccountUI = false;

        AFQStreamAccount* newAccount = _CreateStreamAccount(tmpAuthNode->platform.c_str(),
                                                            tmpAuthNode->channelID.c_str(),
                                                            tmpAuthNode->channelNick.c_str(),
                                                            tmpChannelNode->isStreaming,
                                                            tmpAuthNode->urlRTMP.c_str(),
                                                            tmpAuthNode->keyRTMP.c_str(),
                                                            tmpAuthNode->customID.c_str(),
                                                            tmpAuthNode->customPassword.c_str(),
                                                            tmpAuthNode->uuid.c_str(),
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

    // Main Should Go On Top -- Need To be here For Click Main
    haveChannel |= LoadMainAccount();

    if (haveChannel)
        ui->stackedWidget->setCurrentIndex(0);
    else
        ui->stackedWidget->setCurrentIndex(1);

    RefreshAccountButtons();

    if (m_pMainAccountButton)
    {
        m_pMainAccountButton->clicked();
    }
    else
    {
        foreach(AFQStreamAccount * accountWidget, m_accountButtonList)
        {
            if (accountWidget) {
                accountWidget->clicked();
                break;
            }
        }
    }
}

bool AFQStreamSettingAreaWidget::LoadMainAccount()
{
    AFChannelData* mainChannelNode = nullptr;
    if (AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, mainChannelNode))
    {
        if (!mainChannelNode)
            return false;

        if (m_pMainAccountButton == nullptr)
        {
            AFBasicAuth* mainAuthNode = mainChannelNode->pAuthData;
            AFQStreamAccount* newAccount = _CreateStreamAccount(mainAuthNode->platform.c_str(),
                                                                mainAuthNode->channelID.c_str(),
                                                                mainAuthNode->channelNick.c_str(),
                                                                mainChannelNode->isStreaming,
                                                                mainAuthNode->urlRTMP.c_str(),
                                                                mainAuthNode->keyRTMP.c_str(),
                                                                mainAuthNode->customID.c_str(),
                                                                mainAuthNode->customPassword.c_str(),
                                                                mainAuthNode->uuid.c_str(),
                                                                true);
            newAccount->SetCookie(mainAuthNode->cookie);

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
        if (m_pMainAccountButton)
        {
            m_pMainAccountButton = nullptr;
            s_firstAddAccountUI &= true;
        }

        return false;
    }    
}

void AFQStreamSettingAreaWidget::RefreshAccountButtons()
{
    _ReorderAccountList();
    RemoveAllChildInLayout(ui->widget_OtherAccount->layout(), false);

    foreach(AFQStreamAccount * b, m_accountButtonList)
    {
        ui->widget_OtherAccount->layout()->addWidget(b);
    }
}

void AFQStreamSettingAreaWidget::ReloadBroadInfo()
{
    if (!m_broadInfoWidget)
        return;

    m_broadInfoWidget->LoadBroadInfoDatas();
    m_broadInfoWidget->SetDataChanged(false);
}

QMap<QString, AFQStreamAccount*> AFQStreamSettingAreaWidget::GetLiveChannels()
{
    QMap<QString, AFQStreamAccount*> retVal;
    if(m_pMainAccountButton->GetOnLive())
        retVal.insert(m_pMainAccountButton->GetStreamAccountPlatform(), m_pMainAccountButton);

    foreach(AFQStreamAccount* accountButton, m_accountButtonList)
    {
        if (accountButton->GetOnLive())
            retVal.insert(accountButton->GetStreamAccountPlatform(), accountButton);
    }
    return retVal;
}

void AFQStreamSettingAreaWidget::_SetStreamSettings()
{
    m_pCurrentAccountButton = nullptr;
    qslotScrollLeave();

    std::string absPath;

    connect(ui->pushButton_AddPlatform, &QPushButton::clicked,
        this, &AFQStreamSettingAreaWidget::qslotShowAddAccount);
    connect(ui->pushButton_AddPlatform, &AFQCustomPushbutton::qsignalButtonEnter,
        this, &AFQStreamSettingAreaWidget::qslotAddAccountEnter);
    connect(ui->pushButton_AddPlatform, &AFQCustomPushbutton::qsignalButtonLeave,
        this, &AFQStreamSettingAreaWidget::qslotAddAccountLeave);

    connect(ui->pushButton_ToggleSimulcast, &QPushButton::clicked, 
        this, &AFQStreamSettingAreaWidget::qslotSimulcastToggled);
    connect(ui->pushButton_ChannelSetting, &QPushButton::clicked,
        this, &AFQStreamSettingAreaWidget::qslotEditTriggred);
    connect(ui->pushButton_Disconnect, &QPushButton::clicked,
        this, &AFQStreamSettingAreaWidget::qslotReleaseAccount);

    ui->widget_AddSoop->setStatusTip(PLATFORM_SOOP);
    ui->widget_AddTwitch->setStatusTip(PLATFORM_TWITCH);
    ui->widget_AddYoutube->setStatusTip(PLATFORM_YOUTUBE);
    ui->widget_AddRTMP->setStatusTip(PLATFORM_CUSTOM_RTMP);

    connect(ui->widget_AddSoop, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotAuthTriggered);
    connect(ui->widget_AddTwitch, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotAuthTriggered);
    connect(ui->widget_AddYoutube, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotAuthTriggered);
    connect(ui->widget_AddRTMP, &AFQHoverWidget::qsignalMouseClick,
        this, &AFQStreamSettingAreaWidget::qslotAuthTriggered);
    
    AFSettingUtils::HookWidget(ui->pushButton_ToggleSimulcast, this, BUTTON_CLICKED, STREAM_CHANGED);

    connect(ui->widget_AccountPart, &AFQHoverWidget::qsignalHoverEnter, this, &AFQStreamSettingAreaWidget::qslotScrollEnter);
    connect(ui->widget_AccountPart, &AFQHoverWidget::qsignalHoverLeave, this, &AFQStreamSettingAreaWidget::qslotScrollLeave);


    _MakeBroadInfoSetting();
}

void AFQStreamSettingAreaWidget::_MakeBroadInfoSetting()
{
    if (!m_broadInfoWidget)
    {
        connect(ui->widget_ProfilePicture, &AFQHoverWidget::qsignalMouseClick,
            this, &AFQStreamSettingAreaWidget::qslotProfilePictureClicked);

        m_broadInfoWidget = new AFQSettingBroadInfoWidget(this);

        connect(m_broadInfoWidget, &AFQSettingBroadInfoWidget::qsignalDataChanged,
            this, &AFQStreamSettingAreaWidget::qsignalStreamDataChanged);

        ui->verticalLayout_BroadInfo->addWidget(m_broadInfoWidget);
    }
}

void AFQStreamSettingAreaWidget::_LoadStreamAccountSettings()
{
    if (m_accountButtonList.isEmpty())
        ui->stackedWidget->setCurrentIndex(1);
    else
        ui->stackedWidget->setCurrentIndex(0);
}

void AFQStreamSettingAreaWidget::_ReleaseAccount()
{
    auto& authManager = AUTH_CONTEXT;
    ui->widget_OtherAccount->layout()->removeWidget(m_pCurrentAccountButton);

    if (m_pCurrentAccountButton == m_pMainAccountButton)
    {
        authManager.SetSoopRegistered(false);
        authManager.RemoveMainChannel();
        MAIN_BLOCKMANAGER->DeletePlatformPage();
        m_pMainAccountButton->deleteLater();
        m_pMainAccountButton = nullptr;
        m_pCurrentAccountButton = nullptr;
    }
    else
    {
        int deleteindex = 0;
        for (deleteindex; deleteindex < m_accountButtonList.count(); deleteindex++)
        {
            if (m_accountButtonList[deleteindex] == m_pCurrentAccountButton)
            {
                QString platform = m_accountButtonList[deleteindex]->GetStreamAccountPlatform();
                if (platform == PLATFORM_YOUTUBE)
                    authManager.SetYoutubeRegistered(false);
                else if (platform == PLATFORM_TWITCH)
                    authManager.SetTwitchRegistered(false);
                std::string strUuid = m_accountButtonList[deleteindex]->GetUuid().toStdString();
                authManager.RemoveCachedAuth(strUuid.c_str());
                break;
            }
        }
        m_releasedAccountButtonList.append(m_accountButtonList[deleteindex]);
        m_accountButtonList[deleteindex]->close();

        m_accountButtonList.remove(deleteindex);
        m_pCurrentAccountButton = nullptr;
    }

    if (m_accountButtonList.count() == 0 && !m_pMainAccountButton)
    {
        s_firstAddAccountUI = true;
        ui->stackedWidget->setCurrentIndex(1);
    }
    else
    {
        if (m_pMainAccountButton)
        {
            m_pMainAccountButton->clicked();
            FindAccountButtonWithID(m_pMainAccountButton->GetID(), m_pMainAccountButton->GetStreamAccountPlatform());
        }
        else
        {
            m_accountButtonList[0]->click();
            FindAccountButtonWithID(m_accountButtonList[0]->GetID(), m_accountButtonList[0]->GetStreamAccountPlatform());
        }
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
    
    if (pData->platform == "")
        pData->platform = outRefAccount->GetStreamAccountPlatform().toStdString();
    
    
    QPixmap* resPixmap = MAINFRAME->MakePixmapFromAuthData(pData);
    
    if (resPixmap != nullptr)
    {
        if (outRefAccount->GetPixmapProfileImgObj() == nullptr)
            outRefAccount->SetPixmapProfileImgObj(resPixmap);
        ui->label_ProfilePicture->setPixmap(*resPixmap);
    }
    
    return resPixmap;
}

QPixmap* AFQStreamSettingAreaWidget::_CreateProfileImgObj(AFChannelData* data, AFQStreamAccount*& outRefAccount)
{
    if(data->imgUrlUserThumb.empty())
        return nullptr;

    QPixmap* resPixmap = MAINFRAME->DownloadPixmap(data->imgUrlUserThumb);
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
                                pAuthData->urlRTMP.c_str(), pAuthData->keyRTMP.c_str(),
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
    if (s_firstAddAccountUI)
    {
        s_firstAddAccountUI = false;
        onLive = true;
    }

    if (platformName == PLATFORM_SOOP)
        AUTH_CONTEXT.SetSoopRegistered(true);
    else if (platformName == PLATFORM_YOUTUBE)
        AUTH_CONTEXT.SetYoutubeRegistered(true);
    else if (platformName == PLATFORM_TWITCH)
        AUTH_CONTEXT.SetTwitchRegistered(true);
        
    AFQStreamAccount* account = new AFQStreamAccount(this);
    account->StreamAccountAreaInit(platformName, channelName, channelNickName, id, password, onLive, server, streamKey, uuid);
    connect(account, &QPushButton::clicked, this, &AFQStreamSettingAreaWidget::qslotAccountButtonTriggered);
    QVBoxLayout* layout = reinterpret_cast<QVBoxLayout*>(ui->widget_OtherAccount->layout());

    int index = 0;

    if (platformName == PLATFORM_SOOP)
    {
        m_pMainAccountButton = account;
        ui->widget_Main->layout()->addWidget(m_pMainAccountButton);
    }
    else
    {
        m_accountButtonList.append(account);
    }

    ui->stackedWidget->setCurrentIndex(0);

    if (bFromSavedFile == false)
        qslotStreamDataChanged();
    
    return account;
}

void AFQStreamSettingAreaWidget::_ModifyStreamAccount(QString server, QString streamkey, QString channelName, QString id, QString password)
{
    if (m_pCurrentAccountButton != nullptr)
    {
        m_pCurrentAccountButton->SetServer(server);
        m_pCurrentAccountButton->SetStreamKey(streamkey);
        m_pCurrentAccountButton->SetChannelName(channelName);
        m_pCurrentAccountButton->SetStreamAccountID(id);
        m_pCurrentAccountButton->SetPassword(password);
        m_pCurrentAccountButton->SetModified(true);

        ui->label_Nickname->setText(channelName);
        ui->label_Nickname->setToolTip(channelName);
        m_pCurrentAccountButton->repaint();

        qslotStreamDataChanged();
    }
}

bool AFQStreamSettingAreaWidget::_FindAccountButtonWithPlatform(QString platform, AFQStreamAccount*& outButton)
{
    if (m_pMainAccountButton->GetStreamAccountPlatform() == platform)
    {
        outButton = m_pMainAccountButton;
        return true;
    }

    for (int index = 0; index < m_accountButtonList.count(); index++)
    {
        if (m_accountButtonList[index]->GetStreamAccountPlatform() == platform)
        {
            outButton = m_accountButtonList[index];
            return true;
        }
    }
    return false;
}

void AFQStreamSettingAreaWidget::_ReorderAccountList()
{
    QStringList priority = { PLATFORM_SOOP, PLATFORM_TWITCH, PLATFORM_YOUTUBE };

    std::sort(m_accountButtonList.begin(), m_accountButtonList.end(), [&](AFQStreamAccount* a, AFQStreamAccount* b) {
        QString pa = a->GetStreamAccountPlatform();
        QString pb = b->GetStreamAccountPlatform();

        int indexA = priority.indexOf(pa);
        int indexB = priority.indexOf(pb);

        if (indexA == -1 && indexB == -1)
            return false;
        if (indexA == -1)
            return false;
        if (indexB == -1)
            return true;

        return indexA < indexB;
        });
}
