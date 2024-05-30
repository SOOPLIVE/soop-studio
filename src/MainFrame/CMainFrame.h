#pragma once

#include <deque>

#include <QMainWindow>
#include <QString>
#include <QPointer>
#include <QWidgetAction>
#include <QSystemTrayIcon>
#include <QUrl>
#include <QDebug>
#include <QStandardPaths>
#include <QProcess>



#include "CMainBaseWidget.h"
#include "CMainAccountButton.h"
#include "CResourceExtension.h"
#include "CSystemAlert.h"
#include "UIComponent/CColorSelect.h"
#include "UIComponent/CCustomPushbutton.h"

#include "PopupWindows/CBasicFilters.h"
#include "PopupWindows/CBasicTransform.h"
#include "PopupWindows/CGuideWidget.h"
#include "PopupWindows/CStatFrame.h"
#include "PopupWindows/CProgramInfoDialog.h"

#include "DynamicCompose/CMainDynamicComposit.h"

#include "Utils/CStatusLogManager.h"



#include <obs-frontend-internal.hpp>
#include "ViewModel/Auth/CAuth.h"

#include "Utils/CJsonController.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/OBSOutput/SBasicOutputHandler.h"
#include "CoreModel/Video/CVideo.h"
#include "CoreModel/Source/CSource.h"
#include "CoreModel/UndoStack/CUndoStack.h"

#include "CStatusbarTemp.h"

namespace Ui {
class AFMainFrame;
}

// Forward
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

class AFVideoUtil;
class AFQStudioSettingDialog;
class AFQSliderFrame;
class AFQStatWidget;
class AFQRemux;
class AFQBalloonWidget;
class AFQSourceProperties;
class AFQSceneBottomButton;
class AFQMissingFilesDialog;

class JSON;

typedef std::vector<std::pair<obs_service_t*, std::unique_ptr<AFBasicOutputHandler>>>   OUTPUT_HANDLER_LIST;

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

class AFMainFrame : public AFCQMainBaseWidget
{
#pragma region QT Field
    Q_OBJECT

    friend class AFAuth;

public:
    explicit AFMainFrame(QWidget* parent = nullptr, Qt::WindowFlags flag = Qt::FramelessWindowHint);
    ~AFMainFrame();

    void initializeWithArguments();
    bool m_bNoUpdate = true;
public slots:
    void qslotSaveProject();
    void qslotSaveProjectDeferred();

    void qslotChangeBroadState(bool BroadButtonOn);
    void qslotEnterBroadButton();
    void qslotLeaveBroadButton();
    bool qslotReplayBufferClicked();
    void qslotPauseRecordingClicked();
    void qslotStartCountDown();
    void qslotUpdateCountDown();
    void qslotChangeRecordState(bool checked);
    void qslotTopMenuClicked();
    void qslotPopupBlockClicked();
    void qslotStatsOpenTriggered();
    void qslotProgramGuideOpenTriggered();
    void qslotProgamInfoOpenTriggered();
    void qslotGuideTriggered(int guideNum);
    void qslotGuideClosed();
    void qslotStatsCloseTriggered();
    void qslotStatsMouseLeave(); 
    void qslotFocusChanged(QWidget* old, QWidget* now);

    //test

    //Dpi Setting
    void qslotSetDpiHundred();
    void qslotSetDpiHundredFifty();
    void qslotSetDpiTwoHundred();
    //Dpi Setting

    void qslotCloseAllPopup();

    void qslotToggleBlockArea(bool show);
    void qslotToggleBlockAreaByToggleButton();

    void qslotPreviewResizeTriggered();

    void qslotToggleBlockFromBlockArea(bool show, int key);
    void qslotToggleBlockFromMenu(bool show);
    void qslotShowStudioSettingPopup(bool show);
    void qslotShowStudioSettingWithButtonSender();
    void qslotBlockToggleTriggered(bool show, int blockType);

    //void qslotBlockStopTimer();
    //void qslotLeaveBlockAreaOrLockButton();
    //Block

    void qslotShowCPUSystemAlert();
    void qslotShowMemorySystemAlert();
    void qslotShowNetworkSystemAlert();
    void qslotCheckDiskSpaceRemaining();

    void qslotScreenChanged(QScreen* screen);
    void qslotDpiChanged(qreal rel);

    void qslotShowVolumeSlider();
    void qslotShowMicSlider();
    void qSlotCloseVolumeSlider();
    void qSlotCloseMicSlider();
    void qslotSetAudioPeakValue();
    void qslotSetMicPeakValue();
    void qslotStopVolumeTimer();
    void qslotStopMicTimer();
    void qslotMainAudioValueChanged(int volume);
    void qslotMainMicValueChanged(int volume);
    void qslotSetVolumeMute();
    void qslotSetMicMute();

    void qslotTopMenuDestoryed();
    void qslotShowMainAuthMenu();
    void qslotShowOtherAuthMenu();
    void qslotShowLoginMenu();
    void qslotStopSimulcast(bool checked);
    void qslotLoginAccount();

    bool qslotShowGlobalPage(QString type);
    void qslotShowGlobalPageSender();
    void qslotLogoutGlobalPage();

    void qslotCloseGlobalSoopPage(QString type);
    void qslotHideGlobalSoopPage(QString type);
    void qslotMaximizeGlobalSoopPage(QString type);
    void qslotMinimizeGlobalSoopPage(QString type);

    void qslotExtendResource();

    void qslotReplayBufferSave();
    void qslotReplayBufferSaved();

    // for scene source
    void qslotAddSourceMenu();
    void qslotShowSelectSourcePopup();
    void qslotSceneButtonClicked(OBSScene scene);
    void qslotSceneButtonDoubleClicked(OBSScene scene);
    void qslotSceneButtonDotClicked();

    void qSlotActionResetTransform();

    void Screenshot(OBSSource source = nullptr);
    void qslotTransitionScene();


    void qSlotUndo();
    void qSlotRedo();

protected slots:
    void qslotMinimizeWindow();
    
private slots:
    void qslotIconActivated(QSystemTrayIcon::ActivationReason reason);
    void qslotSetShowing(bool showing);
    void qslotToggleShowHide();
    void qslotDisplayStreamStartError();

    // stat
    void qslotRefreshNetworkText();
    void qslotNetworkState(PCStatState state);

    // streaming
    void qslotStartStreaming();
    void qslotStopStreaming();
    void qslotForceStopStreaming();

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

    void qslotShowReplayBufferPauseWarning();
    void qslotStartReplayBuffer();
    void qslotStopReplayBuffer();

    void qslotReplayBufferStart();
    void qslotReplayBufferStopping();
    void qslotReplayBufferStop(int code);

    void qslotPauseRecording();
    void qslotUnpauseRecording();

    // For Source Context Menu
    void qslotTogglePreview();
    void qslotLockPreview();
    
    void qslotActionScaleWindow();
    void qslotActionScaleCanvas();
    void qslotActionScaleOutput();

    void qSlotActionCopySource();
    void qSlotActionPasteRefSource();
    void qSlotActionPasteDupSource();

    void qSlotActionRenameSource();
    void qSlotActionRemoveSource();

    void qSlotActionEditTransform();
    void qSlotActionCopyTransform();
    void qSlotActionPasteTransform();
    void qSlotActionRotate90CW();
    void qSlotActionRotate90CCW();
    void qSlotActionRotate180();
    void qSlotFlipHorizontal();
    void qSlotFlipVertical();

    void qSlotFitToScreen();
    void qSlotStretchToScreen();
    void qSlotCenterToScreen();
    void qSlotVerticalCenter();
    void qSlotHorizontalCenter();

    void qSlotActionShowInteractionPopup();
    void qSlotActionShowProperties();

    void qSlotOpenSourceFilters();
    void qSlotCopySourceFilters();
    void qSlotPasteSourceFilters();

    void qSlotSourceListItemColorChange();
    void qSlotResizeOutputSizeOfSource();

    void qSlotSetScaleFilter();
    void qSlotBlendingMethod();
    void qSlotBlendingMode();
    void qSlotSetDeinterlaceingMode();
    void qSlotSetDeinterlacingOrder();

    void qslotProcessHotkey(obs_hotkey_id id, bool pressed);

    
    void qSlotChangeProfile();
    void qSlotNewProfile();
    void qSlotDupProfile();
    void qSlotDeleteProfile(bool skipConfirmation);
    void qSlotRenameProfile();
    void qSlotExportProfile();
    void qSlotImportProfile();
    void qSlotNewSceneCollection();
    void qSlotDupSceneCollection();
    void qSlotRenameSceneCollection();
    void qSlotDeleteSceneCollection();
    void qSlotImportSceneCollection();
    void qSlotExportSceneCollection();
    void qSlotShowMissingFiles();

signals:
    void qsignalRefreshTimerTick();
    void qsignalToggleUseVideo(bool);
#pragma endregion QT Field


#pragma region public func
public:
    void AFMainFrameInit(bool bShow);
    void SetButtons();
    void OnActivate(bool force = false);
    void OnDeactivate();
    inline AFMainDynamicComposit* GetMainWindow() const { return m_DynamicCompositMainWindow.data(); }

    void CreatePropertiesPopup(obs_source_t* source);
    void CreatePropertiesWindow(obs_source_t* source);
    void RecvRemovedSource(OBSSceneItem item);
    void ShowSceneSourceSelectList();
    void ShowSystemAlert(QString channelID = "", QString alertText = "");

    inline AFAuth* GetAuth() { return m_auth.get(); }
    inline void ResetAuth() { m_auth.reset(); }

    inline void EnableOutputs(bool enable)
    {
        if(enable) {
            if(--m_disableOutputsRef < 0)
                m_disableOutputsRef = 0;
        } else {
            m_disableOutputsRef++;
        }
    }

    // for output
    static void OBSStreamStarting(void* data, calldata_t* params);
    static void OBSStreamStopping(void* data, calldata_t* params);
    static void OBSStartStreaming(void* data, calldata_t* /* params */);
    static void OBSStopStreaming(void* data, calldata_t* params);
    static void OBSStartRecording(void* data, calldata_t* /* params */);
    static void OBSStopRecording(void* data, calldata_t* params);
    static void OBSRecordStopping(void* data, calldata_t* /* params */);
    static void OBSRecordFileChanged(void* data, calldata_t* params);
    static void OBSStartReplayBuffer(void* data, calldata_t* /* params */);
    static void OBSStopReplayBuffer(void* data, calldata_t* params);
    static void OBSReplayBufferStopping(void* data, calldata_t* /* params */);
    static void OBSReplayBufferSaved(void* data, calldata_t* /* params */);
    void        ResetOutputs();

    void        SetStreamingOutput();
    bool        PrepareStreamingOutput(int index, obs_service_t* service);
    bool        StartStreamingOutput(obs_service_t* service);
    bool        IsStartStreamingOutput(obs_service_t* service);
    bool        StopStreamingOutput(obs_service_t* service);
    void        SetOutputHandler();
    bool        LoadAccounts();
    
    QString GetChannelID(obs_output_t* output);
    //
    static void OBSErrorMessageBox(const char* errorMsg, const char* defaultMsg, const char* errorLabel);

    // preview
    void EnablePreviewDisplay(bool enable);
    bool GetPreviewEnable() { return m_bPreviewEnabled; }

    // scene source
    void        RefreshSceneUI();
    void        CreateSourcePopupMenu(int idx, bool preview = false);
    void        AddSourceMenuButton(const char* id, QWidget* popup);
    AFQCustomMenu*      CreateAddSourcePopupMenu();
    AFQCustomMenu*      AddBackgroundColorMenu(AFQCustomMenu* menu,
                                       QWidgetAction* widgetAction,
                                       AFQColorSelect* select,
                                       obs_sceneitem_t* item);
    void        CreateFiltersWindow(obs_source_t* source);
    void        CreateEditTransformPopup(obs_sceneitem_t* item);
    void        SetSceneBottomButtonStyleSheet(OBSSource scene);
    QAction*    GetRemoveSourceAction();

    void        UpdateEditMenu();
  

    void        ClearSceneData(bool init = false);
    void        ClearSceneBottomButtons();


    //void ToggleVisibleBottomLayerForDock(bool show);

    void SetAudioButtonEnabled(bool enabled);
    void SetMicButtonEnabled(bool enabled);
    void SetAudioVolume(int volume);
    void SetMicVolume(int volume);
    void SetAudioSliderEnabled(bool Enabled);
    void SetMicSliderEnabled(bool Enabled);
    void SetAudioVolumeSliderSignalsBlock(bool block);
    void SetMicVolumeSliderSignalsBlock(bool block);
    void SetAudioMeter(float peak);
    void SetMicMeter(float peak);
    void SetAudioButtonMute(bool bMute);
    void SetAudioSliderMute(bool bMute);
    void SetMicButtonMute(bool bMute);
    void SetMicSliderMute(bool bMute);
    bool IsAudioMuted();
    bool IsMicMuted();
    void SystemTray(bool firstStarted);
    void SystemTrayInit();

    static void HotkeyTriggered(void* data, obs_hotkey_id id, bool pressed);

    static void SetPCStateIconStyle(QLabel* label, PCStatState state);

    //
    bool IsActive();
    bool IsStreamActive();
    bool GetStreamingCheck() { return os_atomic_load_bool(&m_streaming_active); };
    bool IsReplayBufferActive();
    bool IsRecordingActive();
    bool IsPreviewProgramMode();
    bool EnableStartStreaming();
    bool EnableStopStreaming();
    bool EnableStartRecording();
    bool EnableStopRecording();
    bool EnablePauseRecording();
    bool EnableUnPauseRecording();

    void StartStreaming();
    void StopStreaming();
    void ForceStopStreaming();
    void StartRecording();
    void StopRecording();
    void PauseRecording();
    void UnPauseRecording();
    void StartReplayBuffer();
    void StopReplayBuffer();
    void EnablePreview();
    void DisablePreview();
    void EnablePreviewProgam();
    void DiablePreviewProgam();

    void RecvHotkeyTransition();
    void RecvHotkeyResetStats();
    void RecvHotkeyScreenShotSelectedSource();

    void ReloadCustomBrowserMenu();
    void CustomBrowserClosed(QString uuid);

    void SetDisplayAffinity(QWindow* window);

    void ResetStudioModeUI(bool changeLayout);

    // Load/Save
    void            RefreshProfiles();
    bool            CreateProfile(const std::string &newName, bool create_new,
                                  bool showWizardChecked, bool rename = false);
    bool            AddProfile(bool create_new, const char *title, const char *text,
                               const char *init_text = nullptr, bool rename = false);
    void            DeleteProfile(const char* profileName, const char* profileDir);
    
    void            WaitDevicePropertiesThread();
    bool            AddSceneCollection(bool create_new,
                                       const QString& qname = QString());
    void            RefreshSceneCollections();
    void            ShowMissingFilesDialog(obs_missing_files_t* files);

    // for undo/redo
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
    //
    
    // Auth
    bool            ShowPopupPageYoutubeChannel(AFBasicAuth* pAFDataAuted);
    QPixmap*        MakePixmapFromAuthData(AFBasicAuth* pAFDataAuted);
    //
#pragma endregion public func

#pragma region protected func
protected:
    virtual void closeEvent(QCloseEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;
    virtual void moveEvent(QMoveEvent* event) override;
    void changeWidgetBorder(bool isMaximized) override;
#pragma endregion protected func

#pragma region private func
private:
    void _SetupBlock();
    void _MoveBlocks();
    bool _SetupChatPage();
    bool _SetupNewsfeedPage();
    void _MoveSystemAlert(AFQSystemAlert* systemAlert, bool isMainMinimized = false);
    void _MoveProgramGuide();
    void _PopupRequest(bool show, int type);
    void _CreateTopMenu();
    void _ToggleTopMenu();
    void _AccountButtonStreamingToggle(bool stream);
    AFQCustomMenu* _FindSubMenuByTitle(AFQCustomMenu* menu, const QString& name);

    void _ShowBlockArea(int animateSpeed = 300);
    void _HideBlockArea(int animateSpeed = 300);

    void _StopRecording(bool bRecord = false);
    void SetupAutoRemux(const char*& container);
    std::string GetRecordingFilename(
        const char* path, const char* container, bool noSpace, bool overwrite,
        const char* format, bool ffmpeg);

    void _ShowSettingPopup(int tabPage = 0);
    void _ShowSettingPopupWithID(QString id);
    QString _AccountButtonsStyling(bool stream);

    void _SetupAccountUI();
    
    void EnumDialogs();
    bool _OutputPathValid();
    void _OutputPathInvalidMessage();

    void _AutoRemux(QString input, bool no_show = false);

    void _UpdatePause(bool activate = true);
    void _UpdateReplayBuffer(bool activate = true);

    void _SetResourceCheckTimer(int time = 2000);
    bool _LowDiskSpace();
    void _DiskSpaceMessage();

    void _ChangeStreamState(bool enable, bool checked, QString title, int width);
    void _ToggleBroadTimer(bool start);
    void _ChangeRecordState(bool check = false);

    void _RegisterSourceControlAction();

    // Load/Save
    bool            _CopyProfile(const char* fromPartial, const char* to);
    void            _CheckForSimpleModeX264Fallback();
    bool            _ProfileNeedsRestart(config_t* newConfig, QString& settings);
    void            _ResetProfileData();
    bool            _FindSafeProfileDirName(const std::string &profileName,
                                            std::string &dirName);
    bool            _AskForProfileName(QWidget *parent, std::string &name,
                                       const char *title, const char *text,
                                       const bool showWizard, bool &wizardChecked,
                                       const char *oldName = nullptr);
    
    bool            _GetSceneCollectionName(QWidget* parent, std::string& name,
                                            std::string& file,
                                            const char* oldName = nullptr);
    void            _ChangeSceneCollection();
    //
    
    template<typename SlotFunc>
    void _connectAndAddAction(QAction* sender, const typename QtPrivate::FunctionPointer<SlotFunc>::Object* receiver, SlotFunc slot);

    QColor _GetSourceListBackgroundColor(int preset);

    //int _ResetVideo();

    //void _GetFPSCommon(uint32_t& num, uint32_t& den) const;
    //void _GetConfigFPS(uint32_t& num, uint32_t& den) const;
    //void _GetFPSInteger(uint32_t& num, uint32_t& den) const;
    //void _GetFPSFraction(uint32_t& num, uint32_t& den) const;
    //void ResizePreview(uint32_t cx, uint32_t cy);

    obs_frontend_callbacks* _InitializeAPIInterface(AFMainFrame* main);


    // Use Context Source Menu
    AFQCustomMenu* _AddScaleFilteringMenu(AFQCustomMenu* menu, obs_sceneitem_t* item);
    AFQCustomMenu* _AddBlendingModeMenu(AFQCustomMenu* menu, obs_sceneitem_t* item);
    AFQCustomMenu* _AddBlendingMethodMenu(AFQCustomMenu* menu, obs_sceneitem_t* item);
    AFQCustomMenu* _AddDeinterlacingMenu(AFQCustomMenu* menu, obs_source_t* source);
    
    enum DropType {
        DropType_RawText,
        DropType_Text,
        DropType_Image,
        DropType_Media,
        DropType_Html,
        DropType_Url,
    };

    void _AddDropSource(const char* file, DropType image);
    void _AddDropURL(const char* url, QString& name, obs_data_t* settings,
                     const obs_video_info& ovi);
    void _ConfirmDropUrl(const QString& url);

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

    void _ClearAllStreamSignals();
#pragma endregion private func

#pragma region public member var
public:
    /* `undo_s` needs to be declared after `ui` to prevent an uninitialized
     * warning for `ui` while initializing `undo_s`. */
    AFUndoStack m_undo_s;

#pragma region public member var

#pragma region private member var
private:
    Ui::AFMainFrame* ui;

    
    QScopedPointer<QThread>             m_pDevicePropertiesThread;
    
    QMovie* m_qBroadMovie = nullptr;
    QMovie* m_qBroadHoverMovie = nullptr;
    
    QPointer<AFMainDynamicComposit>  m_DynamicCompositMainWindow;
    QPointer<AFQStudioSettingDialog> m_StudioSettingPopup;
    QPointer<AFQSourceProperties>    m_SourceProperties;
    QPointer<AFQBasicTransform>      m_transformPopup;
    QPointer<AFQBasicFilters>        m_dialogFilters;
    QPointer<AFResourceExtension>    m_ResourceExtensionWidget;
    QPointer<AFQProgramInfoDialog>   m_programInfoDialog;
    QPointer<AFVideoUtil> m_VideoUtil;
    QPointer<AFAudioUtil> m_AudioUtil;

    std::vector<AFQSceneBottomButton*>  m_vSceneButton;

    QScopedPointer<QSystemTrayIcon> m_TrayIcon;
    QPointer<QAction> m_qSystemTrayStreamAction;
    QPointer<QAction> m_qSystemTrayRecordAction;
    QPointer<QAction> m_qSystemTrayReplayBufferAction;
    //QPointer<QAction> sysTrayVirtualCam;
    QPointer<QAction> m_qMainShowHideAction;
    QPointer<QAction> m_qExitAction;
    QPointer<AFQCustomMenu> m_qTopMenu;
    QPointer<AFQCustomMenu> m_qTrayMenu;
    QPointer<AFQCustomMenu> m_qPreviewProjector;
    QPointer<AFQCustomMenu> m_qStudioProgramProjector;

    QPointer<AFQProgramGuideWidget> m_ProgramGuideWidget;
    QPointer<QWidget> m_MainGuideWidget;
    //QPointer<AFQSystemAlert> m_SystemAlert;
    QPointer<AFQBlockPopup> m_BlockPopup;
    QPointer<AFQSliderFrame> m_VolumeSliderFrame;
    QPointer<AFQSliderFrame> m_MicSliderFrame;
    QPointer<AFQStatWidget> m_StatFrame;
    QPointer<AFQRemux> m_remux;
    //QPointer<QWidget> m_qBroadCountDownWidget;
    QPointer<QLabel> m_qBroadLabel;

    QPointer<AFQBorderPopupBaseWidget> m_GlobalSoopChatWidget;
    QPointer<AFQBorderPopupBaseWidget> m_GlobalSoopNewsfeedWidget;

    QPointer<QObject> m_shortcutFilter;

    QPointer<QTimer> m_VolumeTimer;
    QPointer<QTimer> m_Mictimer;
    QPointer<QTimer> m_AudioPeakUpdateTimer;
    QPointer<QTimer> m_MicPeakUpdateTimer;
    QPointer<QTimer> m_BroadStartTimer;
    QPointer<QTimer> m_ResourceRefreshTimer;

    QSize m_BlockPopupSize = QSize();

    bool m_bBlockAnimating = false;
    bool m_bIsBlockPopup = true;

    bool m_bTopMenuTriggered = false;

    bool m_bClosing = false;
    bool m_bClearingFailed = false;
    bool m_bPreviewEnabled = true;

    int m_streamingOuputRef = 0;

    int m_disableOutputsRef = 0;
    int m_iBroadTimerRemaining = 0;

    bool m_hasCopiedTransform = false;

    obs_frontend_callbacks* api = nullptr;

    std::shared_ptr<AFAuth> m_auth;
    AFMainAccountButton* m_qCurrentAccountButton = nullptr;
    //
    OBSService m_service;
    OUTPUT_HANDLER_LIST m_outputHandlers;
    bool m_streamingStopping = false;
    bool m_recordingStopping = false;
    bool m_replayBufferStopping = false;

    volatile bool m_streaming_active = false;
    volatile bool m_recording_active = false;
    volatile bool m_recording_paused = false;
    volatile bool m_replaybuf_active = false;

    OBSOutputAutoRelease fileOutput;
    OBSOutputAutoRelease streamOutput;
    OBSEncoder videoRecording;
    OBSEncoder videoStreaming;
    OBSEncoder recordTrack[6];
    OBSEncoder streamAudioEnc;
    OBSEncoder streamArchiveEnc;
    
    QString m_serverUpdateTime;
    QString m_updateTimeFilePath;
    QString m_localUpdateHashStr;
    QString m_serverUpdatehash;
    QString m_localAppdataPath;

    bool ffmpegOutput = false;
    bool ffmpegRecording = false;
    bool useStreamEncoder = false;
    bool useStreamAudioEncoder = false;
    bool usesBitrate = false;

    std::vector<int> fallbackBitrates;
    std::map<std::string, std::vector<int>> encoderBitrates;

    std::string lastRecordingPath;
 
    // Source Popup Menu
    std::deque<SourceCopyInfo>  m_clipboard;

    QPointer<QWidgetAction>     m_widgetActionColor;
    QPointer<AFQColorSelect>    m_widgetColorSelect;

    QPointer<AFQCustomMenu>             m_menuScaleFiltering;
    QPointer<AFQCustomMenu>             m_menuBlendingMode;
    QPointer<AFQCustomMenu>             m_menuBlendingMethodMode;
    QPointer<AFQCustomMenu>             m_menuDeinterlace;

    QPointer<QObject>           m_ScreenshotData;
    
    
    // Load/Save
    QPointer<AFQMissingFilesDialog>   m_MissDialog;
    //
    
    QWidget* chatWidget = nullptr;
    QScreen* currentScreen = nullptr;

    // temp
    AFStatusbarTemp m_statusbar;
    
#pragma endregion private member var

};

extern void undo_redo(const std::string& data);
