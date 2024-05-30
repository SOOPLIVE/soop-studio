#include "CMainAccountButton.h"
#include "ui_main-account-button.h"

#include "platform/platform.hpp"

AFMainAccountButton::AFMainAccountButton(QWidget *parent) :
    QPushButton(parent),
    ui(new Ui::AFMainAccountButton)
{
    ui->setupUi(this);
    connect(ui->pushButton_Platform, &QPushButton::clicked, this, &QPushButton::clicked);
}

AFMainAccountButton::~AFMainAccountButton()
{
    delete ui;
}

void AFMainAccountButton::qslotQuitStream()
{
    m_dChannelData->bIsStreaming = false;
    SetStreaming(false);
}

void AFMainAccountButton::qslotStartStream()
{
    m_dChannelData->bIsStreaming = true;
    SetStreaming(false, true);
}


void AFMainAccountButton::SetChannelData(AFChannelData* data)
{
    m_dChannelData = data;
}

void AFMainAccountButton::SetStreaming(bool streaming, bool setLive, bool disable)
{
    //Image QSS higher qulality on png
                /*QSize testIconSize;
                testIconSize.setWidth(24);
                testIconSize.setHeight(24);
                QPixmap scaled = QPixmap(absPath.c_str()).scaled(testIconSize,
                                                                 Qt::KeepAspectRatio,
                                                                 Qt::SmoothTransformation);*/
                                                                 /*testButton->setIconSize(testIconSize);
                                                                 testButton->setIcon(scaled);*/

                                                                 //QSS Stroke -> changed to image for higher quality
                                                                 //QString sts = QString("QLabel { image:url(%1); border-radius:13px; "\
                                                                 //    "background-color: rgba(255,255,255,0.6%); border:2px solid #24272D; }"\
                                                                 //    "QPushButton {border-radius:15px; background:qlineargradient(x1:0, y1:0.869, x2:1, y2:0.8346, stop:0 #00E0FF , stop:1 #D1FF01 );  }").arg(absPath.c_str());//.arg(absEllipsePath.c_str());

    if (disable)
    {
        QString sts = QString("QPushButton {border-radius:15px; background:transparent; }");
        setStyleSheet(sts);
        return;
    }

    std::string absEllipsePath;

    if (setLive)
    {
        if (streaming)
        {
            GetDataFilePath("assets/platform/broad-ellipse.png", absEllipsePath);
            QString sts = QString(
                "QPushButton {border-radius:15px; background:transparent; image:url(%2);}"\
                "QPushButton:hover{background:#009DA7}; }")
                .arg(absEllipsePath.c_str());
            setStyleSheet(sts);

            m_eCurrentState = ChannelState::Streaming;
        }
        else
        {
            GetDataFilePath("assets/platform/standby-ellipse.png", absEllipsePath);
            QString sts = QString(
                "QPushButton {border-radius:15px; background:transparent; image:url(%2);}"\
                "QPushButton:hover{background:#484848}; }")
                .arg(absEllipsePath.c_str());
            setStyleSheet(sts);

            m_eCurrentState = ChannelState::LoginWithSimulcast;

        }
    }
    else
    {
        QString sts = QString(
            "QPushButton {border-radius:15px; background:transparent;}"\
            "QPushButton:hover{background:#484848}; }");
        setStyleSheet(sts);

        m_eCurrentState = ChannelState::LoginWithoutSimulcast;
    }
}


void AFMainAccountButton::SetPlatformImage(std::string platform, bool disable)
{
    std::string absPath;
    std::string abshoverPath;
    bool foundIcon = false;
    if (platform == "SOOP Global")
    {
        m_bIsMainAccount = true;
        if (disable)
        {
            GetDataFilePath("assets/platform/disable/soopglobal.png", absPath);
            GetDataFilePath("assets/platform/disable/soopglobalhover.png", abshoverPath);
        }
        else
        {
            foundIcon = GetDataFilePath("assets/platform/default/soopglobal.png", absPath);
        }
    }
    else if (platform == "afreecaTV")
        foundIcon = GetDataFilePath("assets/platform/default/soop.png", absPath);
    else if (platform == "Twitch")
        foundIcon = GetDataFilePath("assets/platform/default/twitch.png", absPath);
    else if (platform == "Youtube")
        foundIcon = GetDataFilePath("assets/platform/default/youtube.png", absPath);
    else if (platform == "Custom RTMP")
        foundIcon = GetDataFilePath("assets/platform/default/rtmp.png", absPath);

    QString sts = "";
    if (disable)
    {
        sts = QString("#pushButton_Platform { image:url(%1); border-radius:14px; "\
            "background: transparent; }"\
            "#pushButton_Platform:hover{ image:url(%2); }")
            .arg(absPath.c_str()).arg(abshoverPath.c_str());
        m_eCurrentState = ChannelState::Disable;
    }
    else
    {
        sts = QString("#pushButton_Platform { image:url(%1); border-radius:14px; "\
            "background: transparent; }")
            .arg(absPath.c_str());
        m_eCurrentState = ChannelState::LoginWithoutSimulcast;
    }
    
    ui->pushButton_Platform->setStyleSheet(sts);
}
