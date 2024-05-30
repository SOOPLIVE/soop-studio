#pragma once

#include <QFrame>
#include <QScrollArea>
#include <QBoxLayout>

#include "obs.hpp"

#include "CSceneListItem.h"
#include "UIComponent/CCustomMenu.h"

class AFQSceneListItem;

class AFQSceneListView : public QFrame
{
	Q_OBJECT;

#pragma region class initializer, destructor
public:
	AFQSceneListView(QWidget* parent);
	~AFQSceneListView();
#pragma endregion class initializer, destructor

protected:
	void contextMenuEvent(QContextMenuEvent* event) override;

signals:
	void qsignalSwapItem(int from, int dest);

private slots:
	void _qSlotDuplicateSelectedScene();
	void _qSlotRenameScene();
	void _qSlotRemoveScene();
	void _qSlotCopyFilters();
	void _qSlotPasteFilters();
	void _qSlotScreenshotScene();
	void _qSlotMoveSceneUp();
	void _qSlotMoveSceneDown();
	void _qSlotMoveSceneToTop();
	void _qSlotMoveSceneToBottom();
	void _qSlotShowSceneFilters();

#pragma region private member func
private:
	void	_CreateSceneMenuPopup();
	AFQCustomMenu*	_CreatePerSceneTransitionMenu();
#pragma endregion private member func


#pragma region public member func
public:

#pragma endregion public member func


#pragma region private member var
private:
	QPointer<AFQCustomMenu> m_perSceneTransitionMenu;

#pragma endregion private member var
};