#include "CStreamAccount.h"
#include "ui_stream-account-widget.h"

#include <QPixmap>
#include <QMetaEnum>

#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "platform/platform.hpp"

AFQStreamAccount::AFQStreamAccount(QWidget *parent) :
    QPushButton(parent),
    ui(new Ui::AFQStreamAccount)
{
    ui->setupUi(this);
}

AFQStreamAccount::~AFQStreamAccount()
{
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
    m_server = server;
    m_streamKey = streamKey;
    m_password = password;
    SetOnLive(onLive);
    SetUuid(uuid);
    
    ui->pushButton_Platform->SetFixedSize(ui->pushButton_Platform->size());
    ui->pushButton_Platform->SetPlatform(platform.toStdString());
    ui->pushButton_Platform->TransparentPlatformImage(false);
    ui->pushButton_Platform->setCheckable(false);
}

void AFQStreamAccount::SetStreamAccountPlatform(QString platform)
{
    m_platform = platform;
    
    ui->label_PlatformName->setText(platform);
    //ui->pushButton_Platform->setStyleSheet("");
    //ui->pushButton_Platform->setStyleSheet("background: transparent");
}

void AFQStreamAccount::SetStreamAccountID(QString id)
{
    m_iD = id;

    if (m_platform != PLATFORM_CUSTOM_RTMP)
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
    m_channelName = channelName;
    ui->label_AccountID->setText(channelName);
    ui->label_AccountID->update();
    ui->label_AccountID->setToolTip(channelName);
}

void AFQStreamAccount::SetOnLive(bool onlive)
{
    m_isLive = onlive;
    ui->pushButton_Platform->SetStreaming(AFOutputUtil::IsStreamActive(), m_isLive);
}

bool AFQStreamAccount::GetOnLive()
{
    return ui->pushButton_Platform->IsLive();
}
