#include "CStreamAccount.h"
#include "ui_stream-account-widget.h"

#include <QPixmap>
#include <QMetaEnum>

#include "platform/platform.hpp"

AFQStreamAccount::AFQStreamAccount(QWidget *parent) :
    QPushButton(parent),
    ui(new Ui::AFQStreamAccount)
{
    ui->setupUi(this);
}

AFQStreamAccount::~AFQStreamAccount()
{
    //if (m_pPixmapProfileImg != nullptr &&
    //    m_IsRegistedChannelModelData == false)
    //    delete m_pPixmapProfileImg;
    
    delete ui;
}

void AFQStreamAccount::StreamAccountAreaInit(QString platform, QString channelName, QString channelNickName,
    QString id, QString password, bool onLive, QString server, QString streamKey, QString uuid)
{
    SetStreamAccountInfo(platform, channelName, channelNickName, id, password, onLive, server, streamKey, uuid);
}

void AFQStreamAccount::SetStreamAccountInfo(QString platform, QString channelName, QString channelNickName,
    QString id, QString password, bool onLive, QString server, QString streamKey, QString uuid)
{
    SetStreamAccountPlatform(platform);
    SetStreamAccountID(id);
    SetChannelName(channelName);
    SetChannelNick(channelNickName);
    m_sServer = server;
    m_sStreamKey = streamKey;
    m_sPassword = password;
    SetOnLive(onLive);
    SetUuid(uuid);
}

void AFQStreamAccount::SetStreamAccountPlatform(QString platform)
{
    m_sPlatform = platform;
    ui->label_PlatformName->setText(platform);

    std::string absPath;

    bool foundIcon = false;
    if (platform == "SOOP Global")
        foundIcon = GetDataFilePath("assets/platform/default/soopglobal.png", absPath);
    else if (platform == "afreecaTV")
        foundIcon = GetDataFilePath("assets/platform/default/soop.png", absPath);
    else if (platform == "Twitch")
        foundIcon = GetDataFilePath("assets/platform/default/twitch.png", absPath);
    else if (platform == "Youtube")
        foundIcon = GetDataFilePath("assets/platform/default/youtube.png", absPath);
    else if (platform == "Custom RTMP")
        foundIcon = GetDataFilePath("assets/platform/default/rtmp.png", absPath);
    
    if (foundIcon)
    {
        //Image QSS higher qulality on png
        /*QSize testIconSize = ui->label_AccountPlatform->size();
        testIconSize.setWidth(testIconSize.width() + 10);
        testIconSize.setHeight(testIconSize.height() + 10);
        QPixmap scaled = QPixmap(absPath.c_str()).scaled(testIconSize,
                                                         Qt::KeepAspectRatio,
                                                         Qt::SmoothTransformation);
        ui->label_AccountPlatform->setPixmap(scaled);*/
        QString sts = QString("QLabel { image:url(%1); }").arg(absPath.c_str());
        ui->label_AccountPlatform->setStyleSheet(sts);
    }

    
    //ui->widget_Platform->setpi
}

void AFQStreamAccount::SetStreamAccountID(QString id)
{
    m_sID = id;
    if (m_sPlatform != "Custom Rtmp")
    {
        ui->label_AccountID->setText(id);
    }
}

QString AFQStreamAccount::GetStreamAccountPlatform()
{
    return ui->label_PlatformName->text();
}

void AFQStreamAccount::SetChannelName(QString channelName)
{
    m_sChannelName = channelName;
    ui->label_AccountID->setText(channelName);
    ui->label_AccountID->update();
    ui->label_AccountID->setToolTip(channelName);
}

void AFQStreamAccount::SetOnLive(bool onlive)
{
    m_IsLive = onlive;
    ui->label_Live->setVisible(onlive);
}

bool AFQStreamAccount::GetOnLive()
{
    return ui->label_Live->isVisible();
}