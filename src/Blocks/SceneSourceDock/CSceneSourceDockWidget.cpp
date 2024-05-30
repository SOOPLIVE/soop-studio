#include "CSceneSourceDockWidget.h"
#include "ui_scene-source-dock.h"

#include <QPushButton>
#include <QListWidget>
#include <QScrollBar>

#include "CoreModel/Source/CSource.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"

#include "Application/CApplication.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#define AF_SCENE_ITEM_WIDTH         (192)
#define AF_SCENE_SELECT_ITEM_WIDTH  (201)
#define AF_SCENE_ITEM_HEIGHT        (44)
#define AF_SCENE_ITEM_HOVER_HEIGHT  (140)
#define AF_SCENE_ITEM_SPACE_YPOS    (4)
#define AF_SCENE_ITEM_SPACE_XPOS    (4)

void AFSceneSourceDockWidget::qSlotClickedSceneItem()
{
    AFSceneContext&   sceneContext = AFSceneContext::GetSingletonInstance();
    AFQSceneListItem* clickedItem  = sceneContext.GetCurSelectedSceneItem();

    obs_source_t* source = obs_scene_get_source(clickedItem->GetScene());

    // Temp
    App()->GetMainView()->GetMainWindow()->SetCurrentScene(source);
}

void AFSceneSourceDockWidget::qSlotRenameSceneItem()
{
    App()->GetMainView()->RefreshSceneUI();
}

void AFSceneSourceDockWidget::qSlotDeleteSceneItem()
{

}

void AFSceneSourceDockWidget::qSlotHoverSceneItem(OBSScene scene)
{
    RefreshSceneItem();
}

void AFSceneSourceDockWidget::qSlotSwapItem(int from, int dest)
{
    if (from == dest)
        return;

    AFSceneContext& contextScene = AFSceneContext::GetSingletonInstance();
    contextScene.SwapSceneItem(from, dest);

    RefreshSceneItem();

    App()->GetMainView()->RefreshSceneUI();
}

void AFSceneSourceDockWidget::qSlotCheckSourceClicked(bool clicked)
{
    ui->removeButton->setEnabled(clicked);
    ui->moveUpButton->setEnabled(clicked);
    ui->moveDownButton->setEnabled(clicked);

    bool config = false;
    if (clicked) {
        obs_source_t* source = obs_sceneitem_get_source(GetCurrentSceneItem());
        if (source) {
            config = obs_source_configurable(source);
        }
    } else {
        config = false;
    }
    ui->propsButton->setEnabled(config);
}

void AFSceneSourceDockWidget::qSlotAddSceneButtonClicked()
{
    emit qSignalAddScene();
}

void AFSceneSourceDockWidget::qSlotAddSourceTrigger()
{
    emit qSignalAddSource();
}

void AFSceneSourceDockWidget::qSlotRemoveSourceTrigger()
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    AFSourceUtil::RemoveSourceItems(sceneContext.GetCurrOBSScene());
}

void AFSceneSourceDockWidget::qSlotMoveUpSourceTrigger()
{
    _MoveSceneItem(OBS_ORDER_MOVE_UP, QTStr("Undo.MoveUp"));
}

void AFSceneSourceDockWidget::qSlotMoveDownSourceTrigger()
{
    _MoveSceneItem(OBS_ORDER_MOVE_DOWN, QTStr("Undo.MoveDown"));
}

void AFSceneSourceDockWidget::qSlotMoveToTopSourceTrigger()
{
    _MoveSceneItem(OBS_ORDER_MOVE_TOP, QTStr("Undo.MoveToTop"));
}

void AFSceneSourceDockWidget::qSlotMoveToBottomSourceTrigger()
{
    _MoveSceneItem(OBS_ORDER_MOVE_BOTTOM, QTStr("Undo.MoveToBottom"));
}

void AFSceneSourceDockWidget::qSlotShowPropsTrigger()
{
    const auto item = GetCurrentSceneItem();
    if (!item)
        return;

    obs_source_t* source = obs_sceneitem_get_source(item);
    if (obs_source_configurable(source)) {
        App()->GetMainView()->CreatePropertiesPopup(source);
    }
}

void AFSceneSourceDockWidget::AddSceneItem(OBSSceneItem item)
{
    obs_scene_t* scene = obs_sceneitem_get_scene(item);

    AFLoadSaveManager&  loaderSaver = AFLoadSaveManager::GetSingletonInstance();
    AFSceneContext&     contextScene = AFSceneContext::GetSingletonInstance();

    if(contextScene.GetCurrOBSScene() == scene) {
        ui->sourceListView->Add(item);
    }

    if (!loaderSaver.CheckDisableSaving()) {
        obs_source_t* sceneSource = obs_scene_get_source(scene);
        obs_source_t* itemSource = obs_sceneitem_get_source(item);
        blog(LOG_INFO, "User added source '%s' (%s) to scene '%s'",
             obs_source_get_name(itemSource), obs_source_get_id(itemSource),
             obs_source_get_name(sceneSource));

        AFSourceUtil::SelectedItemOne(scene, (obs_sceneitem_t*)item);
    }
}

void AFSceneSourceDockWidget::ReorderSources(OBSScene scene)
{
    AFSceneContext& contextScene = AFSceneContext::GetSingletonInstance();

    if (scene != contextScene.GetCurrOBSScene() || ui->sourceListView->IgnoreReorder())
        return;

    ui->sourceListView->ReorderItems();
    //SaveProject();
}


void AFSceneSourceDockWidget::RefreshSources(OBSScene scene)
{
    AFSceneContext& contextScene = AFSceneContext::GetSingletonInstance();

    if (scene != contextScene.GetCurrOBSScene() || ui->sourceListView->IgnoreReorder())
        return;

    ui->sourceListView->RefreshItems();
    //SaveProject();
}

AFSceneSourceDockWidget::AFSceneSourceDockWidget(QWidget *parent) :
    AFQBaseDockWidget(parent),
    ui(new Ui::AFSceneSourceDockWidget)
{
    ui->setupUi(this);

    _InitSceneSourceDockUI();

    _InitSceneSourceDockSignalSlot();

    ui->sceneListScrollAreaContents->SetScrollAreaPtr(ui->scenListScrollArea);

    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
    sceneContext.SetSourceListViewPtr(ui->sourceListView);

    if (!m_sceneAddButton) {
        m_sceneAddButton = new QPushButton(ui->sceneListScrollAreaContents);
        m_sceneAddButton->setObjectName("sceneListAddButton");
        m_sceneAddButton->setText(Str("Basic.SceneSourceDock.AddScene"));
        connect(m_sceneAddButton, &QPushButton::clicked,
                this, &AFSceneSourceDockWidget::qSlotAddSceneButtonClicked);
    }
}

AFSceneSourceDockWidget::~AFSceneSourceDockWidget()
{
    delete ui;
}

void AFSceneSourceDockWidget::AddScene(OBSSource source)
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    const char* name = obs_source_get_name(source);
    obs_scene_t* scene = obs_scene_from_source(source);

    obs_hotkey_register_source(
        source, "OBSBasic.SelectScene",
        Str("Basic.Hotkeys.SelectScene"),
        [](void* data, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
            AFMainFrame* main = reinterpret_cast<AFMainFrame*>(
                        App()->GetMainView());

            auto potential_source =
                static_cast<obs_source_t*>(data);
            OBSSourceAutoRelease source =
                obs_source_get_ref(potential_source);
            if (source && pressed)
                main->GetMainWindow()->SetCurrentScene(source.Get());
        },
        static_cast<obs_source_t*>(source));

    signal_handler_t* handler = obs_source_get_signal_handler(source);

    SignalContainer<OBSScene> container;
    container.ref = scene;
    container.handlers.assign({
                std::make_shared<OBSSignal>(handler, "item_add",
                                AFSceneSourceDockWidget::SceneItemAdded, this),
                std::make_shared<OBSSignal>(handler, "reorder",
                                AFSceneSourceDockWidget::SceneReordered, this),
                std::make_shared<OBSSignal>(handler, "refresh",
                                AFSceneSourceDockWidget::SceneRefreshed, this),
        });

    AFQSceneListItem* sceneItem = new AFQSceneListItem(ui->sceneListScrollAreaContents, ui->sceneListFrame, scene, name, container);

    connect(sceneItem, &AFQSceneListItem::qSignalClickedSceneItem, 
            this, &AFSceneSourceDockWidget::qSlotClickedSceneItem);
    connect(sceneItem, &AFQSceneListItem::qSignalDoubleClickedSceneItem,
        this, &AFSceneSourceDockWidget::qSignalSceneDoubleClickedTriggered);
    connect(sceneItem, &AFQSceneListItem::qSignalRenameSceneItem, 
            this, &AFSceneSourceDockWidget::qSlotRenameSceneItem);
    connect(sceneItem, &AFQSceneListItem::qSignalDeleteSceneItem, 
            this, &AFSceneSourceDockWidget::qSlotDeleteSceneItem);
    connect(sceneItem, &AFQSceneListItem::qSignalHoverSceneItem, 
            this, &AFSceneSourceDockWidget::qSlotHoverSceneItem);

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

    sceneContext.SetCurSelectedSceneItem(sceneItem);
    sceneContext.AddSceneItem(sceneItem);

    RefreshSceneItem();
}

void AFSceneSourceDockWidget::RemoveScene(OBSSource source)
{
    AFSceneContext& contextScene = AFSceneContext::GetSingletonInstance();
    SceneItemVector& sceneItemVector = contextScene.GetSceneItemVector();

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
            App()->GetMainView()->GetMainWindow()->SetCurrentScene(sceneSource);
        }
    }
}

void AFSceneSourceDockWidget::RefreshSceneItem()
{
    AFSceneContext& contextScene = AFSceneContext::GetSingletonInstance();
    SceneItemVector& sceneItemVector = contextScene.GetSceneItemVector();

    const AFQSceneListItem* selectedItem = contextScene.GetCurSelectedSceneItem();

    int index = 0;
    int height = AF_SCENE_ITEM_SPACE_YPOS;
    auto iter = sceneItemVector.begin();
    int itemHeight = AF_SCENE_ITEM_HEIGHT;
    for (; iter != sceneItemVector.end(); ++iter, index++) {
        AFQSceneListItem* item = (*iter);
        if (nullptr == item)
            continue;

        bool selectedScene = false;
        int itemWidth = AF_SCENE_ITEM_WIDTH;
        if (selectedItem == item) {
            itemWidth = AF_SCENE_SELECT_ITEM_WIDTH;
            selectedScene = true;
        }

        item->SetSceneIndexLabelNum(index);
        item->SelectScene(selectedScene);
        item->setGeometry(AF_SCENE_ITEM_SPACE_XPOS, height, itemWidth, itemHeight);
        item->repaint();
        item->show();

        height += (itemHeight + AF_SCENE_ITEM_SPACE_YPOS);
    }

    m_sceneAddButton->setGeometry(AF_SCENE_ITEM_SPACE_XPOS, height, AF_SCENE_ITEM_WIDTH, AF_SCENE_ITEM_HEIGHT);
    height += (AF_SCENE_ITEM_HEIGHT + AF_SCENE_ITEM_SPACE_YPOS);

    ui->sceneListScrollAreaContents->setMinimumHeight(height);
    ui->sceneListScrollAreaContents->SetTotalSceneCount(sceneItemVector.size());
}

void AFSceneSourceDockWidget::RegisterShortCut(QAction* removeSourceAction)
{
    ui->sourceListView->RegisterShortCut(removeSourceAction);
}

OBSSceneItem AFSceneSourceDockWidget::GetCurrentSceneItem(int idx_)
{
    int idx = idx_;
    if(idx_ == -1)
        idx = ui->sourceListView->GetTopSelectedSourceItem();

    return ui->sourceListView->Get(idx);
}

void AFSceneSourceDockWidget::SetCurrentScene(OBSSource scene, bool force)
{
    AFSceneContext& contextScene = AFSceneContext::GetSingletonInstance();
    SceneItemVector& sceneItemVector = contextScene.GetSceneItemVector();

    OBSScene curScene = contextScene.GetCurrOBSScene();

    if (obs_scene_get_source(curScene) != scene || force) {
        auto iter = sceneItemVector.begin();
        for ( ; iter != sceneItemVector.end() ; ++iter) {
            AFQSceneListItem* item = (*iter);
            if (!item)
                continue;

            OBSScene itemScene = item->GetScene();
            obs_source_t* source = obs_scene_get_source(itemScene);

            if (source == scene) {
                contextScene.SetCurrOBSScene(itemScene.Get());
                contextScene.SetCurSelectedSceneItem(item);
            }
        }

        ui->sourceListView->RefreshSourceItem();

        RefreshSceneItem();
    }
}

/* OBS Callbacks */

void AFSceneSourceDockWidget::SceneReordered(void* data, calldata_t* params)
{
    AFSceneSourceDockWidget* sceneDock = static_cast<AFSceneSourceDockWidget*>(data);

    obs_scene_t* scene = (obs_scene_t*)calldata_ptr(params, "scene");

    QMetaObject::invokeMethod(sceneDock, "ReorderSources",
                              Q_ARG(OBSScene, OBSScene(scene)));
}

void AFSceneSourceDockWidget::SceneRefreshed(void* data, calldata_t* params)
{
    AFSceneSourceDockWidget* sceneDock = static_cast<AFSceneSourceDockWidget*>(data);

    obs_scene_t* scene = (obs_scene_t*)calldata_ptr(params, "scene");

    QMetaObject::invokeMethod(sceneDock, "RefreshSources",
                              Q_ARG(OBSScene, OBSScene(scene)));
}

void AFSceneSourceDockWidget::SceneItemAdded(void* data, calldata_t* params)
{
    AFSceneSourceDockWidget* sceneDock = static_cast<AFSceneSourceDockWidget*>(data);

    obs_sceneitem_t* item = (obs_sceneitem_t*)calldata_ptr(params, "item");
   
    QMetaObject::invokeMethod(sceneDock, "AddSceneItem",
                              Q_ARG(OBSSceneItem, OBSSceneItem(item)));
}

void AFSceneSourceDockWidget::_InitSceneSourceDockUI()
{
    ui->removeButton->setEnabled(false);
    ui->moveUpButton->setEnabled(false);
    ui->moveDownButton->setEnabled(false);
    ui->propsButton->setEnabled(false);
}

void AFSceneSourceDockWidget::_InitSceneSourceDockSignalSlot()
{

    connect(ui->sceneListScrollAreaContents, &AFQListScrollAreaContent::qsignalSwapItem,
            this, &AFSceneSourceDockWidget::qSlotSwapItem);

    connect(ui->sceneListFrame, &AFQSceneListView::qsignalSwapItem,
            this, &AFSceneSourceDockWidget::qSlotSwapItem);

    connect(ui->sourceListView, &AFQSourceListView::qSignalCheckSourceClicked,
            this, &AFSceneSourceDockWidget::qSlotCheckSourceClicked);

    connect(ui->addButton, &QPushButton::clicked,
            this, &AFSceneSourceDockWidget::qSlotAddSourceTrigger);

    connect(ui->removeButton, &QPushButton::clicked,
            this, &AFSceneSourceDockWidget::qSlotRemoveSourceTrigger);

    connect(ui->moveUpButton, &QPushButton::clicked,
            this, &AFSceneSourceDockWidget::qSlotMoveUpSourceTrigger);
     
    connect(ui->moveDownButton, &QPushButton::clicked,
            this, &AFSceneSourceDockWidget::qSlotMoveDownSourceTrigger);

    connect(ui->propsButton, &QPushButton::clicked,
        this, &AFSceneSourceDockWidget::qSlotShowPropsTrigger);

}

void AFSceneSourceDockWidget::_MoveSceneItem(enum obs_order_movement movement, const QString& action_name)
{
    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    OBSSceneItem item = GetCurrentSceneItem();
    obs_source_t* source = obs_sceneitem_get_source(item);

    if (!source)
        return;

    OBSScene scene = sceneContext.GetCurrOBSScene();
    std::vector<obs_source_t*> sources;
    if (scene != obs_sceneitem_get_scene(item))
        sources.push_back(
            obs_scene_get_source(obs_sceneitem_get_scene(item)));

    AFMainFrame* main = App()->GetMainView();
    OBSData undo_data = main->BackupScene(scene, &sources);

    obs_sceneitem_set_order(item, movement);

    const char* source_name = obs_source_get_name(source);
    const char* scene_name = obs_source_get_name(obs_scene_get_source(scene));

    OBSData redo_data = main->BackupScene(scene, &sources);
    main->CreateSceneUndoRedoAction(action_name.arg(source_name, scene_name), undo_data, redo_data);
}