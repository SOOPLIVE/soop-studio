#include "CLeftNavigationBar.h"
#include "ui_left-navigation-bar.h"

#include <QScrollBar>
#include <QPropertyAnimation>
#include <QToolTip>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "CMainAccountButton.h"

#include "Application/CApplication.h"

#include "ui-validation.hpp"
#include "Blocks/CBlockManager.h"
#include "PopupWindows/CStatFrame.h"
#include "PopupWindows/CSavvyWidget.h"
#include "PopupWindows/CVirtualCamDialog.h"
#include "PopupWindows/BalloonTooltip.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "MainFrame/Output/COutput.h"

static int GetVisibleWidgetHeight(QWidget* widget)
{
    if (!widget || !widget->isVisible())
        return 0;

    return qMax(widget->height(), widget->sizeHint().height());
}

static int GetVisibleWidgetSpacing(QWidget* widget, int spacing)
{
    return (widget && widget->isVisible()) ? spacing : 0;
}

static QString GetMenuIconPath(const QString& name, bool active)
{
    std::string absPath;
    GetDataFilePath("assets", absPath);

    return QString("%1/block-icon/popup/%2/%3.svg")
        .arg(absPath.c_str())
        .arg(active ? "active" : "default")
        .arg(name);
}

AFQLeftNavigationBar::AFQLeftNavigationBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQLeftNavigationBar)
{
    ui->setupUi(this);

    _InitScrollBarHoverEvent();

    connect(this, &AFQLeftNavigationBar::qsignalAddButtonOpacity, 
        this, &AFQLeftNavigationBar::qslotAddButtonOpacity);

    connect(ui->mainChannelButton_2, &QPushButton::clicked, 
        this, &AFQLeftNavigationBar::ShowMainChannelLNB);

    connect(ui->favoriteMenuEmpty, &QPushButton::clicked,
        this, &AFQLeftNavigationBar::ShowMainChannelLNB);
}

AFQLeftNavigationBar::~AFQLeftNavigationBar()
{
    delete ui;
}

static int vcam_install()
{
    wchar_t cwd[MAX_PATH];
    GetCurrentDirectoryW(_countof(cwd) - 1, cwd);
    //
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp(); // bin
    dir.cdUp(); // obs-studio 
    dir.cd("data/obs-plugins/win-dshow");
    QString install_cmd = QString("%1/virtualcam-install.bat").arg(dir.absolutePath());
    std::wstring install_cmd_w = install_cmd.toStdWString();
    //
    SHELLEXECUTEINFO shExInfo = {0};
    shExInfo.cbSize = sizeof(shExInfo);
    shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    shExInfo.hwnd = nullptr;
    shExInfo.lpVerb = L"runas"; /* Operation to perform */
    shExInfo.lpFile = install_cmd_w.c_str();   /* Application to start */
    shExInfo.lpParameters = nullptr; /* Additional parameters */
    shExInfo.lpDirectory = cwd;
    shExInfo.nShow = SW_HIDE;
    shExInfo.hInstApp = nullptr;

    /* annoyingly the actual elevated updater will disappear behind other
     * windows :( */
    AllowSetForegroundWindow(ASFW_ANY);

    BOOL result = ShellExecuteEx(&shExInfo);
    if(result) {
        DWORD exitCode;

        WaitForSingleObject(shExInfo.hProcess, INFINITE);

        if(GetExitCodeProcess(shExInfo.hProcess, &exitCode)) {
            if(exitCode == 1) {
                return exitCode;
            }
        }
        CloseHandle(shExInfo.hProcess);
        //
        g_bRestart = true;
        MAINFRAME->close();
    } else {
        if(ERROR_CANCELLED == GetLastError()) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr, "", QTStr("Basic.VCam.InstallCancelled"));
        }
    }
    return 0;
}
void AFQLeftNavigationBar::qslotVirtualCamClicked()
{
    if(!MAINFRAME->VirtualCamEnabled()) {
        bool result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                 this, QT_UTF8(""),
                                                 QTStr("Basic.VCam.InstallVirtualCam"));
        if(result == QDialog::Accepted) {
            vcam_install();
        }

        QSignalBlocker blocker(virtualCamButton);
        virtualCamButton->setChecked(false);
        return;
    }

    bool active = false;
    if(AFOutputUtil::IsVirtualCamActive()) {
        MAINFRAME->qslotStopVirtualCam();
    } else {
        MAINFRAME->qslotStartVirtualCam();
        active = AFOutputUtil::IsVirtualCamActive();
    }

    UpdateVirtualCamIconState(active);

    virtualCamButton->setToolTip(active ?
        QTStr("Basic.Main.StopVirtualCam") : QTStr("Basic.Main.StartVirtualCam"));
}

void AFQLeftNavigationBar::qslotVirtualCamConfig()
{
    VCamConfig& config = MAINFRAME->VirtualCamConfig();
    AFQVirtualCamDialog dialog(config, AFOutputUtil::IsVirtualCamActive(), MAINFRAME);
    //
    connect(&dialog, &AFQVirtualCamDialog::Accepted, MAINFRAME, &AFMainFrame::UpdateVirtualCamConfig);
    connect(&dialog, &AFQVirtualCamDialog::AcceptedAndRestart, MAINFRAME, &AFMainFrame::RestartVirtualCam);

    dialog.exec();
}
void AFQLeftNavigationBar::showContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    QString label = (AFOutputUtil::IsVirtualCamActive() ?
                     QTStr("Basic.Main.StopVirtualCam") : QTStr("Basic.Main.StartVirtualCam"));
    menu.addAction(label);
    menu.addAction(QTStr("Basic.Main.VirtualCamConfig"));
    QAction* selectedItem = menu.exec(QCursor::pos());
    if(selectedItem) {
        if(selectedItem->text() == label) {
            qslotVirtualCamClicked();
        } else if(selectedItem->text() == QTStr("Basic.Main.VirtualCamConfig")) {
            qslotVirtualCamConfig();
        }
    }
}

void AFQLeftNavigationBar::qslotChannelSelected(bool clicked)
{
    UNUSED_PARAMETER(clicked);

    AFMainAccountButton* channelButton = reinterpret_cast<AFMainAccountButton*>(sender());
    SelectChannleSlide(channelButton);
}

void AFQLeftNavigationBar::qslotShowAddAccountInNavigationBar()
{
    if (AFOutputUtil::IsStreamActive())
        return;

    int channelCnt = AUTH_CONTEXT.GetCntChannel();

    AFChannelData* data = nullptr;
    if (AUTH_CONTEXT.GetMainChannelData(data))
        channelCnt++;

    if (channelCnt > 9)
    {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                                   "", QTStr("Basic.Settings.Stream.Account.Count.Warning"), false, true);
        return;
    }

    if (MAINFRAME->AddStreamAccount(MAINFRAME))
    {
        MAIN_OUTPUT->SetStreamingOutput();
        MAINFRAME->LoadAccounts();
    }
}

void AFQLeftNavigationBar::qslotShowToolTip()
{
    QWidget* senderWidget = qobject_cast<QWidget*>(sender());
    ShowTooltip(senderWidget);
}

void AFQLeftNavigationBar::qslotOnNavigationBarScreenChanged()
{
    update();
    if (m_channelSlide)
        m_channelSlide->update();
}

void AFQLeftNavigationBar::qslotAccountButtonUIReset()
{
    disconnect(m_channelSlide, &AFQChannelSlideWidget::qsignalChannelSlideClosed,
               this, &AFQLeftNavigationBar::qslotAccountButtonUIReset);

    ui->mainChannelButton->SetChecked(false);
    ui->mainChannelButton->TransparentPlatformImage(false);

    QList<AFMainAccountButton*> buttons = ui->scrollAreaWidgetContents_Channel->findChildren<AFMainAccountButton*>();
    foreach(AFMainAccountButton * b, buttons)
    {
        b->SetChecked(false);
        b->TransparentPlatformImage(false);
    }

    ResetChannelSlide();
}

void AFQLeftNavigationBar::qslotStudioModeClicked()
{
    ResetChannelSlide();

    bool isStudioMode = STATEAPP.IsPreviewProgramMode();
    if (!isStudioMode)
        MAINFRAME->EnablePreviewProgam();
    else
        MAINFRAME->DiablePreviewProgam();

}

void AFQLeftNavigationBar::qslotBlockClicked()
{
    ResetChannelSlide();
    QByteArray dockState = DYNAMIC_COMPOSIT->saveState().toBase64().constData();

    DYNAMIC_COMPOSIT->restoreState(dockState);

    QWidget* senderWidget = reinterpret_cast<QWidget*>(sender());

    bool retVal = false;
    int type = senderWidget->property("typeNum").toInt(&retVal);

    emit qsignalBlockButtonTriggered(true, type);
}

void AFQLeftNavigationBar::qslotBlockPressed()
{
    QWidget* senderWidget = reinterpret_cast<QWidget*>(sender());

    bool retVal = false;
    int type = senderWidget->property("typeNum").toInt(&retVal);
    if (retVal)
    {
        ENUM_WINDOW_TYPE enumType = static_cast<ENUM_WINDOW_TYPE>(type);

        AFQBaseDockWidget* outDock = nullptr;
        MAIN_BLOCKMANAGER->GetDock(enumType, outDock);
        if (!outDock)
            return;

        outDock->SetPressedState(true);
        outDock->SetStyle();
    }
}

void AFQLeftNavigationBar::qslotBlockReleased()
{
    QWidget* senderWidget = reinterpret_cast<QWidget*>(sender());

    bool retVal = false;
    int type = senderWidget->property("typeNum").toInt(&retVal);
    if (retVal)
    {
        ENUM_WINDOW_TYPE enumType = static_cast<ENUM_WINDOW_TYPE>(type);

        AFQBaseDockWidget* outDock = nullptr;
        MAIN_BLOCKMANAGER->GetDock(enumType, outDock);
        if (!outDock)
            return;

        outDock->SetPressedState(false);
        outDock->SetStyle();
    }
}

void AFQLeftNavigationBar::qslotSavvyClicked()
{
    ResetChannelSlide();
    //
    bool reactionAble = true;

    QList<QVariant> values = {  };
    SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_SAVVY_REACTION_ABLE, values, this, "qslotSavvyResponse");
}

void AFQLeftNavigationBar::qslotBlockStatusChanged(bool visible, int type, bool enableFavoriteMenu)
{
    UNUSED_PARAMETER(enableFavoriteMenu);

    switch (type)
    {
    case ENUM_WINDOW_TYPE::SceneSource:
        m_sceneSourceButton->setProperty("isShow", visible);
        PolishStyleSheet(m_sceneSourceButton);
        break;

    case ENUM_WINDOW_TYPE::AudioMixer:
        m_audioMixerButton->setProperty("isShow", visible);
        PolishStyleSheet(m_audioMixerButton);
        break;
    }
}

void AFQLeftNavigationBar::qslotButtonPressed()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button)
        return;

    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(button);
    effect->setOpacity(0.2);
    button->setGraphicsEffect(effect);
}

void AFQLeftNavigationBar::qslotButtonReleased()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (!button)
        return;

    button->setGraphicsEffect(nullptr);
}

void AFQLeftNavigationBar::qslotAddButtonOpacity(bool opacity)
{
    if (m_accountAddButton)
    {
        if (opacity)
        {
            QGraphicsOpacityEffect* op = new QGraphicsOpacityEffect(m_accountAddButton);
            op->setOpacity(0.2);
            m_accountAddButton->setGraphicsEffect(op);
        }
        else
        {
            m_accountAddButton->setGraphicsEffect(nullptr);
        }
    }
}

void AFQLeftNavigationBar::qslotSavvyResponse(const QByteArray& responseData)
{
    std::string jsonString = responseData.toStdString();
    std::string err;
    err.clear();
}

void AFQLeftNavigationBar::qslotOpenSignature(bool reactionAble)
{
    MAINFRAME->CreateSignatureAIPopup(reactionAble);
}

void AFQLeftNavigationBar::qslotCloseSignature(bool opened)
{
    qslotChangeSavvyStyle(opened);
}

void AFQLeftNavigationBar::qslotStudioModeStatusChanged(bool studioMode)
{
    if (!studioModeButton)
        return;

    studioModeButton->setIcon(QIcon(GetMenuIconPath("studiomode", studioMode)));
    studioModeButton->setProperty("isActive", studioMode);
    PolishStyleSheet(studioModeButton);
}

void AFQLeftNavigationBar::qslotOpenReaction()
{
    AFQBorderPopupBaseWidget* widget = nullptr;
    MAIN_BLOCKMANAGER->MakePopup(ENUM_WINDOW_TYPE::SAVVYReaction, widget, false, true, false, MAINFRAME);
}

void AFQLeftNavigationBar::qslotChangeSavvyStyle(bool opened)
{
    if (!savygButton)
        return;

    savygButton->setIcon(QIcon(GetMenuIconPath("savvy", opened)));
}

void AFQLeftNavigationBar::qslotChangeVirtualCamStyle(bool opened)
{
    UpdateVirtualCamIconState(opened);
}

void AFQLeftNavigationBar::qslotCheckSubscribeLive()
{
    auto& authManager = AUTH_CONTEXT;

    QMap<std::string, std::string> liveChannels = authManager.GetLiveChannels();
    if (liveChannels.contains(PLATFORM_SOOP))
    {
        QMap<std::string, std::string>::iterator iter;
        for (iter = liveChannels.begin(); iter != liveChannels.end(); ++iter)
        {
            if (iter.key() != PLATFORM_SOOP)
            {
                AFChannelData* data = nullptr;
                if (authManager.GetChannelData(iter.key(), data))
                {
                    if (data)
                    {
                        AFMainAccountButton* outButton = nullptr;
                        FindChannelButton(iter.key(), outButton);
                        if (outButton)
                        {
                            data->isStreaming = false;

                            outButton->GetChannelData()->isStreaming = false;
                            outButton->SetStreaming(AFOutputUtil::IsStreamActive(), false);

                        }
                    }
                }
            }
        }
    }
    MAIN_OUTPUT->SetStreamingOutput();
}

void AFQLeftNavigationBar::InitMainFrameAfter()
{
    disconnect(MAINFRAME, &AFMainFrame::qsignalMainShowEventTriggered, 
        this, &AFQLeftNavigationBar::InitMainFrameAfter);

    RefreshFavoriteLnbMenuButtons();
}
void AFQLeftNavigationBar::moveBalloonTooltip()
{
    if(!balloonTooltip || !extraMenuButton) {
        return;
    }

    const QRect rect = extraMenuButton->rect();
    QPoint pos = extraMenuButton->mapToGlobal(rect.topRight());

    const int centerY = pos.y() + rect.height() / 2;
    const int tooltipY = centerY - balloonTooltip->height() / 2;

    pos.setY(tooltipY - 20);
    pos.setX(pos.x() + 7);

    balloonTooltip->move(pos);
    balloonTooltip->show();
}

void AFQLeftNavigationBar::ShowMainChannelLNB()
{
    SelectChannleSlide(ui->mainChannelButton);
}

void AFQLeftNavigationBar::RecieveBroadState(bool stream)
{
    MAINFRAME->SetLnbMenuDisabled("Breaktime", !stream);
    UpdateFavoriteLnbMenuButtons("Breaktime");

    if (m_channelSlide)
        m_channelSlide->ToggleServiceButton(SERVICE_BREAKTIME, stream);
}

void AFQLeftNavigationBar::CertainMinuteBroadToggled(bool available)
{
    MAINFRAME->SetLnbMenuDisabled("SaveVodNow", !available);
    UpdateFavoriteLnbMenuButtons("SaveVodNow");

    if (m_channelSlide)
        m_channelSlide->ToggleServiceButton(SERVICE_SAVEVOD, available);
}

bool AFQLeftNavigationBar::LeftNavigationBarInit()
{
    ui->mainChannelButton->SetFixedSize(ui->mainChannelButton->size());
    ui->mainChannelButton->setCheckable(true);
    ui->mainChannelButton->setProperty("leftSideBar", true);
    PolishStyleSheet(ui->mainChannelButton);


    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();

    connect(broadInfo, &AFQBroadInfo::qsignalSubscribeCheck, this, &AFQLeftNavigationBar::qslotCheckSubscribeLive);

    _LoadBlocks();
    if (!LoadNavigationAccounts())
        return false;
 
    connect(MAINFRAME, &AFMainFrame::qsignalMainShowEventTriggered, 
        this, &AFQLeftNavigationBar::InitMainFrameAfter);


    return true;
}

void AFQLeftNavigationBar::ConnectNavigationBarScreen()
{
    QWindow* w = windowHandle();
    if(w)
        connect(w, &QWindow::screenChanged, this, &AFQLeftNavigationBar::qslotOnNavigationBarScreenChanged);
}

void AFQLeftNavigationBar::MoveChannelSlide()
{
    if (m_channelSlide != nullptr)
    {
        QSize contentsArea = MAINFRAME->GetContentsArea();
        QPoint globalPos = MAINFRAME->mapToGlobal(QPoint(0, 0));
#ifdef _WIN32
        m_channelSlide->setGeometry(globalPos.x() + width(), globalPos.y() + 40, contentsArea.width(), height());
#elif defined(__APPLE__)
        m_channelSlide->setGeometry(globalPos.x() + width() + 1, globalPos.y() + 1, contentsArea.width() - 3, height() - 3);
#endif
    }

    UpdateFavoriteLnbMenuAreaSize();
}

void AFQLeftNavigationBar::HideChannelSlide()
{
    if(m_channelSlide && m_channelSlide->isVisible()){
        ui->mainChannelButton->SetChecked(true);
        ui->mainChannelButton->TransparentPlatformImage(false);
        
        QList<AFMainAccountButton*> buttons = ui->scrollAreaWidgetContents_Channel->findChildren<AFMainAccountButton*>();
        foreach(AFMainAccountButton * b, buttons)
        {
            b->SetChecked(true);
            b->TransparentPlatformImage(false);
        }

        ResetChannelSlide();
        m_accountAddButton->setGraphicsEffect(nullptr);
    }
}

void AFQLeftNavigationBar::ToggleAddChannelButton(bool isStream)
{
    m_accountAddButton->setVisible(!isStream);
}

bool AFQLeftNavigationBar::FindChannelButton(std::string platform, AFMainAccountButton*& outbutton)
{
    if (ui->mainChannelButton->GetCurrentPlatform() == platform)
    {
        outbutton = ui->mainChannelButton;
        return true;
    }

    QList<AFMainAccountButton*> buttons = ui->scrollAreaWidgetContents_Channel->findChildren<AFMainAccountButton*>();

    foreach(AFMainAccountButton * b, buttons)
    {
        if (b->GetCurrentPlatform() == platform)
        {
            outbutton = b;
            return true;
        }
    }

    return false;
}

void AFQLeftNavigationBar::SelectChannelSlide(std::string platform)
{
    AFMainAccountButton* pAccountBtn = nullptr;
    bool findChannel = FindChannelButton(platform, pAccountBtn);
    if (!findChannel)
        return;

    SelectChannleSlide(pAccountBtn);
}

void AFQLeftNavigationBar::SelectChannleSlide(AFMainAccountButton* channelButton)
{
    if (!channelButton)
        return;

    if (channelButton != ui->mainChannelButton)
    {
        ui->mainChannelButton->SetChecked(false);
        ui->mainChannelButton->TransparentPlatformImage(true);
    }
    else
    {
        ui->mainChannelButton->TransparentPlatformImage(false);
    }

    QList<AFMainAccountButton*> buttons = ui->scrollAreaWidgetContents_Channel->findChildren<AFMainAccountButton*>();
    foreach(AFMainAccountButton * b, buttons)
    {
        if (channelButton == b)
        {
            b->TransparentPlatformImage(false);
            continue;
        }

        b->SetChecked(false);
        b->TransparentPlatformImage(true);
    }

    if (m_pSelectedChannelNum != channelButton)
    {
#ifdef __APPLE__
        ResetChannelSlide();
#endif
        m_pSelectedChannelNum = channelButton;
        if (!m_channelSlide)
        {
            m_channelSlide = new AFQChannelSlideWidget(MAINFRAME);

            QPoint globalPos = MAINFRAME->mapToGlobal(QPoint(0, 0));

            QSize contentsArea = MAINFRAME->GetContentsArea();
#ifdef _WIN32
            m_channelSlide->setGeometry(globalPos.x() + width(), globalPos.y() + 40, contentsArea.width(), height());
#elif defined(__APPLE__)
            m_channelSlide->setGeometry(globalPos.x() + width() + 1, globalPos.y() + 1, contentsArea.width() - 3, height() - 3);
#endif

            m_channelSlide->SetChannelSlideGeometry(QRect(0, 0, 260, height()));
            connect(m_channelSlide, &AFQChannelSlideWidget::qsignalChannelSlideClosed,
                    this, &AFQLeftNavigationBar::qslotAccountButtonUIReset);
        }

        if (m_channelSlide) {
            m_channelSlide->ChangeSlideInfo(m_pSelectedChannelNum);
            m_channelSlide->show();

        }

        MAINFRAME->OnSoopEvent(SOOP_FRONTEND_EVENT_STATE_SIDEBAR_ON, nullptr);

        emit qsignalAddButtonOpacity(true);
    }
    else
    {
        ResetChannelSlide();
    }
}

void AFQLeftNavigationBar::ResetChannelSlide()
{
    if(m_channelSlide)
    {
        QWidget* tmpWidget = m_channelSlide.data();
        m_channelSlide = nullptr;

        tmpWidget->close();
        tmpWidget->deleteLater();

        emit qsignalAddButtonOpacity(false);
    }
    m_pSelectedChannelNum = nullptr;

    MAINFRAME->OnSoopEvent(SOOP_FRONTEND_EVENT_STATE_SIDEBAR_OFF, nullptr);
}

QPushButton* AFQLeftNavigationBar::CreateFavoriteLnbMenuButton(const LnbMenuItem& item)
{
    QPushButton* button = new QPushButton(ui->scrollAreaWidgetContents);

    QString objectName = QString("favoriteMenuButton_%1").arg(item.menuId);
    button->setObjectName(objectName);
    button->setProperty("buttonType", "favoriteMenuButton");
    button->setProperty("favoriteLnbMenuId", item.menuId);
    button->setFixedSize(28, 28);
    button->setCursor(Qt::PointingHandCursor);
    button->setText("");
    button->setToolTip(item.menuName);
    button->setIconSize(QSize(24, 24));

    const QString menuId = item.menuId;

    // save vod, breaktime check....
    bool disabled = false;
    if (menuId.compare("SaveVodNow") == 0) {
        disabled = !MAINFRAME->CheckSplitVodAvailable();
        //button->setDisabled(disabled);
    }

    if (menuId.compare("Breaktime") == 0) {
        disabled = !AFOutputUtil::IsStreamActive();
        //button->setDisabled(disabled);
    }

    QString iconPath;
    if (disabled)
        iconPath = item.iconDisabledPath;
    else 
        iconPath = item.active ? item.iconActivePath : item.iconDefaultPath;

    if (!iconPath.isEmpty())
        button->setIcon(QIcon(iconPath));

    connect(button, &QPushButton::clicked,
        this, [menuId](bool) {
            if (MAINFRAME)
                MAINFRAME->TriggerLnbMenu(menuId);
        });

    return button;
}

void AFQLeftNavigationBar::SetFavoriteLnbMenuEmptyState(bool empty)
{
    ui->favoriteMenu->setVisible(!empty);
    ui->favoriteMenuEmpty->setVisible(empty);
    ui->favoriteMenuFrame->setMinimumHeight(0);

    if (empty)
        ui->favoriteMenuFrame->setFixedHeight(51);
}

void AFQLeftNavigationBar::ClearFavoriteLnbMenuButtons()
{
    for (QPushButton* button : favoriteLnbMenuButtons) {
        if (!button)
            continue;

        ui->verticalLayout_6->removeWidget(button);
        button->hide();
        button->setParent(nullptr);
        button->deleteLater();
    }

    favoriteLnbMenuButtons.clear();
}

void AFQLeftNavigationBar::RefreshFavoriteLnbMenuButtons()
{
    ClearFavoriteLnbMenuButtons();

    if (!MAINFRAME)
        return;

    AFQBroadInfo* info = AUTH_CONTEXT.GetSoopBroadInfo();
    const bool allowSubTitle = info && info->AllowSubTitle();

    QVector<LnbMenuItem> favorites;
    const QVector<LnbMenuItem>& items = MAINFRAME->GetLnbMenuItems();
    favorites.reserve(items.size());

    for (const LnbMenuItem& item : items) {
        if (!item.isFavorite)
            continue;

        if (!allowSubTitle && item.menuId == "SubTitle")
            continue;

        favorites.push_back(item);
    }

    std::sort(favorites.begin(), favorites.end(),
        [](const LnbMenuItem& a, const LnbMenuItem& b) {
            return a.favoriteAt > b.favoriteAt; // recently order
        });

    if (favorites.isEmpty()) {
        SetFavoriteLnbMenuEmptyState(true);
        return;
    }
    SetFavoriteLnbMenuEmptyState(false);

    for (const LnbMenuItem& item : favorites) {

        QPushButton* button = CreateFavoriteLnbMenuButton(item);
        ui->verticalLayout_6->addWidget(button, 0, Qt::AlignHCenter);
        favoriteLnbMenuButtons.push_back(button);
    }

    UpdateFavoriteLnbMenuAreaSize();
}

void AFQLeftNavigationBar::UpdateFavoriteLnbMenuButtons(const QString& menuId)
{
    QPushButton* targetButton = nullptr;

    for (const QPointer<QPushButton>& buttonPtr : favoriteLnbMenuButtons) {
        QPushButton* button = buttonPtr.data();
        if (!button)
            continue;

        if (button->property("favoriteLnbMenuId").toString() != menuId)
            continue;

        targetButton = button;
        break;
    }

    if (!targetButton)
        return;

    const QVector<LnbMenuItem>& items = MAINFRAME->GetLnbMenuItems();

    auto iter = std::find_if(items.cbegin(), items.cend(),
        [&menuId](const LnbMenuItem& item) {
            return item.menuId == menuId;
        });

    if (iter == items.cend())
        return;

    bool disabled = iter->disabled;
    bool active = iter->active;
    QString iconPath;

    if (disabled) {
        iconPath = iter->iconDisabledPath;
    } else {
        QSignalBlocker blocker(targetButton);
        targetButton->setChecked(active);

        iconPath = active
            ? iter->iconActivePath
            : iter->iconDefaultPath;
    }

    if (!iconPath.isEmpty())
        targetButton->setIcon(QIcon(iconPath));
}

void AFQLeftNavigationBar::UpdateFavoriteLnbMenuAreaSize()
{
    const int favoriteCount = favoriteLnbMenuButtons.size();

    if (favoriteCount <= 0) {
        SetFavoriteLnbMenuEmptyState(true);
        return;
    }

    ui->favoriteMenuFrame->setMinimumHeight(0);
    ui->scrollArea->setMinimumHeight(0);
    ui->scrollAreaWidgetContents->setMinimumHeight(0);

    const int buttonSize = 28;
    const int minChannelAreaHeight = 120;

    const QMargins contentMargins = ui->verticalLayout_6->contentsMargins();
    const int contentSpacing = ui->verticalLayout_6->spacing();

    auto CalcContentHeight = [&](int count) -> int {
        int height = contentMargins.top() + contentMargins.bottom();

        const int validCount = qMin(count, favoriteLnbMenuButtons.size());

        for (int i = 0; i < validCount; ++i) {
            QPushButton* button = favoriteLnbMenuButtons[i];
            if (!button)
                continue;

            height += qMax(buttonSize, qMax(button->height(), button->sizeHint().height()));
        }

        height += qMax(0, validCount - 1) * contentSpacing;
        return height;
        };

    const int totalContentHeight =
        CalcContentHeight(favoriteCount);
    ui->scrollAreaWidgetContents->setMinimumHeight(totalContentHeight);

    const QMargins frameMargins = ui->verticalLayout_5->contentsMargins();
    const int frameSpacing = ui->verticalLayout_5->spacing();
    const int emptyFrameHeight = GetVisibleWidgetHeight(ui->emptyFrame);
    const int bottomFrameHeight = GetVisibleWidgetHeight(ui->frame);

    const int frameOverhead =
        frameMargins.top()
        + frameMargins.bottom()
        + emptyFrameHeight
        + bottomFrameHeight
        + GetVisibleWidgetSpacing(ui->emptyFrame, frameSpacing)
        + GetVisibleWidgetSpacing(ui->frame, frameSpacing);

    const int availableFrameHeight =
        height()
        - ui->widget_MainChannel->height()
        - ui->widget_Block->height()
        - minChannelAreaHeight;

    const int availableScrollHeight =
        availableFrameHeight - frameOverhead;

    const int scrollHeight =
        qBound(24, availableScrollHeight, totalContentHeight);

    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    ui->scrollArea->setFixedHeight(scrollHeight);

    const int frameHeight =
        frameOverhead + scrollHeight + 1;

    ui->favoriteMenuFrame->setFixedHeight(frameHeight);
}

void AFQLeftNavigationBar::UpdateVirtualCamIconState(bool active)
{
    if (!virtualCamButton)
        return;

    virtualCamButton->setIcon(QIcon(GetMenuIconPath("virtualcam", active)));
}

bool AFQLeftNavigationBar::LoadNavigationAccounts()
{
    _LoadMultiStreamAccounts();
    bool retVal = _SetBaseAccounts();

    return retVal;
}

bool AFQLeftNavigationBar::_LoadMultiStreamAccounts()
{
    ResetChannelSlide();

    auto& authManager = AUTH_CONTEXT;

    QVBoxLayout* scrollareaLayout = reinterpret_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents_Channel->layout());

    RemoveAllChildInLayout(scrollareaLayout);

    int channelCount = authManager.GetCntChannel();
    bool regOtherAccount = channelCount > 0 ? true : false;
    if (regOtherAccount)
    {
        QMap<std::string, AFChannelData*> platformDatas;

        AFChannelData* main = nullptr;
        authManager.GetMainChannelData(main);
        if(main)
            platformDatas.insert(main->pAuthData->platform, main);

        QList<AFChannelData*> customRtmpDatas;

        for (int idx = 0; idx < channelCount; idx++)
        {
            AFChannelData* tmpChannel = nullptr;
            authManager.GetChannelData(idx, tmpChannel);
            std::string channelName = tmpChannel->pAuthData->platform;
            std::string channelID = tmpChannel->pAuthData->channelID;

            if (channelName == PLATFORM_CUSTOM_RTMP)
                customRtmpDatas.append(tmpChannel);
            else
                platformDatas.insert(channelName, tmpChannel);
        }

        if (platformDatas.contains(PLATFORM_TWITCH))
        {
            AFMainAccountButton* outButton = nullptr;
            _MakeAccountButton(platformDatas.value(PLATFORM_TWITCH), outButton);
            scrollareaLayout->addWidget(outButton);
        }

        if (platformDatas.contains(PLATFORM_YOUTUBE))
        {
            AFMainAccountButton* outButton = nullptr;
            _MakeAccountButton(platformDatas.value(PLATFORM_YOUTUBE), outButton);
            scrollareaLayout->addWidget(outButton);
        }

        foreach(AFChannelData* customRtmpData, customRtmpDatas)
        {
            AFMainAccountButton* outButton = nullptr;
            _MakeAccountButton(customRtmpData, outButton);
            scrollareaLayout->addWidget(outButton);
        }
    }

    m_accountAddButton = new QPushButton(this);
    m_accountAddButton->setFixedSize(QSize(36,36)/*ui->mainChannelButton->size()*/);
    m_accountAddButton->setProperty("buttonType", "addPlatform");
    m_accountAddButton->setToolTip(QTStr("Simulcast.AddChannel"));
    m_accountAddButton->setObjectName("pushButton_AccountAdd");
    
    connect(m_accountAddButton, &QPushButton::clicked, this, &AFQLeftNavigationBar::qslotShowAddAccountInNavigationBar);
    
    scrollareaLayout->addItem(new QSpacerItem(10, 6, QSizePolicy::Minimum, QSizePolicy::Fixed));
    scrollareaLayout->addWidget(m_accountAddButton, 0, Qt::AlignHCenter);
    scrollareaLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));

    bool isStreaming = AFOutputUtil::IsStreamActive();
    ToggleAddChannelButton(isStreaming);

    return true;
}

void AFQLeftNavigationBar::_MakeAccountButton(AFChannelData* data, AFMainAccountButton*& outButton)
{
    std::string channelName = data->pAuthData->platform;
    std::string channelID = data->pAuthData->channelID;

    outButton = new AFMainAccountButton(this);
    outButton->SetChannelData(data);
    outButton->SetFixedSize(ui->mainChannelButton->size());
    outButton->SetPlatform(channelName);
    outButton->TransparentPlatformImage(false);
    outButton->SetStreaming(AFOutputUtil::IsStreamActive(), data->isStreaming);
    outButton->setProperty("leftSideBar", true);
    PolishStyleSheet(outButton);

    if (0 == channelName.compare(PLATFORM_CUSTOM_RTMP)) {
        outButton->setToolTip(QString::fromStdString(data->pAuthData->channelID));
    }
    else {
        outButton->setToolTip(QString::fromStdString(channelName));
    }
    outButton->setCheckable(true);

    connect(outButton, &AFMainAccountButton::clicked, this, &AFQLeftNavigationBar::qslotChannelSelected);
    connect(outButton, &AFMainAccountButton::qsignalAccountButtonMouseMove, this, &AFQLeftNavigationBar::qslotShowToolTip);
}

void AFQLeftNavigationBar::_LoadBlocks()
{
    connect(MAIN_BLOCKMANAGER, &AFQBlockManager::qsignalBlockVisible, 
        this, &AFQLeftNavigationBar::qslotBlockStatusChanged);

    m_sceneSourceButton = new AFQCustomPushbutton(this);
    m_sceneSourceButton->setFixedSize(32, 32);
    m_sceneSourceButton->setProperty("buttonType", "BroadItemList");
    m_sceneSourceButton->setProperty("isShow", false);
    m_sceneSourceButton->setProperty("typeNum", ENUM_WINDOW_TYPE::SceneSource);
    m_sceneSourceButton->setToolTip(QTStr("Block.Tooltip.SceneSourceDock"));
    m_sceneSourceButton->setObjectName("pushButton_SceneSource");

	connect(m_sceneSourceButton, &QPushButton::pressed, this, &AFQLeftNavigationBar::qslotBlockPressed);
	connect(m_sceneSourceButton, &QPushButton::released, this, &AFQLeftNavigationBar::qslotBlockReleased);
    connect(m_sceneSourceButton, &QPushButton::clicked, this, &AFQLeftNavigationBar::qslotBlockClicked);
    connect(m_sceneSourceButton, &AFQCustomPushbutton::qsignalMousePressed, this, &AFQLeftNavigationBar::qslotButtonPressed);
    connect(m_sceneSourceButton, &AFQCustomPushbutton::qsignalMouseReleased, this, &AFQLeftNavigationBar::qslotButtonReleased);

    m_audioMixerButton = new AFQCustomPushbutton(this);
    m_audioMixerButton->setFixedSize(32, 32);
    m_audioMixerButton->setProperty("buttonType", "AudioList");
    m_audioMixerButton->setProperty("isShow", false);
    m_audioMixerButton->setProperty("typeNum", ENUM_WINDOW_TYPE::AudioMixer);
    m_audioMixerButton->setToolTip(QTStr("Block.Tooltip.AudioMixer"));
    m_audioMixerButton->setObjectName("pushButton_AudioMixer");

    connect(m_audioMixerButton, &QPushButton::pressed, this, &AFQLeftNavigationBar::qslotBlockPressed);
    connect(m_audioMixerButton, &QPushButton::released, this, &AFQLeftNavigationBar::qslotBlockReleased);
    connect(m_audioMixerButton, &QPushButton::clicked, this, &AFQLeftNavigationBar::qslotBlockClicked);
    connect(m_audioMixerButton, &AFQCustomPushbutton::qsignalMousePressed, this, &AFQLeftNavigationBar::qslotButtonPressed);
    connect(m_audioMixerButton, &AFQCustomPushbutton::qsignalMouseReleased, this, &AFQLeftNavigationBar::qslotButtonReleased);

    ui->widget_Block->layout()->addWidget(m_sceneSourceButton);
    ui->widget_Block->layout()->addWidget(m_audioMixerButton);

    QString locale = QString::fromStdString(LOCALE_CONTEXT.GetCurrentLocaleStr());

    const QString extraMenuButtonStyle = R"(
        QPushButton {
            border : none;
            text-align: left;
            font-size : 15px;
            color : #D5D7DC;
        }
        QPushButton:hover {
	        background-color: rgba(255, 255, 255, 10%);
        }
        QPushButton:checked,
        QPushButton[isActive="true"] {
            color : #0182FF;
        }
        QPushButton[isActive="false"] {
            color : #D5D7DC;
        })";

    int extraMenuHeight = 126;
    extraMenu = new QFrame(this);
    extraMenu->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    extraMenu->setAttribute(Qt::WA_StyledBackground, true);
    extraMenu->setFocusPolicy(Qt::StrongFocus);
    extraMenu->setStyleSheet("background-color : #202328; border : 1px solid #111111;");
    extraMenu->installEventFilter(this);

    QVBoxLayout* extraMenuLayout = new QVBoxLayout(extraMenu);
    extraMenuLayout->setSpacing(8);
    extraMenuLayout->setContentsMargins(14, 12, 14, 12);

    extraMenuButton = new QPushButton(this);
    extraMenuButton->setFixedSize(QSize(32, 32));
    extraMenuButton->setProperty("buttonType", "leftBarExtraMenu");
    extraMenuButton->setToolTip(QTStr("LeftSideBar.ExtraMenu"));
    extraMenuButton->setCheckable(true);
    ui->widget_Block->layout()->addWidget(extraMenuButton);

    auto setupExtraMenuActionButton = [this, &extraMenuButtonStyle](AFQCustomPushbutton* button,
        const char* objectName, const QString& tooltip, const QString& text)
        {
            if (!button)
                return;
            
            button->setMinimumWidth(172);
            button->setFixedHeight(32);
            button->setIconSize(QSize(32, 32));
            button->setObjectName(objectName);
            button->setToolTip(tooltip);
            button->setText(text);
            button->setStyleSheet(extraMenuButtonStyle);
        };

    virtualCamButton = new AFQCustomPushbutton();
    setupExtraMenuActionButton(
        virtualCamButton,
        "pushButton_VirtualCamera",
        QTStr("Basic.VCam.VirtualCamera"),
        QTStr("Basic.VCam.VirtualCamera"));

    virtualCamButton->setCheckable(true);
    virtualCamButton->setContextMenuPolicy(Qt::CustomContextMenu);

    const bool active =
        MAINFRAME->VirtualCamEnabled() && AFOutputUtil::IsVirtualCamActive();

    UpdateVirtualCamIconState(active);

    connect(virtualCamButton, &QPushButton::clicked,
        this, &AFQLeftNavigationBar::qslotVirtualCamClicked);

    connect(virtualCamButton, &QWidget::customContextMenuRequested,
        this, &AFQLeftNavigationBar::showContextMenu);


    studioModeButton = new AFQCustomPushbutton();
    setupExtraMenuActionButton(
        studioModeButton,
        "pushButton_StudioMode",
        QTStr("Basic.TogglePreviewProgramMode"),
        QTStr("Basic.TogglePreviewProgramMode"));

    studioModeButton->setCheckable(true);
    connect(studioModeButton, &QPushButton::clicked,
        this, &AFQLeftNavigationBar::qslotStudioModeClicked);

    qslotStudioModeStatusChanged(STATEAPP.IsPreviewProgramMode());

    QLabel* menuLabel = new QLabel(extraMenu);
    menuLabel->setText(QTStr("LeftSideBar.ExtraMenu"));
    menuLabel->setStyleSheet("border: none; font-size:15px; color: #9196A1;");

    extraMenuLayout->addWidget(menuLabel);
    if (locale == "ko-KR") {
        extraMenuHeight = 166;

        savygButton = new AFQCustomPushbutton(this);
        setupExtraMenuActionButton(
            savygButton,
            "pushButton_SAVYG",
            QTStr("Savyg.Info"),
            QTStr("Savyg.Info"));

        qslotChangeSavvyStyle(false);

        connect(savygButton, &QPushButton::clicked,
            this, &AFQLeftNavigationBar::qslotSavvyClicked);
        connect(savygButton, &AFQCustomPushbutton::qsignalMousePressed,
            this, &AFQLeftNavigationBar::qslotButtonPressed);
        connect(savygButton, &AFQCustomPushbutton::qsignalMouseReleased,
            this, &AFQLeftNavigationBar::qslotButtonReleased);

        extraMenuLayout->addWidget(savygButton);
    }

    extraMenuLayout->addWidget(virtualCamButton);
    extraMenuLayout->addWidget(studioModeButton);

    connect(extraMenuButton, &QPushButton::clicked,
        this, [this, extraMenuHeight]() {
            if (!extraMenu || !extraMenuButton)
                return;

            if (balloonTooltip) {
                emit balloonTooltip->closeTooltipEvent();
                balloonTooltip->close();
            }

            HideChannelSlide();

            if (ignoreExtraMenuButtonClicked) {
                ignoreExtraMenuButtonClicked = false;

                QSignalBlocker blocker(extraMenuButton);
                extraMenuButton->setChecked(false);
                return;
            }

            if (extraMenu->isVisible()) {
                extraMenu->hide();

                QSignalBlocker blocker(extraMenuButton);
                extraMenuButton->setChecked(false);
                return;
            }

            QSignalBlocker blocker(extraMenuButton);
            extraMenuButton->setChecked(true);

            QPoint globalPos = extraMenuButton->mapToGlobal(
                QPoint(extraMenuButton->width(), 0));

            globalPos += QPoint(11, -(extraMenuHeight - 40));

            extraMenu->move(globalPos);
            extraMenu->show();
            extraMenu->raise();
            extraMenu->activateWindow();

        });

    extraMenu->setMinimumWidth(207);
    extraMenu->setFixedHeight(extraMenuHeight);
}

bool AFQLeftNavigationBar::_SetBaseAccounts()
{
    disconnect(ui->mainChannelButton, &QPushButton::clicked, 0, 0);
    disconnect(ui->mainChannelButton, &AFMainAccountButton::qsignalAccountButtonMouseMove, 0, 0);

    auto& authManager = AUTH_CONTEXT;

    QString platform = PLATFORM_SOOP;
    ui->mainChannelButton->setProperty("platformName", platform);
    ui->mainChannelButton->SetPlatform(platform.toStdString());
    ui->mainChannelButton->SetMainAccount();

    AFChannelData* mainChannel = nullptr;
    if (authManager.GetChannelData(platform.toStdString(), mainChannel))
    {
        std::string mainChannelName = mainChannel->pAuthData->platform;
        std::string mainID = mainChannel->pAuthData->channelID;
        ui->mainChannelButton->SetChannelData(mainChannel);
        ui->mainChannelButton->SetStreaming(AFOutputUtil::IsStreamActive(), mainChannel->isStreaming);
        ui->mainChannelButton->setToolTip(QString::fromStdString(mainChannelName));
        ui->mainChannelButton->TransparentPlatformImage(false);

        connect(ui->mainChannelButton, &QPushButton::clicked, this, &AFQLeftNavigationBar::qslotChannelSelected);
        connect(ui->mainChannelButton, &AFMainAccountButton::qsignalAccountButtonMouseMove,
                this, &AFQLeftNavigationBar::qslotShowToolTip);

        return true;
    }
    else
    {
        ui->mainChannelButton->SetStreaming(AFOutputUtil::IsStreamActive(), false, true);
        ui->mainChannelButton->TransparentPlatformImage(false);

        connect(ui->mainChannelButton, &QPushButton::clicked, this, &AFQLeftNavigationBar::qslotShowAddAccountInNavigationBar);
    }

    return false;
}

void AFQLeftNavigationBar::HandleExtraMenuEvent(QObject* watched, QEvent* event)
{
    if (watched != extraMenu)
        return;

    const QEvent::Type type = event->type();

    if (type == QEvent::WindowDeactivate ||
        type == QEvent::FocusOut) {

        if (extraMenu->isVisible())
            extraMenu->hide();

        return;
    }

    if (type != QEvent::Hide && type != QEvent::Close)
        return;

    ignoreExtraMenuButtonClicked =
        extraMenuButton &&
        extraMenuButton->rect().contains(
            extraMenuButton->mapFromGlobal(QCursor::pos()));

    if (extraMenuButton) {
        QSignalBlocker blocker(extraMenuButton);
        extraMenuButton->setChecked(false);
    }
}

void AFQLeftNavigationBar::HandleScrollAreaHoverEvent(QObject* watched, QEvent* event,
                                                QScrollArea* scrollArea, QWidget* contentsWidget)
{
    if (!scrollArea || !contentsWidget)
        return;

    const bool watchedScrollArea =
        watched == scrollArea ||
        watched == scrollArea->viewport() ||
        watched == contentsWidget;

    if (!watchedScrollArea)
        return;

    if (event->type() == QEvent::Enter) {
        SetScrollBarTransparent(scrollArea, contentsWidget, false);
        return;
    }

    if (event->type() != QEvent::Leave)
        return;

    QTimer::singleShot(0, this, [this, scrollArea, contentsWidget]() {
        if (!scrollArea || !contentsWidget)
            return;

        const QPoint localPos = scrollArea->mapFromGlobal(QCursor::pos());

        if (!scrollArea->rect().contains(localPos))
            SetScrollBarTransparent(scrollArea, contentsWidget, true);
        });
}

bool AFQLeftNavigationBar::eventFilter(QObject* watched, QEvent* event)
{

    HandleExtraMenuEvent(watched, event);

    HandleScrollAreaHoverEvent(
        watched,
        event,
        ui->scrollArea,
        ui->scrollAreaWidgetContents);

    HandleScrollAreaHoverEvent(
        watched,
        event,
        ui->scrollArea_Channel,
        ui->scrollAreaWidgetContents_Channel);

    return QWidget::eventFilter(watched, event);
}

void AFQLeftNavigationBar::_InitScrollBarHoverEvent()
{
    ui->scrollArea->installEventFilter(this);
    ui->scrollArea->viewport()->installEventFilter(this);
    ui->scrollAreaWidgetContents->installEventFilter(this);

    SetScrollBarTransparent(ui->scrollArea,
        ui->scrollAreaWidgetContents,
        true);

    ui->scrollArea_Channel->installEventFilter(this);
    ui->scrollArea_Channel->viewport()->installEventFilter(this);
    ui->scrollAreaWidgetContents_Channel->installEventFilter(this);

    SetScrollBarTransparent(ui->scrollArea_Channel,
        ui->scrollAreaWidgetContents_Channel,
        true);
}