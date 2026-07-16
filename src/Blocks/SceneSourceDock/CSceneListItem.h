#pragma once

#include <memory>

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHoverEvent>
#include <QLayout>
#include <QTimer>
#include <QPointer>

#include "obs.hpp"

#include "Common/MathMiscUtils.h"

#include "UIComponent/CElidedSlideLabel.h"
#include "UIComponent/CQtDisplay.h"


#define SCENE_ITEM_DRAG_MIME ("SCENE_ITEM_DRAG_MIME")

template<typename OBSRef> struct SignalContainer {
	OBSRef ref;
	std::vector<std::shared_ptr<OBSSignal>> handlers;
};

class AFQSceneListView;
class AFQSceneListPreview;

class AFQSceneListItem : public QFrame
{
	Q_OBJECT

public:
	AFQSceneListItem(QWidget* parent, 
					 QFrame* sceneListFrame, 
					 OBSScene scene, 
					 QString name, 
					 const SignalContainer<OBSScene>& signalConainter);
	~AFQSceneListItem();

signals:
	void qsignalClickedSceneItem();
	void qsignalDoubleClickedSceneItem();
	void qsignalRenameSceneItem();
	void qsignalDeleteSceneItem();
	void qsignalShowRenameSceneUI();
	void qsignalHoverSceneItem(OBSScene scene);
	void qsignalHoverButton(QString id);
	void qsignalLeaveButton();

private slots:
	void qslotRenameSceneItem();
	void qslotFavoriteSceneItem(bool checked);
	void qslotSetHoverSceneItemUI(bool hoverd);
	void qslotTimerHoverPreview();

public:
	void SelectScene(bool select);
	void SetSceneIndexLabelNum(int index);
	void SetFavoriteSceneButton(bool favorite);

	OBSScene GetScene();
	const char* GetSceneName();
	int	GetSceneIndex() { return m_sceneIndex; }

	void ShowRenameSceneUI();

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	void _CreateSceneItemUI(QString scene_name);
	void _StartDrag(QPoint pos);
	void _ChangeSceneName(bool change);

	void _SetHoverStyleSheet(bool hover);
	void _ShowPreview(bool on);

	OBSSource _GetSceneSource();
	static void _SceneListPreviewRender(void* data, uint32_t cx, uint32_t cy);

private:
	QLabel*	m_pLabelSceneIndex = nullptr;
	AFQElidedSlideLabel* m_pLabelSceneName = nullptr;
	QLineEdit* m_pTextEdit = nullptr;
	QPushButton* m_pFavoriteSceneButton = nullptr;

	QPointer<AFQSceneListPreview> m_sceneListPreviewWidget = nullptr;

	//
	OBSScene m_obsScene;
	SignalContainer<OBSScene> m_signalContainer;

	int m_sceneIndex;
	QPoint m_startPos;

	//
	bool m_hovered = false;
	bool m_selected = false;
	bool m_editSceneName = false;
	bool m_changingName = false;

	QTimer* m_pTimerHoverPreview = nullptr;
};

class AFQSceneListPreview : public QWidget
{
	Q_OBJECT

public:
	AFQSceneListPreview(QWidget* parent, OBSSource source);
	~AFQSceneListPreview();

	OBSSource GetSceneSource();
	static void SceneListPreviewRender(void* data, uint32_t cx, uint32_t cy);

private:
	OBSSource m_sceneSource;
	OBSWeakSourceAutoRelease m_weakSceneSource;

	QPointer<AFQTDisplay> m_previewScene;
};