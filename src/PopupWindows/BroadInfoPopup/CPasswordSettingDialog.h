#pragma once
#include "UIComponent/CTopBaseWindow.h"
#include <QAbstractButton>

namespace Ui {
	class AFQPasswordSettingDialog;
}

class AFQPasswordSettingDialog : public AFTTopBaseDialog
{
	Q_OBJECT

	enum class PasswordCheck {
		Pass,
		Empty,                      // No password entered
		TooShort,                   // Password is 6 characters or fewer
		MustMixLettersAndNumbers,   // Must contain at least one letter and one number
		RepeatedChars,              // Same character cannot be repeated 3 or more times (e.g. 111, 222, ###)
		SequentialChars,            // Sequential letters or numbers cannot be used 3 or more times (e.g. abc, 123)
		InvalidCharacters           // Only letters, numbers, and special characters are allowed; emojis are not allowed
	};

#pragma region class initializer, destructor
public:
	explicit AFQPasswordSettingDialog(QWidget* parent = nullptr, bool usePassword = false, const QString& password = "");
	~AFQPasswordSettingDialog();

signals:
	void qsignalClickedUsePassword(bool usePassword, const QString& password);

private slots:
	void _qslotClickedSaveButton();
	void _qslotToggledUsePasswordButton();
	void _qslotSetWarningStyle(bool isWarningState);
	void _qslotAsciiError();
#pragma endregion class initializer, destructor

#pragma region private func
private:
	void _Init(bool usePassword, const QString& password);

	PasswordCheck _CheckPassword(const QString& password);
	void _SetWarningText(PasswordCheck checkResult);
#pragma endregion private func

#pragma region private member var
private:
	Ui::AFQPasswordSettingDialog* ui;

	bool m_warningState = false;
#pragma endregion private member var
};
