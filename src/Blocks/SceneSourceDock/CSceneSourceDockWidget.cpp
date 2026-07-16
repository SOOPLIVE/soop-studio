#include "CSceneSourceDockWidget.h"
#include "ui_scene-source-dock.h"

#include <QPushButton>
#include <QListWidget>
#include <QScrollBar>

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Source/CSource.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/OBSOutput/COutput.h"

#include "Application/CApplication.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#pragma region _SOOP_BREAKTIME
#include "Utils/BreakTimeManager.h"  
#pragma endregion


//#define AF_SCENE_ITEM_WIDTH         (192)
#define AF_SCENE_ITEM_WIDTH         (196)
#define AF_SCENE_SELECT_ITEM_WIDTH  (201)
#define AF_SCENE_ITEM_HEIGHT        (44)
#define AF_SCENE_ITEM_HOVER_HEIGHT  (140)
#define AF_SCENE_ITEM_SPACE_YPOS    (4)
#define AF_SCENE_ITEM_SPACE_XPOS    (4)

void AFSceneSourceWidget::qslotClickedSceneItem()
{
    AFQSceneListItem* clickedItem  = SCENE_CONTEXT.GetCurSelectedSceneItem();

    obs_source_t* source = obs_scene_get_source(clickedItem->GetScene());
    
    MAINFRAME->SetCurrentScene(source);

    int nPrevCnt = config_get_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt");
    config_set_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt", 0);

    SceneItemVector& sceneItemVector = SCENE_CONTEXT.GetSceneItemVector();

    struct FindMinsimChk { bool found = false; int nCnt = 0; } findMinsimChk;
    FindMinsimChk info;

    obs_scene_enum_items(clickedItem->GetScene(), [](obs_scene_t*, obs_sceneitem_t* item, void* param)->bool {
        auto* f = static_cast<FindMinsimChk*>(param);
        auto src = obs_sceneitem_get_source(item);
        const char* id = obs_source_get_id(src);
        if (0 == strcmp(id, "soop_chat_source_mood_check")) {
            bool bVisible = obs_source_showing(src);
            if (bVisible) {
                f->found = true;
                f->nCnt += 1;
            }
        }
        return true;
    }, &info);

    auto broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();

    config_set_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt", info.nCnt);

    if (AFOutputUtil::IsStreamActive()) {
        if (nPrevCnt > 0 && info.nCnt == 0) {
            auto endTime = std::chrono::steady_clock::now();
            auto startTime = AUTH_CONTEXT.GetMinsimCheckStartTime();
            if (startTime != std::chrono::steady_clock::time_point{}) {
                AUTH_CONTEXT.InitMinsimCheckStartTime();
            }            
        }
        else if (nPrevCnt == 0 && info.nCnt > 0) {
            auto startTime = std::chrono::steady_clock::now();
            AUTH_CONTEXT.SetMinsimCheckStartTime(startTime);
        }
    }
}

void AFSceneSourceWidget::qslotDoubleClickedSceneItem()
{

}

void AFSceneSourceWidget::qslotRenameSceneItem()
{
    MAINFRAME->RefreshSceneUI();
}

void AFSceneSourceWidget::qslotDeleteSceneItem()
{

}

void AFSceneSourceWidget::qslotHoverSceneItem(OBSScene scene)
{
    RefreshSceneItem();
}

void AFSceneSourceWidget::qslotSwapItem(int from, int dest)
{
    if (from == dest)
        return;

    SCENE_CONTEXT.SwapSceneItem(from, dest);

    RefreshSceneItem();

    MAINFRAME->RefreshSceneUI();
}

void AFSceneSourceWidget::qslotAddSceneButtonClicked()
{
    emit qsignalAddScene();
}

void AFSceneSourceWidget::qslotAddSourceTrigger()
{
    emit qsignalAddSource();
}

void AFSceneSourceWidget::qslotRemoveSourceTrigger()
{
    AFSourceUtil::RemoveSourceItems(SCENE_CONTEXT.GetCurrentScene());
}

void AFSceneSourceWidget::qslotMoveUpSourceTrigger()
{
    _MoveSceneItem(OBS_ORDER_MOVE_UP, QTStr("Undo.MoveUp"));
}

void AFSceneSourceWidget::qslotMoveDownSourceTrigger()
{
    _MoveSceneItem(OBS_ORDER_MOVE_DOWN, QTStr("Undo.MoveDown"));
}

void AFSceneSourceWidget::qslotMoveToTopSourceTrigger()
{
    _MoveSceneItem(OBS_ORDER_MOVE_TOP, QTStr("Undo.MoveToTop"));
}

void AFSceneSourceWidget::qslotMoveToBottomSourceTrigger()
{
    _MoveSceneItem(OBS_ORDER_MOVE_BOTTOM, QTStr("Undo.MoveToBottom"));
}

void AFSceneSourceWidget::qslotShowPropsTrigger()
{
    const auto item = GetCurrentSceneItem();
    if (!item)
        return;

    obs_source_t* source = obs_sceneitem_get_source(item);
    if (obs_source_configurable(source)) {
        MAINFRAME->CreateSourceProperties(source);
    }
}

void AFSceneSourceWidget::qslotFitScreenSizeSourceTrigger()
{
    AFSourceUtil::FitSourceToScreenFromMenu(OBS_BOUNDS_SCALE_INNER);
}

void AFSceneSourceWidget::qslotRestoreSourceTrigger()
{
    AFSourceUtil::ResetTransform();
}

void AFSceneSourceWidget::AddSceneItem(OBSSceneItem item)
{
    obs_scene_t* scene = obs_sceneitem_get_scene(item);

    if(SCENE_CONTEXT.GetCurrentScene() == scene) {
        ui->sourceListView->Add(item);
    }

    if (!LOADSAVE_CONTEXT.CheckDisableSaving()) {
        obs_source_t* sceneSource = obs_scene_get_source(scene);
        obs_source_t* itemSource = obs_sceneitem_get_source(item);
        blog(LOG_INFO, "User added source '%s' (%s) to scene '%s'",
             obs_source_get_name(itemSource), obs_source_get_id(itemSource),
             obs_source_get_name(sceneSource));

        AFSourceUtil::SelectedItemOne(scene, (obs_sceneitem_t*)item);
    }
}

void AFSceneSourceWidget::ReorderSources(OBSScene scene)
{
    if (scene != SCENE_CONTEXT.GetCurrentScene() || ui->sourceListView->IgnoreReorder())
        return;

    ui->sourceListView->ReorderItems();
    //SaveProject();
}


void AFSceneSourceWidget::RefreshSources(OBSScene scene)
{
    if (scene != SCENE_CONTEXT.GetCurrentScene() || ui->sourceListView->IgnoreReorder())
        return;

    ui->sourceListView->RefreshItems();
    //SaveProject();
}


void AFSceneSourceWidget::resizeEvent(QResizeEvent* event)
{
    int  wideDockMode = 0;
    QSize newSize = event->size();
    if (newSize.width() >= 450) {
        wideDockMode = 0;
    }
    else if (newSize.width() >= 425 && newSize.width() < 450) {
        wideDockMode = 1;
    }
    else if(newSize.width() >= 400 && newSize.width() < 425) {
        wideDockMode = 2;
    }
    else if (newSize.width() >= 375 && newSize.width() < 400) {
        wideDockMode = 3;
    }
    else if (newSize.width() >= 350 && newSize.width() < 375) {
        wideDockMode = 4;
    }
    else {
        wideDockMode = 5;
    }

    if (m_dockWideMode != wideDockMode) {
        const int scrollAreaWidth = 205 - (20 * wideDockMode);
        ui->scenListScrollArea->setFixedWidth(scrollAreaWidth);
        m_dockWideMode = wideDockMode;

        RefreshSceneItem();
    }
    QWidget::resizeEvent(event);
}

AFSceneSourceWidget::AFSceneSourceWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFSceneSourceWidget)
{
    ui->setupUi(this);

    _InitSceneSourceDockUI();

    _InitSceneSourceDockSignalSlot();

    ui->sceneListScrollAreaContents->SetScrollAreaPtr(ui->scenListScrollArea);

    SCENE_CONTEXT.SetSourceListViewPtr(ui->sourceListView);

    if (!m_sceneAddButton) {
        m_sceneAddButton = new QPushButton(ui->sceneListScrollAreaContents);
        m_sceneAddButton->setObjectName("sceneListAddButton");
        m_sceneAddButton->setText(Str("Basic.SceneSourceDock.AddScene"));
        connect(m_sceneAddButton, &QPushButton::clicked,
                this, &AFSceneSourceWidget::qslotAddSceneButtonClicked);
    }
}

AFSceneSourceWidget::~AFSceneSourceWidget()
{
    delete ui;
}

QWidget* AFSceneSourceWidget::GetSceneListFrame()
{
    return ui->sceneListFrame;
}

QWidget* AFSceneSourceWidget::GetSourceListView()
{
    return ui->sourceListView;
}

void AFSceneSourceWidget::AddScene(OBSSource source)
{
    const char* name = obs_source_get_name(source);
    obs_scene_t* scene = obs_scene_from_source(source);

    obs_hotkey_register_source(
        source, "OBSBasic.SelectScene",
        Str("Basic.Hotkeys.SelectScene"),
        [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
#pragma region _SOOP_BREAKTIME
            if (BREAKTIME_MANAGER.IsActive())
                return;
#pragma endregion

            auto potential_source = static_cast<obs_source_t*>(data);
            OBSSourceAutoRelease source = obs_source_get_ref(potential_source);
            if (source && pressed)
                MAINFRAME->SetCurrentScene(source.Get());
        },
        static_cast<obs_source_t*>(source));

    signal_handler_t* handler = obs_source_get_signal_handler(source);

    SignalContainer<OBSScene> container;
    container.ref = scene;
    container.handlers.assign({
                std::make_shared<OBSSignal>(handler, "item_add",
                                AFSceneSourceWidget::SceneItemAdded, this),
                std::make_shared<OBSSignal>(handler, "reorder",
                                AFSceneSourceWidget::SceneReordered, this),
                std::make_shared<OBSSignal>(handler, "refresh",
                                AFSceneSourceWidget::SceneRefreshed, this),
        });

    AFQSceneListItem* sceneItem = new AFQSceneListItem(ui->sceneListScrollAreaContents, ui->sceneListFrame, scene, name, container);

    connect(sceneItem, &AFQSceneListItem::qsignalClickedSceneItem, 
            this, &AFSceneSourceWidget::qslotClickedSceneItem);
    connect(sceneItem, &AFQSceneListItem::qsignalDoubleClickedSceneItem,
            this, &AFSceneSourceWidget::qslotDoubleClickedSceneItem);
    connect(sceneItem, &AFQSceneListItem::qsignalRenameSceneItem, 
            this, &AFSceneSourceWidget::qslotRenameSceneItem);
    connect(sceneItem, &AFQSceneListItem::qsignalDeleteSceneItem, 
            this, &AFSceneSourceWidget::qslotDeleteSceneItem);
    connect(sceneItem, &AFQSceneListItem::qsignalHoverSceneItem, 
            this, &AFSceneSourceWidget::qslotHoverSceneItem);

    /* if the scene already has items (a duplicated scene) add them */
    auto addSceneItem = [this](obs_sceneitem_t* item) {
        AddSceneItem(item);
    };

    using addSceneItem_t = decltype(addSceneItem);

    obs_scene_enum_items(
        scene,
        [](obs_scene_t*, obs_sceneitem_t* item, void* param) {
            addSceneItem_t* func;
            func = reinterpret_cast<addSceneItem_t*>(param);
            (*func)(item);
            return true;
        },
        &addSceneItem);

    SCENE_CONTEXT.SetCurSelectedSceneItem(sceneItem);
    SCENE_CONTEXT.AddSceneItem(sceneItem);

    RefreshSceneItem();
}

void AFSceneSourceWidget::RemoveScene(OBSSource source)
{
    SceneItemVector& sceneItemVector = SCENE_CONTEXT.GetSceneItemVector();

    obs_scene_t* scene = obs_scene_from_source(source);
    AFQSceneListItem* delItem = nullptr;

    auto iter = sceneItemVector.begin();
    for (; iter != sceneItemVector.end(); ++iter) {
        AFQSceneListItem* item = (*iter);
        if (!item)
            continue;

        if (item->GetScene() != scene)
            continue;

        delItem = item;
        break;
    }

    int removeSceneIdx = -1;
    if (delItem != nullptr) {

        removeSceneIdx = delItem->GetSceneIndex();
        sceneItemVector.erase(iter); 
        ui->sourceListView->Clear();
        delItem->close();
        delete delItem;
    }

    if (removeSceneIdx != -1) {
        iter = sceneItemVector.begin();
        if (sceneItemVector.size() != 1 && removeSceneIdx != 0) {
            for (int index = 0; iter != sceneItemVector.end(); ++iter, ++index) {
                if ((removeSceneIdx-1) == index)
                    break;
            }
        }
        if (iter == sceneItemVector.end())
            return;

        AFQSceneListItem* newSelectedSceneItem = (*iter);
        if (newSelectedSceneItem) {
            OBSSource sceneSource = obs_scene_get_source(newSelectedSceneItem->GetScene());
            DYNAMIC_COMPOSIT->SetCurrentScene(sceneSource);
        }
    }
}

void AFSceneSourceWidget::RefreshSceneItem()
{
    int buttonWidth = AF_SCENE_ITEM_WIDTH - (m_dockWideMode * 20);
    int buttonSelectedWidth = AF_SCENE_SELECT_ITEM_WIDTH - (m_dockWideMode * 20);

    SceneItemVector& sceneItemVector = SCENE_CONTEXT.GetSceneItemVector();
    const AFQSceneListItem* selectedItem = SCENE_CONTEXT.GetCurSelectedSceneItem();

    AFQSceneListItem* selectedWidget = nullptr;

    int index = 0;
    int height = 1;
    int itemHeight = AF_SCENE_ITEM_HEIGHT;

    for (auto iter = sceneItemVector.begin(); iter != sceneItemVector.end(); ++iter, index++) {
        AFQSceneListItem* item = (*iter);
        if (nullptr == item)
            continue;

        bool selectedScene = (selectedItem == item);
        int itemWidth = selectedScene ? buttonSelectedWidth : buttonWidth;

        item->SetSceneIndexLabelNum(index);
        item->SelectScene(selectedScene);
        item->setGeometry(AF_SCENE_ITEM_SPACE_XPOS, height, itemWidth, itemHeight);
        item->repaint();
        item->show();

        if (selectedScene)
            selectedWidget = item;

        height += (itemHeight + AF_SCENE_ITEM_SPACE_YPOS);
    }

    m_sceneAddButton->setGeometry(AF_SCENE_ITEM_SPACE_XPOS, height, buttonWidth, AF_SCENE_ITEM_HEIGHT);
    height += (AF_SCENE_ITEM_HEIGHT + AF_SCENE_ITEM_SPACE_YPOS);

    ui->sceneListScrollAreaContents->setMinimumHeight(height);
    ui->sceneListScrollAreaContents->SetTotalSceneCount(sceneItemVector.size());

    AFQSceneListItem* target = selectedWidget;
    QTimer::singleShot(0, this, [this, target]() {
        ui->scenListScrollArea->ensureWidgetVisible(
            target, 0, AF_SCENE_ITEM_SPACE_YPOS
        );
    });
}

OBSSceneItem AFSceneSourceWidget::GetCurrentSceneItem(int idx_)
{
    int idx = idx_;
    if(idx_ == -1)
        idx = ui->sourceListView->GetTopSelectedSourceItem();

    return ui->sourceListView->Get(idx);
}

int AFSceneSourceWidget::GetCurrentTopSelectedSceneItemIdx()
{
    return ui->sourceListView->GetTopSelectedSourceItem();
}

void AFSceneSourceWidget::SourceToolBarButtonSetEnable()
{
    bool enable = false;
    bool config = false;
    OBSSceneItem item = GetCurrentSceneItem();
    if (item) {
        OBSSource source = obs_sceneitem_get_source(item);
        config = AFSourceUtil::ShouldShowProperties(source);

        enable = true;

        if (AFSourceUtil::IsMustTopSource(source)) {
            ui->moveUpButton->setEnabled(false);
            ui->moveDownButton->setEnabled(false);
        }

    }

    ui->removeButton->setEnabled(enable);
    ui->fitScreenButton->setEnabled(enable);
    ui->restoreButton->setEnabled(enable);
    ui->moveUpButton->setEnabled(enable);
    ui->moveDownButton->setEnabled(enable);

    ui->propsButton->setEnabled(config);
}

#pragma region _SOOP_BREAKTIME
void AFSceneSourceWidget::SetBreaktime(bool enable, QString name)
{
    if(enable)
    {
        ui->labelBreaktimeName->setText(name);
        ui->labelBreaktimeInfo->setText(QTStr("breaktime.scene.info"));
        ui->stackedWidget->setCurrentIndex(1);
    } else
    {
        ui->stackedWidget->setCurrentIndex(0);
        //ui->labelBreaktimeName->setText("");
        //ui->labelBreaktimeInfo->setText("");
    }
}
#pragma endregion
void AFSceneSourceWidget::SetCurrentScene(OBSSource scene, bool force)
{
    SceneItemVector& sceneItemVector = SCENE_CONTEXT.GetSceneItemVector();

    OBSScene curScene = SCENE_CONTEXT.GetCurrentScene();

    if (obs_scene_get_source(curScene) != scene || force) {
        auto iter = sceneItemVector.begin();
        for ( ; iter != sceneItemVector.end() ; ++iter) {
            AFQSceneListItem* item = (*iter);
            if (!item)
                continue;

            OBSScene itemScene = item->GetScene();
            obs_source_t* source = obs_scene_get_source(itemScene);

            if (source == scene) {
                SCENE_CONTEXT.SetCurrentScene(itemScene.Get());
                SCENE_CONTEXT.SetCurSelectedSceneItem(item);

                VCamConfig& config = MAINFRAME->VirtualCamConfig();
                if(MAINFRAME->VirtualCamEnabled() &&
                   config.type == VCamOutputType::PreviewOutput) {
                    MAINFRAME->SetVirtualCamOutputType(VCamOutputType::PreviewOutput);
                }

                MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
            }
        }

        ui->sourceListView->RefreshSourceItem();

        RefreshSceneItem();
    }
}

/* OBS Callbacks */

void AFSceneSourceWidget::SceneReordered(void* data, calldata_t* params)
{
    AFSceneSourceWidget* sceneDock = static_cast<AFSceneSourceWidget*>(data);

    obs_scene_t* scene = (obs_scene_t*)calldata_ptr(params, "scene");

    QMetaObject::invokeMethod(sceneDock, "ReorderSources",
                              Q_ARG(OBSScene, OBSScene(scene)));
}

void AFSceneSourceWidget::SceneRefreshed(void* data, calldata_t* params)
{
    AFSceneSourceWidget* sceneDock = static_cast<AFSceneSourceWidget*>(data);

    obs_scene_t* scene = (obs_scene_t*)calldata_ptr(params, "scene");

    QMetaObject::invokeMethod(sceneDock, "RefreshSources",
                              Q_ARG(OBSScene, OBSScene(scene)));
}

void AFSceneSourceWidget::SceneItemAdded(void* data, calldata_t* params)
{
    AFSceneSourceWidget* sceneDock = static_cast<AFSceneSourceWidget*>(data);

    obs_sceneitem_t* item = (obs_sceneitem_t*)calldata_ptr(params, "item");
   
    QMetaObject::invokeMethod(sceneDock, "AddSceneItem",
                              Q_ARG(OBSSceneItem, OBSSceneItem(item)));
}

void AFSceneSourceWidget::_InitSceneSourceDockUI()
{
    ui->removeButton->setEnabled(false);
    ui->moveUpButton->setEnabled(false);
    ui->moveDownButton->setEnabled(false);
    ui->propsButton->setEnabled(false);
    ui->fitScreenButton->setEnabled(false);
    ui->restoreButton->setEnabled(false);
}

void AFSceneSourceWidget::_InitSceneSourceDockSignalSlot()
{

    connect(ui->sceneListScrollAreaContents, &AFQListScrollAreaContent::qsignalSwapItem,
            this, &AFSceneSourceWidget::qslotSwapItem);

    connect(ui->sceneListFrame, &AFQSceneListView::qsignalSwapItem,
            this, &AFSceneSourceWidget::qslotSwapItem);

    connect(ui->addButton, &QPushButton::clicked,
            this, &AFSceneSourceWidget::qslotAddSourceTrigger);

    connect(ui->removeButton, &QPushButton::clicked,
            this, &AFSceneSourceWidget::qslotRemoveSourceTrigger);

    connect(ui->moveUpButton, &QPushButton::clicked,
            this, &AFSceneSourceWidget::qslotMoveUpSourceTrigger);
     
    connect(ui->moveDownButton, &QPushButton::clicked,
            this, &AFSceneSourceWidget::qslotMoveDownSourceTrigger);

    connect(ui->propsButton, &QPushButton::clicked,
        this, &AFSceneSourceWidget::qslotShowPropsTrigger);

    connect(ui->fitScreenButton, &QPushButton::clicked,
        this, &AFSceneSourceWidget::qslotFitScreenSizeSourceTrigger);

    connect(ui->restoreButton, &QPushButton::clicked,
        this, &AFSceneSourceWidget::qslotRestoreSourceTrigger);

}

void AFSceneSourceWidget::_MoveSceneItem(enum obs_order_movement movement, const QString& action_name)
{
    OBSSceneItem item = GetCurrentSceneItem();
    obs_source_t* source = obs_sceneitem_get_source(item);

    if (!source)
        return;

    // soop vod move exception
    if (movement == OBS_ORDER_MOVE_DOWN || movement == OBS_ORDER_MOVE_BOTTOM) {
        if (AFSourceUtil::IsMustTopSource(source))
            return;
    }
    else if (movement == OBS_ORDER_MOVE_UP) {
        int selectedIdx = GetCurrentTopSelectedSceneItemIdx();
        if (0 == selectedIdx)
            return;

        OBSSceneItem upperItem = GetCurrentSceneItem(selectedIdx - 1);
        obs_source_t* upperSource = obs_sceneitem_get_source(upperItem);
        if (AFSourceUtil::IsMustTopSource(upperSource))
            return;
    }
    else if (movement == OBS_ORDER_MOVE_TOP) {
        OBSSceneItem topItem = GetCurrentSceneItem(0);
        obs_source_t* topSource = obs_sceneitem_get_source(topItem);
        if (AFSourceUtil::IsMustTopSource(topSource))
            return;
    }

    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
    std::vector<obs_source_t*> sources;
    if (scene != obs_sceneitem_get_scene(item))
        sources.push_back(
            obs_scene_get_source(obs_sceneitem_get_scene(item)));

    OBSData undo_data = MAINFRAME->BackupScene(scene, &sources);

    obs_sceneitem_set_order(item, movement);

    const char* source_name = obs_source_get_name(source);
    const char* scene_name = obs_source_get_name(obs_scene_get_source(scene));

    OBSData redo_data = MAINFRAME->BackupScene(scene, &sources);
    MAINFRAME->CreateSceneUndoRedoAction(action_name.arg(source_name, scene_name), undo_data, redo_data);
}