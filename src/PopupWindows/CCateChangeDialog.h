#pragma once

#include <list>
#include <QFrame>
#include <QPushButton>
#include <QButtonGroup>

#include "UIComponent/CTopBaseWindow.h"

namespace Ui {
	class AFQCateChangeDialog;
}

class AFQCateChangeDialog : public AFTTopBaseDialog
{
	Q_OBJECT

public:
	AFQCateChangeDialog(QWidget* parent, const char* id);
	~AFQCateChangeDialog();

	void AddAllowedCategoryInfo(std::list<int>& m_categoryNums);
	void AddAllowedAnimeAdultCategoryInfo(int categoryNum);
	int  GetSelectedCategory() { return m_allowedCategory; };

private:
	void MoveToParentCenter();

private slots:
	void qslotResponseCategorysAPI(const QByteArray& responseData);
	void qslotResponseAdultCategorysAPI(const QByteArray& responseData);
	void qslotCloseButtonClicked();
	void qslotAcceptButtonClicked();
	void qslotShowAdultInfoButtonClicked();

private:
	Ui::AFQCateChangeDialog* ui;

	std::string m_sourceId;
	QButtonGroup* m_pButtonGroup = nullptr;

	std::list<int> m_allowedCateLists;
	int m_allowedAdultCateNum = 0;

	int  m_allowedCategory = 0;
};
