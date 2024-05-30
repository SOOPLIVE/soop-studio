#pragma once

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHoverEvent>
#include <QLayout>
#include <QTimer>

#include "obs.hpp"

#include "UIComponent/CElidedSlideLabel.h"

#include "Utils/AFScreenshotObj.h"

#define SCENE_ITEM_DRAG_MIME ("SCENE_ITEM_DRAG_MIME")

template<typename OBSRef> struct SignalContainer {
	OBSRef ref;
	std::vector<std::shared_ptr<OBSSignal>> handlers;
};

class AFQSceneListView;

class AFQSceneListItem : public QFrame
{
	Q_OBJECT
#pragma region class initializer, destructor
public:
	AFQSceneListItem(QWidget* parent, 
					 QFrame* sceneListFrame, 
					 OBSScene scene, 
					 QString name, 
					 const SignalContainer<OBSScene>& signalConainter);
	~AFQSceneListItem();
#pragma endregion class initializer, destructor

signals:
	void qSignalClickedSceneItem();
	void qSignalDoubleClickedSceneItem();
	void qSignalRenameSceneItem();
	void qSignalDeleteSceneItem();
	void qSignalShowRenameSceneUI();
	void qSignalHoverSceneItem(OBSScene scene);
	void qSignalHoverButton(QString id);
	void qSignalLeaveButton();

private slots:
	void qSlotRenameSceneItem();
	void qSlotSetHoverSceneItemUI(bool hoverd);
	void qSlotTimerScreenShot();
	void qSlotTimerHoverPreview();
	void qslotSetScreenShotPreview();

public:
	void SelectScene(bool select);
	void SetSceneIndexLabelNum(int index);

	OBSScene	GetScene();
	const char* GetSceneName();
	int			GetSceneIndex() { return m_nSceneIndex; }

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


#pragma region private member var
private:
	QLabel*	m_pLabelSceneIndex = nullptr;
	AFQElidedSlideLabel* m_pLabelSceneName = nullptr;
	QLineEdit* m_pTextEdit = nullptr;
	QPushButton* m_pSceneNameEditButton = nullptr;

	QLabel* m_pScreenshotScene = nullptr;

	//
	OBSScene m_obsScene;
	SignalContainer<OBSScene> m_signalContainer;

	int m_nSceneIndex;;
	QPoint	m_startPos;

	//
	bool m_bHovered = false;
	bool m_bSelected = false;
	bool m_bEditSceneName = false;
	bool m_bChangingName = false;

	QTimer* m_timerScreenShot = nullptr;
	QTimer* m_timerHoverPreview = nullptr;
	QPointer<AFQScreenShotObj>	m_pScreenshotObj;

#pragma endregion private member var

};