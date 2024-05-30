#include "CMainDynamicComposit.h"
#include "ui_aneta-main-window.h"

#include <QString>
#include <QCloseEvent>
#include <QWindow>
#include <QScreen>
#include <QTimer>
#include <qobjectdefs.h>
#include <QPropertyAnimation>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>


#include <util/profiler.hpp>
#include <util/util.hpp>
#include <util/dstr.h>
#include <util/threading.h>
#include <json11.hpp>

#include "qt-wrapper.h"
#include "platform/platform.hpp"
///
//#include "obs-internal.h"
#include "graphics/graphics.h"
///
#include "Blocks/SceneControlDock/CSceneControlDockWidget.h"
#include "Blocks/SceneSourceDock/CSceneSourceDockWidget.h"
#include "Blocks/AudioMixerDock/CAudioMixerDockWidget.h"
#include "Blocks/AudioMixerDock/CVolumeControl.h"
#include "Blocks/BroadInfoDock/CBroadInfoDockWidget.h"
#include "Blocks/AdvanceControlsDock/CAdvanceControlsDockWidget.h"
#include "MainFrame/CMainFrame.h"
#include "MainFrame/MainPreview/CProgramView.h"
#include "MainFrame/MainPreview/CVerticalProgramView.h"
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
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Log/CLogManager.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Scene/CScene.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Icon/CIconContext.h"
#include "CoreModel/Action/CHotkeyContext.h"
#include "CoreModel/Browser/CCefManager.h"

#include "ViewModel/MainWindow/CMainWindowAccesser.h"
#include "ViewModel/MainWindow/CMainWindowRenderModel.h"
#include "UIComponent/CMessageBox.h"

#include "Utils/CStatusLogManager.h"

#ifdef _WIN32
#include <Windows.h>
#endif

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

AFMainDynamicComposit::AFMainDynamicComposit(QWidget* parent) :
    AFQMainWindow(parent),
    m_dockMenu(new AFQCustomMenu(this)),
    ui(new Ui::AFMainDynamicComposit)
{
    ui->setupUi(this);

    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NativeWindow);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);
        
    g_ViewModelsDynamic.GetInstance().m_RenderModel.SetDPIValue(devicePixelRatioF());
   
    ui->widget_Screen->InitSetNugeEventAction();

    connect(ui->button_ShowPreview, &QPushButton::clicked, 
            this, &AFMainDynamicComposit::qslotShowPreview);

    connect(ui->frame_notPreview, &QWidget::customContextMenuRequested,
            this, &AFMainDynamicComposit::qSlotShowContextMenu);
}

AFMainDynamicComposit::~AFMainDynamicComposit()
{
    m_qAdvAudioSettingPopup->deleteLater();
 
    m_MainAudioVolumeControl = nullptr;
    m_MainMicVolumeControl = nullptr;
    m_mixerCopyFiltersSource = nullptr;

    if (m_qSceneTransitionPopup) {
        m_qSceneTransitionPopup->close();
        m_qSceneTransitionPopup = nullptr;
    }

    ClearAllSourceContextPopup();

    delete ui;
}

void AFMainDynamicComposit::qslotToggleLockTriggered()
{
    if (stopHover)
    {
        stopHover = false;
    }
}

void AFMainDynamicComposit::qslotChangeDockToWindow(bool toDock, const char* blocktype)
{
    QDockWidget* dock = nullptr; 
    bool checkDockExist = _FindDock(blocktype, dock);

    if (checkDockExist)
    {
        if (toDock)
        {
            _ClosePopup(blocktype, true);
        }
        else
        {
            _MakePopupForDock(blocktype, !dock->isFloating());
        }
    }
}

void AFMainDynamicComposit::qslotCloseBlock(const char* blocktype)
{
    QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
    int blockTypeNum = BlockTypeEnum.keyToValue(blocktype);
    _ToggleVisibleBlockWindow(blockTypeNum, false);
}

void AFMainDynamicComposit::qslotMaximumBlock(const char* blocktype)
{
	bool checkBlockExist = m_qBlockMap.contains(blocktype);
	if (!checkBlockExist)
		return;

    QPair<QWidget*, QWidget*> block;
    bool checkDockTitle = _FindBlock(blocktype, block);
    if (!checkDockTitle)
        return;

    AFDockTitle* dockTitle = qobject_cast<AFDockTitle*>(block.first);
    if (!dockTitle)
        return;

	QWidget* blockPopup = nullptr;
	bool checkPopup = _FindPopup(blocktype, blockPopup);
	if (!checkPopup)
		return;

	if (blockPopup->isMaximized())
		blockPopup->showNormal();
	else
		blockPopup->showMaximized();

    dockTitle->ChangeMaximizedIcon(blockPopup->isMaximized());
}

void AFMainDynamicComposit::qslotMinimumBlock(const char* blocktype)
{
    bool checkBlockExist = m_qBlockMap.contains(blocktype);
    if (!checkBlockExist)
        return;

    QWidget* blockPopup = nullptr;
    bool checkPopup = _FindPopup(blocktype, blockPopup);
    if (checkPopup)
        blockPopup->showMinimized();
}

void AFMainDynamicComposit::qslotText()
{
    qDebug() << m_qSceneSourceDock->property("floating");
}


void AFMainDynamicComposit::qslotToggleStudioModeBlock(bool enable)
{
    AFStateAppContext* tmpStateApp = AFConfigManager::GetSingletonInstance().GetStates();
    
    if (tmpStateApp->IsPreviewProgramMode() == enable)
        return;
        
    emit qsignalBlockVisibleToggled(enable, BlockType::StudioMode);
    
    tmpStateApp->SetPreviewProgramMode(enable);

    config_t* configGlobalFile = AFConfigManager::GetSingletonInstance().GetGlobal();
    bool studioPortraitLayout = config_get_bool(configGlobalFile, "BasicWindow", "StudioPortraitLayout");
    bool studioLabel = config_get_bool(configGlobalFile, "BasicWindow", "StudioModeLabels");

    m_qAFAdvanceControlsDockWidget->SetStudioMode(tmpStateApp->IsPreviewProgramMode());
    
    if(tmpStateApp->IsPreviewProgramMode())
    {       
        if (studioPortraitLayout)
        {
            _MakeVerticalStudioMode();
        }
        else
        {
            _MakeHorizontalStudioMode();
        }
        ToggleStudioModeLabels(studioPortraitLayout, studioLabel);
    }
    else
    {
        if (studioPortraitLayout)
        {
            _DestroyVerticalStudioMode();
        }
        else
        {
            _DestroyHorizontalStudioMode();
        }
    }
}

void AFMainDynamicComposit::qslotChangeSceneOnDoubleClick()
{
    config_t* configGlobalFile = AFConfigManager::GetSingletonInstance().GetGlobal();
    bool doubleClickSwitch = config_get_bool(configGlobalFile, "BasicWindow", "TransitionOnDoubleClick");

    if (doubleClickSwitch)
    {
        AFStateAppContext* tmpStateApp = AFConfigManager::GetSingletonInstance().GetStates();

        if (tmpStateApp->IsPreviewProgramMode() == false)
            return;

        bool studioPortraitLayout = config_get_bool(configGlobalFile, "BasicWindow", "StudioPortraitLayout");
        if (studioPortraitLayout)
        {
            m_pStudioModeVerticalView->qslotTransitionTriggered();
        }
        else {
            m_pStudioModeView->qslotTransitionTriggered();
        }
    }
}

void AFMainDynamicComposit::qslotAddSceneTriggered()
{
    AFSceneContext& contextScene = AFSceneContext::GetSingletonInstance();

    std::string name;
    QString format{ Str("Basic.Main.DefaultSceneName.Text") };

    int i = 2;
    QString placeHolderText = format.arg(i);
    OBSSourceAutoRelease source = nullptr;
    while ((source = obs_get_source_by_name(QT_TO_UTF8(placeHolderText)))) {
        placeHolderText = format.arg(++i);
    }

    bool accept = AFQNameDialog::AskForName(App()->GetMainView(),
                                  Str("Basic.Main.AddSceneDlg.Title"),
                                  Str("Basic.Main.AddSceneDlg.Text"), 
                                  name, placeHolderText, 170, true);
    if(accept) {

        if (name.empty()) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, 
                                       App()->GetMainView(),
                                       Str("NoNameEntered.Title"),
                                       Str("NoNameEntered.Text"),
                                       true, true);
            qslotAddSceneTriggered();
            return;
        }

        OBSSourceAutoRelease source =
            obs_get_source_by_name(name.c_str());
        if (source) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
                                       App()->GetMainView(),
                                       Str("NameExists.Title"),
                                       Str("NameExists.Text"),
                                       true, true);
            qslotAddSceneTriggered();
            return;
        }

        auto undo_fn = [](const std::string& data) {
            obs_source_t* t = obs_get_source_by_name(data.c_str());
            if(t) {
                obs_source_remove(t);
                obs_source_release(t);
            }
        };
        auto redo_fn = [this](const std::string& data) {
            OBSSceneAutoRelease scene = obs_scene_create(data.c_str());
            obs_source_t* source = obs_scene_get_source(scene);
            SetCurrentScene(source, true);
        };
        AFMainFrame* main = App()->GetMainView();
        main->m_undo_s.AddAction(QTStr("Undo.Add").arg(QString(name.c_str())),
                                 undo_fn, redo_fn, name, name);

        obs_source_t* scene_source = AFSceneUtil::CreateOBSScene(name.c_str());
        SetCurrentScene(scene_source);
    }
}

void AFMainDynamicComposit::qslotAddSourceTriggered()
{
    emit qsignalSelectSourcePopup();
}

void AFMainDynamicComposit::qslotTransitionSceneTriggered()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    OBSSource tr = sceneContext.GetCurTransition();
    int duration = sceneContext.GetCurDuraition();

    ShowSceneTransitionPopup(tr, duration);
}

void AFMainDynamicComposit::qslotShowSceneControlDockTriggered()
{
    ToggleDockVisible(BlockType::SceneControl, true);
}

void AFMainDynamicComposit::qslotStackedMixerAreaContextMenuRequested()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& localeTextManger = AFLocaleTextManager::GetSingletonInstance();

    QAction unhideAllAction(localeTextManger.Str("UnhideAll"), this);

    QAction advPropAction(localeTextManger.Str("Basic.MainMenu.Edit.AdvAudio"), this);

    QAction toggleControlLayoutAction(localeTextManger.Str("VerticalLayout"), this);
    toggleControlLayoutAction.setCheckable(true);
    toggleControlLayoutAction.setChecked(config_get_bool(
        confManager.GetGlobal(), "BasicWindow", "VerticalVolControl"));

    /* ------------------- */

    connect(&unhideAllAction, &QAction::triggered, this,
        &AFMainDynamicComposit::qslotUnhideAllAudioControls, Qt::DirectConnection);

    connect(&advPropAction, &QAction::triggered, this,
        &AFMainDynamicComposit::qslotAdvAudioPropertiesTriggered,
        Qt::DirectConnection);

    /* ------------------- */

    connect(&toggleControlLayoutAction, &QAction::changed, this,
        &AFMainDynamicComposit::ToggleVolControlLayout, Qt::DirectConnection);

    /* ------------------- */

    AFQCustomMenu popup = new AFQCustomMenu(this);
    popup.addAction(&unhideAllAction);
    popup.addSeparator();
    popup.addAction(&toggleControlLayoutAction);
    popup.addSeparator();
    popup.addAction(&advPropAction);
    popup.exec(QCursor::pos());
}

void AFMainDynamicComposit::qslotCloseCustomBrowser(QString uuid)
{
    QWidget* widget = m_qBrowserWidgetMap.value(uuid);
    if (widget)
    {
        m_qBrowserWidgetMap.remove(uuid);
        widget->close();
        delete widget;
        widget = nullptr;
    }
}

void AFMainDynamicComposit::qslotClosedCustomBrowser(QString uuid)
{
    App()->GetMainView()->CustomBrowserClosed(uuid);
}

void AFMainDynamicComposit::qslotMaximizeBrowser(QString uuid) 
{
    AFQBorderPopupBaseWidget* browserWidget = m_qBrowserWidgetMap.value(uuid);
    if (browserWidget)
    {
        if (browserWidget->isMaximized()) {
            browserWidget->showNormal();
        }
        else {
            browserWidget->RestoreNormalWidth(browserWidget->width());
            browserWidget->showMaximized();
        }

        AFDockTitle* dockTitle = qobject_cast<AFDockTitle*>(browserWidget->GetWidgetByName("CustomBrowserDockTitle"));
        if (dockTitle) {
            dockTitle->ChangeMaximizedIcon(browserWidget->isMaximized());
        }
    }
}

void AFMainDynamicComposit::qslotMinimizeBrowser(QString uuid)
{
    AFQBorderPopupBaseWidget* browserWidget = m_qBrowserWidgetMap.value(uuid);
    if (browserWidget)
    {
        browserWidget->showMinimized();
    }
}

void AFMainDynamicComposit::qslotShowPreview()
{
    emit qsignalShowPreview();
}

void AFMainDynamicComposit::closeEvent(QCloseEvent* event)
{
    config_t* configGlobalFile = AFConfigManager::GetSingletonInstance().GetGlobal();
    //Save Dock State
    QMap<const char*, QDockWidget*>::iterator dockiter;
    for (dockiter = m_qDockMap.begin(); dockiter != m_qDockMap.end(); ++dockiter)
    {
        std::string check = QString("%1Popup").arg(dockiter.key()).toStdString();

        config_set_bool(configGlobalFile,
            "BasicWindow", check.c_str(), false);

        config_set_string(configGlobalFile, "BasicWindow", dockiter.key(),
            dockiter.value()->saveGeometry().toBase64().constData());
    }

    config_set_string(configGlobalFile, "BasicWindow", "DockState",
        saveState().toBase64().constData());
    //Save Dock State

    //Save Window State
    QMap<const char*, QWidget*>::iterator iter;
    for (iter = m_qPopupMap.begin(); iter != m_qPopupMap.end(); ++iter)
    {
        std::string check = QString("%1Popup").arg(iter.key()).toStdString();

        config_set_bool(configGlobalFile,
            "BasicWindow", check.c_str(), true);

        config_set_string(configGlobalFile, "BasicWindow", iter.key(),
            iter.value()->saveGeometry().toBase64().constData());
    }

    SaveCustomBrowserList();
    //Save Window State
    
    AFMainWindowAccesser* tmpViewModels = nullptr;
    tmpViewModels = g_ViewModelsDynamic.UnSafeGetInstace();
    
    if (tmpViewModels)
        tmpViewModels->m_RenderModel.RemoveCallbackMainDisplay();
    
    AFGraphicsContext::GetSingletonInstance().FinContext();
    AFCefManager::GetSingletonInstance().FinContext();
    //

    _CloseAllWindows();
    
    ReleaseDock();

    _ClearBrowserInterationQMap();
    _ClearDockMap();
    _ClearPopupMap();
    _ClearBlockMap();

    signalHandlers.clear();
}

void AFMainDynamicComposit::SourceCreated(void* data, calldata_t* params)
{
    obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");

    if (obs_scene_from_source(source) != NULL) {
        QMetaObject::invokeMethod(static_cast<AFMainDynamicComposit*>(data),
            "AddScene", WaitConnection(),
            Q_ARG(OBSSource, OBSSource(source)));
    }
}

void AFMainDynamicComposit::SourceRemoved(void* data, calldata_t* params)
{
    obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");

    if (obs_scene_from_source(source) != NULL)
        QMetaObject::invokeMethod(static_cast<AFMainDynamicComposit*>(data),
            "RemoveScene",
            Q_ARG(OBSSource, OBSSource(source)));
}

void AFMainDynamicComposit::SourceActivated(void* data, calldata_t* params)
{
    obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");
    uint32_t flags = obs_source_get_output_flags(source);

    if (flags & OBS_SOURCE_AUDIO) {
        QMetaObject::invokeMethod(static_cast<AFMainDynamicComposit*>(data),
                                  "qslotActivateAudioSource",
                                   Q_ARG(OBSSource, OBSSource(source)));
        QMetaObject::invokeMethod(static_cast<AFMainDynamicComposit*>(data), 
                                  "qslotSetMainAudioSource", 
                                  Qt::QueuedConnection);
    }
}

void AFMainDynamicComposit::SourceDeactivated(void* data, calldata_t* params)
{
    obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");
    uint32_t flags = obs_source_get_output_flags(source);

    if (flags & OBS_SOURCE_AUDIO) {
        QMetaObject::invokeMethod(static_cast<AFMainDynamicComposit*>(data),
            "qslotDeactivateAudioSource",
            Q_ARG(OBSSource, OBSSource(source)));
        QMetaObject::invokeMethod(static_cast<AFMainDynamicComposit*>(data), 
                                  "qslotSetMainAudioSource", 
                                  Qt::QueuedConnection);
    }
}

void AFMainDynamicComposit::SourceAudioActivated(void* data, calldata_t* params)
{
    obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");

    if (obs_source_active(source))
        QMetaObject::invokeMethod(static_cast<AFMainDynamicComposit*>(data),
            "qslotActivateAudioSource",
            Q_ARG(OBSSource, OBSSource(source)));
}

void AFMainDynamicComposit::SourceAudioDeactivated(void* data, calldata_t* params)
{
    obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");
    QMetaObject::invokeMethod(static_cast<AFMainDynamicComposit*>(data),
        "qslotDeactivateAudioSource",
        Q_ARG(OBSSource, OBSSource(source)));
}

void AFMainDynamicComposit::ShowOrUpdateContextPopup(OBSSource source, bool force)
{
	auto it = m_mapSourceContextPopup.find(source);
    if (m_mapSourceContextPopup.end() != it) {
        AFQSourceControlDialog* sourceContextPopup = (*it);
        if (sourceContextPopup) {
            sourceContextPopup->raise();
            sourceContextPopup->activateWindow();
        }
        return;
    }
    else {
        const char* id = obs_source_get_unversioned_id(source);
        if (is_network_media_source(source, id)) {
            return;
        }

        AFQSourceControlDialog* sourceContextPopup = new AFQSourceControlDialog(App()->GetMainView());
        sourceContextPopup->setAttribute(Qt::WA_DeleteOnClose, true);
        sourceContextPopup->setModal(false);
        sourceContextPopup->SetOBSSource(source, force);

        connect(sourceContextPopup, &AFQSourceControlDialog::qsignalClearContext,
            this, &AFMainDynamicComposit::HideSourceContextPopupFromRemoveSource);

        setCenterPositionNotUseParent(sourceContextPopup, App()->GetMainView());

        sourceContextPopup->show();

        m_mapSourceContextPopup.insert(source, sourceContextPopup);
    }
}

void AFMainDynamicComposit::UpdateContextPopup(bool force)
{
    obs_source_t* source = nullptr;
    OBSSceneItem item = m_qSceneSourceDock->GetCurrentSceneItem();
    if (item)
        source = obs_sceneitem_get_source(item);

    MAP_SOURCE_CONTEXT::iterator it = m_mapSourceContextPopup.begin();
    for (; it != m_mapSourceContextPopup.end(); ++it) {
        AFQSourceControlDialog* sourceContextPopup = (*it);
        if (!sourceContextPopup)
            return;

        //bool selected = (sourceContextPopup->GetOBSSource() == source);
        //sourceContextPopup->SelectedContextSource(selected);
    }
}

void AFMainDynamicComposit::UpdateContextPopupDeferred(bool force)
{
    QMetaObject::invokeMethod(this, "UpdateContextPopup",
        Qt::QueuedConnection, Q_ARG(bool, force));
}

void AFMainDynamicComposit::HideSourceContextPopupFromRemoveSource(OBSSource source)
{
    MAP_SOURCE_CONTEXT::iterator it = m_mapSourceContextPopup.find(source);
	if (m_mapSourceContextPopup.end() == it)
		return;

	AFQSourceControlDialog* sourceContextPopup = (*it);
	if (!sourceContextPopup)
		return;

	sourceContextPopup->close();
	sourceContextPopup = nullptr;

	m_mapSourceContextPopup.remove(source);

}

void AFMainDynamicComposit::ClearAllSourceContextPopup()
{
    MAP_SOURCE_CONTEXT::iterator it = m_mapSourceContextPopup.begin();
    while (it != m_mapSourceContextPopup.end()) {
        auto next = it;
        ++next;

        AFQSourceControlDialog* sourceContextPopup = (*it);
        if (sourceContextPopup) {
            sourceContextPopup->close();
            sourceContextPopup = nullptr;
        }
        it = next;
    }
    m_mapSourceContextPopup.clear();
}

void AFMainDynamicComposit::ShowBrowserInteractionPopup(OBSSource source)
{
    if (m_mapBrowserInteraction.end() != m_mapBrowserInteraction.find(source))
        return;

    AFQBrowserInteraction* interaction = new AFQBrowserInteraction(nullptr, source);
    interaction->setAttribute(Qt::WA_DeleteOnClose, true);
    interaction->setModal(false);

    setCenterPositionNotUseParent(interaction, App()->GetMainView());

    interaction->show();

    connect(interaction, &AFQBrowserInteraction::qsignalClearPopup, 
            this, &AFMainDynamicComposit::ClearBrowserInteractionPopup);

    m_mapBrowserInteraction.insert(source, interaction);
}

void AFMainDynamicComposit::ClearBrowserInteractionPopup(OBSSource source)
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

void AFMainDynamicComposit::HideBrowserInteractionPopupFromRemoveSource(OBSSource source)
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

void AFMainDynamicComposit::ShowSceneTransitionPopup(OBSSource curTransition, int curDuration)
{
    bool closed = true;
    if (m_qSceneTransitionPopup)
        closed = m_qSceneTransitionPopup->close();

    if (!closed)
        return;

    m_qSceneTransitionPopup = new AFQSceneTransitionsDialog(nullptr, curTransition, curDuration);
    m_qSceneTransitionPopup->setAttribute(Qt::WA_DeleteOnClose);
    m_qSceneTransitionPopup->setModal(false);

    setCenterPositionNotUseParent(m_qSceneTransitionPopup, App()->GetMainView());

    m_qSceneTransitionPopup->show();
}

#include "ui_audio-mixer-dock.h"

template<typename QObjectPtr>
void InsertQObjectByName(std::vector<QObjectPtr>& controls, QObjectPtr control)
{
    QString name = control->objectName();
    auto finder = [name](QObjectPtr elem) {
        return name.localeAwareCompare(elem->objectName()) < 0;
        };
    auto found_at = std::find_if(controls.begin(), controls.end(), finder);

    controls.insert(found_at, control);
}

static inline bool SourceVolumeLocked(obs_source_t* source)
{
    OBSDataAutoRelease priv_settings =
        obs_source_get_private_settings(source);
    bool lock = obs_data_get_bool(priv_settings, "volume_locked");

    return lock;
}

void AFMainDynamicComposit::SourceRenamed(void* data, calldata_t* params)
{
    obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");
    const char* newName = calldata_string(params, "new_name");
    const char* prevName = calldata_string(params, "prev_name");

    QMetaObject::invokeMethod(static_cast<AFMainDynamicComposit*>(data),
        "qslotRenameSources", Q_ARG(OBSSource, source),
        Q_ARG(QString, QT_UTF8(newName)),
        Q_ARG(QString, QT_UTF8(prevName)));

    blog(LOG_INFO, "Source '%s' renamed to '%s'", prevName, newName);
}


void AFMainDynamicComposit::qSlotShowContextMenu(const QPoint& pos)
{

}

void AFMainDynamicComposit::qslotActivateAudioSource(OBSSource source)
{
    if (_SourceMixerHidden(source))
        return;
    if (!obs_source_active(source))
        return;
    if (!obs_source_audio_active(source))
        return;

    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& localeTextManger = AFLocaleTextManager::GetSingletonInstance();

    bool vertical = config_get_bool(confManager.GetGlobal(), "BasicWindow", "VerticalVolControl");
    
    AFQVolControl* vol = new AFQVolControl(this, source, true, vertical);
    vol->EnableSlider(!SourceVolumeLocked(source));

    double meterDecayRate =
        config_get_double(confManager.GetBasic(), "Audio", "MeterDecayRate");

    vol->SetMeterDecayRate(meterDecayRate);

    uint32_t peakMeterTypeIdx =
        config_get_uint(confManager.GetBasic(), "Audio", "PeakMeterType");

    enum obs_peak_meter_type peakMeterType;
    switch (peakMeterTypeIdx) {
    case 0:
        peakMeterType = SAMPLE_PEAK_METER;
        break;
    case 1:
        peakMeterType = TRUE_PEAK_METER;
        break;
    default:
        peakMeterType = SAMPLE_PEAK_METER;
        break;
    }

    vol->setPeakMeterType(peakMeterType);
    vol->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(vol, &QWidget::customContextMenuRequested, this,
        &AFMainDynamicComposit::VolControlContextMenu);
    connect(vol, &AFQVolControl::ConfigClicked, this,
        &AFMainDynamicComposit::VolControlContextMenu);

    InsertQObjectByName(volumes, vol);

    for (auto volume : volumes) {
        if (vertical) {
            m_qAudioMixerDockWidget->GetUIPtr()->horizontalLayout->addWidget(volume);
        }
        else {
            m_qAudioMixerDockWidget->GetUIPtr()->verticalLayout->addWidget(volume);
        }
    }

    RefreshVolumeColors();
}


void AFMainDynamicComposit::qslotDeactivateAudioSource(OBSSource source)
{
    for (size_t i = 0; i < volumes.size(); i++) {
        if (volumes[i]->GetSource() == source) 
        {
            delete volumes[i];
            volumes.erase(volumes.begin() + i);
            break;
        }
    }

    qslotSetMainAudioSource();
}

void AFMainDynamicComposit::qslotHideAudioControl()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* source = vol->GetSource();

    if (!_SourceMixerHidden(source)) {
        _SetSourceMixerHidden(source, true);

        /* Due to a bug with QT 6.2.4, the version that's in the Ubuntu
        * 22.04 ppa, hiding the audio mixer causes a crash, so defer to
        * the next event loop to hide it. Doesn't seem to be a problem
        * with newer versions of QT. */
        QMetaObject::invokeMethod(this, "qslotDeactivateAudioSource",
            Qt::QueuedConnection,
            Q_ARG(OBSSource, OBSSource(source)));
    }
}

void AFMainDynamicComposit::qslotUnhideAllAudioControls()
{
    auto UnhideAudioMixer = [this](obs_source_t* source) /* -- */
        {
            if (!obs_source_active(source))
                return true;
            if (!_SourceMixerHidden(source))
                return true;

            _SetSourceMixerHidden(source, false);
            qslotActivateAudioSource(source);
            return true;
        };

    using UnhideAudioMixer_t = decltype(UnhideAudioMixer);

    auto PreEnum = [](void* data, obs_source_t* source) -> bool /* -- */
        {
            return (*reinterpret_cast<UnhideAudioMixer_t*>(data))(source);
        };

    obs_enum_sources(PreEnum, &UnhideAudioMixer);
    qslotSetMainAudioSource();
}

void AFMainDynamicComposit::qslotLockVolumeControl(bool lock)
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* source = vol->GetSource();

    OBSDataAutoRelease priv_settings =
        obs_source_get_private_settings(source);
    obs_data_set_bool(priv_settings, "volume_locked", lock);

    vol->EnableSlider(!lock);
    qslotSetMainAudioSource();
}

void AFMainDynamicComposit::qslotMixerRenameSource()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    OBSSource source = vol->GetSource();

    const char* prevName = obs_source_get_name(source);

    for (;;) {
        std::string name;
        bool accepted = AFQNameDialog::AskForName(
            this, AFLocaleTextManager::GetSingletonInstance().Str("Basic.Main.MixerRename.Title"),
            AFLocaleTextManager::GetSingletonInstance().Str("Basic.Main.MixerRename.Text"), name,
            QT_UTF8(prevName));
        if (!accepted)
            return;

        if (name.empty()) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, 
                                        this,
                                        "",
                                        AFLocaleTextManager::GetSingletonInstance().Str("NoNameEntered.Text"));
            continue;
        }

        OBSSourceAutoRelease sourceTest =
            obs_get_source_by_name(name.c_str());

        if (sourceTest) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
                                        this, 
                                        "",
                                        AFLocaleTextManager::GetSingletonInstance().Str("NameExists.Text"));
            continue;
        }

        obs_source_set_name(source, name.c_str());
        vol->SetName(QString::fromStdString(name));

        break;
    }
}

void AFMainDynamicComposit::qslotAdvAudioPropertiesTriggered()
{
    if (m_qAdvAudioSettingPopup != nullptr)
    {
        m_qAdvAudioSettingPopup->raise();
        return;
    }
    
    m_qAdvAudioSettingPopup = new AFQAudioAdvSettingDialog(nullptr);
    m_qAdvAudioSettingPopup->show();
}

void AFMainDynamicComposit::qslotAudioMixerCopyFilters()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* source = vol->GetSource();

    m_mixerCopyFiltersSource = obs_source_get_weak_source(source);
    //ui->action_PasteFilters->setEnabled(true);
}

void AFMainDynamicComposit::qslotAudioMixerPasteFilters()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* dstSource = vol->GetSource();

    OBSSourceAutoRelease source =
        obs_weak_source_get_source(m_mixerCopyFiltersSource);

    if (source == dstSource)
        return;

    obs_source_copy_filters(dstSource, source);
}

void AFMainDynamicComposit::qslotGetAudioSourceFilters()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* source = vol->GetSource();

    App()->GetMainView()->CreateFiltersWindow(source);
}

void AFMainDynamicComposit::qslotGetAudioSourceProperties()
{
    QAction* action = reinterpret_cast<QAction*>(sender());
    AFQVolControl* vol = action->property("volControl").value<AFQVolControl*>();
    obs_source_t* source = vol->GetSource();

    App()->GetMainView()->CreatePropertiesWindow(source);
}

bool AFMainDynamicComposit::MainWindowInit()
{
    bool retfirstOpen = false;

    ProfileScope("AFMainDynamicComposit::MainWindowInit");

    CreateObsDisplay(ui->widget_Screen);

    // CoreModel Context Init
    QList<QScreen*> screens = QGuiApplication::screens();
    QScreen* primaryScreen = QGuiApplication::primaryScreen();

    uint32_t cx = primaryScreen->size().width();
    uint32_t cy = primaryScreen->size().height();
    uint32_t cntScreen = (uint32_t)screens.count();

    auto& confManager = AFConfigManager::GetSingletonInstance();
    confManager.InitBasic(cntScreen, cx, cy, devicePixelRatioF());

    AFIconContext::GetSingletonInstance().InitContext();
    audioUtil.ResetAudio();
    videoUtil.ResetVideo();


    // Plug-in load - need graphic thread init -> call plugin
    struct obs_module_failure_info mfi;

#if __APPLE__
    static bool bTestOnce = false;
    if (bTestOnce == false)
    {
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
    
    auto& graphicsContext = AFGraphicsContext::GetSingletonInstance();
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    auto& cefManager = AFCefManager::GetSingletonInstance();
    auto& authManager = AFAuthManager::GetSingletonInstance();
    authManager.LoadAllAuthed();
    sceneContext.InitContext();
    graphicsContext.InitContext();
    cefManager.InitContext();
    cefManager.InitPanelCookieManager();
    g_ViewModelsDynamic.GetInstance().m_RenderModel.SetUnsafeAccessContext(
                                                                           &confManager,
                                                                           &graphicsContext,
                                                                           &sceneContext);


    LoadCustomBrowserList();

    InitDock();
    LoadDockPos();


    //CreateFirstRunAudioSource();
    //audioUtil.CreateFirstRunSources();
    //
    /* load audio monitoring */
    audioUtil.LoadAudioMonitoring();

    InitAFCallbacks();

    auto& hotkey = AFHotkeyContext::GetSingletonInstance();
    hotkey.InitContext();

    auto& loaderSaver = AFLoadSaveManager::GetSingletonInstance();
    
    loaderSaver.SetUIObjMainFrame(App()->GetMainView());
    
    {
        const char* sceneCollection = config_get_string(confManager.GetGlobal(),
                                                        "Basic", "SceneCollectionFile");
        char savePath[1024];
        char fileName[1024];
        int ret;

        if (!sceneCollection)
            throw "Failed to get scene collection name";

        ret = snprintf(fileName, sizeof(fileName),
                   "SOOPStudio/basic/scenes/%s.json", sceneCollection);
        if (ret <= 0)
            throw "Failed to create scene collection file name";

        ret = confManager.GetConfigPath(savePath, sizeof(savePath), fileName);
        if (ret <= 0)
            throw "Failed to get scene collection json file path";
        //
        
        
        
        ProfileScope("AFLoadSaveManager::Load");
        loaderSaver.DecreaseCheckSaveCnt();
        retfirstOpen = loaderSaver.Load(savePath);
        loaderSaver.IncreaseCheckSaveCnt();
    }

    App()->GetMainView()->RefreshSceneCollections();
    App()->GetMainView()->RefreshProfiles();
    loaderSaver.DecreaseCheckSaveCnt();
    

    ToggleMixerLayout(config_get_bool(confManager.GetGlobal(), "BasicWindow",
        "VerticalVolControl"));


    AFArgOption* tmpArgs = confManager.GetArgOption();
    if (tmpArgs != nullptr)
    {
        AFStateAppContext* tmpSateApp = confManager.GetStates();
        if (tmpArgs->GetStudioMode() == false)
        {
            bool previewMode = config_get_bool(confManager.GetGlobal(),
                "BasicWindow",
                "PreviewProgramMode");

            qslotToggleStudioModeBlock(previewMode);
        }
        else
        {
            tmpSateApp->SetPreviewProgramMode(true);
            tmpArgs->SetStudioMode(false);
        }
    }

    UpdatePreviewSafeAreas();
    UpdatePreviewSpacingHelpers();
    UpdatePreviewOverflowSettings();
    
    AFAuth::Load();
      
    {
        int cntOfAccount = authManager.GetCntChannel();
        for (int idx = 0; idx < cntOfAccount; idx++)
        {
            AFChannelData* tmpChannel = nullptr;
            authManager.GetChannelData(idx, tmpChannel);
            
            QPixmap* newObj = App()->GetMainView()->MakePixmapFromAuthData(tmpChannel->pAuthData);
            if (newObj != nullptr) {
                tmpChannel->pObjQtPixmap = newObj;
            }
        }
        
        AFChannelData* tmpMainChannel = nullptr;
        authManager.GetMainChannelData(tmpMainChannel);
        
        if (tmpMainChannel != nullptr)
        {
            QPixmap* newObj = App()->GetMainView()->MakePixmapFromAuthData(tmpMainChannel->pAuthData);
            if (newObj != nullptr) {
                tmpMainChannel->pObjQtPixmap = newObj;
            }
        }
    }
    
    
    

    auto& statusLogManager = AFStatusLogManager::GetSingletonInstance();
    statusLogManager.SendLogProgramStart();

    return retfirstOpen;
}

void AFMainDynamicComposit::CreateObsDisplay(QWidget* previewWidget)
{
    
    AFMainWindowAccesser* tmpViewModels = nullptr;
    tmpViewModels = g_ViewModelsDynamic.UnSafeGetInstace();
    
    if (tmpViewModels)
        tmpViewModels->m_RenderModel.SetMainPreview(ui->widget_Screen);
    
    _InitMainDisplay(tmpViewModels);
}

void AFMainDynamicComposit::InitDock()
{
    _CreateSourceDock();
    _CreateAudioDock();
    _CreateSceneControlDock();
    _CreateAdvanceControlsDock();
    CreateCustomBrowserBlocks();
    //_CreateBroadInfoDock();
}

void AFMainDynamicComposit::ReleaseDock()
{
    if (m_qAFAdvanceControlsDockWidget)
    {
        delete m_qAFAdvanceControlsDockWidget;
        m_qAFAdvanceControlsDockWidget = nullptr;
    }
    
    if (m_qSceneControlDock)
    {
        delete m_qSceneControlDock;
        m_qSceneControlDock = nullptr;
    }
    
    if (m_qAudioMixerDockWidget)
    {
        delete m_qAudioMixerDockWidget;
        m_qAudioMixerDockWidget = nullptr;
    }
    
    if (m_qSceneSourceDock)
    {
        delete m_qSceneSourceDock;
        m_qSceneSourceDock = nullptr;
    }
}

void AFMainDynamicComposit::LoadDockPos()
{
    const char* dockStateStr = config_get_string(
        AFConfigManager::GetSingletonInstance().GetGlobal(), "BasicWindow", "DockState");
    QByteArray dockState =
        QByteArray::fromBase64(QByteArray(dockStateStr));
    bool x = restoreState(dockState);
    
    /*if (!dockStateStr) {
        on_resetDocks_triggered(true);
     }
     else {
        if (!)
            on_resetDocks_triggered(true);
     }
     Restore Dock State*/

}

void AFMainDynamicComposit::RaiseBlocks()
{
    foreach (QWidget* popup, m_qPopupMap)
    {
        popup->raise();
        popup->activateWindow();
        popup->setFocus();
    }
}

void AFMainDynamicComposit::SwitchToDock(const char* key, bool isVisible)
{
    QDockWidget* dock = nullptr;
    bool checkDockExist = _FindDock(key, dock);

    if (dock)
    {
        QPair<QWidget*, QWidget*> block;
        bool checkBlockExist = _FindBlock(key, block);

        block.first->setParent(dock);
        block.second->setParent(dock);

        dock->setTitleBarWidget(block.first);
        dock->setWidget(block.second);
        dock->setVisible(isVisible);
        if (isVisible)
        {
            addDockWidget(Qt::BottomDockWidgetArea, dock);
            dock->setFloating(false);
        }
        else
        {
            dock->setFloating(true);
        }
        QWidget* popup = nullptr;
        bool checkPopup = _FindPopup(key, popup);

        if (popup) {
            delete popup;
            popup = nullptr;
        }

        QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
        int blockTypeNum = BlockTypeEnum.keyToValue(key);
        App()->GetMainView()->qslotBlockToggleTriggered(isVisible, blockTypeNum);
        m_qPopupMap.remove(key);
    }
}

void AFMainDynamicComposit::ToggleDockVisible(BlockType type, bool show)
{
    _ToggleVisibleBlockWindow(type, show);
}

void AFMainDynamicComposit::LoadCustomBrowserList()
{
    m_qCustomBrowserVector.clear();

    config_t* globalconfig = AFConfigManager::GetSingletonInstance().GetGlobal();

    const char* jsonStr = config_get_string(
        globalconfig, "BasicWindow", "ExtraBrowserDocks");

    std::string err;
    json11::Json json = json11::Json::parse(jsonStr, err);
    if (!err.empty())
    {
        return;
    }

    json11::Json::array array = json.array_items();

    for (json11::Json& item : array) {
        std::string title = item["title"].string_value();
        std::string url = item["url"].string_value();
        std::string uuid = item["uuid"].string_value();

        AFQCustomBrowserCollection::CustomBrowserInfo info;
        info.Name = QString::fromStdString(title);
        info.Url = QString::fromStdString(url);
        info.Uuid = QString::fromStdString(uuid);
        info.x = item["x"].int_value();
        info.y = item["y"].int_value();
        info.width = item["width"].int_value();
        info.height = item["height"].int_value();
        info.newCustom = false;

        m_qCustomBrowserVector.push_back(info);
    }
}

void AFMainDynamicComposit::ReloadCustomBrowserList(
    QVector<AFQCustomBrowserCollection::CustomBrowserInfo> info,
    QList<QString> deletedUuid)
{
    m_qCustomBrowserVector = info;

    for (QString uuid : deletedUuid)
    {
        QWidget* widget = m_qBrowserWidgetMap.value(uuid);
        if (widget)
        {
            qslotCloseCustomBrowser(uuid);
            for (int i = 0; i < m_qCustomBrowserVector.size(); i++) {
                if (uuid == m_qCustomBrowserVector[i].Uuid) {
                    m_qCustomBrowserVector.remove(i);
                    break;
                }
            }
        }
    }

    int newCount = 0;

    for (AFQCustomBrowserCollection::CustomBrowserInfo info :
         m_qCustomBrowserVector)
    {
        if (info.newCustom) 
        {
            OpenCustomBrowserBlock(info, newCount);
            info.newCustom = false;
            newCount++;
        }
        else
        {
            foreach(AFQBorderPopupBaseWidget* check, m_qBrowserWidgetMap)
            {
                QString uuid = check->property("uuid").toString();
                if (uuid == info.Uuid)
                {
                    QList<AFDockTitle*> checktitleList = check->findChildren<AFDockTitle*>();
                    if (checktitleList.count() > 0)
                    {
                        checktitleList[0]->ChangeLabelText(info.Name);
                    }
                    check->SetUrl(info.Url.toStdString());
                }
            }
        }
    }
}

void AFMainDynamicComposit::CreateCustomBrowserBlocks()
{
    for (AFQCustomBrowserCollection::CustomBrowserInfo info: m_qCustomBrowserVector)
    {
        if (info.width == 0 || info.height == 0)
        {
            continue;
        }

        AFQBorderPopupBaseWidget* customWidget = new AFQBorderPopupBaseWidget(nullptr);
        customWidget->setObjectName("widget_CustomBrowser");
        customWidget->setStyleSheet("border-radius: 0px");
        customWidget->setWindowFlag(Qt::FramelessWindowHint);
        customWidget->setProperty("uuid", info.Uuid);
        customWidget->SetIsCustom(true);
        customWidget->setAttribute(Qt::WA_NativeWindow);
        connect(customWidget, &AFQBorderPopupBaseWidget::qsignalCloseWidget,
            this, &AFMainDynamicComposit::qslotClosedCustomBrowser);
        customWidget->setMinimumSize(QSize(720, 620));

        AFDockTitle* dockTitle = new AFDockTitle(customWidget);
        dockTitle->Initialize(info.Name);
        dockTitle->setProperty("uuid", info.Uuid);
        dockTitle->setObjectName("CustomBrowserDockTitle");
        connect(dockTitle, &AFDockTitle::qsignalCustomBrowserClose,
            this, &AFMainDynamicComposit::qslotCloseCustomBrowser);
        connect(dockTitle, &AFDockTitle::qsignalCustomBrowserMaximize,
            this, &AFMainDynamicComposit::qslotMaximizeBrowser);
        connect(dockTitle, &AFDockTitle::qsignalCustomBrowserMinimize,
            this, &AFMainDynamicComposit::qslotMinimizeBrowser);

        connect(customWidget, &AFCQMainBaseWidget::qsignalBaseWindowMaximized,
                dockTitle, &AFDockTitle::qslotChangeMaximizeIcon);

        auto& cefManager = AFCefManager::GetSingletonInstance();
        QCefWidget* cefWidget = cefManager.GetCef()->
            create_widget(customWidget,
                info.Url.toStdString(),
                cefManager.GetCefCookieManager());
        customWidget->AddWidget(dockTitle);
        customWidget->AddCustomWidget(cefWidget);

        customWidget->setGeometry(QRect(info.x, info.y, info.width, info.height));
        customWidget->show();

        m_qBrowserWidgetMap.insert(info.Uuid, customWidget);
    }
}

void AFMainDynamicComposit::OpenCustomBrowserBlock(AFQCustomBrowserCollection::CustomBrowserInfo info, int overlapCount)
{
    AFQBorderPopupBaseWidget* customWidget = new AFQBorderPopupBaseWidget(nullptr);
    customWidget->setObjectName("widget_CustomBrowser");
    customWidget->setStyleSheet("border-radius: 0px");
    customWidget->setWindowFlag(Qt::FramelessWindowHint);
    customWidget->setProperty("uuid", info.Uuid);
    customWidget->SetIsCustom(true);
    customWidget->setMinimumSize(QSize(720, 620));
    customWidget->setAttribute(Qt::WA_NativeWindow);
    connect(customWidget, &AFQBorderPopupBaseWidget::qsignalCloseWidget,
        this, &AFMainDynamicComposit::qslotClosedCustomBrowser);

    AFDockTitle* dockTitle = new AFDockTitle(customWidget);
    dockTitle->Initialize(info.Name);
    dockTitle->setProperty("uuid", info.Uuid);
    dockTitle->setObjectName("CustomBrowserDockTitle");
    connect(dockTitle, &AFDockTitle::qsignalCustomBrowserClose,
        this, &AFMainDynamicComposit::qslotCloseCustomBrowser);
    connect(dockTitle, &AFDockTitle::qsignalCustomBrowserMaximize,
        this, &AFMainDynamicComposit::qslotMaximizeBrowser);
    connect(dockTitle, &AFDockTitle::qsignalCustomBrowserMinimize,
        this, &AFMainDynamicComposit::qslotMinimizeBrowser);

    connect(customWidget, &AFCQMainBaseWidget::qsignalBaseWindowMaximized,
            dockTitle, &AFDockTitle::qslotChangeMaximizeIcon);

    auto& cefManager = AFCefManager::GetSingletonInstance();
    QCefWidget* cefWidget = cefManager.GetCef()->
        create_widget(customWidget,
            info.Url.toStdString(),
            cefManager.GetCefCookieManager());
    customWidget->AddWidget(dockTitle);
    customWidget->AddCustomWidget(cefWidget);

    int posX = App()->GetMainView()->pos().x() - 720;
    int posY = App()->GetMainView()->pos().y() + overlapCount * dockTitle->height();


    QRect dockPopupPosition = QRect(posX, posY, 720, 620);
    QRect adjustRect;
    AdjustPositionOutSideScreen(dockPopupPosition, adjustRect);
    customWidget->setGeometry(adjustRect);
    customWidget->show();

    m_qBrowserWidgetMap.insert(info.Uuid, customWidget);
}

void AFMainDynamicComposit::SaveCustomBrowserList()
{
    config_t* globalconfig = AFConfigManager::GetSingletonInstance().GetGlobal();
    json11::Json::array array;

    for (AFQCustomBrowserCollection::CustomBrowserInfo info :
         m_qCustomBrowserVector)
    {
        QString title = info.Name;
        QString url = info.Url;
        QString uuid = info.Uuid;
        QWidget* widget = m_qBrowserWidgetMap.value(info.Uuid);

        QRect rect = QRect(0,0,0,0);

        if (widget)
        {
            rect = widget->geometry();
        }

        json11::Json::object obj{
            {"title", QT_TO_UTF8(title)},
            { "url", QT_TO_UTF8(url) },
            { "uuid", QT_TO_UTF8(uuid) },
            { "x", rect.x() },
            { "y", rect.y() },
            { "width", rect.width() },
            { "height", rect.height() }
        };
        array.push_back(obj);
    }

    foreach(QWidget * widget, m_qBrowserWidgetMap)
    {
        if (widget->isWidgetType())
        {
            widget->close();
            widget->deleteLater();
            widget = nullptr;
        }
    }

    std::string output = json11::Json(array).dump();
    config_set_string(globalconfig, "BasicWindow",
        "ExtraBrowserDocks", output.c_str());
    config_save_safe(globalconfig, "tmp", nullptr);
}

AFQCustomMenu* AFMainDynamicComposit::CreateCustomBrowserMenu()
{
    AFLocaleTextManager& localeManager = AFLocaleTextManager::GetSingletonInstance();

    AFQCustomMenu* customBrowserMenu = new AFQCustomMenu(localeManager.Str("Basic.MainMenu.Addon.CustomBrowserDocks"), 
        this, true);
    customBrowserMenu->setFixedWidth(200);

    QAction* browserList = new QAction(customBrowserMenu);
    browserList->setText(localeManager.Str("Basic.MainMenu.Addon.CustomBrowserList"));
    connect(browserList, &QAction::triggered, this, &AFMainDynamicComposit::qslotOpenCustomBrowserCollection);
    
    customBrowserMenu->addAction(browserList);
    customBrowserMenu->addSeparator();

    for (AFQCustomBrowserCollection::CustomBrowserInfo info: m_qCustomBrowserVector)
    {
        QAction* customBrowser = new QAction(customBrowserMenu);
        customBrowser->setProperty("uuid", info.Uuid);
        customBrowser->setText(info.Name);
        customBrowser->setCheckable(true);

        customBrowser->setChecked(m_qBrowserWidgetMap.contains(info.Uuid));

        connect(customBrowser, &QAction::triggered, this, &AFMainDynamicComposit::qslotOpenCustomBrowser);
        customBrowserMenu->addAction(customBrowser);
    }

    return customBrowserMenu;
}

AFBasicPreview* AFMainDynamicComposit::GetMainPreview()
{
    return ui->widget_Screen;
}

QFrame* AFMainDynamicComposit::GetNotPreviewFrame()
{
    return ui->frame_notPreview;
}

AFMainDynamicComposit* AFMainDynamicComposit::Get()
{
    return reinterpret_cast<AFMainDynamicComposit*>(App()->GetMainView()->GetMainWindow());
}

void AFMainDynamicComposit::InitAFCallbacks()
{
    signalHandlers.reserve(signalHandlers.size() + 7);

    signalHandlers.emplace_back(obs_get_signal_handler(), "source_create",
        AFMainDynamicComposit::SourceCreated, this);
    signalHandlers.emplace_back(obs_get_signal_handler(), "source_remove",
        AFMainDynamicComposit::SourceRemoved, this);
    signalHandlers.emplace_back(obs_get_signal_handler(), "source_activate",
        AFMainDynamicComposit::SourceActivated, this);
    signalHandlers.emplace_back(obs_get_signal_handler(), "source_deactivate",
        AFMainDynamicComposit::SourceDeactivated, this);
    signalHandlers.emplace_back(obs_get_signal_handler(), "source_audio_activate",
        AFMainDynamicComposit::SourceAudioActivated, this);
    signalHandlers.emplace_back(obs_get_signal_handler(), "source_audio_deactivate",
        AFMainDynamicComposit::SourceAudioDeactivated, this);
    signalHandlers.emplace_back(obs_get_signal_handler(), "source_rename",
        AFMainDynamicComposit::SourceRenamed, this);

}

void AFMainDynamicComposit::SetDockedValues()
{
    foreach(QDockWidget* dock, m_qDockMap)
    {
        if (dock->isVisible())
        {
            AFDockTitle* ToggleDockButton = reinterpret_cast<AFDockTitle*>(dock->titleBarWidget());

            QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
            int blockType = BlockTypeEnum.keyToValue(ToggleDockButton->GetBlockType());

            emit qsignalBlockVisibleToggled(true, blockType);

            if (dock->parentWidget() == this)
            {
                ToggleDockButton->SetToggleWindowToDockButton(true);
            }
        }
    }
    for (auto [key, value] : m_qPopupMap.asKeyValueRange())
    {
        QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
        int blockType = BlockTypeEnum.keyToValue(key);
        emit qsignalBlockVisibleToggled(true, blockType);
    }
}

void AFMainDynamicComposit::_CreateSourceDock()
{
    if (m_qSceneSourceDock == nullptr)
    {
        m_qSceneSourceDock = new AFSceneSourceDockWidget(this);

        //Move Resize Dont Work on Transparent
        //m_qSceneSourceDock->setAttribute(Qt::WA_TranslucentBackground);

        m_qSceneSourceDock->setAllowedAreas(Qt::DockWidgetArea::BottomDockWidgetArea);
        /*connect(m_qSceneSourceDock, &QDockWidget::dockLocationChanged,
            this, &AFMainDynamicComposit::qslotDockLocationChanged);*/

        connect(m_qSceneSourceDock, &AFSceneSourceDockWidget::qSignalSceneDoubleClickedTriggered,
            this, &AFMainDynamicComposit::qslotChangeSceneOnDoubleClick);

        QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
        const char* key = BlockTypeEnum.valueToKey(BlockType::SceneSource);
        m_qSceneSourceDock->setObjectName(key);

        AFDockTitle* sourceTitle = new AFDockTitle(m_qSceneSourceDock);
        sourceTitle->Initialize(true, Str("Basic.SceneSourceDock.Title"), key);
        m_qSceneSourceDock->setTitleBarWidget(sourceTitle);
        connect(sourceTitle, &AFDockTitle::qsignalToggleDock, this, &AFMainDynamicComposit::qslotChangeDockToWindow);
        connect(sourceTitle, &AFDockTitle::qsignalClose, this, &AFMainDynamicComposit::qslotCloseBlock);
        connect(sourceTitle, &AFDockTitle::qsignalMaximumPopup, this, &AFMainDynamicComposit::qslotMaximumBlock);
        connect(sourceTitle, &AFDockTitle::qsignalMinimumPopup, this, &AFMainDynamicComposit::qslotMinimumBlock);


        connect(sourceTitle, &AFDockTitle::qsignalTransitionScenePopup, this, &AFMainDynamicComposit::qslotTransitionSceneTriggered);
        connect(sourceTitle, &AFDockTitle::qsignalShowSceneControlDock, this, &AFMainDynamicComposit::qslotShowSceneControlDockTriggered);

        connect(m_qSceneSourceDock, &AFSceneSourceDockWidget::qSignalAddScene,
                this, &AFMainDynamicComposit::qslotAddSceneTriggered);

        connect(m_qSceneSourceDock, &AFSceneSourceDockWidget::qSignalAddSource,
                this, &AFMainDynamicComposit::qslotAddSourceTriggered);

        m_qSceneSourceDock->setVisible(false);

        QPair<QWidget*, QWidget*> sourcelist;
        sourcelist.first = sourceTitle;
        sourcelist.second = m_qSceneSourceDock->widget();
        m_qBlockMap.insert(key, sourcelist);

        m_qDockMap.insert(key, m_qSceneSourceDock);

        _RestorePopupBlock(key);
    }
}

void AFMainDynamicComposit::_CreateAudioDock()
{
    if (m_qAudioMixerDockWidget == nullptr)
    {
        m_qAudioMixerDockWidget = new AFAudioMixerDockWidget(this);

        //Move Resize Dont Work on Transparent
        //m_qAudioMixerDockWidget->setAttribute(Qt::WA_TranslucentBackground);

        m_qAudioMixerDockWidget->setAllowedAreas(Qt::DockWidgetArea::BottomDockWidgetArea);

        /*connect(m_qAudioMixerDockWidget, &QDockWidget::dockLocationChanged,
            this, &AFMainDynamicComposit::qslotDockLocationChanged);*/

        //addDockWidget(Qt::BottomDockWidgetArea, m_qAudioMixerDockWidget);

        //QPushButton* detachAudioMixerDock = new QPushButton(m_qAudioMixerDockWidget);
        ////connect(addSource, &QPushButton::clicked, this, &AFMainDynamicComposit::onAddSourceButtonTriggered);
        //detachAudioMixerDock->setObjectName("pushbutton_DetachAudioDock");

        //QPushButton* closeAudioMixerDock = new QPushButton(m_qAudioMixerDockWidget);
        ////connect(addSource, &QPushButton::clicked, this, &AFMainDynamicComposit::onAddSourceButtonTriggered);
        //closeAudioMixerDock->setObjectName("pushbutton_CloseAudioDock");

        QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
        const char* key = BlockTypeEnum.valueToKey(BlockType::AudioMixer);

        AFDockTitle* audioMixerDockTitle = new AFDockTitle(m_qAudioMixerDockWidget);
        audioMixerDockTitle->Initialize(true, AFLocaleTextManager::GetSingletonInstance().Str("Mixer"), key);
        //audioMixerDockTitle->setObjectName("dockTitle_AudioMixer");
        m_qAudioMixerDockWidget->setTitleBarWidget(audioMixerDockTitle);
        
        connect(audioMixerDockTitle, &AFDockTitle::qsignalToggleDock, this, &AFMainDynamicComposit::qslotChangeDockToWindow);
        connect(audioMixerDockTitle, &AFDockTitle::qsignalClose, this, &AFMainDynamicComposit::qslotCloseBlock);

        connect(audioMixerDockTitle, &AFDockTitle::qsignalMaximumPopup, this, &AFMainDynamicComposit::qslotMaximumBlock);
        connect(audioMixerDockTitle, &AFDockTitle::qsignalMinimumPopup, this, &AFMainDynamicComposit::qslotMinimumBlock);

        m_qAudioMixerDockWidget->SetStyle();
        m_qAudioMixerDockWidget->setVisible(false);

        m_qAudioMixerDockWidget->GetUIPtr()->hMixerScrollArea->setContextMenuPolicy(Qt::CustomContextMenu);
        m_qAudioMixerDockWidget->GetUIPtr()->vMixerScrollArea->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_qAudioMixerDockWidget->GetUIPtr()->hMixerScrollArea, &QWidget::customContextMenuRequested, 
                this, &AFMainDynamicComposit::qslotStackedMixerAreaContextMenuRequested);
        connect(m_qAudioMixerDockWidget->GetUIPtr()->vMixerScrollArea, &QWidget::customContextMenuRequested,
            this, &AFMainDynamicComposit::qslotStackedMixerAreaContextMenuRequested);

        QPair<QWidget*, QWidget*> audio;
        audio.first = audioMixerDockTitle;
        audio.second = m_qAudioMixerDockWidget->widget();
        m_qBlockMap.insert(key, audio);

        m_qDockMap.insert(key, m_qAudioMixerDockWidget);

        _RestorePopupBlock(key);
    }
}

void AFMainDynamicComposit::_CreateSceneControlDock()
{
    if (m_qSceneControlDock == nullptr)
    {
        m_qSceneControlDock = new AFSceneControlDockWidget(this);
        m_qSceneControlDock->setAllowedAreas(Qt::DockWidgetArea::BottomDockWidgetArea);

        /*connect(m_qSceneControlDock, &QDockWidget::dockLocationChanged,
            this, &AFMainDynamicComposit::qslotDockLocationChanged);*/

        QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
        const char* key = BlockTypeEnum.valueToKey(BlockType::SceneControl);

        AFDockTitle* sceneControlTitle = new AFDockTitle(m_qSceneControlDock);
        sceneControlTitle->Initialize(false,
           QT_UTF8(AFLocaleTextManager::GetSingletonInstance().
                    Str("Basic.Settings.General.Multiview")), key);
        sceneControlTitle->setProperty("widgetType", "noradius");
        m_qSceneControlDock->setTitleBarWidget(sceneControlTitle);
        
        connect(sceneControlTitle, &AFDockTitle::qsignalToggleDock,
                this, &AFMainDynamicComposit::qslotChangeDockToWindow);
        connect(sceneControlTitle, &AFDockTitle::qsignalClose,
                this, &AFMainDynamicComposit::qslotCloseBlock);
      
        m_qSceneControlDock->SetStyle();
        m_qSceneControlDock->setVisible(false);

        QPair<QWidget*, QWidget*> sceneControl;
        sceneControl.first = sceneControlTitle;
        sceneControl.second = m_qSceneControlDock->widget();
        m_qBlockMap.insert(key, sceneControl);

        m_qDockMap.insert(key, m_qSceneControlDock);

        _RestorePopupBlock(key);
    }
}
//

void AFMainDynamicComposit::_CreateAdvanceControlsDock()
{
    if (m_qAFAdvanceControlsDockWidget == nullptr)
    {
        m_qAFAdvanceControlsDockWidget = new AFAdvanceControlsDockWidget(this);
        m_qAFAdvanceControlsDockWidget->setAllowedAreas(Qt::DockWidgetArea::BottomDockWidgetArea);

        /*connect(m_qAFAdvanceControlsDockWidget, &QDockWidget::dockLocationChanged,
            this, &AFMainDynamicComposit::qslotDockLocationChanged);*/

        QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
        const char* key = BlockTypeEnum.valueToKey(BlockType::AdvanceControls);

        AFDockTitle* advanceControlsTitle = new AFDockTitle(m_qAFAdvanceControlsDockWidget);
        advanceControlsTitle->Initialize(false,
           QT_UTF8(AFLocaleTextManager::GetSingletonInstance().
                    Str("Basic.MainMenu.Tools.AdvControls")), key);
        m_qAFAdvanceControlsDockWidget->setTitleBarWidget(advanceControlsTitle);
        
        connect(advanceControlsTitle, &AFDockTitle::qsignalToggleDock,
                this, &AFMainDynamicComposit::qslotChangeDockToWindow);
        connect(advanceControlsTitle, &AFDockTitle::qsignalClose,
                this, &AFMainDynamicComposit::qslotCloseBlock);
        
        bool replayBuf = false;
        config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();

        const char* mode = config_get_string(basicConfig, "Output", "Mode");
        if (astrcmpi(mode, "Advanced") == 0) {
            const char* advRecType = config_get_string(basicConfig, "AdvOut", "RecType");
            if (astrcmpi(advRecType, "FFmpeg") == 0)
                replayBuf = false;
            else
                replayBuf = config_get_bool(basicConfig, "AdvOut", "RecRB");
        }
        else
            replayBuf = config_get_bool(basicConfig, "SimpleOutput", "RecRB");
        

        m_qAFAdvanceControlsDockWidget->EnableReplayBuffer(replayBuf);

        m_qAFAdvanceControlsDockWidget->setVisible(false);

        QPair<QWidget*, QWidget*> advanceControls;
        advanceControls.first = advanceControlsTitle;
        advanceControls.second = m_qAFAdvanceControlsDockWidget->widget();
        m_qBlockMap.insert(key, advanceControls);

        m_qDockMap.insert(key, m_qAFAdvanceControlsDockWidget);

        _RestorePopupBlock(key);
    }
}

void AFMainDynamicComposit::_ClearBrowserInterationQMap()
{
    //MAP_BROWSER_INTERACTION::iterator it = m_mapBrowserInteraction.begin();
    //while (it != m_mapBrowserInteraction.end()) {
    //    auto next = it;
    //    ++next;

    //    AFQBrowserInteraction* interaction = (*it);
    //    if (interaction) {
    //        interaction->close();
    //        interaction = nullptr;
    //    }
    //    it = next;
    //}
    m_mapBrowserInteraction.clear();
}

void AFMainDynamicComposit::_ClearDockMap()
{
    //QMap<const char*, QDockWidget*>::iterator it = m_qDockMap.begin();
    //for (; it != m_qDockMap.end(); ++it) {
    //    QDockWidget* dockWidget = (*it);
    //    if (!dockWidget)
    //        continue;

    //    dockWidget->close();
    //    dockWidget = nullptr;
    //}
    m_qDockMap.clear();
}

void AFMainDynamicComposit::_ClearPopupMap()
{
    //QMap<const char*, QWidget*>::iterator it = m_qPopupMap.begin();
    //for (; it != m_qPopupMap.end(); ++it) {
    //    QWidget* widget = (*it);
    //    if (!widget)
    //        continue;

    //    widget->close();
    //    widget = nullptr;
    //}
    m_qPopupMap.clear();
}

void AFMainDynamicComposit::_ClearBlockMap()
{
    m_qBlockMap.clear();
}

AFQVolControl* AFMainDynamicComposit::_CompareAudio(AFQVolControl* audio1, AFQVolControl* audio2)
{
    if (audio1 == nullptr)
        return audio2;
    if (audio2 == nullptr)
        return audio1;

    // if current volume locked
    if (SourceVolumeLocked(audio1->GetSource())) {
        if (SourceVolumeLocked(audio2->GetSource())) {
            if ((audio1->IsMuted() == true && audio2->IsMuted()) == true ||
                (audio1->IsMuted() == false && audio2->IsMuted() == false))
            {
                audio1 = (audio1->GetVolume() > audio2->GetVolume()) ? audio1 : audio2;
            }
            else
                audio1 = (audio1->IsMuted() == false) ? audio1 : audio2;
        }
        else
            audio1 = audio2;
    }
    else
    {
        if (SourceVolumeLocked(audio2->GetSource()))
            return audio1;
        if ((audio1->IsMuted() == true && audio2->IsMuted()) == true ||
            (audio1->IsMuted() == false && audio2->IsMuted() == false))
        {
            audio1 = (audio1->GetVolume() > audio2->GetVolume()) ? audio1 : audio2;
        }
        else
            audio1 = (audio1->IsMuted() == false) ? audio1 : audio2;
    }

    return audio1;
}

//void AFMainDynamicComposit::_CreateBroadInfoDock()
//{
//    if (m_qBroadInfoDock)
//    {
//        m_qBroadInfoDock = new AFBroadInfoDockWidget(this);
//        m_qBroadInfoDock->setAllowedAreas(Qt::DockWidgetArea::BottomDockWidgetArea);
//
//        /*connect(m_qBroadInfoDock, &QDockWidget::dockLocationChanged,
//            this, &AFMainDynamicComposit::qslotDockLocationChanged);*/
//
//        QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
//        const char* key = BlockTypeEnum.valueToKey(BlockType::Dashboard);
//        m_qBroadInfoDock->setObjectName(key);
//
//        AFDockTitle* broadInfoTitle = new AFDockTitle(this);
//        broadInfoTitle->ChangeLabelText(QT_UTF8("대시보드"), key);
//        m_qBroadInfoDock->setTitleBarWidget(broadInfoTitle);
//        connect(broadInfoTitle, &AFDockTitle::qsignalToggleDock, this, &AFMainDynamicComposit::qslotChangeDockToWindow);
//        connect(broadInfoTitle, &AFDockTitle::qsignalClose, this, &AFMainDynamicComposit::qslotCloseBlock);
//
//        m_qBroadInfoDock->setVisible(false);
//
//        QPair<QWidget*, QWidget*> sourcelist;
//        sourcelist.first = broadInfoTitle;
//        sourcelist.second = m_qBroadInfoDock->widget();
//        m_qBlockMap.insert(key, sourcelist);
//
//        m_qDockMap.insert(key, m_qBroadInfoDock);
//
//        _RestorePopupBlock(key);
//    }
//}


void AFMainDynamicComposit::_RestorePopupBlock(const char* key)
{
    std::string check = QString("%1Popup").arg(key).toStdString();

    bool isDockPopup = config_get_bool(AFConfigManager::GetSingletonInstance().GetGlobal(),
        "BasicWindow", check.c_str());

    if (strcmp(key, "SceneControl") == 0)
        isDockPopup = false;

    if (isDockPopup)
    {
        _MakePopupForDock(key, false);

        const char* PopupPosition = config_get_string(AFConfigManager::GetSingletonInstance().GetGlobal(),
            "BasicWindow", key);
        QByteArray byteArray =
            QByteArray::fromBase64(QByteArray(PopupPosition));

        if (PopupPosition != NULL) {
            QByteArray byteArray =
                QByteArray::fromBase64(QByteArray(PopupPosition));
            m_qPopupMap[key]->restoreGeometry(byteArray);

            QRect windowGeometry = normalGeometry();
            if (!WindowPositionValid(windowGeometry)) {
                QRect rect =
                    QGuiApplication::primaryScreen()->geometry();
                setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                    Qt::AlignCenter, size(),
                    rect));
            }

            QPair<QWidget*, QWidget*> block;
            bool checkBlockExist = _FindBlock(key, block);
            if (checkBlockExist) {
                AFDockTitle* dockTitle = qobject_cast<AFDockTitle*>(block.first);
                if (dockTitle)
                    dockTitle->ChangeMaximizedIcon(m_qPopupMap[key]->isMaximized());
            }
        }
    }
}

void AFMainDynamicComposit::CreateDefaultScene(bool firstStart)
{
    auto& locale = AFLocaleTextManager::GetSingletonInstance();
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    auto& loadSaver = AFLoadSaveManager::GetSingletonInstance();

    loadSaver.IncreaseCheckSaveCnt();

    MAINFRAME->ClearSceneData();
    sceneContext.InitContext();
    sceneContext.InitDefaultTransition();

    AFSceneUtil::SetTransition(sceneContext.GetFadeTransition());

    if (firstStart)
        audioUtil.CreateFirstRunSources();

    OBSSceneAutoRelease firstScene;
    for (int i = 0; i < 1; i++) {
        obs_scene_t* scene = obs_scene_create(Str("Basic.DefaultScene"));
        
        if (0 == i)
            firstScene = scene;
    }
    SetCurrentScene(firstScene, true);

    loadSaver.DecreaseCheckSaveCnt();
}

void AFMainDynamicComposit::AddDefaultGuideImage()
{
     OBSSource guideSource;

#ifdef _WIN32
    const char* id = "image_source";
#else
    const char* id = "image_source";
#endif

    QString displayText = AFSourceUtil::GetPlaceHodlerText(id);

    obs_transform_info oti;
    {
        oti.pos.x = 0.0f;
        oti.pos.y = 0.0f;
        oti.rot = 0.0f;
        oti.bounds.x = 1.0f;
        oti.bounds.y = 1.0f;
        oti.alignment = 5;
        oti.bounds_type = OBS_BOUNDS_NONE;
        oti.bounds_alignment = 0;
        oti.scale.x = 2.18181825f;
        oti.scale.y = 2.18181825f;
    }

    if (!AFSourceUtil::AddNewSource(this, id, "Guide Image", true, guideSource, &oti))
        return;

    OBSDataAutoRelease nd_settings = obs_source_get_settings(guideSource);

    std::string path;
    GetDataFilePath("assets/preview/Default-screen.png", path);
    obs_data_set_string(nd_settings, "file", path.c_str());

    obs_source_update(guideSource, nd_settings);
}

void AFMainDynamicComposit::AddScene(OBSSource source)
{ 
    // [Refresh Scene Source Dock UI]
    m_qSceneSourceDock->AddScene(source);

    // [Refresh MainFrame Scene Button UI]
    App()->GetMainView()->RefreshSceneUI();

    //SaveProject();

    //if (!disableSaving) {
    //    obs_source_t* source = obs_scene_get_source(scene);
    //    blog(LOG_INFO, "User added scene '%s'",
    //        obs_source_get_name(source));

    //    OBSProjector::UpdateMultiviewProjectors();
    //}

    //if (api)
    //    api->on_event(OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED);
}

void AFMainDynamicComposit::RemoveScene(OBSSource source)
{
    // [Refresh Scene Source Dock UI]
    m_qSceneSourceDock->RemoveScene(source);

    // [Refresh MainFrame Scene Button UI]
    App()->GetMainView()->RefreshSceneUI();

}

void AFMainDynamicComposit::TransitionStopped() {
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    OBSWeakSource swapScene = sceneContext.GetSwapOBSScene();
    if (swapScene) {
        OBSSource scene = OBSGetStrongRef(swapScene);
        if (scene)
            SetCurrentScene(scene);
    }
    //EnableTransitionWidgets(true);
    //UpdatePreviewProgramIndicators();

    //if (api) {
    //    api->on_event(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
    //    api->on_event(OBS_FRONTEND_EVENT_SCENE_CHANGED);
    //}

    sceneContext.SetSwapOBSScene(nullptr);
}

void AFMainDynamicComposit::qslotRenameSources(OBSSource source, QString newName, QString prevName)
{
    //RenameListValues(ui->scenes, newName, prevName);

    for (size_t i = 0; i < volumes.size(); i++) {
        if (volumes[i]->GetName().compare(prevName) == 0)
            volumes[i]->SetName(newName);
    }

    /*for (size_t i = 0; i < projectors.size(); i++) {
        if (projectors[i]->GetSource() == source)
            projectors[i]->RenameProjector(prevName, newName);
    }

    if (vcamConfig.type == VCamOutputType::SourceOutput &&
        prevName == QString::fromStdString(vcamConfig.source))
        vcamConfig.source = newName.toStdString();
    if (vcamConfig.type == VCamOutputType::SceneOutput &&
        prevName == QString::fromStdString(vcamConfig.scene))
        vcamConfig.scene = newName.toStdString();

    SaveProject();

    obs_scene_t* scene = obs_scene_from_source(source);
    if (scene)
        OBSProjector::UpdateMultiviewProjectors();

    UpdateContextBar();
    UpdatePreviewProgramIndicators();*/
}

void AFMainDynamicComposit::qslotOpenCustomBrowserCollection()
{
    if (m_qBrowserCollection)
    {
        m_qBrowserCollection->raise();
    }
    else
    {
        m_qBrowserCollection = new AFQCustomBrowserCollection(nullptr);
        m_qBrowserCollection->CustomBrowserCollectionInit(
            m_qCustomBrowserVector);
        m_qBrowserCollection->show();
    }
}

void AFMainDynamicComposit::qslotOpenCustomBrowser(bool checked)
{
    QAction* custombrowserAction = reinterpret_cast<QAction*>(sender());

    QString uuid = custombrowserAction->property("uuid").toString();

    if (checked)
    {
        for (int i = 0; i < m_qCustomBrowserVector.size(); i++) {
            if (uuid == m_qCustomBrowserVector[i].Uuid) {
                OpenCustomBrowserBlock(m_qCustomBrowserVector[i]);
                break;
            }
        }
    }
    else
    {
        QString uuid = custombrowserAction->property("uuid").toString();
        QWidget* widget = m_qBrowserWidgetMap.value(uuid);
        if (widget)
        {
            widget->close();
            delete widget;
            widget = nullptr;

            m_qBrowserWidgetMap.remove(uuid);
        }
    }
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
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    
    if (vertical)
    {
        AFQVerticalProgramView* studioVerticalModeView = GetVerticalStudioModeViewLayout();
        if (studioVerticalModeView != nullptr)
        {
            OBSSource previewSrc = AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrOBSScene());
            OBSSource programSrc = OBSGetStrongRef(sceneContext.GetProgramOBSScene());

            studioVerticalModeView->ChangeEditSceneName(QT_UTF8(obs_source_get_name(previewSrc)));
            studioVerticalModeView->ChangeLiveSceneName(QT_UTF8(obs_source_get_name(programSrc)));
        }
    }
    else
    {
        AFQProgramView* studioModeView = GetStudioModeViewLayout();
        if (studioModeView != nullptr)
        {
            OBSSource previewSrc = AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrOBSScene());
            OBSSource programSrc = OBSGetStrongRef(sceneContext.GetProgramOBSScene());

            studioModeView->ChangeEditSceneName(QT_UTF8(obs_source_get_name(previewSrc)));
            studioModeView->ChangeLiveSceneName(QT_UTF8(obs_source_get_name(programSrc)));
        }
    }
    //
}

void AFMainDynamicComposit::EnableReplayBuffer(bool enable)
{
    if(m_qAFAdvanceControlsDockWidget) {
        m_qAFAdvanceControlsDockWidget->EnableReplayBuffer(enable);
    }
}

void AFMainDynamicComposit::SetReplayBufferStartStopMode(bool bufferStart)
{
    if (m_qAFAdvanceControlsDockWidget) {
        SetReplayBufferReleased();
        m_qAFAdvanceControlsDockWidget->SetReplayBufferStartStopStyle(bufferStart);
    }
}

void AFMainDynamicComposit::SetReplayBufferStoppingMode()
{
    if (m_qAFAdvanceControlsDockWidget) {
        m_qAFAdvanceControlsDockWidget->SetReplayBufferStoppingStyle();
    }
}

void AFMainDynamicComposit::SetReplayBufferReleased()
{
    if (m_qAFAdvanceControlsDockWidget) {
        m_qAFAdvanceControlsDockWidget->qslotReleasedReplayBuffer();
    }
}

void AFMainDynamicComposit::UpdatePreviewSafeAreas()
{
    bool drawSafeAreas = config_get_bool(AFConfigManager::GetSingletonInstance().GetGlobal(), 
        "BasicWindow",
        "ShowSafeAreas");

    ui->widget_Screen->SetShowSafeAreas(drawSafeAreas);
}

void AFMainDynamicComposit::UpdatePreviewSpacingHelpers()
{
    bool drawSpacingHelpers = config_get_bool(AFConfigManager::GetSingletonInstance().GetGlobal(), 
        "BasicWindow",
        "SpacingHelpersEnabled");

    ui->widget_Screen->SetDrawSpacingHelpers(drawSpacingHelpers);
}

void AFMainDynamicComposit::UpdatePreviewOverflowSettings()
{
    auto globalconfig = AFConfigManager::GetSingletonInstance().GetGlobal();

    bool hidden = config_get_bool(globalconfig, "BasicWindow",
        "OverflowHidden");
    bool select = config_get_bool(globalconfig, "BasicWindow",
        "OverflowSelectionHidden");
    bool always = config_get_bool(globalconfig, "BasicWindow",
        "OverflowAlwaysVisible");

    ui->widget_Screen->SetOverflowHidden(hidden);
    ui->widget_Screen->SetOverflowSelectionHidden(select);
    ui->widget_Screen->SetOverflowAlwaysVisible(always);
}

void AFMainDynamicComposit::SwitchStudioModeLayout(bool vertical)
{
    AFStateAppContext* tmpStateApp = AFConfigManager::GetSingletonInstance().GetStates();

    if (tmpStateApp->IsPreviewProgramMode() == false)
        return;

    if (vertical)
    {
        _DestroyHorizontalStudioMode();
        _MakeVerticalStudioMode();
    }
    else
    {
        _DestroyVerticalStudioMode();
        _MakeHorizontalStudioMode();
    }
}

void AFMainDynamicComposit::ToggleStudioModeLabels(bool vertical, bool visible)
{
    AFStateAppContext* tmpStateApp = AFConfigManager::GetSingletonInstance().GetStates();

    if (tmpStateApp->IsPreviewProgramMode() == false) 
        return;

    if (vertical)
    {
        m_pStudioModeVerticalView->ToggleSceneLabel(visible);
    }
    else
    {
        m_pStudioModeView->ToggleSceneLabel(visible);
    }
}

bool AFMainDynamicComposit::IsMainAudioVolumeControlExist() 
{
    if (!m_MainAudioVolumeControl)
        return false;
    return true;
}

bool AFMainDynamicComposit::IsMainMicVolumeControlExist() 
{
    if (!m_MainMicVolumeControl)
        return false;
    return true;
}

float AFMainDynamicComposit::GetMainAudioVolumePeak() 
{
    if (!m_MainAudioVolumeControl)
        return -96.f;
    return m_MainAudioVolumeControl->GetCurrentPeak();
}

float AFMainDynamicComposit::GetMainMicVolumePeak() 
{
    if (!m_MainMicVolumeControl)
        return -96.f;
    return m_MainMicVolumeControl->GetCurrentPeak();
}

void AFMainDynamicComposit::AdjustPositionOutSideScreen(QRect windowGeometry, QRect& outAdjustGeometry)
{
    QList<QScreen*> screens = QGuiApplication::screens();
    QRect retVal = windowGeometry;

    QRect fullScreenRect;
    for (QScreen* screen : screens) {
        fullScreenRect = fullScreenRect.united(screen->geometry());
    }

    uint32_t orgWidth = windowGeometry.right() - windowGeometry.left();
    uint32_t orgHeight = windowGeometry.bottom() - windowGeometry.top();


    if (windowGeometry.left() <= fullScreenRect.left())
    {
        retVal.setLeft(0);
        retVal.setRight(orgWidth);
    }
    else if (windowGeometry.right() >= fullScreenRect.right())
    {
        retVal.setRight(fullScreenRect.right());
        retVal.setLeft(fullScreenRect.right() - orgWidth);
    }

    if (windowGeometry.top() <= fullScreenRect.top())
    {
        retVal.setTop(fullScreenRect.top());
        retVal.setBottom(orgHeight);
    }
    else if (windowGeometry.bottom() >= fullScreenRect.bottom())
    {
        retVal.setBottom(fullScreenRect.bottom());
        retVal.setTop(fullScreenRect.bottom() - orgHeight);
    }

    outAdjustGeometry = retVal;
}

void AFMainDynamicComposit::SetCurrentScene(OBSSource scene, bool force)
{
    auto& sceneContext = AFSceneContext::GetSingletonInstance();

    if (force && scene) {
         OBSScene obsScene = obs_scene_from_source(scene);
         sceneContext.SetCurrOBSScene(obsScene);
    }
    else {
        OBSScene curScene = sceneContext.GetCurrOBSScene();
        if (scene == obs_scene_get_source(curScene))
            return;
    }

    AFMainWindowAccesser& tmpViewModels = g_ViewModelsDynamic.GetInstance();
    AFStateAppContext* tmpStateApp = AFConfigManager::GetSingletonInstance().GetStates();
    
    
    if (tmpStateApp->IsPreviewProgramMode() == false) {
        AFSceneUtil::TransitionToScene(scene, force);
    }
    else {
        OBSSource actualLastScene = OBSGetStrongRef(sceneContext.GetLastOBSScene());
        if (actualLastScene != scene) {
            if (scene)
                obs_source_inc_showing(scene);
            if (actualLastScene) {
                obs_source_dec_showing(actualLastScene);

                OBSScene obsLastScene = obs_scene_from_source(actualLastScene);
                sceneContext.SetCurrOBSScene(obsLastScene);
            }
            sceneContext.SetLastOBSScene(scene);
        }
    }

    m_qSceneSourceDock->SetCurrentScene(scene, force);

    ClearAllSourceContextPopup();

    // MainFrame UI Refresh
    if (scene)
        App()->GetMainView()->SetSceneBottomButtonStyleSheet(scene);
    
    config_t* configGlobalFile = AFConfigManager::GetSingletonInstance().GetGlobal();
    bool studioPortraitLayout = config_get_bool(configGlobalFile, "BasicWindow", "StudioPortraitLayout");

    UpdateSceneNameStudioMode(studioPortraitLayout);
}
//

void AFMainDynamicComposit::UpdateVolumeControlsDecayRate()
{
    double meterDecayRate =
        config_get_double(AFConfigManager::GetSingletonInstance().GetBasic(), "Audio", "MeterDecayRate");

    for (size_t i = 0; i < volumes.size(); i++) {
        volumes[i]->SetMeterDecayRate(meterDecayRate);
    }
}

void AFMainDynamicComposit::UpdateVolumeControlsPeakMeterType()
{
    uint32_t peakMeterTypeIdx =
        config_get_uint(AFConfigManager::GetSingletonInstance().GetBasic(), "Audio", "PeakMeterType");

    enum obs_peak_meter_type peakMeterType;
    switch (peakMeterTypeIdx) {
    case 0:
        peakMeterType = SAMPLE_PEAK_METER;
        break;
    case 1:
        peakMeterType = TRUE_PEAK_METER;
        break;
    default:
        peakMeterType = SAMPLE_PEAK_METER;
        break;
    }

    for (size_t i = 0; i < volumes.size(); i++) {
        volumes[i]->setPeakMeterType(peakMeterType);
    }
}

void AFMainDynamicComposit::RefreshVolumeColors()
{
    for (AFQVolControl* vol : volumes)
        vol->refreshColors();
}

void AFMainDynamicComposit::qslotSetMainAudioSource() 
{
    m_MainAudioVolumeControl = nullptr;
    m_MainMicVolumeControl = nullptr;

    bool isAllInputAudioMuted = true;
    bool isAllOutputAudioMuted = true;

    const char* input_id = App()->InputAudioSource();
    
    std::vector<OBSSource> sources;
    for (size_t i = 0; i != volumes.size(); i++) 
    {
        if (_SourceMixerHidden(volumes[i]->GetSource()))
            continue;

        const char* source_id = obs_source_get_id(volumes[i]->GetSource());
        
        // input
        if (strcmp(source_id, input_id) == 0)
        {
            if (volumes[i]->IsMuted() == false)
                isAllInputAudioMuted = false;
            if (m_MainMicVolumeControl == nullptr) {
                m_MainMicVolumeControl = volumes[i];
                continue;
            }

            m_MainMicVolumeControl = _CompareAudio(m_MainMicVolumeControl, volumes[i]);
        }
        else
        {
            if (volumes[i]->IsMuted() == false)
                isAllOutputAudioMuted = false;
            if (m_MainAudioVolumeControl == nullptr) {
                m_MainAudioVolumeControl = volumes[i];
                continue;
            }

            m_MainAudioVolumeControl = _CompareAudio(m_MainAudioVolumeControl, volumes[i]);
        }
    }

    // input
    if (m_MainMicVolumeControl == nullptr) {
        App()->GetMainView()->SetMicButtonEnabled(false);
    }
    else {
        App()->GetMainView()->SetMicButtonMute(isAllInputAudioMuted);
        App()->GetMainView()->SetMicSliderMute(m_MainMicVolumeControl->IsMuted());
        App()->GetMainView()->SetMicButtonEnabled(true);
        App()->GetMainView()->SetMicVolumeSliderSignalsBlock(true);
        App()->GetMainView()->SetMicVolume(m_MainMicVolumeControl->GetVolume());

        if (SourceVolumeLocked(m_MainMicVolumeControl->GetSource()))
            App()->GetMainView()->SetMicSliderEnabled(false);
        else
            App()->GetMainView()->SetMicSliderEnabled(true);

        App()->GetMainView()->SetMicVolumeSliderSignalsBlock(false);
    }

    // output
    if (m_MainAudioVolumeControl == nullptr) {
        App()->GetMainView()->SetAudioButtonEnabled(false);
    }
    else {
        App()->GetMainView()->SetAudioButtonMute(isAllOutputAudioMuted);
        App()->GetMainView()->SetAudioSliderMute(m_MainAudioVolumeControl->IsMuted());
        App()->GetMainView()->SetAudioButtonEnabled(true);
        App()->GetMainView()->SetAudioVolumeSliderSignalsBlock(true);
        App()->GetMainView()->SetAudioVolume(m_MainAudioVolumeControl->GetVolume());

        if (SourceVolumeLocked(m_MainAudioVolumeControl->GetSource()))
            App()->GetMainView()->SetAudioSliderEnabled(false);
        else
            App()->GetMainView()->SetAudioSliderEnabled(true);

        App()->GetMainView()->SetAudioVolumeSliderSignalsBlock(false);
    }
}

void AFMainDynamicComposit::MainAudioVolumeChanged(int volume)
{
    if (m_MainAudioVolumeControl)
    {
        App()->GetMainView()->SetAudioVolumeSliderSignalsBlock(true);
        
        const char* input_id = App()->InputAudioSource();
        const char* source_id;

        for (AFQVolControl* vol : volumes)
        {
            // lock
            if (SourceVolumeLocked(vol->GetSource()))
                continue;

            source_id = obs_source_get_id(vol->GetSource());
            if (strcmp(source_id, input_id) != 0) {
                vol->ValueChangedByUser(false);
                vol->ChangeVolume(volume);
                vol->ValueChangedByUser(true);
            }
        }
        App()->GetMainView()->SetAudioVolumeSliderSignalsBlock(false);
    }
}

void AFMainDynamicComposit::MainMicVolumeChanged(int volume)
{
    if (m_MainMicVolumeControl)
    {
        App()->GetMainView()->SetMicVolumeSliderSignalsBlock(true);

        const char* input_id = App()->InputAudioSource();
        const char* source_id;

        for (AFQVolControl* vol : volumes)
        {
            // lock
            if (SourceVolumeLocked(vol->GetSource()))
                continue;
            
            source_id = obs_source_get_id(vol->GetSource());
            if (strcmp(source_id, input_id) == 0) {
                vol->ValueChangedByUser(false);
                vol->ChangeVolume(volume);
                vol->ValueChangedByUser(true);
            }
        }
        App()->GetMainView()->SetMicVolumeSliderSignalsBlock(false);
    }
}

void AFMainDynamicComposit::SetMainAudioMute()
{
    if (m_MainAudioVolumeControl)
    {
        bool isMuted = App()->GetMainView()->IsAudioMuted();
        const char* input_id = App()->InputAudioSource();
        const char* source_id;

        for (AFQVolControl* vol : volumes)
        {
            source_id = obs_source_get_id(vol->GetSource());
            if (strcmp(source_id, input_id) != 0) {
                if (!isMuted != vol->IsMuted()) {
                    vol->ValueChangedByUser(false);
                    vol->ChangeMuteState();
                    vol->SetMuted();
                    vol->ValueChangedByUser(true);
                }
            }
        }
        App()->GetMainView()->SetAudioButtonMute(!isMuted);
        App()->GetMainView()->SetAudioSliderMute(!isMuted);
    }
}

void AFMainDynamicComposit::SetMainMicMute()
{
    if (m_MainMicVolumeControl)
    {
        bool isMuted = App()->GetMainView()->IsMicMuted();
        const char* input_id = App()->InputAudioSource();
        const char* source_id;

        for (AFQVolControl* vol : volumes)
        {
            source_id = obs_source_get_id(vol->GetSource());
            if (strcmp(source_id, input_id) == 0) {
                if (!isMuted != vol->IsMuted()) {
                    vol->ValueChangedByUser(false);
                    vol->ChangeMuteState();
                    vol->SetMuted();
                    vol->ValueChangedByUser(true);
                }
            }
        }
        App()->GetMainView()->SetMicButtonMute(!isMuted);
        App()->GetMainView()->SetMicSliderMute(!isMuted);
    }
}

bool AFMainDynamicComposit::_SourceMixerHidden(obs_source_t* source)
{
    OBSDataAutoRelease priv_settings =
        obs_source_get_private_settings(source);
    bool hidden = obs_data_get_bool(priv_settings, "mixer_hidden");

    return hidden;
}

void AFMainDynamicComposit::_SetSourceMixerHidden(obs_source_t* source, bool hidden)
{
    OBSDataAutoRelease priv_settings =
        obs_source_get_private_settings(source);
    obs_data_set_bool(priv_settings, "mixer_hidden", hidden);
}

void AFMainDynamicComposit::ClearVolumeControls()
{
    for (AFQVolControl* vol : volumes)
        delete vol;

    volumes.clear();
}

void AFMainDynamicComposit::ToggleMixerLayout(bool vertical)
{
    if (vertical) {
        m_qAudioMixerDockWidget->GetUIPtr()->stackedMixerArea->setMinimumSize(180, 220);
        m_qAudioMixerDockWidget->GetUIPtr()->stackedMixerArea->setCurrentIndex(1);
    }
    else {
        m_qAudioMixerDockWidget->GetUIPtr()->stackedMixerArea->setMinimumSize(220, 0);
        m_qAudioMixerDockWidget->GetUIPtr()->stackedMixerArea->setCurrentIndex(0);
    }
}

void AFMainDynamicComposit::ToggleVolControlLayout()
{
    bool vertical = !config_get_bool(AFConfigManager::GetSingletonInstance().GetGlobal(), "BasicWindow",
        "VerticalVolControl");
    config_set_bool(AFConfigManager::GetSingletonInstance().GetGlobal(), "BasicWindow", "VerticalVolControl",
        vertical);
    ToggleMixerLayout(vertical);

    std::vector<OBSSource> sources;
    for (size_t i = 0; i != volumes.size(); i++)
        sources.emplace_back(volumes[i]->GetSource());

    ClearVolumeControls();

    for (const auto& source : sources)
        qslotActivateAudioSource(source);

    qslotSetMainAudioSource();
}

void AFMainDynamicComposit::VolControlContextMenu()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& localeTextManger = AFLocaleTextManager::GetSingletonInstance();

    AFQVolControl* vol = reinterpret_cast<AFQVolControl*>(sender());

    QAction lockAction(localeTextManger.Str("LockVolume"), this);
    lockAction.setCheckable(true);
    lockAction.setChecked(SourceVolumeLocked(vol->GetSource()));

    QAction hideAction(localeTextManger.Str("Hide"), this);    
    QAction unhideAllAction(localeTextManger.Str("UnhideAll"), this);
    QAction mixerRenameAction(localeTextManger.Str("Rename"), this);

    QAction copyFiltersAction(localeTextManger.Str("Copy.Filters"), this);
    QAction pasteFiltersAction(localeTextManger.Str("Paste.Filters"), this);

    QAction toggleControlLayoutAction(
        localeTextManger.Str("VerticalLayout"), this);
    toggleControlLayoutAction.setCheckable(true);
    toggleControlLayoutAction.setChecked(config_get_bool(
        confManager.GetGlobal(), "BasicWindow", "VerticalVolControl"));

    QAction filtersAction(localeTextManger.Str("Filters"), this);
    QAction propertiesAction(localeTextManger.Str("Properties"), this);
    QAction advPropAction(localeTextManger.Str("Basic.MainMenu.Edit.AdvAudio"), this);

    /* ------------------- */

    connect(&hideAction, &QAction::triggered, this,
        &AFMainDynamicComposit::qslotHideAudioControl, Qt::DirectConnection);
    connect(&unhideAllAction, &QAction::triggered, this,
        &AFMainDynamicComposit::qslotUnhideAllAudioControls, Qt::DirectConnection);
    connect(&lockAction, &QAction::toggled, this,
        &AFMainDynamicComposit::qslotLockVolumeControl, Qt::DirectConnection);
    connect(&mixerRenameAction, &QAction::triggered, this,
        &AFMainDynamicComposit::qslotMixerRenameSource, Qt::DirectConnection);

    connect(&copyFiltersAction, &QAction::triggered, this,
        &AFMainDynamicComposit::qslotAudioMixerCopyFilters, Qt::DirectConnection);
    connect(&pasteFiltersAction, &QAction::triggered, this,
        &AFMainDynamicComposit::qslotAudioMixerPasteFilters, Qt::DirectConnection);

    connect(&toggleControlLayoutAction, &QAction::changed, this,
        &AFMainDynamicComposit::ToggleVolControlLayout, Qt::DirectConnection);

    connect(&filtersAction, &QAction::triggered, this,
        &AFMainDynamicComposit::qslotGetAudioSourceFilters, Qt::DirectConnection);
    connect(&propertiesAction, &QAction::triggered, this,
        &AFMainDynamicComposit::qslotGetAudioSourceProperties, Qt::DirectConnection);
    connect(&advPropAction, &QAction::triggered, this,
        &AFMainDynamicComposit::qslotAdvAudioPropertiesTriggered,
        Qt::DirectConnection);

    /* ------------------- */

    hideAction.setProperty("volControl",
        QVariant::fromValue<AFQVolControl*>(vol));
    lockAction.setProperty("volControl",
        QVariant::fromValue<AFQVolControl*>(vol));
    mixerRenameAction.setProperty("volControl",
        QVariant::fromValue<AFQVolControl*>(vol));

    copyFiltersAction.setProperty("volControl",
        QVariant::fromValue<AFQVolControl*>(vol));
    pasteFiltersAction.setProperty("volControl",
        QVariant::fromValue<AFQVolControl*>(vol));

    filtersAction.setProperty("volControl",
        QVariant::fromValue<AFQVolControl*>(vol));
    propertiesAction.setProperty("volControl",
        QVariant::fromValue<AFQVolControl*>(vol));

    copyFiltersAction.setEnabled(obs_source_filter_count(vol->GetSource()) > 0);

    OBSSourceAutoRelease source =
        obs_weak_source_get_source(m_mixerCopyFiltersSource);
    if (source) {
        pasteFiltersAction.setEnabled(true);
    }
    else {
        pasteFiltersAction.setEnabled(false);
    }

    /* ------------------- */

    AFQCustomMenu popup = new AFQCustomMenu(this);
    vol->SetContextMenu(&popup);
    popup.addAction(&lockAction);
    popup.addSeparator();
    popup.addAction(&unhideAllAction);
    popup.addAction(&hideAction);
    popup.addAction(&mixerRenameAction);
    popup.addSeparator();
    popup.addAction(&copyFiltersAction);
    popup.addAction(&pasteFiltersAction);
    popup.addSeparator();
    popup.addAction(&toggleControlLayoutAction);
    popup.addSeparator();
    popup.addAction(&filtersAction);
    popup.addAction(&propertiesAction);
    popup.addAction(&advPropAction);

    // toggleControlLayoutAction deletes and re-creates the volume controls
    // meaning that "vol" would be pointing to freed memory.
    if (popup.exec(QCursor::pos()) != &toggleControlLayoutAction)
        vol->SetContextMenu(nullptr);
}

void AFMainDynamicComposit::_MakePopupForDock(const char* key, bool resetPosition)
{
    QDockWidget* dock;
    bool checkDockExist = _FindDock(key, dock);

    if (checkDockExist)
    {
        QPair<QWidget*, QWidget*> block;
        bool checkBlockExist = _FindBlock(key, block);
        if (checkBlockExist)
        {
            dock->setVisible(false);

            AFQBorderPopupBaseWidget* DockPopup = 
                new AFQBorderPopupBaseWidget(nullptr, Qt::FramelessWindowHint | Qt::Window);

            QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
            int blockTypeNum = BlockTypeEnum.keyToValue(key);
            DockPopup->SetBlockType(key, blockTypeNum);

            DockPopup->setObjectName("widget_Popup");

            DockPopup->setMinimumSize(dock->minimumSize());

            AFDockTitle* dockTitle = reinterpret_cast<AFDockTitle*>(block.first);
            dockTitle->SetToggleWindowToDockButton(false);
            
            connect(DockPopup, &AFCQMainBaseWidget::qsignalBaseWindowMaximized,
                    dockTitle, &AFDockTitle::qslotChangeMaximizeIcon);

            QWidget* dockContents = block.second;      

            if (std::strcmp(key, "SceneControl") != 0)
            {
                DockPopup->setAttribute(Qt::WA_TranslucentBackground);
            }
            else
            {
                DockPopup->setMinimumSize(SCENE_CONTROL_MIN_SIZE_WIDTH, SCENE_CONTROL_MIN_SIZE_HEIGTH);
                DockPopup->setStyleSheet("border-radius: 0px");
                dockContents->setStyleSheet("background-color: #24272D;");
            }
            //
            dockTitle->setStyleSheet("#AFDockTitle { border: none; }");
            if (std::strcmp(key, "SceneControl") != 0) {
                dockContents->setStyleSheet(
                    "#widget_DockContents { border: none; } \
                     #frame_DockContents { border: none; }");
            }

            DockPopup->setWindowTitle(dockTitle->GetLabelText());

            dockTitle->setParent(DockPopup);
            dockContents->setParent(DockPopup);

            DockPopup->AddWidget(dockTitle);
            DockPopup->AddWidget(dockContents);

            auto& confManager = AFConfigManager::GetSingletonInstance();
            bool showPopupLeft = config_get_bool(confManager.GetGlobal(), "BasicWindow", "ShowPopupLeft");

            int posX = dock->x();
            int posY = dock->y();

            if (resetPosition)
            {
                if (showPopupLeft)
                {
                    posX = App()->GetMainView()->pos().x() - dock->width();
                    posY = App()->GetMainView()->pos().y();
                }
                else // Center
                {
                    posX = App()->GetMainView()->pos().x() + ((App()->GetMainView()->width() - dock->width()) / 2);
                    posY = App()->GetMainView()->pos().y() + ((App()->GetMainView()->height() - dock->height()) / 2);
                }
            }

            QRect dockPopupPosition = QRect(posX, posY, dock->width(), dock->height());
            QRect adjustRect;
            AdjustPositionOutSideScreen(dockPopupPosition, adjustRect);
            DockPopup->setGeometry(adjustRect);
            DockPopup->show();

            m_qPopupMap.insert(key, DockPopup);
        }
    }
}

void AFMainDynamicComposit::_ClosePopup(const char* key, bool isVisible)
{
    QWidget* popup = nullptr;
    bool checkPopup = _FindPopup(key, popup);

    QDockWidget* dock;
    bool checkDockExist = _FindDock(key, dock);

    if (checkPopup)
    {
        AFQBorderPopupBaseWidget* borderPopup = reinterpret_cast<AFQBorderPopupBaseWidget*>(popup);
        if (checkDockExist)
        {
            dock->setFloating(true);
            dock->show();
            dock->resize(borderPopup->size());

            dock->hide();
        }

        qDebug() << "Close Popup: " << borderPopup->size();
        qDebug() << "Close Popup dock: " << dock->size();

        borderPopup->SetToDockValue(isVisible);
        borderPopup->close();
    }
    else
    {
        if (checkDockExist)
        {
            qDebug() << "Close Dock: " << dock->size();

            QSize size = dock->size();
            dock->setFloating(true);
            dock->resize(size);
            dock->hide();
        }
    }
}

void AFMainDynamicComposit::_RaiseBlock(const char* key)
{
    QWidget* popup = nullptr;
    bool checkPopup = _FindPopup(key, popup);
}

void AFMainDynamicComposit::_ToggleVisibleBlockWindow(int blocktype, bool show)
{
    QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
    const char* key = BlockTypeEnum.valueToKey(blocktype);

    bool checkBlockExist = m_qBlockMap.contains(key);

    if (checkBlockExist)
    {
        if (show)
        {
            QWidget* blockPopup = nullptr;
            bool checkPopup = _FindPopup(key, blockPopup);
            if (checkPopup)
            {
                if (blockPopup->isMinimized()) {
                    if (blockPopup->isMaximized())
                        blockPopup->showMaximized();
                    else
                        blockPopup->showNormal();
                }
                blockPopup->raise();
                blockPopup->activateWindow();
                blockPopup->setFocus();
            }
            else
            {
                QDockWidget* dock;
                bool checkDockExist = _FindDock(key, dock);
                if (!dock->isVisible())
                    _MakePopupForDock(key);
                
            }
        }
        else
        {
            _ClosePopup(key, false);
        }

        emit qsignalBlockVisibleToggled(show, blocktype);
    }
}

void AFMainDynamicComposit::_CloseAllWindows()
{
    foreach(QDockWidget * dock, m_qDockMap)
    {
        QWidget* DockPopup = dock->parentWidget();
        if (DockPopup == this)
        {
            continue;
        }

        dock->setParent(this);
        dock->setAllowedAreas(Qt::DockWidgetArea::BottomDockWidgetArea);
        dock->setFloating(true);

        DockPopup->close();
    }

    foreach(QWidget* widget, m_qBrowserWidgetMap)
    {
        widget->close();
        delete widget;
        widget = nullptr;
    }
}

bool AFMainDynamicComposit::_IsOutSideScreen(QRect windowGeometry)
{
    QList<QScreen*> screens = QGuiApplication::screens();

    QRect boundingBox;
    for (QScreen* screen : screens) 
        boundingBox = boundingBox.united(screen->availableGeometry());

    QRect intersection = windowGeometry.intersected(boundingBox);

    return intersection.width() * intersection.height() < windowGeometry.width() * windowGeometry.height();
}

bool AFMainDynamicComposit::_FindDock(const char* dockType, QDockWidget*& outDock) const
{
    if (m_qDockMap.contains(dockType))
    {
        outDock = m_qDockMap.value(dockType);
        return true;
    }
    return false;
}

bool AFMainDynamicComposit::_FindDock(int dockType, QDockWidget*& outDock) const
{
    QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
    const char* key = BlockTypeEnum.valueToKey(dockType);
    if (m_qDockMap.contains(key))
    {
        outDock = m_qDockMap.value(key);
        return true;
    }
    return false;
}

bool AFMainDynamicComposit::_FindPopup(const char* dockType, QWidget*& outPopup) const
{
    if (m_qPopupMap.contains(dockType))
    {
        outPopup = m_qPopupMap.value(dockType);
        return true;
    }
    return false;
}

bool AFMainDynamicComposit::_FindPopup(int dockType, QWidget*& outPopup) const
{
    QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
    const char* key = BlockTypeEnum.valueToKey(dockType);
    if (m_qPopupMap.contains(key))
    {
        outPopup = m_qPopupMap.value(key);
        return true;
    }
    return false;
}

bool AFMainDynamicComposit::_FindBlock(const char* dockType, QPair<QWidget*, QWidget*>& outBlock) const
{
    if (m_qBlockMap.contains(dockType))
    {
        outBlock = m_qBlockMap.value(dockType);
        return true;
    }
    return false;
}

bool AFMainDynamicComposit::_FindBlock(int dockType, QPair<QWidget*, QWidget*>& outBlock) const
{
    QMetaEnum BlockTypeEnum = QMetaEnum::fromType<BlockType>();
    const char* key = BlockTypeEnum.valueToKey(dockType);
    if (m_qBlockMap.contains(key))
    {
        outBlock = m_qBlockMap.value(key);
        return true;
    }

    return false;
}

bool AFMainDynamicComposit::_CheckDockedCount()
{
    int dockedWidgetCount = 0;
    foreach(QDockWidget * dock, m_qDockMap)
    {
        qDebug() << dock->size();
        if (dock->isVisible() && !dock->isFloating())
        {
            dockedWidgetCount++;
        }
    }
    if (dockedWidgetCount == 0)
    {
        return false;
    }
    return true;
}

void AFMainDynamicComposit::_MakeVerticalStudioMode()
{
    auto& logManager = AFLogManager::GetSingletonInstance();

    AFMainWindowAccesser& tmpViewModels = g_ViewModelsDynamic.GetInstance();
    ui->centralwidget->setStyleSheet("background-color: #0A0A0A;");
    ui->widget_Screen->SetDisplayBackgroundColor(GREY_TONDOWN_COLOR_BACKGROUND);

    tmpViewModels.m_RenderModel.CreateProgramDisplay();
    _InitProgramDisplay(&tmpViewModels);
    tmpViewModels.m_RenderModel.SetProgramScene();

    if (m_pStudioModeVerticalView == nullptr)
    {
        m_pStudioModeVerticalView = new AFQVerticalProgramView();
        m_pStudioModeVerticalView->Initialize();

        UpdateSceneNameStudioMode(true);

        /*connect(ui->widget_Screen, &AFQTDisplay::qsignalDisplayResized,
            m_pStudioModeVerticalView, &AFQVerticalProgramView::qslotChangeLayoutStrech);*/
        connect(m_qSceneControlDock,
                &AFSceneControlDockWidget::qSignalTransitionButtonClicked,
                m_pStudioModeVerticalView,
                &AFQVerticalProgramView::qslotTransitionTriggered);

        QHBoxLayout* tmpCentLayoutThis = qobject_cast<QHBoxLayout*>(ui->centralwidget->layout());

        tmpCentLayoutThis->insertWidget(0, m_pStudioModeVerticalView);
        m_pStudioModeVerticalView->InsertDisplays(ui->widget_Screen,
            tmpViewModels.m_RenderModel.GetProgramDisplay());

        m_pStudioModeVerticalView->show();
    }

    if (m_qSceneControlDock != nullptr)
        m_qSceneControlDock->ChangeLayoutStudioMode(true);

    logManager.OBSBaseLog(LOG_INFO, "Switched to Vertical Preview/Program mode");
    logManager.OBSBaseLog(LOG_INFO, "-----------------------------"
        "-------------------");
}

void AFMainDynamicComposit::_MakeHorizontalStudioMode()
{
    auto& logManager = AFLogManager::GetSingletonInstance();

    AFMainWindowAccesser& tmpViewModels = g_ViewModelsDynamic.GetInstance();
    ui->centralwidget->setStyleSheet("background-color: #0A0A0A;");
    ui->widget_Screen->SetDisplayBackgroundColor(GREY_TONDOWN_COLOR_BACKGROUND);

    tmpViewModels.m_RenderModel.CreateProgramDisplay();
    _InitProgramDisplay(&tmpViewModels);
    tmpViewModels.m_RenderModel.SetProgramScene();

    if (m_pStudioModeView == nullptr)
    {
        m_pStudioModeView = new AFQProgramView();
        m_pStudioModeView->Initialize();

        UpdateSceneNameStudioMode(false);

        connect(ui->widget_Screen, &AFQTDisplay::qsignalDisplayResized,
                m_pStudioModeView, &AFQProgramView::qslotChangeLayoutStrech);
        connect(m_qSceneControlDock,
                &AFSceneControlDockWidget::qSignalTransitionButtonClicked,
                m_pStudioModeView, &AFQProgramView::qslotTransitionTriggered);

        QHBoxLayout* tmpCentLayoutThis = qobject_cast<QHBoxLayout*>(ui->centralwidget->layout());
        tmpCentLayoutThis->insertWidget(0, m_pStudioModeView);

        m_pStudioModeView->InsertDisplays(ui->widget_Screen,
            tmpViewModels.m_RenderModel.GetProgramDisplay());
        m_pStudioModeView->show();
    }

    if (m_qSceneControlDock != nullptr)
        m_qSceneControlDock->ChangeLayoutStudioMode(true);

    logManager.OBSBaseLog(LOG_INFO, "Switched to Vertical Preview/Program mode");
    logManager.OBSBaseLog(LOG_INFO, "-----------------------------"
        "-------------------");
}

void AFMainDynamicComposit::_DestroyVerticalStudioMode()
{
    auto& logManager = AFLogManager::GetSingletonInstance();
   
    m_pStudioModeVerticalView->hide();
    
    ui->centralwidget->setStyleSheet("background-color: #111111;");
    ui->widget_Screen->SetDisplayBackgroundColor(GREY_COLOR_BACKGROUND);

    QHBoxLayout* tmpCentLayoutThis = qobject_cast<QHBoxLayout*>(ui->centralwidget->layout());
    tmpCentLayoutThis->insertWidget(0, ui->widget_Screen);

    AFMainWindowAccesser& tmpViewModels = g_ViewModelsDynamic.GetInstance();
    m_pStudioModeVerticalView->layout()->removeWidget(tmpViewModels.m_RenderModel.GetProgramDisplay());
    ui->centralwidget->layout()->removeWidget(m_pStudioModeVerticalView);

    //disconnect(ui->widget_Screen, &AFQTDisplay::qsignalDisplayResized,
    //    m_pStudioModeVerticalView, &AFQProgramView::qslotChangeLayoutStrech);

    if (m_pStudioModeVerticalView != nullptr)
    {
        delete m_pStudioModeVerticalView;
        m_pStudioModeVerticalView = nullptr;
    }

    //
    tmpViewModels.m_RenderModel.ResetProgramScene();
    tmpViewModels.m_RenderModel.ReleaseProgramDisplay();

    if (m_qSceneControlDock != nullptr)
        m_qSceneControlDock->ChangeLayoutStudioMode(false);

    logManager.OBSBaseLog(LOG_INFO, "Switched to regular Preview mode From Vertical");
    logManager.OBSBaseLog(LOG_INFO, "-----------------------------"
        "-------------------");
}

void AFMainDynamicComposit::_DestroyHorizontalStudioMode()
{
    auto& logManager = AFLogManager::GetSingletonInstance();

    m_pStudioModeView->hide();

    ui->centralwidget->setStyleSheet("background-color: #111111;");
    ui->widget_Screen->SetDisplayBackgroundColor(GREY_COLOR_BACKGROUND);

    QHBoxLayout* tmpCentLayoutThis = qobject_cast<QHBoxLayout*>(ui->centralwidget->layout());
    tmpCentLayoutThis->insertWidget(0, ui->widget_Screen);

    AFMainWindowAccesser& tmpViewModels = g_ViewModelsDynamic.GetInstance();
    
    m_pStudioModeView->layout()->removeWidget(tmpViewModels.m_RenderModel.GetProgramDisplay());
    ui->centralwidget->layout()->removeWidget(m_pStudioModeView);

    disconnect(ui->widget_Screen, &AFQTDisplay::qsignalDisplayResized,
        m_pStudioModeView, &AFQProgramView::qslotChangeLayoutStrech);

    if (m_pStudioModeView != nullptr)
    {
        delete m_pStudioModeView;
        m_pStudioModeView = nullptr;
    }
    
    //
    tmpViewModels.m_RenderModel.ResetProgramScene();
    tmpViewModels.m_RenderModel.ReleaseProgramDisplay();

    if (m_qSceneControlDock != nullptr)
        m_qSceneControlDock->ChangeLayoutStudioMode(false);

    logManager.OBSBaseLog(LOG_INFO, "Switched to regular Preview mode From Horizontal");
    logManager.OBSBaseLog(LOG_INFO, "-----------------------------"
        "-------------------");
}

void AFMainDynamicComposit::_InitMainDisplay(AFMainWindowAccesser* viewModels)
{
    auto addDisplay = [viewModels](AFQTDisplay* window) {
        if (viewModels == nullptr)
            return;
        
        obs_display_add_draw_callback(window->GetDisplay(),
            AFMainWindowRenderModel::RenderMain, &viewModels->m_RenderModel);

        struct obs_video_info ovi;
        if (obs_get_video_info(&ovi))
            viewModels->m_RenderModel.ResizePreview(ovi.base_width, ovi.base_height);
    };

    connect(ui->widget_Screen, &AFQTDisplay::qsignalDisplayCreated, addDisplay);


    auto displayResize = [this, viewModels]() {
        if (viewModels == nullptr)
            return;
        
        struct obs_video_info ovi;

        if (obs_get_video_info(&ovi))
            viewModels->m_RenderModel.ResizePreview(ovi.base_width, ovi.base_height);

        if (viewModels != nullptr)
            viewModels->m_RenderModel.SetDPIValue(devicePixelRatioF());
        
        
    };

    connect(windowHandle(), &QWindow::screenChanged, displayResize);
    connect(ui->widget_Screen, &AFQTDisplay::qsignalDisplayResized, displayResize);
    //
}

void AFMainDynamicComposit::_InitProgramDisplay(AFMainWindowAccesser* viewModels)
{
    AFQTDisplay* tmpProgramDisplay = viewModels->m_RenderModel.GetProgramDisplay();
    tmpProgramDisplay->SetDisplayBackgroundColor(GREY_TONDOWN_COLOR_BACKGROUND);
    
    tmpProgramDisplay->setContextMenuPolicy(Qt::CustomContextMenu);

    auto displayResize = [viewModels]() {
        struct obs_video_info ovi;

        if (obs_get_video_info(&ovi))
            viewModels->m_RenderModel.ResizeProgram(ovi.base_width, ovi.base_height);
    };

    connect(tmpProgramDisplay, &AFQTDisplay::qsignalDisplayResized, displayResize);

    auto addDisplay = [viewModels](AFQTDisplay *window) {
        obs_display_add_draw_callback(window->GetDisplay(),
            AFMainWindowRenderModel::RenderProgram, &viewModels->m_RenderModel);

        struct obs_video_info ovi;
        if (obs_get_video_info(&ovi))
            viewModels->m_RenderModel.ResizeProgram(ovi.base_width, ovi.base_height);
    };

    connect(tmpProgramDisplay, &AFQTDisplay::qsignalDisplayCreated, addDisplay);

    tmpProgramDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

}
