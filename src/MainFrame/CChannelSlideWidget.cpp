#include "CChannelSlideWidget.h"
#include "ui_channel-slide-widget.h"

#include <QWindow>
#include <QSvgWidget>
#include <QGraphicsOpacityEffect>
#include <QIcon>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"   //[copy-obs] copied

#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Source/CSource.h"

#include "UIComponent/CBasicToggleButton.h"

#include "MainFrame/CMustRaiseMainFrameEventF.h"
#include "MainFrame/CMainAccountButton.h"
#include "MainFrame/Output/COutput.h"

#include "ViewModel/Auth/Soop/auth-soop.hpp"
#include "ViewModel/Auth/Twitch/auth-twitch.h"
#include "ViewModel/Auth/YouTube/auth-youtube.hpp"

#define SERVICE_BUTTON_SIZE QSize(228, 40)

ChannelServiceButton::ChannelServiceButton(QWidget* parent) :
    QFrame(parent)
{
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(8,0,8,0);
    
    icon = new IconLabel(this);
    icon->setIconSize(24);

    buttonInfo = new QLabel(this);
    buttonInfo->setText("");

    icon->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    buttonInfo->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    favoriteButton = new QPushButton(this);
    favoriteButton->setCheckable(true);
    favoriteButton->setFixedSize(QSize(40, 40));
    favoriteButton->setIconSize(QSize(16,16));

    layout->addWidget(icon);
    layout->addWidget(buttonInfo);
    layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
    layout->addWidget(favoriteButton);

    setFixedSize(SERVICE_BUTTON_SIZE);

    setStyleSheet(R"(
        QFrame { background-color:transparent; border-radius: 5px; }
        QFrame:hover { background-color:#36383E; }
    )");

    buttonInfo->setStyleSheet(R"(QLabel { 
        color: #D5D7DC; 
        font-size : 15px; 
    })");

    bool useOpacity = true;
    if (useOpacity) {
        auto* eff = new QGraphicsOpacityEffect();
        eff->setOpacity(0.3);
        setGraphicsEffect(eff);
        eff->setEnabled(false);
    }

    connect(favoriteButton, &QPushButton::clicked,
        this, &ChannelServiceButton::FavoriteButtonClicked);
}

ChannelServiceButton::~ChannelServiceButton()
{

}

void ChannelServiceButton::SetMenuId(const QString& _menuId)
{
    menuId = _menuId;
    buttonInfo->setText(QTStr(menuId.toStdString().c_str()));
}

void ChannelServiceButton::SetMenuIcon(const QString& _iconDefaultPath, const QString& _iconActivePath)
{
    iconDefaultPath = _iconDefaultPath;
    iconActivePath = _iconActivePath;

    SetActive(active);
}

void ChannelServiceButton::SetDisabledMenuIcon(const QString& _iconDisabledPath)
{
    iconDisabledPath = _iconDisabledPath;
}

void ChannelServiceButton::SetFavoriteState(bool isFavorite_)
{
    isFavorite = isFavorite_;

    QSignalBlocker blocker(favoriteButton);
    favoriteButton->setChecked(isFavorite);

    // Change Icon
    std::string absPath;
    GetDataFilePath("assets", absPath);

    const QString iconPath = isFavorite
        ? QString("%1/mainview/leftsidebar/bt_lnb_favorite_on.svg").arg(absPath.c_str())
        : QString("%1/mainview/leftsidebar/bt_lnb_favorite_off.svg").arg(absPath.c_str());

    favoriteButton->setIcon(QIcon(iconPath));
}

void ChannelServiceButton::SetActive(bool _active)
{
	active = _active;

	QString labelStyleSheet;

	if (disabled) {
        if (!iconDisabledPath.isEmpty())
            icon->setIcon(QIcon(iconDisabledPath));

        labelStyleSheet = R"(QLabel { 
            color: #757B8A;  font-size : 15px; 
        })";
	}
	else
	{
		if (!active) {
			if (!iconDefaultPath.isEmpty())
				icon->setIcon(QIcon(iconDefaultPath));

			labelStyleSheet = R"(QLabel { 
            color: #D5D7DC;  font-size : 15px; 
        })";
		}
		else {
			if (!iconActivePath.isEmpty())
				icon->setIcon(QIcon(iconActivePath));

			labelStyleSheet = R"(QLabel { 
            color: #0182FF;  font-size : 15px; 
        })";
		}
	}

	buttonInfo->setStyleSheet(labelStyleSheet);
}

void ChannelServiceButton::SetDisabled(bool _disabled)
{
    disabled = _disabled;

    SetActive(active);
}

void ChannelServiceButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QFrame::mousePressEvent(event);
        return;
    }

    mousePressed = true;

    if (graphicsEffect())
        graphicsEffect()->setEnabled(true);

    event->accept();
}

void ChannelServiceButton::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QFrame::mouseReleaseEvent(event);
        return;
    }

    const bool clicked =
        mousePressed && rect().contains(event->pos());

    if (graphicsEffect())
        graphicsEffect()->setEnabled(false);

    mousePressed = false;

    if (clicked)
        emit ServiceButtonClicked(menuId);

    event->accept();
}

void ChannelServiceButton::leaveEvent(QEvent* event)
{
    if (graphicsEffect())
        graphicsEffect()->setEnabled(false);

    QFrame::leaveEvent(event);
}

void ChannelServiceButton::FavoriteButtonClicked(bool checked)
{
    SetFavoriteState(checked);

    MAINFRAME->SetFavoriteLnbMenu(menuId, checked);

    QMetaObject::invokeMethod(MAINFRAME, []() {
        MAINFRAME->RefreshFavoriteLnbMenus();
        }, Qt::QueuedConnection);
}

AFQChannelSlideWidget::AFQChannelSlideWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQChannelSlideWidget)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    connect(ui->pushButton_Setting, &QPushButton::clicked, MAINFRAME, &AFMainFrame::qslotShowStudioSettingWithButtonSender);
    connect(ui->pushButton_Setting, &QPushButton::clicked, this, &AFQChannelSlideWidget::close);
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQChannelSlideWidget::close);
    connect(ui->widget_Channel, &AFQHoverWidget::qsignalMouseClick, this, &AFQChannelSlideWidget::qslotSlideNicknameClicked);
    connect(ui->pushButton_ProfilePicture, &AFMainAccountButton::clicked, this, &AFQChannelSlideWidget::qslotSlideNicknameClicked);
    connect(ui->widget_Background, &AFQHoverWidget::qsignalMouseClick, this, &AFQChannelSlideWidget::close);
    connect(MAIN_BLOCKMANAGER, &AFQBlockManager::qsignalBlockVisible,
        this, &AFQChannelSlideWidget::qslotPopupVisible);

    connect(MAINFRAME, &AFMainFrame::qsignalTopMenuClicked, this, &AFQChannelSlideWidget::close);
    connect(ui->widget_Channel, &AFQHoverWidget::qsignalHoverEnter,
        ui->label_Nickname, &AFQElidedSlideLabel::qslotHoverLabel);
    connect(ui->widget_Channel, &AFQHoverWidget::qsignalHoverLeave,
        ui->label_Nickname, &AFQElidedSlideLabel::qslotLeaveButton);

    ui->scrollArea_PlatformButtons->setWidgetResizable(true);
    ui->pushButton_ProfilePicture->SetCurrentState(AFMainAccountButton::ChannelState::Disable);

    ui->pushButton_Setting->setProperty("buttonType", "transparentsettings");
    ui->pushButton_Close->setProperty("buttonType", "closeButton");
}

AFQChannelSlideWidget::~AFQChannelSlideWidget()
{
    delete ui;
}

void AFQChannelSlideWidget::qslotSlideStreamChanged()
{
    AFChannelData* data = m_pCurrentAccountButton->GetChannelData();
    ui->pushButton_ProfilePicture->SetStreaming(AFOutputUtil::IsStreamActive(),
        data->isStreaming);
}

void AFQChannelSlideWidget::qslotTransmissionToggle(bool checked)
{
    auto auth = AUTH_CONTEXT;
    //
    if (m_pCurrentAccountButton)
    {
        if (checked)
        {
            if (AFOutputUtil::IsStreamActive())
            {
                do
                {
                    AFChannelData* soopData = nullptr;
                    auth.GetChannelData(PLATFORM_SOOP, soopData);
                    if (soopData)
                    {
                        bool soopIsLive = soopData->isStreaming;
                        if (soopIsLive)
                        {
                            AFQBroadInfo* info = auth.GetSoopBroadInfo();
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

                m_pTransmissionButton->setChecked(false);
                m_pTransmissionButton->ChangeState(false);
                return;
            }
            else
            {
                if (m_pCurrentAccountButton->GetCurrentPlatform() == PLATFORM_SOOP)
                {
                    AFQBroadInfo* broadInfo = auth.GetSoopBroadInfo();
                    if (broadInfo->SubscribeBroad())
                    {
                        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                            "", QTStr("Simulcast.Reject.Subscribe.Broad"), false, true, "", -1, 166);
                        m_pTransmissionButton->setChecked(false);
                        m_pTransmissionButton->ChangeState(false);
                        return;
                    }
                }
                else
                {
                    AFChannelData* soopData = nullptr;
                    if (auth.GetChannelData(PLATFORM_SOOP, soopData))
                    {
                        if (soopData && soopData->isStreaming)
                        {
                            AFQBroadInfo* broadInfo = auth.GetSoopBroadInfo();
                            if (broadInfo->SubscribeBroad())
                            {
                                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                    "", QTStr("Simulcast.Reject.Subscribe.Broad"), false, true, "", -1, 166);

                                m_pTransmissionButton->setChecked(false);
                                m_pTransmissionButton->ChangeState(false);
                                return;

                            }
                        }
                    }
                }

                int liveCount = MAINFRAME->CountSimulcast();
                if (liveCount > 4)
                {
                    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                        "", QTStr("Simulcast.Max.Start"), false, true);

                    m_pTransmissionButton->setChecked(false);
                    m_pTransmissionButton->ChangeState(false);
                    return;
                }
                AFSourceUtil::CheckAddSoopAI("soop_aimanager_source", true);

                m_pCurrentAccountButton->qslotStartStream();
            }
                
        }
        else //Toggle OFF
        {

            AFChannelData* data = m_pCurrentAccountButton->GetChannelData();

            data->isStreaming = checked;
            m_pCurrentAccountButton->SetStreaming(false, data->isStreaming);
            ui->pushButton_ProfilePicture->SetStreaming(false, data->isStreaming);
        }
        MAIN_OUTPUT->SetStreamingOutput();
    }
}

void AFQChannelSlideWidget::qslotOpenDockWithPropertyInSlide()
{
	int popupType = sender()->property("popupType").toInt();
	ENUM_WINDOW_TYPE enumType = static_cast<ENUM_WINDOW_TYPE>(popupType);

	if (enumType == ENUM_WINDOW_TYPE::BroadInfo) {
		close();
	}

	QMetaObject::invokeMethod(qApp, [enumType]() {
		AFQBaseDockWidget* dock = nullptr;
		MAIN_BLOCKMANAGER->MakeDock(enumType, dock, true);
		}, Qt::QueuedConnection);
}

void AFQChannelSlideWidget::qslotOpenPopupWithPropertyInSlide()
{
    int popupType = sender()->property("popupType").toInt();

    AFQBorderPopupBaseWidget* popup = nullptr;
    bool success = MAIN_BLOCKMANAGER->MakePopup(popupType, popup);
}

void AFQChannelSlideWidget::qslotOpenChatInSlide()
{
    int type = _GetWindowType(SERVICE_CHAT);

    if (type != -1)
    {
        AFQBorderPopupBaseWidget* popup = nullptr;
        bool success = MAIN_BLOCKMANAGER->MakePopup(type, popup);

        QWidget* senderobject = reinterpret_cast<QWidget*>(sender());
        senderobject->setProperty("serviceOn", success);

        style()->unpolish(senderobject);
        style()->polish(senderobject);
    }
}

void AFQChannelSlideWidget::qslotSlideNicknameClicked()
{
    AFChannelData* data = m_pCurrentAccountButton->GetChannelData();
    std::string platform = data->pAuthData->platform;

    QString dashboard_url;

    if (platform == PLATFORM_SOOP)
    {
        dashboard_url = QString::fromStdString(SOOP_CHANNEL_URL) + QString::fromStdString(data->pAuthData->channelID);
        MAINFRAME->NavigateDefaultBrowser(dashboard_url);
    }
    else if (platform == PLATFORM_TWITCH)
    {
        dashboard_url = TWITCH_URL + QString::fromStdString(data->pAuthData->channelID);
        MAINFRAME->NavigateDefaultBrowser(dashboard_url);
    }
    else if (platform == PLATFORM_YOUTUBE)
    {
        dashboard_url = YOUTUBE_URL + QString::fromStdString(data->pAuthData->channelNick);
        MAINFRAME->NavigateDefaultBrowser(dashboard_url);
    }
}

void AFQChannelSlideWidget::qslotPopupVisible(bool visible, int type, bool enableFavoriteMenu)
{
    QString buttonType = "";
    switch (type)
    {
    case ENUM_WINDOW_TYPE::SoopChat:
        buttonType = SERVICE_CHAT;
        break;

    case ENUM_WINDOW_TYPE::TwitchChat:
    case ENUM_WINDOW_TYPE::YoutubeChat:
        buttonType = SERVICE_CHAT;
        break;

    case ENUM_WINDOW_TYPE::BroadInfo:
        buttonType = SERVICE_BROADINFO;
        break;
    case ENUM_WINDOW_TYPE::SoopOverlay:
        buttonType = SERVICE_OVERLAY;
        break;
    case ENUM_WINDOW_TYPE::SubTitle:
        buttonType = SERVICE_SUBTITLE;
        break;
    case ENUM_WINDOW_TYPE::Mission:
        buttonType = SERVICE_MISSION;
        break;
    case ENUM_WINDOW_TYPE::Vote:
        buttonType = SERVICE_VOTE;
        break;
    case ENUM_WINDOW_TYPE::AquaControl:
        buttonType = SERVICE_AQUA_CONTROL;
        break;
    case ENUM_WINDOW_TYPE::Extensions:
        buttonType = SERVICE_EXTENSIONS;
        break;
    case ENUM_WINDOW_TYPE::Breaktime:
        buttonType = SERVICE_BREAKTIME;
        break;
    }

    if (enableFavoriteMenu)
    {
        if (serviceButtons.contains(buttonType))
        {
            ChannelServiceButton* button = serviceButtons[buttonType];
            button->SetActive(visible);
        }
    }
    else
    {
        if (m_platformServiceButtons.contains(buttonType))
        {
            QPushButton* button = m_platformServiceButtons[buttonType];
            if (button)
            {
                button->setProperty("serviceOn", visible);
                style()->unpolish(button);
                style()->polish(button);
            }
        }
    }
}

void AFQChannelSlideWidget::qslotSetOpacity()
{
    QPushButton* senderWidget = qobject_cast<QPushButton*>(sender());
    if(senderWidget->graphicsEffect())
        senderWidget->graphicsEffect()->setEnabled(true);
}

void AFQChannelSlideWidget::qslotRemoveOpacity()
{
    QPushButton* senderWidget = qobject_cast<QPushButton*>(sender()); 
    if (senderWidget->graphicsEffect())
        senderWidget->graphicsEffect()->setEnabled(false);
}

void AFQChannelSlideWidget::ChangeSlideInfo(AFMainAccountButton* button)
{
    if (!button)
        return;

    m_pCurrentAccountButton = button;

    AFChannelData* data = m_pCurrentAccountButton->GetChannelData();
    if (!data)
        return;

    QString platformName = QString::fromStdString(data->pAuthData->platform);

    ui->label_Platform->setText(platformName);
    bool isCustom = platformName == PLATFORM_CUSTOM_RTMP ? true : false;

    ui->pushButton_Setting->setProperty("type", 1);
    ui->pushButton_Setting->setProperty("channelID", QString::fromStdString(data->pAuthData->channelID));
    ui->pushButton_Setting->setProperty("platformName", QString::fromStdString(data->pAuthData->platform));

    if (isCustom)
        ui->label_Nickname->setText(QString::fromStdString(data->pAuthData->channelID));
    else
        ui->label_Nickname->setText(QString::fromStdString(data->pAuthData->channelNick));

    ui->pushButton_ProfilePicture->SetStreaming(AFOutputUtil::IsStreamActive(), data->isStreaming);
    QSize picSize = ui->pushButton_ProfilePicture->SetFixedSize(QSize(64,64));
    QPixmap* savedPixmap = (QPixmap*)data->pObjQtPixmap;
    if (savedPixmap != nullptr)
    {
        QPixmap scaledPixmap = savedPixmap->scaled(picSize.width(), picSize.height(),
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation);
        ui->pushButton_ProfilePicture->SetImage(&scaledPixmap);
    }
    else
    {
        std::string absPath;
        bool foundIcon = GetDataFilePath("assets/setting-dialog/Program/profile.png", absPath);
        if (foundIcon)
        {
            QPixmap scaled = QPixmap(absPath.c_str()).scaled(picSize.width(), picSize.height(),
                Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation);
            ui->pushButton_ProfilePicture->SetImage(&scaled);
        }
    }

    _SetPlatformButtonArea();
    _SetTransmissionArea();
}

void AFQChannelSlideWidget::SetChannelSlideGeometry(QRect sliderect)
{
    ui->widget_ChannelSlide->setGeometry(sliderect);
}

QPushButton* AFQChannelSlideWidget::CreateServiceButton(const char* locale, const char* serviceType, int popupType, bool useOpacity)
{
	QPushButton* btn = new QPushButton(this);
	btn->setText(_AddSpaceToString(QTStr(locale)));
	btn->setFixedSize(SERVICE_BUTTON_SIZE);
	btn->setProperty("serviceType", serviceType);
	btn->setProperty("serviceOn", _CheckBlockIsVisible(serviceType));
    btn->setObjectName(QString("pushButton_%1").arg(serviceType));


	if (popupType != ENUM_WINDOW_TYPE::None)
		btn->setProperty("popupType", popupType);

	connect(btn, &QPushButton::pressed, this, &AFQChannelSlideWidget::qslotSetOpacity);
	connect(btn, &QPushButton::released, this, &AFQChannelSlideWidget::qslotRemoveOpacity);

	if (useOpacity) {
		auto* eff = new QGraphicsOpacityEffect();
		eff->setOpacity(0.3);
		btn->setGraphicsEffect(eff);
		eff->setEnabled(false);
	}

	ui->scrollAreaWidgetContents_PlatformButtonsArea->layout()->addWidget(btn);
	m_platformServiceButtons.insert(serviceType, btn);
	return btn;
}

void AFQChannelSlideWidget::ToggleServiceButton(QString type, bool available)
{
    if (serviceButtons.contains(type))
    {
        ChannelServiceButton* button = serviceButtons[type];
        if (!button)
            return;

        button->SetDisabled(!available);
    }
}

void AFQChannelSlideWidget::closeEvent(QCloseEvent* event)
{
    emit qsignalChannelSlideClosed();
}

void AFQChannelSlideWidget::_SetPlatformButtonArea()
{
	QVBoxLayout* broadLayout = reinterpret_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents_PlatformButtonsArea->layout());
	RemoveAllChildInLayout(broadLayout);

	m_platformServiceButtons.clear();
    serviceButtons.clear();
    m_uninstallFreecshotBtn = nullptr;

	bool foundPopup = false;

	AFChannelData* data = m_pCurrentAccountButton->GetChannelData();
	QString platform = QString::fromStdString(data->pAuthData->platform);
	if (platform != PLATFORM_CUSTOM_RTMP)
	{
		////////////////////DASHBOARD////////////////////
		if (platform != PLATFORM_SOOP)
		{
			QPushButton* dashboard = CreateServiceButton("ChannelSlide.Dashboard", SERVICE_DASHBOARD);
			dashboard->setProperty("platform", platform);
			dashboard->setProperty("channelID", QString::fromStdString(data->pAuthData->channelID));
            connect(dashboard, &QPushButton::clicked, MAINFRAME, &AFMainFrame::qslotShowGlobalPageSender);
            connect(dashboard, &QPushButton::clicked, this, &AFQChannelSlideWidget::close);
		}

		////////////////////BROADINFO////////////////////
		if (platform == PLATFORM_SOOP)
		{
            QPushButton* broadInfo = CreateServiceButton("BroadInfo", SERVICE_BROADINFO,ENUM_WINDOW_TYPE::BroadInfo);
            connect(broadInfo, &QPushButton::clicked, this, &AFQChannelSlideWidget::qslotOpenDockWithPropertyInSlide);
            //connect(broadInfo, &QPushButton::clicked, this, &AFQChannelSlideWidget::close);
		}

        if (platform == PLATFORM_SOOP)
        {
			AFQBroadInfo* info = AUTH_CONTEXT.GetSoopBroadInfo();
            bool allowSubTitle = (info && info->AllowSubTitle() ? true : false);

            auto& items = MAINFRAME->GetLnbMenuItems();
            for (auto& item : items) {

                if (!allowSubTitle && 0 == item.menuId.compare("SubTitle"))
                    continue;

                ChannelServiceButton* button = new ChannelServiceButton();
                button->SetMenuId(item.menuId);
                button->SetMenuIcon(item.iconDefaultPath, item.iconActivePath);
                button->SetActive(item.active);
                button->SetFavoriteState(item.isFavorite);

                // check disabled state ....
                if (0 == item.menuId.compare("SaveVodNow")) {
                    button->SetDisabledMenuIcon(item.iconDisabledPath);
                    button->SetDisabled(!MAINFRAME->CheckSplitVodAvailable());
                }

                if (0 == item.menuId.compare("Breaktime")) {
                    button->SetDisabledMenuIcon(item.iconDisabledPath);
                    button->SetDisabled(!AFOutputUtil::IsStreamActive());
                }

                // for log
                button->setObjectName(QString("pushButton_%1").arg(item.menuLog));

                ui->scrollAreaWidgetContents_PlatformButtonsArea->layout()->addWidget(button);


                connect(button, &ChannelServiceButton::ServiceButtonClicked,
                    this, [this, button](const QString& menuId) {
                        close();

                        QMetaObject::invokeMethod(qApp, [menuId]() {
                            // Destination Call : void AFMainFrame::ShowLnbMenuPopup(const QString& menuId, ENUM_WINDOW_TYPE type)
                            MAINFRAME->TriggerLnbMenu(menuId);
                            }, Qt::QueuedConnection);

                    });

                serviceButtons.insert(item.menuLog, button);
			}
        }
        else {
			QPushButton* chat = CreateServiceButton("Chat", SERVICE_CHAT);
			connect(chat, &QPushButton::clicked, this, &AFQChannelSlideWidget::qslotOpenChatInSlide);
			connect(chat, &QPushButton::clicked, this, &AFQChannelSlideWidget::close);
        }
	}
	else
		_NoServiceButtons();

	////////////////////SPACER////////////////////
	ui->scrollAreaWidgetContents_PlatformButtonsArea->layout()->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));

    bool exists = RegKeyExists(CurrentUserRegKey, "SOFTWARE\\soop\\Studio2");
    if (exists)
    {
        m_uninstallFreecshotBtn = new AFQFreecshotUnInstallButton(this);
        m_uninstallFreecshotBtn->setObjectName("pushbutton_uninstallFreecshot");

        ui->scrollAreaWidgetContents_PlatformButtonsArea->layout()->addWidget(m_uninstallFreecshotBtn);
    }
}

void AFQChannelSlideWidget::_SetTransmissionArea()
{
    QHBoxLayout* transmissionLayout = reinterpret_cast<QHBoxLayout*>(ui->widget_Transmission->layout());
    RemoveAllChildInLayout(transmissionLayout);

    transmissionLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));

    AFChannelData* data = m_pCurrentAccountButton->GetChannelData();

    if (m_pCurrentAccountButton->IsMainAccount())
    {
        /*auto& authManager = AUTH_CONTEXT;
        if (authManager.IsSoopGlobalRegistered() ^ authManager.IsSoopRegistered())
        {*/
            transmissionLayout->setSpacing(4);

            std::string absPath;
            bool foundIcon = GetDataFilePath("assets/platform/channel/default/lock.svg", absPath);
            QString path = QString::fromStdString(absPath);

            QSvgWidget* transmission = new QSvgWidget(QString::fromStdString(absPath), ui->widget_Transmission);
            transmissionLayout->addWidget(transmission);
            transmission->setFixedSize(20, 20);

            QLabel* fixed = new QLabel(ui->widget_Transmission);
            fixed->setFixedHeight(21);
            fixed->setStyleSheet("font-size: 12px;");
            fixed->setText(QTStr("Transmission.Essential"));
            transmissionLayout->addWidget(fixed);

            transmissionLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));

            return;
        //}
    }

    transmissionLayout->setSpacing(6);
    QLabel* transmissionLabel = new QLabel(QTStr("Transmission.Simultaneous"), ui->widget_Transmission);
    transmissionLayout->addWidget(transmissionLabel);

    m_pTransmissionButton = new AFQToggleButton(ui->widget_Transmission);
    m_pTransmissionButton->setFixedSize(QSize(24, 12));
    m_pTransmissionButton->setChecked(data->isStreaming);
    m_pTransmissionButton->ChangeState(data->isStreaming);
    connect(m_pTransmissionButton, &AFQToggleButton::clicked, this, &AFQChannelSlideWidget::qslotTransmissionToggle);
    transmissionLayout->addWidget(m_pTransmissionButton);

    transmissionLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding));
}

void AFQChannelSlideWidget::_NoServiceButtons()
{
    QLabel* blankAreaNotice = new QLabel(this);
    blankAreaNotice->setProperty("serviceType", "noservice");
    blankAreaNotice->setFixedHeight(40);
    blankAreaNotice->setText(QTStr("ChannelSlide.NoService")); //Locale
    blankAreaNotice->setAlignment(Qt::AlignCenter);
    ui->scrollAreaWidgetContents_PlatformButtonsArea->layout()->addWidget(blankAreaNotice);
}

bool AFQChannelSlideWidget::_CheckBlockIsVisible(QString type)
{
    bool retVal = false;

    int value = _GetWindowType(type);
    ENUM_WINDOW_TYPE windowType = ENUM_WINDOW_TYPE(value);

    AFQBorderPopupBaseWidget* outPopup = nullptr;
    retVal |= MAIN_BLOCKMANAGER->GetPopup(windowType, outPopup);
    AFQBaseDockWidget* outDock = nullptr;

    if(MAIN_BLOCKMANAGER->GetDock(windowType, outDock))
        retVal |= outDock->isVisible();

    return retVal;
}

int AFQChannelSlideWidget::_GetWindowType(QString serviceType)
{
    int retVal = -1;
    if (serviceType == SERVICE_BROADINFO)
        retVal = ENUM_WINDOW_TYPE::BroadInfo;
    else if (serviceType == SERVICE_MISSION)
        retVal = ENUM_WINDOW_TYPE::Mission;
    else if (serviceType == SERVICE_VOTE)
        retVal = ENUM_WINDOW_TYPE::Vote;
    else if (serviceType == SERVICE_EXTENSIONS)
        retVal = ENUM_WINDOW_TYPE::Extensions;
    else if (serviceType == SERVICE_AQUA_CONTROL)
        retVal = ENUM_WINDOW_TYPE::AquaControl;
    else if (serviceType == SERVICE_CHAT)
    {
        AFChannelData* data = m_pCurrentAccountButton->GetChannelData();
        QString platformName = QString::fromStdString(data->pAuthData->platform);

        if (platformName == PLATFORM_SOOP)
            retVal = ENUM_WINDOW_TYPE::SoopChat;
        else if (platformName == PLATFORM_TWITCH)
            retVal = ENUM_WINDOW_TYPE::TwitchChat;
        else if (platformName == PLATFORM_YOUTUBE)
            retVal = ENUM_WINDOW_TYPE::YoutubeChat;
    }
    else if (serviceType == SERVICE_OVERLAY)
        retVal = ENUM_WINDOW_TYPE::SoopOverlay;
    else if (serviceType == SERVICE_BREAKTIME)
        retVal = ENUM_WINDOW_TYPE::Breaktime;
    else if (serviceType == SERVICE_SUBTITLE)
        retVal = ENUM_WINDOW_TYPE::SubTitle;
    return retVal;
}

QString AFQChannelSlideWidget::_AddSpaceToString(QString text)
{
    return QString(" %1").arg(text);
}

AFQFreecshotUnInstallButton::AFQFreecshotUnInstallButton(QWidget* parent)
    : QPushButton(parent)
{
    std::string absPath;
    GetDataFilePath("assets", absPath);
    QString normalIconPath = QString("%1/platform/freecshot_uninstall/freecshot_icon_normal.svg").arg(absPath.data());
    QString pressedIconPath = QString("%1/platform/freecshot_uninstall/freecshot_icon_pressed.svg").arg(absPath.data());
    QString buttonGifPath = QString("%1/platform/freecshot_uninstall/background.gif").arg(absPath.data());

    m_normalIcon = QPixmap(normalIconPath).scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_pressedIcon = QPixmap(pressedIconPath).scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    setStyleSheet(
        "QPushButton { background-color:#2E3136; }"
        "QPushButton:hover { background-color:#36383E; }"
        "QPushButton:pressed { background-color:#2D3035; }"
		"QLabel { font-size:15px; color:#D5D7DC; }"
		"QLabel[textPressed=\"true\"] { color:#5F6267; }"
        );

    setFixedSize(QSize(228, 40));

    m_movieGIF = new QMovie(buttonGifPath, QByteArray(), this);
    m_movieGIF->setScaledSize(size());

    m_labelGIF = new QLabel(this);
    m_labelGIF->setGeometry(rect());
    m_labelGIF->setScaledContents(true);
    m_labelGIF->setMovie(m_movieGIF);
    m_labelGIF->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_movieGIF->start();

    m_labelIcon = new QLabel(this);
    m_labelIcon->setGeometry(45, 10, 20, 20);
    m_labelIcon->setPixmap(m_normalIcon);
    m_labelIcon->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    m_labelText = new QLabel(this);
    m_labelText->setGeometry(75, -3, width() - 75, height() + 3);
    m_labelText->setText(QTStr("Freecshot.UnInstall.Msg"));
    m_labelText->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    QApplication::processEvents();
}

AFQFreecshotUnInstallButton::~AFQFreecshotUnInstallButton()
{

}

void AFQFreecshotUnInstallButton::enterEvent(QEnterEvent* event) 
{
    if (m_labelGIF)
        m_labelGIF->hide();

    if (m_movieGIF)
        m_movieGIF->setPaused(true);

    QPushButton::enterEvent(event);
}

void AFQFreecshotUnInstallButton::leaveEvent(QEvent* event) 
{
    if (m_labelGIF)
        m_labelGIF->show();

    if (m_movieGIF)
        m_movieGIF->setPaused(false);

    QPushButton::leaveEvent(event);
}

void AFQFreecshotUnInstallButton::mousePressEvent(QMouseEvent* event) 
{
    if (m_labelGIF)
        m_labelGIF->hide();

    if (m_movieGIF)
        m_movieGIF->setPaused(true);

    if(m_labelIcon)
        m_labelIcon->setPixmap(m_pressedIcon);

    if (m_labelText) {
        m_labelText->setProperty("textPressed", "true");
        PolishStyleSheet(m_labelText);
    }

    QPushButton::mousePressEvent(event);
}

void AFQFreecshotUnInstallButton::mouseReleaseEvent(QMouseEvent* event) 
{
    if (m_labelGIF)
        m_labelGIF->show();

    if (m_movieGIF)
        m_movieGIF->setPaused(false);

    if (m_labelIcon)
        m_labelIcon->setPixmap(m_normalIcon);

    if (m_labelText) {
        m_labelText->setProperty("textPressed", "false");
        PolishStyleSheet(m_labelText);
    }

    QMetaObject::invokeMethod(this, "qslotUnInstallFreecShot", Qt::QueuedConnection);

    QPushButton::mouseReleaseEvent(event);
}

void AFQFreecshotUnInstallButton::qslotUnInstallFreecShot()
{
    MAINFRAME->ShowUninstallFreecShotAlert();
}