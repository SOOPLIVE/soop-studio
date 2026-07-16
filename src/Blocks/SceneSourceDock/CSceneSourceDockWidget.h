#pragma once

#include <QMap>
#include <QPointer>
#include <QSpacerItem>
#include <QListWidget>

#include "obs.hpp"

#include "CoreModel/Scene/CSceneContext.h"

#include "CSceneListItem.h"

class AFQSceneListItem;

namespace Ui
{
class AFSceneSourceWidget;
}

class AFSceneSourceWidget final : public QWidget
{
#pragma region QT Field
	Q_OBJECT
signals:
	void qsignalAddScene();
	void qsignalAddSource();
	void qsignalSceneDoubleClickedTriggered();

public slots:

	void qslotAddSourceTrigger();
	void qslotRemoveSourceTrigger();
	void qslotMoveUpSourceTrigger();
	void qslotMoveDownSourceTrigger();
	void qslotMoveToTopSourceTrigger();
	void qslotMoveToBottomSourceTrigger();
	void qslotShowPropsTrigger();
	void qslotFitScreenSizeSourceTrigger();
	void qslotRestoreSourceTrigger();

private slots:
	void qslotClickedSceneItem();
	void qslotDoubleClickedSceneItem();
	void qslotRenameSceneItem();
	void qslotDeleteSceneItem();
	void qslotHoverSceneItem(OBSScene scene);
	void qslotSwapItem(int from, int dest);
	void qslotAddSceneButtonClicked();

	void AddSceneItem(OBSSceneItem item);
	void ReorderSources(OBSScene scene);
	void RefreshSources(OBSScene scene);

protected:
	virtual void resizeEvent(QResizeEvent* event) override;

#pragma endregion QT Field


#pragma region class initializer, destructor
public:
	explicit AFSceneSourceWidget(QWidget* parent = nullptr);
	~AFSceneSourceWidget();
#pragma endregion class initializer, destructor


#pragma region public func
public:

	QWidget* GetSceneListFrame();
	QWidget* GetSourceListView();

	void AddScene(OBSSource scene);
	void RemoveScene(OBSSource scene);
	void SetCurrentScene(OBSSource scene_source, bool force = false);
	void RefreshSceneItem();

	OBSSceneItem		GetCurrentSceneItem(int idx = -1);
	int					GetCurrentTopSelectedSceneItemIdx();

	void SourceToolBarButtonSetEnable();

#pragma region _SOOP_BREAKTIME
	void SetBreaktime(bool enable, QString name);
#pragma endregion

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
    Ui::AFSceneSourceWidget* ui;
    
	QPushButton* m_sceneAddButton = nullptr;

	bool m_isScrollBar = false;

	int m_dockWideMode = 0;

#pragma endregion private member var
};
