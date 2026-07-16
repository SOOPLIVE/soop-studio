#pragma once

#include <obs.hpp>
#include <QWidget>
#include <vector>
#include <memory>
#include "UIComponent/CTopBaseWindow.h"

// [window-basic-adv-audio.hpp]
class AFQAdvAudioCtrl;

namespace Ui
{
	class AFQAudioAdvSettingDialog;
}

class AFQAudioAdvSettingDialog : public AFTTopBaseDialog
{
	Q_OBJECT

private:
	std::vector<OBSSignal> sigs;

	bool showInactive = false;
	bool showVisible = false;

	std::vector<AFQAdvAudioCtrl*> controls;

	inline void AddAudioSource(obs_source_t* source);
	static bool EnumSources(void* param, obs_source_t* source);
	static void OBSSourceAdded(void* param, calldata_t* calldata);
	static void OBSSourceRemoved(void* param, calldata_t* calldata);
	static void OBSSourceActivated(void* param, calldata_t* calldata);

	std::unique_ptr<Ui::AFQAudioAdvSettingDialog> ui;

private slots:
	void SourceAdded(OBSSource source);
	void SourceRemoved(OBSSource source);

	void OnUsePercentToggled(bool checked);
	void OnActiveOnlyToggled(bool checked);

	void CloseButtonClicked();

public:
	explicit AFQAudioAdvSettingDialog(QWidget* parent);
	~AFQAudioAdvSettingDialog();
	void SetShowInactive(bool showInactive);
	void SetIconsVisible(bool visible);

};