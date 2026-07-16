#include "CMainAccountButton.h"
#include "ui_main-account-button.h"

#include <QStyle>
#include <QTimer>
#include <QPainter>

#include <QGraphicsOpacityEffect>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "CoreModel/Auth/CAuthManager.h"

AFMainAccountButton::AFMainAccountButton(QWidget *parent) :
    QPushButton(parent),
    ui(new Ui::AFMainAccountButton)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_Hover, true);
    setAttribute(Qt::WA_StyledBackground, true);

    ui->pushButton_Frame->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->pushButton_Platform->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->pushButton_IsLive->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    connect(ui->pushButton_Platform, &QPushButton::clicked, this, &AFMainAccountButton::click);
    connect(ui->pushButton_IsLive, &QPushButton::clicked, this, &AFMainAccountButton::click);

    ui->pushButton_IsLive->setStyleSheet("border-radius:0; background:transparent");
}

AFMainAccountButton::~AFMainAccountButton()
{
    delete ui;
}

void AFMainAccountButton::qslotQuitStream()
{
    if (m_pChannelData)
    {
        m_pChannelData->isStreaming = false;
        SetStreaming(false);
    }
}

void AFMainAccountButton::qslotStartStream()
{
    if (m_pChannelData)
    {
        m_pChannelData->isStreaming = true;
        SetStreaming(false, true);
    }
}

void AFMainAccountButton::qslotHoverPlatformImage(bool hover)
{
    std::string platform = m_platformStr;
    platform.erase(std::remove(platform.begin(), platform.end(), ' '), platform.end());

    QString imgPath = "";
    if (hover)
    {
        std::string absPath;
        GetDataFilePath("assets", absPath);
        imgPath = QString("%1/platform/mousehover/%2.png")
            .arg(absPath.data()).arg(platform.data());

        ui->pushButton_Platform->setIcon(QIcon(imgPath));
    }
    else
    {
        TransparentPlatformImage(m_isTransparent);
    }
}

void AFMainAccountButton::qslotPressedPlatformImage(bool pressed)
{
    if (pressed)
    {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect();
        effect->setOpacity(0.2);
        setGraphicsEffect(effect);
    }
    else
    {
        setGraphicsEffect(nullptr);
    }
}

void AFMainAccountButton::SetChannelData(AFChannelData* data)
{
    m_pChannelData = data;
}

void AFMainAccountButton::SetStreaming(bool streaming, bool setLive, bool disable)
{
    m_isDisable = disable;
    ui->pushButton_Frame->setProperty("streaming", streaming);
    ui->pushButton_Frame->setProperty("live", setLive);
    PolishStyleSheet(ui->pushButton_Frame);

    setProperty("disable", disable);
    this->style()->unpolish(this);
    this->style()->polish(this);

    if (disable)
        return;

    std::string absEllipsePath;

    if (setLive)
    {
        if (streaming)
        {
            std::string absPath;
            GetDataFilePath("assets", absPath);

            QString imgPath = QString("%1/platform/live-streaming.svg")
                .arg(absPath.data());
            QIcon icon(imgPath);
            ui->pushButton_IsLive->setIconSize(ui->pushButton_IsLive->size());
            ui->pushButton_IsLive->setIcon(icon);
            m_currentState = ChannelState::Streaming;
        }
        else
        {
            ui->pushButton_IsLive->setIcon(QIcon());
            m_currentState = ChannelState::LoginWithSimulcast;
        }
    }
    else
    {
        ui->pushButton_IsLive->setIcon(QIcon());
        m_currentState = ChannelState::LoginWithSimulcast;
    }
}

void AFMainAccountButton::TransparentPlatformImage(bool transparent)
{
    std::string platform = m_platformStr;
    platform.erase(std::remove(platform.begin(), platform.end(), ' '), platform.end());
    m_isTransparent = transparent;

    std::string absPath;
    GetDataFilePath("assets", absPath);
    QString imgPath = QString("%1/platform/default/%2.png")
        .arg(absPath.data()).arg(platform.data());

    if (transparent)
    {
        QPixmap* transparentPixmap = SetTransparentImage(imgPath, 0.5);
        if (transparentPixmap)
        {
            QIcon icon(*transparentPixmap);
            ui->pushButton_Platform->setIconSize(ui->pushButton_Platform->size());
            ui->pushButton_Platform->setIcon(icon);
        }
        else
        {
            QIcon icon(imgPath);
            ui->pushButton_Platform->setIconSize(ui->pushButton_Platform->size());
            ui->pushButton_Platform->setIcon(icon);
        }
    }
    else
    {
        QIcon icon(imgPath);
        ui->pushButton_Platform->setIconSize(ui->pushButton_Platform->size());
        ui->pushButton_Platform->setIcon(icon);
    }


}

void AFMainAccountButton::SetPlatform(std::string platform, bool hover)
{
    m_platformStr = platform;

    if(hover)
        connect(this, &AFMainAccountButton::qsignalAccountButtonHover, this, &AFMainAccountButton::qslotHoverPlatformImage);
    //connect(this, &AFMainAccountButton::qsignalAccountButtonPressed, this, &AFMainAccountButton::qslotPressedPlatformImage);
}

void AFMainAccountButton::SetImage(QPixmap* image)
{
    QIcon icon(*image);
    ui->pushButton_Platform->setIconSize(ui->pushButton_Platform->size());
    ui->pushButton_Platform->setIcon(icon);
}

QSize AFMainAccountButton::SetFixedSize(QSize size)
{
    QSize insideButtonSize = size;
    setFixedSize(size);

    if (width() == 64)
    {
        insideButtonSize = size - QSize(2, 2);
        ui->pushButton_Frame->setFixedSize(insideButtonSize);
        ui->pushButton_Frame->move(1, 1);

        insideButtonSize = size - QSize(8, 8);
        ui->pushButton_Platform->setFixedSize(insideButtonSize);
        ui->pushButton_Platform->move(4,4);

        ui->pushButton_IsLive->resize(12, 12);
        ui->pushButton_IsLive->move(width() - ui->pushButton_IsLive->width() - 6, 6);
    }
    else if (width() == 48)
    {
        insideButtonSize = size - QSize(2, 2);
        ui->pushButton_Frame->setFixedSize(insideButtonSize);
        ui->pushButton_Frame->move(1, 1);

        insideButtonSize = size - QSize(8, 8);
        ui->pushButton_Platform->setFixedSize(insideButtonSize);
        ui->pushButton_Platform->move(4, 4);

        ui->pushButton_IsLive->move(width() - ui->pushButton_IsLive->width() - 3, 3);
    }
    else
    {
        insideButtonSize = size - QSize(10, 10);
        ui->pushButton_Frame->setFixedSize(insideButtonSize);
        ui->pushButton_Frame->move(5, 5);

        insideButtonSize = size - QSize(16, 16);
        ui->pushButton_Platform->setFixedSize(insideButtonSize);
        ui->pushButton_Platform->move(8, 8);

        ui->pushButton_IsLive->resize(10, 10);
        ui->pushButton_IsLive->move(30, 6);
    }

    return insideButtonSize;
}

void AFMainAccountButton::SetChecked(bool checked)
{
    setChecked(checked);
    ui->pushButton_Platform->setChecked(checked);
}

void AFMainAccountButton::checkChecked()
{
    qDebug() << isChecked();
    qDebug() << ui->pushButton_Platform->isChecked();
}

bool AFMainAccountButton::IsLive()
{
    return ui->pushButton_Frame->property("live").toBool();
}

bool AFMainAccountButton::event(QEvent* event)
{
    switch (event->type())
    {
    case QEvent::MouseMove:
        emit qsignalAccountButtonMouseMove();
        break;
    case QEvent::HoverEnter:
        emit qsignalAccountButtonHover(true);
        break;
    case QEvent::HoverLeave:
        emit qsignalAccountButtonHover(false);
        break;
    case QEvent::MouseButtonPress:
        emit qsignalAccountButtonPressed(true);
        break;
    case QEvent::MouseButtonRelease:
        emit qsignalAccountButtonPressed(false);
        break;
    };
    return QPushButton::event(event);
}
