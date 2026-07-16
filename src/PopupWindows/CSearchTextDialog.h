#pragma once

#include <QDialog>

#include "UIComponent/CTopBaseWindow.h"

class AFQSearchDialog;

namespace Ui {
	class AFQSearchDialog;
}

class AFQSearchDialog : public AFTTopBaseDialog {
	Q_OBJECT

public:
	explicit AFQSearchDialog(QWidget *parent = 0);
	~AFQSearchDialog();

protected:
	void closeEvent(QCloseEvent* event) override;
	void reject() override;

signals:
	void qsignalSearchRequested(const QString& text, bool matchCase, bool forward, bool findNext);
	void qsignalStopSearchText(bool clearSelection);

private slots:
	void qslotSearchNextClicked();
	void qslotCloseButtonClicked();
	void qslotResetSearchText();

private:
	Ui::AFQSearchDialog* ui;
	bool m_firstSearch;
};
