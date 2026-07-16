#pragma once

#include <deque>

#include <QMainWindow>
#include <QString>
#include <QPointer>
#include <QWidgetAction>
#include <QSystemTrayIcon>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include <QStandardPaths>
#include <QProcess>
#include <QSystemTrayIcon>
#include <chrono>

#ifdef _WIN32
#include <zlib.h>
#else
#include <cstdlib>
#endif

#include <obs-frontend-internal.hpp>

#include <future>

#include "Application/CApplication.h"
#include "DynamicCompose/CMainDynamicComposit.h"

#include "Utils/CJsonController.h"
#include "Utils/SOOPAPIHandler.h"

#include "CoreModel/UndoStack/CUndoStack.h"
#include "CoreModel/OBSOutput/CVirtualCamDef.h"

#include "ViewModel/Auth/CAuth.h"

#include "CSystemAlert.h"
#include "CMainBaseWidget.h"
#include "CMainAccountButton.h"
#include "CResourceExtension.h"
#include "CMainFrameGuide.h"

#include "UIComponent/CCustomPushbutton.h"
#include "UIComponent/CSliderFrame.h"
#include "UIComponent/CMessageBox.h"
#include "UIComponent/CTopBaseWindow.h"

#include "PopupWindows/CBasicFilters.h"
#include "PopupWindows/CBasicTransform.h"
#include "PopupWindows/CProgramInfoDialog.h"
#include "PopupWindows/SourceDialog/SOOPVodSource/VodSourceDialog/CVodSourceDialog.h"
#include "PopupWindows/SourceDialog/SOOPVodSource/TVBroadDialog/CTvBroadDialog.h"
#include "PopupWindows/SourceDialog/SOOPBrowserSource/CCefPopupDialog.h"
#include "PopupWindows/SourceDialog/SOOPVodSource/DirectBroadDialog/CDirectBroadDialog.h"
#include "PopupWindows/SourceDialog/SOOPDowoomiSource/CDowoomiDialog.h"
#include "PopupWindows/SourceDialog/SOOPDowoomiSource/CDowoomiScoreDialog.h"
#include "PopupWindows/SourceDialog/SOOPDowoomiSource/CDowoomiMoodCheckDialog.h"
#include "PopupWindows/SourceDialog/SOOPMissionSource/CMissionChallengeDialog.h"
#include "PopupWindows/SourceDialog/SOOPMissionSource/CMissionDonationRankDialog.h"
#include "PopupWindows/SourceDialog/EffectSource/CSplitEffectDialog.h"
#include "PopupWindows/SourceDialog/EffectSource/CParticleEffectDialog.h"

#include "PopupWindows/CCateChangeDialog.h"
#include "PopupWindows/CWindowCaptureAreaWidget.h"

#include "Blocks/CBlockManager.h"
#include "PopupWindows/ImportGuide/CImportGuide.h"

#include "SOOPFrontendAPI/CSOOPFrontendAPI.h"


#define MAINFRAME_UI        MAINFRAME->GetMainFrameUI()
//#define MAIN_LOADSAVE       MAINFRAME->GetMainLoadSave()
#define MAIN_PROFILE        MAINFRAME->GetMainProfile()
#define MAIN_SCENECOLLECTION MAINFRAME->GetMainSceneCollection()
#define MAIN_OUTPUT         MAINFRAME->GetMainOutput()
#define MAIN_AUDIOSOURCE    MAINFRAME->GetMainAudioSource()
#define MAIN_SCENESOURCE    MAINFRAME->GetMainSceneSource()
#define MAIN_BLOCKMANAGER   MAINFRAME->GetBlockManager()
//
#define SERVICE_MANAGER     MAINFRAME->GetServiceManager()
#define SCENE_CONTEXT       MAINFRAME->GetScene()
#define OUTPUT_CONTEXT      MAINFRAME->GetOutputContext()
#define GRAPHIC_CONTEXT     MAINFRAME->GetGraphicContext()
#define BREAKTIME_MANAGER   MAINFRAME->GetBreakTimeManager()
#define OVERLAY_MANAGER     MAINFRAME->GetOverlayManager()

#define SOOP_SRC_MANAGER    MAINFRAME->GetSoopSrcManager()
#define CEFMANAGER          MAINFRAME->GetCefManager()

#define UNDO_STACK          MAINFRAME->GetUndoStack()

#define SOOP_API_HANDLER    MAINFRAME->GetSOOPApiHandler()
#define SOOP_VOD_HANDLER    MAINFRAME->GetSOOPVodHandler()

#define MAIN_PREVIEW        DYNAMIC_COMPOSIT->GetMainPreview()

enum BROADSTART_ERROR {
    BS_NONE,
    BS_SUCCESS,
    BS_NOT_EXIST_BROADCAST_CHANNEL,
    BS_STREAM_ACTIVE,
    BS_BROADNOTICE_CANCEL,
    BS_SOOP_MEDIA_SOURCE_CATEGORY_CANCEL,
    BS_BROADSTART_API_JSON_INVALID,
    BS_ADMINBLACK_CASE,
    BS_NEED_REALNAME_AUTH,
    BS_NEED_MINOR_CERTIFY,
    BS_LIMIT_DUPLICATE_BROAD,
    BS_LIMIT_DUPLICATE_BROADING,
    BS_ALREADY_BROADCASTING,
    BS_INVALID_STREAMKEY,
    BS_INVALID_RESOLUTION,
    BS_USER_REJECT,
    BS_SUBSCRIBE_DISABLED,
    BS_SUBSCRIBE_NOTSAME,
    BS_ACCOUNT_RESIGNED,
    BS_ACCOUNT_NO_INFO,
    BS_UNDER_MAINTENANCE,
    BS_EMAIL_CERTIFY,
    BS_NEED_LOGIN,

    BS_COUNT,
};

struct LnbMenuItem
{
    QString menuId;
    QString menuLog;
    QString menuName;
    QString iconDefaultPath;
    QString iconActivePath;
    QString iconDisabledPath;

    QPointer<QAction> action;

    // favorite state
    bool isFavorite = false;
    QDateTime favoriteAt;

    bool active = false;
    bool disabled = false;
};

namespace Ui {
class AFMainFrame;
}

class JSON;

class QMovie;
class QLabel;

struct obs_scene;
struct obs_source;
struct obs_service;
struct obs_output;
struct obs_encoder;

struct os_event_data;
struct os_sem_data;
typedef struct os_event_data os_event_t;

class AFQStudioSettingDialog;
class AFQRemux;
class AFQBalloonWidget;
class AFQSourceProperties;
class AFQSceneBottomButton;
class AFQMissingFilesDialog;
class AFQColorSelect;
class AFQBrowserInteraction;
class AFQLeftNavigationBar;
class AFQExtensionLinkDialog;
class AFQExtensionExecDialog;
class AFQVideoBalloonProps;
class AFQSignaturePopup;
class AFAddStreamWidget;
class AFQStudioUpdateLogDialog;
class AFQFreecshotUninstallAlert;
class AFQSourceControlDialog;
class AFQSceneTransitionsDialog;
class AFQAudioAdvSettingDialog;

// MainFrame Separation Class
class CMainDragDrop;
class AFMainProfile;
class AFMainSceneCollection;
class CMainOutput;
class CMainAudioSource;
class CMainUpdate;
class CMainSceneSource;
class AFSceneContext;
class AFOBSOutputContext;
class AFGraphicsContext;
class BreaktimeManager;
class OverlayManager;
//
class AFServiceManager;
class SOOPMediaSourceManager;
class AFCefManager;
//
class AFBasicAuth;

typedef  QMap<QString, QPointer<AFQExtensionExecDialog>>    MAP_EXTENSION_EXEC;
typedef  QMap<OBSSource, AFQBrowserInteraction*>            MAP_BROWSER_INTERACTION;
typedef  QMap<OBSSource, AFQSourceControlDialog*>           MAP_SOURCE_CONTEXT;

struct VCamConfig;
enum VCamOutputType;

//code 0: API Error
struct BroadStartAPI_s{
    int code = 0;
    std::string broadMsg = "";
    std::string streamNo= "";
    std::string redirectURL = "";
};

struct ChatFeatureParam {
    std::string feature;
    std::string name;
    std::string nickname;
    std::string type;
    std::string term = "0";
};

struct EventTime {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
};

struct MainSignalFlags {
    bool streamingStarting = false;
    bool recordingStarted = false;
    bool isRecordingPausable = false;
    bool recordingPaused = false;

    bool restartingVCam = false;
};

class MainViewFrame : public QFrame
{
    Q_OBJECT

signals:
    void qsignalDoubleClicked();

public:
    inline explicit MainViewFrame(QWidget* parent = nullptr) {
        this->setMouseTracking(true);
        this->setAttribute(Qt::WA_Hover);
        this->installEventFilter(this);
    };
    ~MainViewFrame() {};

protected:
    inline void mouseHoverEvent(QHoverEvent* e) { unsetCursor(); };
    inline void mouseDoubleClickEvent(QMouseEvent* event) {
        QWidget::mouseDoubleClickEvent(event);
        emit qsignalDoubleClicked();
    }
};

class AFMainFrame : public AFTTopBaseWidget
{
    Q_OBJECT

    friend class AFAuth;

public:
    explicit AFMainFrame(QWidget* parent = nullptr,
                         Qt::WindowFlags flag = Qt::WindowFlags(),
                         QString updatePath = "");
    ~AFMainFrame();

    inline Ui::AFMainFrame*       GetMainFrameUI() { return ui; };
    inline AFMainDynamicComposit* GetMainWindow() const { return m_dynamicCompositMainWindow.data(); }

    //Erase After Block Init change
    inline AFQBlockManager*     GetBlockManager() const { return m_blockManager; };

    inline AFMainProfile*       GetMainProfile() const { return m_mainProfile; }
    inline AFMainSceneCollection* GetMainSceneCollection() const { return m_mainSceneCollection; }
    inline CMainOutput*         GetMainOutput() const { return m_pMainOutput; }
    inline CMainAudioSource*    GetMainAudioSource() const { return m_pMainAudioSource; }
    inline CMainSceneSource*    GetMainSceneSource() const { return m_pMainSceneSource; }

    inline AFServiceManager&    GetServiceManager() const { return *m_serviceManager; }
    inline AFSceneContext&      GetScene() const { return *m_scene; }
    inline AFOBSOutputContext&  GetOutputContext() const { return *m_outputContext; }
    inline AFGraphicsContext&   GetGraphicContext() const { return *m_graphicContext; }
    inline BreaktimeManager&    GetBreakTimeManager() const { return *m_breakTimeManager; }
    inline OverlayManager&      GetOverlayManager() const { return *m_overlayManager; }

    inline SOOPMediaSourceManager& GetSoopSrcManager() const { return *m_soopSrcManager; }
    inline AFCefManager&        GetCefManager() const { return *m_cefManager; }

    inline AFUndoStack& GetUndoStack() { return m_undo_s; }

    inline SOOPApiHandler* GetSOOPApiHandler() { return m_soopApiHandler; }

    inline void OnEvent(enum obs_frontend_event event) {
        if(api) {
            api->on_event(event);
        }
    }
    inline void OnSoopEvent(enum soop_frontend_type type, void* param) {
        if(api) {
            api->on_soop_event(type, param);
        }
    }

public slots:
    void qslotSaveProject();
    void qslotSaveProjectDeferred();
    int qslotShowImportGuide();
    int qslotShowImportRecentGuide();

    void qslotShowMigrationGuide();

    // ==================================================
    // [qslotFunc AFMainFrame UI]
    // ==================================================
;
    void qslotCheckBroadAvailable(bool BroadButtonOn);
    void qslotSetButtonOpacity();
    void qslotRemoveButtonOpacity();

    bool qslotReplayBufferClicked();
    void qslotMaximizedChanged(bool maximized);
    //void qslotPauseRecordingClicked();
    void qslotStartCountDown();
    void qslotChangeRecordState(bool checked);
    void qslotTopMenuClicked();
    void qslotPopupBlockClicked();
    void qslotProgamInfoOpenTriggered();
    void qslotMainFrameTutorial();
    void qslotTutorialPosition();
    void qslotMainFrameTutorialClose();
    void qslotGuideClosed();
    void qslotNavigateSoopServiceNoticePage();
    void qslotNavigateSoopServiceFeedBackPage();
    void qslotNavigateStreamerSuppportPage();
    void qslotNavigateSoopliveKrPage();
    void qslotShowStudioUpdatePage();
    
    void qslotPropertiesToggled(bool show);
    void qslotAlwaysOnTopToggled(bool onTop);
    void qslotUIResetTriggered();
    void qslotSceneControlTriggered(bool show);
    void qslotBlockClosedTriggered(bool used, int checkType, bool enableFavoriteMenu);
    void qslotCloseAllBlocks();

    void qslotTopMenuDestoryed();
    void qslotLoginAccountWithProperty();

    void qslotShowGlobalPageSender();

    void qslotResetCertainBroadTime();
    // ==================================================
    // [qslotFunc DPI Setting]
    // ==================================================
    void qslotScreenChanged(QScreen* screen);
    void qslotDpiChanged(qreal rel);

    // ==================================================
    // [qslotFunc Block UI]
    // ==================================================

    void qslotShowStudioSettingPopup(bool show);
    void qslotShowStudioSettingWithButtonSender();

    void qslotShowBlockWithProperty();
    void qslotShowBlock(bool visible, int type);
    void qslotInitShowSoopChat(); //Call Only on Init
    void qslotInitFreecShotPlusUpdateLog(); //Call Only on Init
    void qslotResponseFreecShotPlusUpdateLog(const QByteArray& responseData);
    void qslotShowFreecShotUnInstallAlert();
    void qslotShowDockWithProperty();
    void qslotShowDock(bool visible, int type);

    void qslotShowProjector();


    // ==================================================
    // [qslotFunc Resource Status UI]
    // ==================================================
    void qslotExtendResource();
    void qslotShowCPUSystemAlert();
    void qslotShowMemorySystemAlert();
    void qslotShowNetworkSystemAlert();
    void qslotCheckDiskSpaceRemaining();

    // ==================================================
    // [qslotFunc BraodStart Response]
    // ==================================================
    void qslotCategoryCheckAPIResponse(const QByteArray& responseData);
    void qslotBroadStartAPIResponse(const QByteArray& responseData);
    void qslotGeoBlockCheckAPIResponse(const QByteArray& responseData);
    void qslotBroadStartSuccess(bool success, BroadStartAPI_s info);
    // void qslotBroadStartAPIResponse_Reconnect(const QByteArray& responseData);
    void qslotBroadStartAPIResponse_Disconnect(const QByteArray& responseData);
    void qslotBroadStartAPIResponse_CheckStream(const QByteArray& responseData); // SOOP Simulcast broad api check

    void qslotBroadCheckTimeout();
    void qslotEmailVerify(const QByteArray& responseData);

    // ==================================================
    // [qslotFunc Output]
    // ==================================================
    void qslotReplayBufferSave();
    void qslotReplayBufferSaved();

    // virtual cam
    void qslotStartVirtualCam();
    void qslotStopVirtualCam();

    //
    void qslotScreenShot(OBSSource source = nullptr);

    // ==================================================
    // [qslotFunc Undo/Redo]
    // ==================================================
    void qSlotUndo();
    void qSlotRedo();

    // ==================================================
    // [qslotFunc Scene Source]
    // Exist Function Code in CMainFrame_SceneSource.cpp
    // ==================================================
    // Recv libobs callback slot
    void qslotAddSceneFromCallback(OBSSource scene);
    void qslotRemoveSceneFromCallback(OBSSource scene);

    // common ui slot
    void qslotTransitionScene();
    void qslotTransitionSceneTriggered();

    void qslotPasteClipboardAsSource();

    // ==================================================
    // [qslotFunc Audio UI & Audio Source]
    // Exist Function Code in CMainFrame_AudioSource.cpp
    // ==================================================
    // Recv libobs callback slot
    void qslotActivateAudioSource(OBSSource source);
    void qslotDeactivateAudioSource(OBSSource source);
    void qslotRenameSources(OBSSource source, QString newName, QString prevName);

    // mixer context menu
    void qslotStackedMixerAreaContextMenuRequested();

    // common ui slot
    void qslotVolControlContextMenu();
    void qslotHideAudioControl();
    void qslotUnhideAllAudioControls();
    void qslotLockVolumeControl(bool lock);
    void qslotMixerRenameSource();
    void qslotAudioMixerCopyFilters();
    void qslotAudioMixerPasteFilters();
    void qslotToggleVolControlLayout();
    void qslotGetAudioSourceFilters();
    void qslotGetAudioSourceProperties();
    void qslotAdvAudioPropertiesTriggered();


    // ==================================================
    // [qslotFunc Preview]
    // ==================================================
    void qslotTogglePreview();
    void qslotLockPreview();

    // ==================================================
    // [qslotFunc Auth]
    // ==================================================
    void qslotToggleMainAccount();
    void qslotRecieveBroadInfo();
    void setResolution();
    void qslotToggleLogout(bool useVideo);

    void qslotRefreshSoopCookie();

    // receive frontend-api
    void qslotSetCurrentSceneFrontendAPI(OBSSource scene, bool force);
    void qslotTransitionStudioModeScene();
    void qslotStartStreamingFrontendAPI();
    void qslotStopStreamingFrontendAPI();
    void qslotStartRecordingFrontendAPI();
    void qslotStopRecordingFrontendAPI();

    void qslotToggleMainMicFrontendAPI();
    void qslotToggleMainVolFrontendAPI();

    void qslotToggleSOOPChannelSidebarFrontendAPI();
    void qslotToggleSOOPLnbMenuFrontendAPI(int menuType);

    void qslotEventButtonClicked();
#ifdef __APPLE__
    void qslotMacSwitchToDock(int windowType, int posX, int posY);
#endif
signals:
    /* Streaming signals */
    void StreamingPreparing();
    void StreamingStarting(bool boradcastAutoStart);
    void StreamingStarted(bool withDelay = false);
    void StreamingStopping();
    void StreamingStopped(bool withDelay = false);

    /* Recording signals */
    void RecordingStarted(bool pausable = false);
    void RecordingPaused();
    void RecordingUnpaused();
    void RecordingStopping();
    void RecordingStopped();

protected slots:
    void qslotMinimizeWindow();
    

private slots:

    // ==================================================
    // [qslotFunc System Tray Icon Action]
    // ==================================================
    void qslotIconActivated(QSystemTrayIcon::ActivationReason reason);
    void qslotSetShowing(bool showing);
    void qslotToggleShowHide();
    void SystemTrayNotify(const QString& text, QSystemTrayIcon::MessageIcon n);

    // ==================================================
    // [qslotFunc Error Display] ( Not Used )
    // ==================================================
    void qslotDisplayStreamStartError();

    // ==================================================
    // [qslotFunc Output]
    // Exist Function Code in CMainFrame_Output.cpp
    // ==================================================
    // streaming
    void qslotStartStreaming();     // not call signal
    void qslotStopStreaming();      // not call signal
    void qslotForceStopStreaming(); // not call signal
    void qslotStreamDelayStarting(void* output, int sec);
    void qslotStreamDelayStopping(void* output, int sec);
    void qslotStreamingStart(void* output);
    void qslotStreamStopping(void* output);
    void qslotStreamingStop(void* output, int errorcode, QString last_error);

    // recording
    void qslotStartRecording();
    void qslotStopRecording();
    void qslotRecordingStart();
    void qslotRecordStopping();
    void qslotRecordingStop(int code, QString last_error);
    void qslotRecordingFileChanged(QString lastRecordingPath);
    //void qslotPauseRecording();
    //void qslotUnpauseRecording();

    // replaybuffer
    void qslotStartReplayBuffer();
    void qslotStopReplayBuffer();
    void qslotReplayBufferStart();
    void qslotReplayBufferStopping();
    void qslotReplayBufferStop(int code);
    //void qslotShowReplayBufferPauseWarning();

    // virtual cam
    void qslotVirtualCamStart();
    void qslotVirtualCamStop(int code);

    // ==================================================
    // [qslotFunc Network and Status]
    // ==================================================
    void qslotRefreshMainResourceText();
    void qslotResourceState(PCStatState state);

    // ==================================================
    // Profile & SceneCollection

    void qActionRemigrateSceneCollectionTriggered();

    void qslotImportPreset();
    void qslotShowMissingFiles();

    // ==================================================
    // [qslotFunc Hotkey]
    // ================================================== 
    void qslotProcessHotkey(obs_hotkey_id id, bool pressed);

    // qslotFunc Source Control Popup
    void qslotUpdateContextToolBar(bool force = false);
    void qslotClearBrowserInteractionPopup(OBSSource source);

    // [qslotFunc Split Filter]
    void qslotSplitFilterActivated();

    // [ dummy API Check ]
    void qslotDummyAPI(const QByteArray& responseData) {}

    // [ source ]
    void qslotRefreshVideoBalloonSource();
    
    // [freecshot uninstall]
    void qslotUninstallFreecshotAccept();


    void qslotCheckVersionLimit();
    void qslotGetVersionLimit(QNetworkReply *reply);

signals:
    void qsignalRefreshTimerTick();
    void qsignalToggleUseVideo(bool);
    void qsignalMainResized(QSize oldSize, QSize newSize);
    void qsignalMainShowEventTriggered();
    void firstTutorialClosedEvent();
    void qsignalBroadToggled(bool stream);
    void qsignalReplayBufferSaved();

    void qsignalCertainMinuteBroadToggled(bool broad);

    void qsignalmovedOrResized();
    void qsignalTopMenuClicked();

public:    
    bool AFMainFrameInit(bool bShow, std::string userID = "", std::string soopCookie = "", std::string Intaller_Type = "", std::string Freecshot_Type = "");
    void OnActivate(bool force = false);
    void OnDeactivate();
    void ConnectSignalForScreen();
    QSize GetContentsArea(); //preview + frame_bottom
    int GetFrameBottomPosY();

    bool IsCompleteInit() { return !m_isInitialRun; }

    static void OBSErrorMessageBox(const char* errorMsg, const char* defaultMsg, const char* errorLabel);
    static void HotkeyTriggered(void* data, obs_hotkey_id id, bool pressed);
    static void SetPCStateIconStyle(QLabel* label, PCStatState state);

    // Create & Show UI
    void RestoreMainWindow();

    bool CreateSourceProperties(obs_source_t* source, bool fromDock = false);
    bool HideSourceProperties(obs_source_t* source);
    void CreateSceneTransitionPopup(OBSSource source, int duration);
    void CreateFiltersWindow(obs_source_t* source);
    void CreateEditTransformPopup(obs_sceneitem_t* item);
    void CreateSplitEffectPopup(obs_source_t* source);
    void CreateSoopCefDetailProperties(obs_source_t* source, QWidget* parent = nullptr);
    void CreateSignatureAIPopup(bool reactionAble);
    
    bool IsSmallResolution();

    int GetLeftNavigationBarWidth();
    int GetTopAreaHeight();
    int GetBottomAreaHeight();

    void ResetDockUI();

    int BottomControlHeight();
    int LNBWidth();

    AFQSplitEffectDialog* GetSplitEffectDialog() { return m_splitEffectDialog; }

    void ShowSceneSourceSelectList();
    void ShowSystemAlert(QString alertText = "", QString channelID = "", 
                        AFQSystemAlert::AlertIcon icon = AFQSystemAlert::AlertIcon::Warning);
    void ShowMissingFilesDialog(obs_missing_files_t* files);
    void ApplyMoveArea();
    bool CheckSplitVodByUI();

	void ShowWindowCaptureArea(obs_source_t* source);
	
    // [Source Control Toolbat]

    void UpdateContextToolBarDeferred(bool force = false);

    // [ Browser Interaction Popup ]
    void ShowBrowserInteractionPopup(OBSSource source);
    void HideBrowserInteractionPopup(OBSSource source);

    // [Advance Audio Mixer Popup]
    void EnableReplayBuffer(bool enable);
    void SetReplayBufferStartStopMode(bool bufferStart);
    void SetReplayBufferStoppingMode();
    void SetReplayBufferReleased();

    // [Broad Info Dock]
    void ApplyBroadInfoToUI();
    void RefreshBroadInfoDockUI(bool requestAPI = true);
    void SplitVodSaved();

    // Call Function when deleting the source
    void RecvRemovedSource(OBSSceneItem item);

    // Account & Auth( CMainFrame_Auth.cpp )
    bool     CheckLoginSoopCookie(std::string userID, std::string cookie, bool& autoLogin);
    void     RestoreSoopAccount(std::string userId, std::string cookie, bool loginRetain = false, bool saveID = false);

    bool     LoginSoopForCookie(std::string remainID);
    int      SelectSoopAccount(std::string newAccount, std::string currentAccount);
    bool     ShowPopupPageYoutubeChannel(AFBasicAuth* pAFDataAuted);
    QPixmap* MakePixmapFromChannel(AFChannelData* channelData);
    QPixmap* MakePixmapFromAuthData(AFBasicAuth* pAFDataAuted);
    QPixmap* DownloadPixmap(std::string urlIMG);

    bool     AddStreamAccount(QWidget* parent, QString platformName = "");
    bool     LoadAccounts();
    int      CountSimulcast();
    bool     FindChannelButtonWithPlatform(std::string platform, AFMainAccountButton*& outbutton);
    void     BroadInfoTimerStart();
    void     BroadInfoTimerStop();

    void     RefreshSoopCookiTimerStart();

    void     BroadStatusCheckTimerStart();
    void     BroadStatusCheckTimerStop();
    
    QString  GetChannelID(obs_output_t* output);

    void     LogoutMainAccount(bool tokenExpired = false);

    // For BroadCasting
    int      PrepareBroadStart();
    bool     BroadCastEnd(bool banStop = false);
    bool     CheckSoopBroadStatus(const QByteArray& responseData);

    void SetMainStreaming(bool main) { m_isMainStreaming = main; }
    bool IsMainStreaming() { return m_isMainStreaming; }
    void OffBroadStartAPICheck();

    void ShutDown();

    // ===============================
    // Preview UI
    // ===============================
    bool IsPreviewProgramMode();
    void EnablePreviewDisplay(bool enable);
    bool GetPreviewEnable() { return m_previewEnabled; }
    void SetStudioModeStatus(bool studioMode);

    // =================================================
    // [Scene Source]
    // Exist Function Code in CMainFrame_SceneSource.cpp
    // ==================================================
    void RefreshSceneUI();
    void SetCurrentScene(OBSSource scene, bool force = false);
    void ClearSceneData(bool init = false);
    void RefreshVideoBalloonSource();

    // =================================================
    // [Audio UI]
    // Exist Function Code in CMainFrame_AudioSource.cpp
    // ==================================================
    void ToggleMixerLayout(bool vertical);

    // =================================================
    // [System Tray]
    // Exist Function Code in CMainFrame_SceneSource.cpp
    // ==================================================

    // [ Scene Transition ]
    void        EnableTransitionState(bool enable);
    void        EnableTransitionWidgets();

#ifdef _WIN32
    void        AddRegStartProcessWindows();
    void        UpdaterKill();
#endif

    //void ToggleVisibleBottomLayerForDock(bool show);

    void SystemTray(bool firstStarted);
    void SystemTrayInit();
    void RestoreGeometry(bool firstRun);

    // [Call HotKey]
    bool EnableStartStreaming();
    bool EnableStopStreaming();
    bool EnableStartRecording();
    bool EnableStopRecording();
    bool EnablePauseRecording();
    bool EnableUnPauseRecording();

    void ChangeStreamStateUI(bool enable, bool checked, QString title, int width);
    void ChangeRecordStateUI(bool enable, bool checked, QString title, int width);
    bool CheckSplitVodAvailable();
    void ToggleBroadTimerUI(bool start);
    QString GetBroadTimerUITime();
    void StartStreaming();
    void StopStreaming();
    void ForceStopStreaming();
    void StartRecording();
    void StopRecording();
    //void PauseRecording();
    //void UnPauseRecording();
    void StartReplayBuffer();
    void StopReplayBuffer();
    //
    obs_output_t* GetVirtualCamOutput();
    VCamConfig& VirtualCamConfig() { return m_vcamConfig; }
    void SetVirtualCamOutputType(const VCamOutputType type);
    void UpdateVirtualCamConfig(const VCamConfig& config);
    void RestartVirtualCam(const VCamConfig& config);
    void RestartingVirtualCam();
    void EnablePreview();
    void DisablePreview();
    void EnablePreviewProgam();
    void DiablePreviewProgam();

    // virtual cam
    bool VirtualCamEnabled() { return m_vcamEnabled; }

    void VerifyEmailBeforeBroad(std::string failMsg, std::string url);
    void NavigateDefaultBrowser(QString Url);

    // Custom Browser
    void ReloadCustomBrowserMenu();
    AFQCustomMenu* CreateCustomBrowserMenu();

    //
    AFQCustomMenu* CreateFullScreenProjectorMenu();
    template <typename Receiver, typename... Args>
    void AddProjectorMenuMonitors(QMenu* parent, Receiver* target, void (Receiver::* slot)(Args...));
    QList<QString> GetProjectorMenuMonitorsFormatted();

    void CustomBrowserStateChanged(QString uuid, bool isOpen);
    void CustomBrowserStateAllChanged(bool isOpen);

    void SetSceneCollectionEnabled(bool enable);

    void SetDisplayAffinity(QWindow* window);

    void ResetStudioModeUI(bool changeLayout);

    void ShowPresetGuide();    

    // for undo/redo
    void _RegisterUndoRedoShortCut();
    static OBSData BackupScene(obs_scene_t* scene,
                               std::vector<obs_source_t*>* sources = nullptr);
    static inline OBSData BackupScene(obs_source_t* sceneSource,
                                      std::vector<obs_source_t*>* sources = nullptr)
    {
        obs_scene_t* scene = obs_scene_from_source(sceneSource);
        return BackupScene(scene, sources);
    }
    void CreateSceneUndoRedoAction(const QString& action_name,
                                   OBSData undo_data, OBSData redo_data);
    void CreateFilterPasteUndoRedoAction(const QString& text,
                                         obs_source_t* source,
                                         obs_data_array_t* undo_array,
                                         obs_data_array_t* redo_array);

    // [Settings]
    void IsRestartConfirmationNeeded(bool needConfirm) { m_restartWidthoutConfirm = needConfirm; }

    // [freecshot uninstall]
    void ShowUninstallFreecShotAlert();
    bool IsInEventPeriod(const EventTime& start, const EventTime& end) const;

protected:
    virtual void closeEvent(QCloseEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;
    virtual void moveEvent(QMoveEvent* event) override;
    virtual void dragEnterEvent(QDragEnterEvent* event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent* event) override;
    virtual void dragMoveEvent(QDragMoveEvent* event) override;
    virtual void dropEvent(QDropEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;

    //void changeWidgetBorder(bool isMaximized) override;

private:
    // ================================
    // [ UI ]
    // ==================================
    void _ConnectStatisticsSignals();
    void _SetMainFrameUI();

    void _CreateTopMenu();
    void _ToggleTopMenu();
    AFQCustomMenu* _FindSubMenuByTitle(AFQCustomMenu* menu, const QString& name);

    void _RegisterSourceControlAction();

    void _ShowSettingPopup(int tabPage = 0);
    void _ShowSettingPopupWithID(QString id, QString platform);

    void _MoveSystemAlert(AFQSystemAlert* systemAlert, bool isMainMinimized = false);

    void _RestartApp();

    void _AccountButtonStreamingToggle(bool stream);

    // ============================================
    // [ Resource and System Checking ]
    // ============================================
    void _SetResourceCheckTimer(int time = 2000);
    bool _LowDiskSpace();
    void _DiskSpaceMessage();
    bool _OutputPathValid();
    void _OutputPathInvalidMessage();

    template<typename SlotFunc>
    void _connectAndAddAction(QAction* sender, const typename QtPrivate::FunctionPointer<SlotFunc>::Object* receiver, SlotFunc slot, bool addActionToMain = true);

    //==================================
    // [ Guide ] - Preset
    //==================================
    void _AddBroadPreset();
    void _AddBroadPresetSource(const char* sourceId, float width, float height, float posX, float posY, obs_bounds_type boundType);
    void _AddSceneDefaultName();

    void _DeleteBroadInfoData();

public:
    std::string m_freecshotType;
    std::string m_installType;

    // SOOP Frontend API
    soop_frontend_callbacks* api = nullptr;

private:
    // User Interface
    Ui::AFMainFrame* ui;
    //
    QScreen*                        m_pCurrentScreen = nullptr;
    QMovie*                         m_pBroadMovie = nullptr;
    QPointer<QObject>               m_shortcutFilter;
    //QScopedPointer<QThread>         m_devicePropertiesThread; 
    QPointer<QTimer>                m_broadStartTimer;
    QPointer<QTimer>                m_resourceRefreshTimer;
    QPointer<QTimer>                m_receiveBroadInfoTimer;
    QPointer<QTimer>                m_refreshSoopCookieTimer;
    QPointer<QTimer>                m_refreshVideoBallonTimer;
    QPointer<QTimer>                m_BroadStatusCheckTimer;
    QPointer<QTimer>                m_CheckBroadStartAPITimer;

    // Manager
    QPointer<AFQBlockManager>        m_blockManager;
    QNetworkAccessManager*           m_pNetworkManager;

    QPointer<SOOPApiHandler>        m_soopApiHandler;

    
    // DynamicComposit( Dock, Main Preview ) 
    QPointer<AFMainDynamicComposit>     m_dynamicCompositMainWindow;

    // SOOPStudio Popup  
    QPointer<AFQStudioSettingDialog>    m_studioSettingPopup;
    QPointer<AFQCefPopupDialog>         m_cefPopupProperties;
    QPointer<AFQBasicTransform>         m_transformPopup;
    QPointer<AFQBasicFilters>           m_sourceFilters;
    QPointer<AFQSceneTransitionsDialog> m_sceneTransitionPopup;
    QPointer<AFResourceExtension>       m_resourceExtensionWidget;
    QPointer<AFQProgramInfoDialog>      m_programInfoDialog;

    QPointer<AFQMissingFilesDialog>         m_missDialog;
    QPointer<AFQAudioAdvSettingDialog>      m_advAudioSettingPopup = nullptr;
    QPointer<QWidget>                       m_mainGuideWidget;
    QPointer<AFQLeftNavigationBar>          m_leftNavigationBar;
    QPointer<AFQSplitEffectDialog>          m_splitEffectDialog;
    QPointer<AFQSignaturePopup>             m_signatureAIPopup;
    QPointer<WindowCaptureAreaWidget>       m_WindowCaptureAreaWidget;
    QPointer<AFQStudioUpdateLogDialog>      m_updateLogPopup;
    QPointer<AFQFreecshotUninstallAlert>    m_uninstallFreecshotAlert;
    
    QPointer<QWidget>                   m_mainTutorialWidget;
    QPointer<AFMainFrameGuide>          m_mainTutorialContents;

    QPointer<AFAddStreamWidget>         m_AddStreamWidget;

    MAP_BROWSER_INTERACTION             m_mapBrowserInteraction;

    // SOOP Source Props
    QPointer<AFQSourceProperties> m_sourceProperties;
    QPointer<AFQCefPopupDialog>   m_soopCefDetailProperties;
    std::unordered_map<std::string, QPointer<QDialog>> m_sourcePropsPtr;

    //
    QPointer<AFQSystemAlert> m_systemAlert;

    // SOOPStudio Menus
    QPointer<AFQCustomMenu> m_topMenu;
    QPointer<AFQCustomMenu> m_addonMenu;
    QPointer<AFQCustomMenu> m_trayMenu;
    QPointer<AFQCustomMenu> m_previewProjector;
    QPointer<AFQCustomMenu> m_studioProgramProjector;
    QPointer<QAction> m_loginAction;

    QMetaObject::Connection m_SetPassword;

    // SOOPStudio UI Component
    AFMainAccountButton* m_pCurrentAccountButton = nullptr;

    bool m_bFirstOpen = false;
    bool m_isInitialRun = true;

    // SOOPStudio Status
    bool m_topMenuTriggered = false;
    bool m_clearingFailed = false;
    bool m_previewEnabled = true;
    bool m_broadcastReady = false; //YOUTUBE
    bool m_logOut = false;
    bool m_checkBroadStartAPI = false;

    // [Settings]
    bool m_restartWidthoutConfirm = false;
    bool m_normalInit = true;

    // [ Scene Transition ]
    bool m_transitionWidgetEnabled = true;

    bool m_isMainStreaming = true;
    
    /* `undo_s` needs to be declared after `ui` to prevent an uninitialized
     * warning for `ui` while initializing `undo_s`. */
    AFUndoStack m_undo_s;

    std::unique_ptr<AFServiceManager>   m_serviceManager;
    std::unique_ptr<AFSceneContext>     m_scene;
    std::unique_ptr<AFOBSOutputContext> m_outputContext;
    std::unique_ptr<AFGraphicsContext>  m_graphicContext;
    std::unique_ptr<BreaktimeManager>   m_breakTimeManager;
    std::unique_ptr<OverlayManager>     m_overlayManager;

    std::unique_ptr<SOOPMediaSourceManager> m_soopSrcManager;
    std::unique_ptr<AFCefManager>       m_cefManager;
    //AFProfileUtil m_profileUtil;

    // [ DragDrop ]
    CMainDragDrop* m_pMainDragDrop = nullptr;

    // [ Profile ]
    AFMainProfile* m_mainProfile = nullptr;
    AFMainSceneCollection* m_mainSceneCollection = nullptr;

    // [ Guide ] - Preset
    QVector<QPair<QString, QRectF>> m_presetSourcesGeometry;

    // [ Output ] - Streaming, Recording, ReplayBuffer, StatusBarTemp ...
    CMainOutput* m_pMainOutput = nullptr;

    // [ Output ] - virtual cam
    bool m_vcamEnabled = false;
    VCamConfig m_vcamConfig;
    bool m_restartingVCam = false;

    // [ Audio Source ] - MainFrame Audio UI ...
    CMainAudioSource* m_pMainAudioSource = nullptr;

    // [ Scene & Source ]
    CMainSceneSource* m_pMainSceneSource = nullptr;

    // [ Resource & System Checking ]

    // [ Updater Logic ]
    CMainUpdate* m_pMainUpdate = nullptr;
    QNetworkAccessManager* m_VersionLimit = nullptr;
    ///////////////////////////////////////////
    // [ Not Used ]
    // ///////////////////////////////////////
    // System Tray
    QScopedPointer<QSystemTrayIcon> m_trayIcon;
    QPointer<QAction> m_systemTrayStreamAction;
    QPointer<QAction> m_systemTrayRecordAction;
    QPointer<QAction> m_systemTrayReplayBufferAction;
    QPointer<QAction> m_mainShowHideAction;
    QPointer<QAction> m_exitAction;

    MainSignalFlags m_signalFlags;


    // 
public: 
    void InitLnbMenuItems();

    QVector<LnbMenuItem>& GetLnbMenuItems();
    LnbMenuItem* FindLnbMenuItem(const QString& menuId);
    bool TriggerLnbMenu(const QString& menuId);

    void SetLnbMenuDisabled(const QString& menuId, bool disabled);
    bool SetFavoriteLnbMenu(const QString& menuId, bool favorite);
    void RefreshFavoriteLnbMenus();
    void UpdateFavoirteLnbMenus(const QString& menuId);

public slots:
    void ShowLnbMenuPopup(const QString& menuId, ENUM_WINDOW_TYPE type);
    
public:
    QVector<LnbMenuItem> lnbMenuItems;
};