#include "CMainFrame.h"
#include "ui_aneta-main-frame.h"

#include "Application/CApplication.h"
#include "Utils/CJsonController.h"

#include <QPropertyAnimation>
#include <QBoxLayout>
#include <QUiLoader>
#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QMovie>
#include <QTimer>
#include <QTextEdit>
#include <QLineEdit>
#include <QStyleOption>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QWindow>
#include <QMessageBox>
#include <QDialog>
#include <QFontMetrics>
#include <QGraphicsOpacityEffect>
#include <QToolButton>
#include <QWidgetAction>
#include <QMimeData>
#include <QUrlQuery>
#include <QSettings>
#include <qdir.h>

#include <fstream>
#include <sstream>

#include <iterator>

#include "qt-wrapper.h"
#include "util/dstr.hpp"
#include "util/platform.h"
#include "platform/platform.hpp"

#include "Common/SettingsMiscDef.h"

#include "CUIValidation.h"
#include "CMainFrameGuide.h"
#include "MainPreview/CProgramView.h"
#include "MainPreview/CVerticalProgramView.h"

#include "ViewModel/MainWindow/CMainWindowAccesser.h"
#include "ViewModel/MainWindow/CMainWindowRenderModel.h"

#include "ViewModel/Auth/Soop/auth-soop-global.h"
#include "ViewModel/Auth/Soop/auth-soop.hpp"
#include "ViewModel/Auth/Twitch/auth-twitch.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/OBSData/CInhibitSleepContext.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Source/CSource.h"
#include "CoreModel/Service/CService.h"
#include "CoreModel/Video/CVideo.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Icon/CIconContext.h"
#include "CoreModel/Action/CHotkeyContext.h"

#include "Blocks/SceneControlDock/CProjector.h"
#include "Blocks/SceneSourceDock/CSourceListView.h"
#include "Blocks/SceneSourceDock/CSceneSourceDockWidget.h"

#include "UIComponent/CSliderFrame.h"
#include "UIComponent/CBasicPreview.h"
#include "UIComponent/CCustomMenu.h"
#include "UIComponent/CSceneBottomButton.h"
#include "UIComponent/CMessageBox.h"
#include "UIComponent/CBasicToggleButton.h"

#include "PopupWindows/CStatFrame.h"
#include "PopupWindows/CBalloonWidget.h"
#include "PopupWindows/SourceDialog/CSelectSourceDialog.h"
#include "PopupWindows/SourceDialog/CSourceProperties.h"
#include "PopupWindows/SettingPopup/CStudioSettingDialog.h"
#include "PopupWindows/SourceDialog/CSelectSourceButton.h"
#include "PopupWindows/SourceDialog/CSceneSelectDialog.h"
#include "PopupWindows/CCustomColorDialog.h"
#include "PopupWindows/SettingPopup/CSettingUtils.h"
#include "PopupWindows/CRemuxFrame.h"
#include "PopupWindows/SettingPopup/CAddStreamWidget.h"


#include "ViewModel/Auth/COAuthLogin.hpp"
#include "ViewModel/Auth/CAuth.h"
#include "ViewModel/Auth/COAuthLogin.hpp"
#include "ViewModel/Auth/CAuthListener.hpp"
#include "ViewModel/Auth/YouTube/auth-youtube.hpp"
#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"
const QString SchedulDateAndTimeFormat = "yyyy-MM-dd'T'hh:mm:ss'Z'";

Q_DECLARE_METATYPE(OBSScene);
Q_DECLARE_METATYPE(OBSSceneItem);
Q_DECLARE_METATYPE(OBSSource);

inline void clearLayout(QLayout* layout) {
    if (!layout) return;

    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        else if (QLayout* childLayout = item->layout()) {
            clearLayout(childLayout);
        }
        delete item;
    }
}

AFMainFrame::AFMainFrame(QWidget *parent, Qt::WindowFlags flag) :

    AFCQMainBaseWidget(parent, flag),
    ui(new Ui::AFMainFrame),
    m_undo_s(this),
    m_bBlockAnimating(false),
    m_bIsBlockPopup(true)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NativeWindow);

    setAcceptDrops(true);

    qRegisterMetaType<OBSScene>("OBSScene");
    qRegisterMetaType<OBSSceneItem>("OBSSceneItem");
    qRegisterMetaType<OBSSource>("OBSSource");
       
#ifdef __APPLE__
    ui->action_RemoveSource->setShortcut({ Qt::Key_Backspace });
#endif


    /*QFontMetrics fontMetrics(ui->label->font());
    QString strElidedText = fontMetrics.elidedText(ui->label->text(), Qt::ElideRight, ui->label->width());
    ui->label->setText(strElidedText);*/

    ui->widget_PreviewBottomColor->hide();
    ui->label_NetworkValue->hide();
      
}

AFMainFrame::~AFMainFrame()
{
    _ClearAllStreamSignals();
    ClearSceneBottomButtons();

    m_DynamicCompositMainWindow = nullptr;
    m_outputHandlers.clear();
    m_ProgramGuideWidget->deleteLater();
    m_VolumeSliderFrame->deleteLater();
    m_MicSliderFrame->deleteLater();
    m_StatFrame->deleteLater();

    m_ProgramGuideWidget = nullptr;
    m_VolumeSliderFrame = nullptr;
    m_MicSliderFrame = nullptr;
    m_StatFrame = nullptr;

    m_SourceProperties = nullptr;
    m_transformPopup = nullptr;
    m_dialogFilters = nullptr;
    m_ScreenshotData = nullptr;
    m_programInfoDialog = nullptr;

    delete m_shortcutFilter;

    
    /* When shutting down, sometimes source references can get in to the
     * event queue, and if we don't forcibly process those events they
     * won't get processed until after obs_shutdown has been called.  I
     * really wish there were a more elegant way to deal with this via C++,
     * but Qt doesn't use C++ in a normal way, so you can't really rely on
     * normal C++ behavior for your data to be freed in the order that you
     * expect or want it to. */
    QApplication::sendPostedEvents(nullptr);
    
    delete ui;
}

void AFMainFrame::AFMainFrameInit(bool bShow)
{
    setWindowTitle("SOOP Studio");

    ui->stackedWidget_Platform->setCurrentIndex(0);
    // Register shortcuts for Undo/Redo
    m_undo_s.m_actionMainUndo->setShortcut(Qt::CTRL | Qt::Key_Z);
    m_undo_s.m_actionMainUndo->setShortcutContext(Qt::ApplicationShortcut);
    addAction(m_undo_s.m_actionMainUndo);
    connect(m_undo_s.m_actionMainUndo, &QAction::triggered, this, &AFMainFrame::qSlotUndo);

    QList<QKeySequence> shrt;
    shrt << QKeySequence((Qt::CTRL | Qt::SHIFT) | Qt::Key_Z)
        << QKeySequence(Qt::CTRL | Qt::Key_Y);
    m_undo_s.m_actionMainRedo->setShortcuts(shrt);
    m_undo_s.m_actionMainRedo->setShortcutContext(Qt::ApplicationShortcut);
    addAction(m_undo_s.m_actionMainRedo);
    connect(m_undo_s.m_actionMainRedo, &QAction::triggered, this, &AFMainFrame::qSlotRedo);


    setAttribute(Qt::WA_DeleteOnClose, true);

    setFocusPolicy(Qt::FocusPolicy::StrongFocus);

    const char* strGeometryConfig = config_get_string(GetGlobalConfig(),
        "BasicWindow", "geometry");
    // Restore Window
    QByteArray byteArray =
        QByteArray::fromBase64(QByteArray(strGeometryConfig));
    if (strGeometryConfig != NULL) {
        QByteArray byteArray =
            QByteArray::fromBase64(QByteArray(strGeometryConfig));
        restoreGeometry(byteArray);
        auto& graphicsContext = AFGraphicsContext::GetSingletonInstance();
        QRect rect = geometry();
        graphicsContext.SetMainPreviewY(rect.height());
        graphicsContext.SetMainPreviewCY(rect.y());

        QRect windowGeometry = normalGeometry();
        if (!WindowPositionValid(windowGeometry)) {
            QRect rect =
                QGuiApplication::primaryScreen()->geometry();
            setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                Qt::AlignCenter, size(),
                rect));
        }

        if (isMaximized())
            changeWidgetBorder(true);
    }
    else {
        /*QRect desktopRect =
            QGuiApplication::primaryScreen()->geometry();
        QSize adjSize = desktopRect.size() / 2 - size() / 2;*/
        this->resize(minimumSize());
    }

    if (!m_qTopMenu)
        _CreateTopMenu();

    currentScreen = App()->screenAt(this->mapToGlobal(rect().center()));

    m_DynamicCompositMainWindow = new AFMainDynamicComposit(this);

    m_DynamicCompositMainWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_DynamicCompositMainWindow, &AFMainDynamicComposit::qsignalBlockVisibleToggled,
        this, &AFMainFrame::qslotBlockToggleTriggered);
    connect(m_DynamicCompositMainWindow, &AFMainDynamicComposit::qsignalSelectSourcePopup,
        this, &AFMainFrame::qslotShowSelectSourcePopup);
    connect(m_DynamicCompositMainWindow, &AFMainDynamicComposit::qsignalShowPreview,
        this, &AFMainFrame::qslotTogglePreview);

    bool firstOpen = m_DynamicCompositMainWindow->MainWindowInit();

    ui->widget_Basic->layout()->addWidget(m_DynamicCompositMainWindow);

    if (firstOpen)
        qslotProgramGuideOpenTriggered();

    AFStatistics* statistics = App()->GetStatistics();
    if(statistics) {
        connect(statistics, &AFStatistics::qsignalCheckDiskSpaceRemaining,
                this, &AFMainFrame::qslotCheckDiskSpaceRemaining);
        connect(statistics, &AFStatistics::qsignalMemoryError,
                this, &AFMainFrame::qslotShowMemorySystemAlert);
        connect(statistics, &AFStatistics::qsignalCPUError,
            this, &AFMainFrame::qslotShowCPUSystemAlert);
        connect(statistics, &AFStatistics::qsignalNetworkError,
            this, &AFMainFrame::qslotShowNetworkSystemAlert);
        connect(statistics, &AFStatistics::qsignalNetworkState,
            this, &AFMainFrame::qslotNetworkState);
    }

    // Init Network State Icon
    qslotNetworkState(PCStatState::None);

    /*QVBoxLayout* qMainWindowLayout = new QVBoxLayout(this);

    qMainWindowLayout->setContentsMargins(0, 0, 0, 0);
    qMainWindowLayout->setSpacing(0);
    qMainWindowLayout->addWidget(m_DynamicCompositMainWindow);
    ui->widget_Basic->setLayout(qMainWindowLayout);*/

    ///
    SetButtons();
    
    m_outputHandlers.reserve(5);

    _SetupAccountUI();
    if (!m_qTopMenu)
    {
        _CreateTopMenu();
    }


    ResetOutputs();
    // init service
    if(!AFServiceManager::GetSingletonInstance().InitService()) {
        throw "Failed to initialize service";
    }

    ui->widget_BroadTimer->setVisible(false);
    ui->widget_RecordTimer->setVisible(false);
    ui->line_Time->setVisible(false);

    EnablePreviewDisplay(true);

    /*QList<QScreen*> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.length(); i++)
    {
        connect(screens.at(i), SIGNAL(physicalDotsPerInchChanged(qreal)), this, SLOT(qslotDpiChanged(qreal)));
    }*/
    //connect(window()->windowHandle(), SIGNAL(screenChanged(QScreen*)), this, SLOT(qslotScreenChanged(QScreen*))); 

    delete m_shortcutFilter;
    m_shortcutFilter = CreateShortcutFilter();
    installEventFilter(m_shortcutFilter);

    /* Show the main window, unless the tray icon isn't available
     * or neither the setting nor flag for starting minimized is set. */
    /*bool sysTrayEnabled = config_get_bool(configManager.GetGlobal(),
        "BasicWindow", "SysTrayEnabled");
    bool sysTrayWhenStarted = config_get_bool(configManager.GetGlobal(), 
        "BasicWindow", "SysTrayWhenStarted");
    bool hideWindowOnStart = QSystemTrayIcon::isSystemTrayAvailable() &&
        sysTrayEnabled &&
        (g_opt_minimize_tray || sysTrayWhenStarted);*/

#ifdef _WIN32
    SetWin32DropStyle(this);

    //if (!hideWindowOnStart)
    if(bShow)
        show();
#endif

    bool alwaysOnTop = config_get_bool(GetGlobalConfig(), "BasicWindow", "AlwaysOnTop");
    if (alwaysOnTop || g_opt_always_on_top)
    {
        SetAlwaysOnTop(this, true);
    }

    //System Tray Disable
    //SystemTray(true);
    _RegisterSourceControlAction();

    UpdateEditMenu();

    _SetResourceCheckTimer();
    connect(this, &AFMainFrame::qsignalRefreshTimerTick,
        this, &AFMainFrame::qslotRefreshNetworkText);

    ReloadCustomBrowserMenu();
}

void AFMainFrame::SetButtons()
{
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
    
    connect(this, &AFCQMainBaseWidget::qsignalBaseWindowMouseRelease,
            this, &AFMainFrame::qslotToggleBlockAreaByToggleButton);
    connect(this, &AFCQMainBaseWidget::qsignalBaseWindowMaximized,
            this, &AFMainFrame::qslotToggleBlockAreaByToggleButton);

    ////Top Menu Buttons
    connect(ui->pushButton_TopMenu, &QPushButton::clicked, this, &AFMainFrame::qslotTopMenuClicked);
    connect(ui->pushButton_RaisePopup, &QPushButton::clicked, this, &AFMainFrame::qslotPopupBlockClicked);
    ui->pushButton_RaisePopup->setToolTip(QT_UTF8(locale.Str("Basic.Main.RaisePopup")));
    connect(ui->pushButton_MinimumWindow, &QPushButton::clicked, this, &AFMainFrame::qslotMinimizeWindow);
    connect(ui->pushButton_MaximumWindow, &QPushButton::clicked, this, &AFMainFrame::qslotMaximizeWindow);
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFMainFrame::qslotCloseAllPopup);
    ////Top Menu Buttons

    connect(ui->frame_Top, &MainViewFrame::qsignalDoubleClicked, this, &AFMainFrame::qslotMaximizeWindow);

    //Bottom Menu Buttons
    //ui->pushButton_ShortcutSettings->setWhatsThis("4");
    //connect(ui->pushButton_ShortcutSettings, &QPushButton::clicked,
    //    this, &AFMainFrame::qslotShowStudioSettingWithButtonSender);

    connect(ui->pushButton_Broad, &QPushButton::clicked, this, &AFMainFrame::qslotChangeBroadState);
    connect(ui->pushButton_Broad, &AFQCustomPushbutton::qsignalButtonEnter, this, &AFMainFrame::qslotEnterBroadButton);
    connect(ui->pushButton_Broad, &AFQCustomPushbutton::qsignalButtonLeave, this, &AFMainFrame::qslotLeaveBroadButton);
    connect(ui->pushButton_Record, &QPushButton::clicked, this, &AFMainFrame::qslotChangeRecordState);
    

    m_VolumeSliderFrame = new AFQSliderFrame(this);
    //m_VolumeSliderFrame->InitSliderFrame("assets/mainview/mousehover/soundvolume.svg", true, 4096, 4096);
    m_VolumeSliderFrame->InitSliderFrame("assets/mainview/default/soundvolume.svg", true, 4096, 4096);
    connect(m_VolumeSliderFrame, &AFQSliderFrame::qsignalMouseLeave, 
        this, &AFMainFrame::qSlotCloseVolumeSlider);
    connect(m_VolumeSliderFrame, &AFQSliderFrame::qsignalMouseEnterSlider, 
        this, &AFMainFrame::qslotStopVolumeTimer);
    connect(m_VolumeSliderFrame, &AFQSliderFrame::qsignalVolumeChanged,
        this, &AFMainFrame::qslotMainAudioValueChanged);

    m_MicSliderFrame = new AFQSliderFrame(this);
    m_MicSliderFrame->setWindowFlag(Qt::WindowStaysOnTopHint);
    //m_MicSliderFrame->InitSliderFrame("assets/mainview/mousehover/micvolume.svg", true, 4096, 4096);
    m_MicSliderFrame->InitSliderFrame("assets/mainview/default/micvolume.svg", true, 4096, 4096);
    connect(m_MicSliderFrame, &AFQSliderFrame::qsignalMouseLeave, 
        this, &AFMainFrame::qSlotCloseMicSlider);
    connect(m_MicSliderFrame, &AFQSliderFrame::qsignalMouseEnterSlider,
        this, &AFMainFrame::qslotStopMicTimer);
    connect(m_MicSliderFrame, &AFQSliderFrame::qsignalVolumeChanged,
        this, &AFMainFrame::qslotMainMicValueChanged);

    connect(ui->widget_Volume, &AFQHoverWidget::qsignalHoverEnter, this, &AFMainFrame::qslotShowVolumeSlider);
    connect(ui->widget_Mic, &AFQHoverWidget::qsignalHoverEnter, this, &AFMainFrame::qslotShowMicSlider);
    connect(m_VolumeSliderFrame, &AFQSliderFrame::qsignalMuteButtonClicked, this, &AFMainFrame::qslotSetVolumeMute);
    connect(m_MicSliderFrame, &AFQSliderFrame::qsignalMuteButtonClicked, this, &AFMainFrame::qslotSetMicMute);
    
    App()->GetMainView()->GetMainWindow()->qslotSetMainAudioSource();

    connect(ui->pushButton_ShortcutSettings, &QPushButton::clicked, this, &AFMainFrame::qslotShowStudioSettingPopup);

    connect(ui->widget_ResourceNetwork, &AFQHoverWidget::qsignalMouseClick, this, &AFMainFrame::qslotExtendResource);

    AFStatistics::InitializeValues();

    //Bottom Menu Buttons
}

#ifdef _WIN32
static inline void UpdateProcessPriority()
{
    const char* priority = config_get_string(AFConfigManager::GetSingletonInstance().GetGlobal(), "General",
        "ProcessPriority");
    if (priority && strcmp(priority, "Normal") != 0)
        SetProcessPriority(priority);
}

static inline void ClearProcessPriority()
{
    const char* priority = config_get_string(AFConfigManager::GetSingletonInstance().GetGlobal(), "General",
        "ProcessPriority");
    if (priority && strcmp(priority, "Normal") != 0)
        SetProcessPriority("Normal");
}
#else
#define UpdateProcessPriority() \
	do {                    \
	} while (false)
#define ClearProcessPriority() \
	do {                   \
	} while (false)
#endif

void AFMainFrame::OnActivate(bool force)
{
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
    AFQCustomMenu* profileMenu = _FindSubMenuByTitle(m_qTopMenu,
                                             QT_UTF8(locale.Str("Basic.MainMenu.ProfileFix")));

    if (profileMenu->isEnabled() || force) {
        profileMenu->setEnabled(false);
        //ui->autoConfigure->setEnabled(false);
        AFInhibitSleepContext::GetSingletonInstance().IncrementSleepInhibition();
        UpdateProcessPriority();

        TaskbarOverlaySetStatus(TaskbarOverlayStatusActive);

        //SystemTray Disable
        /*if (trayIcon && trayIcon->isVisible()) {
#ifdef __APPLE__
            QIcon trayMask =
                QIcon(":/res/images/tray_active_macos.svg");
            trayMask.setIsMask(true);
            trayIcon->setIcon(
                QIcon::fromTheme("obs-tray", trayMask));
#else
            trayIcon->setIcon(QIcon::fromTheme(
                "obs-tray-active",
                QIcon(":/res/images/tray_active.png")));
#endif
        }*/
    }
}

void AFMainFrame::OnDeactivate()
{
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
    AFQCustomMenu* profileMenu = _FindSubMenuByTitle(m_qTopMenu,
                                             QT_UTF8(locale.Str("Basic.MainMenu.ProfileFix")));

    if (!IsActive() && !profileMenu->isEnabled()) {
        profileMenu->setEnabled(true);
        //ui->autoConfigure->setEnabled(true);
        AFInhibitSleepContext::GetSingletonInstance().DecrementSleepInhibition();
        ClearProcessPriority();

        TaskbarOverlaySetStatus(TaskbarOverlayStatusInactive);

        //SystemTray Disable
        /*if (trayIcon && trayIcon->isVisible()) {
#ifdef __APPLE__
            QIcon trayIconFile =
                QIcon(":/res/images/obs_macos.svg");
            trayIconFile.setIsMask(true);
#else
            QIcon trayIconFile = QIcon(":/res/images/obs.png");
#endif
            trayIcon->setIcon(
                QIcon::fromTheme("obs-tray", trayIconFile));
        }
    }
    else if (outputHandler->Active() && trayIcon &&
        trayIcon->isVisible()) {
        if (os_atomic_load_bool(&recording_paused)) {
#ifdef __APPLE__
            QIcon trayIconFile =
                QIcon(":/res/images/obs_paused_macos.svg");
            trayIconFile.setIsMask(true);
#else
            QIcon trayIconFile =
                QIcon(":/res/images/obs_paused.png");
#endif
            trayIcon->setIcon(QIcon::fromTheme("obs-tray-paused",
                trayIconFile));
            TaskbarOverlaySetStatus(TaskbarOverlayStatusPaused);
        }
        else {
#ifdef __APPLE__
            QIcon trayIconFile =
                QIcon(":/res/images/tray_active_macos.svg");
            trayIconFile.setIsMask(true);
#else
            QIcon trayIconFile =
                QIcon(":/res/images/tray_active.png");
#endif
            trayIcon->setIcon(QIcon::fromTheme("obs-tray-active",
                trayIconFile));
            TaskbarOverlaySetStatus(TaskbarOverlayStatusActive);
        }*/
    }
}

void AFMainFrame::qslotSaveProject()
{
    auto& loaderSaver = AFLoadSaveManager::GetSingletonInstance();
    
    if (loaderSaver.CheckCanSaveProject())
        QMetaObject::invokeMethod(this, "qslotSaveProjectDeferred",
                                  Qt::QueuedConnection);
}

void AFMainFrame::qslotSaveProjectDeferred()
{
    AFLoadSaveManager::GetSingletonInstance().SaveProjectDeferred();
}

void AFMainFrame::CreatePropertiesPopup(obs_source_t* source)
{
    bool closed = true;
    if (m_SourceProperties)
        closed = m_SourceProperties->close();

    if (!closed)
        return;

    m_SourceProperties = new AFQSourceProperties(this, source);
    m_SourceProperties->setAttribute(Qt::WA_DeleteOnClose, true);
    m_SourceProperties->setModal(false);

    setCenterPositionNotUseParent(m_SourceProperties, this);

    m_SourceProperties->show();
}
void AFMainFrame::CreatePropertiesWindow(obs_source_t* source)
{
    bool closed = true;
    if(m_SourceProperties)
        closed = m_SourceProperties->close();

    if(!closed)
        return;

    m_SourceProperties = new AFQSourceProperties(this, source);
    m_SourceProperties->show();
    m_SourceProperties->setAttribute(Qt::WA_DeleteOnClose, true);
}

void AFMainFrame::RecvRemovedSource(OBSSceneItem item)
{
    if (!item)
        return;

    // Check Hide Properties
    if (m_SourceProperties && m_SourceProperties->isVisible()) {

        OBSSource source = obs_sceneitem_get_source(item);
        if (m_SourceProperties->GetOBSSource() == source) {
            m_SourceProperties->close();
        }
    }

    // Check Hide Source Context Popup
    AFMainDynamicComposit* dynamicComposit = GetMainWindow();
    if (!dynamicComposit)
        return;

    OBSSource source = obs_sceneitem_get_source(item);
    dynamicComposit->HideSourceContextPopupFromRemoveSource(source);
    dynamicComposit->HideBrowserInteractionPopupFromRemoveSource(source);
}

void AFMainFrame::ShowSceneSourceSelectList()
{
    AFQSceneSelectDialog dlg(this);
    if (QDialog::Accepted != dlg.exec())
        return;

    AFSourceUtil::AddExistingSource(QT_TO_UTF8(dlg.m_sourceName), true, false, nullptr,
        nullptr, nullptr, nullptr);
}
//

void AFMainFrame::ShowSystemAlert(QString channelID, QString alertText)
{
    bool isMainMinimized = this->isMinimized();
    AFQSystemAlert* systemAlert = new AFQSystemAlert(nullptr, 
                                                    channelID, 
                                                    alertText, 
                                                    isMainMinimized,
                                                    this->width());
    systemAlert->show();
    _MoveSystemAlert(systemAlert, isMainMinimized);
}

void AFMainFrame::ResetOutputs()
{
    const char* mode = config_get_string(GetBasicConfig(), "Output", "Mode");
    bool advOut = astrcmpi(mode, "Advanced") == 0;

#define MAX_OUTPUT_STREAM 5
    if (MAX_OUTPUT_STREAM != m_outputHandlers.size()) {
        for (int i = 0; i < MAX_OUTPUT_STREAM; ++i) {
            OBSService service = nullptr;
            m_outputHandlers.emplace_back(service, 
                                          advOut ? CreateAdvancedOutputHandler(this) : 
                                                   CreateSimpleOutputHandler(this));
        }        
    }
    else {
        OUTPUT_HANDLER_LIST::iterator outputIter;
        for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
            if (!outputIter->second->Active()) {
                outputIter->second.reset(advOut ? CreateAdvancedOutputHandler(this) : 
                                                  CreateSimpleOutputHandler(this));
            }
            else {
                outputIter->second->Update();
            }
        }
    }

    OUTPUT_HANDLER_LIST::iterator outputIter = m_outputHandlers.begin();
    m_statusbar.SetOutputHandler(outputIter->second.get());
    App()->GetStatistics()->SetOutputHandler(outputIter->second.get());

    // Register Hotkey
    AFHotkeyContext& hotkey = AFHotkeyContext::GetSingletonInstance();

    bool useReplayBuffer = false;
    if (advOut)
        useReplayBuffer = config_get_bool(GetBasicConfig(), "AdvOut", "RecRB");
    else
        useReplayBuffer = config_get_bool(GetBasicConfig(), "SimpleOutput", "RecRB");

    if (useReplayBuffer)
        hotkey.RegisterHotkeyReplayBufferSave(m_outputHandlers[0].second->replayBuffer);
    else
        hotkey.UnRegisterHotkeyReplayBufferSave();
}

void AFMainFrame::SetStreamingOutput()
{
    int handlerIdx = 0;
    auto & authManager = AFAuthManager::GetSingletonInstance();

    AFChannelData* mainChannel = nullptr;
    if (authManager.GetMainChannelData(mainChannel))
        if (mainChannel != nullptr 
            && !mainChannel->bIsStreaming)
        {
            if (IsStreamActive()) {
                StopStreamingOutput((obs_service_t*)mainChannel->pObjOBSService);
            }
        }
     
    int cntOfAccount = authManager.GetCntChannel();
    for (int idx = 0; idx < cntOfAccount; idx++)
    {
        AFChannelData* tmpChannel = nullptr;
        authManager.GetChannelData(idx, tmpChannel);
        if (!tmpChannel)
            continue;

        if (tmpChannel->bIsStreaming)
        {
            /*
            if (!IsStartStreamingOutput((obs_service_t*)tmpChannel->pObjOBSService)) {
                OBSDataAutoRelease settingData = obs_data_create();
                obs_data_set_bool(settingData, "bwtest", false);
                obs_data_set_string(settingData, "key", tmpChannel->pAuthData->strKeyRTMP.c_str());
                obs_data_set_string(settingData, "server", tmpChannel->pAuthData->strUrlRTMP.c_str());
                obs_data_set_bool(settingData, "use_auth", false);
                // set service
                obs_service_t* obsService = obs_service_create("rtmp_common", "default_service", settingData, nullptr);
                tmpChannel->pObjOBSService = obsService;
            }
            if (os_atomic_load_bool(&m_streaming_active)) {
                if (PrepareStreamingOutput(-1, (obs_service_t*)tmpChannel->pObjOBSService)) {
                    if (!StartStreamingOutput((obs_service_t*)tmpChannel->pObjOBSService)) {
                        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                            QTStr("Output.Streaming.Failed"),
                            QTStr("Output.StartStreaming.Failed"));
                    }
                }
                handlerIdx++;
                if (tmpChannel->pObjOBSService != m_outputHandlers[handlerIdx].first) {
                    OUTPUT_HANDLER_LIST::iterator outputIter;
                    for (outputIter = m_outputHandlers.begin() + 1; outputIter != m_outputHandlers.end(); ++outputIter) {
                        if (tmpChannel->pObjOBSService == outputIter->first) {
                            std::iter_swap(outputIter, m_outputHandlers.begin() + handlerIdx);
                        }
                    }
                }
            }*/
        }
        else {
            if (IsStreamActive()) {
                StopStreamingOutput((obs_service_t*)tmpChannel->pObjOBSService);
            }
        }
    }


}

bool AFMainFrame::PrepareStreamingOutput(int index, obs_service_t* service)
{
    if (!service)
        return false;

    if (IsStartStreamingOutput(service))
        return  false;

    if (-1 == index) {
        OUTPUT_HANDLER_LIST::iterator outputIter;
        for (outputIter = m_outputHandlers.begin()+1; outputIter != m_outputHandlers.end(); ++outputIter) {
            if (!outputIter->first) {
                index = std::distance(m_outputHandlers.begin(), outputIter);
                break;
            }            
        }
    }

    if (m_outputHandlers[index].first && m_outputHandlers[index].second->StreamingActive())
        return false;

    m_outputHandlers[index].first = service;
    if (!m_outputHandlers[index].second->SetupStreaming(m_outputHandlers[index].first)) {
        m_outputHandlers[index].first = nullptr;
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
            QTStr("Output.Streaming.Failed"),
            QTStr("Output.SetupStreaming.Failed"));
        return false;
    }
    return true;
}

bool AFMainFrame::StartStreamingOutput(obs_service_t* service)
{
    if (!service)
        return false;

    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (service == outputIter->first) {
            if (!outputIter->second->StartStreaming(outputIter->first)) {
                return false;
            }
            return true;
        }
    }    
    return false;
}

bool AFMainFrame::IsStartStreamingOutput(obs_service_t* service)
{
    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (service == outputIter->first) {
            if (outputIter->second->StreamingActive())
                return true;
        }
    }    
    return false;
}

bool AFMainFrame::StopStreamingOutput(obs_service_t* service)
{
    OUTPUT_HANDLER_LIST::iterator outputIter;
    bool currentStreamOutputStopped = false;

    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (service == outputIter->first) {
            if (m_statusbar.GetOutputHandler() == outputIter->second.get())
                currentStreamOutputStopped = true;

            outputIter->second->StopStreaming(true);
            obs_service_release(outputIter->first);
            outputIter->first = nullptr;

            if (currentStreamOutputStopped)
                SetOutputHandler();
            return true;
        }
    }
    return false;
}

void AFMainFrame::SetOutputHandler()
{
    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter)
    {
        if (outputIter->first != nullptr) {
            App()->GetStatistics()->SetOutputHandler(outputIter->second.get());
            m_statusbar.SetOutputHandler(outputIter->second.get());
            return;
        }
    }
}
//
void AFMainFrame::EnablePreviewDisplay(bool enable)
{
    AFBasicPreview* preview = GetMainWindow()->GetMainPreview();
    obs_display_set_enabled(preview->GetDisplay(), enable);
    preview->setVisible(enable);
    GetMainWindow()->GetNotPreviewFrame()->setVisible(!enable);
}

void AFMainFrame::RefreshSceneUI()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    SceneItemVector& sceneItems = sceneContext.GetSceneItemVector();
    if (sceneItems.empty())
        return;

    clearLayout(ui->sceneButtonLayout);

    ClearSceneBottomButtons();

    size_t sceneCount = sceneItems.size();
    size_t visibleCount = sceneCount;
    if (visibleCount > 4)
        visibleCount = 4;

	for (size_t i = 0; i < visibleCount; i++) {
		OBSScene scene = sceneItems.at(i)->GetScene();
		int index = sceneItems.at(i)->GetSceneIndex();
		const char* name = sceneItems.at(i)->GetSceneName();

		AFQSceneBottomButton* sceneButton = new AFQSceneBottomButton(this, scene, index, name);
        sceneButton->setObjectName("AFQSceneBottomButton");

        bool selected = (sceneContext.GetCurrOBSScene() == scene);
        sceneButton->SetSelectedState(selected);

		connect(sceneButton, &AFQSceneBottomButton::qsignalSceneButtonClicked,
			    this, &AFMainFrame::qslotSceneButtonClicked);
        connect(sceneButton, &AFQSceneBottomButton::qsignalSceneButtonDoubleClicked,
            this, &AFMainFrame::qslotSceneButtonDoubleClicked);

		ui->sceneButtonLayout->addWidget(sceneButton);

        m_vSceneButton.emplace_back(sceneButton);
	}

    //if (sceneCount > 4) {
        std::string absPath;
        GetDataFilePath("assets", absPath);

        QString iconPath = QString("%1/mainview/scene-button-dot.svg").
                           arg(absPath.data());
        QIcon icon = QIcon(iconPath);

        //QString styleSheet = 
        //            QString("border-image:url('%1/mainview/scene-button-dot.svg') 0 0 2 0 stretch;")
        //            .arg(absPath.data());
        QPushButton* dotButton = new QPushButton(this);
        dotButton->setObjectName("bottomSceneDotButton");
        dotButton->setFixedSize(29,30);
        dotButton->setIcon(icon);
        dotButton->setIconSize(QSize(14, 17));
        //dotButton->setStyleSheet(styleSheet);

        connect(dotButton, &QPushButton::clicked, 
                this, &AFMainFrame::qslotSceneButtonDotClicked);

        ui->sceneButtonLayout->addWidget(dotButton);
    //}
       
    AFQProjector::UpdateMultiviewProjectors();
}

void AFMainFrame::CreateSourcePopupMenu(int idx, bool preview)
{
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

    AFQCustomMenu popup(this);
    AFQCustomMenu order(locale.Str("Basic.MainMenu.Edit.Order"), this, true);
    AFQCustomMenu colorMenu(locale.Str("ChangeBG"), this, true);
    AFQCustomMenu transform(locale.Str("Basic.MainMenu.Edit.Transform"), this, true);

    if (preview) {
        QAction* action = popup.addAction(Str("Basic.Main.PreviewConextMenu.Enable"),
                                          this, &AFMainFrame::qslotTogglePreview);
        action->setCheckable(true);
        action->setChecked(
            obs_display_enabled(GetMainWindow()->GetMainPreview()->GetDisplay()));
        if (0 /*IsPreviewProgramMode()*/)
            action->setEnabled(false);

        popup.addAction(ui->action_LockPreview);

        AFQCustomMenu* scalingMenu = new AFQCustomMenu(this, true);
        scalingMenu->setFixedWidth(200);

        {
            obs_video_info ovi;
            obs_get_video_info(&ovi);

            QAction *action = ui->action_ScaleCanvas;
            QString text = QTStr("Basic.MainMenu.Edit.Scale.Canvas");
            text = text.arg(QString::number(ovi.base_width),
                    QString::number(ovi.base_height));
            action->setText(text);

            action = ui->action_ScaleOutput;
            text = QTStr("Basic.MainMenu.Edit.Scale.Output");
            text = text.arg(QString::number(ovi.output_width),
                    QString::number(ovi.output_height));
            action->setText(text);
            action->setVisible(!(ovi.output_width == ovi.base_width &&
                         ovi.output_height == ovi.base_height));
            
            {
                bool fixedScaling = GetMainWindow()->GetMainPreview()->IsFixedScaling();
                float scalingAmount = GetMainWindow()->GetMainPreview()->GetScalingAmount();
                if (!fixedScaling)
                {
                    ui->action_ScaleWindow->setChecked(true);
                    ui->action_ScaleCanvas->setChecked(false);
                    ui->action_ScaleOutput->setChecked(false);
                }
                else
                {
                    obs_video_info ovi;
                    obs_get_video_info(&ovi);
                    
                    ui->action_ScaleWindow->setChecked(false);
                    ui->action_ScaleCanvas->setChecked(scalingAmount == 1.0f);
                    ui->action_ScaleOutput->setChecked(scalingAmount ==
                                                      float(ovi.output_width) /
                                                      float(ovi.base_width));
                }
            }
        }
        
        scalingMenu->addAction(ui->action_ScaleWindow);
        scalingMenu->addAction(ui->action_ScaleCanvas);
        scalingMenu->addAction(ui->action_ScaleOutput);

        connect(ui->action_ScaleWindow, &QAction::triggered, this, &AFMainFrame::qslotActionScaleWindow);
        connect(ui->action_ScaleCanvas, &QAction::triggered, this, &AFMainFrame::qslotActionScaleCanvas);
        connect(ui->action_ScaleOutput, &QAction::triggered, this, &AFMainFrame::qslotActionScaleOutput);
    
        popup.addMenu(scalingMenu)->setText(QT_UTF8(locale.Str("Basic.MainMenu.Edit.Scale")));;
        //
        
        popup.addSeparator();

        popup.addAction(Str("AddSource"), this, &AFMainFrame::qslotShowSelectSourcePopup);
    }
    else {
        QPointer<AFQCustomMenu> addSourceMenu = CreateAddSourcePopupMenu();
        if (addSourceMenu)
            popup.addMenu(addSourceMenu);
    }

    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    AFQSourceListView* sourceListView = sceneContext.GetSourceListViewPtr();

    bool mulitple = true;
    if (sourceListView->MultipleBaseSelected()) {
        popup.addSeparator();
        popup.addAction(locale.Str("Basic.Main.GroupItems"), sourceListView,
            &AFQSourceListView::GroupSelectedItems);
    }
    else if (sourceListView->GroupsSelected()) {
        popup.addSeparator();
        popup.addAction(locale.Str("Basic.Main.Ungroup"), sourceListView,
            &AFQSourceListView::UngroupSelectedGroups);
    }

    popup.addSeparator();
    popup.addAction(ui->action_CopySource);
    popup.addAction(ui->action_PasteSourceRef);
    popup.addAction(ui->action_PasteSourceDuplicate);
    popup.addSeparator();

	popup.addSeparator();
    if (idx != -1) {
        popup.addAction(ui->action_Filters);
    }
	popup.addAction(ui->action_CopyFilters);
	popup.addAction(ui->action_PasteFilters);
	popup.addSeparator();

    if (idx != -1)
    {
        OBSSceneItem sceneItem = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem(idx);
        if (!sceneItem)
            return;

        obs_source_t* source = obs_sceneitem_get_source(sceneItem);
        if (!source)
            return;

        uint32_t flags = obs_source_get_output_flags(source);
        bool isAsyncVideo = (flags & OBS_SOURCE_ASYNC_VIDEO) ==
            OBS_SOURCE_ASYNC_VIDEO;
        bool hasAudio = (flags & OBS_SOURCE_AUDIO) == OBS_SOURCE_AUDIO;
        bool hasVideo = (flags & OBS_SOURCE_VIDEO) == OBS_SOURCE_VIDEO;

        popup.addSeparator();
        {
            m_widgetActionColor = new QWidgetAction(&colorMenu);
            m_widgetColorSelect = new AFQColorSelect(&colorMenu);

            popup.addMenu(AddBackgroundColorMenu(
                &colorMenu, m_widgetActionColor, m_widgetColorSelect, sceneItem));
            popup.addAction(ui->action_RenameSource);
            popup.addAction(ui->action_RemoveSource);
        }
        popup.addSeparator();

        popup.addSeparator();
        {
            order.addAction(ui->action_OrderMoveUp);
            order.addAction(ui->action_OrderMoveDown);
            order.addAction(ui->action_OrderMoveToTop);
            order.addAction(ui->action_OrderMoveToBottom);
        }
        popup.addMenu(&order);
        popup.addSeparator();

        if (hasVideo) {
            popup.addSeparator();
            transform.addAction(ui->action_EditTransform);
            transform.addAction(ui->action_CopyTransform);
            transform.addAction(ui->action_PasteTransform);
            transform.addAction(ui->action_ResetTransform);
            transform.addSeparator();
            transform.addAction(ui->action_Rotate90CW);
            transform.addAction(ui->action_Rotate90CCW);
            transform.addAction(ui->action_Rotate180);
            transform.addSeparator();
            transform.addAction(ui->action_FlipHorizontal);
            transform.addAction(ui->action_FlipVertical);
            transform.addSeparator();
            transform.addAction(ui->action_FitToScreen);
            transform.addAction(ui->action_StretchToScreen);
            transform.addAction(ui->action_CenterToScreen);
            transform.addAction(ui->action_VerticalCenter);
            transform.addAction(ui->action_HorizontalCenter);
            popup.addMenu(&transform);
            popup.addSeparator();
        }

        if (hasAudio) {
            //QAction* actionHideMixer =
            //    popup.addAction(QTStr("HideMixer"), this,
            //        &OBSBasic::ToggleHideMixer);
            //actionHideMixer->setCheckable(true);
            //actionHideMixer->setChecked(SourceMixerHidden(source));
            //popup.addSeparator();
        }

        if (hasVideo) {
            QAction* resizeOutput = popup.addAction(Str("ResizeOutputSizeOfSource"), 
                                                    this, &AFMainFrame::qSlotResizeOutputSizeOfSource);

            int width = obs_source_get_width(source);
            int height = obs_source_get_height(source);

            resizeOutput->setEnabled(!obs_video_active());

            if (width < 32 || height < 32)
                resizeOutput->setEnabled(false);

            m_menuScaleFiltering = new AFQCustomMenu(Str("ScaleFiltering"), this, true);
            popup.addMenu(_AddScaleFilteringMenu(m_menuScaleFiltering, sceneItem));

            m_menuBlendingMode = new AFQCustomMenu(Str("BlendingMode"), this, true);
            popup.addMenu(_AddBlendingModeMenu(m_menuBlendingMode, sceneItem));

            m_menuBlendingMethodMode = new AFQCustomMenu(Str("BlendingMethod"), this, true);
            popup.addMenu(_AddBlendingMethodMenu(m_menuBlendingMethodMode, sceneItem));


            if (isAsyncVideo) {
                m_menuDeinterlace = new AFQCustomMenu(Str("Deinterlacing"), this, true);
                popup.addMenu(_AddDeinterlacingMenu(m_menuDeinterlace, source));
            }

            popup.addSeparator();

            //popup.addMenu(CreateVisibilityTransitionMenu(true));
            //popup.addMenu(CreateVisibilityTransitionMenu(false));
            //popup.addSeparator();

            //sourceProjector = new QMenu(QTStr("SourceProjector"));
            //AddProjectorMenuMonitors(
            //    sourceProjector, this,
            //    &OBSBasic::OpenSourceProjector);
            //popup.addMenu(sourceProjector);
            //popup.addAction(QTStr("SourceWindow"), this,
            //    &OBSBasic::OpenSourceWindow);

            //popup.addAction(QTStr("Screenshot.Source"), this,
            //    &OBSBasic::ScreenshotSelectedSource);
        }

        popup.addSeparator();
        {
            if (flags & OBS_SOURCE_INTERACTION)
                popup.addAction(ui->action_ShowInteract);

            popup.addAction(ui->action_ShowProperties);
            ui->action_ShowProperties->setEnabled(obs_source_configurable(source));
        }
        popup.addSeparator();
    }

	popup.exec(QCursor::pos());
}

bool CheckEnableInputSource(const char* id)
{
    size_t idx = 0;
    const char* unversioned_type;
    const char* type;

    while (obs_enum_input_types2(idx++, &type, &unversioned_type)) {
        const char* name = obs_source_get_display_name(type);
        uint32_t caps = obs_get_source_output_flags(type);

        if ((caps & OBS_SOURCE_CAP_DISABLED) != 0)
            continue;

        if ((caps & OBS_SOURCE_DEPRECATED) == 0) {
            return true;
        }
    }

    return false;
}

void AFMainFrame::AddSourceMenuButton(const char* id, QWidget* popup)
{
    AFIconContext& iconContext = AFIconContext::GetSingletonInstance();

    const char* name = obs_source_get_display_name(id);
    AFQAddSourceMenuButton* button = new AFQAddSourceMenuButton(id, popup);
    button->setIconSize(QSize(24, 24));
    button->setText(QT_UTF8(name));
    button->setObjectName("addSourceMenuButton");
    button->setIcon(iconContext.GetSourceIcon(id));

    connect(button, &QPushButton::clicked,
            this, &AFMainFrame::qslotAddSourceMenu);

    QWidgetAction* popupItem = new QWidgetAction(popup);
    popupItem->setDefaultWidget(button);
    popup->addAction(popupItem);
}

AFQCustomMenu* AFMainFrame::CreateAddSourcePopupMenu()
{
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
    AFIconContext& iconContext  = AFIconContext::GetSingletonInstance();

    AFQCustomMenu* popup = new AFQCustomMenu(locale.Str("Add"), this, true);
    AFQCustomMenu* deprecated = new AFQCustomMenu(locale.Str("Deprecated"), popup);

    popup->setStyleSheet("QMenu::item { height:24px;}       \
                          QPushButton { \
                          text-align:left; \
                          color:rgba(255, 255, 255, 80%); \
                          font-size: 14px; \
                          font-style: normal; \
                          font-weight: 400;   \
                          line-height: normal;    \
                          padding:5px; \
                          background-color:rgb(36, 39, 45); \
                          border-radius: 6px; }  \
                          QPushButton:hover{ background-color:rgba(255, 255, 255, 10%); }");

    // Source Type Info From CSelectSourceDailog.h 
    // [g_pszScreenSectionSource, g_pszAudioSectionSource, g_pszEtcSectionSource]
    size_t arrSize = sizeof(g_pszScreenSectionSource) / sizeof(g_pszScreenSectionSource[0]);
    for (int i = 0; i < arrSize; i++) {
        const char* sourceid = g_pszScreenSectionSource[i];
        if (CheckEnableInputSource(sourceid))
            AddSourceMenuButton(sourceid, popup);
    }

    arrSize = sizeof(g_pszAudioSectionSource) / sizeof(g_pszAudioSectionSource[0]);
    for (int i = 0; i < arrSize; i++) {
        const char* sourceid = g_pszAudioSectionSource[i];
        if (CheckEnableInputSource(sourceid))
            AddSourceMenuButton(sourceid, popup);
    }

    arrSize = sizeof(g_pszEtcSectionSource) / sizeof(g_pszEtcSectionSource[0]);
    for (int i = 0; i < arrSize; i++) {
        const char* sourceid = g_pszEtcSectionSource[i];
        if (CheckEnableInputSource(sourceid))
            AddSourceMenuButton(sourceid, popup);
    }

    // Add Scene Menu
    AFQAddSourceMenuButton* sceneButton = new AFQAddSourceMenuButton("scene", popup);
    sceneButton->setIconSize(QSize(24, 24));
    sceneButton->setText(QT_UTF8(locale.Str("Basic.Scene")));
    sceneButton->setObjectName("addSourceMenuButton");
    sceneButton->setIcon(iconContext.GetSceneIcon());
    connect(sceneButton, &QPushButton::clicked,
            this, &AFMainFrame::qslotAddSourceMenu);

    QWidgetAction* popupSceneItem = new QWidgetAction(popup);
    popupSceneItem->setDefaultWidget(sceneButton);
    popup->addAction(popupSceneItem);

    // Add Group Menu
    popup->addSeparator();
    AFQAddSourceMenuButton* groupButton = new AFQAddSourceMenuButton("group", popup);
    groupButton->setIconSize(QSize(24, 24));
    groupButton->setText(QT_UTF8(locale.Str("Group")));
    groupButton->setObjectName("addSourceMenuButton");
    groupButton->setIcon(iconContext.GetGroupIcon());

    connect(groupButton, &QPushButton::clicked,
            this, &AFMainFrame::qslotAddSourceMenu);

    QWidgetAction* popupGroupItem = new QWidgetAction(popup);
    popupGroupItem->setDefaultWidget(groupButton);
    popup->addAction(popupGroupItem);

    return popup;
}

AFQCustomMenu* AFMainFrame::AddBackgroundColorMenu(AFQCustomMenu* menu,
                                           QWidgetAction* widgetAction,
                                           AFQColorSelect* select,
                                           obs_sceneitem_t* item)
{
    QAction* action;

    menu->setStyleSheet(QString(
        "*[bgColor=\"1\"]{background-color:rgba(255,68,68,33%);}"
        "*[bgColor=\"2\"]{background-color:rgba(255,255,68,33%);}"
        "*[bgColor=\"3\"]{background-color:rgba(68,255,68,33%);}"
        "*[bgColor=\"4\"]{background-color:rgba(68,255,255,33%);}"
        "*[bgColor=\"5\"]{background-color:rgba(68,68,255,33%);}"
        "*[bgColor=\"6\"]{background-color:rgba(255,68,255,33%);}"
        "*[bgColor=\"7\"]{background-color:rgba(68,68,68,33%);}"
        "*[bgColor=\"8\"]{background-color:rgba(255,255,255,33%);}"));

    obs_data_t* privData = obs_sceneitem_get_private_settings(item);
    obs_data_release(privData);

    obs_data_set_default_int(privData, "color-preset", 0);
    int preset = obs_data_get_int(privData, "color-preset");

    action = menu->addAction(QTStr("Clear"), this, &AFMainFrame::qSlotSourceListItemColorChange);
    action->setCheckable(true);
    action->setProperty("bgColor", 0);
    action->setChecked(preset == 0);

    action = menu->addAction(QTStr("CustomColor"), this,
        &AFMainFrame::qSlotSourceListItemColorChange);
    action->setCheckable(true);
    action->setProperty("bgColor", 1);
    action->setChecked(preset == 1);

    menu->addSeparator();

    widgetAction->setDefaultWidget(select);

    for (int i = 1; i < 9; i++) {
        std::stringstream button;
        button << "preset" << i;

        std::stringstream buttonFrame;
        buttonFrame << "framePreset" << i;

        QFrame* colorButtonFrame =
            select->findChild<QFrame*>(buttonFrame.str().c_str());
        if (preset == i + 1)
            colorButtonFrame->setStyleSheet("QFrame { border: 1px solid #D9D9D9; border-radius:6px; }");

        QPushButton* colorButton =
            colorButtonFrame->findChild<QPushButton*>(button.str().c_str());

        colorButton->setProperty("bgColor", i);
        select->connect(colorButton, &QPushButton::released, this,
            &AFMainFrame::qSlotSourceListItemColorChange);
    }

    menu->addAction(widgetAction);

    return menu;
}

void AFMainFrame::CreateFiltersWindow(obs_source_t* source)
{
    bool closed = true;
    if (m_dialogFilters)
        closed = m_dialogFilters->close();

    if (!closed)
        return;

    m_dialogFilters = new AFQBasicFilters(nullptr, source);

    setCenterPositionNotUseParent(m_dialogFilters,this);

    m_dialogFilters->show();
    m_dialogFilters->setAttribute(Qt::WA_DeleteOnClose, true);
}

void AFMainFrame::CreateEditTransformPopup(obs_sceneitem_t* item)
{
    if (m_transformPopup)
        m_transformPopup->close();

    m_transformPopup = new AFQBasicTransform(item, this);
    //connect(ui->scenes, &QListWidget::currentItemChanged, transformWindow,
    //    &OBSBasicTransform::OnSceneChanged);

    setCenterPositionNotUseParent(m_transformPopup, this);

    m_transformPopup->show();
    m_transformPopup->setAttribute(Qt::WA_DeleteOnClose, true);
}

void AFMainFrame::SetSceneBottomButtonStyleSheet(OBSSource scene)
{
    std::vector<AFQSceneBottomButton*>::iterator iter = m_vSceneButton.begin();
    for (; iter != m_vSceneButton.end(); ++iter) {
        AFQSceneBottomButton* button = (*iter);
        if (!button || !button->GetObsScene())
            continue;

        const OBSSource source = OBSSource(obs_scene_get_source(button->GetObsScene()));

        button->SetSelectedState(source == scene);
    }
}

QAction* AFMainFrame::GetRemoveSourceAction()
{
    return ui->action_RemoveSource;
}

void AFMainFrame::UpdateEditMenu()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    AFQSourceListView* sourceListView = sceneContext.GetSourceListViewPtr();
    if (!sourceListView)
        return;

    QModelIndexList items = sourceListView->selectionModel()->selectedIndexes();
    int totalCount = items.count();
    size_t filter_count = 0;

    if (totalCount == 1) {
        OBSSceneItem sceneItem =
            sourceListView->Get(sourceListView->GetTopSelectedSourceItem());
        OBSSource source = obs_sceneitem_get_source(sceneItem);
        filter_count = obs_source_filter_count(source);
    }

    bool allowPastingDuplicate = !!m_clipboard.size();
    for (size_t i = m_clipboard.size(); i > 0; i--) {
        const size_t idx = i - 1;
        OBSWeakSource& weak = m_clipboard[idx].weak_source;
        if (obs_weak_source_expired(weak)) {
            m_clipboard.erase(m_clipboard.begin() + idx);
            continue;
        }
        OBSSourceAutoRelease strong =
            obs_weak_source_get_source(weak.Get());
        if (allowPastingDuplicate &&
            obs_source_get_output_flags(strong) &
            OBS_SOURCE_DO_NOT_DUPLICATE)
            allowPastingDuplicate = false;
    }

    int videoCount = 0;
    bool canTransformMultiple = false;
    for (int i = 0; i < totalCount; i++) {
        OBSSceneItem item = sourceListView->Get(items.value(i).row());
        OBSSource source = obs_sceneitem_get_source(item);
        const uint32_t flags = obs_source_get_output_flags(source);
        const bool hasVideo = (flags & OBS_SOURCE_VIDEO) != 0;
        if (hasVideo && !obs_sceneitem_locked(item))
            canTransformMultiple = true;

        if (hasVideo)
            videoCount++;
    }

    const bool canTransformSingle = videoCount == 1 && totalCount == 1;

    ui->action_CopySource->setEnabled(totalCount > 0);
    ui->action_EditTransform->setEnabled(canTransformSingle);
    ui->action_CopyTransform->setEnabled(canTransformSingle);
    ui->action_PasteTransform->setEnabled(m_hasCopiedTransform &&
        videoCount > 0);
    ui->action_CopyFilters->setEnabled(filter_count > 0);
    ui->action_PasteFilters->setEnabled(
        !obs_weak_source_expired(sceneContext.m_obsCopyFiltersSource) && totalCount > 0);
    ui->action_PasteSourceRef->setEnabled(!!m_clipboard.size());
    ui->action_PasteSourceDuplicate->setEnabled(allowPastingDuplicate);

    ui->action_OrderMoveUp->setEnabled(totalCount > 0);
    ui->action_OrderMoveDown->setEnabled(totalCount > 0);
    ui->action_OrderMoveToTop->setEnabled(totalCount > 0);
    ui->action_OrderMoveToBottom->setEnabled(totalCount > 0);

    ui->action_ResetTransform->setEnabled(canTransformMultiple);
    ui->action_Rotate90CW->setEnabled(canTransformMultiple);
    ui->action_Rotate90CCW->setEnabled(canTransformMultiple);
    ui->action_Rotate180->setEnabled(canTransformMultiple);
    ui->action_FlipHorizontal->setEnabled(canTransformMultiple);
    ui->action_FlipVertical->setEnabled(canTransformMultiple);
    ui->action_FitToScreen->setEnabled(canTransformMultiple);
    ui->action_StretchToScreen->setEnabled(canTransformMultiple);
    ui->action_CenterToScreen->setEnabled(canTransformMultiple);
    ui->action_VerticalCenter->setEnabled(canTransformMultiple);
    ui->action_HorizontalCenter->setEnabled(canTransformMultiple);
}


void AFMainFrame::Screenshot(OBSSource source)
{
    if (!!m_ScreenshotData) {
        blog(LOG_WARNING, "Cannot take new screenshot, "
                          "screenshot currently in progress");
        return;
    }

    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    m_ScreenshotData = new AFQScreenShotObj(
                       AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrOBSScene()));
}

void AFMainFrame::qslotTransitionScene()
{
    // exist : CProgramView.h -> void _qslotTransitionClicked();
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    AFSceneUtil::TransitionToScene(
        AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrOBSScene()));
}

void AFMainFrame::SetAudioButtonEnabled(bool enabled) {
    ui->widget_Volume->setEnabled(enabled);

    if (enabled) {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect();
        effect->setOpacity(1.0);
        ui->widget_Volume->setGraphicsEffect(effect);
    }
    else {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect();
        effect->setOpacity(0.3);
        ui->widget_Volume->setGraphicsEffect(effect);
    }
}

void AFMainFrame::SetMicButtonEnabled(bool enabled) {
    ui->widget_Mic->setEnabled(enabled);

    if (enabled) {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect();
        effect->setOpacity(1.0);
        ui->widget_Mic->setGraphicsEffect(effect);
    }
    else {
        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect();
        effect->setOpacity(0.3);
        ui->widget_Mic->setGraphicsEffect(effect);
    }
}

void AFMainFrame::SetAudioVolume(int volume)
{
    if (m_VolumeSliderFrame)
        m_VolumeSliderFrame->SetVolumeSize(volume);
}

void AFMainFrame::SetMicVolume(int volume)
{
    if (m_MicSliderFrame)
        m_MicSliderFrame->SetVolumeSize(volume);
}

void AFMainFrame::SetAudioSliderEnabled(bool Enabled) 
{
    if (m_VolumeSliderFrame)
        m_VolumeSliderFrame->SetVolumeSliderEnabled(Enabled);
}

void AFMainFrame::SetMicSliderEnabled(bool Enabled) 
{
    if (m_MicSliderFrame)
        m_MicSliderFrame->SetVolumeSliderEnabled(Enabled);
}

void AFMainFrame::SetAudioVolumeSliderSignalsBlock(bool block) 
{
    if (m_VolumeSliderFrame)
        m_VolumeSliderFrame->BlockSliderSignal(block);
}

void AFMainFrame::SetMicVolumeSliderSignalsBlock(bool block)
{
    if (m_MicSliderFrame)
        m_MicSliderFrame->BlockSliderSignal(block);
}

void AFMainFrame::SetAudioMeter(float peak)
{
    if (!m_VolumeSliderFrame)
        return;

    if(m_VolumeSliderFrame->isVisible())
        m_VolumeSliderFrame->SetVolumePeak(peak);
}

void AFMainFrame::SetMicMeter(float peak)
{
    if (!m_MicSliderFrame)
        return;

    if(m_MicSliderFrame->isVisible())
        m_MicSliderFrame->SetVolumePeak(peak);
}

void AFMainFrame::SetAudioButtonMute(bool bMute)
{
    if (m_VolumeSliderFrame)
    {
        const char* imagepath;
        if (bMute)
        {
            imagepath = "assets/mainview/default/soundvolume-mute.svg";
            m_VolumeSliderFrame->SetButtonProperty("soundVolumeMute");
        }
        else
        {
            imagepath = "assets/mainview/default/soundvolume.svg";
            m_VolumeSliderFrame->SetButtonProperty("soundVolume");
        }

        std::string absPath;
        GetDataFilePath(imagepath, absPath);
        QString qstrImgPath(absPath.data());
        QString style = "image: url(%1) 0 0 0 0 stretch stretch;";
        ui->widget_Volume->setStyleSheet(style.arg(qstrImgPath));
    }
}

void AFMainFrame::SetAudioSliderMute(bool bMute) {
    if (m_VolumeSliderFrame)
        m_VolumeSliderFrame->SetVolumeMuted(bMute);
}

void AFMainFrame::SetMicButtonMute(bool bMute)
{
    if (m_MicSliderFrame)
    {
        const char* imagepath;
        if (bMute)
        {
            imagepath = "assets/mainview/default/micvolume-mute.svg";
            m_MicSliderFrame->SetButtonProperty("micVolumeMute");
        }
        else
        {
            imagepath = "assets/mainview/default/micvolume.svg)";
            m_MicSliderFrame->SetButtonProperty("micVolume");
        }

        std::string absPath;
        GetDataFilePath(imagepath, absPath);
        QString qstrImgPath(absPath.data());
        QString style = "image: url(%1) 0 0 0 0 stretch stretch;";
        ui->widget_Mic->setStyleSheet(style.arg(qstrImgPath));
    }
}

void AFMainFrame::SetMicSliderMute(bool bMute) {
    if (m_MicSliderFrame)
        m_MicSliderFrame->SetVolumeMuted(bMute);
}

bool AFMainFrame::IsAudioMuted() 
{
    if (m_VolumeSliderFrame)
        return m_VolumeSliderFrame->IsVolumeMuted();
    return true;
}

bool AFMainFrame::IsMicMuted() 
{
    if (m_MicSliderFrame)
        return m_MicSliderFrame->IsVolumeMuted();
    return true;
}

void AFMainFrame::SystemTray(bool firstStarted)
{
    auto& configMan = AFConfigManager::GetSingletonInstance();
    auto& localeMan = AFLocaleTextManager::GetSingletonInstance();

    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;
    if (!m_TrayIcon && !firstStarted)
        return;

    bool sysTrayWhenStarted = config_get_bool(configMan.GetGlobal(),
        "BasicWindow", "SysTrayWhenStarted");
    bool sysTrayEnabled = config_get_bool(configMan.GetGlobal(), 
        "BasicWindow", "SysTrayEnabled");

    if (firstStarted)
        SystemTrayInit();

    if (!sysTrayEnabled) {
        m_TrayIcon->hide();
    }
    else {
        m_TrayIcon->show();
        if (firstStarted && (sysTrayWhenStarted || g_opt_minimize_tray)) {
            EnablePreviewDisplay(false);
#ifdef __APPLE__
            EnableOSXDockIcon(false);
#endif
            g_opt_minimize_tray = false;
        }
    }

    if (isVisible())
        m_qMainShowHideAction->setText(QT_UTF8(localeMan.Str(("Basic.SystemTray.Hide"))));
    else
        m_qMainShowHideAction->setText(QT_UTF8(localeMan.Str(("Basic.SystemTray.Show"))));
}

void AFMainFrame::SystemTrayInit()
{
    auto& localeMan = AFLocaleTextManager::GetSingletonInstance();

#ifdef __APPLE__
    QIcon trayIconFile = QIcon(":/image/resource/Platform/default/soop.png");
    trayIconFile.setIsMask(true);
#else
    QIcon trayIconFile = QIcon(":/image/resource/Platform/default/soop.png");
#endif
    m_TrayIcon.reset(new QSystemTrayIcon(
        QIcon::fromTheme("obs-tray", trayIconFile), this));
    m_TrayIcon->setToolTip("OBS Studio");

    m_qMainShowHideAction = new QAction(QT_UTF8(localeMan.Str("Basic.SystemTray.Show")), m_TrayIcon.data());
    m_qSystemTrayStreamAction = new QAction(QT_UTF8(localeMan.Str("Basic.Main.StartStreaming")));
    m_qSystemTrayRecordAction = new QAction(QT_UTF8(localeMan.Str("Basic.Main.StartRecording")));
    m_qExitAction = new QAction(QT_UTF8(localeMan.Str("Exit")), m_TrayIcon.data());

    m_qTrayMenu = new AFQCustomMenu(this);
    m_qTrayMenu->setFixedWidth(300);
    m_qPreviewProjector = new AFQCustomMenu(QT_UTF8(localeMan.Str("PreviewProjector")), m_qTrayMenu);
    m_qStudioProgramProjector = new AFQCustomMenu(QT_UTF8(localeMan.Str("StudioProgramProjector")), m_qTrayMenu);

    m_qTrayMenu->addAction(m_qMainShowHideAction);
    m_qTrayMenu->addSeparator();
    m_qTrayMenu->addMenu(m_qPreviewProjector);
    m_qTrayMenu->addMenu(m_qStudioProgramProjector);
    m_qTrayMenu->addSeparator();
    m_qTrayMenu->addAction(m_qSystemTrayStreamAction);
    m_qTrayMenu->addAction(m_qSystemTrayRecordAction);
    m_qTrayMenu->addAction(m_qSystemTrayReplayBufferAction);

    m_qTrayMenu->addSeparator();
    m_qTrayMenu->addAction(m_qExitAction);
    m_TrayIcon->setContextMenu(m_qTrayMenu);
    m_TrayIcon->show();


    connect(m_TrayIcon.data(), &QSystemTrayIcon::activated, this,
        &AFMainFrame::qslotIconActivated);
    connect(m_qMainShowHideAction, &QAction::triggered, this, &AFMainFrame::qslotToggleShowHide);
    connect(m_qSystemTrayStreamAction, &QAction::triggered, this,
        &AFMainFrame::qslotChangeBroadState);
    connect(m_qSystemTrayRecordAction, &QAction::triggered, this,
        &AFMainFrame::qslotChangeRecordState);
    /*connect(m_qSystemTrayReplayBufferAction, &QAction::triggered, this,
        &AFMainFrame::ReplayBufferClicked);*/
    /*connect(sysTrayVirtualCam.data(), &QAction::triggered, this,
        &OBSBasic::VCamButtonClicked);*/
    connect(m_qExitAction, &QAction::triggered, this, &AFMainFrame::close);
}

void AFMainFrame::HotkeyTriggered(void* data, obs_hotkey_id id, bool pressed)
{
    AFMainFrame& mainFrame = *static_cast<AFMainFrame*>(data);
    QMetaObject::invokeMethod(&mainFrame, "qslotProcessHotkey",
                              Q_ARG(obs_hotkey_id, id),
                              Q_ARG(bool, pressed));
}

void AFMainFrame::SetPCStateIconStyle(QLabel* label, PCStatState state)
{
    std::string absPath;
    const char* imagePath;

    if (state == PCStatState::None)
        imagePath = "assets/stat/normal-ellipse.svg";
    else if (state == PCStatState::Normal)
        imagePath = "assets/stat/green-ellipse.svg";
    //else if (state == PCStatState::Warning) 
    //    imagePath = "assets/stat/normal-ellipse.svg";
    else // Error
        imagePath = "assets/stat/red-ellipse.svg";

    GetDataFilePath(imagePath, absPath);
    QString qstrImgPath(absPath.data());
    QString style = "QLabel {image: url(%1);}";

    label->setStyleSheet(style.arg(qstrImgPath));
}

#include "CoreModel/OBSOutput/COBSOutputContext.h"
bool AFMainFrame::IsActive()
{
    bool isActive = false;
    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (outputIter->second->Active()) {
            isActive = true;
            break;
        }
    }
    return isActive;
}

bool AFMainFrame::IsStreamActive() 
{
    bool isActive = false;
    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (outputIter->first && outputIter->second->StreamingActive()) {
            isActive = true;
            break;
        }
    }
    return isActive;
}

bool AFMainFrame::IsReplayBufferActive()
{
    if (m_outputHandlers[0].second && m_outputHandlers[0].second->ReplayBufferActive())
        return true;
    return false;
}

bool AFMainFrame::IsRecordingActive()
{
    if (m_outputHandlers[0].second && m_outputHandlers[0].second->RecordingActive())
        return true;
    return false;
}

bool AFMainFrame::EnableStartStreaming()
{
    return (!IsStreamActive() && ui->pushButton_Broad->isEnabled());
}

bool AFMainFrame::IsPreviewProgramMode()
{
    auto& configManager = AFConfigManager::GetSingletonInstance();
    return configManager.GetStates()->IsPreviewProgramMode();
}

bool AFMainFrame::EnableStopStreaming()
{
    //return (IsStreamActive() && ui->pushButton_Broad->isEnabled());
    return true;
}

bool AFMainFrame::EnableStartRecording()
{
    return (!IsRecordingActive() && ui->pushButton_Record->isEnabled());
}

bool AFMainFrame::EnableStopRecording()
{
    return (IsRecordingActive() && ui->pushButton_Record->isEnabled());
}

bool AFMainFrame::EnablePauseRecording()
{
    return true;
}

bool AFMainFrame::EnableUnPauseRecording()
{
    return true;
}

void AFMainFrame::StartStreaming()
{
    qslotChangeBroadState(true);
}

void AFMainFrame::StopStreaming()
{
    qslotChangeBroadState(false);
}

void AFMainFrame::ForceStopStreaming()
{
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
        this, QT_UTF8("방송 종료"), QT_UTF8("방송 종료"));
}

void AFMainFrame::StartRecording()
{
    qslotChangeRecordState(true);
}

void AFMainFrame::StopRecording()
{
    qslotChangeRecordState(false);
}

void AFMainFrame::PauseRecording()
{
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
        this, QT_UTF8("녹화 중지"), QT_UTF8("녹화 중지"));
}

void AFMainFrame::UnPauseRecording()
{
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
        this, QT_UTF8("녹화 재개?"), QT_UTF8("녹화 재개?"));
}

void AFMainFrame::StartReplayBuffer()
{
    qslotStartReplayBuffer();
}

void AFMainFrame::StopReplayBuffer()
{
    qslotStopReplayBuffer();
}

void AFMainFrame::EnablePreview()
{
    if (IsPreviewProgramMode())
        return;

    m_bPreviewEnabled = true;
    EnablePreviewDisplay(true);
}

void AFMainFrame::DisablePreview()
{
    if (IsPreviewProgramMode())
        return;

    m_bPreviewEnabled = false;
    EnablePreviewDisplay(false);
}

void AFMainFrame::EnablePreviewProgam()
{
    AFMainDynamicComposit* dynamicComposit = GetMainWindow();
    if (dynamicComposit)
        dynamicComposit->qslotToggleStudioModeBlock(true);
}

void AFMainFrame::DiablePreviewProgam()
{
    AFMainDynamicComposit* dynamicComposit = GetMainWindow();
    if (dynamicComposit)
        dynamicComposit->qslotToggleStudioModeBlock(false);
}

void AFMainFrame::RecvHotkeyTransition()
{

}

void AFMainFrame::RecvHotkeyResetStats()
{

}

void AFMainFrame::RecvHotkeyScreenShotSelectedSource()
{

}

void AFMainFrame::ReloadCustomBrowserMenu()
{
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

    AFQCustomMenu* addon = _FindSubMenuByTitle(m_qTopMenu, QT_UTF8(locale.Str("Basic.MainMenu.Addon")));

    if (addon)
    {
        AFQCustomMenu* customMenu = _FindSubMenuByTitle(m_qTopMenu,
            QT_UTF8(locale.Str("Basic.MainMenu.Addon.CustomBrowserDocks")));

        if (customMenu)
        {
            addon->removeAction(customMenu->menuAction());
        }

        addon->addMenu(m_DynamicCompositMainWindow->CreateCustomBrowserMenu());
    }
}

void AFMainFrame::CustomBrowserClosed(QString uuid)
{
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

    AFQCustomMenu* addon = _FindSubMenuByTitle(m_qTopMenu, QT_UTF8(locale.Str("Basic.MainMenu.Addon")));

    if (addon)
    {
        AFQCustomMenu* customMenu = _FindSubMenuByTitle(m_qTopMenu,
            QT_UTF8(locale.Str("Basic.MainMenu.Addon.CustomBrowserDocks")));

        if (customMenu)
        {
            QList<QAction*> actions = customMenu->actions();
            for (int i = 0; i < actions.count(); i++)
            {
                QString propertyuuid = actions[i]->property("uuid").toString();
                if (propertyuuid == uuid)
                {
                    actions[i]->setChecked(false);
                }
            }
        }
    }
}

void AFMainFrame::SetDisplayAffinity(QWindow* window)
{
    if (!SetDisplayAffinitySupported())
        return;

    auto& confManager = AFConfigManager::GetSingletonInstance();

    bool hideFromCapture = config_get_bool(confManager.GetGlobal(),
        "BasicWindow",
        "HideOBSWindowsFromCapture");

    // Don't hide projectors, those are designed to be visible / captured
    if (window->property("isOBSProjectorWindow") == true)
        return;

#ifdef _WIN32
    HWND hwnd = (HWND)window->winId();

    DWORD curAffinity;
    if (GetWindowDisplayAffinity(hwnd, &curAffinity)) {
        if (hideFromCapture && curAffinity != WDA_EXCLUDEFROMCAPTURE)
            SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
        else if (!hideFromCapture && curAffinity != WDA_NONE)
            SetWindowDisplayAffinity(hwnd, WDA_NONE);
    }

#else
    // TODO: Implement for other platforms if possible. Don't forget to
    // implement SetDisplayAffinitySupported too!
    UNUSED_PARAMETER(hideFromCapture);
#endif
}

void AFMainFrame::ResetStudioModeUI(bool changeLayout)
{
    config_t* configGlobalFile = AFConfigManager::GetSingletonInstance().GetGlobal();
    bool studioPortraitLayout = config_get_bool(configGlobalFile, "BasicWindow", "StudioPortraitLayout");

    if (changeLayout)
    {
        m_DynamicCompositMainWindow->SwitchStudioModeLayout(studioPortraitLayout);
    }

    bool studioModeLabels = config_get_bool(configGlobalFile, "BasicWindow", "StudioModeLabels");
    m_DynamicCompositMainWindow->ToggleStudioModeLabels(studioPortraitLayout, studioModeLabels);
}

void AFMainFrame::qslotChangeBroadState(bool BroadButtonOn)
{
    
    if (BroadButtonOn) {
        if (m_BroadStartTimer && m_BroadStartTimer->isActive())
            return;
    }

    if (m_MainGuideWidget)
    {
        m_MainGuideWidget->deleteLater();
        m_MainGuideWidget = nullptr;
    }

    if (m_ProgramGuideWidget && m_ProgramGuideWidget->isWidgetType())
        m_ProgramGuideWidget->NextMission(2);

    QPushButton* BroadButton = reinterpret_cast<QPushButton*>(sender());
    if (!BroadButton)
        BroadButton = ui->pushButton_Broad;

    auto& authManager = AFAuthManager::GetSingletonInstance();
    int cntOfAccount = authManager.GetCntChannel();
    bool existStream = false;

    AFChannelData* mainChannel = nullptr;
    if (authManager.GetMainChannelData(mainChannel))
        existStream = mainChannel->bIsStreaming;
    

    for (int idx = 0; idx < cntOfAccount; idx++) {
        AFChannelData* tmpChannel = nullptr;
        authManager.GetChannelData(idx, tmpChannel);
        if (tmpChannel->bIsStreaming) {
            existStream = true;
            break;
        }
    }

    if (!existStream) { 
        ui->pushButton_Broad->setChecked(false);
        int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
            "",
            AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Stream.MissingStreamKey"));

        if (result == QDialog::Accepted)
        {
            _ShowSettingPopup(1);
        }

        BroadButton->setChecked(false);
        return;
    }
     
    if (BroadButtonOn)
    {
        if(IsStreamActive())
            return;

        auto& authManager = AFAuthManager::GetSingletonInstance();
        int cntOfAccount = authManager.GetCntChannel();
        int handlerIdx = 0;
        for (int idx = 0; idx < cntOfAccount; idx++) 
        {
            AFChannelData* tmpChannel = nullptr;
            authManager.GetChannelData(idx, tmpChannel);
            if (!tmpChannel || !tmpChannel->pAuthData)
                continue;

            if (tmpChannel->pAuthData->strPlatform == "Youtube")
            {
                if (tmpChannel->bIsStreaming == true)
                {
                    AFBasicAuth* authData = tmpChannel->pAuthData;
                    if (authData->bCheckedRTMPKey == false)
                    {
                        if (ShowPopupPageYoutubeChannel(authData) != QDialog::Accepted)
                        {
                            BroadButton->setChecked(false);
                            return;
                        }
                    }
                }
            }
        }
    
        bool confirm = config_get_bool(GetGlobalConfig(), "BasicWindow", "WarnBeforeStartingStream");
        bool recordWhenStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "RecordWhenStreaming");
        bool replayWhileStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "ReplayBufferWhileStreaming");

        if (confirm)
        {
            int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                AFLocaleTextManager::GetSingletonInstance().Str("ConfirmStart.Title"),
                AFLocaleTextManager::GetSingletonInstance().Str("ConfirmStart.Text"));

            if (result != 1)
            {
                BroadButton->setChecked(false);
                return;
            }
        }

        // delay start
        m_BroadStartTimer = new QTimer(this);
        m_iBroadTimerRemaining = 3;

        /*m_qBroadCountDownWidget = new QWidget(this);
        m_qBroadCountDownWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        m_qBroadCountDownWidget->setAttribute(Qt::WA_TranslucentBackground);
        m_qBroadCountDownWidget->setAttribute(Qt::WA_DeleteOnClose);
        m_qBroadCountDownWidget->setFixedSize(260, 260);

        m_qBroadLabel = new QLabel(m_qBroadCountDownWidget);
        m_qBroadLabel->setAlignment(Qt::AlignCenter);
        m_qBroadLabel->setObjectName("label_Countdown");
        m_qBroadLabel->setText(QString::number(m_iBroadTimerRemaining));

        QWidget* broadCountWidget = new QWidget(m_qBroadCountDownWidget);
        broadCountWidget->setObjectName("widget_BroadCountDown");
        QVBoxLayout* countDownLayout = new QVBoxLayout(broadCountWidget);
        countDownLayout->setContentsMargins(0, 0, 0, 0);
        countDownLayout->setSpacing(0);
        countDownLayout->addWidget(m_qBroadLabel);
        broadCountWidget->setLayout(countDownLayout);

        QVBoxLayout* countDownOuterLayout = new QVBoxLayout(m_qBroadCountDownWidget);
        countDownOuterLayout->setContentsMargins(0, 0, 0, 0);
        countDownOuterLayout->setSpacing(0);
        countDownOuterLayout->addWidget(broadCountWidget);
        m_qBroadCountDownWidget->setLayout(countDownOuterLayout);

        int countDownWidth = x() + (width() / 2) - (m_qBroadCountDownWidget->width() / 2);
        int height = y() + (m_DynamicCompositMainWindow->GetMainPreview()->height() / 2)
            - (m_qBroadCountDownWidget->height() / 2) + ui->frame_Top->height();
        m_qBroadCountDownWidget->move(countDownWidth, height);
        m_qBroadCountDownWidget->show();*/
        if (m_qBroadMovie == nullptr)
        {
            std::string absPath;
            GetDataFilePath("assets", absPath);
            QString gifPath = QString("%1/mainview/broad-spinner-black.gif").
                arg(absPath.data());
            m_qBroadMovie = new QMovie(gifPath, QByteArray(), this);
        }
        QSize s = ui->pushButton_Broad->rect().size();
        connect(m_qBroadMovie, &QMovie::frameChanged, [=] {
            ui->pushButton_Broad->setIcon(m_qBroadMovie->currentPixmap());
            ui->pushButton_Broad->setIconSize(s);
            });
        m_qBroadMovie->start();
        ui->pushButton_Broad->setText("");
        connect(m_BroadStartTimer, &QTimer::timeout, this, &AFMainFrame::qslotStartCountDown);
        m_BroadStartTimer->start(3000);

        
        // => qslotStartCountDown
       
    }
    else
    {
        bool confirm = config_get_bool(GetGlobalConfig(), "BasicWindow", "WarnBeforeStoppingStream");
        bool recordWhenStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "RecordWhenStreaming");
        bool keepRecordingWhenStreamStops = config_get_bool(GetGlobalConfig(), "BasicWindow", "KeepRecordingWhenStreamStops");
        bool replayWhileStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
        bool keepReplayStreamStops = config_get_bool(GetGlobalConfig(), "BasicWindow", "KeepReplayBufferStreamStops");
        
        if (confirm)
        {
            int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                                                    AFLocaleTextManager::GetSingletonInstance().Str("ConfirmStop.Title"),
                                                    AFLocaleTextManager::GetSingletonInstance().Str("ConfirmStop.Text"));
            
            if (result != 1)
            {
                BroadButton->setChecked(true);
                return;
            }
        }
        
        ui->pushButton_Broad->setText("LIVE");
        ui->pushButton_Broad->setFixedWidth(77);
        if (m_qBroadMovie != nullptr)
        {
            m_qBroadMovie->stop();
            ui->pushButton_Broad->setIcon(QIcon());
        }
        
        if (m_BroadStartTimer != nullptr)
        {
            if (m_BroadStartTimer->isActive())
            {
                m_BroadStartTimer->stop();
                
                /*if (m_qBroadCountDownWidget != nullptr)
                    if (m_qBroadCountDownWidget->isVisible())
                        m_qBroadCountDownWidget->close();*/
            }
        }
        

        // broad stop
        qslotStopStreaming();
    }
    
}
void AFMainFrame::qslotEnterBroadButton()
{
    if(!ui->pushButton_Broad->isChecked())
        ui->pushButton_Broad->
        setStyleSheet("#pushButton_Broad {background: #D1FF01;} #pushButton_Broad:checked {background-color: #FF2424;}");

}
void AFMainFrame::qslotLeaveBroadButton()
{
    if (!ui->pushButton_Broad->isChecked())
        ui->pushButton_Broad->
        setStyleSheet("#pushButton_Broad {background: qlineargradient( x1:0 y1:0, x2:1 y2:0, stop:0 #0EE3F1 , stop:1 #D1FF00);} #pushButton_Broad:checked {background-color: #FF2424;}");
}
bool AFMainFrame::qslotReplayBufferClicked()
{
    if(IsReplayBufferActive()) {
        qslotStopReplayBuffer();
        qslotReplayBufferSave();
        return false;
    } else {
        qslotStartReplayBuffer();
        return true;
    }
}
void AFMainFrame::qslotPauseRecordingClicked()
{
    if (!m_outputHandlers[0].second || !m_outputHandlers[0].second->fileOutput) {
        return;
    }
    obs_output_t* output_ = m_outputHandlers[0].second->fileOutput;
    bool paused = obs_output_paused(output_);
    if(paused) {
        UnPauseRecording();
    } else {
        PauseRecording();
    }
}
void AFMainFrame::qslotStartCountDown()
{
    m_qBroadMovie->stop();
    ui->pushButton_Broad->setIcon(QIcon());
    m_BroadStartTimer->stop();
    qslotStartStreaming();
}

void AFMainFrame::qslotUpdateCountDown()
{
    //ui->pushButton_Broad->setText(QString::number(m_iBroadTimerRemaining));
}

void AFMainFrame::qslotChangeRecordState(bool checked)
{
    // already recording start
    if(IsRecordingActive())
    {
        bool confirm = config_get_bool(GetGlobalConfig(), "BasicWindow", "WarnBeforeStoppingRecord");
        if(confirm && isVisible()) {
            int result =  AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                                                     QTStr("ConfirmStopRecord.Title"),
                                                     QTStr("ConfirmStopRecord.Text"));

            if(result == QDialog::Rejected) {
                ui->pushButton_Record->setChecked(true);
                return;
            }
        }
        // stop recording
        qslotStopRecording();
    }
    else
    {
        if(!AFUIValidation::NoSourcesConfirmation(this)) {
            ui->pushButton_Record->setChecked(false);
            return;
        }
        // start recording
        qslotStartRecording();
    }
}

static inline void SetEncoderName(obs_encoder_t* encoder, const char* name,
    const char* defaultName)
{
    obs_encoder_set_name(encoder, (name && *name) ? name : defaultName);
}

void AFMainFrame::SetupAutoRemux(const char*& container)
{
    bool autoRemux = config_get_bool(GetBasicConfig(), "Video", "AutoRemux");
    if (autoRemux && strcmp(container, "mp4") == 0)
        container = "mkv";
}

std::string AFMainFrame::GetRecordingFilename(
    const char* path, const char* container, bool noSpace, bool overwrite,
    const char* format, bool ffmpeg)
{
    if (!ffmpeg)
        SetupAutoRemux(container);

    std::string dst = AFSettingUtilsA::GetOutputFilename(path, container, noSpace, overwrite, format);
    lastRecordingPath = dst;
    return dst;
}

void AFMainFrame::_ShowSettingPopup(int tabPage)
{
    if(m_BlockPopup != nullptr)
        if(m_BlockPopup->isVisible())
            _HideBlockArea(0);
      

    if(!m_StudioSettingPopup)
    {
        m_StudioSettingPopup = new AFQStudioSettingDialog(this);
        m_StudioSettingPopup->AFQStudioSettingDialogInit(tabPage);
    }

    m_StudioSettingPopup->exec();

    if (g_bRestart)
    {
        auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();
        int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                                                "", QT_UTF8(localeTextManager.Str("NeedsRestart")));

        if(result == QDialog::Accepted)
        {
            close();
        } 
        else
        {
            g_bRestart = false;
        }
    }

    LoadAccounts();

}

void AFMainFrame::_ShowSettingPopupWithID(QString id)
{
    if (m_BlockPopup != nullptr)
        if (m_BlockPopup->isVisible())
            _HideBlockArea(0);


    if (!m_StudioSettingPopup)
    {
        m_StudioSettingPopup = new AFQStudioSettingDialog(this);
        m_StudioSettingPopup->AFQStudioSettingDialogInit(1);
        m_StudioSettingPopup->SetID(id);
    }

    m_StudioSettingPopup->exec();

    if (g_bRestart)
    {
        auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();
        int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
            "", QT_UTF8(localeTextManager.Str("NeedsRestart")));

        if (result == QDialog::Accepted)
        {
            close();
        }
        else
        {
            g_bRestart = false;
        }
    }

    LoadAccounts();
}

QString AFMainFrame::_AccountButtonsStyling(bool stream)
{
    return QString();
}

void AFMainFrame::_SetupAccountUI()
{
    //temp
    ui->pushButton_Soop->SetMainAccount();
    connect(ui->pushButton_Soop, &QPushButton::clicked, this, &AFMainFrame::qslotShowMainAuthMenu);

    connect(ui->pushButton_Login, &QPushButton::clicked, this, &AFMainFrame::qslotShowLoginMenu);
    
    connect(ui->pushButton_AuthSettings, &QPushButton::clicked,
            this, &AFMainFrame::qslotShowStudioSettingWithButtonSender);
    ui->pushButton_AuthSettings->setProperty("type", "1");
    // !!

    auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();
    //ui->pushButton_Login->setText(QTStr("Login"));
    
    
    LoadAccounts();
}

bool AFMainFrame::LoadAccounts()
{
    auto& authManager = AFAuthManager::GetSingletonInstance();
        

    bool oneMoreStreaming = false;

    AFChannelData* mainChannel = nullptr;
    bool regMainAccount = authManager.GetMainChannelData(mainChannel);
    bool mainAccountStreaming = false;
    if (regMainAccount)
        mainAccountStreaming = mainChannel->bIsStreaming;

    bool regOtherAccount = authManager.GetCntChannel() > 0 ? true : false;
    
    std::vector<AFChannelData*> listStreamingChannel;
    int cntOfAccount = authManager.GetCntChannel();

    for (int idx = 0; idx < cntOfAccount; idx++)
    {
        AFChannelData* tmpChannel = nullptr;
        authManager.GetChannelData(idx, tmpChannel);
        
        if (tmpChannel->bIsStreaming)
        {
            oneMoreStreaming = true;
            listStreamingChannel.emplace_back(tmpChannel);
        }
    }

    QList<AFMainAccountButton*> accountButtons = ui->widget_Platform->findChildren<AFMainAccountButton*>();
   
    for (auto& tb : accountButtons)
    {
        if (tb->IsMainAccount())
            continue;

        ui->widget_Platform->layout()->removeWidget(tb);
        tb->close();
        delete tb;
    }

    ui->pushButton_Soop->SetPlatformImage("SOOP Global", !regMainAccount);
    ui->pushButton_Soop->SetStreaming(IsStreamActive(), mainAccountStreaming, !regMainAccount);
    if (regMainAccount)
    {
        ui->pushButton_Soop->SetChannelData(mainChannel);
        oneMoreStreaming = true;
    }
    
    if (oneMoreStreaming)
    {       
        for (int i = 0; i < ui->widget_Platform->layout()->count(); ++i)
        {
            QLayoutItem* item = ui->widget_Platform->layout()->itemAt(i);
            if (item->spacerItem())
            {
                ui->widget_Platform->layout()->takeAt(i);
                delete item;
                break;
            }
        }
        
        ui->widget_Platform->layout()->removeWidget(ui->pushButton_AuthSettings);
        ui->widget_Platform->layout()->addWidget(ui->widget_PlatformLine);


        for (auto& channel :
             std::vector<AFChannelData*>(listStreamingChannel.rbegin(),
                                         listStreamingChannel.rend()))
        {
            AFMainAccountButton* accountButton = new AFMainAccountButton(this);
            accountButton->SetChannelData(channel);

            if (!channel->pAuthData)
                continue;

            accountButton->SetPlatformImage(channel->pAuthData->strPlatform);
            accountButton->SetStreaming(IsStreamActive());
            connect(accountButton, &QPushButton::clicked, this, &AFMainFrame::qslotShowOtherAuthMenu);

            ui->widget_Platform->layout()->addWidget(accountButton);

        }


        ui->widget_Platform->layout()->addWidget(ui->pushButton_AuthSettings);
        QHBoxLayout* layout = reinterpret_cast<QHBoxLayout*>(ui->widget_Platform->layout());
        layout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
        
        ui->stackedWidget_Platform->setCurrentIndex(1);

        return true;
    }
    else if (regMainAccount == false && regOtherAccount == true &&
             oneMoreStreaming == false)
    {
        ui->widget_Platform->layout()->removeWidget(ui->widget_PlatformLine);
        
        ui->widget_Login->layout()->addWidget(ui->widget_PlatformLine);
        //
        
        ui->stackedWidget_Platform->setCurrentIndex(1);

        return true;
    }
    else if (regMainAccount == false && regOtherAccount == false)
    {
        ui->widget_Platform->layout()->addWidget(ui->widget_PlatformLine);

        if (oneMoreStreaming)
        {
            ui->widget_Platform->layout()->addWidget(ui->widget_PlatformLine);
        }

        ui->stackedWidget_Platform->setCurrentIndex(0);
        return false;
    }

    return true;
}

QString AFMainFrame::GetChannelID(obs_output_t* output)
{
    QString channelID = "";
    if (!output)
        return channelID;

    AFChannelData* channelData = nullptr;
    OUTPUT_HANDLER_LIST::iterator outputIter;
    
    // Main Output
    if (output == m_outputHandlers[0].second.get()->streamOutput)
    {
        auto& authManager = AFAuthManager::GetSingletonInstance();
        authManager.GetMainChannelData(channelData);
        std::string strChannelID = channelData->pAuthData->strChannelID;
        if (!strChannelID.empty())
            channelID = QString::fromStdString(strChannelID);
        return channelID;
    }

    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (!outputIter->second.get())
            continue;
        if (outputIter->second.get()->streamOutput == output) 
        {
            auto& authManager = AFAuthManager::GetSingletonInstance();
            authManager.GetChannelData(outputIter->first, channelData);
            if (channelData) 
            {
                std::string strChannelID = channelData->pAuthData->strChannelID;
                if (!strChannelID.empty())
                    channelID = QString::fromStdString(strChannelID);
                break;
            }
        }
    }

    return channelID;
}

void AFMainFrame::EnumDialogs()
{

}

inline const char* GetCurrentOutputPath()
{
    const char* path = nullptr;
    const char* mode = config_get_string(GetBasicConfig(), "Output", "Mode");

    if(strcmp(mode, "Advanced") == 0)
    {
        const char* advanced_mode = config_get_string(GetBasicConfig(), "AdvOut", "RecType");
        if(strcmp(advanced_mode, "FFmpeg") == 0) {
            path = config_get_string(GetBasicConfig(), "AdvOut", "FFFilePath");
        } else {
            path = config_get_string(GetBasicConfig(), "AdvOut", "RecFilePath");
        }
    } else {
        path = config_get_string(GetBasicConfig(), "SimpleOutput", "FilePath");
    }

    return path;
}

bool AFMainFrame::_OutputPathValid()
{
    const char* mode = config_get_string(GetBasicConfig(), "Output", "Mode");
    if(strcmp(mode, "Advanced") == 0) {
        const char* advanced_mode = config_get_string(GetBasicConfig(), "AdvOut", "RecType");
        if(strcmp(advanced_mode, "FFmpeg") == 0) {
            bool is_local = config_get_bool(GetBasicConfig(), "AdvOut", "FFOutputToFile");
            if(!is_local)
                return true;
        }
    }

    const char* path = GetCurrentOutputPath();
    return path && *path && QDir(path).exists();
}
void AFMainFrame::_OutputPathInvalidMessage()
{
    blog(LOG_ERROR, "Recording stopped because of bad output path");

    // AFCMessageBox::critical(this, QTStr("Output.BadPath.Title"), QTStr("Output.BadPath.Text"));
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                               QTStr("Output.BadPath.Title"),
                               QTStr("Output.BadPath.Text"));
}

void AFMainFrame::_SetResourceCheckTimer(int time)
{
    m_ResourceRefreshTimer = new QTimer(this);

    connect(m_ResourceRefreshTimer, &QTimer::timeout, 
        this, &AFMainFrame::qsignalRefreshTimerTick);

    m_ResourceRefreshTimer->start(time);
}

bool AFMainFrame::_LowDiskSpace()
{
    const char* path;

    path = GetCurrentOutputPath();
    if(!path)
        return false;

    if(!QDir(path).exists())
        return false;

    uint64_t num_bytes = os_get_free_disk_space(path);

    if(num_bytes < (MAX_BYTES_LEFT))
        return true;
    else
        return false;
}
void AFMainFrame::_DiskSpaceMessage()
{
    blog(LOG_ERROR, "Recording stopped because of low disk space");

    // AFCMessageBox::critical(this, QTStr("Output.RecordNoSpace.Title"), QTStr("Output.RecordNoSpace.Msg"));
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                               QTStr("Output.RecordNoSpace.Title"),
                               QTStr("Output.RecordNoSpace.Msg"));
}

QColor AFMainFrame::_GetSourceListBackgroundColor(int preset)
{
    QColor color;
    switch (preset) {
    case 1: color.setRgb(255, 68, 68, 84); break;
    case 2: color.setRgb(255, 255, 68, 84); break;
    case 3: color.setRgb(68, 255, 68, 84); break;
    case 4: color.setRgb(68, 255, 255, 84); break;
    case 5: color.setRgb(68, 68, 255, 84); break;
    case 6: color.setRgb(255, 68, 255, 84); break;
    case 7: color.setRgb(68, 68, 68, 84); break;
    case 8: color.setRgb(255, 255, 255, 84); break;
    default:
        break;
    }

    return color;
}

void AFMainFrame::ClearSceneBottomButtons()
{
    auto it = m_vSceneButton.begin();
    for (; it != m_vSceneButton.end(); it++) {
        AFQSceneBottomButton* button = (*it);
        if (button) {
            button->close();
            delete button;
        }
    }
    m_vSceneButton.clear();
}

AFQCustomMenu* AFMainFrame::_AddScaleFilteringMenu(AFQCustomMenu* menu, obs_sceneitem_t* item)
{
    obs_scale_type scaleFilter = obs_sceneitem_get_scale_filter(item);
    QAction* action;

#define ADD_MODE(name, mode)                                 \
	action = menu->addAction(Str("" name), this,             \
				 &AFMainFrame::qSlotSetScaleFilter);         \
	action->setProperty("mode", (int)mode);                  \
	action->setCheckable(true);                              \
	action->setChecked(scaleFilter == mode);

    ADD_MODE("Disable", OBS_SCALE_DISABLE);
    ADD_MODE("ScaleFiltering.Point", OBS_SCALE_POINT);
    ADD_MODE("ScaleFiltering.Bilinear", OBS_SCALE_BILINEAR);
    ADD_MODE("ScaleFiltering.Bicubic", OBS_SCALE_BICUBIC);
    ADD_MODE("ScaleFiltering.Lanczos", OBS_SCALE_LANCZOS);
    ADD_MODE("ScaleFiltering.Area", OBS_SCALE_AREA);
#undef ADD_MODE

    return menu;
}

AFQCustomMenu* AFMainFrame::_AddBlendingModeMenu(AFQCustomMenu* menu, obs_sceneitem_t* item)
{
    obs_blending_type blendingMode = obs_sceneitem_get_blending_mode(item);
    QAction* action;

#define ADD_MODE(name, mode)                                    \
	action = menu->addAction(Str("" name), this,                \
				 &AFMainFrame::qSlotBlendingMode);              \
	action->setProperty("mode", (int)mode);                     \
	action->setCheckable(true);                                 \
	action->setChecked(blendingMode == mode);

    ADD_MODE("BlendingMode.Normal", OBS_BLEND_NORMAL);
    ADD_MODE("BlendingMode.Additive", OBS_BLEND_ADDITIVE);
    ADD_MODE("BlendingMode.Subtract", OBS_BLEND_SUBTRACT);
    ADD_MODE("BlendingMode.Screen", OBS_BLEND_SCREEN);
    ADD_MODE("BlendingMode.Multiply", OBS_BLEND_MULTIPLY);
    ADD_MODE("BlendingMode.Lighten", OBS_BLEND_LIGHTEN);
    ADD_MODE("BlendingMode.Darken", OBS_BLEND_DARKEN);
#undef ADD_MODE

    return menu;
}

AFQCustomMenu* AFMainFrame::_AddBlendingMethodMenu(AFQCustomMenu* menu, obs_sceneitem_t* item)
{
    obs_blending_method blendingMethod =
        obs_sceneitem_get_blending_method(item);
    QAction* action;
    
#define ADD_MODE(name, method)                                  \
	action = menu->addAction(Str("" name), this,                \
				 &AFMainFrame::qSlotBlendingMethod);            \
	action->setProperty("method", (int)method);                 \
	action->setCheckable(true);                                 \
	action->setChecked(blendingMethod == method);

    ADD_MODE("BlendingMethod.Default", OBS_BLEND_METHOD_DEFAULT);
    ADD_MODE("BlendingMethod.SrgbOff", OBS_BLEND_METHOD_SRGB_OFF);
#undef ADD_MODE

    return menu;
}

AFQCustomMenu* AFMainFrame::_AddDeinterlacingMenu(AFQCustomMenu* menu, obs_source_t* source)
{
    obs_deinterlace_mode deinterlaceMode =
        obs_source_get_deinterlace_mode(source);
    obs_deinterlace_field_order deinterlaceOrder =
        obs_source_get_deinterlace_field_order(source);
    QAction* action;

#define ADD_MODE(name, mode)                                        \
	action = menu->addAction(Str("" name), this,                    \
				 &AFMainFrame::qSlotSetDeinterlaceingMode);         \
	action->setProperty("mode", (int)mode);                         \
	action->setCheckable(true);                                     \
	action->setChecked(deinterlaceMode == mode);

    ADD_MODE("Disable", OBS_DEINTERLACE_MODE_DISABLE);
    ADD_MODE("Deinterlacing.Discard", OBS_DEINTERLACE_MODE_DISCARD);
    ADD_MODE("Deinterlacing.Retro", OBS_DEINTERLACE_MODE_RETRO);
    ADD_MODE("Deinterlacing.Blend", OBS_DEINTERLACE_MODE_BLEND);
    ADD_MODE("Deinterlacing.Blend2x", OBS_DEINTERLACE_MODE_BLEND_2X);
    ADD_MODE("Deinterlacing.Linear", OBS_DEINTERLACE_MODE_LINEAR);
    ADD_MODE("Deinterlacing.Linear2x", OBS_DEINTERLACE_MODE_LINEAR_2X);
    ADD_MODE("Deinterlacing.Yadif", OBS_DEINTERLACE_MODE_YADIF);
    ADD_MODE("Deinterlacing.Yadif2x", OBS_DEINTERLACE_MODE_YADIF_2X);
#undef ADD_MODE

    menu->addSeparator();

#define ADD_ORDER(name, order)                                      \
	action = menu->addAction(QTStr("Deinterlacing." name), this,    \
				 &AFMainFrame::qSlotSetDeinterlacingOrder);         \
	action->setProperty("order", (int)order);                       \
	action->setCheckable(true);                                     \
	action->setChecked(deinterlaceOrder == order);

    ADD_ORDER("TopFieldFirst", OBS_DEINTERLACE_FIELD_ORDER_TOP);
    ADD_ORDER("BottomFieldFirst", OBS_DEINTERLACE_FIELD_ORDER_BOTTOM);
#undef ADD_ORDER

    return menu;
}


template<typename SlotFunc>
void AFMainFrame::_connectAndAddAction(QAction* sender,
     const typename QtPrivate::FunctionPointer<SlotFunc>::Object* receiver, 
     SlotFunc slot)
{
    QObject::connect(sender, &QAction::triggered, receiver, slot);
    addAction(sender);
}
void AFMainFrame::_RegisterSourceControlAction()
{
    _connectAndAddAction(ui->action_LockPreview, this, &AFMainFrame::qslotLockPreview);

    _connectAndAddAction(ui->action_CopySource, this, &AFMainFrame::qSlotActionCopySource);
    _connectAndAddAction(ui->action_PasteSourceRef, this, &AFMainFrame::qSlotActionPasteRefSource);
    _connectAndAddAction(ui->action_PasteSourceDuplicate, this, &AFMainFrame::qSlotActionPasteDupSource);

    _connectAndAddAction(ui->action_Filters, this, &AFMainFrame::qSlotOpenSourceFilters);
    _connectAndAddAction(ui->action_CopyFilters, this, &AFMainFrame::qSlotCopySourceFilters);
    _connectAndAddAction(ui->action_PasteFilters, this, &AFMainFrame::qSlotPasteSourceFilters);

    _connectAndAddAction(ui->action_RenameSource, this, &AFMainFrame::qSlotActionRenameSource);
    _connectAndAddAction(ui->action_RemoveSource, this, &AFMainFrame::qSlotActionRemoveSource);

    _connectAndAddAction(ui->action_EditTransform, this, &AFMainFrame::qSlotActionEditTransform);
    _connectAndAddAction(ui->action_CopyTransform, this, &AFMainFrame::qSlotActionCopyTransform);
    _connectAndAddAction(ui->action_PasteTransform, this, &AFMainFrame::qSlotActionPasteTransform);
    _connectAndAddAction(ui->action_ResetTransform, this, &AFMainFrame::qSlotActionResetTransform);

    _connectAndAddAction(ui->action_Rotate90CW, this, &AFMainFrame::qSlotActionRotate90CW);
    _connectAndAddAction(ui->action_Rotate90CCW, this, &AFMainFrame::qSlotActionRotate90CCW);
    _connectAndAddAction(ui->action_Rotate180, this, &AFMainFrame::qSlotActionRotate180);

    _connectAndAddAction(ui->action_FlipHorizontal, this, &AFMainFrame::qSlotFlipHorizontal);
    _connectAndAddAction(ui->action_FlipVertical, this, &AFMainFrame::qSlotFlipVertical);

    _connectAndAddAction(ui->action_FitToScreen, this, &AFMainFrame::qSlotFitToScreen);
    _connectAndAddAction(ui->action_StretchToScreen, this, &AFMainFrame::qSlotStretchToScreen);
    _connectAndAddAction(ui->action_CenterToScreen, this, &AFMainFrame::qSlotCenterToScreen);
    _connectAndAddAction(ui->action_VerticalCenter, this, &AFMainFrame::qSlotVerticalCenter);
    _connectAndAddAction(ui->action_HorizontalCenter, this, &AFMainFrame::qSlotHorizontalCenter);

    _connectAndAddAction(ui->action_OrderMoveUp, GetMainWindow()->GetSceneSourceDock(), &AFSceneSourceDockWidget::qSlotMoveUpSourceTrigger);
    _connectAndAddAction(ui->action_OrderMoveDown, GetMainWindow()->GetSceneSourceDock(), &AFSceneSourceDockWidget::qSlotMoveDownSourceTrigger);
    _connectAndAddAction(ui->action_OrderMoveToTop, GetMainWindow()->GetSceneSourceDock(), &AFSceneSourceDockWidget::qSlotMoveToTopSourceTrigger);
    _connectAndAddAction(ui->action_OrderMoveToBottom, GetMainWindow()->GetSceneSourceDock(), &AFSceneSourceDockWidget::qSlotMoveToBottomSourceTrigger);

    _connectAndAddAction(ui->action_ShowInteract, this, &AFMainFrame::qSlotActionShowInteractionPopup);
    _connectAndAddAction(ui->action_ShowProperties, this, &AFMainFrame::qSlotActionShowProperties);

    GetMainWindow()->GetMainPreview()->RegisterShortCutAction(ui->action_RemoveSource);
    GetMainWindow()->GetMainPreview()->RegisterShortCutAction(ui->action_CopySource);
    GetMainWindow()->GetMainPreview()->RegisterShortCutAction(ui->action_PasteSourceRef);

    GetMainWindow()->GetSceneSourceDock()->RegisterShortCut(ui->action_RemoveSource);
    GetMainWindow()->GetSceneSourceDock()->RegisterShortCut(ui->action_CopySource);
    GetMainWindow()->GetSceneSourceDock()->RegisterShortCut(ui->action_PasteSourceRef);
    GetMainWindow()->GetSceneSourceDock()->RegisterShortCut(ui->action_RenameSource);
}

void AFMainFrame::_ChangeStreamState(bool enable, bool checked, QString title, int width)
{

    ui->pushButton_Broad->setEnabled(enable);
    ui->pushButton_Broad->setChecked(checked);
    ui->pushButton_Broad->setText(title);
    ui->pushButton_Broad->setFixedWidth(width);
}

void AFMainFrame::_ToggleBroadTimer(bool start)
{
    if (start)
    {
        ui->label_BroadTime->StartCount();
    }
    else
    {
        ui->label_BroadTime->StopCount();
    }

    ui->widget_BroadTimer->setVisible(start);
    if (ui->widget_RecordTimer->isVisible())
    {
        ui->line_Time->setVisible(start);
    }
    _AccountButtonStreamingToggle(start);
}

void AFMainFrame::_ChangeRecordState(bool check)
{
    ui->pushButton_Record->setChecked(check);
    if(check) {
        // start recording
        ui->pushButton_Record->setFixedWidth(87);
        ui->pushButton_Record->setText("STOP REC");
        ui->label_RecordTime->StartCount();

        ui->widget_RecordTimer->setVisible(true);
        if(ui->widget_BroadTimer->isVisible()) {
            ui->line_Time->setVisible(true);
        }
    } else {
        // stop recording
        ui->pushButton_Record->setFixedWidth(48);
        ui->pushButton_Record->setText("REC");
        ui->label_RecordTime->StopCount();

        ui->widget_RecordTimer->setVisible(false);
        ui->line_Time->setVisible(false);
    }
}

void AFMainFrame::_StopRecording(bool force)
{
    if(!m_outputHandlers[0].second) {
        return;
    }

    //SaveProject();
    QMetaObject::invokeMethod(this, "SaveProjectDeferred", Qt::QueuedConnection);

    if(IsRecordingActive()) {
        m_outputHandlers[0].second->StopRecording(force);
    }

    OnDeactivate();
}



void AFMainFrame::qslotTopMenuClicked()
{
    _ToggleTopMenu();
}

void AFMainFrame::qslotPopupBlockClicked()
{
    m_DynamicCompositMainWindow->RaiseBlocks();
    if (m_GlobalSoopChatWidget && m_GlobalSoopChatWidget->isVisible())
        m_GlobalSoopChatWidget->raise();
    if (m_GlobalSoopNewsfeedWidget && m_GlobalSoopNewsfeedWidget->isVisible())
        m_GlobalSoopNewsfeedWidget->raise();

}

void AFMainFrame::qslotStatsOpenTriggered()
{
    if (m_StatFrame == nullptr)
        m_StatFrame = new AFQStatWidget(nullptr);

    QRect adjustRect;
    QRect position = QRect(x() - m_StatFrame->width(), y(), m_StatFrame->width(), m_StatFrame->height());
    m_DynamicCompositMainWindow->AdjustPositionOutSideScreen(position, adjustRect);
    m_StatFrame->setGeometry(adjustRect);

    m_StatFrame->show();
    m_StatFrame->activateWindow();
    m_StatFrame->raise();
}

void AFMainFrame::qslotProgramGuideOpenTriggered()
{
    if (!m_ProgramGuideWidget)
    {
        m_ProgramGuideWidget = new AFQProgramGuideWidget(nullptr);
        connect(m_ProgramGuideWidget, &AFQProgramGuideWidget::qsignalMissionTrigger,
            this, &AFMainFrame::qslotGuideTriggered);
   }

   m_ProgramGuideWidget->show();
   _MoveProgramGuide();
   QRect adjustRect;
   m_DynamicCompositMainWindow->AdjustPositionOutSideScreen(m_ProgramGuideWidget->geometry(), adjustRect);
   m_ProgramGuideWidget->setGeometry(adjustRect);
   m_ProgramGuideWidget->raise();
}

void AFMainFrame::qslotProgamInfoOpenTriggered()
{
    bool closed = true;
    if (m_programInfoDialog)
        closed = m_programInfoDialog->close();

    if (!closed)
        return;

    m_programInfoDialog = new AFQProgramInfoDialog(this);
    m_programInfoDialog->setAttribute(Qt::WA_DeleteOnClose);
    m_programInfoDialog->show();
}

void AFMainFrame::qslotGuideTriggered(int guideNum)
{
    raise();

    if (!m_MainGuideWidget)
    {
        m_MainGuideWidget = new QWidget(this);
        
#if defined(_WIN32)
        m_MainGuideWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
#elif defined(__APPLE__)
        m_MainGuideWidget->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
#endif
        m_MainGuideWidget->setAttribute(Qt::WA_TranslucentBackground);
        m_MainGuideWidget->setAttribute(Qt::WA_DeleteOnClose);

        AFMainFrameGuide* guideWidgetContents = new AFMainFrameGuide(m_MainGuideWidget);
        connect(guideWidgetContents, &AFQHoverWidget::qsignalMouseClick, this, &AFMainFrame::qslotGuideClosed);

        if (guideNum == 1)
        {
            if (!m_BlockPopup->isVisible())
            {
                _MoveBlocks();
                m_BlockPopup->show();
            }

            int buttonx = m_BlockPopup->pos().x() - pos().x() + 22;
            int buttony = m_BlockPopup->pos().y() - pos().y() + 12;
            int buttonwidth = 0;
            int buttonheight = 0;
            guideWidgetContents->SceneSourceGuide(QRect(buttonx, buttony, buttonwidth, buttonheight));

            connect(guideWidgetContents, &AFMainFrameGuide::qsignalSceneSourceTriggered, 
                this, &AFMainFrame::qslotToggleBlockFromBlockArea);
            
        }
        else if (guideNum == 2)
        {
            int buttonx = 15;
            int buttony = ui->frame_Top->height() + ui->widget_Middle->height() + 10;
            int buttonwidth = ui->pushButton_Login->width() + 5;
            int buttonheight = ui->pushButton_Login->height() + 5;
            guideWidgetContents->LoginGuide(QRect(buttonx, buttony, buttonwidth, buttonheight));
            connect(guideWidgetContents, &AFMainFrameGuide::qsignalLoginTrigger, 
                this, &AFMainFrame::qslotShowLoginMenu);
        }
        else if (guideNum == 3)
        {
            int buttonx = ui->widget_BroadAndRecord->pos().x() + ui->widget_BroadAndRecordButtons->pos().x() - 1;
            int buttony = ui->frame_Top->height() + ui->widget_Middle->height() + 8;
            int buttonwidth = ui->pushButton_Broad->width();
            int buttonheight = ui->pushButton_Broad->height();
            guideWidgetContents->BroadGuide(QRect(buttonx, buttony, buttonwidth, buttonheight));
            connect(guideWidgetContents, &AFMainFrameGuide::qsignalBroadTriggered,
                ui->pushButton_Broad, &QPushButton::click);
        }


        QHBoxLayout* layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(guideWidgetContents);
        guideWidgetContents->setStyleSheet("QWidget{background-color:rgba(0,0,0,50%); border-radius:10px}");

        m_MainGuideWidget->setLayout(layout);
        m_MainGuideWidget->setGeometry(geometry());
        m_MainGuideWidget->show();
    }
}

void AFMainFrame::qslotGuideClosed()
{
    if (m_MainGuideWidget)
    {
        m_MainGuideWidget->close();
        m_MainGuideWidget->deleteLater();
        m_MainGuideWidget = nullptr;
    }
}

void AFMainFrame::qslotStatsCloseTriggered()
{
    m_StatFrame->hide();
}

void AFMainFrame::qslotStatsMouseLeave()
{
    ui->widget_ResourceNetwork->setStyleSheet("");
}

void AFMainFrame::qslotFocusChanged(QWidget* old, QWidget* now)
{
    qDebug() << "focused Changed";
    if (old == nullptr)
    {
        qDebug() << "old null";
    }
    else
    {
        qDebug() << "old: " << old->objectName();
    }

    if (now == nullptr)
    {
        qDebug() << "now null";
    }
    else
    {
        qDebug() << "now: " << now->objectName();
    }
}

void AFMainFrame::qslotSetDpiHundred()
{
    AFJsonController dpiJson;
    dpiJson.WriteDpiSample("1.0");
}

void AFMainFrame::qslotSetDpiHundredFifty()
{
    AFJsonController dpiJson;
    dpiJson.WriteDpiSample("1.5");
}

void AFMainFrame::qslotSetDpiTwoHundred()
{
    AFJsonController dpiJson;
    dpiJson.WriteDpiSample("2.0");
}

void AFMainFrame::qslotCloseAllPopup()
{
    
    this->close();
}

void AFMainFrame::qslotToggleBlockArea(bool show)
{
    if (show)
    {
        _ShowBlockArea();
    }
    else
    {
        _HideBlockArea();
    }
}

void AFMainFrame::qslotToggleBlockAreaByToggleButton()
{
    if (ui->pushButton_BlockToggle->isChecked())
    {
        _ShowBlockArea(0);
    }
    else
    {
        _HideBlockArea(0);
    }
}

void AFMainFrame::qslotPreviewResizeTriggered()
{
    _MoveBlocks();
}

void AFMainFrame::qslotToggleBlockFromBlockArea(bool show, int key)
{
    _PopupRequest(show, key);

    m_MainGuideWidget->deleteLater();
    m_MainGuideWidget = nullptr;
}

void AFMainFrame::qslotToggleBlockFromMenu(bool show)
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    QString whatsThisAction = action->whatsThis();
    std::string str = whatsThisAction.toStdString();
     
    QMetaEnum BlockTypeEnum = QMetaEnum::fromType<ENUM_BLOCK_TYPE>();
    int blockType = BlockTypeEnum.keyToValue(str.c_str());

    _PopupRequest(true, blockType);
}

void AFMainFrame::qslotShowStudioSettingPopup(bool show)
{
    _ShowSettingPopup();
}

void AFMainFrame::qslotShowStudioSettingWithButtonSender()
{
    m_DynamicCompositMainWindow->qslotText();

    QPushButton* pushButtonSender = reinterpret_cast<QPushButton*>(sender());

    bool ok;
    int settingPage = pushButtonSender->property("type").toInt(&ok);
    if (!ok) {
        settingPage = 0;
    }

    if (settingPage == 1)
    {
        QString id = pushButtonSender->property("channelID").toString();
        _ShowSettingPopupWithID(id);
    }
    else
    {
        _ShowSettingPopup(settingPage);
    }

}

void AFMainFrame::qslotBlockToggleTriggered(bool show, int blockType)
{
    if (m_BlockPopup) {
        m_BlockPopup->BlockButtonToggled(show, blockType);
    }

    switch (blockType)
    {
    case ENUM_BLOCK_TYPE::SceneSource:
        ui->action_SceneSource->setChecked(show);
        break;
    case ENUM_BLOCK_TYPE::AudioMixer:
        ui->action_AudioMixer->setChecked(show);
        break;
    case ENUM_BLOCK_TYPE::Chat:
        ui->action_Chat->setChecked(show);
        if(m_BlockPopup)
            m_BlockPopup->BlockButtonToggled(show, blockType);
        break;
    case ENUM_BLOCK_TYPE::Dashboard:
        ui->action_Dashboard->setChecked(show);
        if(m_BlockPopup)
            m_BlockPopup->BlockButtonToggled(show, blockType);
        break;
    default:
        break;
    }
}

void AFMainFrame::qslotShowCPUSystemAlert()
{
    ShowSystemAlert(QTStr("Basic.Status.Alert.CPU"));
}

void AFMainFrame::qslotShowMemorySystemAlert() 
{
    ShowSystemAlert(QTStr("Basic.Status.Alert.Memory"));
}

void AFMainFrame::qslotShowNetworkSystemAlert()
{
    ShowSystemAlert(QTStr("Basic.Status.Alert.Network"));
}

void AFMainFrame::qslotCheckDiskSpaceRemaining()
{
    if(_LowDiskSpace()) {
        qslotStopRecording();
        qslotStopReplayBuffer();

        _DiskSpaceMessage();
    }
}

void AFMainFrame::qslotScreenChanged(QScreen* screen)
{
    QList<QWidget*> widgets = this->findChildren<QWidget*>();
    
    foreach(QWidget * w, widgets)
        w->update(rect());
}

void AFMainFrame::qslotDpiChanged(qreal rel)
{
    qDebug() << "dpi changed:" << rel;
}


void AFMainFrame::qslotShowVolumeSlider()
{
    if (!ui->widget_Volume->isEnabled())
        return;

    m_VolumeTimer = new QTimer(this);
    m_VolumeTimer->setInterval(300);
    m_VolumeTimer->setSingleShot(true);
    
    m_AudioPeakUpdateTimer = new QTimer(this);
    m_AudioPeakUpdateTimer->setInterval(50);
    
    connect(m_VolumeTimer, &QTimer::timeout, this, &AFMainFrame::qSlotCloseVolumeSlider);
    connect(m_AudioPeakUpdateTimer, &QTimer::timeout, this, &AFMainFrame::qslotSetAudioPeakValue);

    QPoint globalPos = ui->widget_Volume->mapToGlobal(QPoint(0, 0));
    globalPos.setX(globalPos.x() - 6);
    globalPos.setY(globalPos.y() - m_VolumeSliderFrame->height() + 33);
    m_VolumeSliderFrame->move(globalPos);

    m_VolumeSliderFrame->show();
    m_VolumeTimer->start();
    m_AudioPeakUpdateTimer->start();
}

void AFMainFrame::qslotShowMicSlider()
{
    if (!ui->widget_Mic->isEnabled())
        return;

    m_Mictimer = new QTimer(this);
    m_Mictimer->setInterval(300);
    m_Mictimer->setSingleShot(true);

    m_MicPeakUpdateTimer = new QTimer(this);
    m_MicPeakUpdateTimer->setInterval(50);

    connect(m_Mictimer, &QTimer::timeout, this, &AFMainFrame::qSlotCloseMicSlider);
    connect(m_MicPeakUpdateTimer, &QTimer::timeout, this, &AFMainFrame::qslotSetMicPeakValue);

    QPoint globalPos = ui->widget_Mic->mapToGlobal(QPoint(0, 0));
    globalPos.setX(globalPos.x() - 6);
    globalPos.setY(globalPos.y() - m_MicSliderFrame->height() + 33);
    m_MicSliderFrame->move(globalPos);
    
    m_MicSliderFrame->show();
    m_Mictimer->start();
    m_MicPeakUpdateTimer->start();
}

void AFMainFrame::qSlotCloseVolumeSlider()
{
    int volumeSize = m_VolumeSliderFrame->VolumeSize();
    bool volumechecked = m_VolumeSliderFrame->ButtonIsChecked();
    
    disconnect(m_AudioPeakUpdateTimer, &QTimer::timeout, this, &AFMainFrame::qslotSetAudioPeakValue);
    m_VolumeSliderFrame->hide();
    m_AudioPeakUpdateTimer->stop();
}


void AFMainFrame::qSlotCloseMicSlider()
{
    int volumeSize = m_MicSliderFrame->VolumeSize();
    bool micchecked = m_MicSliderFrame->ButtonIsChecked();

    disconnect(m_MicPeakUpdateTimer, &QTimer::timeout, this, &AFMainFrame::qslotSetMicPeakValue);
    m_MicSliderFrame->hide();
    m_MicPeakUpdateTimer->stop();
}

void AFMainFrame::qslotSetAudioPeakValue() 
{
    if (!GetMainWindow()->IsMainAudioVolumeControlExist())
        return;
    if (!m_VolumeSliderFrame)
        return;

    float peak = GetMainWindow()->GetMainAudioVolumePeak();
    if (m_VolumeSliderFrame->isVisible())
        m_VolumeSliderFrame->SetVolumePeak(peak);
}

void AFMainFrame::qslotSetMicPeakValue() 
{
    if (!GetMainWindow()->IsMainMicVolumeControlExist())
        return;
    if (!m_MicSliderFrame)
        return;

    float peak = GetMainWindow()->GetMainMicVolumePeak();
    if (m_MicSliderFrame->isVisible())
        m_MicSliderFrame->SetVolumePeak(peak);
}

void AFMainFrame::qslotStopVolumeTimer()
{
    m_VolumeTimer->stop();
}

void AFMainFrame::qslotStopMicTimer()
{
    m_Mictimer->stop();
}

void AFMainFrame::qslotMainAudioValueChanged(int volume)
{
    m_DynamicCompositMainWindow->MainAudioVolumeChanged(volume);
}

void AFMainFrame::qslotMainMicValueChanged(int volume)
{
    m_DynamicCompositMainWindow->MainMicVolumeChanged(volume);
}

void AFMainFrame::qslotSetVolumeMute()
{
    m_DynamicCompositMainWindow->SetMainAudioMute();
}

void AFMainFrame::qslotSetMicMute()
{
    m_DynamicCompositMainWindow->SetMainMicMute();
}

void AFMainFrame::qslotTopMenuDestoryed()
{
    m_bTopMenuTriggered = false;
}

void AFMainFrame::qslotShowMainAuthMenu()
{
    AFLocaleTextManager& localeManager = AFLocaleTextManager::GetSingletonInstance();

    AFMainAccountButton* accountButton = reinterpret_cast<AFMainAccountButton*>(sender());
    m_qCurrentAccountButton = accountButton;

    if (AFAuthManager::GetSingletonInstance().IsSoopGlobalRegistered())
    {
        AFChannelData* channelData = accountButton->GetChannelData();
        AFQBalloonWidget* authMenu = new AFQBalloonWidget();
        authMenu->BalloonWidgetInit(QMargins(6, 10, 6, 10), 0);
        authMenu->setAttribute(Qt::WA_DeleteOnClose);

        QLabel* profilepic = new QLabel(authMenu);
        profilepic->setFixedSize(24, 24);
        profilepic->setObjectName("label_AuthMenuThumbnail");
        
        QPixmap* savedPixmap = (QPixmap*)channelData->pObjQtPixmap;
        if (savedPixmap != nullptr)
        {
            QPixmap scaledPixmap = savedPixmap->scaled(24,24,
                                                       Qt::IgnoreAspectRatio,
                                                       Qt::SmoothTransformation);
            profilepic->setPixmap(scaledPixmap);
        }

        AFQElidedSlideLabel* profileNick = new AFQElidedSlideLabel(authMenu);
        AFBasicAuth* rawAuthData = channelData->pAuthData;
        QString nick = rawAuthData->strChannelID.c_str();
        QString platform = rawAuthData->strPlatform.c_str();
        profileNick->setText(nick);
        profileNick->setToolTip(nick);
        profileNick->setObjectName("label_AuthMenuNick");
        profileNick->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QHBoxLayout* profileLayout = new QHBoxLayout();
        profileLayout->setSpacing(6);
        profileLayout->setContentsMargins(10, 2, 10, 2);

        profileLayout->addWidget(profilepic);
        profileLayout->addWidget(profileNick);

        QWidget* profileWidget = new QWidget(authMenu);
        profileWidget->setLayout(profileLayout);
        profileWidget->setFixedSize(168, 28);

        //----------------------------------------------------

        QLabel* channelLabel = new QLabel(authMenu);
        channelLabel->setText(QTStr("Basic.Settings.Audio.Channels"));
        channelLabel->setObjectName("label_AuthMenuChannel");

        AFQToggleButton* togglebutton = new AFQToggleButton(authMenu);
        togglebutton->setFixedSize(30, 16);
        togglebutton->ChangeState(channelData->bIsStreaming);
        togglebutton->setChecked(channelData->bIsStreaming);

        connect(togglebutton, &QPushButton::clicked, authMenu, &AFQBalloonWidget::qsignalFromChild);
        connect(authMenu, &AFQBalloonWidget::qsignalFromChild, this, &AFMainFrame::qslotStopSimulcast);

        QHBoxLayout* channelLayout = new QHBoxLayout();
        channelLayout->setSpacing(6);
        channelLayout->setContentsMargins(10, 2, 10, 2);

        channelLayout->addWidget(channelLabel);
        channelLayout->addWidget(togglebutton);

        QWidget* channelWidget = new QWidget(authMenu);
        channelWidget->setLayout(channelLayout);
        channelWidget->setFixedSize(168, 28);

        //----------------------------------------------------

        //Setting
        QPushButton* setting = new QPushButton(authMenu);
        setting->setFixedHeight(28);
        setting->setText(QTStr("Settings"));
        setting->setObjectName("pushButton_AuthSetting");
        setting->setProperty("type", "1");
        setting->setProperty("channelID", nick);


        connect(setting, &QPushButton::clicked, this, &AFMainFrame::qslotShowStudioSettingWithButtonSender);

        QHBoxLayout* settingLayout = new QHBoxLayout();
        settingLayout->setSpacing(0);
        settingLayout->setContentsMargins(0, 0, 0, 0);

        settingLayout->addWidget(setting);

        QWidget* settingWidget = new QWidget(authMenu);
        settingWidget->setLayout(settingLayout);
        settingWidget->setFixedSize(168, 28);

        //----------------------------------------------------

        //DashBoard
        QPushButton* news = new QPushButton(authMenu);
        news->setFixedHeight(28);
        news->setText(QTStr("Block.Tooltip.Dashboard"));
        news->setObjectName("pushButton_AuthMenuDashboard");
        news->setProperty("type", "Dashboard");
        news->setProperty("channelID", nick);
        news->setProperty("platform", platform);

        connect(news, &QPushButton::clicked, this, &AFMainFrame::qslotShowGlobalPageSender);

        QHBoxLayout* newsLayout = new QHBoxLayout();
        newsLayout->setSpacing(0);
        newsLayout->setContentsMargins(0, 0, 0, 0);

        newsLayout->addWidget(news);

        QWidget* newsWidget = new QWidget(authMenu);
        newsWidget->setLayout(newsLayout);
        newsWidget->setFixedSize(168, 28);

        authMenu->AddWidgetToBalloon(profileWidget);
        authMenu->AddWidgetToBalloon(channelWidget);
        authMenu->AddWidgetToBalloon(settingWidget);
        authMenu->AddWidgetToBalloon(newsWidget);

        int x = pos().x() + accountButton->pos().x() - (authMenu->width() / 2) + 32;
        int y = pos().y() + ui->frame_Top->height() + ui->widget_Middle->height() - authMenu->height() + 3;

        authMenu->ShowBalloon(QPoint(x, y));
    }
    else
    {
        if (IsStreamActive()) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                "", QTStr("Simulcast.Start.Denied"), false, true);
            return;
        }
        
        if (IsRecordingActive()) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                "", QTStr("Simulcast.Rec.Start.Denied"), false, true);
            return;
        }
        
        AFAddStreamWidget* addStream = new AFAddStreamWidget(this);
        addStream->AddStreamWidgetInit("SOOP Global");

        if (addStream->exec() == QDialog::Accepted)
        {
            AFBasicAuth& resAuth = addStream->GetRawAuth();
            AFChannelData* newChannel = new AFChannelData();
            resAuth.strPlatform = addStream->GetPlatform().toStdString();
            const char* uuid = QUuid::createUuid().toString().toStdString().c_str();
            newChannel->bIsStreaming = true;
            auto& authManager = AFAuthManager::GetSingletonInstance();
            authManager.RegisterChannel(uuid, newChannel);

            auto& auth = AFAuthManager::GetSingletonInstance();
            //auth.FlushAuthCache();
            //auth.FlushAuthMain();
            auth.SaveAllAuthed();

            SetStreamingOutput();
            LoadAccounts();
        }
    }
}

void AFMainFrame::qslotShowOtherAuthMenu()
{
    AFLocaleTextManager& localeManager = AFLocaleTextManager::GetSingletonInstance();

    AFMainAccountButton* accountButton = reinterpret_cast<AFMainAccountButton*>(sender());
    m_qCurrentAccountButton = accountButton;

    AFChannelData* channelData = accountButton->GetChannelData();

    AFQBalloonWidget* authMenu = new AFQBalloonWidget();
    authMenu->BalloonWidgetInit(QMargins(6, 10, 6, 10), 0);
    authMenu->setAttribute(Qt::WA_DeleteOnClose);

    //Auth ThumbNail
    QLabel* profilepic = new QLabel(authMenu);
    profilepic->setFixedSize(24, 24);
    profilepic->setObjectName("label_AuthMenuThumbnail");
    
    QPixmap* savedPixmap = (QPixmap*)channelData->pObjQtPixmap;
    if (savedPixmap != nullptr)
    {
        QPixmap scaledPixmap = savedPixmap->scaled(24,24,
                                                   Qt::IgnoreAspectRatio,
                                                   Qt::SmoothTransformation);
        profilepic->setPixmap(scaledPixmap);
    }

    AFQElidedSlideLabel* profileNick = new AFQElidedSlideLabel(authMenu);
    AFBasicAuth* rawAuthData = channelData->pAuthData;
    QString nick = rawAuthData->strChannelID.c_str();
    QString platform = rawAuthData->strPlatform.c_str();
    profileNick->setText(nick);
    profileNick->setToolTip(nick);
    profileNick->setObjectName("label_AuthMenuNick");
    profileNick->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    /*
    connect(this, &AFMainFrame::qsignalHoverEnter, profileNick,
            &AFQElidedSlideLabel::qSlotHoverButton);
    connect(this, &AFMainFrame::qSignalLeaveButton, profileNick,
            &AFQElidedSlideLabel::qSlotLeaveButton);
    */

    QHBoxLayout* profileLayout = new QHBoxLayout();
    profileLayout->setSpacing(6);
    profileLayout->setContentsMargins(10, 2, 10, 2);

    profileLayout->addWidget(profilepic);
    profileLayout->addWidget(profileNick);

    QWidget* profileWidget = new QWidget(authMenu);
    profileWidget->setLayout(profileLayout);
    profileWidget->setFixedSize(168, 28);

    //----------------------------------------------------

    //Setting
    QPushButton* setting = new QPushButton(authMenu);
    setting->setFixedHeight(28);
    setting->setText(QTStr("Settings"));
    setting->setObjectName("pushButton_AuthSetting");
    setting->setProperty("type", "1");
    setting->setProperty("channelID", nick);


    connect(setting, &QPushButton::clicked, this, &AFMainFrame::qslotShowStudioSettingWithButtonSender);

    QHBoxLayout* settingLayout = new QHBoxLayout();
    settingLayout->setSpacing(0);
    settingLayout->setContentsMargins(0, 0, 0, 0);

    settingLayout->addWidget(setting);

    QWidget* settingWidget = new QWidget(authMenu);
    settingWidget->setLayout(settingLayout);
    settingWidget->setFixedSize(168, 28);

    //----------------------------------------------------

    //DashBoard
    QPushButton* news = new QPushButton(authMenu);
    news->setFixedHeight(28);
    news->setText(QTStr("Block.Tooltip.Dashboard"));
    news->setObjectName("pushButton_AuthMenuDashboard");
    news->setProperty("type", "Dashboard");
    news->setProperty("channelID", nick);
    news->setProperty("platform", platform);

    connect(news, &QPushButton::clicked, this, &AFMainFrame::qslotShowGlobalPageSender);

    QHBoxLayout* newsLayout = new QHBoxLayout();
    newsLayout->setSpacing(0);
    newsLayout->setContentsMargins(0, 0, 0, 0);

    newsLayout->addWidget(news);

    QWidget* newsWidget = new QWidget(authMenu);
    newsWidget->setLayout(newsLayout);
    newsWidget->setFixedSize(168, 28);

    //----------------------------------------------------

    QPushButton* disconnect = new QPushButton(authMenu);
    disconnect->setFixedHeight(28);
    disconnect->setText(QTStr("Simulcast.Stop"));
    disconnect->setObjectName("pushButton_AuthMenuDashboard");
    disconnect->setProperty("type", "StopSimulcast");

    connect(disconnect, &QPushButton::clicked, authMenu, &AFQBalloonWidget::qsignalFromChild);
    connect(authMenu, &AFQBalloonWidget::qsignalFromChild, this, &AFMainFrame::qslotStopSimulcast);

    QHBoxLayout* channelLayout = new QHBoxLayout();
    channelLayout->setSpacing(0);
    channelLayout->setContentsMargins(0, 0, 0, 0);

    channelLayout->addWidget(disconnect);

    QWidget* channelWidget = new QWidget(authMenu);
    channelWidget->setLayout(channelLayout);
    channelWidget->setFixedSize(168, 28);

    authMenu->AddWidgetToBalloon(profileWidget);
    authMenu->AddWidgetToBalloon(settingWidget);
    authMenu->AddWidgetToBalloon(newsWidget);
    authMenu->AddWidgetToBalloon(channelWidget);

    int x = pos().x() + accountButton->pos().x() - (authMenu->width() / 2) + 32;
    int y = pos().y() + ui->frame_Top->height() + ui->widget_Middle->height() - authMenu->height() + 3;

    authMenu->ShowBalloon(QPoint(x, y));
}

void AFMainFrame::qslotShowLoginMenu()
{
    if (m_MainGuideWidget)
    {
        m_MainGuideWidget->deleteLater();
        m_MainGuideWidget = nullptr;
    }

    if (m_ProgramGuideWidget && m_ProgramGuideWidget->isWidgetType())
        m_ProgramGuideWidget->NextMission(1);

    if (ui->stackedWidget_Platform->currentIndex() != 0)
    {
        return;
    }

    if (IsRecordingActive()) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
            "", QTStr("Simulcast.Rec.Start.Denied"), false, true);
        return;
    }
    
    AFQBalloonWidget* loginBalloon = new AFQBalloonWidget(this);
    loginBalloon->BalloonWidgetInit(QMargins(6,6,6,6), 4);
    loginBalloon->setAttribute(Qt::WA_DeleteOnClose);

    QPushButton* soopgButton = new QPushButton(loginBalloon);
    soopgButton->setFixedSize(150, 36);
    soopgButton->setObjectName("pushButton_MenuSoopGlobal");
    soopgButton->setProperty("platform", "SOOP Global");
    connect(soopgButton, &QPushButton::clicked,
        this, &AFMainFrame::qslotLoginAccount);
    loginBalloon->AddWidgetToBalloon(soopgButton);

    QPushButton* soopButton = new QPushButton(loginBalloon);
    soopButton->setFixedSize(150, 36);
    soopButton->setObjectName("pushButton_MenuSoop");
    soopButton->setProperty("platform", "afreecaTV");
    connect(soopButton, &QPushButton::clicked,
        this, &AFMainFrame::qslotLoginAccount);
    loginBalloon->AddWidgetToBalloon(soopButton);

    QPushButton* twitchButton = new QPushButton(loginBalloon);
    twitchButton->setFixedSize(150, 36);
    twitchButton->setObjectName("pushButton_MenuTwitch");
    twitchButton->setProperty("platform", "Twitch");
    connect(twitchButton, &QPushButton::clicked,
        this, &AFMainFrame::qslotLoginAccount);
    loginBalloon->AddWidgetToBalloon(twitchButton);

    QPushButton* youtubeButton = new QPushButton(loginBalloon);
    youtubeButton->setFixedSize(150, 36);
    youtubeButton->setObjectName("pushButton_MenuYoutube");
    youtubeButton->setProperty("platform", "Youtube");
    youtubeButton->setWhatsThis("1");
    connect(youtubeButton, &QPushButton::clicked,
        this, &AFMainFrame::qslotLoginAccount);
    loginBalloon->AddWidgetToBalloon(youtubeButton);

    QPushButton* rtmpButton = new QPushButton(loginBalloon);
    rtmpButton->setFixedSize(150, 36);
    rtmpButton->setText("+ RTMP");
    rtmpButton->setObjectName("pushButton_MenuRtmp");
    rtmpButton->setProperty("platform", "Custom RTMP");
    rtmpButton->setWhatsThis("1");
    connect(rtmpButton, &QPushButton::clicked,
        this, &AFMainFrame::qslotLoginAccount);
    loginBalloon->AddWidgetToBalloon(rtmpButton);

    int x = pos().x() - ui->pushButton_Login->width() / 2 - 2;
    int y = pos().y() + ui->frame_Top->height() + ui->widget_Middle->height() - loginBalloon->height() + 3;

    loginBalloon->ShowBalloon(QPoint(x, y));
}

void AFMainFrame::qslotStopSimulcast(bool checked)
{
    if (m_qCurrentAccountButton)
    {
        if (checked)
        {
            if (IsStreamActive())
            {
                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                    "", QTStr("Simulcast.Start.Denied"), false, true);
                return;
            }
            else
            {
                m_qCurrentAccountButton->qslotStartStream();
            }
        }
        else
        {
            if (IsStreamActive())
            {
                QList<AFMainAccountButton*> accountList = findChildren<AFMainAccountButton*>();

                int liveCount = 0;
                foreach (AFMainAccountButton* account, accountList) {
                    if (!account || !account->GetChannelData())
                        continue;
                    if (account->GetChannelData()->bIsStreaming)
                        liveCount++;
                }
                if (liveCount == 1)
                {
                    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                        "", QTStr("Simulcast.Stop.Denied"), false, true);
                    return;
                }
            }

            m_qCurrentAccountButton->qslotQuitStream();
        }

        
        SetStreamingOutput();
        LoadAccounts();

        QWidget* balloon = reinterpret_cast<QWidget*>(sender());
        balloon->close();
    }
}

void AFMainFrame::qslotLoginAccount()
{
    QPushButton* authButton = reinterpret_cast<QPushButton*>(sender());
    QString platform = authButton->property("platform").toString();

    AFAddStreamWidget* addStream = new AFAddStreamWidget(this);
    addStream->AddStreamWidgetInit(platform);

    if (addStream->exec() == QDialog::Accepted)
    {
        AFBasicAuth& resAuth = addStream->GetRawAuth();
        AFChannelData* newChannel = new AFChannelData();
        std::string strUuid = QUuid::createUuid().toString().toStdString();
        const char * uuid = strUuid.c_str();
        resAuth.strUuid = uuid;
        newChannel->bIsStreaming = true;

        auto& authManager = AFAuthManager::GetSingletonInstance();

        std::string platform = resAuth.strPlatform;
        if (platform == "SOOP Global") {
            authManager.CacheAuth(true, resAuth);
        } else {
            authManager.CacheAuth(false, resAuth);
        }
        authManager.RegisterChannel(uuid, newChannel);

        //auth.FlushAuthCache();
        //auth.FlushAuthMain();
        authManager.SaveAllAuthed();

        SetStreamingOutput();
    }

    LoadAccounts();
}


bool AFMainFrame::qslotShowGlobalPage(QString type)
{
    auto& authManager = AFAuthManager::GetSingletonInstance();

    AFChannelData* mainChannel = nullptr;
    if (!authManager.IsSoopGlobalRegistered()) {
        if (IsStreamActive()) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                "", QTStr("Simulcast.Start.Denied"), false, true);
            return false;
        }
        
        if (IsRecordingActive()) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                "", QTStr("Simulcast.Rec.Start.Denied"), false, true);
            return false;
        }
        
        AFAddStreamWidget* addStream = new AFAddStreamWidget(this);
        addStream->AddStreamWidgetInit("SOOP Global");

        if (addStream->exec() == QDialog::Accepted)
        {
            AFBasicAuth& resAuth = addStream->GetRawAuth();
            AFChannelData* newChannel = new AFChannelData();
            resAuth.strPlatform = addStream->GetPlatform().toStdString();
            const char * uuid = QUuid::createUuid().toString().toStdString().c_str();
            newChannel->bIsStreaming = true;
            auto& authManager = AFAuthManager::GetSingletonInstance();
            authManager.RegisterChannel(uuid, newChannel);

            auto& auth = AFAuthManager::GetSingletonInstance();
            //auth.FlushAuthCache();
            //auth.FlushAuthMain();
            auth.SaveAllAuthed();

            SetStreamingOutput();
            LoadAccounts();
        }
        else
            return false;
    }

    bool accountValid = true;

    if (type == "Chat") {
        if (!m_GlobalSoopChatWidget) 
            accountValid = _SetupChatPage();
        
        if (accountValid)
        {
            if (m_GlobalSoopChatWidget->isMaximized())
                m_GlobalSoopChatWidget->showMaximized();
            else
            {
                if (m_GlobalSoopChatWidget->isVisible()) {
                    m_GlobalSoopChatWidget->showNormal();
                    m_GlobalSoopChatWidget->raise();
                }
                else
                {
                    QRect adjustRect;
                    QRect position = QRect(x() - m_GlobalSoopChatWidget->width(), y(),
                        m_GlobalSoopChatWidget->width(), m_GlobalSoopChatWidget->height());
                    m_DynamicCompositMainWindow->AdjustPositionOutSideScreen(position, adjustRect);
                    m_GlobalSoopChatWidget->setGeometry(adjustRect);

                    m_GlobalSoopChatWidget->showNormal();
                }
            }
        }            
    }
    else if (type == "Newsfeed") 
    {
        if (!m_GlobalSoopNewsfeedWidget) 
            _SetupNewsfeedPage();
        
        if (m_GlobalSoopNewsfeedWidget->isMaximized())
            m_GlobalSoopNewsfeedWidget->showMaximized();
        else
        {
            if (m_GlobalSoopNewsfeedWidget->isVisible()) {
                m_GlobalSoopNewsfeedWidget->showNormal();
                m_GlobalSoopNewsfeedWidget->raise();
            }
            else
            {
                QRect adjustRect;
                QRect position = QRect(x() - m_GlobalSoopNewsfeedWidget->width(), y(),
                    m_GlobalSoopNewsfeedWidget->width(), m_GlobalSoopNewsfeedWidget->height());
                m_DynamicCompositMainWindow->AdjustPositionOutSideScreen(position, adjustRect);
                m_GlobalSoopNewsfeedWidget->setGeometry(adjustRect);

                m_GlobalSoopNewsfeedWidget->showNormal();
            }
        }
    }
    return true;
}

void AFMainFrame::qslotShowGlobalPageSender()
{
    QPushButton* button = reinterpret_cast<QPushButton*>(sender());
    if (button) {
        QString platform = button->property("platform").toString();
        
        if (platform == "SOOP Global") {
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
            dashboard_url = TWITCH_DASHBOARD_URL + button->property("channelID").toString() + "/home";
            QDesktopServices::openUrl(QUrl(dashboard_url));
        }
        else if (platform == "Youtube") {
            auto& authManager = AFAuthManager::GetSingletonInstance();
            int cntOfAccount = authManager.GetCntChannel();
            for (int idx = 0; idx < cntOfAccount; idx++)
            {
                AFChannelData* tmpChannel = nullptr;
                authManager.GetChannelData(idx, tmpChannel);
                if (!tmpChannel || !tmpChannel->pAuthData)
                    continue;
                if (tmpChannel->pAuthData->strPlatform == "Youtube")
                {
                    ShowPopupPageYoutubeChannel(tmpChannel->pAuthData);
                    break;
                }
            }
        }        
    }    
}

void AFMainFrame::qslotLogoutGlobalPage()
{
    if (m_BlockPopup)
    {
        m_BlockPopup->BlockButtonToggled(false, ENUM_BLOCK_TYPE::Chat);
        m_BlockPopup->BlockButtonToggled(false, ENUM_BLOCK_TYPE::Dashboard);
    }

    if (m_GlobalSoopChatWidget) {
        delete m_GlobalSoopChatWidget;
        m_GlobalSoopChatWidget = nullptr;
    }
    if (m_GlobalSoopNewsfeedWidget) {
        delete m_GlobalSoopNewsfeedWidget;
        m_GlobalSoopNewsfeedWidget = nullptr;
    }
}

void AFMainFrame::qslotCloseGlobalSoopPage(QString type)
{
    AFQBorderPopupBaseWidget* pPageWidget = nullptr;
    if (type == "Chat") {
        pPageWidget = qobject_cast<AFQBorderPopupBaseWidget*> (m_GlobalSoopChatWidget);
        if (m_BlockPopup)
            m_BlockPopup->BlockButtonToggled(false, ENUM_BLOCK_TYPE::Chat);        
    }
    else if (type == "Newsfeed") {
        pPageWidget = qobject_cast<AFQBorderPopupBaseWidget*> (m_GlobalSoopNewsfeedWidget);
        if (m_BlockPopup)
            m_BlockPopup->BlockButtonToggled(false, ENUM_BLOCK_TYPE::Dashboard);
    }
    if (!pPageWidget)
        return;
}

void AFMainFrame::qslotHideGlobalSoopPage(QString type)
{
    AFQBorderPopupBaseWidget* pPageWidget = nullptr;
    if (type == "Chat") {
        pPageWidget = qobject_cast<AFQBorderPopupBaseWidget*> (m_GlobalSoopChatWidget);
        if (m_BlockPopup)
            m_BlockPopup->BlockButtonToggled(false, ENUM_BLOCK_TYPE::Chat);
    }
    else if (type == "Newsfeed") {
        pPageWidget = qobject_cast<AFQBorderPopupBaseWidget*> (m_GlobalSoopNewsfeedWidget);
        if (m_BlockPopup)
            m_BlockPopup->BlockButtonToggled(false, ENUM_BLOCK_TYPE::Dashboard);
    }
    if (!pPageWidget)
        return;
    pPageWidget->hide();

}

void AFMainFrame::qslotMaximizeGlobalSoopPage(QString type)
{
    AFQBorderPopupBaseWidget* pPageWidget = nullptr;
    if (type == "Chat") {
        pPageWidget = qobject_cast<AFQBorderPopupBaseWidget*> (m_GlobalSoopChatWidget);
    }
    else if (type == "Newsfeed") {
        pPageWidget = qobject_cast<AFQBorderPopupBaseWidget*> (m_GlobalSoopNewsfeedWidget);
    }
    if (!pPageWidget)
        return;

    if (pPageWidget->isMaximized()) {
        pPageWidget->showNormal();
    }
    else {
        pPageWidget->RestoreNormalWidth(pPageWidget->width());
        pPageWidget->showMaximized();
    }

    AFDockTitle* dockTitle = qobject_cast<AFDockTitle*>(pPageWidget->GetWidgetByName("CustomBrowserDockTitle"));
    if (dockTitle) {
        dockTitle->ChangeMaximizedIcon(pPageWidget->isMaximized());
    }
}

void AFMainFrame::qslotMinimizeGlobalSoopPage(QString type)
{
    AFQBorderPopupBaseWidget* pPageWidget = nullptr;
    if (type == "Chat") {
        pPageWidget = qobject_cast<AFQBorderPopupBaseWidget*> (m_GlobalSoopChatWidget);
    }
    else if (type == "Newsfeed") {
        pPageWidget = qobject_cast<AFQBorderPopupBaseWidget*> (m_GlobalSoopNewsfeedWidget);
    }
    if (!pPageWidget)
        return;
    pPageWidget->showMinimized();
}

void AFMainFrame::qslotExtendResource()
{
    if (m_ResourceExtensionWidget == nullptr)
    {
        ui->label_ResourceExtension->setProperty("resourceExtend", true);

        m_ResourceExtensionWidget = new AFResourceExtension(this);
        m_ResourceExtensionWidget->ResourceExtensionInit();
        ui->frame_Bottom->layout()->addWidget(m_ResourceExtensionWidget);
        ui->frame_Bottom->setMinimumHeight(142);
        ui->frame_Bottom->setMaximumHeight(142);
        if (!isMaximized())
            resize(QSize(width(), height() + 42));
        setMinimumHeight(750);

        // qslotCheckDiskSpaceRemaining
        connect(m_ResourceExtensionWidget, &AFResourceExtension::qsignalCheckDiskSpaceRemaining,
                this, &AFMainFrame::qslotCheckDiskSpaceRemaining);
        connect(m_ResourceExtensionWidget, &AFResourceExtension::qsignalStatWindowTriggered, this,
            &AFMainFrame::qslotStatsOpenTriggered);
    }
    else
    {
        ui->label_ResourceExtension->setProperty("resourceExtend", false);

        m_ResourceExtensionWidget->close();
        m_ResourceExtensionWidget->deleteLater();
        ui->frame_Bottom->setMinimumHeight(100);
        ui->frame_Bottom->setMaximumHeight(100);
        setMinimumHeight(708);
        if (!isMaximized())
            resize(QSize(width(), height() - 42));
    }

    style()->unpolish(ui->label_ResourceExtension);
    style()->polish(ui->label_ResourceExtension);
}

void AFMainFrame::qslotReplayBufferSave()
{
    if (!IsReplayBufferActive())
        return;

    calldata_t cd = { 0 };
    proc_handler_t* ph = obs_output_get_proc_handler(m_outputHandlers[0].second->replayBuffer);
    proc_handler_call(ph, "save", &cd);
    calldata_free(&cd);
}
void AFMainFrame::qslotReplayBufferSaved()
{
    if (!IsReplayBufferActive())
        return;

    calldata_t cd = { 0 };
    proc_handler_t* ph = obs_output_get_proc_handler(m_outputHandlers[0].second->replayBuffer);
    proc_handler_call(ph, "get_last_replay", &cd);
    std::string path = calldata_string(&cd, "path");
    QString msg = QTStr("Basic.StatusBar.ReplayBufferSavedTo").arg(QT_UTF8(path.c_str()));

    //    ShowStatusBarMessage(msg);
    m_statusbar.qslotClearMessage();
    m_statusbar.qslotShowMessage(msg, 10000);
    //lastReplay = path;
    calldata_free(&cd);

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED);

    _AutoRemux(QT_UTF8(path.c_str()));

    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                               "", msg, false, true);
}

void AFMainFrame::qslotAddSourceMenu()
{
    AFQAddSourceMenuButton* button = qobject_cast<AFQAddSourceMenuButton*>(sender());

    QString id = button->GetSourceId();

    if (0 == id.compare("scene")) {
        ShowSceneSourceSelectList();
        return;
    }

    OBSSource newSource;
    QString displayText = AFSourceUtil::GetPlaceHodlerText(id.toStdString().c_str());
    if (!AFSourceUtil::AddNewSource(this, id.toStdString().c_str(), displayText.toStdString().c_str(), true, newSource))
        return;

    if (AFSourceUtil::ShouldShowProperties(newSource))
        CreatePropertiesPopup(newSource);
}

void AFMainFrame::qslotShowSelectSourcePopup()
{
    AFQSelectSourceDialog sourceSelect(nullptr);
    setCenterPositionNotUseParent(&sourceSelect, this);

    if (sourceSelect.exec() != QDialog::DialogCode::Accepted)
        return;

    QString id = sourceSelect.m_sourceId;
    if (0 == id.compare("scene")) {
        ShowSceneSourceSelectList();
        return;
    }

    OBSSource newSource;
    QString displayText = AFSourceUtil::GetPlaceHodlerText(id.toStdString().c_str());
    if (!AFSourceUtil::AddNewSource(this, id.toStdString().c_str(), displayText.toStdString().c_str(), true, newSource))
        return;

    if (AFSourceUtil::ShouldShowProperties(newSource))
        CreatePropertiesPopup(newSource);
}

void AFMainFrame::qslotSceneButtonClicked(OBSScene scene)
{
    if (!scene)
        return;

    obs_source_t* source = obs_scene_get_source(scene);

    AFMainDynamicComposit* dynamicComposit = GetMainWindow();
    if (!dynamicComposit)
        return;

    dynamicComposit->SetCurrentScene(source);

}

void AFMainFrame::qslotSceneButtonDoubleClicked(OBSScene scene)
{
    if (!scene)
        return;

    obs_source_t* source = obs_scene_get_source(scene);

    AFMainDynamicComposit* dynamicComposit = GetMainWindow();
    if (!dynamicComposit)
        return;

    UNUSED_PARAMETER(scene);

    dynamicComposit->qslotChangeSceneOnDoubleClick();
}

void AFMainFrame::qslotSceneButtonDotClicked()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    SceneItemVector& sceneItems = sceneContext.GetSceneItemVector();
    const OBSScene curObsScene = sceneContext.GetCurrOBSScene();

    if (sceneItems.empty())
        return;

    AFQCustomMenu popup(this);

    QAction* actionSelectSourcePopup = new QAction(Str("AddSource"), this);
    connect(actionSelectSourcePopup, &QAction::triggered, 
            this, &AFMainFrame::qslotShowSelectSourcePopup);

    popup.addAction(actionSelectSourcePopup);
    popup.addSeparator();

    QAction* actionPopupSceneDock = new QAction(Str("Basic.BottomSceneMenu.ShowSceneSourceDock"), this);
    auto popupSceneDock = [this] {
        _PopupRequest(true, ENUM_BLOCK_TYPE::SceneSource);
    };
    connect(actionPopupSceneDock, &QAction::triggered, popupSceneDock);
    popup.addAction(actionPopupSceneDock);

    popup.addSeparator();

    popup.exec(QCursor::pos());
}

void AFMainFrame::qslotMinimizeWindow()
{
    AFCQMainBaseWidget::qslotMinimizeWindow();
    
    
    if (m_BlockPopup != nullptr)
    {
        if (m_BlockPopup->isVisible())
        {
            m_BlockPopup->hide();
        }
    }
}

void AFMainFrame::qslotIconActivated(QSystemTrayIcon::ActivationReason reason)
{	
#ifdef __APPLE__
    UNUSED_PARAMETER(reason);
#else
    if (reason == QSystemTrayIcon::Trigger) {
        EnablePreviewDisplay(m_bPreviewEnabled && !isVisible());
        qslotToggleShowHide();
    }
#endif
}

void AFMainFrame::qslotSetShowing(bool showing)
{
    auto& configMan = AFConfigManager::GetSingletonInstance();
    auto& localeMan = AFLocaleTextManager::GetSingletonInstance();

    if (!showing && isVisible()) {
        config_set_string(configMan.GetGlobal(), "BasicWindow",
            "geometry",
            saveGeometry().toBase64().constData());

        /* hide all visible child dialogs */
        /*visDlgPositions.clear();
        if (!visDialogs.isEmpty()) {
            for (QDialog* dlg : visDialogs) {
                visDlgPositions.append(dlg->pos());
                dlg->hide();
            }
        }*/

        if (m_qMainShowHideAction)
            m_qMainShowHideAction->setText(QT_UTF8(localeMan.Str(("Basic.SystemTray.Show"))));
        QTimer::singleShot(0, this, &AFMainFrame::hide);

        if (m_bPreviewEnabled)
            EnablePreviewDisplay(false);

#ifdef __APPLE__
        EnableOSXDockIcon(false);
#endif

    }
    else if (showing && !isVisible()) {
        if (m_qMainShowHideAction)
            m_qMainShowHideAction->setText(QT_UTF8(localeMan.Str(("Basic.SystemTray.Hide"))));
        QTimer::singleShot(0, this, &AFMainFrame::show);

        if (m_bPreviewEnabled)
            EnablePreviewDisplay(true);

#ifdef __APPLE__
        EnableOSXDockIcon(true);
#endif

        /* raise and activate window to ensure it is on top */
        //raise();
        activateWindow();

        /* show all child dialogs that was visible earlier */
        /*if (!visDialogs.isEmpty()) {
            for (int i = 0; i < visDialogs.size(); ++i) {
                QDialog* dlg = visDialogs[i];
                dlg->move(visDlgPositions[i]);
                dlg->show();
            }
        }*/

        /* Unminimize window if it was hidden to tray instead of task
         * bar. */
        bool sysTrayMinimizeToTray = config_get_bool(configMan.GetGlobal(), "BasicWindow",
            "SysTrayMinimizeToTray");

        if (sysTrayMinimizeToTray) {
            Qt::WindowStates state;
            state = windowState() & ~Qt::WindowMinimized;
            state |= Qt::WindowActive;
            setWindowState(state);
        }
    }
}

void AFMainFrame::qslotToggleShowHide()
{
    bool showing = isVisible();
    if (showing) {
        /* check for modal dialogs */
        /*EnumDialogs();
        if (!modalDialogs.isEmpty() || !visMsgBoxes.isEmpty())
            return;*/
    }
    qslotSetShowing(!showing);
}

void AFMainFrame::qslotDisplayStreamStartError()
{
    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {

        QString message = !outputIter->second->lastError.empty()
            ? QTStr(outputIter->second->lastError.c_str())
            : QTStr("Output.StartFailedGeneric");
        //
        QMessageBox::critical(this, QTStr("Output.StartStreamFailed"), message);
    }

    if (!IsStreamActive()) {
        _ChangeStreamState(true, false, "LIVE", 77);
        ShowSystemAlert(QTStr("Output.StartStreamFailed"));
    }
}

void AFMainFrame::qslotTogglePreview()
{
    m_bPreviewEnabled = !m_bPreviewEnabled;
    EnablePreviewDisplay(m_bPreviewEnabled);
}

void AFMainFrame::qslotLockPreview()
{
    AFBasicPreview* preview = GetMainWindow()->GetMainPreview();
    if (preview) {
        preview->ToggleLocked();
        ui->action_LockPreview->setChecked(preview->GetLocked());
    }
}

void AFMainFrame::qslotActionScaleWindow()
{
    GetMainWindow()->GetMainPreview()->SetFixedScaling(false);
    GetMainWindow()->GetMainPreview()->ResetScrollingOffset();
    emit GetMainWindow()->GetMainPreview()->qsignalDisplayResized();
}

void AFMainFrame::qslotActionScaleCanvas()
{
    GetMainWindow()->GetMainPreview()->SetFixedScaling(true);
    GetMainWindow()->GetMainPreview()->SetScalingLevel(0);
    emit GetMainWindow()->GetMainPreview()->qsignalDisplayResized();
}

void AFMainFrame::qslotActionScaleOutput()
{
    obs_video_info ovi;
    obs_get_video_info(&ovi);

    GetMainWindow()->GetMainPreview()->SetFixedScaling(true);
    float scalingAmount = float(ovi.output_width) / float(ovi.base_width);
    // log base ZOOM_SENSITIVITY of x = log(x) / log(ZOOM_SENSITIVITY)
    int32_t approxScalingLevel =
        int32_t(round(log(scalingAmount) / log(ZOOM_SENSITIVITY)));
    GetMainWindow()->GetMainPreview()->SetScalingLevel(approxScalingLevel);
    GetMainWindow()->GetMainPreview()->SetScalingAmount(scalingAmount);
    emit GetMainWindow()->GetMainPreview()->qsignalDisplayResized();
}

void AFMainFrame::qSlotActionCopySource()
{
    m_clipboard.clear();

    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    AFQSourceListView* sourceListView = sceneContext.GetSourceListViewPtr();

    for (auto& selectedSource : sourceListView->selectionModel()->selectedIndexes()) {
        OBSSceneItem item = sourceListView->Get(selectedSource.row());
        if (!item)
            continue;

        OBSSource source = obs_sceneitem_get_source(item);

        SourceCopyInfo copyInfo;
        copyInfo.weak_source = OBSGetWeakRef(source);
        obs_sceneitem_get_info(item, &copyInfo.transform);
        obs_sceneitem_get_crop(item, &copyInfo.crop);
        copyInfo.blend_method = obs_sceneitem_get_blending_method(item);
        copyInfo.blend_mode = obs_sceneitem_get_blending_mode(item);
        copyInfo.visible = obs_sceneitem_visible(item);

        m_clipboard.push_back(copyInfo);
    }

    UpdateEditMenu();
}

void AFMainFrame::qSlotActionPasteRefSource()
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	OBSScene scene = sceneContext.GetCurrOBSScene();
	OBSSource scene_source = OBSSource(obs_scene_get_source(scene));

	OBSData undo_data = BackupScene(scene_source);
	m_undo_s.PushDisabled();

	for (size_t i = m_clipboard.size(); i > 0; i--) {
		SourceCopyInfo& copyInfo = m_clipboard[i - 1];

		OBSSource source = OBSGetStrongRef(copyInfo.weak_source);
		if (!source)
			continue;

		const char* name = obs_source_get_name(source);

		/* do not allow duplicate refs of the same group in the same
		 * scene */
		if (!!obs_scene_get_group(scene, name)) {
			continue;
		}

		AFSourceUtil::SourcePaste(copyInfo, false);
		//OBSBasicSourceSelect::SourcePaste(copyInfo, false);
	}

	m_undo_s.PopDisabled();

	QString action_name = QTStr("Undo.PasteSourceRef");
	const char* scene_name = obs_source_get_name(scene_source);

	OBSData redo_data = BackupScene(scene_source);
	CreateSceneUndoRedoAction(action_name.arg(scene_name), undo_data, redo_data);
}

void AFMainFrame::qSlotActionPasteDupSource()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    OBSScene scene = sceneContext.GetCurrOBSScene();
    OBSSource scene_source = OBSSource(obs_scene_get_source(scene));

    OBSData undo_data = BackupScene(scene_source);
    m_undo_s.PushDisabled();


    for (size_t i = m_clipboard.size(); i > 0; i--) {
        SourceCopyInfo& copyInfo = m_clipboard[i - 1];
        AFSourceUtil::SourcePaste(copyInfo, true);
    }

    m_undo_s.PopDisabled();

    QString action_name = QTStr("Undo.PasteSource");
    const char* scene_name = obs_source_get_name(scene_source);

    OBSData redo_data = BackupScene(scene_source);
    CreateSceneUndoRedoAction(action_name.arg(scene_name), undo_data, redo_data);
}

void AFMainFrame::qSlotActionRenameSource()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    AFQSourceListView* sourceListView = sceneContext.GetSourceListViewPtr();
    if (!sourceListView)
        return;

    int idx = sourceListView->GetTopSelectedSourceItem();
    sourceListView->Edit(idx);
}

void AFMainFrame::qSlotActionRemoveSource()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    AFSourceUtil::RemoveSourceItems(sceneContext.GetCurrOBSScene());
}

void AFMainFrame::qSlotActionEditTransform()
{
    const auto item = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
    if (!item)
        return;

    CreateEditTransformPopup(item);
}

void AFMainFrame::qSlotActionCopyTransform()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    const auto item = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
    if (!item)
        return;

    obs_sceneitem_get_info(item, &sceneContext.m_copiedTransformInfo);
    obs_sceneitem_get_crop(item, &sceneContext.m_copiedCropInfo);

    ui->action_PasteTransform->setEnabled(true);
    m_hasCopiedTransform = true;
}

void undo_redo(const std::string& data)
{
    OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
    OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "scene_uuid"));
    AFMainFrame* main = App()->GetMainView();
    main->GetMainWindow()->SetCurrentScene(source.Get(), true);
    obs_scene_load_transform_states(data.c_str());
}
void AFMainFrame::qSlotActionPasteTransform()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    OBSScene curScene = sceneContext.GetCurrOBSScene();

    OBSDataAutoRelease wrapper = obs_scene_save_transform_states(curScene, false);
    auto func = [](obs_scene_t*, obs_sceneitem_t* item, void* data) {
        if (!obs_sceneitem_selected(item))
            return true;

        AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

        obs_sceneitem_defer_update_begin(item);
        obs_sceneitem_set_info(item, &sceneContext.m_copiedTransformInfo);
        obs_sceneitem_set_crop(item, &sceneContext.m_copiedCropInfo);
        obs_sceneitem_defer_update_end(item);

        return true;
    };

    obs_scene_enum_items(curScene, func, this);

    OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(curScene, false);
    std::string undo_data(obs_data_get_json(wrapper));
    std::string redo_data(obs_data_get_json(rwrapper));
    OBSSource curSource = sceneContext.GetCurrOBSSceneSource();
    m_undo_s.AddAction(QTStr("Undo.Transform.Paste").arg(obs_source_get_name(curSource)),
                       undo_redo, undo_redo, undo_data, redo_data);
}

static bool reset_tr(obs_scene_t* /* scene */, obs_sceneitem_t* item, void*)
{
    if (obs_sceneitem_is_group(item))
        obs_sceneitem_group_enum_items(item, reset_tr, nullptr);
    if (!obs_sceneitem_selected(item))
        return true;
    if (obs_sceneitem_locked(item))
        return true;

    obs_sceneitem_defer_update_begin(item);

    obs_transform_info info;
    vec2_set(&info.pos, 0.0f, 0.0f);
    vec2_set(&info.scale, 1.0f, 1.0f);
    info.rot = 0.0f;
    info.alignment = OBS_ALIGN_TOP | OBS_ALIGN_LEFT;
    info.bounds_type = OBS_BOUNDS_NONE;
    info.bounds_alignment = OBS_ALIGN_CENTER;
    vec2_set(&info.bounds, 0.0f, 0.0f);
    obs_sceneitem_set_info(item, &info);

    obs_sceneitem_crop crop = {};
    obs_sceneitem_set_crop(item, &crop);

    obs_sceneitem_defer_update_end(item);

    return true;
}


void AFMainFrame::qSlotActionResetTransform()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    OBSScene curScene = sceneContext.GetCurrOBSScene();
    OBSDataAutoRelease wrapper = obs_scene_save_transform_states(curScene, false);
    obs_scene_enum_items(sceneContext.GetCurrOBSScene(), reset_tr, nullptr);
    OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(curScene, false);

    std::string undo_data(obs_data_get_json(wrapper));
    std::string redo_data(obs_data_get_json(rwrapper));
    OBSSource curSource = sceneContext.GetCurrOBSSceneSource();
    m_undo_s.AddAction(QTStr("Undo.Transform.Reset").arg(obs_source_get_name(curSource)),
                       undo_redo, undo_redo, undo_data, redo_data);

    obs_scene_enum_items(sceneContext.GetCurrOBSScene(), reset_tr, nullptr);
}

void AFMainFrame::qSlotActionRotate90CW()
{
    AFSourceUtil::RotateSourceFromMenu(90.0f);
}

void AFMainFrame::qSlotActionRotate90CCW()
{
    AFSourceUtil::RotateSourceFromMenu(-90.0f);
}

void AFMainFrame::qSlotActionRotate180()
{
    AFSourceUtil::RotateSourceFromMenu(180.0f);
}

void AFMainFrame::qSlotFlipHorizontal()
{
    AFSourceUtil::FlipSourceFromMenu(-1.0f, 1.0f);
}

void AFMainFrame::qSlotFlipVertical()
{
    AFSourceUtil::FlipSourceFromMenu(1.0f, -1.0f);
}

void AFMainFrame::qSlotFitToScreen()
{
    AFSourceUtil::FitSourceToScreenFromMenu(OBS_BOUNDS_SCALE_INNER);
}

void AFMainFrame::qSlotStretchToScreen()
{
    AFSourceUtil::FitSourceToScreenFromMenu(OBS_BOUNDS_STRETCH);
}

void AFMainFrame::qSlotCenterToScreen()
{
    AFSourceUtil::SetCenterToScreenFromMenu(CenterType::Scene);
}

void AFMainFrame::qSlotVerticalCenter()
{
    AFSourceUtil::SetCenterToScreenFromMenu(CenterType::Vertical);
}

void AFMainFrame::qSlotHorizontalCenter()
{
    AFSourceUtil::SetCenterToScreenFromMenu(CenterType::Horizontal);
}

void AFMainFrame::qSlotActionShowInteractionPopup()
{
    const auto item = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
    if (!item)
        return;

    OBSSource source = obs_sceneitem_get_source(item);

    if (source)
        GetMainWindow()->ShowBrowserInteractionPopup(source);
}

void AFMainFrame::qSlotActionShowProperties()
{
    const auto item = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
    if (!item)
        return;

    obs_source_t* source = obs_sceneitem_get_source(item);
    if (obs_source_configurable(source)) {
        App()->GetMainView()->CreatePropertiesPopup(source);
    }
}

void AFMainFrame::qSlotOpenSourceFilters()
{
    OBSSceneItem item = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
    OBSSource source = obs_sceneitem_get_source(item);

    CreateFiltersWindow(source);
}

void AFMainFrame::qSlotCopySourceFilters()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    OBSSceneItem item = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
    if (!item)
        return;

    OBSSource source = obs_sceneitem_get_source(item);

    sceneContext.m_obsCopyFiltersSource = obs_source_get_weak_source(source);

    ui->action_PasteFilters->setEnabled(true);
}

void AFMainFrame::qSlotPasteSourceFilters()
{
    AFLocaleTextManager& locale  = AFLocaleTextManager::GetSingletonInstance();
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    OBSSourceAutoRelease source =
        obs_weak_source_get_source(sceneContext.m_obsCopyFiltersSource);

    OBSSceneItem sceneItem = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
    if (!sceneItem)
        return;

    OBSSource dstSource = obs_sceneitem_get_source(sceneItem);

    if (source == dstSource)
        return;

    OBSDataArrayAutoRelease undo_array = obs_source_backup_filters(dstSource);
    obs_source_copy_filters(dstSource, source);
    OBSDataArrayAutoRelease redo_array = obs_source_backup_filters(dstSource);

    const char* srcName = obs_source_get_name(source);
    const char* dstName = obs_source_get_name(dstSource);
    QString text = locale.Str("Undo.Filters.Paste.Multiple");
    text = text.arg(srcName, dstName);

    CreateFilterPasteUndoRedoAction(text, dstSource, undo_array, redo_array);
}

static void ConfirmColor(AFQSourceListView* sourceList, const QColor& color,
                         QModelIndexList selectedItems)
{
    for (int x = 0; x < selectedItems.count(); x++) {
        AFQSourceViewItem* sourceItem =
            sourceList->GetItemWidget(selectedItems[x].row());

        sourceItem->SetBackgroundColor(color);

        OBSSceneItem sceneItem = sourceList->Get(selectedItems[x].row());
        OBSDataAutoRelease privData =
                    obs_sceneitem_get_private_settings(sceneItem);
        obs_data_set_int(privData, "color-preset", 1);
        obs_data_set_string(privData, "color",
                            QT_TO_UTF8(color.name(QColor::HexArgb)));
    }
}

void AFMainFrame::qSlotSourceListItemColorChange()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    AFQSourceListView* sourceList = sceneContext.GetSourceListViewPtr();
    QModelIndexList selectedItems =
                    sourceList->selectionModel()->selectedIndexes();

    QAction* action = qobject_cast<QAction*>(sender());
    QPushButton* colorButton = qobject_cast<QPushButton*>(sender());
   
    if (selectedItems.count() == 0)
        return;

    if (colorButton) {
        QFrame* colorButtonFrame = qobject_cast<QFrame*>(colorButton->parentWidget());

        int preset = colorButton->property("bgColor").value<int>();
        for (int x = 0; x < selectedItems.count(); x++) {
            AFQSourceViewItem* sourceItem = sourceList->GetItemWidget(selectedItems[x].row());
            
            QColor color = _GetSourceListBackgroundColor(preset);
            sourceItem->SetBackgroundColor(color);

            OBSSceneItem sceneItem =
                sourceList->Get(selectedItems[x].row());
            OBSDataAutoRelease privData =
                obs_sceneitem_get_private_settings(sceneItem);
            obs_data_set_int(privData, "color-preset", preset + 1);
            obs_data_set_string(privData, "color", "");
        }

        for (int i = 1; i < 9; i++) {
            std::stringstream button;
            button << "framePreset" << i;
            QFrame* cButtonFrame =
                colorButton->parentWidget()->parentWidget()
                        ->findChild<QFrame*>(
                                button.str().c_str());
            cButtonFrame->setStyleSheet("QFrame { border: 1px solid black; border-radius:6px; }");
        }
        colorButtonFrame->setStyleSheet("QFrame { border: 1px solid #D9D9D9; border-radius:6px; }");
    }
    else if (action) {
        int preset = action->property("bgColor").value<int>();
		if (1 == preset) {
			OBSSceneItem sceneItem = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
			if (!sceneItem)
				return;

            AFQSourceViewItem* sourceItem =
                               sourceList->GetItemWidgetFromSceneItem(sceneItem);

            OBSDataAutoRelease curPrivData =
                               obs_sceneitem_get_private_settings(sceneItem);

            int oldPreset =
                obs_data_get_int(curPrivData, "color-preset");

            const QString oldSheet = sourceItem->styleSheet();
            const QColor  oldColor = sourceItem->GetBackgroundColor();

            auto liveChangeColor = [=](const QColor& color) {
                if (color.isValid()) {
                    sourceItem->SetBackgroundColor(color);
                }
            };

            auto changedColor = [=](const QColor& color) {
                if (color.isValid()) {
                    ConfirmColor(sourceList, color,
                                 selectedItems);
                }
            };

            auto rejected = [=]() {
                if (oldPreset == 1) {
                    sourceItem->SetBackgroundColor(oldColor);
                }
                else if (oldPreset == 0) {
                    sourceItem->SetBackgroundColor(QColor(24, 27, 32, 84));
                }
                else {
                    QColor color = _GetSourceListBackgroundColor(oldPreset - 1);
                    sourceItem->SetBackgroundColor(color);
                }
            };

            QColorDialog::ColorDialogOptions options =
                QColorDialog::ShowAlphaChannel;

#ifdef _WIN32
            AFQCustomColorDialog* colorDialog = new AFQCustomColorDialog(oldColor,
                                                        Str("CustomColorDialog.title"), this);
            colorDialog->setOptions(options);

            connect(colorDialog, &AFQCustomColorDialog::qSignalCurrentColorChanged,
                    liveChangeColor);

            connect(colorDialog, &AFQCustomColorDialog::qSignalColorSelected,
                    changedColor);

            connect(colorDialog, &AFQCustomColorDialog::rejected, rejected);

            colorDialog->open();
#else
            QColorDialog* colorDialog = new QColorDialog(this);
            colorDialog->setOptions(options);
            colorDialog->setCurrentColor(oldColor);
            connect(colorDialog, &QColorDialog::currentColorChanged,
                liveChangeColor);
            connect(colorDialog, &QColorDialog::colorSelected,
                changedColor);
            connect(colorDialog, &QColorDialog::rejected, rejected);
            colorDialog->open();
#endif
		}
        else {
            for (int x = 0; x < selectedItems.count(); x++) {
                AFQSourceViewItem* sourceItem 
                            = sourceList->GetItemWidget(selectedItems[x].row());

                QColor color(24, 27, 32, 84);
                sourceItem->SetBackgroundColor(color);

                OBSSceneItem sceneItem = sourceList->Get(selectedItems[x].row());
                OBSDataAutoRelease privData =
                                obs_sceneitem_get_private_settings(
                                sceneItem);
                obs_data_set_int(privData, "color-preset",
                                 preset);
                obs_data_set_string(privData, "color", "");
            }
        }
    }
}

void AFMainFrame::qSlotResizeOutputSizeOfSource()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();

    if (obs_video_active())
        return;

    int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, 
                                            this, "",
                                            QString(Str("ResizeOutputSizeOfSource.Text")) + "\n\n" +
                                            QString(Str("ResizeOutputSizeOfSource.Continue")));

    if (!result)
        return;

    OBSSceneItem sceneItem = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
    OBSSource source = obs_sceneitem_get_source(sceneItem);

    int width = obs_source_get_width(source);
    int height = obs_source_get_height(source);
  
    config_set_uint(confManager.GetBasic(), "Video", "BaseCX", width);
    config_set_uint(confManager.GetBasic(), "Video", "BaseCY", height);
    config_set_uint(confManager.GetBasic(), "Video", "OutputCX", width);
    config_set_uint(confManager.GetBasic(), "Video", "OutputCY", height);

    AFVideoUtil* videoUtil = GetMainWindow()->GetVideoUtil();
    videoUtil->ResetVideo();
    ResetOutputs();

    config_save_safe(confManager.GetBasic(), "tmp", nullptr);
    qSlotFitToScreen();
}

void AFMainFrame::qSlotSetScaleFilter()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    obs_scale_type mode = (obs_scale_type)action->property("mode").toInt();
    OBSSceneItem sceneItem = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();

    obs_sceneitem_set_scale_filter(sceneItem, mode);
}

void AFMainFrame::qSlotBlendingMethod()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    obs_blending_method method =
        (obs_blending_method)action->property("method").toInt();
    OBSSceneItem sceneItem = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();

    obs_sceneitem_set_blending_method(sceneItem, method);
}

void AFMainFrame::qSlotBlendingMode()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    obs_blending_type mode =
        (obs_blending_type)action->property("mode").toInt();
    OBSSceneItem sceneItem = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();

    obs_sceneitem_set_blending_mode(sceneItem, mode);
}

void AFMainFrame::qSlotSetDeinterlaceingMode()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    obs_deinterlace_mode mode =
        (obs_deinterlace_mode)action->property("mode").toInt();
    OBSSceneItem sceneItem = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
    obs_source_t* source = obs_sceneitem_get_source(sceneItem);

    obs_source_set_deinterlace_mode(source, mode);
}

void AFMainFrame::qSlotSetDeinterlacingOrder()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    obs_deinterlace_field_order order =
        (obs_deinterlace_field_order)action->property("order").toInt();
    OBSSceneItem sceneItem = GetMainWindow()->GetSceneSourceDock()->GetCurrentSceneItem();
    obs_source_t* source = obs_sceneitem_get_source(sceneItem);

    obs_source_set_deinterlace_field_order(source, order);
}

void AFMainFrame::qslotProcessHotkey(obs_hotkey_id id, bool pressed)
{
    obs_hotkey_trigger_routed_callback(id, pressed);
}
void AFMainFrame::qSlotUndo()
{
    m_undo_s.Undo();
}
void AFMainFrame::qSlotRedo()
{
    m_undo_s.Redo();
}

void AFMainFrame::closeEvent(QCloseEvent* event)
{
    /* Do not close window if inside of a temporary event loop because we
     * could be inside of an Auth::LoadUI call.  Keep trying once per
     * second until we've exit any known sub-loops. */
    /*if (os_atomic_load_long(&insideEventLoop) != 0) {
        QTimer::singleShot(1000, this, &AFMainFrame::close);
        event->ignore();
        return;
    }*/
    config_t* configGlobalFile = AFConfigManager::GetSingletonInstance().GetGlobal();

    bool confirmOnExit =
        config_get_bool(configGlobalFile, "General", "ConfirmOnExit");

    if (confirmOnExit && IsActive() &&
        !m_bClearingFailed)
    {
        //System Tray Disable - MainView can't be hidden
        //qslotSetShowing(true);

        auto& localeMan = AFLocaleTextManager::GetSingletonInstance();
        bool result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            this, "", QT_UTF8(localeMan.Str("ConfirmExit.Text")));
        if (result == QDialog::Rejected)
        {
            event->ignore();
            g_bRestart = false;
            return;
        }

    }

    //Save chat

    if (m_GlobalSoopChatWidget)
    {
        std::string check = QString("%1Popup").arg("Chat").toStdString();

        config_set_bool(configGlobalFile,
            "BasicWindow", check.c_str(), m_GlobalSoopChatWidget->isVisible());

        config_set_string(configGlobalFile, "BasicWindow", "Chat",
            m_GlobalSoopChatWidget->saveGeometry().toBase64().constData());
    }


    if (m_GlobalSoopNewsfeedWidget)
    {
        std::string check = QString("%1Popup").arg("Dashboard").toStdString();
        config_set_bool(configGlobalFile,
            "BasicWindow", check.c_str(), m_GlobalSoopNewsfeedWidget->isVisible());

        config_set_string(configGlobalFile, "BasicWindow", "Dashboard",
            m_GlobalSoopNewsfeedWidget->saveGeometry().toBase64().constData());
    }


    if(m_GlobalSoopChatWidget)
        m_GlobalSoopChatWidget->close();

    if (m_GlobalSoopNewsfeedWidget)
        m_GlobalSoopNewsfeedWidget->close();

    auto& authManager = AFAuthManager::GetSingletonInstance();
    
    int cntOfAccount = authManager.GetCntChannel();
    for (int idx = 0; idx < cntOfAccount; idx++)
    {
        AFChannelData* tmpChannel = nullptr;
        authManager.GetChannelData(idx, tmpChannel);
        
        if (tmpChannel != nullptr)
        {
            QPixmap* delObj = (QPixmap*)tmpChannel->pObjQtPixmap;
            if (delObj != nullptr) {
                delete delObj;
                tmpChannel->pObjQtPixmap = nullptr;
            }
        }
    }
    
    AFChannelData* tmpMainChannel = nullptr;
    authManager.GetMainChannelData(tmpMainChannel);
    
    if (tmpMainChannel != nullptr)
    {
        QPixmap* delObj = (QPixmap*)tmpMainChannel->pObjQtPixmap;
        if (delObj != nullptr) {
            delete delObj;
            tmpMainChannel->pObjQtPixmap = nullptr;
        }
    }
    
    std::vector<void*>* pVecDeferrdDel = authManager.GetContainerDeferrdDelObj();
    for(void* node : *pVecDeferrdDel)
    {
        QPixmap* delObj = (QPixmap*)node;
        delete delObj;
    }
    pVecDeferrdDel->clear();
        
        
    auto& statusLogManager = AFStatusLogManager::GetSingletonInstance();
    statusLogManager.SendLogProgramStart(false);

    if (m_VolumeSliderFrame)
    {
        m_VolumeSliderFrame->close();
        delete m_VolumeSliderFrame;
        m_VolumeSliderFrame = nullptr;
    }

    if (m_MicSliderFrame)
    {
        m_MicSliderFrame->close();
        delete m_MicSliderFrame;
        m_MicSliderFrame = nullptr;
    }


    if (isVisible())
        config_set_string(configGlobalFile, "BasicWindow",
            "geometry",
            saveGeometry().toBase64().constData());

    config_set_bool(GetGlobalConfig(),
        "BasicWindow", "BlockAreaOff", m_BlockPopup->isVisible() ? false : true);

    disconnect(App(), &QApplication::focusChanged, this, &AFMainFrame::qslotFocusChanged);


    //Remux
    /*if (remux && !remux->close()) {
        event->ignore();
        restart = false;
        return;
    }*/

    QWidget::closeEvent(event);
    if (!event->isAccepted())
        return;

    //blog(LOG_INFO, SHUTDOWN_SEPARATOR);
    m_bClosing = true;

    config_set_bool(configGlobalFile, "BasicWindow",
        "PreviewProgramMode", IsPreviewProgramMode());

    config_save_safe(GetGlobalConfig(), "tmp", nullptr);


    AFAuthManager::GetSingletonInstance().SaveAllAuthed();
    AFLoadSaveManager::GetSingletonInstance().SaveProjectNow();

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN);

    AFMainWindowAccesser* tmpViewModels = g_ViewModelsDynamic.UnSafeGetInstace();
    if (tmpViewModels)
        tmpViewModels->m_RenderModel.RemoveCallbackMainDisplay();

    /* Clear all scene data (dialogs, widgets, widget sub-items, scenes,
     * sources, etc) so that all references are released before shutdown */
    ClearSceneData();
    
    if (api)
        api->on_event(OBS_FRONTEND_EVENT_EXIT);

    // Destroys the frontend API so plugins can't continue calling it
    obs_frontend_set_callbacks_internal(nullptr);
    api = nullptr;

    m_DynamicCompositMainWindow->close();

    QMetaObject::invokeMethod(App(), "quit", Qt::QueuedConnection);
}

void AFMainFrame::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void AFMainFrame::showEvent(QShowEvent* event)
{
    if (m_BlockPopup == nullptr)
    {
        _SetupBlock();
        connect(m_DynamicCompositMainWindow->GetMainPreview(), &AFQTDisplay::qsignalDisplayResized,
            this, &AFMainFrame::qslotPreviewResizeTriggered);


        //First Execute Show Chat and NewsFeed
        config_t* globalconfig = AFConfigManager::GetSingletonInstance().GetGlobal();

        bool showChat = config_get_bool(
            globalconfig, "BasicWindow", "ChatPopup");

        bool showDashboard = config_get_bool(
            globalconfig, "BasicWindow", "DashboardPopup");

        if (showChat)
        {
            bool accountValid = true;
            if (!m_GlobalSoopChatWidget)
                accountValid = _SetupChatPage();

            if (accountValid)
            {
                const char* chatPos = config_get_string(
                    globalconfig, "BasicWindow", "Chat");
                QByteArray byteArray =
                    QByteArray::fromBase64(QByteArray(chatPos));
                m_GlobalSoopChatWidget->restoreGeometry(byteArray);
                m_GlobalSoopChatWidget->show();
                if (m_BlockPopup)
                    m_BlockPopup->BlockButtonToggled(showChat, ENUM_BLOCK_TYPE::Chat);
            }
        }

        if (showDashboard)
        {
            if (!m_GlobalSoopNewsfeedWidget)
                _SetupNewsfeedPage();

            const char* dashboardPos = config_get_string(
                globalconfig, "BasicWindow", "Dashboard");

            QByteArray byteArray =
                QByteArray::fromBase64(QByteArray(dashboardPos));
            m_GlobalSoopNewsfeedWidget->restoreGeometry(byteArray);
            m_GlobalSoopNewsfeedWidget->show();
            if (m_BlockPopup)
                m_BlockPopup->BlockButtonToggled(showDashboard, ENUM_BLOCK_TYPE::Dashboard);

        }
    }

    qslotToggleBlockAreaByToggleButton();
    
    if (config_get_bool(AFConfigManager::GetSingletonInstance().GetBasic(), "General", "OpenStatsOnStartup"))
        qslotStatsOpenTriggered();
    
    m_DynamicCompositMainWindow->SetDockedValues();
}

void AFMainFrame::moveEvent(QMoveEvent* event)
{
    //_MoveBlocks();
    //_MoveStatWidget();
    if (m_BlockPopup != nullptr)
    {
        if (m_BlockPopup->isVisible())
        {
            m_BlockPopup->hide();
        }
    }

    if (currentScreen != App()->screenAt(this->mapToGlobal(rect().center())))
    {
        currentScreen = App()->screenAt(this->mapToGlobal(rect().center()));
        repaint();
    }
}

void AFMainFrame::changeWidgetBorder(bool isMaximized)
{
    this->setProperty("frame_maximized", isMaximized);

    if (!isMaximized)
        ui->pushButton_MaximumWindow->setObjectName("pushButton_MaximumWindow");
    else
        ui->pushButton_MaximumWindow->setObjectName("pushButton_MaxedWindow");

    updateStyleSheet(ui->pushButton_MaximumWindow);
    updateStyleSheet(ui->frame_Top);
    updateStyleSheet(ui->frame_Bottom);
    updateStyleSheet(this);
}

//Block = Block Area + Lock Button
void AFMainFrame::_SetupBlock()
{
    m_BlockPopup = new AFQBlockPopup(this);
    m_BlockPopup->BlockWindowInit(false, m_DynamicCompositMainWindow);
    
    connect(ui->pushButton_BlockToggle, &QPushButton::clicked, this, &AFMainFrame::qslotToggleBlockArea);

    connect(m_BlockPopup, &AFQBlockPopup::qsignalBlockButtonTriggered, 
        this, &AFMainFrame::qslotToggleBlockFromBlockArea);
    connect(m_BlockPopup, &AFQBlockPopup::qsignalShowSetting, 
        this, &AFMainFrame::qslotShowStudioSettingPopup);

    bool blockAreaOff = config_get_bool(GetGlobalConfig(),
        "BasicWindow", "BlockAreaOff");

    if(!blockAreaOff)
        _ShowBlockArea(0);

    m_BlockPopup->raise();
}

void AFMainFrame::_MoveBlocks()
{
    if (m_BlockPopup != nullptr)
    {
        int32_t mainDisplayYPosSize = m_DynamicCompositMainWindow->GetMainPreview()->y() +
                                        m_DynamicCompositMainWindow->GetMainPreview()->height();
        
        auto& configManager = AFConfigManager::GetSingletonInstance();

        if (configManager.GetStates()->IsPreviewProgramMode())
        {
            bool studioPortraitLayout = config_get_bool(configManager.GetGlobal(), 
                "BasicWindow", "StudioPortraitLayout");
            if (studioPortraitLayout)
            {
                AFQVerticalProgramView* tmpStudioModeView = m_DynamicCompositMainWindow->GetVerticalStudioModeViewLayout();
                if (tmpStudioModeView)
                    mainDisplayYPosSize = tmpStudioModeView->y() + tmpStudioModeView->height();
            }
            else
            {
                AFQProgramView* tmpStudioModeView = m_DynamicCompositMainWindow->GetStudioModeViewLayout();
                if(tmpStudioModeView)
                    mainDisplayYPosSize = tmpStudioModeView->y() + tmpStudioModeView->height();
            }
        }

        //Pos -> Width: Total/2 - block/2 (make it center), Height: Preview Bottom - block/2
        m_BlockPopup->move(x() + width() / 2 - m_BlockPopup->width() / 2,
                           y() + mainDisplayYPosSize - m_BlockPopup->height() / 2 - 12);
    }
}

bool AFMainFrame::_SetupChatPage()
{
    m_GlobalSoopChatWidget = new AFQBorderPopupBaseWidget(nullptr);
    m_GlobalSoopChatWidget->setObjectName("widget_CustomBrowser");
    m_GlobalSoopChatWidget->setStyleSheet("border-radius: 0px");
    m_GlobalSoopChatWidget->setWindowFlag(Qt::FramelessWindowHint);
    m_GlobalSoopChatWidget->setProperty("uuid", "Chat");
    m_GlobalSoopChatWidget->SetIsCustom(true);
    m_GlobalSoopChatWidget->setWindowTitle(QTStr("Block.Tooltip.Chat"));
    m_GlobalSoopChatWidget->setMinimumSize(QSize(360, 650));
    m_GlobalSoopChatWidget->setAttribute(Qt::WA_DeleteOnClose);

    connect(m_GlobalSoopChatWidget, &AFQBorderPopupBaseWidget::qsignalCloseWidget,
        this, &AFMainFrame::qslotCloseGlobalSoopPage);

    AFDockTitle* dockTitle = new AFDockTitle(m_GlobalSoopChatWidget);
    dockTitle->Initialize(QTStr("Block.Tooltip.Chat"));
    dockTitle->setProperty("uuid", "Chat");
    dockTitle->setObjectName("CustomBrowserDockTitle");

    connect(dockTitle, &AFDockTitle::qsignalCustomBrowserClose,
        this, &AFMainFrame::qslotHideGlobalSoopPage);
    connect(dockTitle, &AFDockTitle::qsignalCustomBrowserMaximize,
        this, &AFMainFrame::qslotMaximizeGlobalSoopPage);
    connect(dockTitle, &AFDockTitle::qsignalCustomBrowserMinimize,
        this, &AFMainFrame::qslotMinimizeGlobalSoopPage);

    connect(m_GlobalSoopChatWidget, &AFCQMainBaseWidget::qsignalBaseWindowMaximized,
            dockTitle, &AFDockTitle::qslotChangeMaximizeIcon);

    auto& auth = AFAuthManager::GetSingletonInstance();
    AFChannelData* mainChannel = nullptr;
    auth.GetMainChannelData(mainChannel);
    if (!mainChannel)
        return false;

    std::string strID = mainChannel->pAuthData->strChannelID;
    std::string strChatUrl = SOOP_GLOBAL_CHAT_URL + strID;

    auto& cefManager = AFCefManager::GetSingletonInstance();
    QCefWidget* cefWidget = cefManager.GetCef()->
        create_widget(m_GlobalSoopChatWidget,
            strChatUrl);
    m_GlobalSoopChatWidget->AddWidget(dockTitle);
    m_GlobalSoopChatWidget->AddCustomWidget(cefWidget);
    return true;
}

bool AFMainFrame::_SetupNewsfeedPage()
{
    m_GlobalSoopNewsfeedWidget = new AFQBorderPopupBaseWidget(nullptr);
    m_GlobalSoopNewsfeedWidget->setObjectName("widget_CustomBrowser");
    m_GlobalSoopNewsfeedWidget->setStyleSheet("border-radius: 0px");
    m_GlobalSoopNewsfeedWidget->setWindowFlag(Qt::FramelessWindowHint);
    m_GlobalSoopNewsfeedWidget->setProperty("uuid", "Newsfeed");
    m_GlobalSoopNewsfeedWidget->SetIsCustom(true);
    m_GlobalSoopNewsfeedWidget->setWindowTitle(QTStr("Block.Tooltip.Newsfeed"));
    m_GlobalSoopNewsfeedWidget->setMinimumSize(QSize(360, 650));
    m_GlobalSoopNewsfeedWidget->setAttribute(Qt::WA_DeleteOnClose);

    connect(m_GlobalSoopNewsfeedWidget, &AFQBorderPopupBaseWidget::qsignalCloseWidget,
        this, &AFMainFrame::qslotCloseGlobalSoopPage);

    AFDockTitle* dockTitle = new AFDockTitle(m_GlobalSoopNewsfeedWidget);
    dockTitle->Initialize(QTStr("Block.Tooltip.Newsfeed"));
    dockTitle->setProperty("uuid", "Newsfeed");
    dockTitle->setObjectName("CustomBrowserDockTitle");

    connect(dockTitle, &AFDockTitle::qsignalCustomBrowserClose,
        this, &AFMainFrame::qslotHideGlobalSoopPage);
    connect(dockTitle, &AFDockTitle::qsignalCustomBrowserMaximize,
        this, &AFMainFrame::qslotMaximizeGlobalSoopPage);
    connect(dockTitle, &AFDockTitle::qsignalCustomBrowserMinimize,
        this, &AFMainFrame::qslotMinimizeGlobalSoopPage);

    connect(m_GlobalSoopNewsfeedWidget, &AFCQMainBaseWidget::qsignalBaseWindowMaximized,
            dockTitle, &AFDockTitle::qslotChangeMaximizeIcon);

    auto& cefManager = AFCefManager::GetSingletonInstance();
    QCefWidget* cefWidget = cefManager.GetCef()->
        create_widget(m_GlobalSoopNewsfeedWidget,
            SOOP_GLOBAL_NEWSFEED_URL);
    m_GlobalSoopNewsfeedWidget->AddWidget(dockTitle);
    m_GlobalSoopNewsfeedWidget->AddCustomWidget(cefWidget);
    return true;
}

void AFMainFrame::_MoveSystemAlert(AFQSystemAlert* systemAlert, bool isMainMinimized)
{
    if (!systemAlert)
        return;

    if (!systemAlert->isVisible())
        return;

    int globalPosX;
    int globalPosY;

    if (isMainMinimized) {
        QScreen* primaryScreen = QGuiApplication::primaryScreen();
        if (primaryScreen == nullptr)
            return;

        uint32_t cx = primaryScreen->size().width();
        uint32_t cy = primaryScreen->size().height();
        int space = 10;
        int taskbarHeight = 0;

        QRect screenGeometry = primaryScreen->geometry();

#ifdef _WIN32
        RECT workAreaRect;
        if (SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRect, 0)) {
            QRect workArea(workAreaRect.left, workAreaRect.top,
                workAreaRect.right - workAreaRect.left,
                workAreaRect.bottom - workAreaRect.top);

            taskbarHeight = screenGeometry.height() - workArea.height();
        }
#elif __APPLE__
        taskbarHeight =  GetHeightDock(this->window());
#endif
        
        globalPosX = cx - systemAlert->width() - space;
        globalPosY = cy - systemAlert->height() - taskbarHeight - space;
    }
    else {
        globalPosX = this->mapToGlobal(QPoint(this->width() / 2, 0)).x();
        globalPosY = ui->widget_ResourceNetwork->mapToGlobal(QPoint(0, 0)).y();
        globalPosX -= (systemAlert->width() / 2);
        globalPosY -= (systemAlert->height() / 2);
    }
    
    systemAlert->move(QPoint(globalPosX, globalPosY));
}

void AFMainFrame::_MoveProgramGuide()
{
    if (!m_ProgramGuideWidget)
        return;

    if (!m_ProgramGuideWidget->isVisible())
        return;

    QPoint globalPos = this->mapToGlobal(QPoint(0, 0));
    globalPos.setX(globalPos.x() - m_ProgramGuideWidget->width());
    globalPos.setY(globalPos.y() + height() - m_ProgramGuideWidget->height());

    m_ProgramGuideWidget->move(globalPos);
}

void AFMainFrame::_PopupRequest(bool show, int type)
{
    switch (type)
    {
    case ENUM_BLOCK_TYPE::SceneSource:
        if (m_ProgramGuideWidget && m_ProgramGuideWidget->isWidgetType())
            m_ProgramGuideWidget->NextMission(0);
        
        m_DynamicCompositMainWindow->ToggleDockVisible(ENUM_BLOCK_TYPE(type), show);
        break;
    case ENUM_BLOCK_TYPE::Chat:
        if (qslotShowGlobalPage("Chat"))
            if (m_BlockPopup)
                m_BlockPopup->BlockButtonToggled(show, type);
        break;
    case ENUM_BLOCK_TYPE::Dashboard:
        if(qslotShowGlobalPage("Newsfeed"))
            if (m_BlockPopup)
                m_BlockPopup->BlockButtonToggled(show, type);
        break;
    case ENUM_BLOCK_TYPE::Settings:
        qslotShowStudioSettingPopup(show);
        break;
    default: 
        m_DynamicCompositMainWindow->ToggleDockVisible(ENUM_BLOCK_TYPE(type), show);
        break;
    }
}

void AFMainFrame::_CreateTopMenu()
{ 
    auto funcSetWhatsThis = [](QList<QAction*> list) {
        foreach(QAction* menuAction, list)
        {
            QStringList parts = menuAction->objectName().split("_");
            if (parts.size() > 1)
            {
                QString value = parts[1];
                menuAction->setWhatsThis(value);
            }
        }
    };
    
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
    
    
    //TOOL
    AFQCustomMenu* toolMenu = new AFQCustomMenu(this, true);
    toolMenu->setFixedWidth(200);

    toolMenu->addAction(ui->action_Settings);
    toolMenu->addAction(ui->action_AdvanceControls);
    toolMenu->addAction(ui->action_Stats);

    connect(ui->action_Settings, &QAction::triggered, 
            this, &AFMainFrame::qslotShowStudioSettingPopup);
    //
    connect(ui->action_AdvanceControls, &QAction::triggered, 
            this, &AFMainFrame::qslotToggleBlockFromMenu);
    connect(ui->action_Stats, &QAction::triggered,
            this, &AFMainFrame::qslotStatsOpenTriggered);

    funcSetWhatsThis(toolMenu->actions());


    //ADD-ON
    AFQCustomMenu* addonMenu = new AFQCustomMenu(this, true);
    addonMenu->setFixedWidth(200);
    
    //addonMenu->addAction(ui->action_Plugin);

    funcSetWhatsThis(addonMenu->actions());
        
    //PROFILE
    AFQCustomMenu* profileMenu = new AFQCustomMenu(this, true);
    profileMenu->setObjectName("menu_ProfileTopMenu");
    profileMenu->setFixedWidth(200);

    profileMenu->addAction(ui->action_NewProfile);
    profileMenu->addAction(ui->action_DupProfile);
    profileMenu->addAction(ui->action_RenameProfile);
    profileMenu->addAction(ui->action_RemoveProfile);
    profileMenu->addAction(ui->action_ExportProfile);
    profileMenu->addAction(ui->action_ImportProfile);
    profileMenu->addSeparator();

    connect(ui->action_NewProfile, &QAction::triggered,
            this, &AFMainFrame::qSlotNewProfile);
    connect(ui->action_DupProfile, &QAction::triggered,
        this, &AFMainFrame::qSlotDupProfile);
    connect(ui->action_RenameProfile, &QAction::triggered,
        this, &AFMainFrame::qSlotRenameProfile);
    connect(ui->action_RemoveProfile, &QAction::triggered,
        this, &AFMainFrame::qSlotDeleteProfile);

    connect(ui->action_ExportProfile, &QAction::triggered,
            this, &AFMainFrame::qSlotExportProfile);
    connect(ui->action_ImportProfile, &QAction::triggered,
            this, &AFMainFrame::qSlotImportProfile);
    
    funcSetWhatsThis(profileMenu->actions());
    
    ui->action_Profile->setMenu(profileMenu);
    
    
    //SCENE_COLLECTION
    AFQCustomMenu* sceneCollectionMenu = new AFQCustomMenu(this, true);
    sceneCollectionMenu->setObjectName("menu_SceneCollectionTopMenu");
    sceneCollectionMenu->setFixedWidth(200);

    sceneCollectionMenu->addAction(ui->action_NewSceneCollection);
    sceneCollectionMenu->addAction(ui->action_DupSceneCollection);
    sceneCollectionMenu->addAction(ui->action_RenameSceneCollection);
    sceneCollectionMenu->addAction(ui->action_RemoveSceneCollection);
    sceneCollectionMenu->addAction(ui->action_ExportSceneCollection);
    sceneCollectionMenu->addAction(ui->action_ImportSceneCollection);
    sceneCollectionMenu->addSeparator();
    sceneCollectionMenu->addAction(ui->action_ShowMissingFiles);
    sceneCollectionMenu->addSeparator();

    connect(ui->action_NewSceneCollection, &QAction::triggered,
            this, &AFMainFrame::qSlotNewSceneCollection);
    connect(ui->action_DupSceneCollection, &QAction::triggered,
        this, &AFMainFrame::qSlotDupSceneCollection);
    connect(ui->action_RenameSceneCollection, &QAction::triggered,
        this, &AFMainFrame::qSlotRenameSceneCollection);
    connect(ui->action_RemoveSceneCollection, &QAction::triggered,
        this, &AFMainFrame::qSlotDeleteSceneCollection);

    connect(ui->action_ExportSceneCollection, &QAction::triggered,
            this, &AFMainFrame::qSlotExportSceneCollection);
    connect(ui->action_ImportSceneCollection, &QAction::triggered,
            this, &AFMainFrame::qSlotImportSceneCollection);
    connect(ui->action_ShowMissingFiles, &QAction::triggered,
            this, &AFMainFrame::qSlotShowMissingFiles);
    
    funcSetWhatsThis(sceneCollectionMenu->actions());
    
    ui->action_SceneCollection->setMenu(sceneCollectionMenu);
    
        
    //INFO
    AFQCustomMenu* infoMenu = new AFQCustomMenu(this, true);
    infoMenu->setFixedWidth(200);

    infoMenu->addAction(ui->action_Guide);
    infoMenu->addAction(ui->action_ProgramInfo);
    //infoMenu->addAction(ui->action_Homepage);
    infoMenu->addAction(ui->action_UpdateLog);
    //infoMenu->addAction(ui->action_Update);

    connect(ui->action_Guide, &QAction::triggered,
        this, &AFMainFrame::qslotProgramGuideOpenTriggered);

    connect(ui->action_ProgramInfo, &QAction::triggered,
            this, &AFMainFrame::qslotProgamInfoOpenTriggered);

    funcSetWhatsThis(infoMenu->actions());

    //TOP MENU
    m_qTopMenu = new AFQCustomMenu(this);
    connect(m_qTopMenu, &AFQCustomMenu::aboutToHide, this, &AFMainFrame::qslotTopMenuDestoryed);

    m_qTopMenu->setFixedWidth(200);
    m_qTopMenu->addMenu(toolMenu)->setText(QT_UTF8(locale.Str("Basic.MainMenu.ToolsFix")));
    m_qTopMenu->addMenu(addonMenu)->setText(QT_UTF8(locale.Str("Basic.MainMenu.Addon")));
    m_qTopMenu->addMenu(profileMenu)->setText(QT_UTF8(locale.Str("Basic.MainMenu.ProfileFix")));
    m_qTopMenu->addMenu(sceneCollectionMenu)->setText(QT_UTF8(locale.Str("Basic.MainMenu.SceneCollectionFix")));
    m_qTopMenu->addMenu(infoMenu)->setText(QT_UTF8(locale.Str("Basic.MainMenu.HelpFix")));
}

void AFMainFrame::_ToggleTopMenu()
{
    QPushButton* topMenuButton = reinterpret_cast<QPushButton*>(sender());

    if (m_bTopMenuTriggered) {
        m_qTopMenu->hide();
    }
    else {
        QPoint position = this->pos();
        m_qTopMenu->show(QPoint(position.x(), 
            position.y() + ui->frame_Top->height()));
    }
    m_bTopMenuTriggered = !m_bTopMenuTriggered;
}

void AFMainFrame::_AccountButtonStreamingToggle(bool stream)
{
    QList<AFMainAccountButton*> buttons = ui->widget_Platform->findChildren<AFMainAccountButton*>();
    foreach(AFMainAccountButton * button, buttons)
    {
        if (button->IsMainAccount())
        {
            if (button->GetCurrentState() == AFMainAccountButton::ChannelState::Disable)
            {
                continue;
            }
            else
            {
                if (!button->GetChannelData()->bIsStreaming)
                    continue;
            }
        }
            
        button->SetStreaming(stream);
    }
}

AFQCustomMenu* AFMainFrame::_FindSubMenuByTitle(AFQCustomMenu* menu, const QString& name)
{
    QList<QAction*> actions = menu->actions();
    for (QAction* action : actions) {
        AFQCustomMenu* submenu = reinterpret_cast<AFQCustomMenu*>(action->menu());
        if (submenu) {
            if (submenu->title() == name) {
                return submenu;
            }
            else {
                AFQCustomMenu* foundMenu = _FindSubMenuByTitle(submenu, name);
                if (foundMenu) {
                    return foundMenu;
                }
            }
        }
    }
    return nullptr;
}

void AFMainFrame::_ShowBlockArea(int animateSpeed)
{
    if (m_bIsBlockPopup && !m_bBlockAnimating && m_BlockPopup->isHidden())
    {
        QPropertyAnimation* blockanim = new QPropertyAnimation(m_BlockPopup, "pos", this);
        blockanim->setDuration(animateSpeed);

        int32_t mainDisplayYPosSize = m_DynamicCompositMainWindow->GetMainPreview()->y() +
            m_DynamicCompositMainWindow->GetMainPreview()->height();

        auto& configManager = AFConfigManager::GetSingletonInstance();
        if (configManager.GetStates()->IsPreviewProgramMode())
        {
            bool studioPortraitLayout = config_get_bool(configManager.GetGlobal(),
                "BasicWindow", "StudioPortraitLayout");
            if (studioPortraitLayout)
            {
                AFQVerticalProgramView* tmpStudioModeView = m_DynamicCompositMainWindow->GetVerticalStudioModeViewLayout();
                if (tmpStudioModeView)
                    mainDisplayYPosSize = tmpStudioModeView->y() + tmpStudioModeView->height();
            }
            else
            {
                AFQProgramView* tmpStudioModeView = m_DynamicCompositMainWindow->GetStudioModeViewLayout();
                if (tmpStudioModeView)
                    mainDisplayYPosSize = tmpStudioModeView->y() + tmpStudioModeView->height();
            }
        }

        //(18) Preview Margin
        blockanim->setStartValue(QPoint(x() + width() / 2 - m_BlockPopup->width() / 2,
            y() + mainDisplayYPosSize - m_BlockPopup->height() / 2 + 14));
        blockanim->setEndValue(QPoint(x() + width() / 2 - m_BlockPopup->width() / 2,
            y() + mainDisplayYPosSize - m_BlockPopup->height() / 2 - 12));

        m_bBlockAnimating = true;

        auto PopupShowFinished = [this]() {
            m_bBlockAnimating = false;
        };
        connect(blockanim, &QPropertyAnimation::finished, PopupShowFinished);

        blockanim->start(QAbstractAnimation::DeleteWhenStopped);
       
        m_BlockPopup->show();
        ui->pushButton_BlockToggle->setChecked(true);
    }
}

void AFMainFrame::_HideBlockArea(int animateSpeed)
{
    if (m_bIsBlockPopup && !m_bBlockAnimating && !m_BlockPopup->isHidden())
    {
        QPropertyAnimation* blockanim = new QPropertyAnimation(m_BlockPopup, "pos", this);
        blockanim->setDuration(animateSpeed);

        blockanim->setStartValue(QPoint(m_BlockPopup->x(), m_BlockPopup->y()));
        blockanim->setEndValue(QPoint(m_BlockPopup->x(), m_BlockPopup->y() + 14));
        m_bBlockAnimating = true;

        auto PopupHideFinished = [this]() {
            m_bBlockAnimating = false;
            m_BlockPopup->hide();
        };
        connect(blockanim, &QPropertyAnimation::finished, PopupHideFinished);

        blockanim->start(QAbstractAnimation::DeleteWhenStopped);
        
        ui->pushButton_BlockToggle->setChecked(false);
    }
}

