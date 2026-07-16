#include "CMainFrame.h"

#include <QPainterPath>

#include "qt-wrappers.hpp"

#include "CLeftNavigationBar.h"
#include "Common/CURLMiscUtils.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/OBSOutput/COutput.h"

#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"

#include "PopupWindows/StreamChannelConfig/YouTube/CYouTubeSettingDialog.hpp"
#include "PopupWindows/SettingPopup/CAddStreamWidget.h"

#include "UIComponent/CMessageAlert.h"

#include "MainFrame/Output/COutput.h"

#include "Utils/OverlayManager.h"

#define BROAD_INFO_REFRESH_TIME 1000 * 60
#define WEB_ACCOUNT_SELECTED 10
#define LOCAL_ACCOUNT_SELECTED 11
#define CANCEL_ACCOUNT_SELECT 12

void AFMainFrame::qslotToggleMainAccount()
{
    LogoutMainAccount();
}

void AFMainFrame::qslotRecieveBroadInfo()
{
    m_blockManager->ApplyBroadInfoToUI();
}

void AFMainFrame::qslotToggleLogout(bool useVideo)
{
    AFChannelData* channelData;
    QString logoutText = "";
    if (AUTH_CONTEXT.GetMainChannelData(channelData))
    {
        QString channelID = QString::fromStdString(channelData->pAuthData->channelID);
        logoutText = QTStr("Logout.With.Account").arg(channelID);
    }

    QAction* menu = nullptr;
    QList<QAction*> actions = m_topMenu->actions();
    for (QAction* action : actions) {
        if(action->text() == logoutText)
            menu = action;
    }

    if(menu)
        menu->setDisabled(useVideo);
}

void AFMainFrame::qslotRefreshSoopCookie()
{
    AUTH_CONTEXT.RefreshCookie();
}

//Only check autologin / Check saved Id  before set url
bool AFMainFrame::CheckLoginSoopCookie(std::string userID, std::string cookie, bool& loginRetain)
{
    bool retVal = true;
    bool needLoginRenewal = true;

    auto& authManager = AUTH_CONTEXT;
    std::string remainingID = "";

    AFChannelData* data = nullptr;
    if (authManager.GetChannelData(PLATFORM_SOOP, data))
    {
        remainingID = data->pAuthData->channelID;
        if (cookie != "") //웹 실행
        {
            if (!userID.empty() && userID != "undefined")
            {
                if (remainingID == userID)
                {
                    data->pAuthData->cookie = cookie;
                    data->isStreaming = true;
                    needLoginRenewal = false;

                    authManager.RefreshCookie();
                }
                else
                {
                    bool restoreAccount = true;

                    loginRetain = data->pAuthData->loginRetain;
                    if (loginRetain)
                    {
                        int result = SelectSoopAccount(userID, data->pAuthData->channelID);

                        if (result == LOCAL_ACCOUNT_SELECTED)
                        {
                            if (authManager.RefreshCookie())
                            {
                                needLoginRenewal = false;
                                restoreAccount = false;
                            }
                        }
                        else if (result == CANCEL_ACCOUNT_SELECT)
                            return false;
                    }

                    if (restoreAccount)
                    {
                        RestoreSoopAccount(userID, cookie, loginRetain, data->pAuthData->saveId);
                        needLoginRenewal = false;
                        authManager.RefreshCookie();
                    }
                }
            }
            else
            {
                needLoginRenewal = true;
            }
            
        }
        else
        {
            if (authManager.GetChannelData(PLATFORM_SOOP, data))
            {
                loginRetain = data->pAuthData->loginRetain;

                if (loginRetain)
                {
                    if (authManager.RefreshCookie())
                        needLoginRenewal = false;
                }
            }
        }
    }
    else
    {
        if (cookie != "")
        {
            RestoreSoopAccount(userID, cookie);
            needLoginRenewal = false;
        }
    }

    if (needLoginRenewal)
    {
#ifdef _WIN32 
        UpdaterKill();
#endif
        if (data)
            data->pAuthData->cookie = "";
        retVal = LoginSoopForCookie(remainingID);
    }
    return retVal;
}


void AFMainFrame::RestoreSoopAccount(std::string userId, std::string cookie, bool loginRetain, bool saveID)
{
    auto& authManager = AUTH_CONTEXT;
    authManager.RemoveAllChannel();

    AFBasicAuth rawAuthData;
    rawAuthData.channelID = userId;
    rawAuthData.cookie = cookie;
    rawAuthData.uuid = QUuid::createUuid().toString().toStdString();
    rawAuthData.platform = PLATFORM_SOOP;
    rawAuthData.loginRetain = loginRetain;
    rawAuthData.saveId = saveID;
    const char* uuid = rawAuthData.uuid.c_str();
    AFChannelData* newChannel = new AFChannelData();
    newChannel->isStreaming = true;

    authManager.CacheAuth(true, rawAuthData);
    authManager.RegisterChannel(rawAuthData.uuid.c_str(), newChannel);

    authManager.SetSoopCookie(cookie);
}

bool AFMainFrame::LoginSoopForCookie(std::string remainID)
{
    bool retVal = false;

    AFAddStreamWidget* addStream = new AFAddStreamWidget(nullptr);    
    addStream->AddStreamWidgetInit(PLATFORM_SOOP);    
 
    addStream->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    addStream->show();
    addStream->raise();
    addStream->activateWindow();

    addStream->setWindowFlag(Qt::WindowStaysOnTopHint, false);
    addStream->show();

    m_blockManager->ApplyMoveInAllArea(addStream);

    if (addStream->exec() == QDialog::Accepted)
    {
        AFBasicAuth& resAuth = addStream->GetRawAuth();
        AFChannelData* newChannel = new AFChannelData();
        std::string strUuid = QUuid::createUuid().toString().toStdString();
        const char* uuid = strUuid.c_str();
        resAuth.uuid = uuid;
        newChannel->isStreaming = true;

        auto& authManager = AUTH_CONTEXT;

        std::string platform = resAuth.platform;

        if (remainID != resAuth.channelID)
        {
            //Profile Reset If needed
            //LOADSAVE_CONTEXT.MoveProfileToBackup(remainID);
            //LOADSAVE_CONTEXT.MoveSceneCollectionToBackup(remainID);
            authManager.RemoveAllChannel();
        }

        retVal = authManager.CacheAuth(true, resAuth);
        authManager.RegisterChannel(uuid, newChannel);

        authManager.LoadSoopStreamerInfo();

        authManager.SaveAllAuthed();

        authManager.SetSoopCookie(addStream->GetCookie());
    }
    else
        retVal = false;

    return retVal;
}

int AFMainFrame::SelectSoopAccount(std::string newAccount, std::string currentAccount)
{
    AFTTopBaseDialog* checkAccount = new AFTTopBaseDialog(nullptr);
    checkAccount->setWindowTitle(APPNAME);
    checkAccount->SetWidthResizeEnabled(false);
    checkAccount->SetHeightResizeEnabled(false);

    checkAccount->setFixedSize(328, 287);

    QLabel* infolabel = new QLabel(checkAccount);
    infolabel->setAlignment(Qt::AlignCenter);
    infolabel->setText(QTStr("Login.Select.Account"));
    infolabel->setProperty("labelType", "loginTitle");
    PolishStyleSheet(infolabel);

    QLabel* selectlabel = new QLabel(checkAccount);
    selectlabel->setAlignment(Qt::AlignCenter);
    selectlabel->setText(QTStr("Login.Select.Account.Info"));
    selectlabel->setProperty("labelType", "loginInfo");
    PolishStyleSheet(selectlabel);

    QPushButton* savedButton = new QPushButton(checkAccount);
    savedButton->setFixedSize(272, 36);
    savedButton->setProperty("pushButtonTheme", "type2");
    PolishStyleSheet(savedButton);
    savedButton->setText(QTStr("Login.Studio.Account").arg(QString::fromStdString(currentAccount)));
    connect(savedButton, &QPushButton::clicked, checkAccount, [checkAccount]() { checkAccount->done(LOCAL_ACCOUNT_SELECTED); });

    QPushButton* webButton = new QPushButton(checkAccount);
    webButton->setFixedSize(272, 36);
    webButton->setText(QTStr("Login.Web.Account").arg(QString::fromStdString(newAccount)));
    connect(webButton, &QPushButton::clicked, checkAccount, [checkAccount]() { checkAccount->done(WEB_ACCOUNT_SELECTED); });

    QPushButton* cancelButton = new QPushButton(checkAccount);
    cancelButton->setFixedSize(272, 36);
    cancelButton->setProperty("pushButtonTheme", "type4");
    PolishStyleSheet(savedButton);
    cancelButton->setText(QTStr("Cancel"));
    connect(cancelButton, &QPushButton::clicked, checkAccount, [checkAccount]() { checkAccount->done(CANCEL_ACCOUNT_SELECT); });

    QVBoxLayout* layout = new QVBoxLayout();
    layout->setContentsMargins(28, 28, 28, 28);
    layout->setSpacing(0);

    layout->addWidget(infolabel);
    layout->addSpacerItem(new QSpacerItem(10, 8, QSizePolicy::Fixed, QSizePolicy::Fixed));
    layout->addWidget(selectlabel);
    layout->addSpacerItem(new QSpacerItem(10, 16, QSizePolicy::Fixed, QSizePolicy::Fixed));
    layout->addWidget(savedButton);
    layout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
    layout->addWidget(webButton);
    layout->addSpacerItem(new QSpacerItem(10, 20, QSizePolicy::Fixed, QSizePolicy::Fixed));
    layout->addWidget(cancelButton);

    checkAccount->setLayout(layout);

    AFQBlockManager::ApplyMoveInAllArea(checkAccount);

    checkAccount->exec();

    return checkAccount->result();
}

bool AFMainFrame::ShowPopupPageYoutubeChannel(AFBasicAuth* pAFDataAuted)
{
    if (pAFDataAuted == nullptr)
        return false;
    
    
    AFAuth::Def rawAuthDef = { "YouTube - RTMP",
                                AFAuth::Type::OAuth_LinkedAccount,
                                true, true };
    
    std::shared_ptr<YoutubeApiWrappers> tmpApiYoutube = std::make_shared<YoutubeApiWrappers>(rawAuthDef);
    AFOAuth* rawAuth = reinterpret_cast<AFOAuth*>(tmpApiYoutube.get());
    if (rawAuth->ConnectAuthedAFBase(pAFDataAuted) == false)
        return false;
    
    
    AFQYouTubeSettingDialog modal(this, rawAuth, false);
    connect(&modal, &AFQYouTubeSettingDialog::qsignalMakeChat, m_blockManager, &AFQBlockManager::qslotCreateYoutubePage);

    bool retVal = modal.exec() == QDialog::Accepted;
    if (retVal)
        m_broadcastReady = true; // Leave it true to search fast

    return retVal;
}

QPixmap* AFMainFrame::MakePixmapFromChannel(AFChannelData* channelData)
{
    if (channelData->pAuthData->platform == "")
        return nullptr;

    std::string urlIMG = AUTH_CONTEXT.CreateUrlProfileImg(channelData);

    return DownloadPixmap(urlIMG);
}

QPixmap* AFMainFrame::MakePixmapFromAuthData(AFBasicAuth* pAFDataAuted)
{
    if (pAFDataAuted->platform == "")
        return nullptr;
    
    AFChannelData tmpForMakeUrl;
    tmpForMakeUrl.pAuthData = pAFDataAuted;
    
    std::string urlIMG = AUTH_CONTEXT.CreateUrlProfileImg(&tmpForMakeUrl);
    
    return DownloadPixmap(urlIMG);
}

QPixmap* AFMainFrame::DownloadPixmap(std::string urlIMG)
{
    std::string readBufferImgBuffer;

    if (urlIMG == "")
        return nullptr;

    bool resCURL = CURLWriteDataOutStringBuffer(urlIMG.c_str(), readBufferImgBuffer);

    if (resCURL)
    {
        if (readBufferImgBuffer.find("404 Not Found") == std::string::npos)
        {
            QImage orgimage;
            orgimage.loadFromData((const unsigned char*)readBufferImgBuffer.c_str(),
                readBufferImgBuffer.size());


            int minDimension = std::fmin(orgimage.width(), orgimage.height());
            int margin = minDimension * 0.1;
            int diameter = minDimension - 2 * margin;

            QImage circularImage(diameter, diameter, QImage::Format_ARGB32);
            circularImage.fill(Qt::transparent);

            QPainter painter(&circularImage);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            QPainterPath path;
            path.addEllipse(0, 0, diameter, diameter);
            painter.setClipPath(path);

            int xOffset = (orgimage.width() - diameter) / 2;
            int yOffset = (orgimage.height() - diameter) / 2;
            painter.drawImage(-xOffset, -yOffset, orgimage);


            QPixmap pixmap = QPixmap::fromImage(circularImage);
            const uint scaledSize = 80;
            QPixmap* scaledPixmap = new QPixmap();
            *scaledPixmap = pixmap.scaled(scaledSize,
                scaledSize,
                Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation);

            return scaledPixmap;
        }
    }

    return nullptr;
}

bool AFMainFrame::AddStreamAccount(QWidget* parent, QString platformName)
{
    bool retVal = false;

    if (m_AddStreamWidget)
    {
        m_AddStreamWidget->raise();
        m_AddStreamWidget->move(((x() + width()) / 2) - (m_AddStreamWidget->width() / 2),
            ((y() + height()) / 2) - (m_AddStreamWidget->height() / 2));
        return false;
    }

    m_AddStreamWidget = new AFAddStreamWidget(parent);
    m_AddStreamWidget->AddStreamWidgetInit(platformName);

    m_AddStreamWidget->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    m_AddStreamWidget->show();
    m_AddStreamWidget->raise();
    m_AddStreamWidget->activateWindow();

    m_AddStreamWidget->setWindowFlag(Qt::WindowStaysOnTopHint, false);
    m_AddStreamWidget->show();

    m_blockManager->ApplyMoveInAllArea(m_AddStreamWidget);

    if (m_AddStreamWidget->exec() == QDialog::Accepted)
    {
        AFBasicAuth& resAuth = m_AddStreamWidget->GetRawAuth();
        AFChannelData* newChannel = new AFChannelData();
        std::string strUuid = QUuid::createUuid().toString().toStdString();
        const char* uuid = strUuid.c_str();
        resAuth.uuid = uuid;
        newChannel->isStreaming = false;
        newChannel->imgUrlUserThumb = m_AddStreamWidget->GetThumbnailPath();
       
        auto& authManager = AUTH_CONTEXT;
        //
        std::string platform = resAuth.platform;
        bool isMain = false;

        retVal = authManager.CacheAuth(isMain, resAuth);
        authManager.RegisterChannel(uuid, newChannel);

        authManager.SetSoopCookie(m_AddStreamWidget->GetCookie());

        QPixmap* newObj = nullptr;
        if (newChannel->imgUrlUserThumb.empty())
            newObj = MakePixmapFromAuthData(newChannel->pAuthData);
        else
            newObj = DownloadPixmap(newChannel->imgUrlUserThumb);

        if (newObj != nullptr) {
            newChannel->pObjQtPixmap = newObj;
        }

        authManager.SaveAllAuthed();

        m_blockManager->CreatePlatformPage(resAuth.platform, true);

        if (!isMain && platform == PLATFORM_SOOP) {
            authManager.RequestBroadInfoAPI();
        }
    }

    if (m_AddStreamWidget)
        m_AddStreamWidget->deleteLater();
    
    return retVal;
}

bool AFMainFrame::LoadAccounts()
{
    bool retVal = false;
    MAIN_OUTPUT->SetStreamingOutput();
    if(m_leftNavigationBar)
        retVal = m_leftNavigationBar->LoadNavigationAccounts();

    return retVal;
}

int AFMainFrame::CountSimulcast()
{
    int retVal = 0;
    auto& authManager = AUTH_CONTEXT;
    //
    AFChannelData* main = nullptr;
    if (authManager.GetMainChannelData(main))
        if (main && main->isStreaming)
            retVal++;

    int channelCount = authManager.GetCntChannel();
    for (int idx = 0; idx < channelCount; idx++)
    {
        AFChannelData* tmpChannel = nullptr;
        authManager.GetChannelData(idx, tmpChannel);
        if (tmpChannel && tmpChannel->isStreaming)
            retVal++;
    }

    return retVal;
}

bool AFMainFrame::FindChannelButtonWithPlatform(std::string platform, AFMainAccountButton*& outbutton)
{
    return m_leftNavigationBar->FindChannelButton(platform, outbutton);
}

void AFMainFrame::BroadInfoTimerStart()
{
    if (!m_receiveBroadInfoTimer)
    {
        m_receiveBroadInfoTimer = new QTimer(this);
        m_receiveBroadInfoTimer->setInterval(BROAD_INFO_REFRESH_TIME);
        connect(m_receiveBroadInfoTimer, &QTimer::timeout, this, &AFMainFrame::qslotRecieveBroadInfo);
    }

    m_receiveBroadInfoTimer->start();
}

void AFMainFrame::BroadInfoTimerStop()
{
    if (m_receiveBroadInfoTimer)
    {
        m_receiveBroadInfoTimer->stop();
        delete m_receiveBroadInfoTimer;
        m_receiveBroadInfoTimer = nullptr;
    }
}

void AFMainFrame::RefreshSoopCookiTimerStart()
{
    const int SOOP_COOKIE_REFRESH_INTERVAL_MS = 
        5 /*days*/ * 24 /*hours*/ * 60 /*min*/ * 60 /*sec*/ * 1000 /*ms*/;

    if (!m_refreshSoopCookieTimer)
    {
        m_refreshSoopCookieTimer = new QTimer(this);
        m_refreshSoopCookieTimer->setInterval(SOOP_COOKIE_REFRESH_INTERVAL_MS);
        connect(m_refreshSoopCookieTimer, &QTimer::timeout, this, &AFMainFrame::qslotRefreshSoopCookie);
    }

    m_refreshSoopCookieTimer->start();
}

void AFMainFrame::LogoutMainAccount(bool tokenExpired)
{
    if(tokenExpired)
    {
        if(AFOutputUtil::IsStreamActive())
            qslotStopStreaming();

        MAINFRAME->BroadStatusCheckTimerStop();
        MAINFRAME->ChangeStreamStateUI(true, false, "LIVE", 77);

        BroadInfoTimerStop();

        QString message = QTStr("Confirm.Token.Expired");

        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
            "", message, false, true);
        m_logOut = true;
        g_bRestart = true;
        close();
    }
    else
    {
        AFQMessagBoxAlert dlg(this, QTStr("ConfirmLogout.Text"), QTStr("ConfirmLogout.TextInfo"), QTStr("Logout"));
        if (QDialog::Accepted == dlg.exec())
        {
            m_logOut = true;
            g_bRestart = true;
            close();
        }
    }
}