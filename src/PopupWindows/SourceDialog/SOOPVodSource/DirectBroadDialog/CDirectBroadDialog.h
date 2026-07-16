#pragma once

#include "obs.hpp"

#include <vector>
#include <string>

#include "UIComponent/CTopBaseWindow.h"

class AFQDirectBroadItem;

namespace Ui {
	class AFQDirectBroadDialog;
}

class AFQDirectBroadDialog : public AFTTopBaseDialog
{
	Q_OBJECT
public:
	AFQDirectBroadDialog(QWidget* parent, obs_source_t* source);
	~AFQDirectBroadDialog();

private slots:
	void _qslotCloseButtonClicked();
	void _qslotPrevDateButtonClicked();
	void _qslotNextDateButtonClicked();
	void _qslotCategoryAllButtonClicked();
	void _qslotCategorySportButtonClicked();
	void _qslotCategoryESportButtonClicked();
	void _qslotCategoryOfficialBroadButtonClicked();
	void _qslotRefreshDirectBroadListButtonClicked();

public slots:
	void qslotRefreshDirectBroadList();
	void qslotResponseDirectBroadOnetimeUrl(int requestIdx, QString url);
	void qslotRequestOneTimeUrl(int idx);
	void qslotRequestStopDirectBroad(int idx);

protected:
	void showEvent(QShowEvent* event);

private:
	void _SetDirectBroadList(QString date, int category = -1);
	static void _SourceRemoved(void* data, calldata_t* params);

private:
	Ui::AFQDirectBroadDialog* ui;

	QString m_selectDate;

	std::vector<AFQDirectBroadItem*> m_directBroadItems;
	OBSSignal removeSignal;
};
