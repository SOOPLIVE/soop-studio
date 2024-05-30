#pragma once

#include <QMap>
#include <QPointer>
#include <QSpacerItem>
#include <QListWidget>

#include "obs.hpp"

#include "CoreModel/Scene/CScene.h"

#include "Blocks/CBaseDockWidget.h"

#include "CSceneListItem.h"

class AFQSceneListItem;

namespace Ui
{
class AFSceneSourceDockWidget;
}

class AFSceneSourceDockWidget final : public AFQBaseDockWidget
{
#pragma region QT Field
	Q_OBJECT
signals:
	void qSignalAddScene();
	void qSignalAddSource();
	void qSignalSceneDoubleClickedTriggered();

public slots:

	void qSlotAddSourceTrigger();
	void qSlotRemoveSourceTrigger();
	void qSlotMoveUpSourceTrigger();
	void qSlotMoveDownSourceTrigger();
	void qSlotMoveToTopSourceTrigger();
	void qSlotMoveToBottomSourceTrigger();
	void qSlotShowPropsTrigger();

private slots:
	void qSlotClickedSceneItem();
	void qSlotRenameSceneItem();
	void qSlotDeleteSceneItem();
	void qSlotHoverSceneItem(OBSScene scene);
	void qSlotSwapItem(int from, int dest);
	void qSlotCheckSourceClicked(bool clicked);
	void qSlotAddSceneButtonClicked();

	void AddSceneItem(OBSSceneItem item);
	void ReorderSources(OBSScene scene);
	void RefreshSources(OBSScene scene);

#pragma endregion QT Field


#pragma region class initializer, destructor
public:
	explicit AFSceneSourceDockWidget(QWidget* parent = nullptr);
	~AFSceneSourceDockWidget();
#pragma endregion class initializer, destructor


#pragma region public func
public:

	void AddScene(OBSSource scene);
	void RemoveScene(OBSSource scene);
	void SetCurrentScene(OBSSource scene_source, bool force = false);
	void RefreshSceneItem();
	void RegisterShortCut(QAction* removeSourceAction);

	OBSSceneItem		GetCurrentSceneItem(int idx = -1);



#pragma endregion public func

#pragma region private func
private:
	/* OBS Callbacks */
	static void SceneReordered(void* data, calldata_t* params);
	static void SceneRefreshed(void* data, calldata_t* params);
	static void SceneItemAdded(void* data, calldata_t* params);

	void _InitSceneSourceDockUI();
	void _InitSceneSourceDockSignalSlot();

	void _MoveSceneItem(enum obs_order_movement movement, const QString& action_name);

#pragma endregion private func


#pragma region private member var
private:
    Ui::AFSceneSourceDockWidget* ui;
    
	QPushButton* m_sceneAddButton = nullptr;

	bool m_bIsScrollBar = false;

#pragma endregion private member var
};
