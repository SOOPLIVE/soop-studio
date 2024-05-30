#include "CSceneListView.h"

#include "CSceneListItem.h"
#include "CSceneSourceDockWidget.h"

#include <QSpinBox>
#include <QWidgetAction>
#include <QPushButton>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Source/CSource.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#include "UIComponent/CNameDialog.h"

using namespace std;

AFQSceneListView::AFQSceneListView(QWidget* parent)
	:QFrame(parent)
{
	setAcceptDrops(true);

	QAction* actionRename = new QAction(this);
	actionRename->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	actionRename->setShortcut({ Qt::Key_F2 });
	connect(actionRename, &QAction::triggered,
			this, &AFQSceneListView::_qSlotRenameScene);

	QAction* actionRemove = new QAction(this);
	actionRemove->setShortcutContext(Qt::WidgetWithChildrenShortcut);

#ifdef __APPLE__
	actionRemove->setShortcut({ Qt::Key_Backspace });
#else
	actionRemove->setShortcut({ Qt::Key_Delete });
#endif
	connect(actionRemove, &QAction::triggered,
			this, &AFQSceneListView::_qSlotRemoveScene);

	addAction(actionRename);
	addAction(actionRemove);

}

AFQSceneListView::~AFQSceneListView()
{

}

void AFQSceneListView::contextMenuEvent(QContextMenuEvent* event)
{
	_CreateSceneMenuPopup();

	QWidget::contextMenuEvent(event);
}


void AFQSceneListView::_CreateSceneMenuPopup()
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	AFQSceneListItem* selectedSceneItem = sceneContext.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	AFQCustomMenu menu(this);
	AFQCustomMenu order(locale.Str("Basic.MainMenu.Edit.Order"), this, true);

	QAction* duplicateScene = new QAction(locale.Str("Duplicate"), this);
	QAction* renameScene = new QAction(locale.Str("Rename"), this);
	QAction* removeScene = new QAction(locale.Str("RemoveScene"), this);

	connect(duplicateScene, &QAction::triggered, 
			this, &AFQSceneListView::_qSlotDuplicateSelectedScene);
	connect(renameScene, &QAction::triggered, this, &AFQSceneListView::_qSlotRenameScene);
	connect(removeScene, &QAction::triggered, this, &AFQSceneListView::_qSlotRemoveScene);

	QAction* copyFilters = new QAction(locale.Str("Copy.Filters"), this);
	QAction* pasteFilters = new QAction(locale.Str("Paste.Filters"), this);

	pasteFilters->setEnabled(!obs_weak_source_expired(sceneContext.m_obsCopyFiltersSource));
	connect(copyFilters, &QAction::triggered, this, &AFQSceneListView::_qSlotCopyFilters);
	connect(pasteFilters, &QAction::triggered, this, &AFQSceneListView::_qSlotPasteFilters);

	menu.addAction(duplicateScene);
	menu.addAction(renameScene);
	menu.addAction(removeScene);

	menu.addSeparator();

	menu.addAction(Str("Filters"), this,&AFQSceneListView::_qSlotShowSceneFilters);
	menu.addAction(copyFilters);
	menu.addAction(pasteFilters);

	menu.addSeparator();
	order.addAction(locale.Str("Basic.MainMenu.Edit.Order.MoveUp"), 
					this, &AFQSceneListView::_qSlotMoveSceneUp);
	order.addAction(locale.Str("Basic.MainMenu.Edit.Order.MoveDown"), 
					this, &AFQSceneListView::_qSlotMoveSceneDown);
	order.addSeparator();
	order.addAction(locale.Str("Basic.MainMenu.Edit.Order.MoveToTop"), 
					this, &AFQSceneListView::_qSlotMoveSceneToTop);
	order.addAction(locale.Str("Basic.MainMenu.Edit.Order.MoveToBottom"), 
					this, &AFQSceneListView::_qSlotMoveSceneToBottom);

	menu.addMenu(&order);

	menu.addSeparator();
	menu.addAction(Str("Screenshot.Scene"), this, &AFQSceneListView::_qSlotScreenshotScene);

	delete m_perSceneTransitionMenu;
	m_perSceneTransitionMenu = _CreatePerSceneTransitionMenu();
	//menu.addMenu(m_perSceneTransitionMenu);

	menu.exec(QCursor::pos());
}

AFQCustomMenu* AFQSceneListView::_CreatePerSceneTransitionMenu()
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	AFSceneContext& context = AFSceneContext::GetSingletonInstance();

	AFQCustomMenu* menu = new AFQCustomMenu(locale.Str("TransitionOverride"),this);
	QAction* action;

	OBSSource scene = OBSSource(obs_scene_get_source(context.GetCurrOBSScene()));
	OBSDataAutoRelease data = obs_source_get_private_settings(scene);

	obs_data_set_default_int(data, "transition_duration", 300);

	const char* curTransition = obs_data_get_string(data, "transition");
	int curDuration = (int)obs_data_get_int(data, "transition_duration");

	QSpinBox* duration = new QSpinBox(menu);
	duration->setMinimum(50);
	duration->setSuffix(" ms");
	duration->setMaximum(20000);
	duration->setSingleStep(50);
	duration->setValue(curDuration);

	QWidgetAction* durationAction = new QWidgetAction(menu);
	durationAction->setDefaultWidget(duration);

	menu->addAction("none");
	menu->addAction("cut");
	menu->addAction("fade out");
	menu->addAction("slide");

	menu->addSeparator();
	menu->addAction(durationAction);
	return menu;
}

void AFQSceneListView::_qSlotDuplicateSelectedScene()
{
	AFSceneContext& context = AFSceneContext::GetSingletonInstance();

	OBSScene curScene = context.GetCurrOBSScene();

	if (!curScene)
		return;

	OBSSource curSceneSource = obs_scene_get_source(curScene);
	QString format{ obs_source_get_name(curSceneSource) };
	format += " %1";

	int i = 2;
	QString placeHolderText = format.arg(i);
	OBSSourceAutoRelease source = nullptr;
	while ((source = obs_get_source_by_name(QT_TO_UTF8(placeHolderText)))) {
		placeHolderText = format.arg(++i);
	}

	for (;;) {
		string name;
		bool accepted = AFQNameDialog::AskForName(
			this, Str("Basic.Main.AddSceneDlg.Title"),
			Str("Basic.Main.AddSceneDlg.Text"), name,
			placeHolderText);

		if (!accepted)
			return;

		if (name.empty()) {
			continue;
		}

		obs_source_t* source = obs_get_source_by_name(name.c_str());
		if (source) {

			obs_source_release(source);
			continue;
		}

		OBSSceneAutoRelease scene = obs_scene_duplicate(
			curScene, name.c_str(), OBS_SCENE_DUP_REFS);
		source = obs_scene_get_source(scene);
		
		AFMainFrame* main = App()->GetMainView();
		main->GetMainWindow()->SetCurrentScene(source, true);
        
		auto undo = [](const std::string& data) {
			OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
			obs_source_remove(source);
		};
		auto redo = [this, name](const std::string& data) {
			OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
			obs_scene_t* scene = obs_scene_from_source(source);
			scene = obs_scene_duplicate(scene, name.c_str(),
				OBS_SCENE_DUP_REFS);
			source = obs_scene_get_source(scene);
			App()->GetMainView()->GetMainWindow()->SetCurrentScene(source.Get(), true);
		};

		main->m_undo_s.AddAction(QTStr("Undo.Scene.Duplicate").arg(obs_source_get_name(source)),
								 undo, redo, obs_source_get_name(source),
								 obs_source_get_name(obs_scene_get_source(curScene)));

		break;
	}

}


void AFQSceneListView::_qSlotRenameScene()
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
	AFQSceneListItem* selectedSceneItem = sceneContext.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	selectedSceneItem->ShowRenameSceneUI();
}

static bool save_undo_source_enum(obs_scene_t* /* scene */,
	obs_sceneitem_t* item, void* p)
{
	obs_source_t* source = obs_sceneitem_get_source(item);
	if (obs_obj_is_private(source) && !obs_source_removed(source))
		return true;

	obs_data_array_t* array = (obs_data_array_t*)p;

	/* check if the source is already stored in the array */
	const char* name = obs_source_get_name(source);
	const size_t count = obs_data_array_count(array);
	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease sourceData = obs_data_array_item(array, i);
		if (strcmp(name, obs_data_get_string(sourceData, "name")) == 0)
			return true;
	}

	if (obs_source_is_group(source))
		obs_scene_enum_items(obs_group_from_source(source), save_undo_source_enum, p);

	OBSDataAutoRelease source_data = obs_save_source(source);
	obs_data_array_push_back(array, source_data);
	return true;
}

static inline void RemoveSceneAndReleaseNested(obs_source_t* source)
{
	obs_source_remove(source);
	auto cb = [](void*, obs_source_t* source) {
		if (strcmp(obs_source_get_id(source), "scene") == 0)
			obs_scene_prune_sources(obs_scene_from_source(source));
		return true;
	};
	obs_enum_scenes(cb, NULL);
}

void AFQSceneListView::_qSlotRemoveScene()
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	AFQSceneListItem* selectedSceneItem = sceneContext.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;
	
	OBSScene scene = selectedSceneItem->GetScene();
	obs_source_t* source = obs_scene_get_source(scene);

	if (!source || !AFSourceUtil::QueryRemoveSource(source)) {
		return;
	}

	/* ------------------------------ */
	/* save all sources in scene      */

	OBSDataArrayAutoRelease sources_in_deleted_scene =
		obs_data_array_create();

	obs_scene_enum_items(scene, save_undo_source_enum,
		sources_in_deleted_scene);

	OBSDataAutoRelease scene_data = obs_save_source(source);
	obs_data_array_push_back(sources_in_deleted_scene, scene_data);

	OBSDataArrayAutoRelease scene_used_in_other_scenes =
		obs_data_array_create();

	struct other_scenes_cb_data {
		obs_source_t* oldScene;
		obs_data_array_t* scene_used_in_other_scenes;
	} other_scenes_cb_data;
	other_scenes_cb_data.oldScene = source;
	other_scenes_cb_data.scene_used_in_other_scenes =
		scene_used_in_other_scenes;

	auto other_scenes_cb = [](void* data_ptr, obs_source_t* scene) {
		struct other_scenes_cb_data* data =
			(struct other_scenes_cb_data*)data_ptr;
		if (strcmp(obs_source_get_name(scene),
			obs_source_get_name(data->oldScene)) == 0)
			return true;
		obs_sceneitem_t* item = obs_scene_find_source(
			obs_group_or_scene_from_source(scene),
			obs_source_get_name(data->oldScene));
		if (item) {
			OBSDataAutoRelease scene_data =
				obs_save_source(obs_scene_get_source(
					obs_sceneitem_get_scene(item)));
			obs_data_array_push_back(
				data->scene_used_in_other_scenes, scene_data);
		}
		return true;
	};
	obs_enum_scenes(other_scenes_cb, &other_scenes_cb_data);

	/* --------------------------- */
	/* undo/redo                   */

	auto undo = [this](const std::string& json) {
		OBSDataAutoRelease base = obs_data_create_from_json(json.c_str());
		OBSDataArrayAutoRelease sources_in_deleted_scene = obs_data_get_array(base, "sources_in_deleted_scene");
		OBSDataArrayAutoRelease scene_used_in_other_scenes = obs_data_get_array(base, "scene_used_in_other_scenes");
		int savedIndex = (int)obs_data_get_int(base, "index");
		std::vector<OBSSource> sources;

		/* create missing sources */
		size_t count = obs_data_array_count(sources_in_deleted_scene);
		sources.reserve(count);

		for(size_t i = 0; i < count; i++)
		{
			OBSDataAutoRelease data = obs_data_array_item(sources_in_deleted_scene, i);
			const char* name = obs_data_get_string(data, "name");

			OBSSourceAutoRelease source = obs_get_source_by_name(name);
			if(!source) {
				source = obs_load_source(data);
				sources.push_back(source.Get());
			}
		}

		/* actually load sources now */
		for(obs_source_t* source : sources)
			obs_source_load2(source);

		/* Add scene to scenes and groups it was nested in */
		for(size_t i = 0; i < obs_data_array_count(scene_used_in_other_scenes); i++)
		{
			OBSDataAutoRelease data = obs_data_array_item(scene_used_in_other_scenes, i);
			const char* name = obs_data_get_string(data, "name");
			OBSSourceAutoRelease source = obs_get_source_by_name(name);
			OBSDataAutoRelease settings = obs_data_get_obj(data, "settings");
			OBSDataArrayAutoRelease items = obs_data_get_array(settings, "items");

			/* Clear scene, but keep a reference to all sources in the scene to make sure they don't get destroyed */
			std::vector<OBSSource> existing_sources;
			auto cb = [](obs_scene_t*, obs_sceneitem_t* item, void* data) {
				std::vector<OBSSource>* existing = (std::vector<OBSSource> *)data;
				OBSSource source = obs_sceneitem_get_source(item);
				obs_sceneitem_remove(item);
				existing->push_back(source);
				return true;
			};
			obs_scene_enum_items(obs_group_or_scene_from_source(source), cb, (void*)&existing_sources);

			/* Re-add sources to the scene */
			obs_sceneitems_add( obs_group_or_scene_from_source(source), items);
		}

		obs_source_t* scene_source = sources.back();
		OBSScene scene = obs_scene_from_source(scene_source);
		App()->GetMainView()->GetMainWindow()->SetCurrentScene(scene, true);

		/* set original index in list box */
        AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
        qsignalSwapItem(sceneContext.GetSceneItemSize() - 1, savedIndex);
	};

	auto redo = [](const std::string& name) {
		OBSSourceAutoRelease source = obs_get_source_by_name(name.c_str());
		RemoveSceneAndReleaseNested(source);
	};

	OBSDataAutoRelease data = obs_data_create();
	obs_data_set_array(data, "sources_in_deleted_scene", sources_in_deleted_scene);
	obs_data_set_array(data, "scene_used_in_other_scenes", scene_used_in_other_scenes);
    obs_data_set_int(data, "index", selectedSceneItem->GetSceneIndex());

	const char* scene_name = obs_source_get_name(source);
	AFMainFrame* main = App()->GetMainView();
	main->m_undo_s.AddAction(QTStr("Undo.Delete").arg(scene_name), undo, redo, obs_data_get_json(data), scene_name);

	/* --------------------------- */
	/* remove                      */

	RemoveSceneAndReleaseNested(source);
}

void AFQSceneListView::_qSlotCopyFilters()
{
	AFSceneContext& context = AFSceneContext::GetSingletonInstance();
	OBSSource source = OBSSource(obs_scene_get_source(context.GetCurrOBSScene()));
	context.m_obsCopyFiltersSource = obs_source_get_weak_source(source);
}

void AFQSceneListView::_qSlotPasteFilters()
{
	AFSceneContext& context = AFSceneContext::GetSingletonInstance();

	OBSSourceAutoRelease source =
		obs_weak_source_get_source(context.m_obsCopyFiltersSource);

	OBSSource dstSource = OBSSource(obs_scene_get_source(context.GetCurrOBSScene()));

	if (source == dstSource)
		return;

	obs_source_copy_filters(dstSource, source);
}

void AFQSceneListView::_qSlotScreenshotScene()
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	App()->GetMainView()->Screenshot(sceneContext.GetCurrOBSSceneSource());
}

void AFQSceneListView::_qSlotMoveSceneUp()
{
	AFSceneContext&		sceneContext = AFSceneContext::GetSingletonInstance();
	AFQSceneListItem*	selectedSceneItem = sceneContext.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	const int index = selectedSceneItem->GetSceneIndex();
	if (index == 0)
		return;

	emit qsignalSwapItem(index, index - 1);
}

void AFQSceneListView::_qSlotMoveSceneDown()
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
	AFQSceneListItem* selectedSceneItem = sceneContext.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	const int index = selectedSceneItem->GetSceneIndex();
	if (index == sceneContext.GetSceneItemSize())
		return;

	emit qsignalSwapItem(index, index + 1);
}

void AFQSceneListView::_qSlotMoveSceneToTop()
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
	AFQSceneListItem* selectedSceneItem = sceneContext.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	const int index = selectedSceneItem->GetSceneIndex();
	if (index == 0)
		return;

	emit qsignalSwapItem(index, 0);
}

void AFQSceneListView::_qSlotMoveSceneToBottom()
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
	AFQSceneListItem* selectedSceneItem = sceneContext.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	const int index = selectedSceneItem->GetSceneIndex();
	const int dest = sceneContext.GetSceneItemSize() - 1;
	if (index == dest)
		return;

	emit qsignalSwapItem(index, dest);
}

void AFQSceneListView::_qSlotShowSceneFilters()
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	OBSScene scene = sceneContext.GetCurrOBSScene();
	OBSSource source = obs_scene_get_source(scene);

	App()->GetMainView()->CreateFiltersWindow(source);
}