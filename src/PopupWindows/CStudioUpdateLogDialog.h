#pragma once

#include <QFrame>
#include <QPushButton>

#include "UIComponent/CTopBaseWindow.h"
#include "MainFrame/CMainFrame.h"

class QCefWidget;

namespace Ui {
	class AFQStudioUpdateLogDialog;
}

class AFQStudioUpdateLogDialog : public AFTTopBaseDialog
{
	Q_OBJECT
public:
	AFQStudioUpdateLogDialog(QWidget* parent = nullptr);
	~AFQStudioUpdateLogDialog();

	enum Mode { UpdateLog = 0, FAQ = 1 };
	void setMode(int mode);   // 0 = UpdateLog, 1 = FAQ

private slots:
	void qslotRefreshButtonClicked();

private:
	Ui::AFQStudioUpdateLogDialog* ui;
		
	QCefWidget* m_cefWidget = nullptr;
	int m_mode = UpdateLog;

};