#pragma once

#include <obs.hpp>
#include <QWidget>
#include <vector>
#include <memory>
#include "UIComponent/CRoundedDialogBase.h"

class AFQAdvAudioCtrl;

namespace Ui
{
	class AFQAudioAdvSettingDialog;
}

class AFQAudioAdvSettingDialog : public AFQRoundedDialogBase
{
#pragma region QT Field
	Q_OBJECT

private slots:
	void qslotSourceAdded(OBSSource source);
	void qslotSourceRemoved(OBSSource source);

	void qslotOnUsePercentToggled(bool checked);
	void qslotOnActiveOnlyToggled(bool checked);

	void qslotCloseButtonClicked();
#pragma endregion QT Field

#pragma region class initializer, destructor
public:
	explicit AFQAudioAdvSettingDialog(QWidget* parent);
	~AFQAudioAdvSettingDialog();
#pragma endregion class initializer, destructor

#pragma region public func
public:
	void SetShowInactive(bool showInactive);
#pragma endregion public func

#pragma region private func
private:
	void _ChangeLanguage();

	inline void _AddAudioSource(obs_source_t* source);
	static bool _EnumSources(void* param, obs_source_t* source);
	static void _OBSSourceAdded(void* param, calldata_t* calldata);
	static void _OBSSourceRemoved(void* param, calldata_t* calldata);
#pragma region private func

#pragma region private member var
private:
	OBSSignal m_sourceAddedSignal;
	OBSSignal m_sourceRemovedSignal;

	bool m_bShowInactive;
	bool m_bShowVisible;

	std::vector<AFQAdvAudioCtrl*> m_vControls;
	Ui::AFQAudioAdvSettingDialog* ui;
#pragma endregion private member var
};