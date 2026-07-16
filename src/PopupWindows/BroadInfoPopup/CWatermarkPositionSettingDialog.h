#pragma once
#include "UIComponent/CTopBaseWindow.h"
#include <QAbstractButton>

namespace Ui {
	class AFQWatermarkPositionSettingDialog;
}

class AFQWatermarkPositionSettingDialog : public AFTTopBaseDialog
{
	Q_OBJECT

#pragma region class initializer, destructor
public:
	explicit AFQWatermarkPositionSettingDialog(QWidget* parent, const std::vector<QString>& watermarkList, int watermarkPos);
	~AFQWatermarkPositionSettingDialog();

signals:
	void qsignalWatermarkPositionChanged(int watermarkPos);

private slots:
	void _qslotClickedSaveButton();
	void _qslotWatermarkPositionChanged(int index);
	void _qslotWatermarkInitSetting();
#pragma endregion class initializer, destructor

#pragma region private func
private:
	void _Init(const std::vector<QString>& watermarkList);
	void _SetWatermarkPosition(int position);
#pragma endregion private func

#pragma region private member var
private:
	Ui::AFQWatermarkPositionSettingDialog* ui;
	int m_watermarkPos = 0;
#pragma endregion private member var
};