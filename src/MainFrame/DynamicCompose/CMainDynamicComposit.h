#pragma once


#include <QMainWindow>
#include <QPushButton>
#include <QTimer>
#include <QPushButton>
#include <QPointer>
#include <QMetaEnum>

#include <vector>

#include "CMainWindow.h"
#include "Blocks/CDockTitle.h"
#include "MainFrame/CBlockPopup.h"

#include <obs.h>
#include <obs.hpp>
#include <util/platform.h>

#include "CoreModel/Video/CVideo.h"
#include "CoreModel/Audio/CAudio.h"
#include "CoreModel/Encoder/CEncoder.h"


#include "PopupWindows/CSceneTransitionsDialog.h"
#include "PopupWindows/SourceDialog/CSourceControlDialog.h"
#include "Blocks/AudioMixerDock/CAudioAdvSettingWidget.h"
#include "PopupWindows/CustomBrowser/CCustomBrowserCollection.h"
#include "UIComponent/CBorderPopupBaseWidget.h"

namespace Ui {
class AFMainDynamicComposit;
}

// Forward
class AFGraphicsContext;
class AFMainWindowAccesser;


// Qt UI Class Forward
class AFBasicPreview;
class AFQProgramView;
class AFQVerticalProgramView;
class AFSceneSourceDockWidget;
class AFAudioMixerDockWidget;
class AFSceneControlDockWidget;
class AFBroadInfoDockWidget;
class AFAdvanceControlsDockWidget;
class AFQVolControl;

class AFQBrowserInteraction;


#define ENUM_BLOCK_TYPE             AFMainDynamicComposit::BlockType

typedef  QMap<OBSSource, AFQBrowserInteraction*>    MAP_BROWSER_INTERACTION;
typedef  QMap<OBSSource, AFQSourceControlDialog*>   MAP_SOURCE_CONTEXT;

class AFMainDynamicComposit : public AFQMainWindow
{

#pragma region QT Field
    Q_OBJECT

public:
    explicit AFMainDynamicComposit(QWidget* parent = nullptr);
    ~AFMainDynamicComposit();

    //Bookmarks
    enum BlockType {
        SceneSource = 0,
        AudioMixer,
        StudioMode,
        Chat,
        Dashboard,
        Settings,
        Control,
        SceneControl,
        AdvanceControls,
        NewsFeed,
    };
    Q_ENUM(BlockType)

public slots:

    void qslotToggleLockTriggered();
    void qslotChangeDockToWindow(bool toDock, const char* blocktype);
    void qslotCloseBlock(const char* blocktype);
    void qslotMaximumBlock(const char* blocktype);
    void qslotMinimumBlock(const char* blocktype);

    void qslotText();

    void qslotToggleStudioModeBlock(bool enable);
    void qslotChangeSceneOnDoubleClick();

    // [Recv Scene Source Dock Title]
    void qslotAddSceneTriggered();
    void qslotAddSourceTriggered();
    void qslotTransitionSceneTriggered();
    void qslotShowSceneControlDockTriggered();
    void qslotStackedMixerAreaContextMenuRequested();
    void qslotCloseCustomBrowser(QString uuid);
    void qslotClosedCustomBrowser(QString uuid);
    void qslotMaximizeBrowser(QString uuid);
    void qslotMinimizeBrowser(QString uuid);

    // [ Scene Preview ]
    void qslotShowPreview();


    // OBS Callbacks Signal
    static void SourceCreated(void* data, calldata_t* params);
    static void SourceRemoved(void* data, calldata_t* params);
    static void SourceActivated(void* data, calldata_t* params);
    static void SourceDeactivated(void* data, calldata_t* params);
    static void SourceAudioActivated(void* data, calldata_t* params);
    static void SourceAudioDeactivated(void* data, calldata_t* params);
    static void SourceRenamed(void* data, calldata_t* params);

    // [Source Control Dialog]
    void ShowOrUpdateContextPopup(OBSSource source, bool force = false);
    void UpdateContextPopup(bool force = false);
    void UpdateContextPopupDeferred(bool force = false);
    void HideSourceContextPopupFromRemoveSource(OBSSource source);
    void ClearAllSourceContextPopup();

    void ShowBrowserInteractionPopup(OBSSource source);
    void ClearBrowserInteractionPopup(OBSSource source);
    void HideBrowserInteractionPopupFromRemoveSource(OBSSource source);

    void ShowSceneTransitionPopup(OBSSource source, int duration);

    void qslotSetMainAudioSource();

private slots:
    void qSlotShowContextMenu(const QPoint& pos);

    // For Audio Mixer
    void qslotActivateAudioSource(OBSSource source);
    void qslotDeactivateAudioSource(OBSSource source);
    void qslotHideAudioControl();
    void qslotUnhideAllAudioControls();
    void qslotLockVolumeControl(bool lock);
    void qslotMixerRenameSource();
    void qslotAdvAudioPropertiesTriggered();
    void qslotAudioMixerCopyFilters();
    void qslotAudioMixerPasteFilters();
    void qslotGetAudioSourceFilters();
    void qslotGetAudioSourceProperties();
    //

    void AddScene(OBSSource scene);
    void RemoveScene(OBSSource source);
    void TransitionStopped();
    void qslotRenameSources(OBSSource source, QString newName, QString prevName);

    void qslotOpenCustomBrowserCollection();
    void qslotOpenCustomBrowser(bool checked);
    //void qslotDockLocationChanged(Qt::DockWidgetArea area);

signals:
    void qsignalShowPreview();
    void qsignalShowBlock();
    void qsignalBlockVisibleToggled(bool, int);
    void qsignalSelectSourcePopup();
#pragma endregion QT Field


#pragma region public func
public:
    bool MainWindowInit() override;
    void CreateObsDisplay(QWidget* previewWidget);
    void InitDock();
    void ReleaseDock();
    void LoadDockPos();
    void RaiseBlocks();
    void SwitchToDock(const char* key, bool isVisible);
    void ToggleDockVisible(BlockType type, bool show);

    void LoadCustomBrowserList();
    void ReloadCustomBrowserList(
        QVector<AFQCustomBrowserCollection::CustomBrowserInfo>
            info,
        QList<QString> deletedUuid);
    void CreateCustomBrowserBlocks();
    void OpenCustomBrowserBlock(AFQCustomBrowserCollection::CustomBrowserInfo info, int overlapCount = 0);
    void SaveCustomBrowserList();
    AFQCustomMenu* CreateCustomBrowserMenu();

    AFBasicPreview* GetMainPreview();
    QFrame*         GetNotPreviewFrame();

    static AFMainDynamicComposit* Get();

    inline AFSceneSourceDockWidget* GetSceneSourceDock() const { return m_qSceneSourceDock; }
    inline AFVideoUtil* GetVideoUtil() { return &videoUtil; }

    inline AFQProgramView*          GetStudioModeViewLayout() { return m_pStudioModeView; };
    inline AFQVerticalProgramView*  GetVerticalStudioModeViewLayout() { return m_pStudioModeVerticalView; };
    
    void InitAFCallbacks();

    void SetDockedValues();
    // For Audio Mixer
    void UpdateVolumeControlsDecayRate();
    void UpdateVolumeControlsPeakMeterType();
    void RefreshVolumeColors();
    //
    void MainAudioVolumeChanged(int volume);
    void MainMicVolumeChanged(int volume);
    void SetMainAudioMute();
    void SetMainMicMute();

    // For Scene Source
    // [Scene Source]
    void CreateDefaultScene(bool firstStart);
    void AddDefaultGuideImage();
    void SetCurrentScene(OBSSource scene, bool force = false);
    void SetCurrentScene(obs_scene_t* scene, bool force);
    void RefreshSourceBorderColor();
    void UpdateSceneNameStudioMode(bool vertical);

    void EnableReplayBuffer(bool enable);
    void SetReplayBufferStartStopMode(bool bufferStart);
    void SetReplayBufferStoppingMode();
    void SetReplayBufferReleased();

    void UpdatePreviewSafeAreas();
    void UpdatePreviewSpacingHelpers();
    void UpdatePreviewOverflowSettings();

    void SwitchStudioModeLayout(bool vertical);
    void ToggleStudioModeLabels(bool vertical, bool visible);

    bool IsMainAudioVolumeControlExist();
    bool IsMainMicVolumeControlExist();
    float GetMainAudioVolumePeak();
    float GetMainMicVolumePeak();

    void AdjustPositionOutSideScreen(QRect windowGeometry, QRect& outAdjustGeometry);
#pragma endregion public func

#pragma region protected func
protected:
    virtual void closeEvent(QCloseEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
    void _CreateSourceDock();
    void _CreateAudioDock();
    void _CreateSceneControlDock();
    void _CreateAdvanceControlsDock();

    void _ClearBrowserInterationQMap();
    void _ClearDockMap();
    void _ClearPopupMap();
    void _ClearBlockMap();

    AFQVolControl* _CompareAudio(AFQVolControl* audio1, AFQVolControl* audio2);

    void _RestorePopupBlock(const char* key);

    AFQCustomMenu* CreateAddSourcePopupMenu();

    // void CreateFirstTestAudioSource();

    // For Audio Mixer
    bool _SourceMixerHidden(obs_source_t* source);
    void _SetSourceMixerHidden(obs_source_t* source, bool hidden);
    //

    void ClearVolumeControls();
    void ToggleMixerLayout(bool vertical);
    void ToggleVolControlLayout();
    void VolControlContextMenu();

    void _MakePopupForDock(const char* key, bool resetPosition = true);
    void _ClosePopup(const char* key, bool isVisible);
    void _RaiseBlock(const char* key);
    void _ToggleVisibleBlockWindow(int blocktype, bool show);
    void _CloseAllWindows();

    bool _IsOutSideScreen(QRect windowGeometry);

    bool _FindDock(const char* dockType, QDockWidget*& outDock) const;
    bool _FindDock(int dockType, QDockWidget*& outDock) const;
    bool _FindPopup(const char* dockType, QWidget*& outPopup) const;
    bool _FindPopup(int dockType, QWidget*& outPopup) const;
    bool _FindBlock(const char* dockType, QPair<QWidget*, QWidget*>& outBlock) const;
    bool _FindBlock(int dockType, QPair<QWidget*, QWidget*>& outBlock) const;

    bool _CheckDockedCount();

    void _MakeVerticalStudioMode();
    void _MakeHorizontalStudioMode();
    void _DestroyVerticalStudioMode();
    void _DestroyHorizontalStudioMode();
    
    void                        _InitMainDisplay(AFMainWindowAccesser* viewModels);
    void                        _InitProgramDisplay(AFMainWindowAccesser* viewModels);
#pragma endregion private func

#pragma region public member var
public:
    AFQCustomMenu* m_dockMenu;
    bool animationing = false;

#pragma endregion public member var

#pragma region private member var
private:
    Ui::AFMainDynamicComposit* ui;

    AFSceneSourceDockWidget*                        m_qSceneSourceDock = nullptr;
    AFAudioMixerDockWidget*                         m_qAudioMixerDockWidget = nullptr;
    AFSceneControlDockWidget*                       m_qSceneControlDock = nullptr;
    AFAdvanceControlsDockWidget*                    m_qAFAdvanceControlsDockWidget = nullptr;
    QMap<QString, AFQBorderPopupBaseWidget*>        m_qBrowserWidgetMap;
    QPointer<AFQCustomBrowserCollection>            m_qBrowserCollection = nullptr;

    AFBroadInfoDockWidget*                          m_qBroadInfoDock;
    QDockWidget*                                    m_qChatDock;

    QMap<const char*, QDockWidget*>                 m_qDockMap;
    QMap<const char*, QWidget*>                     m_qPopupMap;
    QMap<const char*, QPair<QWidget*, QWidget*>>    m_qBlockMap;
    QPointer<AFQAudioAdvSettingDialog>                           m_qAdvAudioSettingPopup = nullptr;

    QVector<AFQCustomBrowserCollection::CustomBrowserInfo>
        m_qCustomBrowserVector;


    // [source-control-dialog]
    //QPointer<AFQSourceControlDialog>                m_qSourceContextPopup = nullptr;
    QPointer<AFQSceneTransitionsDialog>             m_qSceneTransitionPopup = nullptr;

    MAP_BROWSER_INTERACTION m_mapBrowserInteraction;
    MAP_SOURCE_CONTEXT      m_mapSourceContextPopup;

    QByteArray                                      startingDockLayout;


    AFQVolControl*                                  m_MainAudioVolumeControl = nullptr;
    AFQVolControl*                                  m_MainMicVolumeControl = nullptr;

    bool                                            stopHover = false;


    std::vector<OBSSignal>                          signalHandlers;
    std::vector<AFQVolControl*>                     volumes;
    OBSWeakSourceAutoRelease                        m_mixerCopyFiltersSource = nullptr;

    // Test AudioMixer Value
    //bool                                          verticalAudioMixer = false;
    
    AFQProgramView*                                 m_pStudioModeView = nullptr;
    AFQVerticalProgramView*                         m_pStudioModeVerticalView = nullptr;


    AFVideoUtil     videoUtil;
    AFAudioUtil     audioUtil;
    AFEncoderUtil   encoderUtil;
#pragma endregion private member var
};
