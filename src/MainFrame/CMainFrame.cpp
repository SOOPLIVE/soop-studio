#include "CMainFrame.h"
#include "ui_aneta-main-frame.h"

#include "Application/CApplication.h"
#include "Utils/CJsonController.h"
#include "Utils/soop-crypt.hpp"
#include "Utils/OverlayManager.h"
#include "Utils/BreaktimeManager.h"

#include "Common/StudioDefine.h"

#include <util/profiler.hpp>
#include <QBoxLayout>
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
#include <QDialog>
#include <QFontMetrics>
#include <QToolButton>
#include <QMimeData>
#include <QUrlQuery>
#include <QSettings>
#include <qdir.h>

#include <fstream>
#include <sstream>
#include <iterator>


#ifdef __APPLE__
#include <QFileOpenEvent>
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "qt-wrappers.hpp"
#include "util/dstr.hpp"
#include "util/platform.h"
#include "platform/platform.hpp"
#include "Utils/soop-imageprinter.hpp"

#include "Common/SettingsMiscDef.h"
#include "Common/StringMiscUtils.h"

#include "ui-validation.hpp"
#include "CLeftNavigationBar.h"
#include "MainPreview/CProgramViewHorizontal.h"
#include "MainPreview/CProgramViewVertical.h"

#include "ViewModel/MainWindow/CMainWindowAccesser.h"
#include "ViewModel/MainWindow/CMainWindowRenderModel.h"

#include "ViewModel/Auth/Soop/auth-soop.hpp"
#include "ViewModel/Auth/Twitch/auth-twitch.h"

#include "CoreModel/Profile/CProfile.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/OBSData/CInhibitSleepContext.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Statistics/CStatistics.h"
#include "CoreModel/Service/CService.h"
#include "CoreModel/Action/CHotkeyContext.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Icon/CIconContext.h"
#include "CoreModel/OBSOutput/COBSOutputContext.h"
#include "CoreModel/Video/CVideo.h"
#include "CoreModel/Audio/CAudio.h"
#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"
#include "CoreModel/Encoder/CEncoder.h"

#include "Blocks/SceneSourceDock/CSourceListView.h"
#include "Blocks/AdvanceControlsDock/CAdvanceControlsDockWidget.h"
#include "Blocks/BroadInfoDock/CBroadInfoDockWidget.h"
#include "Blocks/AudioMixerDock/CAudioAdvSettingWidget.h"

#include "UIComponent/CColorSelect.h"
#include "UIComponent/CBasicPreview.h"
#include "UIComponent/CBasicToggleButton.h"
#include "UIComponent/CMessageAlert.h"

#include "PopupWindows/CBalloonWidget.h"
#include "PopupWindows/CSceneTransitionsDialog.h"
#include "PopupWindows/SourceDialog/CSourceProperties.h"
#include "PopupWindows/SettingPopup/CStudioSettingDialog.h"
#include "PopupWindows/SourceDialog/CSelectSourceButton.h"
#include "PopupWindows/SourceDialog/CSceneSelectDialog.h"
#include "PopupWindows/SettingPopup/CSettingUtils.h"
#include "PopupWindows/CRemuxFrame.h"
#include "PopupWindows/SettingPopup/CAddStreamWidget.h"
#include "PopupWindows/CMissingFilesDialog.h"
#include "PopupWindows/CBrowserInteractionDialog.h"
#include "PopupWindows/CEndBroadDialog.h"
#include "PopupWindows/CCateChangeDialog.h"
#include "PopupWindows/CEmptyDialog.h"
#include "PopupWindows/SourceDialog/SOOPBrowserSource/CVideoBalloonProperty.h"
#include "PopupWindows/CSignaturePopup.h"
#include "PopupWindows/CFreecshotUnInstallAlert.h"
#include "PopupWindows/CStudioUpdateLogDialog.h"
#include "PopupWindows/SourceDialog/SOOPDowoomiSource/CAquaSubtitleDialog.h"

#include "ViewModel/Auth/COAuthLogin.hpp"
#include "ViewModel/Auth/CAuth.h"
#include "ViewModel/Auth/COAuthLogin.hpp"
#include "ViewModel/Auth/CAuthListener.hpp"
#include "ViewModel/Auth/YouTube/auth-youtube.hpp"
#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"

// MainFrame Separation Class
#include "DragDrop/CDragDrop.h"
#include "Profile/CMainProfile.h"
#include "SceneCollection/CMainSceneCollection.h"
#include "Output/COutput.h"
#include "AudioSource/CAudioSource.h"
#include "Update/CMainUpdate.h"
#include "SceneSource/CMainSceneSource.h"

bool StudioConfig::isStaging = false;
const QString SchedulDateAndTimeFormat = "yyyy-MM-dd'T'hh:mm:ss'Z'";

Q_DECLARE_METATYPE(OBSScene);
Q_DECLARE_METATYPE(OBSSceneItem);
Q_DECLARE_METATYPE(OBSSource);

extern soop_frontend_callbacks* InitializeAPIInterface(AFMainFrame* main);
static std::chrono::milliseconds g_loginDuration(0);

#define STARTUP_SEPARATOR "==== Startup complete ==============================================="
#define SHUTDOWN_SEPARATOR "==== Shutting down =================================================="

#define UNSUPPORTED_ERROR                                                     \
	"Failed to initialize video:\n\nRequired graphics API functionality " \
	"not found.  Your GPU may not be supported."

#define UNKNOWN_ERROR                                                  \
	"Failed to initialize video.  Your GPU may not be supported, " \
	"or your graphics drivers may need to be updated."

static inline void LogEncoders()
{
    constexpr uint32_t hide_flags = OBS_ENCODER_CAP_DEPRECATED | OBS_ENCODER_CAP_INTERNAL;

    auto list_encoders = [](obs_encoder_type type) {
        size_t idx = 0;
        const char* encoder_type;

        while(obs_enum_encoder_types(idx++, &encoder_type)) {
            if(obs_get_encoder_caps(encoder_type) & hide_flags ||
                obs_get_encoder_type(encoder_type) != type) {
                continue;
            }

            blog(LOG_INFO, "\t- %s (%s)", encoder_type, obs_encoder_get_display_name(encoder_type));
        }
    };

    blog(LOG_INFO, "---------------------------------");
    blog(LOG_INFO, "Available Encoders:");
    blog(LOG_INFO, "  Video Encoders:");
    list_encoders(OBS_ENCODER_VIDEO);
    blog(LOG_INFO, "  Audio Encoders:");
    list_encoders(OBS_ENCODER_AUDIO);
}


void AFMainFrame::OBSErrorMessageBox(const char* errorMsg, const char* defaultMsg, const char* errorLabel)
{
    QString error_reason;
    if (errorMsg)
        error_reason = QT_UTF8(errorMsg);
    else
        error_reason = QTStr(defaultMsg);
    //
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, DYNAMIC_COMPOSIT, "", errorMsg);
}

//MissingFileDialog shows on back of Mainframe on Startup if it opens in BlockManager
//Need Change Move to BlockManager - Need to solve open in front of Mainframe when first execute
void AFMainFrame::ShowMissingFilesDialog(obs_missing_files_t* files)
{
    if(obs_missing_files_count(files) > 0)
    {
        /* When loading the missing files dialog on launch, the
        * window hasn't fully initialized by this point on macOS,
        * so put this at the end of the current task queue. Fixes
        * a bug where the window is behind OBS on startup. */
        QTimer::singleShot(0, [this, files] {
            m_missDialog = new AFQMissingFilesDialog(files, this);
            m_missDialog->setAttribute(Qt::WA_DeleteOnClose, true);
            m_blockManager->ApplyMoveInAllArea(m_missDialog);
            m_missDialog->show();
            m_missDialog->raise();
        });
    } else {
        obs_missing_files_destroy(files);

        /* Only raise dialog if triggered manually */
        if(LOADSAVE_CONTEXT.CheckDisableSaving() == false)
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                       QT_UTF8(Str("MissingFiles.NoMissing.Title")),
                                       QT_UTF8(Str("MissingFiles.NoMissing.Text")));
    }
}

void AFMainFrame::ApplyMoveArea()
{
    ui->widget_BottomControl->setProperty("MoveInAllArea", true);
    ui->widget_Title->setProperty("MoveInAllArea", true);
    ui->widget_TopControl->setProperty("MoveInAllArea", true);
    ui->widget_TopMenu->setProperty("MoveInAllArea", true);
    ui->label_SoopStudio->setProperty("MoveInAllArea", true);

    QObjectList broadrecordchildren = ui->widget_BroadAndRecord->children();
    foreach(QObject * object, broadrecordchildren)
    {
        if (QPushButton* button = dynamic_cast<QPushButton*>(object))
            continue;
        object->setProperty("MoveInAllArea", true);
    }
}

bool AFMainFrame::CheckSplitVodByUI()
{
    return ui->label_BroadTime->IsSplitVodAvailable();
}


void AFMainFrame::ShowWindowCaptureArea(obs_source_t* source)
{
    WindowCaptureAreaWidget::launch(source, this);
}

#ifdef __APPLE__
    bool handleOpenURL(const QUrl & url) {
        if (url.scheme() == "soopstudio") {
            QString path = url.path();
            return true;
        }
        return false;
    }

    class URLHandler : public QObject {
        bool eventFilter(QObject* obj, QEvent* event) override {
            if (event->type() == QEvent::FileOpen) {
                QUrl url = static_cast<QFileOpenEvent*>(event)->url();
                return handleOpenURL(url);
            }
            return QObject::eventFilter(obj, event);
        }
    };
#endif // __APPLE__

AFMainFrame::AFMainFrame(QWidget *parent, Qt::WindowFlags flag, QString updatePath) :

    AFTTopBaseWidget(parent, flag),
    ui(new Ui::AFMainFrame),
    m_undo_s(this),
    m_serviceManager(std::make_unique<AFServiceManager>()),
    m_scene(std::make_unique<AFSceneContext>()),
    m_outputContext(std::make_unique<AFOBSOutputContext>()),
    m_graphicContext(std::make_unique<AFGraphicsContext>()),
    m_breakTimeManager(std::make_unique<BreaktimeManager>()),
    m_overlayManager(std::make_unique<OverlayManager>()),

    m_soopSrcManager(std::make_unique<SOOPMediaSourceManager>()),
    // Dynamic Composit
    m_dynamicCompositMainWindow(new AFMainDynamicComposit),
    m_leftNavigationBar(new AFQLeftNavigationBar(this)),
    m_refreshVideoBallonTimer(new QTimer(this)),
    // Manager
    m_blockManager(new AFQBlockManager(this)),
    m_pNetworkManager(new QNetworkAccessManager(this)),
    // MainFrame Func Separation class
    m_pMainDragDrop(new CMainDragDrop(this)),
    m_mainProfile(new AFMainProfile(this)),
    m_mainSceneCollection(new AFMainSceneCollection(this)),
    m_pMainOutput(new CMainOutput(this)),
    m_pMainAudioSource(new CMainAudioSource(this)),
    m_pMainSceneSource(new CMainSceneSource(this)),
    m_pMainUpdate(new CMainUpdate(this, m_pNetworkManager))
{
    ui->setupUi(this);
#ifdef __APPLE__
    ui->widget_Top->hide();
#endif

    ui->label_BroadTime->SetPrefix("LIVE ");
    ui->label_RecordTime->SetPrefix("REC ");

    m_soopApiHandler = new SOOPApiHandler();

    if (m_pMainUpdate)
        m_pMainUpdate->SetUpdatePath(updatePath);

#ifdef _WIN32
    AddRegStartProcessWindows();
#endif
#ifdef __APPLE__
    URLHandler* urlHandler = new URLHandler();
    QCoreApplication::instance()->installEventFilter(urlHandler);   
#endif
    
    setAcceptDrops(true);

    qRegisterMetaType<OBSScene>("OBSScene");
    qRegisterMetaType<OBSSceneItem>("OBSSceneItem");
    qRegisterMetaType<OBSSource>("OBSSource");
       
#ifdef __APPLE__
    ui->action_RemoveSource->setShortcut({ Qt::Key_Backspace });
#endif

    //No Preview Margin Design Changed
    ui->widget_PreviewBottomColor->hide();
    //No Preview Margin Design Changed
      
    m_CheckBroadStartAPITimer = new QTimer(this);
    m_CheckBroadStartAPITimer->setInterval(5000);
    m_CheckBroadStartAPITimer->setSingleShot(true);
    connect(m_CheckBroadStartAPITimer, &QTimer::timeout, [this]() {
        m_checkBroadStartAPI = false; 
        });
}

AFMainFrame::~AFMainFrame()
{
    m_breakTimeManager->Finalize();

#ifdef _WIN32
    if (AUTH_CONTEXT.IsSoopRegistered() == true)
        m_overlayManager->Finalize();
#endif // _WIN32_
    m_pMainOutput->ClearAllStreamSignals();
    m_pMainSceneSource->ClearSceneBottomButtons();

    m_dynamicCompositMainWindow = nullptr;

    m_outputContext.reset();

    m_pMainAudioSource->DestoryMainFrameAudioUI();

    m_sourceProperties = nullptr;
    m_transformPopup = nullptr;
    m_sourceFilters = nullptr;
    m_programInfoDialog = nullptr;
    m_sceneTransitionPopup = nullptr;
    m_advAudioSettingPopup = nullptr;
    m_signatureAIPopup = nullptr;

    for (auto& [id, prop] : m_sourcePropsPtr) {
        if (prop)
            prop = nullptr;
    }

    delete m_shortcutFilter;
    
    /* When shutting down, sometimes source references can get in to the
     * event queue, and if we don't forcibly process those events they
     * won't get processed until after obs_shutdown has been called.  I
     * really wish there were a more elegant way to deal with this via C++,
     * but Qt doesn't use C++ in a normal way, so you can't really rely on
     * normal C++ behavior for your data to be freed in the order that you
     * expect or want it to. */
    QApplication::sendPostedEvents(nullptr);

    config_set_int(APPCONFIG, "General", "LastVersion", LIBOBS_API_VER);
    config_save_safe(APPCONFIG, "tmp", nullptr);

    delete ui;
}

bool AFMainFrame::AFMainFrameInit(bool bShow, std::string userID, std::string soopCookie, std::string Install_Type, std::string Freecshot_Type)  // type  국내:1 ,  글로벌:0 또는 빈값
{
    setWindowTitle(APPNAME);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);

    _RegisterUndoRedoShortCut();

    m_pCurrentScreen = App()->screenAt(this->mapToGlobal(rect().center()));

    m_freecshotType = Freecshot_Type;
    m_installType = Install_Type;
    _CreateTopMenu();
    _SetMainFrameUI();

    // 
    m_soopSrcManager->InitContext();

    api = InitializeAPIInterface(this);

    /* Set up streaming connections */
    connect(this, &AFMainFrame::StreamingStarting, this, [this] {
        m_signalFlags.streamingStarting = true;
    }, Qt::DirectConnection);
    connect(this, &AFMainFrame::StreamingStarted, this, [this] {
        m_signalFlags.streamingStarting = false;
    }, Qt::DirectConnection);
    connect(this, &AFMainFrame::StreamingStopped, this, [this] {
        m_signalFlags.streamingStarting = false;
    }, Qt::DirectConnection);

    /* Set up recording connections */
    connect(this, &AFMainFrame::RecordingStarted, this, [this]() {
        m_signalFlags.recordingStarted = true; m_signalFlags.recordingPaused = false;
    }, Qt::DirectConnection);
    connect(this, &AFMainFrame::RecordingPaused, this, [this]() {
        m_signalFlags.recordingPaused = true;
    }, Qt::DirectConnection);
    connect(this, &AFMainFrame::RecordingUnpaused, this, [this]() {
        m_signalFlags.recordingPaused = false;
    }, Qt::DirectConnection);
    connect(this, &AFMainFrame::RecordingStopped, this, [this]() {
        m_signalFlags.recordingStarted = false; m_signalFlags.recordingPaused = false;
    }, Qt::DirectConnection);

    // CoreModel Context Init
    CONFIG_CONTEXT.InitBasic();

    // Video & Audio Utils
    int ret = AFVideoUtil::ResetVideo();

    switch(ret) {
        case OBS_VIDEO_MODULE_NOT_FOUND:
            throw "Failed to initialize video:  Graphics module not found";
        case OBS_VIDEO_NOT_SUPPORTED:
            throw UNSUPPORTED_ERROR;
        case OBS_VIDEO_INVALID_PARAM:
            throw "Failed to initialize video:  Invalid parameters";
        default:
            if(ret != OBS_VIDEO_SUCCESS)
                throw UNKNOWN_ERROR;
    }

    AFAudioUtil::ResetAudio();
    AFAudioUtil::LoadAudioMonitoring();
    
    auto mute = config_get_bool(APPCONFIG, "Audio", "MainAudioMute");
    soop_set_master_output_muted(mute);
    mute = config_get_bool(APPCONFIG, "Audio", "MainMicMute");
    soop_set_master_input_muted(mute); 

    HOTKEY_CONTEXT.InitHotkeys();

    // Context Init
    ICON_CONTEXT.InitContext();
    m_graphicContext->InitContext();
    m_scene->InitContext();
    m_scene->InitSourceSignalCallback();

    /* hack to prevent elgato from loading its own QtNetwork that it tries
     * to ship with */
#if defined(_WIN32) && !defined(_DEBUG)
    LoadLibraryW(L"Qt6Network");
#endif
    struct obs_module_failure_info mfi;

    /* Modules can access frontend information (i.e. profile and scene collection data) during their initialization, and some modules (e.g. obs-websockets) are known to use the filesystem location of the current profile in their own code.

     Thus the profile and scene collection discovery needs to happen before any access to that information (but after intializing global settings) to ensure legacy code gets valid path information.
     */
    m_mainSceneCollection->RefreshSceneCollections(true);

#if __APPLE__
    static bool bTestOnce = false;
    if(bTestOnce == false) {
        bTestOnce = true;
#endif
    //"---------------------------------"
    obs_load_all_modules2(&mfi);
    //"---------------------------------"
    obs_log_loaded_modules();
    // "---------------------------------"
    obs_post_load_modules();
    ///
#if __APPLE__
    }
#endif
    BPtr<char*> failed_modules = mfi.failed_modules;
    //

    OBSDataAutoRelease data = obs_get_private_data();
    m_vcamEnabled = obs_data_get_bool(data, "vcamEnabled");
    
    //Locale setting for obs-browser
    const char* currentLocale = LOCALE_CONTEXT.GetCurrentLocale();
    obs_set_locale(currentLocale);

    m_cefManager = std::make_unique<AFCefManager>();
    m_cefManager->InitBrowserPanelSafeBlock();

    auto& authManager = AUTH_CONTEXT;
    //
    authManager.LoadAllAuthed();
    authManager.InitSoopBroadInfo();

    m_blockManager->ApplyMoveInAllArea(this);

#ifdef _WIN32
    UpdaterKill();
#endif

    const bool soopLoginProcess = false;

    connect(this, &AFMainFrame::qsignalMainShowEventTriggered,
            this, &AFMainFrame::qslotShowFreecShotUnInstallAlert, Qt::QueuedConnection);

    bool loginRetain = false;
    if (soopLoginProcess) {
        auto loginStart = std::chrono::steady_clock::now();
        m_normalInit = CheckLoginSoopCookie(userID, soopCookie, loginRetain);
        auto loginEnd = std::chrono::steady_clock::now();
        g_loginDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loginEnd - loginStart);
    }
    else {
        RestoreSoopAccount("Temp Login SOOP", "");
    }

    if (!m_normalInit)
        return false;

	if (soopLoginProcess)
	{
		AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
		broadInfo->RequestCategoryListAPI();

		connect(authManager.GetSoopBroadInfo(), &AFQBroadInfo::checkResolution, this, &AFMainFrame::setResolution);

		authManager.RequestBroadInfoAPI(true);
		if (loginRetain)
			authManager.LoadSoopStreamerInfo();

		BroadInfoTimerStart();
        RefreshSoopCookiTimerStart();
	}

    //Remove GLOBAL
    authManager.RemoveChannel("SOOP Global");

    STATISTICS.InitializeValues();

    //No Main Account Exception Needed
    if (!m_leftNavigationBar->LeftNavigationBarInit())
        blog(LOG_WARNING, "LeftNavigationBarInit Failed");

    ui->horizontalLayout_LeftNavigationArea->insertWidget(0, m_leftNavigationBar);
    
    connect(m_leftNavigationBar, &AFQLeftNavigationBar::qsignalBlockButtonTriggered, this, &AFMainFrame::qslotShowDock);
    connect(m_leftNavigationBar, &AFQLeftNavigationBar::qsignalBlockPopupButtonTriggered, this, &AFMainFrame::qslotShowBlock);
    
    connect(m_blockManager, &AFQBlockManager::qsignalBlockVisible, this, &AFMainFrame::qslotBlockClosedTriggered);
    connect(m_blockManager, &AFQBlockManager::qsignalAddSource, m_pMainSceneSource, &CMainSceneSource::qslotAddSource);

    connect(this, &AFMainFrame::qsignalBroadToggled, 
        m_leftNavigationBar, &AFQLeftNavigationBar::RecieveBroadState);

    connect(this, &AFMainFrame::qsignalCertainMinuteBroadToggled,
        m_leftNavigationBar, &AFQLeftNavigationBar::CertainMinuteBroadToggled);

    m_dynamicCompositMainWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(m_dynamicCompositMainWindow, &AFMainDynamicComposit::PreviewShowRequested, this, &AFMainFrame::qslotTogglePreview);
    m_dynamicCompositMainWindow->MainWindowInit();
    //

    bool PropertiesOn = config_get_bool(USERCONFIG, "BasicWindow", "ShowContextToolbars");
    ui->action_ViewToggleProperties->setChecked(PropertiesOn);
    qslotPropertiesToggled(PropertiesOn);

    m_blockManager->InitPopups();

    // Account Menu Sync
    {
        QString loginText = QTStr("Login");
        AFChannelData* channelData;
        if (AUTH_CONTEXT.GetMainChannelData(channelData)) {
            QString channelID = QString::fromStdString(channelData->pAuthData->channelID);
            int maxLen = 9;
            if (channelID.length() > maxLen) {
                channelID = channelID.left(maxLen) + "...";
            }
            loginText = QTStr("Logout.With.Account").arg(channelID);
        }
        if (m_loginAction)
            m_loginAction->setText(loginText);
    }

    ui->widget_Basic->layout()->addWidget(m_dynamicCompositMainWindow);

    // Sutdio First Run Check
    bool firstRun = false;
    {
        const char* sceneCollectionFile = config_get_string(USERCONFIG, "Basic", "SceneCollectionFile");
        char savePath[1024];
        char fileName[1024];

        if (!sceneCollectionFile)
            throw "Failed to get scene collection name";

        ret = snprintf(fileName, sizeof(fileName), (LOCAL_FOLDER_NAME + "/basic/scenes/%s").c_str(), sceneCollectionFile);

        if (ret <= 0)
            throw "Failed to create scene collection file name";

        ret = GetAppConfigPath(savePath, sizeof(savePath), fileName);

        if (!os_file_exists(savePath)) {
            firstRun = true;
        }
    }
    m_mainProfile->UpdateProfileEncoders();
    if (firstRun) {
        
       //Show Guide After ShowEvent - UI fully set after show
        connect(this, &AFMainFrame::qsignalMainShowEventTriggered, this, &AFMainFrame::qslotMainFrameTutorial);
    }

    setProperty("IsFirstRun", firstRun);
    if(!firstRun)
        connect(this, &AFMainFrame::qsignalMainShowEventTriggered, this, &AFMainFrame::qslotInitShowSoopChat);

    if(!firstRun)
        connect(this, &AFMainFrame::qsignalMainShowEventTriggered,
                this, &AFMainFrame::qslotInitFreecShotPlusUpdateLog, Qt::QueuedConnection);

	m_bFirstOpen = LOADSAVE_CONTEXT.InitLoadSave();

    // audio mixer
    ToggleMixerLayout(config_get_bool(USERCONFIG, "BasicWindow", "VerticalVolControl"));

    // Add Preset Source
    _AddBroadPreset();

    _ConnectStatisticsSignals();

    // Init Network State Icon
    qslotResourceState(PCStatState::None);

    
    LogEncoders();
    //
    //
    m_pMainOutput->ResetOutputs();
    HOTKEY_CONTEXT.CreateHotkeys();

    // init service
    if(!m_serviceManager->InitService()) {
        throw "Failed to initialize service";
    }

    EnablePreviewDisplay(true);

#ifdef _WIN32
    SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);
    int enabled = 0;
    bool bRet = ::SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &enabled, 0);
    if (enabled)
    {
        bRet = ::SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, 0, (PVOID)0, SPIF_SENDCHANGE);
    }

    _DeleteBroadInfoData();    
#endif
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
    /*bool sysTrayEnabled = config_get_bool(USERCONFIG, "BasicWindow", "SysTrayEnabled");
    bool sysTrayWhenStarted = config_get_bool(USERCONFIG,  "BasicWindow", "SysTrayWhenStarted");
    bool hideWindowOnStart = QSystemTrayIcon::isSystemTrayAvailable() &&
        sysTrayEnabled &&
        (g_opt_minimize_tray || sysTrayWhenStarted);*/

#ifdef _WIN32
    SetWin32DropStyle(this);

#endif

    if (config_get_bool(ACTIVECONFIG, "General", "OpenStatsOnStartup"))
    {
        QTimer::singleShot(0, [this] {
            AFQBorderPopupBaseWidget* popup = nullptr;
            m_blockManager->MakePopup(ENUM_WINDOW_TYPE::StatPage, popup);
            });
    }

    //System Tray Disable
    //SystemTray(true);

    _RegisterSourceControlAction();

    m_pMainSceneSource->UpdateEditMenu();

    _SetResourceCheckTimer();
    connect(this, &AFMainFrame::qsignalRefreshTimerTick, this, &AFMainFrame::qslotRefreshMainResourceText);

    ReloadCustomBrowserMenu();

    ui->widget_TopMenu->setMinimumWidth(24);
    ui->widget_TopMenu->setMaximumWidth(24);
    ui->horizontalSpacer_Title->changeSize(8, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);

#ifdef _WIN32 
    m_pMainUpdate->UpdaterCheck();
#endif

    RestoreGeometry(this->property("IsFirstRun").toBool());

    connect(m_refreshVideoBallonTimer, &QTimer::timeout, this, &AFMainFrame::qslotRefreshVideoBalloonSource);

#ifdef _WIN32
    if (AUTH_CONTEXT.IsSoopRegistered() == true)
        m_overlayManager->Initialize();
#endif // _WIN32

    m_breakTimeManager->Initialize();
	{
        int cntOfAccount = authManager.GetCntChannel();
        for (int idx = 0; idx < cntOfAccount; idx++)
        {
            AFChannelData* tmpChannel = nullptr;
            authManager.GetChannelData(idx, tmpChannel);

            QPixmap* newObj = MakePixmapFromAuthData(tmpChannel->pAuthData);
            if (newObj != nullptr) {
                tmpChannel->pObjQtPixmap = newObj;
            }
        }

        AFChannelData* tmpMainChannel = nullptr;
        authManager.GetMainChannelData(tmpMainChannel);

        if (tmpMainChannel != nullptr)
        {
            QPixmap* newObj = MakePixmapFromAuthData(tmpMainChannel->pAuthData);
            if (newObj != nullptr) {
                tmpMainChannel->pObjQtPixmap = newObj;
            }
        }
    }

    /* ------------------------------------------- */
    /* display warning message for failed modules  */

    if(mfi.count) {
        QString failed_plugins;

        char** plugin = mfi.failed_modules;
        while(*plugin) {
            failed_plugins += *plugin;
            failed_plugins += "\n";
            plugin++;
        }

        QString failed_msg = QTStr("PluginsFailedToLoad.Text").arg(failed_plugins);
        QMessageBox::warning(this, QTStr("PluginsFailedToLoad.Title"), failed_msg);
    }

    InitLnbMenuItems();
    
    return true;
}

#ifdef _WIN32
static inline void UpdateProcessPriority()
{
    const char* priority = config_get_string(APPCONFIG, "General", "ProcessPriority");
    if (priority && strcmp(priority, "Normal") != 0)
        SetProcessPriority(priority);
}

static inline void ClearProcessPriority()
{
    const char* priority = config_get_string(APPCONFIG, "General", "ProcessPriority");
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
    AFQCustomMenu* profileMenu = _FindSubMenuByTitle(m_topMenu, QT_UTF8(Str("Basic.MainMenu.ProfileFix")));
    if (profileMenu->isEnabled() || force) {
        profileMenu->setEnabled(false);
        //ui->autoConfigure->setEnabled(false);
        INHIBITSLEEP_CONTEXT.IncrementSleepInhibition();
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
    if (m_loginAction)
    {
        if (m_loginAction->isEnabled() || force) {
            m_loginAction->setEnabled(false);
        }
    }
}

void AFMainFrame::OnDeactivate()
{
    AFQCustomMenu* profileMenu = _FindSubMenuByTitle(m_topMenu, QTStr("Basic.MainMenu.ProfileFix"));
    if (!AFOutputUtil::IsActive() && !profileMenu->isEnabled()) {
        profileMenu->setEnabled(true);
        //ui->autoConfigure->setEnabled(true);
        INHIBITSLEEP_CONTEXT.DecrementSleepInhibition();
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

    if (m_loginAction)
    {
        if (!AFOutputUtil::IsActive() && !m_loginAction->isEnabled()) {
            m_loginAction->setEnabled(true);
        }
    }
}

void AFMainFrame::ConnectSignalForScreen()
{
    if (m_leftNavigationBar)
        m_leftNavigationBar->ConnectNavigationBarScreen();
}

QSize AFMainFrame::GetContentsArea()
{
    return ui->widget_MainArea->size();
}
int AFMainFrame::GetFrameBottomPosY()
{
    return ui->widget_MainArea->height() - ui->frame_Bottom->height();
}


void AFMainFrame::qslotSaveProject()
{
    if (LOADSAVE_CONTEXT.CheckCanSaveProject())
        QMetaObject::invokeMethod(this, "qslotSaveProjectDeferred", Qt::QueuedConnection);
}

void AFMainFrame::qslotSaveProjectDeferred()
{
    LOADSAVE_CONTEXT.SaveProjectDeferred();
}

void AFMainFrame::RestoreMainWindow()
{
    const char* dockStateStr = config_get_string(USERCONFIG, "BasicWindow", "DockState");
    if(!dockStateStr) {
        ResetDockUI();
    } else {
        QByteArray dockState = QByteArray::fromBase64(QByteArray(dockStateStr));
        if(!DYNAMIC_COMPOSIT->restoreState(dockState))
            ResetDockUI();
        else
            m_blockManager->ShowAllCustomBrowser();
    }
}

bool AFMainFrame::CreateSourceProperties(obs_source_t* source, bool fromDock)
{
    if (!source)
        return false;

    const char* id = obs_source_get_id(source);

    bool closed = true;
    closed = HideSourceProperties(source);

    if (!closed)
        return false;

    bool basicProp = false;
    int  sourceType = 0;

    QDialog* prop = nullptr;
    if (AFSourceUtil::IsSoopVodSource(id))
    {
        SOOP_VOD_TYPE type = m_soopSrcManager->GetSoopVodSourceType(id);
        prop = m_sourcePropsPtr[id];
        if (!prop)
            prop = new AFQVodSourceDialog(this, source);
        else {
            AFQVodSourceDialog* vodDialog = qobject_cast<AFQVodSourceDialog*>(prop);

            if (vodDialog) {
                vodDialog->SetSource(source);
                vodDialog->raise();
                vodDialog->show();
            }
        }
            prop->raise();
    }
    else if (0 == strcmp(id, "soop_directbroad_source")) {
        prop = new AFQDirectBroadDialog(this, source);
    }
    else if (0 == strcmp(id, "soop_tv_cable_source")) {
        prop = new AFQTvBroadDialog(this, source);
    }
    else if (nullptr != strstr(id, "soop_kbo_graphic_source_") ||
        nullptr != strstr(id, "soop_commerce_source_")) {
        prop = new AFQDowoomiDialog(this, source);
    }
    else if (nullptr != strstr(id, "soop_football_graphic_source_")) {
        prop = new AFQDowoomiDialog(this, source);
    }
    else if (0 == strcmp(id, "soop_chat_source_score")) {
        prop = new AFQDowoomiScoreDialog(this, source);
    }
    else if(nullptr != strstr(id, "soop_chat_source_mood_check"))
    {
        prop = new AFQDowoomiMoodCheckDialog(this, source);
    }
    else if(nullptr != strstr(id, "soop_chat_source_anmSubtitle")) {
        prop = new AFQAquaSubtitleDialog(this, source);
    }
    else if (nullptr != strstr(id, "soop_chat_source_")) {
        prop = new AFQDowoomiDialog(this, source);
    }
    else if (nullptr != strstr(id, "soop_particle_effect_source")) {
        prop = new AFQParticleEffectDialog(this, source);
    }
    else if (nullptr != strstr(id, "soop_videoballoon_source")) {
        prop = new AFQVideoBalloonProps(this, source);
    }
    else if (nullptr != strstr(id, "soop_mission_source_"))
    {
        int type = (0 == strcmp(id, "soop_mission_source_battle_joinusers") ? 1 :
            0 == strcmp(id, "soop_mission_source_battle_fundingrank") ? 2 : -1);

        prop = new AFQMissionDonationRankDialog(this, source, type);
    }
    else {
        m_sourceProperties = new AFQSourceProperties(this, source);
        prop = m_sourceProperties;
        basicProp = true;
    }

    setCenterPositionNotUseParent(prop, this);
    AFQBlockManager::ApplyMoveInAllArea(prop);
    prop->setModal(false);
    if (!AFSourceUtil::IsSoopVodSource(id)) {
        prop->setAttribute(Qt::WA_DeleteOnClose, true);
    }

    int sourceX = this->x() + (this->width() / 2) - (prop->width() / 2);
    int sourceY = this->y() + (this->height() / 2) - (prop->height() / 2);

    QRect adjust;
    QRect originRect = QRect(sourceX, sourceY, prop->width(), prop->height());
    m_blockManager->AdjustPositionOutSideFullScreen(originRect, adjust);
    prop->setGeometry(adjust);

    prop->show();

    prop->move(prop->x(), prop->y() - 1);

    //Layout Minium Window Size

    if (!basicProp)
        m_sourcePropsPtr[id] = prop;

    return true;
}

bool AFMainFrame::HideSourceProperties(obs_source_t* source)
{
    if(!source)
        return false;

    bool closed = true;
    const char* sourceId = obs_source_get_id(source);

    for(auto& [id, prop] : m_sourcePropsPtr) {
        if(prop) {
            if(!AFSourceUtil::IsSoopVodSource(id.c_str())) {
                if(prop && prop->isVisible()) {
                    bool _closed = prop->close();
                    if(!_closed) {
                        closed = false;
                    }
                }
            } else {
                prop->hide();
                closed = true;
            }
        }
    }

    if(m_sourceProperties)
        if(m_sourceProperties && m_sourceProperties->isVisible())
            closed = m_sourceProperties->close();

    return closed;
}

void AFMainFrame::CreateSceneTransitionPopup(OBSSource source, int duration)
{
    bool closed = true;
    if (m_sceneTransitionPopup)
        closed = m_sceneTransitionPopup->close();

    if (!closed)
        return;

    m_sceneTransitionPopup = new AFQSceneTransitionsDialog(this, source, duration);
    m_sceneTransitionPopup->setAttribute(Qt::WA_DeleteOnClose);
    m_sceneTransitionPopup->setModal(false);

    m_blockManager->ApplyMoveInAllArea(m_sceneTransitionPopup);
    m_sceneTransitionPopup->show();
    EnableTransitionWidgets();
}

void AFMainFrame::CreateSignatureAIPopup(bool reactionAble)
{
    bool closed = true;
	if (m_signatureAIPopup) {
		if (m_signatureAIPopup->isVisible()) {
			return;
		}
		else {
			closed = m_signatureAIPopup->close();
		}
	}

    m_signatureAIPopup = new AFQSignaturePopup(reactionAble, this);
    if(!reactionAble)
        connect(m_signatureAIPopup, &AFQSignaturePopup::qsignalCloseSignature,
                m_leftNavigationBar, &AFQLeftNavigationBar::qslotCloseSignature);

    m_signatureAIPopup->setAttribute(Qt::WA_DeleteOnClose);
    m_signatureAIPopup->setModal(false);
    m_signatureAIPopup->show();
}

bool AFMainFrame::IsSmallResolution()
{
    QList<QScreen*> screens = QGuiApplication::screens();
    for (QScreen* screen : screens)
    {
        QRect geom = screen->geometry();
        if (geom.width() == 800 && geom.height() == 600)
            return true;
    }
    return false;
}

int AFMainFrame::GetLeftNavigationBarWidth()
{
    int retVal = 0;
    if (m_leftNavigationBar)
        retVal = m_leftNavigationBar->width();
    return retVal;
}

int AFMainFrame::GetTopAreaHeight()
{
    int retVal = 0;
    if (ui->widget_Top && ui->widget_Top->isVisible())
        retVal = ui->widget_Top->height();
    return retVal;
}

int AFMainFrame::GetBottomAreaHeight()
{
    int retVal = 0;
    if (ui->frame_Bottom && ui->frame_Bottom->isVisible())
        retVal = ui->frame_Bottom->height();

    return retVal;
}

void AFMainFrame::ResetDockUI()
{
    AFQBaseDockWidget* dock = nullptr;

    QList<ENUM_WINDOW_TYPE> list = {
        ENUM_WINDOW_TYPE::SceneSource,
        ENUM_WINDOW_TYPE::Mission,
        ENUM_WINDOW_TYPE::Vote,
        ENUM_WINDOW_TYPE::Extensions,
        ENUM_WINDOW_TYPE::AquaControl
    };

    if (!m_blockManager->GetDock(ENUM_WINDOW_TYPE::SoopChat, dock))
        list.append(ENUM_WINDOW_TYPE::SoopChat);

    list.append(ENUM_WINDOW_TYPE::BroadInfo);

    m_blockManager->CloseAllBlocks(list);
    m_blockManager->InitDefaultDock();
    if (AUTH_CONTEXT.IsSoopRegistered())
        m_leftNavigationBar->qslotBlockStatusChanged(false, ENUM_WINDOW_TYPE::AudioMixer, false);
}

int AFMainFrame::BottomControlHeight()
{
    int retHeight = 0;
    if (ui->widget_BottomControl)
        retHeight = ui->widget_BottomControl->height();
    return retHeight;
}

int AFMainFrame::LNBWidth()
{
    int retWidth = 0;
    if (m_leftNavigationBar)
        retWidth = m_leftNavigationBar->width();
    return retWidth;
}

void AFMainFrame::UpdateContextToolBarDeferred(bool force)
{
    QMetaObject::invokeMethod(this, "qslotUpdateContextToolBar",
        Qt::QueuedConnection, Q_ARG(bool, force));
}

void AFMainFrame::ShowBrowserInteractionPopup(OBSSource source)
{
    if(!source)
        return;

    AFQBrowserInteraction* interaction = nullptr;

    auto it = m_mapBrowserInteraction.find(source);
    if (m_mapBrowserInteraction.end() == it)
    {
        interaction = new AFQBrowserInteraction(this, source);
        interaction->setAttribute(Qt::WA_DeleteOnClose, true);
        interaction->setModal(false);

        m_blockManager->ApplyMoveInAllArea(interaction);

        connect(interaction, &AFQBrowserInteraction::qsignalClearPopup,
                this, &AFMainFrame::qslotClearBrowserInteractionPopup);

        m_mapBrowserInteraction.insert(source, interaction);

    } else {
        interaction = (*it);
    }

    if(!interaction)
        return;

    if (!interaction->isVisible()) {
        QRect targetRect = frameGeometry();
        QSize newSize = interaction->size();

        int newX = targetRect.x() + (targetRect.width() - newSize.width()) / 2;
        int newY = targetRect.y() + (targetRect.height() - newSize.height()) / 2;

        if (IsSmallResolution())
            interaction->resize(780, 550);

        QRect adjust;
        QRect interactionRect = QRect(newX, newY,interaction->width(), interaction->height());
        m_blockManager->AdjustPositionOutSideFullScreen(interactionRect, adjust);

        interaction->setGeometry(adjust);

        interaction->show();
    }

    interaction->raise();
}

void AFMainFrame::HideBrowserInteractionPopup(OBSSource source)
{
    MAP_BROWSER_INTERACTION::iterator it = m_mapBrowserInteraction.find(source);
    if (m_mapBrowserInteraction.end() == it)
        return;

    AFQBrowserInteraction* interaction = (*it);
    if (!interaction)
        return;

    interaction->close();
    interaction = nullptr;

    m_mapBrowserInteraction.remove(source);
}

void AFMainFrame::ApplyBroadInfoToUI()
{
    if (m_blockManager)
        m_blockManager->ApplyBroadInfoToUI();
}

void AFMainFrame::RefreshBroadInfoDockUI(bool requestAPI)
{
    QWidget* outBlock = nullptr;
    if (!m_blockManager->FindBlock(ENUM_WINDOW_TYPE::BroadInfo, outBlock))
        return;

    if(requestAPI)
        AUTH_CONTEXT.RequestBroadInfoAPI();

    AFBroadInfoDockWidget* broadInfoBlock = reinterpret_cast<AFBroadInfoDockWidget*>(outBlock);
    broadInfoBlock->LoadBroadInfoUI();
}

void AFMainFrame::SplitVodSaved()
{
    ui->label_BroadTime->qslotVodSplitSaved();
}

void AFMainFrame::RecvRemovedSource(OBSSceneItem item)
{
    if (!item)
        return;

    OBSSource source = obs_sceneitem_get_source(item);
    if (!source)
        return;

    // Check Hide Properties
    if (AFSourceUtil::IsSoopSource(source)) {

        if (AFSourceUtil::IsSoopMediaSource(source)) {
            // Detach SOOP Media Source
            m_soopSrcManager->SetSoopMediaSource(nullptr, true);
        }
        HideSourceProperties(source);
    }
    else {
        if (m_sourceProperties && m_sourceProperties->isVisible()) {
            if (m_sourceProperties->GetOBSSource() == source) {
                m_sourceProperties->close();
            }
        }
    }

    HideBrowserInteractionPopup(source);
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

void AFMainFrame::ShowSystemAlert(QString alertText, QString channelID, AFQSystemAlert::AlertIcon icon)
{
    if (m_systemAlert)
        m_systemAlert->close();

    int studioWidth = 400;

    bool isMainMinimized = this->isMinimized();
    if (isMainMinimized) {
        QScreen* primaryScreen = QGuiApplication::primaryScreen();
        if(primaryScreen)
            studioWidth = primaryScreen->size().width() / 2;
    }
    else {
        studioWidth = this->width();
    }

    m_systemAlert = new AFQSystemAlert(nullptr,
                                        alertText,
                                        channelID,
                                        isMainMinimized,
                                        studioWidth,
                                        icon);
    m_systemAlert->show();
    _MoveSystemAlert(m_systemAlert, isMainMinimized);
}

void AFMainFrame::EnableReplayBuffer(bool enable)
{
    QWidget* outBlock = nullptr;
    if (m_blockManager->FindBlock(ENUM_WINDOW_TYPE::AdvanceControls, outBlock))
    {
        AFAdvanceControlsWidget* advanceControl = reinterpret_cast<AFAdvanceControlsWidget*>(outBlock);
        advanceControl->EnableReplayBuffer(enable);
    }
}

void AFMainFrame::SetReplayBufferStartStopMode(bool bufferStart)
{
    QWidget* outBlock = nullptr;
    if (m_blockManager->FindBlock(ENUM_WINDOW_TYPE::AdvanceControls, outBlock))
    {
        AFAdvanceControlsWidget* advanceControl = reinterpret_cast<AFAdvanceControlsWidget*>(outBlock);
        SetReplayBufferReleased();
        advanceControl->SetReplayBufferStartStopStyle(bufferStart);
    }
}

void AFMainFrame::SetReplayBufferStoppingMode()
{
    QWidget* outBlock = nullptr;
    if (m_blockManager->FindBlock(ENUM_WINDOW_TYPE::AdvanceControls, outBlock))
    {
        AFAdvanceControlsWidget* advanceControl = reinterpret_cast<AFAdvanceControlsWidget*>(outBlock);
        advanceControl->SetReplayBufferStoppingStyle();
    }
}

void AFMainFrame::SetReplayBufferReleased()
{
    QWidget* outBlock = nullptr;
    if (m_blockManager->FindBlock(ENUM_WINDOW_TYPE::AdvanceControls, outBlock))
    {
        AFAdvanceControlsWidget* advanceControl = reinterpret_cast<AFAdvanceControlsWidget*>(outBlock);
        advanceControl->ReplayBufferReleased();
    }
}

//Check if broadcast start routine is in progress (prevent duplicate call broad start button)
void AFMainFrame::OffBroadStartAPICheck()
{
    m_checkBroadStartAPI = false;
    m_CheckBroadStartAPITimer->stop();
}

void AFMainFrame::ShutDown()
{
    blog(LOG_INFO, "=== Shutdown Start =================");

    if(AFOutputUtil::IsStreamActive())
    {
        if(AUTH_CONTEXT.IsSoopStreaming())
        {
            AFQBroadInfo* soopBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
            if(soopBroadInfo) {
                QList<QVariant> values = {soopBroadInfo->BroadNumber(), 1, 10};

                m_soopApiHandler->postAPIfromId(POST_KR_CLOSE_REQUEST, values);
            }

            if(m_breakTimeManager->IsActive()) {
                m_breakTimeManager->Stop(true);
            }
        }
        qslotStopStreaming();
    }

    blog(LOG_INFO, "=== Shutdown End =================");
}

//
bool AFMainFrame::IsPreviewProgramMode()
{
    return STATEAPP.IsPreviewProgramMode();
}
void AFMainFrame::EnablePreviewDisplay(bool enable)
{
    obs_display_set_enabled(MAIN_PREVIEW->GetDisplay(), enable);
    MAIN_PREVIEW->setVisible(enable);
    m_dynamicCompositMainWindow->GetNotPreviewFrame()->setVisible(!enable);
}


void AFMainFrame::SetStudioModeStatus(bool studioMode)
{
    if (m_leftNavigationBar)
        m_leftNavigationBar->qslotStudioModeStatusChanged(studioMode);
}

void AFMainFrame::SystemTray(bool firstStarted)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;
    if (!m_trayIcon && !firstStarted)
        return;

    bool sysTrayWhenStarted = config_get_bool(USERCONFIG, "BasicWindow", "SysTrayWhenStarted");
    bool sysTrayEnabled = config_get_bool(USERCONFIG, "BasicWindow", "SysTrayEnabled");

    if (firstStarted)
        SystemTrayInit();

    if (!sysTrayEnabled) {
        m_trayIcon->hide();
    }
    else {
        m_trayIcon->show();
        if (firstStarted && (sysTrayWhenStarted || g_opt_minimize_tray)) {
            EnablePreviewDisplay(false);
#ifdef __APPLE__
            EnableOSXDockIcon(false);
#endif
            g_opt_minimize_tray = false;
        }
    }

    if (isVisible())
        m_mainShowHideAction->setText(QTStr(("Basic.SystemTray.Hide")));
    else
        m_mainShowHideAction->setText(QTStr(("Basic.SystemTray.Show")));
}

void AFMainFrame::SystemTrayInit()
{
    QIcon trayIconFile;
    LoadIconFromABSPath("assets/platform/default/soopglobal.png", trayIconFile);

#ifdef __APPLE__
    //QIcon trayIconFile = QIcon(":/image/resource/Platform/default/soop.png");
    trayIconFile.setIsMask(true);
#else
    //QIcon trayIconFile = QIcon(":/image/resource/Platform/default/soop.png");
#endif
    m_trayIcon.reset(new QSystemTrayIcon(
        QIcon::fromTheme("obs-tray", trayIconFile), this));
    m_trayIcon->setToolTip("OBS Studio");

    m_mainShowHideAction = new QAction(QTStr("Basic.SystemTray.Show"), m_trayIcon.data());
    m_systemTrayStreamAction = new QAction(QTStr("Basic.Main.StartStreaming"));
        /*m_dynamicCompositMainWindow->StreamingActive() ? QT_UTF8(Str("Basic.Main.StopStreaming"))
        : QT_UTF8(Str("Basic.Main.StartStreaming")),
        m_TrayIcon.data());*/
    m_systemTrayRecordAction = new QAction(QTStr("Basic.Main.StartRecording"));
        /*m_dynamicCompositMainWindow->RecordingActive() ? QT_UTF8(Str("Basic.Main.StopRecording"))
        : QT_UTF8(Str("Basic.Main.StartRecording")),
        m_TrayIcon.data());*/
    /*sysTrayReplayBuffer = new QAction(
        ReplayBufferActive() ? QT_UTF8(Str("Basic.Main.StopReplayBuffer"))
        : QT_UTF8(Str("Basic.Main.StartReplayBuffer")),
        trayIcon.data());*/
    /*sysTrayVCam = new QAction(
        VCamActive() ? QTStr("Basic.Main.StopVCam")
        : QTStr("Basic.Main.StartVCam"),
        trayIcon.data());*/
    m_exitAction = new QAction(QTStr("Exit"), m_trayIcon.data());
    
    m_trayMenu = new AFQCustomMenu(this);
    m_trayMenu->setFixedWidth(300);
    m_previewProjector = new AFQCustomMenu(QTStr("PreviewProjector"), m_trayMenu);
    m_studioProgramProjector = new AFQCustomMenu(QTStr("StudioProgramProjector"), m_trayMenu);
    /*AddProjectorMenuMonitors(previewProjector, this,
        &OBSBasic::OpenPreviewProjector);
    AddProjectorMenuMonitors(studioProgramProjector, this,
        &OBSBasic::OpenStudioProgramProjector);*/

    m_trayMenu->addAction(m_mainShowHideAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addMenu(m_previewProjector);
    m_trayMenu->addMenu(m_studioProgramProjector);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_systemTrayStreamAction);
    m_trayMenu->addAction(m_systemTrayRecordAction);
    m_trayMenu->addAction(m_systemTrayReplayBufferAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_exitAction);
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->show();


    connect(m_trayIcon.data(), &QSystemTrayIcon::activated, this, &AFMainFrame::qslotIconActivated);
    connect(m_mainShowHideAction, &QAction::triggered, this, &AFMainFrame::qslotToggleShowHide);
    connect(m_systemTrayStreamAction, &QAction::triggered, this, &AFMainFrame::qslotCheckBroadAvailable);
    connect(m_systemTrayRecordAction, &QAction::triggered, this, &AFMainFrame::qslotChangeRecordState);
    connect(m_exitAction, &QAction::triggered, this, &AFMainFrame::close);
}

void AFMainFrame::RestoreGeometry(bool firstRun)
{
    if (!firstRun)
    {
        const char* strGeometry = config_get_string(USERCONFIG, "BasicWindow", "geometry");
        if (strGeometry != NULL) {
            QByteArray byteArray = QByteArray::fromBase64(QByteArray(strGeometry));
            restoreGeometry(byteArray);

            //Check Position
            QString strPoint = QString(config_get_string(USERCONFIG, "BasicWindow", "MainframeCheckRect"));
            QStringList parts = strPoint.split('.');

            //double check rect
            QRect checkRect;
            if (parts.size() == 4) {
                checkRect.setX(parts[0].toInt());
                checkRect.setY(parts[1].toInt());
                checkRect.setWidth(parts[2].toInt());
                checkRect.setHeight(parts[3].toInt());

                if (checkRect.x() != pos().x() || checkRect.y() != pos().y())
                    move(checkRect.x(), checkRect.y());

                if (checkRect.width() != size().width() || checkRect.height() != size().height())
                    resize(checkRect.width(), checkRect.height());
            }

            QRect adjustRect = geometry();
            m_blockManager->AdjustPositionOutSideFullScreen(geometry(), adjustRect);
            this->setGeometry(adjustRect);

            QRect rect = geometry();
            m_graphicContext->SetMainPreviewY(rect.height());
            m_graphicContext->SetMainPreviewCY(rect.y());

            QRect windowGeometry = normalGeometry();
            if (!WindowPositionValid(windowGeometry)) {
                rect = QGuiApplication::primaryScreen()->geometry();
                setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), rect));
            }

            m_blockManager->RestoreMagnetPopup();

            return;
        }
    }

    // Init Studio Geometry
    int initStudioWidth = 900;
    int initStudioHeight = 860;

    if (IsSmallResolution())
    {
        initStudioWidth = minimumWidth();
        initStudioHeight = minimumHeight();
    }

    QRect screenGeometry = QApplication::screens().first()->geometry();
    int x = (screenGeometry.width() - initStudioWidth) / 2;
    int y = (screenGeometry.height() - initStudioHeight) / 2;

    QRect adjustRect = QRect(x, y, initStudioWidth, initStudioHeight);
    setGeometry(adjustRect);
}

void AFMainFrame::HotkeyTriggered(void* data, obs_hotkey_id id, bool pressed)
{
    QWidget* focusWidget = QApplication::focusWidget();
    if(focusWidget) {
        bool isTyping = qobject_cast<QLineEdit*>(focusWidget) ||
            qobject_cast<QTextEdit*>(focusWidget) ||
            qobject_cast<QPlainTextEdit*>(focusWidget) ||
            qobject_cast<QAbstractSpinBox*>(focusWidget);

        if(isTyping) {
            return;
        }
    }

    AFMainFrame& mainFrame = *static_cast<AFMainFrame*>(data);
    QMetaObject::invokeMethod(&mainFrame, "qslotProcessHotkey",
                              Q_ARG(obs_hotkey_id, id),
                              Q_ARG(bool, pressed));
}

void AFMainFrame::SetPCStateIconStyle(QLabel* label, PCStatState state)
{
    if (state == PCStatState::None)
        label->setProperty("pcStat", "none");
    else if (state == PCStatState::Normal)
        label->setProperty("pcStat", "normal");
    else // Error
        label->setProperty("pcStat", "error");

    PolishStyleSheet(label);
}

bool AFMainFrame::EnableStartStreaming()
{
    return (!AFOutputUtil::IsStreamActive() && !m_signalFlags.streamingStarting);
}

bool AFMainFrame::EnableStopStreaming()
{
    return (AFOutputUtil::IsStreamActive() && !m_signalFlags.streamingStarting);
}

bool AFMainFrame::EnableStartRecording()
{
    return (!AFOutputUtil::IsRecordingActive() && !m_signalFlags.recordingStarted);
}

bool AFMainFrame::EnableStopRecording()
{
    return (AFOutputUtil::IsRecordingActive() && m_signalFlags.recordingStarted);
}

bool AFMainFrame::EnablePauseRecording()
{
    return (m_signalFlags.isRecordingPausable && !m_signalFlags.recordingPaused);
}

bool AFMainFrame::EnableUnPauseRecording()
{
    return (m_signalFlags.isRecordingPausable && m_signalFlags.recordingPaused);
}

void AFMainFrame::ChangeStreamStateUI(bool enable, bool checked, QString title, int width)
{
    ui->pushButton_Broad->setEnabled(enable);
    ui->pushButton_Broad->setChecked(checked);
    ui->pushButton_Broad->setText(title);
    ui->pushButton_Broad->setFixedWidth(width);

    ui->widget_BroadAndRecordButtons->adjustSize();

    ui->pushButton_Broad->setProperty("IsLive", checked);
    style()->unpolish(ui->pushButton_Broad);
    style()->polish(ui->pushButton_Broad);
}

void AFMainFrame::ChangeRecordStateUI(bool enable, bool checked, QString title, int width)
{
    ui->pushButton_Record->setEnabled(enable);
    ui->pushButton_Record->setChecked(checked);
    ui->pushButton_Record->setFixedWidth(width);
    ui->pushButton_Record->setText(title);

    if (checked) 
    {
        ui->label_RecordTime->StartCount();
        ui->widget_RecordTimer->setVisible(true);
        if (ui->widget_BroadTimer->isVisible())
            ui->line_Time->setVisible(true);
    }
    else 
    {
        ui->label_RecordTime->StopCount();
        ui->widget_RecordTimer->setVisible(false);
        ui->line_Time->setVisible(false);
    }
    ui->pushButton_Record->setProperty("IsRec", checked);
    style()->unpolish(ui->pushButton_Record);
    style()->polish(ui->pushButton_Record);
}

bool AFMainFrame::CheckSplitVodAvailable()
{
    if (CheckSplitVodByUI())
    {
        if (AUTH_CONTEXT.VodSaveAvailableFromAPI())
        {
            //Category Check
            return true;
        }
    }

    return false;
}

void AFMainFrame::ToggleBroadTimerUI(bool start)
{
    if (start)
    {
        ui->label_BroadTime->StartCount();
        connect(ui->label_BroadTime, &AFQTimerLabel::qsignalCertainMinuteBroad,
                this, &AFMainFrame::qsignalCertainMinuteBroadToggled);
    }
    else
    {
        ui->label_BroadTime->StopCount();
        disconnect(ui->label_BroadTime, &AFQTimerLabel::qsignalCertainMinuteBroad,
                   this, &AFMainFrame::qsignalCertainMinuteBroadToggled);
    }

    ui->widget_BroadTimer->setVisible(start);
    if (ui->widget_RecordTimer->isVisible())
        ui->line_Time->setVisible(start);

    _AccountButtonStreamingToggle(start);
}

QString AFMainFrame::GetBroadTimerUITime()
{
    return ui->label_BroadTime->GetHHMMSS();
}

void AFMainFrame::StartStreaming()
{
    qslotCheckBroadAvailable(true);
}

void AFMainFrame::StopStreaming()
{
    qslotCheckBroadAvailable(false);
}

void AFMainFrame::ForceStopStreaming()
{
    qslotForceStopStreaming();
}

void AFMainFrame::StartRecording()
{
    qslotChangeRecordState(true);
}

void AFMainFrame::StopRecording()
{
    qslotChangeRecordState(false);
}

//void AFMainFrame::PauseRecording()
//{
//}
//
//void AFMainFrame::UnPauseRecording()
//{
//}

void AFMainFrame::StartReplayBuffer()
{
    qslotStartReplayBuffer();
}

void AFMainFrame::EnablePreview()
{
    if (IsPreviewProgramMode())
        return;

    m_previewEnabled = true;
    EnablePreviewDisplay(true);
}

void AFMainFrame::DisablePreview()
{
    if (IsPreviewProgramMode())
        return;

    m_previewEnabled = false;
    EnablePreviewDisplay(false);
}

void AFMainFrame::EnablePreviewProgam()
{
    AFMainDynamicComposit* freecshotWindow = GetMainWindow();
    if (freecshotWindow)
        freecshotWindow->ToggleStudioModeBlock(true);
}

void AFMainFrame::DiablePreviewProgam()
{
    AFMainDynamicComposit* freecshotWindow = GetMainWindow();
    if (freecshotWindow)
        freecshotWindow->ToggleStudioModeBlock(false);
}

void AFMainFrame::VerifyEmailBeforeBroad(std::string failMsg, std::string url)
{
    QPointer<AFMainFrame> self(this);

    QString msg = QString::fromStdString(failMsg);
    bool result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
        "", msg, false, true);

    if (!self) return;

    if (result == QDialog::Accepted)
    {
        const QByteArray urlUtf8 = QByteArray::fromStdString(url);
        if (m_soopApiHandler)
            m_soopApiHandler->getAPI("BS_EMAIL_CERTIFY", urlUtf8.constData(), this, "qslotEmailVerify", QList<int>());
    }
}

void AFMainFrame::NavigateDefaultBrowser(QString Url)
{
    if (Url.isEmpty())
        return;

    QDesktopServices::openUrl(Url);
    return;
    
}

void AFMainFrame::ReloadCustomBrowserMenu()
{
    AFQCustomMenu* addon = _FindSubMenuByTitle(m_topMenu, QTStr("Basic.MainMenu.Addon"));

    if (addon)
    {
        AFQCustomMenu* customMenu = _FindSubMenuByTitle(m_topMenu,
            QTStr("Basic.MainMenu.Addon.CustomBrowserDocks"));

        if (customMenu)
            addon->removeAction(customMenu->menuAction());
        
        if (!addon->actions().isEmpty()) {
            auto first_action = addon->actions().first();
            addon->insertMenu(first_action, CreateCustomBrowserMenu());
        }
        else {
            addon->addMenu(CreateCustomBrowserMenu());
        }
    }

    AFQBorderPopupBaseWidget* browserCollection = nullptr;
    if (m_blockManager->GetPopup(ENUM_WINDOW_TYPE::CustomBrowserCollection, browserCollection))
        if (browserCollection)
            browserCollection->raise();
}

AFQCustomMenu* AFMainFrame::CreateCustomBrowserMenu()
{
    AFQCustomMenu* customBrowserMenu = new AFQCustomMenu(Str("Basic.MainMenu.Addon.CustomBrowserDocks"), this, true);
    customBrowserMenu->setFixedWidth(200);

    QAction* browserList = new QAction(customBrowserMenu);
    browserList->setText(Str("Basic.MainMenu.Addon.CustomBrowserList"));

    browserList->setProperty("windowtype", ENUM_WINDOW_TYPE::CustomBrowserCollection);
    connect(browserList, &QAction::triggered, this, &AFMainFrame::qslotShowBlockWithProperty);

    customBrowserMenu->addAction(browserList);
    customBrowserMenu->addSeparator();

    m_blockManager->CreateCustomBrowserListMenu(customBrowserMenu);

    return customBrowserMenu;
}

AFQCustomMenu* AFMainFrame::CreateFullScreenProjectorMenu()
{
    AFQCustomMenu* customBrowserMenu = new AFQCustomMenu(Str("SceneProjector"), this, true);
    AFMainFrame::AddProjectorMenuMonitors(customBrowserMenu, this, &AFMainFrame::qslotShowProjector);

    return customBrowserMenu;
}

template<typename Receiver, typename ...Args>
void AFMainFrame::AddProjectorMenuMonitors(QMenu* parent, Receiver* target, void(Receiver::* slot)(Args...))
{
    auto projectors = GetProjectorMenuMonitorsFormatted();
    for (int i = 0; i < projectors.size(); i++) {
        QString str = projectors[i];
        QAction* action = parent->addAction(str, target, slot);
        action->setProperty("monitor", i);
    }
}

QList<QString> AFMainFrame::GetProjectorMenuMonitorsFormatted()
{
    QList<QString> projectorsFormatted;
    QList<QScreen*> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); i++) {
        QScreen* screen = screens[i];
        QRect screenGeometry = screen->geometry();
        qreal ratio = screen->devicePixelRatio();
        QString name = "";
#if defined(_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
        QTextStream fullname(&name);
        fullname << GetMonitorName(screen->name());
        fullname << " (";
        fullname << (i + 1);
        fullname << ")";
#elif defined(__APPLE__) || defined(_WIN32)
        name = screen->name();
#else
        name = screen->model().simplified();

        if (name.length() > 1 && name.endsWith("-"))
            name.chop(1);
#endif
        name = name.simplified();

        if (name.length() == 0) {
            name = QString("%1 %2")
                .arg(QTStr("Display"))
                .arg(QString::number(i + 1));
        }
        QString str = QString("%1: %2x%3 @ %4,%5")
            .arg(name, QString::number(screenGeometry.width() * ratio),
                 QString::number(screenGeometry.height() * ratio),
                 QString::number(screenGeometry.x()),
                 QString::number(screenGeometry.y()));
        projectorsFormatted.push_back(str);
    }
    return projectorsFormatted;
}

void AFMainFrame::CustomBrowserStateChanged(QString uuid, bool isOpen)
{
    AFQCustomMenu* addon = _FindSubMenuByTitle(m_topMenu, QTStr("Basic.MainMenu.Addon"));

    if (addon)
    {
        AFQCustomMenu* customMenu = _FindSubMenuByTitle(m_topMenu,
            QTStr("Basic.MainMenu.Addon.CustomBrowserDocks"));

        if (customMenu)
        {
            QList<QAction*> actions = customMenu->actions();
            for (int i = 0; i < actions.count(); i++)
            {
                QString propertyuuid = actions[i]->property("uuid").toString();
                if (propertyuuid == uuid)
                {
                    actions[i]->setChecked(isOpen);
                    m_blockManager->ChangeCustomBrowserOpenState(uuid, isOpen);
                }
            }
        }
    }
}

void AFMainFrame::CustomBrowserStateAllChanged(bool isOpen)
{
    AFQCustomMenu* addon = _FindSubMenuByTitle(m_topMenu, QTStr("Basic.MainMenu.Addon"));
    if (addon)
    {
        AFQCustomMenu* customMenu = _FindSubMenuByTitle(m_topMenu, QTStr("Basic.MainMenu.Addon.CustomBrowserDocks"));
        if (customMenu)
        {
            QList<QAction*> actions = customMenu->actions();
            for (int i = 0; i < actions.count(); i++)
            {
                actions[i]->setChecked(isOpen);
            }

            m_blockManager->ChangeAllCustomBrowserOpenState(isOpen);
        }
    }
}

void AFMainFrame::SetSceneCollectionEnabled(bool enable)
{
    AFQCustomMenu* customMenu = _FindSubMenuByTitle(m_topMenu, QTStr("Basic.MainMenu.SceneCollectionFix"));
    if (customMenu)
        customMenu->setEnabled(enable);
}

void AFMainFrame::SetDisplayAffinity(QWindow* window)
{
    if (!SetDisplayAffinitySupported())
        return;

    bool hideFromCapture = config_get_bool(USERCONFIG, "BasicWindow", "HideOBSWindowsFromCapture");
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
    bool studioPortraitLayout = config_get_bool(USERCONFIG, "BasicWindow", "StudioPortraitLayout");

    if (changeLayout)
    {
        m_dynamicCompositMainWindow->SwitchStudioModeLayout(studioPortraitLayout);
    }

    bool studioModeLabels = config_get_bool(USERCONFIG, "BasicWindow", "StudioModeLabels");
    m_dynamicCompositMainWindow->ToggleStudioModeLabels(studioPortraitLayout, studioModeLabels);
}

void AFMainFrame::qslotShowMigrationGuide()
{
    AFQStudioUpdateLogDialog dialog(this);
    dialog.setMode(1);
    dialog.exec();
}

void AFMainFrame::qslotCheckBroadAvailable(bool BroadButtonOn)
{
    if (BroadButtonOn) {
        if (m_broadStartTimer && m_broadStartTimer->isActive()) {
            blog(LOG_ERROR, "change broad state - already broading");
            ui->pushButton_Broad->setChecked(false);
            return;
        }

        //Check if broadcast start routine is in progress (prevent duplicate call broad start button)
        if (m_checkBroadStartAPI)
        {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", QTStr("Broadcast.Starting"), false, true);
            return;
        }

        m_checkBroadStartAPI = true;
        m_CheckBroadStartAPITimer->start();

        emit StreamingPreparing();

        bool result = false;
        do {
            // PrePare Start Broad
            // API CALL : qslotBroadStartAPIResponse -> qslotBroadStartSuccess
            int ret = PrepareBroadStart();

            if (BS_SUCCESS != ret) {
                blog(LOG_ERROR, "prepare broad failed - err [%d]", ret);
                break;
            }

            qslotCheckVersionLimit();
            result = true;
        } while (false);
        if (false == result) {
            ui->pushButton_Broad->setChecked(false);
            OffBroadStartAPICheck();
        }
    }
    else
    {
        if (AFOutputUtil::IsStreamActive() == true)
        {
            BroadCastEnd();
        }
    }
}

void AFMainFrame::qslotSetButtonOpacity()
{
    QPushButton* broadButton = qobject_cast<QPushButton*>(sender());
    if (broadButton->graphicsEffect())
        broadButton->graphicsEffect()->setEnabled(true);
}

void AFMainFrame::qslotRemoveButtonOpacity()
{
    QPushButton* broadButton = qobject_cast<QPushButton*>(sender());
    if (broadButton->graphicsEffect())
        broadButton->graphicsEffect()->setEnabled(false);
}

bool AFMainFrame::qslotReplayBufferClicked()
{
    if(AFOutputUtil::IsReplayBufferActive()) {
        qslotStopReplayBuffer();
        qslotReplayBufferSave();
        return false;
    } else {
        qslotStartReplayBuffer();
        return true;
    }
}

void AFMainFrame::qslotMaximizedChanged(bool maximized)
{
    if (!maximized) {
        ui->pushButton_MaximumWindow->setObjectName("pushButton_MaximumWindow");
        m_blockManager->ChangeAllMagnet(true);
    }
    else {
        ui->pushButton_MaximumWindow->setObjectName("pushButton_MaxedWindow");
        m_blockManager->ChangeAllMagnet(false);
    }

    PolishStyleSheet(ui->pushButton_MaximumWindow);

    FinishAboutToMax();
}

//void AFMainFrame::qslotPauseRecordingClicked()
//{
//    if(AFOutputUtil::PauseOutput())
//        UnPauseRecording();
//    else
//        PauseRecording();
//}

void AFMainFrame::qslotStartCountDown()
{
    m_pBroadMovie->stop();
    ui->pushButton_Broad->setIcon(QIcon());
    m_broadStartTimer->stop();

    qslotStartStreaming();
}

void AFMainFrame::qslotChangeRecordState(bool checked)
{
    if(AFOutputUtil::IsRecordingActive())
    {
        bool confirm = config_get_bool(USERCONFIG, "BasicWindow", "WarnBeforeStoppingRecord");
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
        if(!UIValidation::NoSourcesConfirmation(this)) {
            ui->pushButton_Record->setChecked(false);
            return;
        }
        // start recording
        qslotStartRecording();
    }
}

void AFMainFrame::_ShowSettingPopup(int tabPage)
{
    if (AUTH_CONTEXT.IsSoopRegistered())
        AUTH_CONTEXT.LoadSoopStreamerInfo();

    m_leftNavigationBar->ResetChannelSlide();

    if(!m_studioSettingPopup)
    {
        m_studioSettingPopup = new AFQStudioSettingDialog(this);
        m_studioSettingPopup->AFQStudioSettingDialogInit(tabPage);
        m_blockManager->ApplyMoveInAllArea(m_studioSettingPopup);
    }

    BroadInfoTimerStop();

    m_studioSettingPopup->exec();

    if (!m_logOut)
    {
        _RestartApp();

        BroadInfoTimerStart();

        if (AUTH_CONTEXT.IsSoopRegistered())
            AUTH_CONTEXT.LoadSoopStreamerInfo();
        else
            m_blockManager->qslotClosePopup(ENUM_WINDOW_TYPE::BroadInfo);

        LoadAccounts();
        ApplyBroadInfoToUI();
    }
}

void AFMainFrame::_DeleteBroadInfoData()
{
    char path[512];
    GetAppConfigPath(path, sizeof(path), "SOOPStudio");
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s\\broad_info", path);
    os_unlink(file_path);
}

void AFMainFrame::_ShowSettingPopupWithID(QString id, QString platform)
{
    if (AUTH_CONTEXT.IsSoopRegistered())
        AUTH_CONTEXT.LoadSoopStreamerInfo();

    m_leftNavigationBar->ResetChannelSlide();

    if (!m_studioSettingPopup)
    {
        m_studioSettingPopup = new AFQStudioSettingDialog(this);
        m_studioSettingPopup->AFQStudioSettingDialogInit(1);
        m_studioSettingPopup->SetID(id, platform);
        m_blockManager->ApplyMoveInAllArea(m_studioSettingPopup);
    }


    BroadInfoTimerStop();

    m_studioSettingPopup->exec();

    if (!m_logOut)
    {
        _RestartApp();

        BroadInfoTimerStart();

        if (AUTH_CONTEXT.IsSoopRegistered())
            AUTH_CONTEXT.LoadSoopStreamerInfo();
        else
            m_blockManager->qslotClosePopup(ENUM_WINDOW_TYPE::BroadInfo);

        LoadAccounts();
        ApplyBroadInfoToUI();
    }
}

void AFMainFrame::BroadStatusCheckTimerStart()
{
    if (!m_BroadStatusCheckTimer)
    {
        m_BroadStatusCheckTimer = new QTimer(this);
        connect(m_BroadStatusCheckTimer, &QTimer::timeout,
            this, &AFMainFrame::qslotBroadCheckTimeout);
        m_BroadStatusCheckTimer->setInterval(1000 * 30);
        m_BroadStatusCheckTimer->start();

    }
}

void AFMainFrame::BroadStatusCheckTimerStop()
{
    if (m_BroadStatusCheckTimer)
    {
        m_BroadStatusCheckTimer->stop();
        disconnect(m_BroadStatusCheckTimer, &QTimer::timeout, this, &AFMainFrame::qslotBroadCheckTimeout);

        m_BroadStatusCheckTimer->deleteLater();
        m_BroadStatusCheckTimer = nullptr;
    }
}

QString AFMainFrame::GetChannelID(obs_output_t* output)
{
    QString channelID = "";
    if (!output)
        return channelID;

    AFChannelData* channelData = nullptr;
    
    // Main Output
    if (AFOutputUtil::IsMainStreamOutput(output))
    {
        AUTH_CONTEXT.GetMainChannelData(channelData);
        std::string strChannelID = channelData->pAuthData->channelID;
        if (!strChannelID.empty())
            channelID = QString::fromStdString(strChannelID);
        return channelID;
    }

    OUTPUT_HANDLER_LIST::iterator outputIter;
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    for (outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
        if (!outputIter->second.get())
            continue;
        if (outputIter->second.get()->streamOutput == output) 
        {
            AUTH_CONTEXT.GetChannelData(outputIter->first, channelData);
            if (channelData) 
            {
                std::string strChannelID = channelData->pAuthData->channelID;
                if (!strChannelID.empty())
                    channelID = QString::fromStdString(strChannelID);
                break;
            }
        }
    }

    return channelID;
}

int AFMainFrame::PrepareBroadStart()
{
    auto& auth = AUTH_CONTEXT;
    //
	// Broadcast Channel Inspection
	bool existStream = false;
    bool customChannelStreamming = false;
	int  cntOfAccount = auth.GetCntChannel();

	AFChannelData* mainChannel = nullptr;
	if (auth.GetMainChannelData(mainChannel))
		existStream = mainChannel->isStreaming;

	for (int idx = 0; idx < cntOfAccount; idx++) {
		AFChannelData* tmpChannel = nullptr;
        auth.GetChannelData(idx, tmpChannel);
		if (tmpChannel->isStreaming) {
			existStream = true;
            customChannelStreamming = true;
			break;
		}
	}

	if (!existStream) {
		int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
			"", Str("Basic.Settings.Stream.MissingStreamKey"));

		if (result == QDialog::Accepted) {
			_ShowSettingPopup(1);
		}
		return BS_NOT_EXIST_BROADCAST_CHANNEL;
	}

    if (AFOutputUtil::IsStreamActive())
        return BS_STREAM_ACTIVE;

    config_t* activeConfig = ACTIVECONFIG;
    //
    OBSData advEncorderData;
    int outCx = config_get_uint(activeConfig, "Video", "OutputCX");
    int outCy = config_get_uint(activeConfig, "Video", "OutputCY");
    int baseCx = config_get_uint(activeConfig, "Video", "BaseCX");
    int baseCy = config_get_uint(activeConfig, "Video", "BaseCY");
    int vBitrate;
    const char* simpleMode = config_get_string(activeConfig, "Output", "Mode");
    std::string encoder;
    if (0 == strcmp(simpleMode, "Simple")) {
        vBitrate = config_get_uint(activeConfig, "SimpleOutput", "VBitrate");
        encoder = config_get_string(activeConfig, "SimpleOutput", "StreamEncoder");
    }
    else {
        advEncorderData = AFProfileUtil::GetDataFromJsonFile("streamEncoder.json");
        vBitrate = (int)obs_data_get_int(advEncorderData, "bitrate");
        encoder = config_get_string(activeConfig, "AdvOut", "Encoder");
    }
    // av1 valid check
    bool isAV1Codec = AFEncoderUtil::isAV1Codec(encoder.c_str());
    if(isAV1Codec) {
        // resolution
        bool changed = false;
        if(av1Resolution.width != outCx || av1Resolution.height != outCy) {
            changed = true;
            outCx = baseCx = av1Resolution.width;
            outCy = baseCy = av1Resolution.height;
            //
            config_set_uint(activeConfig, "Video", "OutputCX", outCx);
            config_set_uint(activeConfig, "Video", "OutputCY", outCy);
            config_set_uint(activeConfig, "Video", "BaseCX", outCx);
            config_set_uint(activeConfig, "Video", "BaseCY", outCy);
        }
        // bitrate
        if(vBitrate <= 8000) {
            changed = true;
            vBitrate = 16000;
            if(0 == strcmp(simpleMode, "Simple")) {
                config_set_uint(activeConfig, "SimpleOutput", "VBitrate", vBitrate);
            } else {
                obs_data_set_int(advEncorderData, "bitrate", vBitrate);
                AFProfileUtil::SetDataToJsonFile("streamEncoder.json", advEncorderData);
            }
        }
        if(changed) {
            config_save_safe(activeConfig, "tmp", nullptr);
            AFVideoUtil::ResetVideo();
        }
    }

    bool validResolution = (outCx > 1920 || outCy > 1080) ? false : true;
    if (!validResolution && outCx == 720 && outCy == 1280)
        validResolution = true;

    bool validBitrate = vBitrate > 8000 ? false : true;

    if(!validResolution || !validBitrate)
    {
        AFQBroadInfo* broadInfo = auth.GetSoopBroadInfo();
        if(broadInfo) {
            if(broadInfo->Allow1440P()) {
                validResolution = (outCx > 2560 || outCy > 1440) ? false : true;
                validBitrate = vBitrate > 16000 ? false : true;
                if(!validResolution || !validBitrate) {
                    AFQMessagBoxAlert alert(this, QTStr("Basic.1440PResolution.Title"),
                                            QTStr("Basic.1440PResolution.Info2"), QTStr("Change"));
                    if(QDialog::Accepted == alert.exec())
                    {
                        if(!validResolution) {
                            outCx = 2560;
                            outCy = 1440;

                            baseCx = 2560;
                            baseCy = 1440;
                            config_set_uint(activeConfig, "Video", "OutputCX", outCx);
                            config_set_uint(activeConfig, "Video", "OutputCY", outCy);
                                            
                            config_set_uint(activeConfig, "Video", "BaseCX", baseCx);
                            config_set_uint(activeConfig, "Video", "BaseCY", baseCy);
                        }
                        if(!validBitrate) {
                            vBitrate = 16000;
                            if(0 == strcmp(simpleMode, "Simple")) {
                                config_set_uint(activeConfig, "SimpleOutput", "VBitrate", vBitrate);
                            } else {
                                obs_data_set_int(advEncorderData, "bitrate", vBitrate);
                                AFProfileUtil::SetDataToJsonFile("streamEncoder.json", advEncorderData);
                            }
                        }
                        config_save_safe(activeConfig, "tmp", nullptr);
                        AFVideoUtil::ResetVideo();
                    } else
                        return BS_INVALID_RESOLUTION;
                }
            } else {
                AFQMessagBoxAlert alert(this, QTStr("Basic.1440PResolution.Title"),
                                        QTStr("Basic.1440PResolution.Info2"), QTStr("Change"));
                if(QDialog::Accepted == alert.exec())
                {
                    if(!validResolution) {
                        outCx = 1920;
                        outCy = 1080;

                        baseCx = 1920;
                        baseCy = 1080;
                        config_set_uint(activeConfig, "Video", "OutputCX", outCx);
                        config_set_uint(activeConfig, "Video", "OutputCY", outCy);

                        config_set_uint(activeConfig, "Video", "BaseCX", baseCx);
                        config_set_uint(activeConfig, "Video", "BaseCY", baseCy);
                    }
                    if(!validBitrate) {
                        vBitrate = 8000;
                        if(0 == strcmp(simpleMode, "Simple")) {
                            config_set_uint(activeConfig, "SimpleOutput", "VBitrate", vBitrate);
                        } else {
                            obs_data_set_int(advEncorderData, "bitrate", vBitrate);
                            AFProfileUtil::SetDataToJsonFile("streamEncoder.json", advEncorderData);
                        }
                    }
                    config_save_safe(activeConfig, "tmp", nullptr);
                    AFVideoUtil::ResetVideo();
                } else
                    return BS_INVALID_RESOLUTION;
            }
        }
    }

    if (!m_pMainOutput->IsStreamingOnlySoop()) {
        if (outCx == 2560 || outCy == 1440 ) {
            AFQMessagBoxAlert alert(this,QTStr("Basic.1440PResolution.Title"), 
                                         QTStr("Basic.1440PResolution.Info"),QTStr("Change"));
            if (QDialog::Accepted == alert.exec())
            {
                config_set_uint(activeConfig, "Video", "OutputCX", 1920);
                config_set_uint(activeConfig, "Video", "OutputCY", 1080);

                config_set_uint(activeConfig, "Video", "BaseCX", 1920);
                config_set_uint(activeConfig, "Video", "BaseCY", 1080);

                if (vBitrate > 8000) {
                    vBitrate = 8000;
                    if (0 == strcmp(simpleMode, "Simple")) {
                        config_set_uint(activeConfig, "SimpleOutput", "VBitrate", vBitrate);
                    }
                    else {
                        obs_data_set_int(advEncorderData, "bitrate", vBitrate);
                        AFProfileUtil::SetDataToJsonFile("streamEncoder.json", advEncorderData);
                    }
                }
                config_save_safe(activeConfig, "tmp", nullptr);
                AFVideoUtil::ResetVideo();
            }
            else
                return BS_INVALID_RESOLUTION;
        }

        m_soopSrcManager->ForceStopSoopSource();
    }

    for (int idx = 0; idx < cntOfAccount; idx++) {
        AFChannelData* tmpChannel = nullptr;
        auth.GetChannelData(idx, tmpChannel);
        if (!tmpChannel || !tmpChannel->pAuthData)
            continue;

        if (tmpChannel->pAuthData->platform == PLATFORM_YOUTUBE) {
            if (tmpChannel->isStreaming == true) {
                AFBasicAuth* authData = tmpChannel->pAuthData;
                if (!m_broadcastReady) {
                    if (ShowPopupPageYoutubeChannel(authData) != QDialog::Accepted) {
                        if (authData->keyRTMP.empty() || authData->urlRTMP.empty())
                            tmpChannel->isStreaming = false;
                        break;
                    }
                }
            }
        }
    }

    if (auth.IsSoopStreaming())
    {
        if (m_bFirstOpen) {
            m_blockManager->ShowVodAutoUploadNotice();
            m_bFirstOpen = false;
        }

        QString strcheckedDate = config_get_string(APPCONFIG, "BroadInfo", SOOP_BROADCAST_NOTICE_DATE_CHECK);
        QDate checkDate   = QDate::fromString(strcheckedDate, "yyyy-MM-dd");
        QDate currentDate = QDateTime::currentDateTime().date();
        if (checkDate < currentDate) {
            if (!m_blockManager->ShowBroadcastNotice()) {
                return BS_BROADNOTICE_CANCEL;
            }
        }
    }
    else
    {
        bool confirm = config_get_bool(USERCONFIG, "BasicWindow", "WarnBeforeStartingStream");
        if (confirm)
        {
            int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                this,
                Str("ConfirmStart.Title"),
                Str("ConfirmStart.Text"));

            if (result != 1)
            {
                ui->pushButton_Broad->setChecked(false);
                return BROADSTART_ERROR::BS_USER_REJECT;
            }
        }
    }

    // Check SOOP Media Source
    OBSSource source = m_soopSrcManager->GetSoopMediaSource();
    if (source)
    {
        int  allowed_category = 0;
        bool is_adult_content = false;
        std::list<int> allowedCategorys;
        obs_media_state media_state = obs_source_media_get_state(source);
        if (media_state == OBS_MEDIA_STATE_PLAYING ||
            media_state == OBS_MEDIA_STATE_OPENING ||
            media_state == OBS_MEDIA_STATE_BUFFERING ||
            media_state == OBS_MEDIA_STATE_PAUSED)
        {
            const char* id = obs_source_get_id(source);

            AFQBroadInfo* broadInfo = auth.GetSoopBroadInfo();
            if (AFSourceUtil::IsSoopVodSource(source))
            {
                VodInfo_s curVodInfo = m_soopSrcManager->GetCurVodInfo(id);
                allowed_category = curVodInfo.allowed_category;
                is_adult_content = curVodInfo.is_adult;

                allowedCategorys.push_back(allowed_category);
            }
            else
            {
                if (0 == strcmp(id, "soop_directbroad_source")) {
                    allowedCategorys = m_soopSrcManager->GetDirectBroadArrowedCategorys();
                }
                else if (0 == strcmp(id, "soop_tv_cable_source")) {
                    int tvCableCategory = 390000 + m_soopSrcManager->GetTvLiveCPNo();
                    allowedCategorys.push_back(tvCableCategory);
                }
            }

            bool categoryMatch = false;
            for (auto it = allowedCategorys.begin(); it != allowedCategorys.end(); ++it) {
                if (broadInfo->CategoryNumber() == (*it)) {
                    categoryMatch = true;
                    break;
                }
            }

            bool adultOptionNotMatch = false;
            if (!broadInfo->AdultOnly()) {
                adultOptionNotMatch = (is_adult_content != broadInfo->AdultOnly());
            } 

            if (!categoryMatch || adultOptionNotMatch) {

                AFQCateChangeDialog dlg(this, id);
                if (is_adult_content) {
                    dlg.AddAllowedAnimeAdultCategoryInfo(allowed_category);
                }
                else {
                    dlg.AddAllowedCategoryInfo(allowedCategorys);
                }

                if (QDialog::Accepted != dlg.exec()) {
                    ui->pushButton_Broad->setChecked(false);
                    blog(LOG_ERROR, "prepare broadstart category check - not Accept Dialog");
                    return BS_SOOP_MEDIA_SOURCE_CATEGORY_CANCEL;
                }
                int selectCategory = dlg.GetSelectedCategory();
                broadInfo->SetAdultOnly(is_adult_content);
                broadInfo->SetCategory(selectCategory);

                RefreshBroadInfoDockUI(false);
            }
        }
    }
    return BS_SUCCESS;
}

bool AFMainFrame::BroadCastEnd(bool banStop)
{
    auto& auth = AUTH_CONTEXT;
    //
    int nBreaktimeCheck = 0;
    if (m_breakTimeManager->IsActive())
    {
        nBreaktimeCheck = AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                                   this, QT_UTF8(""),
                                                                   QTStr("breaktime.broad.end"), QTStr("breaktime.broad.end.btn"));
        if (nBreaktimeCheck != 1)
        {
            ui->pushButton_Broad->setChecked(true);
            return false;
        }
    }

    bool retVal = true;
    if (!auth.IsSoopStreaming())
    {
        if (!banStop)
        {
            bool confirm = config_get_bool(USERCONFIG, "BasicWindow", "WarnBeforeStoppingStream");
            if (confirm)
            {
                int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                    this,
                    Str("ConfirmStop.Title"),
                    Str("ConfirmStop.Text"));

                if (result != 1)
                {
                    ui->pushButton_Broad->setChecked(true);
                    return false;
                }
            }
        }

        ChangeStreamStateUI(true, false, "LIVE", 77);
        if (m_pBroadMovie != nullptr)
        {
            m_pBroadMovie->stop();
            ui->pushButton_Broad->setIcon(QIcon());
        }

        if (m_broadStartTimer != nullptr)
        {
            if (m_broadStartTimer->isActive())
            {
                m_broadStartTimer->stop();
            }
        }

        qslotStopStreaming();
        m_broadcastReady = false;
    }
    else
    {
        if (!banStop)
        {
            auth.RequestBroadInfoAPI();
            //
            AFQEndBroadDialog* endBroad = new AFQEndBroadDialog(this);
            connect(auth.GetSoopBroadInfo(), &AFQBroadInfo::qsignalBroadInfoReceived,
                    endBroad, &AFQEndBroadDialog::qslotRefreshWaitTime);

            bool splitAvailable = CheckSplitVodAvailable();

            endBroad->EndBroadInfoInit(CheckSplitVodAvailable());

            if (endBroad->exec() == QDialog::Accepted)
            {
                if (nBreaktimeCheck) {
                    QEventLoop loop;
                    QMetaObject::Connection c1, c2;

                    QTimer timer;
                    timer.setSingleShot(true);
                    timer.start(3000);

                    c1 = connect(&BREAKTIME_MANAGER, &BreaktimeManager::signalStopApisDone, &loop, [&loop](bool) {
                        loop.quit();
                    });

                    c2 = connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

                    m_breakTimeManager->Stop(true);

                    loop.exec();

                    disconnect(c1);
                    disconnect(c2);
                }

                ChangeStreamStateUI(true, false, "LIVE", 77);
                if (m_pBroadMovie != nullptr)
                {
                    m_pBroadMovie->stop();
                    ui->pushButton_Broad->setIcon(QIcon());
                }

                if (m_broadStartTimer != nullptr)
                {
                    if (m_broadStartTimer->isActive())
                    {
                        m_broadStartTimer->stop();
                    }
                }

                qslotStopStreaming();
                m_broadcastReady = false;
            }
            else
            {
                ui->pushButton_Broad->setChecked(true);
                retVal = false;
            }

            endBroad->close();
            endBroad->deleteLater();
        }
        else
        {
            ChangeStreamStateUI(true, false, "LIVE", 77);
            if (m_pBroadMovie != nullptr)
            {
                m_pBroadMovie->stop();
                ui->pushButton_Broad->setIcon(QIcon());
            }

            if (m_broadStartTimer != nullptr)
            {
                if (m_broadStartTimer->isActive())
                {
                    m_broadStartTimer->stop();
                }
            }

            qslotStopStreaming();
            m_broadcastReady = false;
        }
        
    }

    m_breakTimeManager->Stop(true);

    return retVal;
}

bool AFMainFrame::CheckSoopBroadStatus(const QByteArray& responseData)
{
    if (responseData == "[]")
        return true;
    
    BroadStartAPI_s info = {};

    std::string jsonString = responseData.toStdString();
    std::string err = "";
    QString broadingStop = QTStr("Output.ConnectFail.Disconnected");

    BroadStatusCheckTimerStop();
    BroadCastEnd(true);

    QString failMsg = "[BROADING FAIL]: " + broadingStop;
    blog(LOG_INFO, failMsg.toUtf8().constData());
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", broadingStop, false, true);
    return false;
}

bool AFMainFrame::_OutputPathValid()
{
    config_t* activeConfig = ACTIVECONFIG;
    //
    const char* mode = config_get_string(activeConfig, "Output", "Mode");
    if(strcmp(mode, "Advanced") == 0) {
        const char* advanced_mode = config_get_string(activeConfig, "AdvOut", "RecType");
        if(strcmp(advanced_mode, "FFmpeg") == 0) {
            bool is_local = config_get_bool(activeConfig, "AdvOut", "FFOutputToFile");
            if(!is_local)
                return true;
        }
    }

    const char* path = AFOutputUtil::GetCurrentOutputPath();
    return path && *path && QDir(path).exists();
}
void AFMainFrame::_OutputPathInvalidMessage()
{
    blog(LOG_ERROR, "Recording stopped because of bad output path");

    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                               QTStr("Output.BadPath.Title"),
                               QTStr("Output.BadPath.Text"));
}

void AFMainFrame::_SetResourceCheckTimer(int time)
{
    m_resourceRefreshTimer = new QTimer(this);

    connect(m_resourceRefreshTimer, &QTimer::timeout, 
        this, &AFMainFrame::qsignalRefreshTimerTick);

    qslotRefreshMainResourceText();

    m_resourceRefreshTimer->start(time);
}

bool AFMainFrame::_LowDiskSpace()
{
    const char* path;

    path = AFOutputUtil::GetCurrentOutputPath();
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


template<typename SlotFunc>
void AFMainFrame::_connectAndAddAction(QAction* sender,
                                       const typename QtPrivate::FunctionPointer<SlotFunc>::Object* receiver,
                                       SlotFunc slot, bool addActionToMain)
{
    QObject::connect(sender, &QAction::triggered, receiver, slot);
    if(addActionToMain)
        addAction(sender);
}
void AFMainFrame::_RegisterSourceControlAction()
{
    QWidget* outBlock = nullptr;
    if (!m_blockManager->FindBlock(ENUM_WINDOW_TYPE::SceneSource, outBlock))
        return;

    AFSceneSourceWidget* scenesourceBlock = reinterpret_cast<AFSceneSourceWidget*>(outBlock);
    if (!scenesourceBlock)
        return;

    _connectAndAddAction(ui->action_LockPreview, this, &AFMainFrame::qslotLockPreview);

    _connectAndAddAction(ui->action_CopySource, m_pMainSceneSource, &CMainSceneSource::qslotActionCopySource);
    _connectAndAddAction(ui->action_PasteSource, m_pMainSceneSource, &CMainSceneSource::qslotActionPasteSource);

    //_connectAndAddAction(ui->action_PasteSourceRef, m_pMainSceneSource, &CMainSceneSource::qslotActionPasteRefSource);
    //_connectAndAddAction(ui->action_PasteSourceDuplicate, m_pMainSceneSource, &CMainSceneSource::qslotActionPasteDupSource);

    _connectAndAddAction(ui->action_Filters, m_pMainSceneSource, &CMainSceneSource::qslotOpenSourceFilters);
    _connectAndAddAction(ui->action_CopyFilters, m_pMainSceneSource, &CMainSceneSource::qslotCopySourceFilters);
    _connectAndAddAction(ui->action_PasteFilters, m_pMainSceneSource, &CMainSceneSource::qslotPasteSourceFilters);
    _connectAndAddAction(ui->action_SplitEffect, m_pMainSceneSource, &CMainSceneSource::qslotSetSplitEffectFilter);

    _connectAndAddAction(ui->action_EditTransform, m_pMainSceneSource, &CMainSceneSource::qslotActionEditTransform);
    _connectAndAddAction(ui->action_CopyTransform, m_pMainSceneSource, &CMainSceneSource::qslotActionCopyTransform);
    _connectAndAddAction(ui->action_PasteTransform, m_pMainSceneSource, &CMainSceneSource::qslotActionPasteTransform);
    _connectAndAddAction(ui->action_ResetTransform, m_pMainSceneSource, &CMainSceneSource::qslotActionResetTransform);

    _connectAndAddAction(ui->action_Rotate90CW, m_pMainSceneSource, &CMainSceneSource::qslotActionRotate90CW);
    _connectAndAddAction(ui->action_Rotate90CCW, m_pMainSceneSource, &CMainSceneSource::qslotActionRotate90CCW);
    _connectAndAddAction(ui->action_Rotate180, m_pMainSceneSource, &CMainSceneSource::qslotActionRotate180);

    _connectAndAddAction(ui->action_FlipHorizontal, m_pMainSceneSource, &CMainSceneSource::qslotFlipHorizontal);
    _connectAndAddAction(ui->action_FlipVertical, m_pMainSceneSource, &CMainSceneSource::qslotFlipVertical);

    _connectAndAddAction(ui->action_FitToScreen, m_pMainSceneSource, &CMainSceneSource::qslotFitToScreen);
    _connectAndAddAction(ui->action_StretchToScreen, m_pMainSceneSource, &CMainSceneSource::qslotStretchToScreen);
    _connectAndAddAction(ui->action_CenterToScreen, m_pMainSceneSource, &CMainSceneSource::qslotCenterToScreen);
    _connectAndAddAction(ui->action_VerticalCenter, m_pMainSceneSource, &CMainSceneSource::qslotVerticalCenter);
    _connectAndAddAction(ui->action_HorizontalCenter, m_pMainSceneSource, &CMainSceneSource::qslotHorizontalCenter);

    _connectAndAddAction(ui->action_OrderMoveUp, scenesourceBlock, &AFSceneSourceWidget::qslotMoveUpSourceTrigger);
    _connectAndAddAction(ui->action_OrderMoveDown, scenesourceBlock, &AFSceneSourceWidget::qslotMoveDownSourceTrigger);
    _connectAndAddAction(ui->action_OrderMoveToTop, scenesourceBlock, &AFSceneSourceWidget::qslotMoveToTopSourceTrigger);
    _connectAndAddAction(ui->action_OrderMoveToBottom, scenesourceBlock, &AFSceneSourceWidget::qslotMoveToBottomSourceTrigger);

    _connectAndAddAction(ui->action_ShowInteract, m_pMainSceneSource, &CMainSceneSource::qslotActionShowInteractionPopup);
    _connectAndAddAction(ui->action_ShowProperties, m_pMainSceneSource, &CMainSceneSource::qslotActionShowProperties);

    _connectAndAddAction(ui->action_RemoveScene, m_pMainSceneSource, &CMainSceneSource::qslotActionRemoveScene, false);
    _connectAndAddAction(ui->action_RenameScene, m_pMainSceneSource, &CMainSceneSource::qslotActionRenameScene, false);

    _connectAndAddAction(ui->action_RenameSource, m_pMainSceneSource, &CMainSceneSource::qslotActionRenameSource, false);
    _connectAndAddAction(ui->action_RemoveSource, m_pMainSceneSource, &CMainSceneSource::qslotActionRemoveSource, false);

    // preview add action
    CBasicPreview* preview = GetMainWindow()->GetMainPreview();
    if(preview) {
        preview->addAction(ui->action_RemoveSource);
        preview->addAction(ui->action_CopySource);
        preview->addAction(ui->action_PasteSource);
        //preview->addAction(ui->action_PasteSourceRef);
    }

    QWidget* sceneListFrame = scenesourceBlock->GetSceneListFrame();
    if(sceneListFrame) {
        sceneListFrame->addAction(ui->action_RemoveScene);
        sceneListFrame->addAction(ui->action_RenameScene);
    }

    QWidget* sourceListView = scenesourceBlock->GetSourceListView();
    if(sourceListView) {
        sourceListView->addAction(ui->action_RemoveSource);
        sourceListView->addAction(ui->action_CopySource);
        sourceListView->addAction(ui->action_PasteSource);
        //sourceListView->addAction(ui->action_PasteSourceRef);
        sourceListView->addAction(ui->action_RenameSource);
    }
}

void AFMainFrame::qslotTopMenuClicked()
{
    emit qsignalTopMenuClicked();
    _ToggleTopMenu();
}

void AFMainFrame::qslotPopupBlockClicked()
{
    m_blockManager->RaiseAllPopup();
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
    m_blockManager->ApplyMoveInAllArea(m_programInfoDialog);
    m_programInfoDialog->show();
}

void AFMainFrame::qslotMainFrameTutorial()
{
    disconnect(this, &AFMainFrame::qsignalMainShowEventTriggered, this, &AFMainFrame::qslotMainFrameTutorial);


        if (m_freecshotType != "2" && m_freecshotType != "3")
        {
            int nFreecShotSetting = qslotShowImportGuide();
            if (nFreecShotSetting == 1)
            {
                config_set_bool(APPCONFIG, "General", "MigrationUser", true);
                config_save_safe(APPCONFIG, "tmp", nullptr);                
            }
        }
        else if (m_freecshotType == "3")
        {
            AFQImportGuide importGuide(nullptr);
            importGuide.LoadFromStudio2Json();
        }

        m_blockManager->InitDefaultDock();

        m_mainSceneCollection->RefreshSceneCollections(true);
        MAIN_PROFILE->RefreshProfiles();        

    raise();

    if (!m_mainGuideWidget)
    {
        m_mainGuideWidget = new QWidget(this);

#if defined(_WIN32)
        m_mainGuideWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
#elif defined(__APPLE__)
        m_mainGuideWidget->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
#endif
        m_mainGuideWidget->setAttribute(Qt::WA_TranslucentBackground);
        m_mainGuideWidget->setAttribute(Qt::WA_DeleteOnClose);

        m_mainTutorialContents = new AFMainFrameGuide(m_mainGuideWidget);

        QHBoxLayout* layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_mainTutorialContents);
        m_mainTutorialContents->setStyleSheet("QWidget{background-color:rgba(0,0,0,30%);}");

        m_mainGuideWidget->setLayout(layout);
        m_mainGuideWidget->setGeometry(x(), y(), width(), height());

        connect(this, &AFMainFrame::qsignalmovedOrResized, this, &AFMainFrame::qslotMainFrameTutorialClose);
        connect(m_mainTutorialContents, &AFMainFrameGuide::qsignalCloseGuide, this, &AFMainFrame::qslotMainFrameTutorialClose);

        m_mainTutorialContents->TutorialInit(m_mainGuideWidget->geometry());

        qslotTutorialPosition();
        m_mainGuideWidget->show();
    }
}

void AFMainFrame::qslotTutorialPosition()
{
    if (m_mainGuideWidget)
    {
        m_mainGuideWidget->setGeometry(geometry());

        QSize gnb, channel, broad, button;

        const char* localeChar = LOCALE_CONTEXT.GetCurrentLocale();
        QString locale = QString(localeChar);

        if (this->height() < 601)
        {
            if (locale == "ko-KR")
            {
                gnb = QSize(202, 46);
                channel = QSize(250, 97);
                broad = QSize(157, 156);
                button = QSize(258, 132);
            }
            else if (locale == "en-US")
            {
                gnb = QSize(219, 46);
                channel = QSize(261, 97);
                broad = QSize(217, 156);
                button = QSize(291, 132);
            }
            else if (locale == "th-TH")
            {
                gnb = QSize(199, 46);
                channel = QSize(256, 97);
                broad = QSize(230, 156);
                button = QSize(260, 132);
            }
            else if (locale == "zh-TW")
            {
                gnb = QSize(207, 46);
                channel = QSize(255, 97);
                broad = QSize(179, 156);
                button = QSize(263, 132);
            }
            else if (locale == "zh-CN")
            {
                gnb = QSize(207, 46);
                channel = QSize(255, 97);
                broad = QSize(179, 156);
                button = QSize(263, 132);
            }
        }
        else
        {
            if (locale == "ko-KR")
            {
                gnb = QSize(461, 85);
                channel = QSize(444, 203);
                broad = QSize(271, 267);
                button = QSize(523, 192);
            }
            else if (locale == "en-US")
            {
                gnb = QSize(625, 85);
                channel = QSize(688, 182);
                broad = QSize(383, 267);
                button = QSize(514, 192);
            }
            else if (locale == "th-TH")
            {
                gnb = QSize(538, 85);
                channel = QSize(593, 182);
                broad = QSize(293, 267);
                button = QSize(478, 192);
            }
            else if (locale == "zh-TW")
            {
                gnb = QSize(480, 85);
                channel = QSize(545, 182);
                broad = QSize(213, 267);
                button = QSize(454, 192);
            }
            else if (locale == "zh-CN")
            {
                gnb = QSize(480, 85);
                channel = QSize(545, 182);
                broad = QSize(213, 267);
                button = QSize(445, 192);
            }
        }

        if (m_mainTutorialContents)
        {    
            //GNB Position
            QRect gnbRect = QRect(ui->pushButton_TopMenu->geometry().x() + 6, 
                ui->pushButton_TopMenu->geometry().y(), gnb.width(), gnb.height());

            //Channel Position
            QRect channelRect = QRect(0, ui->widget_Top->height(), channel.width(), channel.height());

            //Broad Position
            QRect broadRect = QRect(width() - broad.width() - 16,
                height() - broad.height() - ui->widget_BottomControl->height() - 10,
                broad.width(), broad.height());

            //Button Position
            QRect buttonRect = QRect(11, height() - button.height() - 9, button.width(), button.height());

            m_mainTutorialContents->TutorialPosition(gnbRect, channelRect, broadRect, buttonRect);
        }
    }
}

void AFMainFrame::qslotMainFrameTutorialClose()
{
    disconnect(this, &AFMainFrame::qsignalMainResized, this, &AFMainFrame::qslotTutorialPosition);

    if (m_mainTutorialContents)
    {
        disconnect(m_mainTutorialContents, &AFMainFrameGuide::qsignalCloseGuide, this, &AFMainFrame::qslotMainFrameTutorialClose);
        m_mainTutorialContents->close();

        bool isFirstOpen = this->property("IsFirstRun").toBool();
        if (isFirstOpen) {
            qslotInitShowSoopChat();
            qslotInitFreecShotPlusUpdateLog();
            this->setProperty("IsFirstRun", false);

            emit firstTutorialClosedEvent();
        }

        delete m_mainTutorialContents;
        m_mainTutorialContents = nullptr;
    }

    if (m_mainGuideWidget)
    {
        m_mainGuideWidget->close();
        delete m_mainGuideWidget;
        m_mainGuideWidget = nullptr;
    }
}

void AFMainFrame::qslotGuideClosed()
{
    if (m_mainGuideWidget)
    {
        m_mainGuideWidget->close();
        m_mainGuideWidget->deleteLater();
        m_mainGuideWidget = nullptr;
    }
}

void AFMainFrame::qslotNavigateSoopServiceNoticePage()
{
    NavigateDefaultBrowser(QString::fromStdString(SOOP_SERVICE_NOTICE_URL));
}

void AFMainFrame::qslotNavigateSoopServiceFeedBackPage()
{
    NavigateDefaultBrowser(QString::fromStdString(SOOP_FREECSHOT_COMMNET_URL));
}

void AFMainFrame::qslotNavigateStreamerSuppportPage()
{
    NavigateDefaultBrowser(QString::fromStdString(SOOP_STREAMER_SUPPORT_URL));
}

void AFMainFrame::qslotNavigateSoopliveKrPage()
{
    NavigateDefaultBrowser(QString::fromStdString(SOOPLIVE_KR_URL));
}

void AFMainFrame::qslotShowStudioUpdatePage()
{
    bool closed = true;
    if (m_updateLogPopup)
        closed = m_updateLogPopup->close();

    if (!closed)
        return;

    m_updateLogPopup = new AFQStudioUpdateLogDialog(this);
    m_updateLogPopup->setMode(0);
    m_updateLogPopup->setAttribute(Qt::WA_DeleteOnClose);
    m_updateLogPopup->show();
}

bool AFMainFrame::IsInEventPeriod(const EventTime& start, const EventTime& end) const
{
    auto toTimeT = [](const EventTime& t) -> std::time_t {
        std::tm tmTime = {};
        tmTime.tm_year = t.year - 1900;
        tmTime.tm_mon = t.month - 1;
        tmTime.tm_mday = t.day;
        tmTime.tm_hour = t.hour;
        tmTime.tm_min = t.minute;
        tmTime.tm_sec = t.second;
        tmTime.tm_isdst = -1;
        return std::mktime(&tmTime);
    };

    std::time_t now = static_cast<std::time_t>(AUTH_CONTEXT.GetServerTime());
    if (now <= 0) 
        now = std::time(nullptr);

    const std::time_t startT = toTimeT(start);
    const std::time_t endT = toTimeT(end);

    if (startT == static_cast<std::time_t>(-1) ||
        endT == static_cast<std::time_t>(-1) ||
        startT > endT) {
        return false;
    }

    return (now >= startT && now <= endT);
}

void AFMainFrame::qslotEventButtonClicked() 
{
    NavigateDefaultBrowser(QString::fromStdString(LINK_EVENT));
}


void AFMainFrame::qslotPropertiesToggled(bool show)
{
    config_set_bool(USERCONFIG, "BasicWindow", "ShowContextToolbars", ui->action_ViewToggleProperties->isChecked());
    m_dynamicCompositMainWindow->SetVisibleSourceToolBar(show);
}

void AFMainFrame::qslotAlwaysOnTopToggled(bool onTop)
{
    config_set_bool(USERCONFIG, "General", "AlwaysOnTop", ui->action_ViewToggleOnTop->isChecked());
    SetAlwaysOnTop(this, onTop);
}

void AFMainFrame::qslotUIResetTriggered()
{
    AFQMessagBoxAlert dlg(this, QTStr("Basic.ResetUI.Title"), QTStr("Basic.ResetUI.Info"), QTStr("Reset"));
    if (QDialog::Accepted == dlg.exec()) {
        ResetDockUI();
    }
}

void AFMainFrame::qslotSceneControlTriggered(bool show)
{
    QAction* sceneControlAction = reinterpret_cast<QAction*>(sender());

    sceneControlAction->setChecked(true);
    m_blockManager->qslotShowSceneControlDockTriggered();
}

void AFMainFrame::qslotBlockClosedTriggered(bool used, int checkType, bool enableFavoriteMenu)
{
    UNUSED_PARAMETER(enableFavoriteMenu);

    if(checkType == ENUM_WINDOW_TYPE::SceneControl)
        ui->action_ViewSceneControl->setChecked(used);
}

void AFMainFrame::qslotCloseAllBlocks()
{
    m_blockManager->CloseAllBlocks();
}

void AFMainFrame::qslotShowStudioSettingPopup(bool show)
{
    _ShowSettingPopup();
}

void AFMainFrame::qslotShowStudioSettingWithButtonSender()
{
    QObject* pushButtonSender = reinterpret_cast<QObject*>(sender());

    bool ok;
    int settingPage = pushButtonSender->property("type").toInt(&ok);
    if (!ok) {
        settingPage = 0;
    }

    if (settingPage == 1)
    {
        QString id = pushButtonSender->property("channelID").toString();
        QString platformName = pushButtonSender->property("platformName").toString();
        _ShowSettingPopupWithID(id, platformName);
    }
    else
    {
        _ShowSettingPopup(settingPage);
    }

}

void AFMainFrame::qslotShowBlockWithProperty()
{
    bool ok = false;
    int type = sender()->property("windowtype").toInt(&ok);
    if (ok)
    {
        AFQBorderPopupBaseWidget* popup = nullptr;
        m_blockManager->MakePopup(type, popup);
    }
}

void AFMainFrame::qslotShowBlock(bool visible, int type)
{
    Q_UNUSED(visible);

    AFQBorderPopupBaseWidget* popup = nullptr;
    m_blockManager->MakePopup(type, popup);
}

void AFMainFrame::qslotInitShowSoopChat()
{
    disconnect(this, &AFMainFrame::qsignalMainShowEventTriggered, this, &AFMainFrame::qslotInitShowSoopChat);

    RestoreMainWindow();

    bool sideDockOn = config_get_bool(USERCONFIG, "BasicWindow", "SideDocks");

    DYNAMIC_COMPOSIT->SetSideDock(sideDockOn);

    bool skipReset = false;

    m_blockManager->DoubleCheckPosition(ENUM_WINDOW_TYPE::TwitchChat);
    m_blockManager->ResetMagnet(ENUM_WINDOW_TYPE::TwitchChat);
    m_blockManager->DoubleCheckPosition(ENUM_WINDOW_TYPE::YoutubeChat);
    m_blockManager->ResetMagnet(ENUM_WINDOW_TYPE::YoutubeChat);

    AFQBorderPopupBaseWidget* popup = nullptr;
    m_blockManager->MakePopup(ENUM_WINDOW_TYPE::SoopChat, popup);
    if (popup)
    {

        if (!this->property("IsFirstRun").toBool())
        {
            QMetaEnum BlockTypeEnum = QMetaEnum::fromType<ENUM_WINDOW_TYPE>();
            const char* key = BlockTypeEnum.valueToKey(ENUM_WINDOW_TYPE::SoopChat);

            const char* PopupPosition = config_get_string(USERCONFIG, "BasicWindow", key);
            if ((PopupPosition != nullptr) && (PopupPosition[0] != '\0'))
            {
                m_blockManager->DoubleCheckPosition(ENUM_WINDOW_TYPE::SoopChat);
                skipReset = true;
            }
            if(!skipReset)
                popup->setGeometry(frameGeometry().x() + frameGeometry().width(), frameGeometry().y(), 420, this->height());
        }

        m_blockManager->ResetMagnet(ENUM_WINDOW_TYPE::SoopChat);

        QTimer::singleShot(10, this, [popup] {
            popup->raise();
            popup->activateWindow(); });
    }

}

void AFMainFrame::qslotInitFreecShotPlusUpdateLog()
{
	disconnect(this, &AFMainFrame::qsignalMainShowEventTriggered, this, &AFMainFrame::qslotInitFreecShotPlusUpdateLog);

    m_soopApiHandler->getAPIfromId(SOOP_API_KEY::GET_FREECSHOTPLUS_UPDATELOG_VERSION, {},
        this, "qslotResponseFreecShotPlusUpdateLog");
}

void AFMainFrame::qslotResponseFreecShotPlusUpdateLog(const QByteArray& responseData)
{
}

void AFMainFrame::qslotShowFreecShotUnInstallAlert()
{
    disconnect(this, &AFMainFrame::qsignalMainShowEventTriggered, this, &AFMainFrame::qslotShowFreecShotUnInstallAlert);

    config_t* appConfig = APPCONFIG;
    //
    bool isAlreadyRun = config_get_bool(appConfig, "General", "AlreadyRun");
    if (!isAlreadyRun) {
        config_set_bool(appConfig, "General", "AlreadyRun", true);
        config_save_safe(appConfig, "tmp", nullptr);
        return;
    }

	if (!RegKeyExists(CurrentUserRegKey, "SOFTWARE\\soop\\Studio2"))
		return;

	QDate today = QDate::currentDate();
	const char* lastStr = config_get_string(appConfig, "General", "FsUninstallPopupDate");
   
    bool needWeeklyPopup = false;
    if (nullptr == lastStr) {
        needWeeklyPopup = true;
    }
    else {
        QDate last = QDate::fromString(QString::fromStdString(lastStr), "yyyyMMdd");
        if ((last.daysTo(today) >= 7)) {
            needWeeklyPopup = true;
        }
    }

	if (!needWeeklyPopup) {
		config_save_safe(appConfig, "tmp", nullptr);
		return;
	}

	ShowUninstallFreecShotAlert();

	const QString todayStr = today.toString("yyyyMMdd");
	config_set_string(appConfig, "General", "FsUninstallPopupDate", todayStr.toStdString().c_str());
	config_save_safe(appConfig, "tmp", nullptr);
}

void AFMainFrame::qslotShowDockWithProperty()
{
    bool ok = false;
    int type = sender()->property("windowtype").toInt(&ok);
    if (ok)
    {
        ENUM_WINDOW_TYPE enumType = static_cast<ENUM_WINDOW_TYPE>(type);
        AFQBaseDockWidget* dock = nullptr;
        m_blockManager->MakeDock(enumType, dock, true);
    }
}

void AFMainFrame::qslotShowDock(bool visible, int type)
{
    ENUM_WINDOW_TYPE enumType = static_cast<ENUM_WINDOW_TYPE>(type);
    AFQBaseDockWidget* dock = nullptr;
    m_blockManager->MakeDock(enumType, dock, visible);
}

void AFMainFrame::qslotShowProjector()
{
    QObject* senderMonitor = reinterpret_cast<QObject*>(sender());
    int monitorNum = senderMonitor->property("monitor").toInt();
    m_blockManager->MakeProjector(monitorNum);
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

void AFMainFrame::qslotCategoryCheckAPIResponse(const QByteArray& responseData)
{
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    if(broadInfo) {
        broadInfo->RefreshCategoryList(responseData);
    }

    m_soopApiHandler->getAPIfromId(GET_BROAD_GEO_BLOCK, {}, this, "qslotGeoBlockCheckAPIResponse");
}

void AFMainFrame::qslotGeoBlockCheckAPIResponse(const QByteArray& responseData)
{
    BOOL bGeoBlock = false;

    std::string jsonString = responseData.toStdString();
    std::string err;

    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    if(!broadInfo) {
        return;
    }

    if(bGeoBlock) {

        ui->pushButton_Broad->setChecked(false);
        OffBroadStartAPICheck();

        QString msg = QTStr("Basic.GeoBlock.CategoryBlocked");
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", msg, false, true, "", 0, 0, "type1");

        return;
    } else
    {
        QString GetNation = broadInfo->GetUserNation();

        bool isKorea =
            (GetNation == "kr") ||
            (GetNation == "ko_kr") ||
            (GetNation == "ko");

        if(!isKorea)
        {
            static const char* kSoopKboSectionSources[] = {
                "soop_kbo_graphic_source_all",
                "soop_kbo_graphic_source_score",
                "soop_kbo_graphic_source_stadium",
                "soop_kbo_graphic_source_player",
                "soop_kbo_graphic_source_livetext",
                "soop_football_graphic_source_all",
                "soop_football_graphic_source_player",                
                "soop_football_graphic_source_change",
                "soop_football_graphic_source_score",
                "soop_football_graphic_source_livetext",
            };

            bool bKboSource = false;

            obs_source_t* currentSceneSource = obs_frontend_get_current_scene();
            if(currentSceneSource) {
                obs_scene_t* scene = obs_scene_from_source(currentSceneSource);
                if(scene) {

                    bool foundKbo = false;

                    auto enumFunc = [](obs_scene_t*, obs_sceneitem_t* item, void* param) -> bool {
                        bool* found = reinterpret_cast<bool*>(param);
                        if(*found)
                            return false;

                        obs_source_t* src = obs_sceneitem_get_source(item);
                        if(!src)
                            return true;

                        const char* id = obs_source_get_unversioned_id(src);
                        const char* name = obs_source_get_name(src);

                        for(const char* kboId : kSoopKboSectionSources) {
                            if((id && strcmp(id, kboId) == 0) ||
                                (name && strcmp(name, kboId) == 0)) {
                                *found = true;
                                return false; 
                            }
                        }

                        return true;
                    };

                    obs_scene_enum_items(scene, enumFunc, &foundKbo);
                    bKboSource = foundKbo;
                }

                obs_source_release(currentSceneSource);
            }

            if(bKboSource)
            {
                QString msg = QTStr("Basic.GeoBlock.Blocked");
                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", msg, false, true, "", 0, 0, "type1");
                return;
            }
        }
        if(broadInfo) {
            std::string categoryString;
            bool findCategory = broadInfo->ReceiveCategoryString(broadInfo->CategoryNumber(), categoryString);
            if(!findCategory) {
                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, QT_UTF8(""), QTStr("Basic.CheckAllowedCategoryMsg"));
                AFChannelData* channelData = nullptr;
                AUTH_CONTEXT.GetMainChannelData(channelData);
                if(channelData && channelData->pAuthData->platform == PLATFORM_SOOP) {
                    ui->pushButton_Broad->setChecked(false);
                }
                return;
            }
        }

        AUTH_CONTEXT.SendCheckBroadStart(this, "qslotBroadStartAPIResponse");
    }
}

void AFMainFrame::qslotBroadStartAPIResponse(const QByteArray& responseData)
{
    bool result = false;

    BroadStartAPI_s info = {};

    std::string jsonString = responseData.toStdString();
    std::string err = "";

    if(jsonString.empty())
        LogoutMainAccount(true);
}

void AFMainFrame::qslotBroadStartSuccess(bool success, BroadStartAPI_s info_)
{
    auto& authManager = AUTH_CONTEXT;
    //
    int ret = BS_NONE;
    BroadStartAPI_s info = info_;
    do {
        if (!success) {
            switch (info.code)
            {
            case -9999://COMMON
                ret = BS_UNDER_MAINTENANCE;
                break;
            case 401:
                ret = BS_NEED_LOGIN;
                break;
            case -1504:
                ret = BS_ACCOUNT_RESIGNED;
                break;
            case -1200:
            case -1203:
            case -1204:
            case -1205:
            case -1206:
            case -1207:
                ret = BS_ADMINBLACK_CASE;
                break;
            case -1505:
                ret = BS_ACCOUNT_NO_INFO;
                break;
            case -1506:
                ret = BS_EMAIL_CERTIFY;
                break;
            case -1501:
                ret = BS_NEED_REALNAME_AUTH;
                break;
            case -1342:
                ret = BS_NEED_MINOR_CERTIFY;
                break;
            case -1340:
                ret = BS_LIMIT_DUPLICATE_BROADING;
                break;
            case -1341:
                ret = BS_LIMIT_DUPLICATE_BROAD;
                break;
            case -1300:
                ret = BS_ALREADY_BROADCASTING;
                break;
            case -1347:
                ret = BS_SUBSCRIBE_NOTSAME;
                break;
            default:
                ret = BS_BROADSTART_API_JSON_INVALID;
                break;
            }
            break;
        }
        
        ret = BS_SUCCESS;

        if (info.streamNo.empty()) {
            ret = BS_INVALID_STREAMKEY;
        }

    } while (false);
    if (BS_SUCCESS == ret)
    {
        AFChannelData* pSoopChannel = nullptr;
        authManager.GetChannelData(PLATFORM_SOOP, pSoopChannel);
        if (pSoopChannel)
        {
            pSoopChannel->pAuthData->keyRTMP = pSoopChannel->pAuthData->channelID + "-" + info.streamNo;
        }

        if (!m_broadStartTimer)
        {
            m_broadStartTimer = new QTimer(this);
            connect(m_broadStartTimer, &QTimer::timeout, this, &AFMainFrame::qslotStartCountDown);
        }

        if (m_pBroadMovie == nullptr)
        {
            std::string absPath;
            GetDataFilePath("assets", absPath);
            QString gifPath = QString("%1/mainview/broad-spinner-black.gif").
                arg(absPath.data());
            m_pBroadMovie = new QMovie(gifPath, QByteArray(), this);
        }
        QSize s = ui->pushButton_Broad->rect().size();
        connect(m_pBroadMovie, &QMovie::frameChanged, [=] {
            ui->pushButton_Broad->setIcon(m_pBroadMovie->currentPixmap());
            ui->pushButton_Broad->setIconSize(s);
            });
        
        m_pBroadMovie->start();
        ui->pushButton_Broad->setText("");

        bool sendSubscribe = false;
        QString outMessage = "";

        authManager.SendSoopBroadInfoSetting(sendSubscribe);
        
        m_broadStartTimer->start(1000);
    }
    else {
        ui->pushButton_Broad->setChecked(false);

        BroadStatusCheckTimerStop();

        blog(LOG_ERROR, "soop broadstart api err : [%d]", ret);

        bool    showMessageBox = true;
        QString broadStartFailMsg;

        int messageBoxFixedWidth = 0;
        int messageBoxFixedHeight = 0;
        QString failTopMsg = "";

        if (BS_UNDER_MAINTENANCE == ret) {
            broadStartFailMsg = QString::fromStdString(info.broadMsg);
        }
        else if (BS_ADMINBLACK_CASE == ret) {
            blog(LOG_ERROR, "soop admin black case : [%d]", info.code);
            if ((info.code > -1206) && (info.code < -1200)) {
                broadStartFailMsg = QTStr("Popup.AdminRestrict");
                if (info.code == -1201)
                    broadStartFailMsg += "10";
                else if (info.code == -1202)
                    broadStartFailMsg += "30";
                else if (info.code == -1203)
                    broadStartFailMsg += "60";
                else if (info.code == -1204)
                    broadStartFailMsg += "12";
                else if (info.code == -1205)
                    broadStartFailMsg += "24";

                if ((info.code >= -1203) && (info.code <= -1201))
                    broadStartFailMsg += QTStr("Minutes");
                else
                    broadStartFailMsg += QTStr("Hours");

                broadStartFailMsg += QTStr("Popup.AdminRestrict.Time");
            }
            else {
                std::string tcURL = ADMIN_BLACK_POPUP_URL;
                AFChannelData* pSoopChannelData = nullptr;
                authManager.GetChannelData(PLATFORM_SOOP, pSoopChannelData);
                std::string tcKey = pSoopChannelData->pAuthData->channelID;
                tcKey += "|";
                tcKey += std::to_string(time(nullptr));
                char szOut[2048] = { 0, };
                if (soop_crypt_encrypt((char*)tcKey.c_str(), szOut, 2048) <= 0)
                    tcURL += tcKey;
                else
                    tcURL += szOut;

                QCefWidget* cefWidget = CEFMANAGER.createWidget(nullptr, tcURL.c_str());
                if (cefWidget) {

                    AFQEmptyDialog dialog(this);

                    connect(cefWidget, SIGNAL(cefQueryRequest(const QCefQuery&)),
                            &dialog, SLOT(qslotQueryRecieved(const QCefQuery&)));

                    dialog.setTitle(QTStr("Popup.AdminBlackInfo"));
                    dialog.setFixedSize(422, 512);
                    dialog.addWidget(cefWidget);
                    dialog.exec();
                }

                showMessageBox = false;
            }
        }
        else if (BS_LIMIT_DUPLICATE_BROAD == ret) {
            broadStartFailMsg = QString::fromStdString(info.broadMsg); 
        }
        else if (BS_LIMIT_DUPLICATE_BROADING == ret) {
            broadStartFailMsg = QString::fromStdString(info.broadMsg);
        }
        else if (BS_BROADSTART_API_JSON_INVALID == ret) {
            if(info.broadMsg.empty())
                broadStartFailMsg = "API.Failed";
            else
                broadStartFailMsg = info.broadMsg.c_str();
        }
        else if (BS_ALREADY_BROADCASTING == ret) {
            broadStartFailMsg = QTStr("Popup.AlreadyBroadCasting");
        }
        else if (BS_INVALID_STREAMKEY == ret) {
            broadStartFailMsg = QTStr("API.Failed");
        }
        else if (BS_SUBSCRIBE_DISABLED == ret)
        {
            broadStartFailMsg = QTStr("Disable.Reject.Subscribe.Broad");
        }
        else if (BS_SUBSCRIBE_NOTSAME == ret)
        {
            failTopMsg = QTStr("Subscribe.Not.Same.Broad");
            broadStartFailMsg = QTStr("Subscribe.Needs.Same");
            messageBoxFixedHeight = 224;
        }
        else if (BS_ACCOUNT_RESIGNED == ret || BS_ACCOUNT_NO_INFO == ret)
        {
            broadStartFailMsg = QTStr("Confirm.Token.Expired");
        }

        // BS_NEED_REALNAME_AUTH, BS_NEED_MINOR_CERTIFY, BS_EMAIL_CERTIFY
        if (!info.redirectURL.empty()) 
        {
            if (BS_EMAIL_CERTIFY == ret)
                VerifyEmailBeforeBroad(info.broadMsg, info.redirectURL);
            else
                NavigateDefaultBrowser(QString::fromStdString(info.redirectURL));

            showMessageBox = false;
        }

        QString failMsg = "[BROAD START FAIL]: " + broadStartFailMsg;
        blog(LOG_INFO, failMsg.toUtf8().constData());
        if (showMessageBox) {
            if (BS_ACCOUNT_RESIGNED == ret || BS_ACCOUNT_NO_INFO == ret)
            {
                OffBroadStartAPICheck();
                LogoutMainAccount(true);
                return;
            }
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                       "", broadStartFailMsg, false, true,
                                       failTopMsg, messageBoxFixedWidth, messageBoxFixedHeight);
        }
    }

    OffBroadStartAPICheck();
}

void AFMainFrame::qslotBroadStartAPIResponse_Disconnect(const QByteArray& responseData) {
    CheckSoopBroadStatus(responseData);
}

void AFMainFrame::qslotBroadStartAPIResponse_CheckStream(const QByteArray& responseData)
{
    OffBroadStartAPICheck();
    CheckSoopBroadStatus(responseData);
}

void AFMainFrame::qslotBroadCheckTimeout()
{
    AUTH_CONTEXT.SendCheckBroading(this, "qslotBroadStartAPIResponse_CheckStream");
}

void AFMainFrame::qslotEmailVerify(const QByteArray& responseData)
{
    std::string temp = std::string(responseData.constData());
    blog(LOG_INFO, temp.c_str());
}

void AFMainFrame::qslotScreenChanged(QScreen* screen)
{
    //invalidate needed on change screen (dpi problem)
    QList<QWidget*> widgets = this->findChildren<QWidget*>();
    
    foreach(QWidget * w, widgets)
        w->update(rect());
}

void AFMainFrame::qslotDpiChanged(qreal rel)
{
    //invalidate needed on change screen (dpi problem)
    qDebug() << "dpi changed:" << rel;
}

void AFMainFrame::qslotTopMenuDestoryed()
{
    m_topMenuTriggered = false;
}

void AFMainFrame::qslotLoginAccountWithProperty()
{
    QObject* objectSender = reinterpret_cast<QObject*>(sender());
    QString platform = objectSender->property("platform").toString();
    if (AddStreamAccount(this, platform))
    {
        m_pMainOutput->SetStreamingOutput();
        LoadAccounts();
    }
}

void AFMainFrame::qslotShowGlobalPageSender()
{
    auto& authManager = AUTH_CONTEXT;
    //
    QPushButton* button = reinterpret_cast<QPushButton*>(sender());
    if (button) {
        QString platform = button->property("platform").toString();
        
        if (platform == PLATFORM_SOOP) {
            QString dashboard_url;
            dashboard_url = QString::fromStdString(SOOP_DASHBOARD_URL);
            NavigateDefaultBrowser(dashboard_url);
        }
        else if (platform == PLATFORM_TWITCH) {
            QString dashboard_url;
            dashboard_url = TWITCH_DASHBOARD_URL + button->property("channelID").toString() + "/home";
            NavigateDefaultBrowser(dashboard_url);
        }
        else if (platform == PLATFORM_YOUTUBE) {
            int cntOfAccount = authManager.GetCntChannel();
            for (int idx = 0; idx < cntOfAccount; idx++)
            {
                AFChannelData* tmpChannel = nullptr;
                authManager.GetChannelData(idx, tmpChannel);
                if (!tmpChannel || !tmpChannel->pAuthData)
                    continue;
                if (tmpChannel->pAuthData->platform == PLATFORM_YOUTUBE)
                {
                    ShowPopupPageYoutubeChannel(tmpChannel->pAuthData);
                    break;
                }
            }
        }

    }    
}

void AFMainFrame::qslotResetCertainBroadTime()
{
    ui->label_BroadTime->ResetCetainTime();
}

void AFMainFrame::qslotExtendResource()
{
    if (m_resourceExtensionWidget == nullptr)
    {
        ui->label_ResourceExtension->setProperty("resourceExtend", true);

        m_resourceExtensionWidget = new AFResourceExtension(this);
        m_resourceExtensionWidget->ResourceExtensionInit();
        ui->frame_Bottom->layout()->addWidget(m_resourceExtensionWidget);
        ui->frame_Bottom->setMinimumHeight(142);
        ui->frame_Bottom->setMaximumHeight(142);
        if (!isMaximized())
            resize(QSize(width(), height() + 42));
        //setMinimumHeight(750);

        m_resourceExtensionWidget->setProperty("windowtype", ENUM_WINDOW_TYPE::StatPage);

        // qslotCheckDiskSpaceRemaining
        connect(m_resourceExtensionWidget, &AFResourceExtension::qsignalCheckDiskSpaceRemaining,
                this, &AFMainFrame::qslotCheckDiskSpaceRemaining);
        connect(m_resourceExtensionWidget, &AFResourceExtension::qsignalStatWindowTriggered, this,
            &AFMainFrame::qslotShowBlockWithProperty);
    }
    else
    {
        ui->label_ResourceExtension->setProperty("resourceExtend", false);

        m_resourceExtensionWidget->close();
        m_resourceExtensionWidget->deleteLater();
        ui->frame_Bottom->setMinimumHeight(100);
        ui->frame_Bottom->setMaximumHeight(100);
        //setMinimumHeight(708);
        if (!isMaximized())
            resize(QSize(width(), height() - 42));
    }

    PolishStyleSheet(ui->label_ResourceExtension);
}

void AFMainFrame::qslotReplayBufferSave()
{
    if (!AFOutputUtil::IsReplayBufferActive())
        return;

    AFOutputUtil::SaveReplayBuffer();
}

void AFMainFrame::qslotReplayBufferSaved()
{
    if (!AFOutputUtil::IsReplayBufferActive())
        return;

    std::string path = AFOutputUtil::SavedReplayBuffer();
    QString msg = QTStr("Basic.StatusBar.ReplayBufferSavedTo").arg(QT_UTF8(path.c_str()));

    //OnEvent(OBS_FRONTEND_EVENT_REPLAY_BUFFER_SAVED);

    m_pMainOutput->AutoRemux(QT_UTF8(path.c_str()));

    ShowSystemAlert(msg, "", AFQSystemAlert::AlertIcon::Success);

    emit qsignalReplayBufferSaved();
}

void AFMainFrame::qslotMinimizeWindow()
{
    GetController()->minimizeWindow();
}

void AFMainFrame::qslotIconActivated(QSystemTrayIcon::ActivationReason reason)
{	
    //projector
    // Refresh projector list
    /*previewProjector->clear();
    studioProgramProjector->clear();
    AddProjectorMenuMonitors(previewProjector, this,
        &OBSBasic::OpenPreviewProjector);
    AddProjectorMenuMonitors(studioProgramProjector, this,
        &OBSBasic::OpenStudioProgramProjector);*/

#ifdef __APPLE__
    UNUSED_PARAMETER(reason);
#else
    if (reason == QSystemTrayIcon::Trigger) {
        EnablePreviewDisplay(m_previewEnabled && !isVisible());
        qslotToggleShowHide();
    }
#endif
}

void AFMainFrame::qslotSetShowing(bool showing)
{
    if (!showing && isVisible()) {
        config_set_string(USERCONFIG, "BasicWindow", "geometry", saveGeometry().toBase64().constData());

        /* hide all visible child dialogs */
        /*visDlgPositions.clear();
        if (!visDialogs.isEmpty()) {
            for (QDialog* dlg : visDialogs) {
                visDlgPositions.append(dlg->pos());
                dlg->hide();
            }
        }*/

        if (m_mainShowHideAction)
            m_mainShowHideAction->setText(QTStr(("Basic.SystemTray.Show")));
        QTimer::singleShot(0, this, &AFMainFrame::hide);

        if (m_previewEnabled)
            EnablePreviewDisplay(false);

#ifdef __APPLE__
        EnableOSXDockIcon(false);
#endif

    }
    else if (showing && !isVisible()) {
        if (m_mainShowHideAction)
            m_mainShowHideAction->setText(QTStr(("Basic.SystemTray.Hide")));
        QTimer::singleShot(0, this, &AFMainFrame::show);

        if (m_previewEnabled)
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
        bool sysTrayMinimizeToTray = config_get_bool(USERCONFIG, "BasicWindow", "SysTrayMinimizeToTray");
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

void AFMainFrame::SystemTrayNotify(const QString& text, QSystemTrayIcon::MessageIcon n)
{
    if (m_trayIcon && m_trayIcon->isVisible() &&
        QSystemTrayIcon::supportsMessages()) {
        QSystemTrayIcon::MessageIcon icon =
            QSystemTrayIcon::MessageIcon(n);
        m_trayIcon->showMessage("OBS Studio", text, icon, 10000);
    }
}

void AFMainFrame::qslotDisplayStreamStartError()
{ 
    OUTPUT_HANDLER_LIST::iterator outputIter;
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();

    emit StreamingStopped();

    for (outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {

        QString message = !outputIter->second->lastError.empty()
                            ? QTStr(outputIter->second->lastError.c_str())
                            : QTStr("Output.StartFailedGeneric");

        QMessageBox::critical(this, QTStr("Output.StartStreamFailed"), message);
    }

    if (!AFOutputUtil::IsStreamActive()) {
        ChangeStreamStateUI(true, false, "LIVE", 77);
        ShowSystemAlert(QTStr("Output.StartStreamFailed")); 
    }
}

void AFMainFrame::qslotTogglePreview()
{
    m_previewEnabled = !m_previewEnabled;
    EnablePreviewDisplay(m_previewEnabled);
}

void AFMainFrame::qslotLockPreview()
{
    CBasicPreview* preview = GetMainWindow()->GetMainPreview();
    if (preview) {
        preview->ToggleLocked();
        ui->action_LockPreview->setChecked(preview->GetLocked());
    }
}

//
void AFMainFrame::qslotSetCurrentSceneFrontendAPI(OBSSource scene, bool force)
{
    SetCurrentScene(scene, force);
}

void AFMainFrame::qslotTransitionStudioModeScene()
{
    if (m_dynamicCompositMainWindow)
        m_dynamicCompositMainWindow->TransitionStudioModeScene();
}

void AFMainFrame::qslotStartStreamingFrontendAPI()
{
    StartStreaming();
}

void AFMainFrame::qslotStopStreamingFrontendAPI()
{
    StopStreaming();
}

void AFMainFrame::qslotStartRecordingFrontendAPI()
{
    StartRecording();
}

void AFMainFrame::qslotStopRecordingFrontendAPI()
{
    StopRecording();
}

void AFMainFrame::qslotToggleMainMicFrontendAPI()
{
    if (m_pMainAudioSource)
        m_pMainAudioSource->qslotSetMicMute();
}

void AFMainFrame::qslotToggleMainVolFrontendAPI()
{
    if (m_pMainAudioSource)
        m_pMainAudioSource->qslotSetVolumeMute();
}

void AFMainFrame::qslotToggleSOOPChannelSidebarFrontendAPI()
{
    if (m_leftNavigationBar)
        m_leftNavigationBar->SelectChannelSlide(PLATFORM_SOOP);
}

void AFMainFrame::qslotToggleSOOPLnbMenuFrontendAPI(int menuType)
{
    auto& authManager = AUTH_CONTEXT;
    if (!authManager.IsSoopRegistered())
        return;

    if (menuType == 99) {

        if (CheckSplitVodAvailable()) {
            m_blockManager->ShowVodSplit(this);
        }
        else {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", QTStr("SplitVod.Condition.Info"), false, true);
        }
        return;
    }
    else if (menuType != -1)
    {
        AFQBorderPopupBaseWidget* popup = nullptr;
        if (m_blockManager->GetPopup((ENUM_WINDOW_TYPE)menuType, popup))
        {
            m_blockManager->qslotClosePopup(menuType);
            return;
        }
        bool success = m_blockManager->MakePopup(menuType, popup);
    }
}

#ifdef __APPLE__
void AFMainFrame::qslotMacSwitchToDock(int windowType, int posX, int posY)
{
    AFQCustomMenu menu(this);
    menu.addAction(QTStr("Block.Title.ToDock"), this, [this, windowType](){
        this->m_blockManager->qslotSwitchBlockWindowType(false, windowType);
    });

    QPoint globalPos(posX,posY);
    menu.exec(globalPos);
}
#endif
void AFMainFrame::qslotProcessHotkey(obs_hotkey_id id, bool pressed)
{
    obs_hotkey_trigger_routed_callback(id, pressed);
}

void AFMainFrame::qslotUpdateContextToolBar(bool force)
{
    m_dynamicCompositMainWindow->UpdateSourceToolBar(force);
}

void AFMainFrame::qslotClearBrowserInteractionPopup(OBSSource source)
{
    MAP_BROWSER_INTERACTION::iterator it = m_mapBrowserInteraction.find(source);

    if (m_mapBrowserInteraction.end() == it)
        return;

    AFQBrowserInteraction* interaction = (*it);
    if (!interaction)
        return;

    interaction->close();
    interaction = nullptr;

    m_mapBrowserInteraction.remove(source);
}

void AFMainFrame::qslotSplitFilterActivated()
{
    if (m_dynamicCompositMainWindow)
        m_dynamicCompositMainWindow->RefreshToolBarSplitFilterToggleButton();
}

void AFMainFrame::qslotUninstallFreecshotAccept()
{
	QString intallPath;
	if (!GetRegKeyValue(CurrentUserRegKey, "SOFTWARE\\soop\\Studio2", "Path", intallPath))
	{
		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", QTStr("Freecshot.UnInstall.Message_3"));
		return;
	}

	QString unInstallPath = QString("%1\\UnInstall2.exe").arg(intallPath);
	bool exists = os_file_exists(unInstallPath.toStdString().c_str());
	if (false == exists)
	{
		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", QTStr("Freecshot.UnInstall.Message_3"));
		return;
	}
    
    // backup freecshot settings
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    dir.cdUp();

    QString freecshotSettingDataPath = dir.absolutePath() + "/SOOP/freecshot/settings";
    QString backupPath = appDataPath + "/freecshot_backup";
    if (QDir(freecshotSettingDataPath).exists()) {
        QDir().mkpath(backupPath);

        QDir srcDir(freecshotSettingDataPath);
        QStringList entries = srcDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);

        for (const QString& entry : entries) {
            QString srcFilePath = srcDir.absoluteFilePath(entry);
            QString destFilePath = backupPath + "/" + entry;

            QFileInfo info(srcFilePath);
            if (info.isDir()) {
                QDir().mkpath(destFilePath);
                QDir subDir(srcFilePath);
                QStringList subEntries = subDir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries);
                for (const QString& subEntry : subEntries) {
                    QFile::copy(subDir.absoluteFilePath(subEntry),
                        destFilePath + "/" + subEntry);
                }
            }
            else {
                QFile::copy(srcFilePath, destFilePath);
            }
        }
    }

    // uninstall freecshot
	UninstallResult ret = RunUninstallerWithUAC(unInstallPath);
	if (UninstallResult::Succeeded == ret)
	{
		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   QTStr("Freecshot.UnInstall.Title_2"), QTStr("Freecshot.UnInstall.Message_2"));
	}
}

void AFMainFrame::qslotCheckVersionLimit()
{
    if(!m_VersionLimit)
    {
        m_VersionLimit = new QNetworkAccessManager(this);

        connect(m_VersionLimit, &QNetworkAccessManager::finished,
            this, &AFMainFrame::qslotGetVersionLimit);

    }

    QUrl url;
    QNetworkRequest request(url);
    request.setTransferTimeout(2000);

    m_VersionLimit->get(request);
}

void AFMainFrame::qslotGetVersionLimit(QNetworkReply* reply)
{
    bool restartNeeded = false;

    do
    {
        if(reply->error() == QNetworkReply::NoError) {
            QStringList formats = {"yyyy/MM/dd", "yyyy/M/d", "yyyy-MM-dd", "yy-MM-dd", "yy-M-d"};

            QByteArray data = reply->readAll();
            QString versionInfo = QString::fromUtf8(data);
            QString rawData = versionInfo.trimmed();
            QDate serverDate;

            for(const QString& fmt : formats) {
                serverDate = QDate::fromString(rawData, fmt);
                if(serverDate.isValid()) break;
            }

            if(!serverDate.isValid())
                break;


            QString appDir = QCoreApplication::applicationDirPath();
            QDir dir(appDir);

            dir.cdUp();
            dir.cdUp();
            dir.cdUp();

            QString updateText = dir.absoluteFilePath("UpdateTime_soop.txt");
            QString filePath = QDir::toNativeSeparators(updateText);

            QFile file(filePath);

            if(file.exists()) {
                if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QTextStream in(&file);
                    QString fullText = in.readAll().trimmed();
                    file.close();

                    QString dateOnly = fullText.section(' ', 0, 0);

                    QDate fileDate;
                    for(const QString& fmt : formats) {
                        fileDate = QDate::fromString(dateOnly, fmt);
                        if(fileDate.isValid()) break;
                    }
                    if(!fileDate.isValid())
                        break;

                    if(fileDate < serverDate) {
                        restartNeeded = true;
                    }
                }
            }
        }
    } while(false);


    reply->deleteLater();

    if(m_VersionLimit) {
        m_VersionLimit->deleteLater();
        m_VersionLimit = nullptr;
    }


    if(restartNeeded)
    {
        bool result = AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                                                               QT_UTF8(""),
                                                               QTStr("Basic.Version.Limit"), QTStr("Basic.Restart"));
        if(result)
        {
            g_bRestart = true;
            m_restartWidthoutConfirm = true;
            _RestartApp();
        } else
        {
            ui->pushButton_Broad->setChecked(false);
            m_checkBroadStartAPI = false;
            m_CheckBroadStartAPITimer->stop();
        }
    } else
    {
        SOOP_API_HANDLER->getAPIfromId(GET_CATEGORY_LIST, {}, this, "qslotCategoryCheckAPIResponse");
    }
}

void AFMainFrame::qslotScreenShot(OBSSource source)
{
    MAIN_SCENESOURCE->Screenshot(source);
}

void AFMainFrame::qSlotUndo()
{
#pragma region _SOOP_BREAKTIME
    if(m_breakTimeManager->IsActive())
        return;
#pragma endregion

    m_undo_s.Undo();
}
void AFMainFrame::qSlotRedo()
{
#pragma region _SOOP_BREAKTIME
    if(m_breakTimeManager->IsActive())
        return;
#pragma endregion

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

    auto& auth = AUTH_CONTEXT;
    //
    if (m_normalInit)
    {
        if (AFOutputUtil::IsStreamActive() &&
            !m_clearingFailed)
        {
            int nBreaktimeCheck = 0;
            if (m_breakTimeManager->IsActive())
            {
                nBreaktimeCheck = AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                                           this, QT_UTF8(""),
                                                                           QTStr("breaktime.broad.end"), QTStr("breaktime.broad.end.btn"));
                if (nBreaktimeCheck != 1)
                {
                    event->ignore();
                    g_bRestart = false;
                    m_logOut = false;
                    return;
                }
            }

            //System Tray Disable - MainView can't be hidden
            //qslotSetShowing(true);
            if (auth.IsSoopStreaming())
            {
                AFQEndBroadDialog* endBroad = new AFQEndBroadDialog(this, true);

                bool splitAvailable = CheckSplitVodAvailable();

                endBroad->EndBroadInfoInit(splitAvailable);
                endBroad->deleteLater();

                if (endBroad->exec() == QDialog::Rejected)
                {
                    event->ignore();
                    g_bRestart = false;
                    m_logOut = false;

                    endBroad->close();
                    return;
                }

                if (nBreaktimeCheck) {
                    QEventLoop loop;
                    QMetaObject::Connection c1, c2;

                    QTimer timer;
                    timer.setSingleShot(true);
                    timer.start(3000);

                    c1 = connect(&BREAKTIME_MANAGER, &BreaktimeManager::signalStopApisDone, &loop, [&loop](bool) { loop.quit(); });

                    c2 = connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

                    m_breakTimeManager->Stop(true);

                    loop.exec();

                    disconnect(c1);
                    disconnect(c2);
                }
              
                endBroad->close();
            }
            else
            {
                bool warnStopBroad = config_get_bool(USERCONFIG, "BasicWindow", "WarnBeforeStoppingStream");
                if (warnStopBroad) {
                    int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                        "",
                        QTStr("Output.EndStudioOnBroad.Info"));

                    if (result == QDialog::Rejected) {
                        event->ignore();
                        g_bRestart = false;
                        m_logOut = false;
                        return;
                    }
                }
            }
            //
            int nMinsimCheckCnt = config_get_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt");
            if (nMinsimCheckCnt > 0) {
                auto broadInfo = auth.GetSoopBroadInfo();
                
                auto endTime = std::chrono::steady_clock::now();
                auto startTime = auth.GetMinsimCheckStartTime();
                if (startTime != std::chrono::steady_clock::time_point{}) {
                    auth.InitMinsimCheckStartTime();
                }
            }
        }
        else if (AFOutputUtil::IsRecordingActive() && !m_clearingFailed)
        {
            bool warnStopRecord = config_get_bool(USERCONFIG, "BasicWindow", "WarnBeforeStoppingRecord");
            if (warnStopRecord) {
                int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                    "",
                    QTStr("ConfirmExit.Text"));

                if (result == QDialog::Rejected) {
                    event->ignore();
                    g_bRestart = false;
                    m_logOut = false;
                    return;
                }
            }
        }
    }

    if (m_AddStreamWidget)
        m_AddStreamWidget->close(); //delete on AFMainFrame::AddStreamAccount

    BroadInfoTimerStop();

    int cntOfAccount = auth.GetCntChannel();
    for (int idx = 0; idx < cntOfAccount; idx++)
    {
        AFChannelData* tmpChannel = nullptr;
        auth.GetChannelData(idx, tmpChannel);

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
    auth.GetMainChannelData(tmpMainChannel);

    if (tmpMainChannel != nullptr)
    {
        QPixmap* delObj = (QPixmap*)tmpMainChannel->pObjQtPixmap;
        if (delObj != nullptr) {
            delete delObj;
            tmpMainChannel->pObjQtPixmap = nullptr;
        }
    }
    
    std::vector<void*>* pVecDeferrdDel = AUTH_CONTEXT.GetContainerDeferrdDelObj();
    for(void* node : *pVecDeferrdDel)
    {
        QPixmap* delObj = (QPixmap*)node;
        delete delObj;
    }
    pVecDeferrdDel->clear();

    if (m_refreshVideoBallonTimer) {
        m_refreshVideoBallonTimer->stop();
        delete m_refreshVideoBallonTimer;
        m_refreshVideoBallonTimer = nullptr;
    }

    if (isVisible())
    {
        config_set_string(USERCONFIG, "BasicWindow", "geometry", saveGeometry().toBase64().constData());

        QRect mainRect = normalGeometry();
        std::string strPos = std::to_string(mainRect.x()) + "." + std::to_string(mainRect.y())
            + "." + std::to_string(mainRect.width()) + "." + std::to_string(mainRect.height());
        config_set_string(USERCONFIG, "BasicWindow", "MainframeCheckRect", strPos.c_str());
    }

    //Remux
    /*if (remux && !remux->close()) {
        event->ignore();
        restart = false;
        return;
    }*/

    QWidget::closeEvent(event);
    if (!event->isAccepted())
    {
        m_logOut = false;
        return;
    }

    if (m_logOut)
    {
        AFChannelData* tmpSoopChannel = nullptr;
        if (auth.GetChannelData(PLATFORM_SOOP, tmpSoopChannel))
            if(tmpSoopChannel)
                tmpSoopChannel->pAuthData->loginRetain = false;

        auth.RemoveAllChannel(false);
    }

    if (m_normalInit && m_dynamicCompositMainWindow)
    {
        m_blockManager->FinPopups();

        config_set_bool(USERCONFIG, "BasicWindow", "PreviewProgramMode", IsPreviewProgramMode());
        config_save_safe(USERCONFIG, "tmp", nullptr);

        auth.SaveAllAuthed();
        auth.DeleteSoopBroadInfo();
        LOADSAVE_CONTEXT.SaveProjectNow();
    }

    //OnEvent(OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN);

    //disableSaving++;

    AFMainWindowAccesser* tmpViewModels = g_viewModelsDynamic.UnSafeGetInstace();
    if (tmpViewModels)
        tmpViewModels->m_renderModel.RemoveCallbackMainDisplay();

    if(AFOutputUtil::IsVirtualCamActive()) {
        OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
        if(!outputHandlers.empty()) {
            OUTPUT_HANDLER_PTR& outputHandler = outputHandlers[0].second;
            if(outputHandler)
                outputHandler->StopVirtualCam();
        }
    }

    /* Clear all scene data (dialogs, widgets, widget sub-items, scenes,
     * sources, etc) so that all references are released before shutdown */
    ClearSceneData();
    
    //OnEvent(OBS_FRONTEND_EVENT_EXIT);

    // Destroys the frontend API so plugins can't continue calling it
    soop_frontend_set_callbacks_internal(nullptr);
    api = nullptr;

    if (m_normalInit && m_dynamicCompositMainWindow)
        m_dynamicCompositMainWindow->close();

    QApplication::sendPostedEvents(nullptr);
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

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
    QWidget::showEvent(event);

    if (m_isInitialRun) {
        emit qsignalMainShowEventTriggered();
        m_isInitialRun = false;
    }
}

void AFMainFrame::moveEvent(QMoveEvent* event)
{
    if (m_leftNavigationBar) {
#ifdef _WIN32
        m_leftNavigationBar->MoveChannelSlide();
#elif defined(__APPLE__)
        m_leftNavigationBar->HideChannelSlide();
#endif
    }

    if (m_systemAlert && m_systemAlert->isVisible()) {
        _MoveSystemAlert(m_systemAlert, this->isMinimized());
    }

    if (m_pCurrentScreen != App()->screenAt(this->mapToGlobal(rect().center())))
    {
        m_pCurrentScreen = App()->screenAt(this->mapToGlobal(rect().center()));
        repaint();
    }

    bool isAboutToMax = GetAboutToMax();

    if (!isAboutToMax)
        emit qsignalmovedOrResized();
}

void AFMainFrame::dragEnterEvent(QDragEnterEvent* event)
{
    // refuse drops of our own widgets
    if (event->source() != nullptr) {
        event->setDropAction(Qt::IgnoreAction);
        return;
    }

    event->acceptProposedAction();
}

void AFMainFrame::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

void AFMainFrame::dragMoveEvent(QDragMoveEvent* event)
{
    event->acceptProposedAction();
}

void AFMainFrame::dropEvent(QDropEvent* event)
{
    if (m_pMainDragDrop)
        m_pMainDragDrop->DropEvent(event);
}

void AFMainFrame::resizeEvent(QResizeEvent* event)
{
    if (m_leftNavigationBar) {
#ifdef _WIN32
        m_leftNavigationBar->MoveChannelSlide();
#elif defined(__APPLE__)
        m_leftNavigationBar->HideChannelSlide();
#endif
    }

    if (m_systemAlert) {
        m_systemAlert->close();
    }

    emit qsignalMainResized(event->oldSize(), size());

    bool isAboutToMax = GetAboutToMax();

    if (!isAboutToMax)
        emit qsignalmovedOrResized();
}

void AFMainFrame::_ConnectStatisticsSignals()
{
    auto& statistics = STATISTICS;
    //
    connect(&statistics, &AFStatistics::qsignalCheckDiskSpaceRemaining, this, &AFMainFrame::qslotCheckDiskSpaceRemaining);
    connect(&statistics, &AFStatistics::qsignalMemoryError, this, &AFMainFrame::qslotShowMemorySystemAlert);
    connect(&statistics, &AFStatistics::qsignalCPUError, this, &AFMainFrame::qslotShowCPUSystemAlert);
    connect(&statistics, &AFStatistics::qsignalNetworkError, this, &AFMainFrame::qslotShowNetworkSystemAlert);
    connect(&statistics, &AFStatistics::qsignalFPSState, this, &AFMainFrame::qslotResourceState);
}

void AFMainFrame::_SetMainFrameUI()
{
    // Connect MainFrame Signals
    
    // QSS - Broad: Color Different, Record: false
    ui->pushButton_Broad->setProperty("IsLive", false);
    ui->pushButton_Record->setProperty("IsRec", false);

    PolishStyleSheet(ui->pushButton_Broad);
    PolishStyleSheet(ui->pushButton_Record);

    // Top Menu Buttons
    connect(ui->pushButton_TopMenu, &QPushButton::clicked, this, &AFMainFrame::qslotTopMenuClicked);
    connect(ui->pushButton_MinimumWindow, &QPushButton::clicked, this, &AFMainFrame::qslotMinimizeWindow);
    connect(ui->pushButton_MaximumWindow, &QPushButton::clicked, GetController(), &AFQBaseWindowController::maximizeWindow);
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFMainFrame::close);
    connect(GetController(), &AFQBaseWindowController::qsignalMaximized, this, &AFMainFrame::qslotMaximizedChanged);

    // Bottom Menu Buttons
    //Check Version Limit -> need change
    connect(ui->pushButton_Broad, &QPushButton::clicked, this, &AFMainFrame::qslotCheckBroadAvailable);
    connect(ui->pushButton_Broad, &QPushButton::pressed, this, &AFMainFrame::qslotSetButtonOpacity);
    connect(ui->pushButton_Broad, &QPushButton::released, this, &AFMainFrame::qslotRemoveButtonOpacity);

    QGraphicsOpacityEffect* broadEffect = new QGraphicsOpacityEffect();
    broadEffect->setOpacity(0.5);
    ui->pushButton_Broad->setGraphicsEffect(broadEffect);
    ui->pushButton_Broad->graphicsEffect()->setEnabled(false);

    connect(ui->pushButton_Record, &QPushButton::clicked, this, &AFMainFrame::qslotChangeRecordState);
    connect(ui->pushButton_Record, &QPushButton::pressed, this, &AFMainFrame::qslotSetButtonOpacity);
    connect(ui->pushButton_Record, &QPushButton::released, this, &AFMainFrame::qslotRemoveButtonOpacity);

    QGraphicsOpacityEffect* recEffect = new QGraphicsOpacityEffect();
    recEffect->setOpacity(0.5);
    ui->pushButton_Record->setGraphicsEffect(recEffect);
    ui->pushButton_Record->graphicsEffect()->setEnabled(false);

    if (m_pMainAudioSource)
        m_pMainAudioSource->SetupMainFrameAudioUI(this);

    connect(ui->pushButton_ShortcutSettings, &QPushButton::clicked, this, &AFMainFrame::qslotShowStudioSettingPopup);
    connect(ui->widget_ResourceExtension, &AFQHoverWidget::qsignalMouseClick, this, &AFMainFrame::qslotExtendResource);

    ui->widget_BroadTimer->setVisible(false);
    ui->widget_RecordTimer->setVisible(false);
    ui->line_Time->setVisible(false);
    ui->pushButton_ShortcutSettings->setProperty("buttonType", "settings");
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
        globalPosY = ui->widget_ResourceButton->mapToGlobal(QPoint(0, 0)).y();
        globalPosX -= (systemAlert->width() / 2);
        globalPosY -= (systemAlert->height() / 2);
    }
    
    systemAlert->move(QPoint(globalPosX, globalPosY));
}

void AFMainFrame::_RestartApp()
{
    if (g_bRestart)
    {
        if (!m_restartWidthoutConfirm)  // Ask Whether To Restart
        {
            int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                "", QTStr("NeedsRestart"));

            if (result == QDialog::Accepted)
            {
                close();
            }
            else
            {
                g_bRestart = false;
            }
        }
        else // Force Restart
        {
            close();
        }
    }
}

void AFMainFrame::_CreateTopMenu()
{ 
    if (m_topMenu)
        return;

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
    //TOP MENU
    m_topMenu = new AFQCustomMenu("Top Menu", this);
    connect(m_topMenu, &AFQCustomMenu::aboutToHide, this, &AFMainFrame::qslotTopMenuDestoryed);

    m_topMenu->setFixedWidth(200);

    //TOOL
    AFQCustomMenu* toolMenu = new AFQCustomMenu(this, true);
    toolMenu->setFixedWidth(200);

    toolMenu->addAction(ui->action_Settings);
    toolMenu->addAction(ui->action_VirtualCamera);
    toolMenu->addAction(ui->action_AdvanceControls);
    toolMenu->addAction(ui->action_Stats);

    ui->action_AdvanceControls->setProperty("windowtype", ENUM_WINDOW_TYPE::AdvanceControls);
    ui->action_Stats->setProperty("windowtype", ENUM_WINDOW_TYPE::StatPage);

    connect(ui->action_Settings, &QAction::triggered, this, &AFMainFrame::qslotShowStudioSettingPopup);
    //
    connect(ui->action_VirtualCamera, &QAction::triggered, m_leftNavigationBar, &AFQLeftNavigationBar::qslotVirtualCamConfig);
    connect(ui->action_AdvanceControls, &QAction::triggered, this, &AFMainFrame::qslotShowBlockWithProperty);
    connect(ui->action_Stats, &QAction::triggered, this, &AFMainFrame::qslotShowBlockWithProperty);

    funcSetWhatsThis(toolMenu->actions());
    m_topMenu->addMenu(toolMenu)->setText(QT_UTF8(Str("Basic.MainMenu.ToolsFix")));

    //View
    
    AFQCustomMenu* viewMenu = new AFQCustomMenu(this, true);

    bool PropertiesOn = config_get_bool(USERCONFIG, "BasicWindow", "ShowContextToolbars");
    ui->action_ViewToggleProperties->setChecked(PropertiesOn);
    qslotPropertiesToggled(PropertiesOn);

    if (false) //Always On Top Hide until All Dialog Fix Z order
    {
        bool AlwaysOnTop = config_get_bool(USERCONFIG, "General", "AlwaysOnTop");
        ui->action_ViewToggleOnTop->setChecked(AlwaysOnTop);
        if (AlwaysOnTop || g_opt_always_on_top)
            SetAlwaysOnTop(this, true);

        viewMenu->addAction(ui->action_ViewToggleOnTop);
    }
    else
    {
        config_set_bool(USERCONFIG, "General", "AlwaysOnTop", false);
        g_opt_always_on_top = false;
        SetAlwaysOnTop(this, false);
    }

    bool sideDockOn = config_get_bool(USERCONFIG, "BasicWindow", "SideDocks");
    ui->action_SideDocks->setChecked(sideDockOn);

    viewMenu->addAction(ui->action_ViewToggleProperties);
    viewMenu->addAction(ui->action_ViewUIReset);
    viewMenu->addAction(ui->action_ViewSceneControl);
    //viewMenu->addAction(ui->action_LockDocks);
    viewMenu->addAction(ui->action_SideDocks);
    viewMenu->addAction(ui->action_ViewRaiseAllBlock);
    viewMenu->addAction(ui->action_ViewCloseAllBlocks);

    connect(ui->action_ViewToggleProperties, &QAction::triggered, this, &AFMainFrame::qslotPropertiesToggled);
    connect(ui->action_ViewToggleOnTop, &QAction::triggered, this, &AFMainFrame::qslotAlwaysOnTopToggled);
    connect(ui->action_ViewUIReset, &QAction::triggered, this, &AFMainFrame::qslotUIResetTriggered);
    connect(ui->action_ViewSceneControl, &QAction::triggered, this, &AFMainFrame::qslotSceneControlTriggered);
    connect(ui->action_LockDocks, &QAction::triggered, MAIN_BLOCKMANAGER, &AFQBlockManager::qslotLockDock);
    connect(ui->action_SideDocks, &QAction::triggered, DYNAMIC_COMPOSIT, &AFMainDynamicComposit::SetSideDock);
    connect(ui->action_ViewRaiseAllBlock, &QAction::triggered, this, &AFMainFrame::qslotPopupBlockClicked);
    connect(ui->action_ViewCloseAllBlocks, &QAction::triggered, this, &AFMainFrame::qslotCloseAllBlocks);

    funcSetWhatsThis(viewMenu->actions());
    m_topMenu->addMenu(viewMenu)->setText(QT_UTF8(Str("Basic.TopMenu.View")));

    //ADD-ON
    m_addonMenu = new AFQCustomMenu(this, true);
    m_addonMenu->setFixedWidth(200);

    connect(ui->action_ImportRecentBroadcastSettings, &QAction::triggered, this, &AFMainFrame::qslotShowImportRecentGuide);
    
    funcSetWhatsThis(m_addonMenu->actions());
    m_topMenu->addMenu(m_addonMenu)->setText(QT_UTF8(Str("Basic.MainMenu.Addon")));
        
    //m_topMenu->addAction(ui->action_ImportPreset);

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

    connect(ui->action_NewProfile, &QAction::triggered, m_mainProfile, &AFMainProfile::qActionNewProfileTriggered);
    connect(ui->action_DupProfile, &QAction::triggered, m_mainProfile, &AFMainProfile::qActionDupProfileTriggered);
    connect(ui->action_RenameProfile, &QAction::triggered, m_mainProfile, &AFMainProfile::qActionRenameProfileTriggered);
    connect(ui->action_RemoveProfile, &QAction::triggered, m_mainProfile, &AFMainProfile::qActionRemoveProfileTriggered);

    connect(ui->action_ExportProfile, &QAction::triggered, m_mainProfile, &AFMainProfile::qActionExportProfileTriggered);
    connect(ui->action_ImportProfile, &QAction::triggered, m_mainProfile, &AFMainProfile::qActionImportProfileTriggered);
    
    funcSetWhatsThis(profileMenu->actions());
    
    ui->action_Profile->setMenu(profileMenu);
    m_topMenu->addMenu(profileMenu)->setText(QT_UTF8(Str("Basic.MainMenu.ProfileFix")));
        
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

    connect(ui->action_NewSceneCollection, &QAction::triggered, m_mainSceneCollection, &AFMainSceneCollection::qActionNewSceneCollectionTriggered);
    connect(ui->action_DupSceneCollection, &QAction::triggered, m_mainSceneCollection, &AFMainSceneCollection::qActionDupSceneCollectionTriggered);
    connect(ui->action_RenameSceneCollection, &QAction::triggered, m_mainSceneCollection, &AFMainSceneCollection::qActionRenameSceneCollectionTriggered);
    connect(ui->action_RemoveSceneCollection, &QAction::triggered, m_mainSceneCollection, &AFMainSceneCollection::qActionRemoveSceneCollectionTriggered);
    connect(ui->action_ExportSceneCollection, &QAction::triggered, m_mainSceneCollection, &AFMainSceneCollection::qActionExportSceneCollectionTriggered);
    connect(ui->action_ImportSceneCollection, &QAction::triggered, m_mainSceneCollection, &AFMainSceneCollection::qActionImportSceneCollectionTriggered);
    //
    connect(ui->action_ImportPreset, &QAction::triggered, this, &AFMainFrame::qslotImportPreset);
    connect(ui->action_ShowMissingFiles, &QAction::triggered, this, &AFMainFrame::qslotShowMissingFiles);
    
    funcSetWhatsThis(sceneCollectionMenu->actions());

    ui->action_SceneCollection->setMenu(sceneCollectionMenu);
    m_topMenu->addMenu(sceneCollectionMenu)->setText(QT_UTF8(Str("Basic.MainMenu.SceneCollectionFix")));

    connect(ui->action_LinkServiceNotice, &QAction::triggered, this, &AFMainFrame::qslotNavigateSoopServiceNoticePage);

    m_topMenu->addAction(ui->action_LinkServiceNotice);

    AFQCustomMenu* infoMenu = new AFQCustomMenu(this, true);
    infoMenu->setFixedWidth(200);

    infoMenu->addAction(ui->action_Guide);

    connect(ui->action_LinkStreammerSupport, &QAction::triggered, this, &AFMainFrame::qslotNavigateStreamerSuppportPage);
    connect(ui->action_Homepage, &QAction::triggered, this, &AFMainFrame::qslotNavigateSoopliveKrPage);

    infoMenu->addAction(ui->action_LinkStreammerSupport);
    QString strMenuHome = QString("SOOP %1").arg(QTStr("Basic.MainMenu.Help.WebHome"));
    ui->action_Homepage->setText(strMenuHome);
    infoMenu->addAction(ui->action_Homepage);

    infoMenu->addAction(ui->action_UpdateLog);
    connect(ui->action_UpdateLog, &QAction::triggered, this, &AFMainFrame::qslotShowStudioUpdatePage);

    connect(ui->action_FAQ, &QAction::triggered, this, &AFMainFrame::qslotShowMigrationGuide);
    infoMenu->addAction(ui->action_FAQ);
    
    infoMenu->addAction(ui->action_ProgramInfo);
    //infoMenu->addAction(ui->action_Update);

    connect(ui->action_Guide, &QAction::triggered, this, &AFMainFrame::qslotMainFrameTutorial);
    connect(ui->action_ProgramInfo, &QAction::triggered, this, &AFMainFrame::qslotProgamInfoOpenTriggered);

    funcSetWhatsThis(infoMenu->actions());

    m_topMenu->addMenu(infoMenu)->setText(QT_UTF8(Str("Basic.MainMenu.HelpFix")));

    //INFO
    QString loginText = QTStr("Login");
    AFChannelData* channelData;

    if (AUTH_CONTEXT.GetMainChannelData(channelData))
    {
        QString channelID = QString::fromStdString(channelData->pAuthData->channelID);
        int maxLen = 9;

        if (channelID.length() > maxLen) {
            channelID = channelID.left(maxLen) + "...";
        }

        loginText = QTStr("Logout.With.Account").arg(channelID);
    }

    m_loginAction = new QAction(this);
    m_loginAction->setText(loginText);
    QAction* exitAction = new QAction(this);
    exitAction->setText(QTStr("Exit"));

    connect(m_loginAction, &QAction::triggered, this, &AFMainFrame::qslotToggleMainAccount);
    connect(exitAction, &QAction::triggered, this, &AFMainFrame::close);

    m_topMenu->addAction(m_loginAction);
    m_topMenu->addAction(exitAction);

#ifdef __APPLE__
    QMenuBar *menuBar = new QMenuBar(this);
    menuBar->setNativeMenuBar(true);
    
    menuBar->addMenu(m_topMenu);
#endif
}

void AFMainFrame::_ToggleTopMenu()
{
	QPushButton* topMenuButton = reinterpret_cast<QPushButton*>(sender());

	if (m_topMenuTriggered) {
		m_topMenu->hide();
	}
	else {
        QList<QAction*> actions = m_addonMenu->actions();

        AFQCustomMenu* customBrowserMenu = _FindSubMenuByTitle(m_topMenu, QTStr("Basic.MainMenu.Addon.CustomBrowserDocks"));
        if (customBrowserMenu)
        {
            QAction* customBrowserAction = customBrowserMenu->menuAction();
            auto it = std::find(actions.begin(), actions.end(), customBrowserAction);
            if (it != actions.end()) {
                auto nextIt = std::next(it);
                if (!actions.contains(ui->action_ImportPreset)) {
                    if (nextIt != actions.end())
                        m_addonMenu->insertAction(*nextIt, ui->action_ImportPreset);
                    else
                        m_addonMenu->addAction(ui->action_ImportPreset); // add end menu
                }

                if (!actions.contains(ui->action_ImportRecentBroadcastSettings)) {
                    m_addonMenu->insertAction(ui->action_ImportPreset, ui->action_ImportRecentBroadcastSettings);
                }
            }
        }

		QPoint position = this->pos();
		m_topMenu->show(QPoint(position.x(), position.y() + ui->widget_Top->height()));
	}
	m_topMenuTriggered = !m_topMenuTriggered;
}

void AFMainFrame::_AddBroadPreset()
{
    int sourcesCount = m_presetSourcesGeometry.size();
    if (sourcesCount < 1)
        return;

    obs_video_info ovi;
    obs_get_video_info(&ovi);

    QString sourceType;
    QRectF sourceRelativeRect;

    for (int idx = 0; idx < sourcesCount; idx++)
    {
        sourceType = m_presetSourcesGeometry[idx].first;
        const char* sourceId = "";

        obs_bounds_type boundType = OBS_BOUNDS_NONE;

        if (sourceType == PRESET_SOURCETYPE_VIDEOCAPTURE)
        {
#if defined(_WIN32)
            sourceId = "dshow_input";
#elif defined(__APPLE__)
            sourceId = "av_capture_input";
#endif
            boundType = OBS_BOUNDS_SCALE_INNER;
        }
        else if (sourceType == PRESET_SOURCETYPE_GAMECAPTURE)
        {
#if defined(_WIN32)
            sourceId = "game_capture";
#elif defined(__APPLE__)
            sourceId = "syphon-input";
#endif
            boundType = OBS_BOUNDS_SCALE_INNER;
        }
        else if (sourceType == PRESET_SOURCETYPE_DIRECTBROAD) {
            sourceId = "soop_directbroad_source";
            boundType = OBS_BOUNDS_STRETCH;
        }
        else if (sourceType == PRESET_SOURCETYPE_ALERT) {
            sourceId = "soop_chat_source_notice";
            boundType = OBS_BOUNDS_STRETCH;
        }
        else if (sourceType == PRESET_SOURCETYPE_CHATTING) {
            sourceId = "soop_chat_source_chat";
            boundType = OBS_BOUNDS_STRETCH;
        }
        else if (sourceType == PRESET_SOURCETYPE_TARGETGRAPH) {
            sourceId = "soop_chat_source_goal";
            boundType = OBS_BOUNDS_STRETCH;
        }
        else if (sourceType == PRESET_SOURCETYPE_GIFTSUBTITLE) {
            sourceId = "soop_chat_source_subtitle";
            boundType = OBS_BOUNDS_STRETCH;
        }
        else
            continue;

        if (sourceId && sourceId[0] == '\0')
            continue;

        // Set Source Size, Position
        sourceRelativeRect = m_presetSourcesGeometry[idx].second;

        float sourceWidth = ovi.base_width * sourceRelativeRect.width();
        float sourceHeight = ovi.base_height * sourceRelativeRect.height();
        float sourcePosX = ovi.base_width * sourceRelativeRect.x();
        float sourcePosY = ovi.base_height * sourceRelativeRect.y();

        // Add Source
        _AddBroadPresetSource(sourceId, sourceWidth, sourceHeight, sourcePosX, sourcePosY, boundType);
    }

    LOADSAVE_CONTEXT.ForceSaveProjectNow();
    m_mainSceneCollection->RefreshSceneCollections();
}

void AFMainFrame::_AddBroadPresetSource(const char* sourceId, float width, float height, float posX, float posY, obs_bounds_type boundType)
{
    obs_transform_info itemInfo;
    vec2_set(&itemInfo.pos, posX, posY);
    vec2_set(&itemInfo.scale, 1.0f, 1.0f);
    vec2_set(&itemInfo.bounds, width, height);

    itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
    itemInfo.rot = 0.0f;
    itemInfo.bounds_type = boundType;
    itemInfo.bounds_alignment = OBS_ALIGN_CENTER;

    OBSSource newSource;
    QString sourceName = AFSourceUtil::GetPlaceHodlerText(sourceId);

    AFSourceUtil::AddNewSource(this, sourceId, sourceName.toStdString().c_str(), true, newSource, &itemInfo);
    
    if (AFSourceUtil::IsSoopMediaSource(newSource)) {
        SOOP_SRC_MANAGER.SetSoopMediaSource(newSource);
    }

    // To resize preset empty source
    OBSDataAutoRelease data = obs_source_get_settings(newSource);
    obs_data_set_bool(data, "preset", true);
    obs_data_set_int(data, "width", width);
    obs_data_set_int(data, "height", height);
    
    //soop_source_empty_video(newSource, width, height);
}

void AFMainFrame::_AccountButtonStreamingToggle(bool stream)
{
    QList<AFMainAccountButton*> buttons = findChildren<AFMainAccountButton*>();

    foreach(AFMainAccountButton * button, buttons) {
        if (button->GetCurrentState() == AFMainAccountButton::ChannelState::Disable)
            continue;
        else
            if (button->GetChannelData() && !button->GetChannelData()->isStreaming)
                continue;


        button->SetStreaming(stream);
    }

    m_leftNavigationBar->ToggleAddChannelButton(stream);

    emit qsignalBroadToggled(stream);
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

void AFMainFrame::EnableTransitionState(bool enable)
{
    m_transitionWidgetEnabled = enable;

    EnableTransitionWidgets();
}

void AFMainFrame::EnableTransitionWidgets()
{
    bool enable = m_transitionWidgetEnabled;

    // Set Scene Transition Popup Widget Enabled
    if (m_sceneTransitionPopup)
        m_sceneTransitionPopup->SetWidgetsEnabled(enable);

    // Set Stinger Properties Enabled
    if (!enable)
    {
        if (m_sourceProperties != nullptr)
        {
            if (m_sourceProperties->isVisible() &&
                m_sourceProperties->GetSourceType() == OBS_SOURCE_TYPE_TRANSITION)
            {
                m_sourceProperties->CloseSourcePropertise();
            }
        }
    }

}

void AFMainFrame::ShowUninstallFreecShotAlert()
{
    if (m_leftNavigationBar)
        m_leftNavigationBar->ResetChannelSlide();

    bool closed = true;
    if (m_uninstallFreecshotAlert)
        closed = m_uninstallFreecshotAlert->close();

    if (!closed)
        return;

    m_uninstallFreecshotAlert = new AFQFreecshotUninstallAlert(this);
    m_uninstallFreecshotAlert->setAttribute(Qt::WA_DeleteOnClose, true);

    connect(m_uninstallFreecshotAlert, &AFQFreecshotUninstallAlert::qsignalninstallFreecshotAccept,
        this, &AFMainFrame::qslotUninstallFreecshotAccept);

    m_uninstallFreecshotAlert->show();
}

#ifdef _WIN32

void AFMainFrame::AddRegStartProcessWindows() {
    auto createOrOpenRegKey = [](HKEY rootKey, const QString& subKey) -> HKEY {
        HKEY hKey;
        LONG lRes = RegCreateKeyEx(rootKey, subKey.toStdWString().c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
        if (lRes != ERROR_SUCCESS) {
            qDebug() << "Failed to create or open key:" << subKey << "Error code:" << lRes;
            return NULL;
        }
        return hKey;
    };

    auto setRegValue = [](HKEY hKey, const QString& valueName, const QString& value) -> bool {
        LONG lRes = RegSetValueEx(hKey, valueName.toStdWString().c_str(), 0, REG_SZ, (BYTE*)value.utf16(), (DWORD)((value.length() + 1) * sizeof(wchar_t)));
        if (lRes != ERROR_SUCCESS) {
            qDebug() << "Failed to set value:" << valueName << "Error code:" << lRes;
            return false;
        }
        return true;
    };

    auto getRegValue = [](HKEY hKey, const QString& valueName) -> QString {
        DWORD size = 0;
        LONG lRes = RegQueryValueEx(hKey, valueName.toStdWString().c_str(), NULL, NULL, NULL, &size);
        if (lRes != ERROR_SUCCESS) {
            return "";
        }

        std::wstring buffer(size / sizeof(wchar_t), L'\0');
        lRes = RegQueryValueEx(hKey, valueName.toStdWString().c_str(), NULL, NULL, (LPBYTE)buffer.data(), &size);
        if (lRes == ERROR_SUCCESS) {
            return QString::fromStdWString(buffer);
        }
        return "";
    };

    QString targetPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QString workingDir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());

    QString bitDir = QFileInfo(workingDir).absolutePath();
    QString binDir = QFileInfo(bitDir).absolutePath();
    QString updaterDir = QFileInfo(binDir).absolutePath() + "\\update";
    QString launcherPath = QDir::toNativeSeparators(updaterDir + "\\" UPDATE_LAUNCHER_NAME);

    HKEY hKey;
    LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Classes\\soopstudio\\shell\\open\\command"), 0, KEY_READ | KEY_WRITE, &hKey);
    if (lRes == ERROR_SUCCESS) {

        QString existingCommand = getRegValue(hKey, "");

        if (existingCommand.contains("SOOPUpdaterLauncher.exe", Qt::CaseInsensitive)) {
            RegCloseKey(hKey);
            return;
        }

        QString newCommand = QString("\"%1\" \"%2\"").arg(launcherPath).arg("%1");
        if (!setRegValue(hKey, "", newCommand)) {
            qDebug() << "Failed to update registry command value.";
        }
        else {
            qDebug() << "Updated existing registry key with new launcher path.";
        }
        RegCloseKey(hKey);
        return;
    }

    hKey = createOrOpenRegKey(HKEY_CURRENT_USER, "Software\\Classes\\soopstudio");
    if (hKey) {
        if (setRegValue(hKey, "", "URL:SOOPStudio") &&
            setRegValue(hKey, "URL Protocol", "") &&
            setRegValue(hKey, "FriendlyTypeName", "SOOPStudio 열기")) {
            RegCloseKey(hKey);
        }
        else {
            qDebug() << "Failed to set registry values for base key.";
            RegCloseKey(hKey);
            return;
        }
    }
    else {
        qDebug() << "Failed to create or open base registry key.";
        return;
    }

    if ((hKey = createOrOpenRegKey(HKEY_CURRENT_USER, "Software\\Classes\\soopstudio\\shell"))) {
        RegCloseKey(hKey);
}
    else {
        return;
    }

    if ((hKey = createOrOpenRegKey(HKEY_CURRENT_USER, "Software\\Classes\\soopstudio\\shell\\open"))) {
        RegCloseKey(hKey);
    }
    else {
        return;
    }

    hKey = createOrOpenRegKey(HKEY_CURRENT_USER, "Software\\Classes\\soopstudio\\shell\\open\\command");
    if (hKey) {
        QString strCommand = QString("\"%1\" \"%2\"").arg(launcherPath).arg("%1");
        if (!setRegValue(hKey, "", strCommand)) {
            qDebug() << "Failed to set command value.";
        }
        else {
            qDebug() << "Successfully created registry keys and values.";
        }
        RegCloseKey(hKey);
    }
    else {
        qDebug() << "Failed to create command key.";
    }
}

void AFMainFrame::UpdaterKill()
{
    auto killIfRunning = [](const QString& imageName) {
        QProcess process;
        process.start("tasklist", { "/fi", QString("IMAGENAME eq %1").arg(imageName) });
        if (!process.waitForFinished(3000))
            return;

        const QString output = process.readAllStandardOutput();
        if (output.contains(imageName, Qt::CaseInsensitive)) {
            QProcess::execute("taskkill", { "/f", "/im", imageName });
        }
    };

    killIfRunning("SOOPUpdaterLauncher.exe");
    killIfRunning("updater.exe");
}

#else // APPLE


#endif

void AFMainFrame::InitLnbMenuItems()
{
    for (LnbMenuItem& item : lnbMenuItems) {
        delete item.action.data();
    }
    lnbMenuItems.clear();

    std::string absPath;
    GetDataFilePath("assets", absPath);

    const QString iconBasePath =
        QString("%1/platform/channel").arg(QString::fromStdString(absPath));

    const QString iconDefaultPath = QString("%1/default").arg(iconBasePath);
    const QString iconActivePath = QString("%1/checked").arg(iconBasePath);
    const QString iconDisabledPath = QString("%1/disabled").arg(iconBasePath);

    auto addFavoriteMenu =
        [this, &iconDefaultPath, &iconActivePath, &iconDisabledPath](
            const char* menuId,
            const char* menuLog,
            const char* iconFileName,
            ENUM_WINDOW_TYPE windowType)
        {
            const QString menuIdText = QString::fromUtf8(menuId);
            const QString menuLogText = QString::fromUtf8(menuLog);
            const QString iconFileNameText = QString::fromUtf8(iconFileName);

            QAction* action = new QAction(this);
            connect(action, &QAction::triggered,
                this, [this, menuIdText, windowType]() {
                    if (m_leftNavigationBar)
                        m_leftNavigationBar->HideChannelSlide();

                    ShowLnbMenuPopup(menuIdText, windowType);
                });

            const QByteArray favoriteAtKey =
                QString("%1_favorite_at").arg(menuLogText).toUtf8();

            const char* favoriteAtValue = config_get_string(
                USERCONFIG, "FavoriteMenu", favoriteAtKey.constData());

            LnbMenuItem item;
            item.menuId = menuIdText;
            item.menuLog = menuLogText;
            item.menuName = QTStr(menuId);
            item.iconDefaultPath = QString("%1/%2").arg(iconDefaultPath, iconFileNameText);
            item.iconActivePath = QString("%1/%2").arg(iconActivePath, iconFileNameText);
            item.iconDisabledPath = QString("%1/%2").arg(iconDisabledPath, iconFileNameText);
            item.action = action;
            item.isFavorite = config_get_bool(USERCONFIG, "FavoriteMenu", menuLog);
            item.favoriteAt = QDateTime::fromString(
                QString::fromUtf8(favoriteAtValue ? favoriteAtValue : ""),
                Qt::ISODateWithMs);

           lnbMenuItems.push_back(item);
        };

    addFavoriteMenu("Chat", "chat", "chat.svg", ENUM_WINDOW_TYPE::SoopChat);
    addFavoriteMenu("LiveOverlay", "overlay", "overlay.svg", ENUM_WINDOW_TYPE::SoopOverlay);
    addFavoriteMenu("SubTitle", "subtitle", "subtitle.svg", ENUM_WINDOW_TYPE::SubTitle);
    addFavoriteMenu("Mission", "mission", "mission.svg", ENUM_WINDOW_TYPE::Mission);
    addFavoriteMenu("Vote", "vote", "vote.svg", ENUM_WINDOW_TYPE::Vote);
    //addFavoriteMenu("ExtensionProgram", "extensions", "extensions.svg", ENUM_WINDOW_TYPE::Extensions);
    addFavoriteMenu("AquaRemoteControl", "aquaControl", "aquacontrol.svg", ENUM_WINDOW_TYPE::AquaControl);
    addFavoriteMenu("SaveVodNow", "savevod", "savevod.svg", ENUM_WINDOW_TYPE::None);
    addFavoriteMenu("Breaktime", "breaktime", "breaktime.svg", ENUM_WINDOW_TYPE::Breaktime);
}

LnbMenuItem* AFMainFrame::FindLnbMenuItem(const QString& menuId)
{
    for (LnbMenuItem& item : lnbMenuItems) {
        if (item.menuId == menuId)
            return &item;
    }

    return nullptr;
}

bool AFMainFrame::TriggerLnbMenu(const QString& menuId)
{
    LnbMenuItem* item = FindLnbMenuItem(menuId);
    if (!item) {
        return false;
    }

    if (!item->action) {
        return false;
    }

    item->action->trigger();
    return true;
}

void AFMainFrame::SetLnbMenuDisabled(const QString& menuId, bool disabled)
{
    for (LnbMenuItem& item : lnbMenuItems) {
        if (item.menuId != menuId)
            continue;

        item.disabled = disabled;
        return;
    }
}

bool AFMainFrame::SetFavoriteLnbMenu(const QString& menuId, bool favorite)
{
    config_t* userConfig = USERCONFIG;

    LnbMenuItem* item = FindLnbMenuItem(menuId);
    if (!item)
        return false;

    if (item->isFavorite == favorite)
        return true;

    item->isFavorite = favorite;
    item->favoriteAt = favorite ? QDateTime::currentDateTime() : QDateTime();

    const QByteArray favoriteAtKey =
        QString("%1_favorite_at").arg(item->menuLog).toUtf8();

    const QByteArray now = item->favoriteAt.toString(Qt::ISODateWithMs).toUtf8();

    config_set_bool(USERCONFIG, "FavoriteMenu", item->menuLog.toStdString().c_str(), favorite);
    config_set_string(userConfig, "FavoriteMenu", favoriteAtKey.constData(), now.constData());

    config_save_safe(userConfig, "tmp", nullptr);

    return true;
}

void AFMainFrame::RefreshFavoriteLnbMenus()
{
    if(m_leftNavigationBar)
        m_leftNavigationBar->RefreshFavoriteLnbMenuButtons();
}


void AFMainFrame::UpdateFavoirteLnbMenus(const QString& menuId)
{
    if (m_leftNavigationBar)
        m_leftNavigationBar->UpdateFavoriteLnbMenuButtons(menuId);
}

QVector<LnbMenuItem>& AFMainFrame::GetLnbMenuItems()
{
    return lnbMenuItems;
}

void AFMainFrame::ShowLnbMenuPopup(const QString& menuId, ENUM_WINDOW_TYPE type)
{
    if (!m_blockManager)
        return;

    if (type != ENUM_WINDOW_TYPE::None)
    {
        if (type == ENUM_WINDOW_TYPE::SubTitle)
        {
            AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
            QString url_ = QString::fromStdString(SOOP_SUBTITLE).arg(broadInfo->Lang().c_str());
            NavigateDefaultBrowser(url_);
        }
        else
        {
            AFQBorderPopupBaseWidget* popup = nullptr;
            if (m_blockManager->GetPopup(type, popup)) {
                popup->raise();
                return;
            }

            bool active = m_blockManager->MakePopup(type, popup);
            for (LnbMenuItem& item : lnbMenuItems) {
                if (0 == item.menuId.compare(menuId)) {
                    item.active = active;
                    break;
                }
            }
        }
    }
    else
    {
        if (0 == menuId.compare("SaveVodNow"))
        {
            if (CheckSplitVodAvailable())
                m_blockManager->ShowVodSplit(this);
            else 
                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, 
                    "", QTStr("SplitVod.Condition.Info"), false, true);
        }
    }
}