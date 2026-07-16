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
#include "MainFrame/SceneSource/CMainSceneSource.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#include "UIComponent/CNameDialog.h"

#include "Blocks/SceneControlDock/CProjector.h"

using namespace std;

AFQSceneListView::AFQSceneListView(QWidget* parent)
	:QFrame(parent)
{
	setAcceptDrops(true);
	setFocusPolicy(Qt::StrongFocus);
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
	AFQSceneListItem* selectedSceneItem = SCENE_CONTEXT.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	OBSScene scene = selectedSceneItem->GetScene();

	obs_source_t* source = obs_scene_get_source(scene);
	obs_data_t* scene_data = obs_source_get_settings(source);

	AFQCustomMenu menu(this);
	AFQCustomMenu order(Str("Basic.MainMenu.Edit.Order"), this, true);

	QAction* duplicateScene = new QAction(Str("Duplicate"), this);
	QAction* renameScene = new QAction(Str("Rename"), this);
	QAction* removeScene = new QAction(Str("RemoveScene"), this);

	int isFavoriteScene = obs_data_get_int(scene_data, "favorite_scene");
	int nFavoriteSceneCount = SCENE_CONTEXT.GetFavoriteSceneCount();
	const int nMaxFavoriteSceneSize = SCENE_CONTEXT.GetFavoriteSceneMaxCount();
	QString str;
	if (isFavoriteScene) {
		str = QTStr("Basic.Scene.FavoriteOff").arg(nFavoriteSceneCount).arg(nMaxFavoriteSceneSize);
	} else {
		str = QTStr("Basic.Scene.Favorite").arg(nFavoriteSceneCount).arg(nMaxFavoriteSceneSize);
	}

	QAction* favoriteScene = new QAction(Str(str.toStdString().c_str()), this);

	connect(duplicateScene, &QAction::triggered, this, &AFQSceneListView::_qslotDuplicateSelectedScene);
	connect(renameScene, &QAction::triggered, MAIN_SCENESOURCE, &CMainSceneSource::qslotActionRenameScene);
	connect(removeScene, &QAction::triggered, MAIN_SCENESOURCE, &CMainSceneSource::qslotActionRemoveScene);
	connect(favoriteScene, &QAction::triggered, this, &AFQSceneListView::_qslotRegisterFavoriteScene);

	QAction* copyFilters = new QAction(Str("Copy.Filters"), this);
	QAction* pasteFilters = new QAction(Str("Paste.Filters"), this);

	pasteFilters->setEnabled(!obs_weak_source_expired(SCENE_CONTEXT.m_obsCopyFiltersSource));
	connect(copyFilters, &QAction::triggered, this, &AFQSceneListView::_qslotCopyFilters);
	connect(pasteFilters, &QAction::triggered, this, &AFQSceneListView::_qslotPasteFilters);

	menu.addAction(duplicateScene);
	menu.addAction(renameScene);
	menu.addAction(removeScene);
	menu.addAction(favoriteScene);

	menu.addSeparator();

	menu.addAction(Str("Filters"), this,&AFQSceneListView::_qslotShowSceneFilters);
	menu.addAction(copyFilters);
	menu.addAction(pasteFilters);

	menu.addSeparator();
	order.addAction(Str("Basic.MainMenu.Edit.Order.MoveUp"), 
					this, &AFQSceneListView::_qslotMoveSceneUp);
	order.addAction(Str("Basic.MainMenu.Edit.Order.MoveDown"), 
					this, &AFQSceneListView::_qslotMoveSceneDown);
	order.addSeparator();
	order.addAction(Str("Basic.MainMenu.Edit.Order.MoveToTop"), 
					this, &AFQSceneListView::_qslotMoveSceneToTop);
	order.addAction(Str("Basic.MainMenu.Edit.Order.MoveToBottom"), 
					this, &AFQSceneListView::_qslotMoveSceneToBottom);

	menu.addMenu(&order);

	menu.addSeparator();

	AFQCustomMenu* fullScreen = MAINFRAME->CreateFullScreenProjectorMenu();
	menu.addMenu(fullScreen);

	QAction* windowProjectorAction = new QAction(Str("SceneWindow"));
	windowProjectorAction->setProperty("monitor", -1);
	windowProjectorAction->setProperty("fullscreen", false);
	connect(windowProjectorAction, &QAction::triggered, MAINFRAME, &AFMainFrame::qslotShowProjector);
	menu.addAction(windowProjectorAction);

	menu.addAction(Str("Screenshot.Scene"), this, &AFQSceneListView::_qslotScreenshotScene);

	delete m_perSceneTransitionMenu;
	m_perSceneTransitionMenu = _CreatePerSceneTransitionMenu();
	//menu.addMenu(m_perSceneTransitionMenu);

	menu.exec(QCursor::pos());
}

AFQCustomMenu* AFQSceneListView::_CreatePerSceneTransitionMenu()
{
	AFQCustomMenu* menu = new AFQCustomMenu(Str("TransitionOverride"),this);
	QAction* action;

	OBSSource scene = OBSSource(obs_scene_get_source(SCENE_CONTEXT.GetCurrentScene()));
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

void AFQSceneListView::_qslotDuplicateSelectedScene()
{
	OBSScene curScene = SCENE_CONTEXT.GetCurrentScene();
	if (!curScene)
		return;

	OBSSource curSceneSource = obs_scene_get_source(curScene);
	QString format{ obs_source_get_name(curSceneSource) };
	format += " %1";

	int i = 2;
	QString placeHolderText = format.arg(i);
	OBSSourceAutoRelease findSrc = nullptr;
	while ((findSrc = obs_get_source_by_name(QT_TO_UTF8(placeHolderText)))) {
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
			//OBSMessageBox::warning(this,
			//	"NoNameEntered.Title",
			//	"NoNameEntered.Text");
			continue;
		}

		obs_source_t* source = obs_get_source_by_name(name.c_str());
		if (source) {
			//OBSMessageBox::warning(this,"NameExists.Title",
			//	"NameExists.Text");

			obs_source_release(source);
			continue;
		}

		OBSSceneAutoRelease scene = obs_scene_duplicate(curScene, name.c_str(), OBS_SCENE_DUP_REFS);
		source = obs_scene_get_source(scene);
		
		DYNAMIC_COMPOSIT->SetCurrentScene(source, true);
        
		auto undo = [](const std::string& data) {
			OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
			obs_source_remove(source);
		};
		auto redo = [this, name](const std::string& data) {
			OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
			obs_scene_t* scene = obs_scene_from_source(source);
			scene = obs_scene_duplicate(scene, name.c_str(), OBS_SCENE_DUP_REFS);
			source = obs_scene_get_source(scene);
			DYNAMIC_COMPOSIT->SetCurrentScene(source.Get(), true);
		};

		UNDO_STACK.AddAction(QTStr("Undo.Scene.Duplicate").arg(obs_source_get_name(source)),
							 undo, redo, obs_source_get_name(source),
							 obs_source_get_name(obs_scene_get_source(curScene)));

		break;
	}

}

void AFQSceneListView::_qslotRegisterFavoriteScene()
{
	AFQSceneListItem* selectedSceneItem = SCENE_CONTEXT.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	OBSScene scene = selectedSceneItem->GetScene();

	obs_source_t* source = obs_scene_get_source(scene);
	obs_data_t* scene_data = obs_source_get_settings(source);

	bool refreshSceneList = false;
	int favorite_scene = obs_data_get_int(scene_data, "favorite_scene");
	if (1 == favorite_scene) {
		obs_data_set_int(scene_data, "favorite_scene", 0);
		refreshSceneList = true;
	}
	else {
		const int nMaxFavoriteSceneSize = SCENE_CONTEXT.GetFavoriteSceneMaxCount();
		int nFavoriteSceneCount = SCENE_CONTEXT.GetFavoriteSceneCount();
		if (nFavoriteSceneCount == nMaxFavoriteSceneSize) {
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
				"", QTStr("Basic.Scene.FavoriteInfo"), false, true);
		}
		else {
			obs_data_set_int(scene_data, "favorite_scene", 1);
			refreshSceneList = true;
		}
	}
	obs_data_release(scene_data);

	selectedSceneItem->SetFavoriteSceneButton(0 == favorite_scene ? true : false);

	if(refreshSceneList)
		MAINFRAME->RefreshSceneUI();
}

void AFQSceneListView::_qslotCopyFilters()
{
	OBSSource source = OBSSource(obs_scene_get_source(SCENE_CONTEXT.GetCurrentScene()));
	SCENE_CONTEXT.m_obsCopyFiltersSource = obs_source_get_weak_source(source);
}

void AFQSceneListView::_qslotPasteFilters()
{
	OBSSourceAutoRelease source = obs_weak_source_get_source(SCENE_CONTEXT.m_obsCopyFiltersSource);
	OBSSource dstSource = OBSSource(obs_scene_get_source(SCENE_CONTEXT.GetCurrentScene()));

	if (source == dstSource)
		return;

	obs_source_copy_filters(dstSource, source);
}

void AFQSceneListView::_qslotScreenshotScene()
{
	MAIN_SCENESOURCE->Screenshot(SCENE_CONTEXT.GetCurrentSceneSource());
}

void AFQSceneListView::_qslotMoveSceneUp()
{
	AFQSceneListItem* selectedSceneItem = SCENE_CONTEXT.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	const int index = selectedSceneItem->GetSceneIndex();
	if (index == 0)
		return;

	emit qsignalSwapItem(index, index - 1);
}

void AFQSceneListView::_qslotMoveSceneDown()
{
	AFQSceneListItem* selectedSceneItem = SCENE_CONTEXT.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	const int index = selectedSceneItem->GetSceneIndex();
	if (index == SCENE_CONTEXT.GetSceneItemSize())
		return;

	emit qsignalSwapItem(index, index + 1);
}

void AFQSceneListView::_qslotMoveSceneToTop()
{
	AFQSceneListItem* selectedSceneItem = SCENE_CONTEXT.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	const int index = selectedSceneItem->GetSceneIndex();
	if (index == 0)
		return;

	emit qsignalSwapItem(index, 0);
}

void AFQSceneListView::_qslotMoveSceneToBottom()
{
	AFQSceneListItem* selectedSceneItem = SCENE_CONTEXT.GetCurSelectedSceneItem();
	if (!selectedSceneItem)
		return;

	const int index = selectedSceneItem->GetSceneIndex();
	const int dest = SCENE_CONTEXT.GetSceneItemSize() - 1;
	if (index == dest)
		return;

	emit qsignalSwapItem(index, dest);
}

void AFQSceneListView::_qslotShowSceneFilters()
{
	OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
	OBSSource source = obs_scene_get_source(scene);

	MAINFRAME->CreateFiltersWindow(source);
}