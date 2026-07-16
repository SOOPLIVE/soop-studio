#pragma once
#include "UIComponent/CTopBaseWindow.h"
#include "Application/CApplication.h"

#define AGE_RESTRICTION_POLICY_DATE_CHECK "HideAgeRestrictionPolicyDate"

namespace Ui {
	class AFQAgeRestrictionPolicyDialog;
}

class AFQAgeRestrictionPolicyDialog : public AFTTopBaseDialog
{
	Q_OBJECT

#pragma region QT Field, CTOR/DTOR
public:
	explicit AFQAgeRestrictionPolicyDialog(QWidget* parent = nullptr);
	~AFQAgeRestrictionPolicyDialog();

private slots:
	void _qslotClickedButtonBox(QAbstractButton* button);
#pragma endregion QT Field, CTOR/DTOR

#pragma region private func
private:
	void _Init();
#pragma endregion private func

#pragma region private member var
private:
	Ui::AFQAgeRestrictionPolicyDialog* ui;
#pragma endregion private member var
};