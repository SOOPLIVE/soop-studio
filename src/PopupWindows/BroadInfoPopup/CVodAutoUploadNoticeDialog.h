#pragma once
#include "UIComponent/CTopBaseWindow.h"
#include "Application/CApplication.h"

namespace Ui {
	class AFQVodAutoUploadNoticeDialog;
}

class AFQVodAutoUploadNoticeDialog : public AFTTopBaseDialog
{
	Q_OBJECT

#pragma region QT Field, CTOR/DTOR
public:
	explicit AFQVodAutoUploadNoticeDialog(QWidget* parent = nullptr);
	~AFQVodAutoUploadNoticeDialog();

private slots:
	void _qslotClickedButtonBox(QAbstractButton* button);
#pragma endregion QT Field, CTOR/DTOR

#pragma region private func
private:
	void _Init();
#pragma endregion private func

#pragma region private member var
private:
	Ui::AFQVodAutoUploadNoticeDialog* ui;
#pragma endregion private member var
};