#pragma once
#include "UIComponent/CTopBaseWindow.h"
#include "Application/CApplication.h"


namespace Ui {
	class AFQSoopBroadcastNoticeDialog;
}

class AFQSoopBroadcastNoticeDialog : public AFTTopBaseDialog
{
	Q_OBJECT

#pragma region QT Field, CTOR/DTOR
public:
	explicit AFQSoopBroadcastNoticeDialog(QWidget* parent = nullptr);
	~AFQSoopBroadcastNoticeDialog();

private slots:
	void _qslotClickedResponsibilityCheckBox(bool checked);
	void _qslotClickedOkButtonBox();
#pragma endregion QT Field, CTOR/DTOR

#pragma region protected func
protected:
	void showEvent(QShowEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
	void _Init();
#pragma endregion private func

#pragma region private member var
private:
	Ui::AFQSoopBroadcastNoticeDialog* ui;
#pragma endregion private member var
};