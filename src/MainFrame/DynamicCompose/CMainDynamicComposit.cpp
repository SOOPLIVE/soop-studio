#include "CMainDynamicComposit.h"
#include "ui_aneta-main-window.h"
#include "ui_audio-mixer-dock.h"

#include <QString>
#include <QCloseEvent>
#include <QWindow>
#include <QScreen>
#include <qobjectdefs.h>
#include <QGraphicsDropShadowEffect>

#include <json11.hpp>

#include "platform/platform.hpp"
#include <util/profiler.hpp>
///
//#include "obs-internal.h"
#include "graphics/graphics.h"
///
#include "Blocks/SceneSourceDock/CSceneSourceDockWidget.h"
#include "Blocks/SceneControlDock/CSceneControlDockWidget.h"
#include "Blocks/AudioMixerDock/CAudioMixerDockWidget.h"
#include "Blocks/AudioMixerDock/CVolumeControl.h"

#include "Application/CApplication.h"
#include "UIComponent/CNameDialog.h"
#include "UIComponent/CCustomMenu.h"
#include "PopupWindows/CBrowserInteractionDialog.h"
#include "Utils/CJsonController.h"
#include "Common/SettingsMiscDef.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Browser/CCefManager.h"
#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Log/CLogManager.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"
#include "CoreModel/Icon/CIconContext.h"

#include "ViewModel/MainWindow/CMainWindowAccesser.h"
#include "ViewModel/MainWindow/CMainWindowRenderModel.h"
#include "UIComponent/CMessageBox.h"

#include "PopupWindows/SourceDialog/CSourceControlDialog.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/MainPreview/CProgramViewHorizontal.h"
#include "MainFrame/MainPreview/CProgramViewVertical.h"
#include "MainFrame/SceneSource/CMainSceneSource.h"

#include "MainFrame/SourceToolbar/CImageSourceToolbar.h"
#include "MainFrame/SourceToolbar/CTextSourceToolbar.h"
#include "MainFrame/SourceToolbar/CVodSourceToolbar.h"
#include "MainFrame/SourceToolbar/CDeviceSourceToolbar.h"
#include "MainFrame/SourceToolbar/CBrowserSourceToolbar.h"
#include "MainFrame/SourceToolbar/CMonitorSourceToolbar.h"
#include "MainFrame/SourceToolbar/CAudioSourceToolbar.h"
#include "MainFrame/SourceToolbar/CColorSourceToolbar.h"
#include "MainFrame/SourceToolbar/CMediaSourceToolbar.h"
#include "MainFrame/SourceToolbar/CMediaListSourceToolbar.h"
#include "MainFrame/SourceToolbar/CWindowSourceToolbar.h"
#include "MainFrame/SourceToolbar/CAreaSourceToolbar.h"
#include "MainFrame/SourceToolbar/CDirectBroadSourceToolbar.h"
#include "MainFrame/SourceToolbar/CTvLiveSourceToolbar.h"
#include "MainFrame/SourceToolbar/CAquaSourceToolbar.h"
#include "MainFrame/SourceToolbar/CAquaScoreSourceToolbar.h"
#include "MainFrame/SourceToolbar/CSpoutSourceToolbar.h"
#include "MainFrame/SourceToolbar/CSlideShowSourceToolbar.h"
#include "MainFrame/SourceToolbar/CCommerceSourceToolbar.h"
#include "MainFrame/SourceToolbar/CKBOSourceToolbar.h"
#include "MainFrame/SourceToolbar/CVideoBalloonSourceToolbar.h"
#include "MainFrame/SourceToolbar/CGameSourceToolbar.h"
#include "MainFrame/SourceToolbar/CPainterSourceToolbar.h"
#include "MainFrame/SourceToolbar/CAIManagerToolbar.h"

#define QT_UTF8(str) QString::fromUtf8(str, -1)

void setupDockAction(QDockWidget* dock)
{
    QAction* action = dock->toggleViewAction();

    auto neverDisable = [action]() {
        QSignalBlocker block(action);
        action->setEnabled(true);
        };

    auto newToggleView = [dock](bool check) {
        QSignalBlocker block(dock);
        dock->setVisible(check);
        };

    // Replace the slot connected by default
    action->disconnect(SIGNAL(triggered(bool)));
    dock->connect(action, &QAction::triggered, newToggleView);

    // Make the action unable to be disabled
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    action->connect(action, &QAction::changed, neverDisable);
#else
    action->connect(action, &QAction::enabledChanged, neverDisable);
#endif
}

AFMainDynamicComposit::AFMainDynamicComposit(QWidget* parent)
    : OBSMainWindow(parent),
    ui(new Ui::AFMainDynamicComposit)
{
    ui->setupUi(this);

    ui->contextSourceIcon->hide();
    ui->sourcePropertiesButton->hide();
    ui->sourceInteractButton->hide();

    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NativeWindow);
    //setAttribute(Qt::WA_DontCreateNativeAncestors);
    setDockOptions(QMainWindow::AllowTabbedDocks | QMainWindow::AllowNestedDocks | QMainWindow::AnimatedDocks);

    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);

    g_viewModelsDynamic.GetInstance().m_renderModel.SetDPIValue(devicePixelRatioF());
   
    ui->preview->InitSetNugeEventAction();

    connect(ui->enablePreviewButton, &QPushButton::clicked, this, &AFMainDynamicComposit::ShowPreview);
    connect(ui->sourcePropertiesButton, &QPushButton::clicked, this, &AFMainDynamicComposit::ShowToolbarSourceProps);
    connect(ui->sourceInteractButton, &QPushButton::clicked, this, &AFMainDynamicComposit::ShowBrowserInteraction);
}

AFMainDynamicComposit::~AFMainDynamicComposit()
{
    delete ui;
}

void AFMainDynamicComposit::ToggleStudioModeBlock(bool enable)
{
    auto& stateApp = STATEAPP;
    if (stateApp.IsPreviewProgramMode() == enable)
        return;
            
    stateApp.SetPreviewProgramMode(enable);

    bool studioPortraitLayout = config_get_bool(USERCONFIG, "BasicWindow", "StudioPortraitLayout");
    bool studioLabel = config_get_bool(USERCONFIG, "BasicWindow", "StudioModeLabels");
    
    MAINFRAME->SetStudioModeStatus(enable);

    if(stateApp.IsPreviewProgramMode())
    {       
        if (!MAINFRAME->GetPreviewEnable())
            MAINFRAME->EnablePreviewDisplay(true);

        if (studioPortraitLayout)
            MakeVerticalStudioMode();
        else
            MakeHorizontalStudioMode();

        ToggleStudioModeLabels(studioPortraitLayout, studioLabel);

        MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED);
    }
    else
    {
        if (!MAINFRAME->GetPreviewEnable())
            MAINFRAME->EnablePreviewDisplay(false);

        if (studioPortraitLayout)
            DestroyVerticalStudioMode();
        else
            DestroyHorizontalStudioMode();

        MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED);
    }
}

void AFMainDynamicComposit::ChangeSceneOnDoubleClick()
{
    //Double Click Transition Hide
    return;
    //Double Click Transition Hide

    bool doubleClickSwitch = config_get_bool(USERCONFIG, "BasicWindow", "TransitionOnDoubleClick");
    if (doubleClickSwitch)
    {
        if (STATEAPP.IsPreviewProgramMode() == false) 
            return;

        bool studioPortraitLayout = config_get_bool(USERCONFIG, "BasicWindow", "StudioPortraitLayout");
        if (studioPortraitLayout) {        
            studioModeVerticalView->qslotTransitionTriggered();
        } else {
           studioModeView->qslotTransitionTriggered();
        }
    }
}

void AFMainDynamicComposit::ShowPreview()
{
    emit PreviewShowRequested();
}

void AFMainDynamicComposit::closeEvent(QCloseEvent* event)
{
    AFMainWindowAccesser* tmpViewModels = nullptr;
    tmpViewModels = g_viewModelsDynamic.UnSafeGetInstace();
    
    if (tmpViewModels)
        tmpViewModels->m_renderModel.RemoveCallbackMainDisplay();
    
    GRAPHIC_CONTEXT.FinContext();
    //

    SCENE_CONTEXT.ClearSourceSignalCallback();
}

bool AFMainDynamicComposit::MainWindowInit()
{
    ProfileScope("AFMainDynamicComposit::MainWindowInit");

    CreateObsDisplay(ui->preview);

    auto& argOption = ARGOPTION;
    if (argOption.GetStudioMode() == false)
    {
        bool previewMode = config_get_bool(USERCONFIG, "BasicWindow", "PreviewProgramMode");
        ToggleStudioModeBlock(previewMode);
    }
    else
    {
        STATEAPP.SetPreviewProgramMode(true);
        argOption.SetStudioMode(false);
    }

    config_set_default_bool(USERCONFIG, "BasicWindow", "SideDocks", true);
    config_save_safe(USERCONFIG, "tmp", nullptr);


    UpdatePreviewSafeAreas();
    UpdatePreviewSpacingHelpers();
    UpdatePreviewOverflowSettings();

    return true;
}

void AFMainDynamicComposit::CreateObsDisplay(QWidget* previewWidget)
{
    AFMainWindowAccesser* tmpViewModels = nullptr;
    tmpViewModels = g_viewModelsDynamic.UnSafeGetInstace();
    
    if (tmpViewModels)
        tmpViewModels->m_renderModel.SetMainPreview(ui->preview);
    
    InitMainDisplay(tmpViewModels);
}

CBasicPreview* AFMainDynamicComposit::GetMainPreview()
{
    return ui->preview;
}

int AFMainDynamicComposit::GetMainPreviewYPos()
{
    int mainDisplayYPosSize = ui->preview->y() + ui->preview->height();

    if (STATEAPP.IsPreviewProgramMode())
    {
        bool studioPortraitLayout = config_get_bool(USERCONFIG, "BasicWindow", "StudioPortraitLayout");
        if (studioPortraitLayout)
        {
            if (studioModeVerticalView)
                mainDisplayYPosSize = studioModeVerticalView->y() + studioModeVerticalView->height();
        }
        else
        {
            if (studioModeView)
                mainDisplayYPosSize = studioModeView->y() + studioModeView->height();
        }
    }

    return mainDisplayYPosSize;
}

QFrame* AFMainDynamicComposit::GetNotPreviewFrame()
{
    return ui->previewDisabledWidget;
}

void AFMainDynamicComposit::TransitionStopped()
{
    OBSWeakSource swapScene = SCENE_CONTEXT.GetSwapScene();
    if (swapScene) {
        OBSSource scene = OBSGetStrongRef(swapScene);
        if (scene)
            SetCurrentScene(scene);
    }

    MAINFRAME->EnableTransitionState(true);
    //EnableTransitionWidgets(true);
    //UpdatePreviewProgramIndicators();

    MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
    MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);

    SCENE_CONTEXT.SetSwapScene(nullptr);
}

void AFMainDynamicComposit::UpdateVideoCaptureSplitAcitved()
{
    AFQSplitEffectDialog* pSplitEffectDailog = MAINFRAME->GetSplitEffectDialog();
    if (pSplitEffectDailog && pSplitEffectDailog->isVisible()) {
        pSplitEffectDailog->RefreshSplitFilterUI();
    }
}

void AFMainDynamicComposit::UpdateSourceToolbar(QString dataType)
{
    UNUSED_PARAMETER(dataType);
    UpdateSourceToolBar(true);
}

void AFMainDynamicComposit::SetSideDock(bool side)
{
    isSideDock = side;
    config_set_bool(USERCONFIG, "BasicWindow", "SideDocks", isSideDock);

    if (isSideDock) {
        setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
    }
    else {
        setCorner(Qt::TopLeftCorner, Qt::TopDockWidgetArea);
        setCorner(Qt::TopRightCorner, Qt::TopDockWidgetArea);
        setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
    }
    CheckDocksState();
}

void AFMainDynamicComposit::RenameSource(QString name)
{
    const int elidedWidth = 150;
    QFontMetrics fontMetrics(ui->contextSourceLabel->font());
    QString strElidedText = fontMetrics.elidedText(name, Qt::ElideRight, elidedWidth);
    ui->contextSourceLabel->setText(strElidedText);
}

void AFMainDynamicComposit::ShowToolbarSourceProps()
{
    QWidget* outBlock = nullptr;
    if (!MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::SceneSource, outBlock))
        return;

    AFSceneSourceWidget* scenesourceBlock = reinterpret_cast<AFSceneSourceWidget*>(outBlock);
    if (!scenesourceBlock)
        return;

    OBSSceneItem item = scenesourceBlock->GetCurrentSceneItem();
    OBSSource source = obs_sceneitem_get_source(item);

    const char* id = obs_source_get_id(source);

    if (AFSourceUtil::ShouldShowProperties(source))
        MAINFRAME->CreateSourceProperties(source);
}

void AFMainDynamicComposit::ShowBrowserInteraction()
{
    const auto item = SCENE_CONTEXT.GetCurrentOBSSceneItem();
    if (!item)
        return;

    OBSSource source = obs_sceneitem_get_source(item);

    if (source)
        MAINFRAME->ShowBrowserInteractionPopup(source);
}

void AFMainDynamicComposit::SetCurrentScene(obs_scene_t* scene, bool force)
{
    obs_source_t* source = obs_scene_get_source(scene);
    SetCurrentScene(source, force);
}

void AFMainDynamicComposit::RefreshSourceBorderColor()
{
    GetMainPreview()->SetSourceBorderColor();
}

void AFMainDynamicComposit::UpdateSceneNameStudioMode(bool vertical)
{
    auto& sceneContext = SCENE_CONTEXT;
    if (vertical)
    {
        AFQVerticalProgramView* studioVerticalModeView = GetVerticalStudioModeViewLayout();
        if (studioVerticalModeView != nullptr)
        {
            OBSSource previewSrc = AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrentScene());
            OBSSource programSrc = OBSGetStrongRef(sceneContext.GetProgramScene());

            studioVerticalModeView->ChangeEditSceneName(QT_UTF8(obs_source_get_name(previewSrc)));
            studioVerticalModeView->ChangeLiveSceneName(QT_UTF8(obs_source_get_name(programSrc)));
        }
    }
    else
    {
        AFQProgramView* studioModeView = GetStudioModeViewLayout();
        if (studioModeView != nullptr)
        {
            OBSSource previewSrc = AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrentScene());
            OBSSource programSrc = OBSGetStrongRef(sceneContext.GetProgramScene());

            studioModeView->ChangeEditSceneName(QT_UTF8(obs_source_get_name(previewSrc)));
            studioModeView->ChangeLiveSceneName(QT_UTF8(obs_source_get_name(programSrc)));
        }
    }
    //
}

void AFMainDynamicComposit::UpdatePreviewSafeAreas()
{
    bool drawSafeAreas = config_get_bool(USERCONFIG, "BasicWindow", "ShowSafeAreas");
    ui->preview->SetShowSafeAreas(drawSafeAreas);
}

void AFMainDynamicComposit::UpdatePreviewSpacingHelpers()
{
    bool drawSpacingHelpers = config_get_bool(USERCONFIG, "BasicWindow", "SpacingHelpersEnabled");
    ui->preview->SetDrawSpacingHelpers(drawSpacingHelpers);
}

void AFMainDynamicComposit::UpdatePreviewOverflowSettings()
{
    bool hidden = config_get_bool(USERCONFIG, "BasicWindow", "OverflowHidden");
    bool select = config_get_bool(USERCONFIG, "BasicWindow", "OverflowSelectionHidden");
    bool always = config_get_bool(USERCONFIG, "BasicWindow", "OverflowAlwaysVisible");

    ui->preview->SetOverflowHidden(hidden);
    ui->preview->SetOverflowSelectionHidden(select);
    ui->preview->SetOverflowAlwaysVisible(always);
}

void AFMainDynamicComposit::SwitchStudioModeLayout(bool vertical)
{
    if (STATEAPP.IsPreviewProgramMode() == false)
        return;

    if (vertical)
    {
        DestroyHorizontalStudioMode();
        MakeVerticalStudioMode();
    }
    else
    {
        DestroyVerticalStudioMode();
        MakeHorizontalStudioMode();
    }
}

void AFMainDynamicComposit::ToggleStudioModeLabels(bool vertical, bool visible)
{
    if (STATEAPP.IsPreviewProgramMode() == false)
        return;

    if (vertical)
    {
        studioModeVerticalView->ToggleSceneLabel(visible);
    }
    else
    {
        studioModeView->ToggleSceneLabel(visible);
    }
}

void AFMainDynamicComposit::SceneControlStudioModeSignal(AFSceneControlWidget* sceneControl, bool connectSignal)
{
    bool studioPortraitLayout = config_get_bool(USERCONFIG, "BasicWindow", "StudioPortraitLayout");
    if (studioPortraitLayout)
    {
        disconnect(sceneControl,
            &AFSceneControlWidget::qSignalTransitionButtonClicked,
            studioModeVerticalView, &AFQVerticalProgramView::qslotTransitionTriggered);

        if (connectSignal)
        {
            connect(sceneControl,
                &AFSceneControlWidget::qSignalTransitionButtonClicked,
                studioModeVerticalView, &AFQVerticalProgramView::qslotTransitionTriggered);
        }
    }
    else
    {
        disconnect(sceneControl,
            &AFSceneControlWidget::qSignalTransitionButtonClicked,
            studioModeView, &AFQProgramView::qslotTransitionTriggered);

        if (connectSignal)
        {
            connect(sceneControl,
                &AFSceneControlWidget::qSignalTransitionButtonClicked,
                studioModeView, &AFQProgramView::qslotTransitionTriggered);
        }
    }
}

void AFMainDynamicComposit::TransitionStudioModeScene()
{
    bool studioPortraitLayout = config_get_bool(USERCONFIG, "BasicWindow", "StudioPortraitLayout");
    if (studioPortraitLayout)
    {
        QMetaObject::invokeMethod(studioModeVerticalView, "qslotTransitionTriggered");
    }
    else
    {
        QMetaObject::invokeMethod(studioModeView, "qslotTransitionTriggered");
    }
}

void AFMainDynamicComposit::UpdateContextBarVisibility()
{
    int width = ui->centralwidget->size().width();

    ContextBarSize contextBarSizeNew;
    if (width >= 740) {
        contextBarSizeNew = ContextBarSize_Normal;
    }
    else if (width >= 600) {
        contextBarSizeNew = ContextBarSize_Reduced;
    }
    else {
        contextBarSizeNew = ContextBarSize_Minimized;
    }

    if (contextBarSize == contextBarSizeNew)
        return;

    contextBarSize = contextBarSizeNew;
    UpdateSourceToolBar(true);
}

void AFMainDynamicComposit::SetVisibleSourceToolBar(bool visible)
{
    ui->contextContainer->setVisible(visible);
    UpdateSourceToolBar(true);
    CheckDocksState();
}

void AFMainDynamicComposit::UpdateSourceToolBar(bool force)
{
    obs_source_t* source = nullptr;

    QWidget* outBlock = nullptr;
    if (!MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::SceneSource, outBlock))
        return;

    AFSceneSourceWidget* scenesourceBlock = reinterpret_cast<AFSceneSourceWidget*>(outBlock);
    if (!scenesourceBlock)
        return;

    scenesourceBlock->SourceToolBarButtonSetEnable();

    ui->sourceInteractButton->hide();

    SOOP_SRC_MANAGER.SetSoopMediaSourceToolBar(nullptr);


    CBaseSourceToolbar* panel = nullptr;
    OBSSceneItem item = scenesourceBlock->GetCurrentSceneItem();
    if (item) {
        source = obs_sceneitem_get_source(item);

        bool updateNeeded = true;

        const char* id = obs_source_get_unversioned_id(source);
        uint32_t flags = obs_source_get_output_flags(source);

        if (updateNeeded || force)
        {
            const int elidedWidth = 150;

            ClearSourceToolBar();
            
            ui->contextSourceLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            ui->contextSourceLabel->setMinimumWidth(elidedWidth);
            ui->contextSourceLabel->setMaximumWidth(elidedWidth);

            const char* name = obs_source_get_name(source);

            QFontMetrics fontMetrics(ui->contextSourceLabel->font());
            QString strElidedText = fontMetrics.elidedText(name, Qt::ElideRight, elidedWidth);
            ui->contextSourceLabel->setText(strElidedText);

            ui->sourcePropertiesButton->setEnabled(true);
            ui->sourcePropertiesButton->show();

            weakToolbarSource = OBSGetWeakRef(source);

            signalRenamed = OBSSignal(obs_source_get_signal_handler(source), "rename", AFMainDynamicComposit::SourceRenamed, this);

            QIcon icon;
            QLabel* iconLabel = new QLabel(this);
            iconLabel->setFixedSize(24, 24);
            iconLabel->setObjectName("sourceIconLabel");

            if (strcmp(id, "scene") == 0)
                icon = ICON_CONTEXT.GetSceneIcon();
            else if (strcmp(id, "group") == 0)
                icon = ICON_CONTEXT.GetGroupIcon();
            else
                icon = ICON_CONTEXT.GetSourceIcon(id);

            QPixmap pixmapIconSource = icon.pixmap(QSize(24, 24));
            QPainter painter(&pixmapIconSource);

            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(pixmapIconSource.rect(), QColor(145, 150, 161, 255));
            painter.end();

            ui->contextSourceIcon->setPixmap(pixmapIconSource);
            ui->contextSourceIcon->show();

			if (flags & OBS_SOURCE_CONTROLLABLE_MEDIA)
			{

				if (strcmp(id, "ffmpeg_source") == 0) {
					panel = new AFQMediaSourceToolbar(ui->emptyContainer, source);
					ui->emptyContainer->layout()->addWidget(panel);
				}
                else if (!is_network_media_source(source, id))
				{
                    if (strcmp(id, "ffmpeg_list_source") == 0) {
                        panel = new AFQMediaListSourceToolbar(ui->emptyContainer, source);
                        ui->emptyContainer->layout()->addWidget(panel);
                    }
                    else if (strcmp(id, "soop_anivod_source") == 0 ||
                             strcmp(id, "soop_sportvod_source") == 0 ||
                             strcmp(id, "soop_dramavod_source") == 0 ||
                             strcmp(id, "soop_movievod_source") == 0)
                    {
                        panel = new AFQVodSourceToolbar(ui->emptyContainer, source);
                        ui->emptyContainer->layout()->addWidget(panel);

                        SOOP_SRC_MANAGER.SetSoopMediaSourceToolBar(panel);
                    }
                    else if (strcmp(id, "soop_directbroad_source") == 0)
                    {
                        panel = new AFQDirectBroadSourceToolbar(ui->emptyContainer, source);
                        ui->emptyContainer->layout()->addWidget(panel);

                        SOOP_SRC_MANAGER.SetSoopMediaSourceToolBar(panel);
                    }
                    else if (strcmp(id, "soop_tv_cable_source") == 0)
                    {
                        panel = new AFQTvLiveSourceToolbar(ui->emptyContainer, source);
                        ui->emptyContainer->layout()->addWidget(panel);

                        SOOP_SRC_MANAGER.SetSoopMediaSourceToolBar(panel);
                    }
                    else if (strcmp(id, "slideshow") == 0) {
                        panel = new AFQSlideShowSourceToolbar(ui->emptyContainer, source);
                        ui->emptyContainer->layout()->addWidget(panel);
                    }
				}
			}
            else if (strcmp(id, "monitor_capture") == 0)
            {
                panel = new AFQMonitorSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "window_capture") == 0)
            {
                panel = new AFQWindowSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "window_area_capture") == 0)
            {
                panel = new AFQAreaSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "image_source") == 0)
            {
                panel = new AFQImageSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "game_capture") == 0)
            {
                panel = new AFQGameCaptureSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "browser_source") == 0)
            {
                ui->sourceInteractButton->show();

                panel = new AFQBrowserSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "soop_chat_source_chat") == 0 ||
                     strcmp(id, "soop_chat_source_notice") == 0 || 
                     strcmp(id, "soop_chat_source_goal") == 0 || 
                     strcmp(id, "soop_chat_source_banner") == 0 ||
                     strcmp(id, "soop_chat_source_subtitle") == 0 ||
                     strcmp(id, "soop_chat_source_c_mission") == 0 ||
                     strcmp(id, "soop_chat_source_timer") == 0 ) {

                ui->sourceInteractButton->show();

                panel = new AFQAquaSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "soop_chat_source_score") == 0) {

                panel = new AFQAquaScoreSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            } else if(0 == strcmp(id, "soop_chat_source_mood_check")) {
                ui->sourceInteractButton->show();

                panel = new AFQBrowserSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "soop_videoballoon_source") == 0) {
                ui->sourceInteractButton->show();
                panel = new AFQVideoBalloonSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "dshow_input") == 0 ||
                     strcmp(id, "av_capture_input") == 0)
            {
                panel = new AFQDeviceSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);

                AFQDeviceSourceToolbar* devicePanel = reinterpret_cast<AFQDeviceSourceToolbar*>(panel);

                connect(devicePanel, &AFQDeviceSourceToolbar::qsignalSplitFilterActived,
                    this, &AFMainDynamicComposit::UpdateVideoCaptureSplitAcitved);
            }
            else if (strcmp(id, "text_gdiplus") == 0)
            {
                panel = new AFQTextSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "wasapi_input_capture") == 0
                || strcmp(id, "wasapi_output_capture") == 0
                || strcmp(id, "wasapi_process_output_capture") == 0)
            {
                panel = new AFQAudioSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "color_source") == 0)
            {
                panel = new AFQColorSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "soop_spout2") == 0) {
                panel = new AFQSpoutSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "soop_commerce_source_goal") == 0 || strcmp(id, "soop_commerce_source_rank") == 0) {
                panel = new AFQCommerceSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "soop_kbo_graphic_source_all") == 0 ||
                     strcmp(id, "soop_kbo_graphic_source_score") == 0 ||
                     strcmp(id, "soop_kbo_graphic_source_stadium") == 0 ||
                     strcmp(id, "soop_kbo_graphic_source_player") == 0 ||
                     strcmp(id, "soop_kbo_graphic_source_livetext") == 0) {
                panel = new AFQKBOSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "soop_football_graphic_source_all") == 0 ||
                     strcmp(id, "soop_football_graphic_source_player") == 0 ||
                     strcmp(id, "soop_football_graphic_source_change") == 0 ||
                     strcmp(id, "soop_football_graphic_source_score") == 0 ||                     
                     strcmp(id, "soop_football_graphic_source_livetext") == 0) {
                panel = new AFQKBOSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "painter_source") == 0) {
                panel = new AFQPainterSourceToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
            }
            else if (strcmp(id, "soop_aimanager_source") == 0) {
                panel = new AFQAIManagerToolbar(ui->emptyContainer, source);
                ui->emptyContainer->layout()->addWidget(panel);
                }

            if (panel)
                panel->SetContextBarSize(contextBarSize);

            bool isConfigure = AFSourceUtil::ShouldShowProperties(source);
            ui->sourcePropertiesButton->setEnabled(isConfigure);

            QString iconPath;
            std::string absPath;
            GetDataFilePath("assets", absPath);

            if (isConfigure) {
                iconPath = QString("%1/source-toolbar/bt_props.svg")
                    .arg(absPath.c_str());
            }
            else {
                iconPath = QString("%1/source-toolbar/bt_props_disabled.svg")
                    .arg(absPath.c_str());
            }

            QIcon iconProps(iconPath);
            ui->sourcePropertiesButton->setIcon(iconProps);
        }
    }
    else {
        ClearSourceToolBar();

        ui->contextSourceLabel->setFixedWidth(178);
        ui->contextSourceLabel->setText(QTStr("Toolbar.NonSelectedSource"));
        ui->sourcePropertiesButton->setEnabled(false);
        ui->sourcePropertiesButton->hide();
        ui->contextSourceIcon->hide();
    }

    if (contextBarSize == ContextBarSize_Normal) {
        // add button text space
        ui->sourcePropertiesButton->setText(" " + QTStr("Toolbar.Props"));
        ui->sourceInteractButton->setText(" " + QTStr("Toolbar.Interaction"));
    }
    else {
        ui->sourcePropertiesButton->setText("");
        ui->sourceInteractButton->setText("");
    }
}

void AFMainDynamicComposit::ClearSourceToolBar()
{
    QLayoutItem* la = ui->emptyContainer->layout()->itemAt(0);
    if (la) {
        delete la->widget();
        ui->emptyContainer->layout()->removeItem(la);
    }
    weakToolbarSource = nullptr;
}

void AFMainDynamicComposit::RefreshToolBarSplitFilterToggleButton()
{
    QLayout* layout = ui->emptyContainer->layout();
    if (!layout || layout->count() == 0)
        return;

    QLayoutItem* item = layout->itemAt(0);
    if (item && item->widget()) {
        if (AFQDeviceSourceToolbar* toolbar = dynamic_cast<AFQDeviceSourceToolbar*>(item->widget())) {
            toolbar->RefreshSplitToogleButtonState();
        }
    }
}

void AFMainDynamicComposit::CheckDocksState()
{
    //Width(SideDock OFF): Bottom / Top / (Left + Right) compare - largest
    //Width(SideDock ON): Bottom / Top compare - largest + Left + Right
    //Height(SideDock OFF): Right / Left compare - largest + Top + Bottom
    //Height(SideDock ON): Bottom + Top

    if (!MAINFRAME->IsCompleteInit())
        return;

    MAIN_BLOCKMANAGER->CheckDockState();
    QTimer::singleShot(0, this, [this]() {

        //TAB DOCK: largest minimum width (all same height)
        QList<QDockWidget*> allDocks = findChildren<QDockWidget*>();
        QHash<QPoint, QDockWidget*> selectedDocks;
        QSet<QDockWidget*> visited;

        bool hasTabGroup = false;

        for (auto* dock : allDocks)
        {
            if (visited.contains(dock)) 
                continue;

            QList<QDockWidget*> group = tabifiedDockWidgets(dock);
            group.prepend(dock);

            if (group.size() > 1)
                hasTabGroup = true;

            for (auto* d : group)
            {
                visited.insert(d);
            }

            QDockWidget* best = group.first();
            int bestWidth = best->minimumWidth();
            QPoint tabifiedPoint = QPoint(0, 0);
            bool firstPoint = true;

            for (auto* d : group)
            {
                if (firstPoint)
                {
                    tabifiedPoint = QPoint(d->x(), d->y());
                    firstPoint = false;
                }
                else
                {
                    if (tabifiedPoint.y() < 0 || tabifiedPoint.x() < 0)
                    {
                        tabifiedPoint = QPoint(d->x(), d->y());
                    }
                }

                if (d->minimumWidth() > bestWidth)
                {
                    best = d;
                    bestWidth = d->minimumWidth();
                }
            }
            selectedDocks.insert(tabifiedPoint, best);
        }
        //TAB DOCK

        //BOTTOM compare TOP WIDTH
        bool hasTopDock = false;
        bool hasBottomDock = false;

        int bottomWidth = MAINFRAME->GetLeftNavigationBarWidth() + 6 + 12;
        int topWidth = MAINFRAME->GetLeftNavigationBarWidth() + 6 + 12;

        QMap<int, int> bottomXs;
        QMap<int, int> topXs;

        //BOTTOM + TOP HEIGHT
        QMap<int, int> bottomTopYs;
                
        //LEFT + RIGHT WIDTH
        int leftRightWidth = MAINFRAME->GetLeftNavigationBarWidth() + ui->contextContainer->minimumWidth() + 6 + 18;
        if (isSideDock)
            leftRightWidth = 6 + 18;

        QMap<int, int> leftRightXs;

        //LEFT + RIGHT HEIGHT
        QMap<int, int> leftRightYs;

        for (auto it = selectedDocks.begin(); it != selectedDocks.end(); ++it) {

            QPoint dockPoint = it.key();
            QDockWidget* dock = it.value();

            if (!dock->isFloating())
            {
                if (dockWidgetArea(dock) == Qt::BottomDockWidgetArea || dockWidgetArea(dock) == Qt::TopDockWidgetArea)
                {
                    if (dockWidgetArea(dock) == Qt::BottomDockWidgetArea)
                    {
                        int addVal = dock->minimumWidth() + 6;
                        int dockX = dock->x();
                        if (dockX < 0)
                            dockX = 0;
                        if (bottomXs.contains(dockX))
                        {
                            if (addVal > bottomXs.value(dock->x()))
                                bottomWidth = bottomWidth - bottomXs[dock->x()] + addVal;
                        }
                        else
                        {
                            bottomXs.insert(dockX, addVal);
                            bottomWidth += addVal;
                        }
                        hasBottomDock = true;
                    }
                    else if (dockWidgetArea(dock) == Qt::TopDockWidgetArea)
                    {
                        int addVal = dock->minimumWidth() + 6;
                        int dockX = dock->x();
                        if (dockX < 0)
                            dockX = 0;
                        if (topXs.contains(dockX))
                        {
                            if (addVal > topXs.value(dock->x()))
                                bottomWidth = bottomWidth - topXs[dock->x()] + addVal;
                        }
                        else
                        {
                            topXs.insert(dockX, addVal);
                            topWidth += addVal;
                        }
                        hasTopDock = true;
                    }

                    int minHeight = dock->minimumHeight() + 6;

                    if (bottomTopYs.contains(dockPoint.y()))
                        minHeight = bottomTopYs.value(dockPoint.y()) > minHeight ? bottomTopYs.value(dockPoint.y()) : minHeight;

                    bottomTopYs.insert(dockPoint.y(), minHeight);
                }
                else if (dockWidgetArea(dock) == Qt::LeftDockWidgetArea || dockWidgetArea(dock) == Qt::RightDockWidgetArea)
                {
                    int addVal = dock->minimumWidth() + 6;
                    if (leftRightXs.contains(dock->x()))
                    {
                        if (addVal > leftRightXs.value(dock->x()))
                            bottomWidth = bottomWidth - leftRightXs[dock->x()] + addVal;
                    }
                    else
                    {
                        leftRightXs.insert(dock->x(), addVal);
                        leftRightWidth += addVal;
                    }

                    int minHeight = dock->minimumHeight() + 6;

                    int dockY = dock->y();
                    if (dockY < 0)
                        dockY = 0;

                    if (leftRightYs.contains(dockY))
                        minHeight = leftRightYs.value(dockY) > minHeight ? leftRightYs.value(dockY) : minHeight;
                    
                    leftRightYs.insert(dockY, minHeight);

                }
            }
            
        }

        //WIDTH
        int lastWidth = 800;

        if (isSideDock)
        {
            lastWidth = bottomWidth > topWidth ? bottomWidth : topWidth;
            lastWidth += leftRightWidth;
            if (lastWidth < 800)
                lastWidth = 800;
        }
        else
        {
            if (bottomWidth > 800 || topWidth > 800 || leftRightWidth > 800)
            {
                lastWidth = bottomWidth > topWidth ? bottomWidth : topWidth;
                lastWidth = lastWidth > leftRightWidth ? lastWidth : leftRightWidth;
            }
        }
        MAINFRAME->setMinimumWidth(lastWidth);
        //WIDTH
        // 
        //HEIGHT
        int lastHeight = MAINFRAME->GetTopAreaHeight() + MAINFRAME->GetBottomAreaHeight() + 12 + 6;
        if (ui->contextContainer && ui->contextContainer->isVisible())
            lastHeight += ui->contextContainer->minimumHeight();
        for (QMap<int, int>::iterator it = bottomTopYs.begin(); it != bottomTopYs.end(); ++it)
            lastHeight += it.value();
        
        if (isSideDock)
        {
            if (bottomXs.count() > 0)
                lastHeight -= 156;
        }

        for (QMap<int, int>::iterator it = leftRightYs.begin(); it != leftRightYs.end(); ++it)
            lastHeight += it.value();

        if (lastHeight < 550)
            lastHeight = 550;

        if (hasTopDock && hasBottomDock && hasTabGroup)
        {
            if (ui->contextContainer && ui->contextContainer->isVisible())
                if (lastHeight < 550 + ui->contextContainer->height())
                    lastHeight += ui->contextContainer->height();
        }

        MAINFRAME->setMinimumHeight(lastHeight);
        //HEIGHT
        });
}

void AFMainDynamicComposit::SetCurrentScene(OBSSource scene, bool force)
{
    auto& sceneContext = SCENE_CONTEXT;

    if (force && scene) {
        OBSScene obsScene = obs_scene_from_source(scene);
        sceneContext.SetCurrentScene(obsScene);
    }
    else {
        OBSScene curScene = sceneContext.GetCurrentScene();
        if (scene == obs_scene_get_source(curScene))
            return;
    }

    if (STATEAPP.IsPreviewProgramMode() == false) {
        sceneContext.TransitionToScene(scene, force);
    }
    else {
        OBSSource actualLastScene = OBSGetStrongRef(sceneContext.GetLastScene());
        if (actualLastScene != scene) {
            if (scene)
                obs_source_inc_showing(scene);
            if (actualLastScene) {
                obs_source_dec_showing(actualLastScene);

                OBSScene obsLastScene = obs_scene_from_source(actualLastScene);
                sceneContext.SetCurrentScene(obsLastScene);
            }
            sceneContext.SetLastScene(scene);
        }
    }

    QWidget* outBlock = nullptr;
    if (!MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::SceneSource, outBlock))
        return;

    AFSceneSourceWidget* scenesourceBlock = reinterpret_cast<AFSceneSourceWidget*>(outBlock);
    if (scenesourceBlock)
        scenesourceBlock->SetCurrentScene(scene, force);
    
    bool studioPortraitLayout = config_get_bool(USERCONFIG, "BasicWindow", "StudioPortraitLayout");

    UpdateSceneNameStudioMode(studioPortraitLayout);

    UpdateSourceToolBar(true);

    // Attach SOOP Media Source
	OBSScene changeScene = obs_scene_from_source(scene);
	obs_scene_enum_items(changeScene, [](obs_scene_t* scene, obs_sceneitem_t* item, void* param) {
		obs_source_t* source = obs_sceneitem_get_source(item);
		if (source && AFSourceUtil::IsSoopMediaSource(source)) {
			SOOP_SRC_MANAGER.SetSoopMediaSource(source);
			return false;
		}
		return true;
		}, nullptr);
}
//

void AFMainDynamicComposit::MakeVerticalStudioMode()
{
    AFMainWindowAccesser& tmpViewModels = g_viewModelsDynamic.GetInstance();
    ui->centralwidget->setStyleSheet("background-color: #0A0A0A;");
    ui->preview->SetDisplayBackgroundColor(GREY_TONDOWN_COLOR_BACKGROUND);

    tmpViewModels.m_renderModel.CreateProgramDisplay();
    InitProgramDisplay(&tmpViewModels);
    tmpViewModels.m_renderModel.SetProgramScene();

    if (studioModeVerticalView == nullptr)
    {
       studioModeVerticalView = new AFQVerticalProgramView();
       studioModeVerticalView->Initialize();

        UpdateSceneNameStudioMode(true);

        /*connect(ui->preview, &AFQTDisplay::qsignalDisplayResized,
            studioModeVerticalView, &AFQVerticalProgramView::qslotChangeLayoutStrech);*/
        
        QHBoxLayout* tmpCentLayoutThis = qobject_cast<QHBoxLayout*>(ui->centralwidget->layout());

        tmpCentLayoutThis->insertWidget(0, studioModeVerticalView);
        studioModeVerticalView->InsertDisplays(ui->preview,
            tmpViewModels.m_renderModel.GetProgramDisplay());

        studioModeVerticalView->show();

        emit StudioModeToggled();
    }

    blog(LOG_INFO, "Switched to Vertical Preview/Program mode");
    blog(LOG_INFO, "-----------------------------"
        "-------------------");
}

void AFMainDynamicComposit::MakeHorizontalStudioMode()
{
    AFMainWindowAccesser& tmpViewModels = g_viewModelsDynamic.GetInstance();
    ui->centralwidget->setStyleSheet("background-color: #0A0A0A;");
    ui->preview->SetDisplayBackgroundColor(GREY_TONDOWN_COLOR_BACKGROUND);

    tmpViewModels.m_renderModel.CreateProgramDisplay();
    InitProgramDisplay(&tmpViewModels);
    tmpViewModels.m_renderModel.SetProgramScene();

    if (studioModeView == nullptr)
    {
       studioModeView = new AFQProgramView();
       studioModeView->Initialize();

        UpdateSceneNameStudioMode(false);

        connect(ui->preview, &AFQTDisplay::qsignalDisplayResized, studioModeView, &AFQProgramView::qslotChangeLayoutStrech);

        QHBoxLayout* tmpCentLayoutThis = qobject_cast<QHBoxLayout*>(ui->centralwidget->layout());
        tmpCentLayoutThis->insertWidget(0, studioModeView);

        studioModeView->InsertDisplays(ui->preview, tmpViewModels.m_renderModel.GetProgramDisplay());
        studioModeView->show();

        emit StudioModeToggled();
    }

    blog(LOG_INFO, "Switched to Vertical Preview/Program mode");
    blog(LOG_INFO, "-----------------------------"
        "-------------------");
}

void AFMainDynamicComposit::DestroyVerticalStudioMode()
{
    studioModeVerticalView->hide();
    
    ui->centralwidget->setStyleSheet("background-color: #111111;");
    ui->preview->SetDisplayBackgroundColor(GREY_COLOR_BACKGROUND);

    QHBoxLayout* tmpCentLayoutThis = qobject_cast<QHBoxLayout*>(ui->centralwidget->layout());
    tmpCentLayoutThis->insertWidget(0, ui->preview);

    AFMainWindowAccesser& tmpViewModels = g_viewModelsDynamic.GetInstance();
    studioModeVerticalView->layout()->removeWidget(tmpViewModels.m_renderModel.GetProgramDisplay());
    ui->centralwidget->layout()->removeWidget(studioModeVerticalView);

    //disconnect(ui->widget_Screen, &AFQTDisplay::qsignalDisplayResized,
    //    m_pStudioModeVerticalView, &AFQProgramView::qslotChangeLayoutStrech);

    if (studioModeVerticalView != nullptr)
    {
        delete studioModeVerticalView;
        studioModeVerticalView = nullptr;
    }

    //
    tmpViewModels.m_renderModel.ResetProgramScene();
    tmpViewModels.m_renderModel.ReleaseProgramDisplay();

    QWidget* outBlock = nullptr;
    if (MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::SceneControl, outBlock))
    {
        AFSceneControlWidget* sceneControl = reinterpret_cast<AFSceneControlWidget*>(outBlock);

        sceneControl->ChangeLayoutStudioMode();
    }

    blog(LOG_INFO, "Switched to regular Preview mode From Vertical");
    blog(LOG_INFO, "------------------------------------------------");
}

void AFMainDynamicComposit::DestroyHorizontalStudioMode()
{
   studioModeView->hide();

    ui->centralwidget->setStyleSheet("background-color: #111111;");
    ui->preview->SetDisplayBackgroundColor(GREY_COLOR_BACKGROUND);

    QHBoxLayout* tmpCentLayoutThis = qobject_cast<QHBoxLayout*>(ui->centralwidget->layout());
    tmpCentLayoutThis->insertWidget(0, ui->preview);

    AFMainWindowAccesser& tmpViewModels = g_viewModelsDynamic.GetInstance();
    
   studioModeView->layout()->removeWidget(tmpViewModels.m_renderModel.GetProgramDisplay());
    ui->centralwidget->layout()->removeWidget(studioModeView);

    disconnect(ui->preview, &AFQTDisplay::qsignalDisplayResized, studioModeView, &AFQProgramView::qslotChangeLayoutStrech);

    if (studioModeView != nullptr)
    {
        delete studioModeView;
        studioModeView = nullptr;
    }
    
    //
    tmpViewModels.m_renderModel.ResetProgramScene();
    tmpViewModels.m_renderModel.ReleaseProgramDisplay();

    QWidget* outBlock = nullptr;
    if (MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::SceneControl, outBlock))
    {
        AFSceneControlWidget* sceneControl = reinterpret_cast<AFSceneControlWidget*>(outBlock);
        sceneControl->ChangeLayoutStudioMode();
    }

    blog(LOG_INFO, "Switched to regular Preview mode From Horizontal");
    blog(LOG_INFO, "------------------------------------------------");
}

void AFMainDynamicComposit::InitMainDisplay(AFMainWindowAccesser* viewModels)
{
    auto addDisplay = [viewModels](AFQTDisplay* window) {
        if (viewModels == nullptr)
            return;
        
        obs_display_add_draw_callback(window->GetDisplay(),
            AFMainWindowRenderModel::RenderMain, &viewModels->m_renderModel);

        struct obs_video_info ovi;
        if (obs_get_video_info(&ovi))
            viewModels->m_renderModel.ResizePreview(ovi.base_width, ovi.base_height);
    };

    connect(ui->preview, &AFQTDisplay::qsignalDisplayCreated, addDisplay);


    auto displayResize = [this, viewModels]() {
        if (viewModels == nullptr)
            return;
        
        struct obs_video_info ovi;

        if (obs_get_video_info(&ovi))
            viewModels->m_renderModel.ResizePreview(ovi.base_width, ovi.base_height);

        UpdateContextBarVisibility();

        if (viewModels != nullptr)
            viewModels->m_renderModel.SetDPIValue(devicePixelRatioF());
        
        
    };

    connect(windowHandle(), &QWindow::screenChanged, displayResize);
    connect(ui->preview, &AFQTDisplay::qsignalDisplayResized, displayResize);
    //
}

void AFMainDynamicComposit::InitProgramDisplay(AFMainWindowAccesser* viewModels)
{
    AFQTDisplay* tmpProgramDisplay = viewModels->m_renderModel.GetProgramDisplay();
    tmpProgramDisplay->SetDisplayBackgroundColor(GREY_TONDOWN_COLOR_BACKGROUND);  
    tmpProgramDisplay->setContextMenuPolicy(Qt::CustomContextMenu);

    auto displayResize = [viewModels]() {
        struct obs_video_info ovi;

        if (obs_get_video_info(&ovi))
            viewModels->m_renderModel.ResizeProgram(ovi.base_width, ovi.base_height);
    };

    connect(tmpProgramDisplay, &AFQTDisplay::qsignalDisplayResized, displayResize);

    auto addDisplay = [viewModels](AFQTDisplay *window) {
        obs_display_add_draw_callback(window->GetDisplay(),
            AFMainWindowRenderModel::RenderProgram, &viewModels->m_renderModel);

        struct obs_video_info ovi;
        if (obs_get_video_info(&ovi))
            viewModels->m_renderModel.ResizeProgram(ovi.base_width, ovi.base_height);
    };

    connect(tmpProgramDisplay, &AFQTDisplay::qsignalDisplayCreated, addDisplay);

    tmpProgramDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void AFMainDynamicComposit::SourceRenamed(void* data, calldata_t* params)
{
    const char* name = calldata_string(params, "new_name");

    QMetaObject::invokeMethod(static_cast<AFMainDynamicComposit*>(data),
        "RenameSource", Q_ARG(QString, QT_UTF8(name)));
}
