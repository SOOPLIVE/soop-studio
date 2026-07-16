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
	Q_OBJECT

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
	void _qslotDuplicateSelectedScene();
	void _qslotRegisterFavoriteScene();
	void _qslotCopyFilters();
	void _qslotPasteFilters();
	void _qslotScreenshotScene();
	void _qslotMoveSceneUp();
	void _qslotMoveSceneDown();
	void _qslotMoveSceneToTop();
	void _qslotMoveSceneToBottom();
	void _qslotShowSceneFilters();

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