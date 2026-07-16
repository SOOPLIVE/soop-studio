#include "CAudioAdvSettingWidget.h"
#include "Application/CApplication.h"
#include "UIComponent/CItemWidgetHelper.h"
#include "CAudioAdvControl.h"
#include "ui_audio-adv-setting-area.h"

#include "MainFrame/CMainFrame.h"

AFQAudioAdvSettingDialog::AFQAudioAdvSettingDialog(QWidget* parent)
	: AFTTopBaseDialog(parent),
	ui(new Ui::AFQAudioAdvSettingDialog),
	showInactive(false)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	signal_handler_t* sh = obs_get_signal_handler();
	sigs.emplace_back(sh, "source_audio_activate", OBSSourceAdded, this);
	sigs.emplace_back(sh, "source_audio_deactivate", OBSSourceRemoved, this);
	sigs.emplace_back(sh, "source_activate", OBSSourceActivated, this);
	sigs.emplace_back(sh, "source_deactivate", OBSSourceRemoved, this);
    
#ifdef __APPLE__
    setWindowTitle(QTStr("Basic.AdvAudio"));
    ui->titleFrame->hide();
#endif
	
	VolumeType volType = (VolumeType)config_get_int(USERCONFIG, "BasicWindow", "AdvAudioVolumeType");

	if (volType == VolumeType::Percent)
		ui->usePercent->setChecked(true);

	connect(ui->usePercent, &QCheckBox::clicked, 
		this, &AFQAudioAdvSettingDialog::OnUsePercentToggled);
	connect(ui->activeOnly, &QCheckBox::clicked, 
		this, &AFQAudioAdvSettingDialog::OnActiveOnlyToggled);
	connect(ui->closeButton, &QPushButton::clicked, 
		this, &AFQAudioAdvSettingDialog::CloseButtonClicked);

	installEventFilter(CreateShortcutFilter());

	/* enum user scene/sources */
	obs_enum_sources(EnumSources, this);
}

AFQAudioAdvSettingDialog::~AFQAudioAdvSettingDialog()
{
	for(auto control : controls)
		delete control;

	MAINFRAME->qslotSaveProject();
}

inline void AFQAudioAdvSettingDialog::AddAudioSource(obs_source_t* source)
{
	for(auto control : controls) {
		if (control->GetSource() == source)
			return;
	}

	AFQAdvAudioCtrl* control = new AFQAdvAudioCtrl(ui->mainLayout, source);

#pragma region _SOOP_BREAKTIME
	auto channel = obs_get_output_source_channel(source);
	if(channel == 7) // BreakTime BGM
		controls.insert(controls.begin(), control); // Must Front Add
	else
		InsertQObjectByName(controls, control); // Sort Control  
#pragma endregion

	for (auto control : controls)
		control->ShowAudioControl(ui->mainLayout);
}

bool AFQAudioAdvSettingDialog::EnumSources(void* param, obs_source_t* source)
{
	AFQAudioAdvSettingDialog* dialog = reinterpret_cast<AFQAudioAdvSettingDialog*>(param);
	uint32_t flags = obs_source_get_output_flags(source);

	if ((flags & OBS_SOURCE_AUDIO) != 0 &&
		(dialog->showInactive || (obs_source_active(source) && obs_source_audio_active(source))))
		dialog->AddAudioSource(source);

	return true;
}

void AFQAudioAdvSettingDialog::OBSSourceAdded(void* param, calldata_t* calldata)
{
	OBSSource source((obs_source_t*)calldata_ptr(calldata, "source"));

	QMetaObject::invokeMethod(reinterpret_cast<AFQAudioAdvSettingDialog*>(param),
							  "SourceAdded", Q_ARG(OBSSource, source));
}

void AFQAudioAdvSettingDialog::OBSSourceRemoved(void* param, calldata_t* calldata)
{
	OBSSource source((obs_source_t*)calldata_ptr(calldata, "source"));

	QMetaObject::invokeMethod(reinterpret_cast<AFQAudioAdvSettingDialog*>(param),
							  "SourceRemoved", Q_ARG(OBSSource, source));
}

void AFQAudioAdvSettingDialog::OBSSourceActivated(void* param, calldata_t* calldata)
{
	OBSSource source((obs_source_t*)calldata_ptr(calldata, "source"));

	if(obs_source_audio_active(source))
		QMetaObject::invokeMethod(reinterpret_cast<AFQAudioAdvSettingDialog*>(param),
							      "SourceAdded", Q_ARG(OBSSource, source));
}

void AFQAudioAdvSettingDialog::SourceAdded(OBSSource source)
{
	uint32_t flags = obs_source_get_output_flags(source);

	if ((flags & OBS_SOURCE_AUDIO) == 0)
		return;

	AddAudioSource(source);
}

void AFQAudioAdvSettingDialog::SourceRemoved(OBSSource source)
{
	uint32_t flags = obs_source_get_output_flags(source);

	if ((flags & OBS_SOURCE_AUDIO) == 0)
		return;

	for (size_t i = 0; i < controls.size(); i++) {
		if (controls[i]->GetSource() == source) {
			delete controls[i];
			controls.erase(controls.begin() + i);
			break;
		}
	}
}

void AFQAudioAdvSettingDialog::OnUsePercentToggled(bool checked)
{
	VolumeType type;

	if (checked)
		type = VolumeType::Percent;
	else
		type = VolumeType::dB;

	for(auto contrl : controls)
		contrl->SetVolumeWidget(type);

	config_set_int(USERCONFIG, "BasicWindow", "AdvAudioVolumeType", (int)type);
}

void AFQAudioAdvSettingDialog::OnActiveOnlyToggled(bool checked)
{
	SetShowInactive(!checked);
}

void AFQAudioAdvSettingDialog::CloseButtonClicked()
{
	close();
}

void AFQAudioAdvSettingDialog::SetShowInactive(bool show)
{
	if (showInactive == show)
		return;

	showInactive = show;

	sigs.clear();

	signal_handler_t* sh = obs_get_signal_handler();
	if (showInactive) {
		sigs.emplace_back(sh, "source_create", OBSSourceAdded, this);
		sigs.emplace_back(sh, "source_remove", OBSSourceRemoved, this);

		obs_enum_sources(EnumSources, this);

		//SetIconsVisible(showInactive);
	}
	else {
		sigs.emplace_back(sh, "source_audio_activate", OBSSourceAdded, this);
		sigs.emplace_back(sh, "source_audio_deactivate", OBSSourceRemoved, this);
		sigs.emplace_back(sh, "source_activate", OBSSourceActivated, this);
		sigs.emplace_back(sh, "source_deactivate", OBSSourceRemoved, this);

		for (size_t i = 0; i < controls.size(); i++) {
			const auto source = controls[i]->GetSource();
			if (!obs_source_active(source)) {
				delete controls[i];
				controls.erase(controls.begin() + i);
				i--;
			}
		}
	}
}

void AFQAudioAdvSettingDialog::SetIconsVisible(bool visible)
{
	showVisible = visible;

	QLayoutItem* item = ui->mainLayout->itemAtPosition(0, 0);
	QLabel* headerLabel = qobject_cast<QLabel*>(item->widget());
	visible ? headerLabel->show() : headerLabel->hide();

	for(size_t i = 0; i < controls.size(); i++) {
		controls[i]->SetIconVisible(visible);
	}
}
