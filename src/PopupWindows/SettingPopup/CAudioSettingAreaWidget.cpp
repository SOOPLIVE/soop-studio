#include "CAudioSettingAreaWidget.h"
#include "UIComponent/CAudioEncoders.h"
#include "UIComponent/CSourceLabel.h"
#include "util/dstr.h"
#include "platform/platform.hpp"
#include "ui_audio-setting-area.h"
#include "../CoreModel/Encoder/CEncoder.inl"
#include "UIComponent/CMessageBox.h"
#include "UIComponent/CCustomSpinbox.h"

#define AUDIO_RESTART   &AFQAudioSettingAreaWidget::qslotAudioChangedRestart
#define AUDIO_CHANGED   &AFQAudioSettingAreaWidget::qslotAudioDataChanged

#pragma region class initializer, destructor
AFQAudioSettingAreaWidget::AFQAudioSettingAreaWidget(QWidget* parent) :
	ui(new Ui::AFQAudioSettingAreaWidget)
{
	ui->setupUi(this);
	
	_SetAudioSettingUi();
	_SetAudioSettingSignal();
	_ChangeLanguage();
}

AFQAudioSettingAreaWidget::~AFQAudioSettingAreaWidget()
{
	delete ui;
}
#pragma endregion class initializer, destructor

#pragma region public func
void AFQAudioSettingAreaWidget::LoadAudioSettings(bool reset)
{
	m_bLoading = true;

	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();

	uint32_t sampleRate;
	const char* speakers;
	const char* streamAudioEnc;
	int audioBitrate;
	double meterDecayRate;
	uint32_t peakMeterTypeIdx; 
	QString monDevName;
	QString monDevId;
	bool disableAudioDucking;
	bool enableLLAudioBuffering;
	const char* encoderType;

	sampleRate = config_get_uint(basicConfig, "Audio", "SampleRate");
	speakers = config_get_string(basicConfig, "Audio", "ChannelSetup");
	streamAudioEnc = config_get_string(basicConfig, "SimpleOutput", "StreamAudioEncoder");
	audioBitrate = config_get_uint(basicConfig, "SimpleOutput", "ABitrate");
	meterDecayRate = config_get_double(basicConfig, "Audio", "MeterDecayRate");
	peakMeterTypeIdx = config_get_uint(basicConfig, "Audio", "PeakMeterType");
	disableAudioDucking = config_get_bool(basicConfig, "Audio", "DisableAudioDucking");
	enableLLAudioBuffering = config_get_bool(basicConfig, "Audio", "LowLatencyAudioBuffering");
	encoderType = config_get_string(basicConfig, "AdvOut", "AudioEncoder");

	if (obs_audio_monitoring_available()) {
		monDevName = config_get_string(basicConfig, "Audio", "MonitoringDeviceName");
		monDevId = config_get_string(basicConfig, "Audio", "MonitoringDeviceId");
	}

	const char* strSampleRate;
	if (sampleRate == 48000)
		strSampleRate = "48 kHz";
	else
		strSampleRate = "44.1 kHz";

	int sampleRateIdx = ui->comboBox_SampleRate->findText(strSampleRate);
	if (sampleRateIdx != -1)
		ui->comboBox_SampleRate->setCurrentIndex(sampleRateIdx);

	if (strcmp(speakers, "Mono") == 0)
		ui->comboBox_ChannelSetup->setCurrentIndex(0);
	else if (strcmp(speakers, "2.1") == 0)
		ui->comboBox_ChannelSetup->setCurrentIndex(2);
	else if (strcmp(speakers, "4.0") == 0)
		ui->comboBox_ChannelSetup->setCurrentIndex(3);
	else if (strcmp(speakers, "4.1") == 0)
		ui->comboBox_ChannelSetup->setCurrentIndex(4);
	else if (strcmp(speakers, "5.1") == 0)
		ui->comboBox_ChannelSetup->setCurrentIndex(5);
	else if (strcmp(speakers, "7.1") == 0)
		ui->comboBox_ChannelSetup->setCurrentIndex(6);
	else
		ui->comboBox_ChannelSetup->setCurrentIndex(1);

	bool isOpus = strcmp(streamAudioEnc, "opus") == 0;
	audioBitrate = isOpus ? FindClosestAvailableSimpleOpusBitrate(audioBitrate)
							: FindClosestAvailableSimpleAACBitrate(audioBitrate);

	_PopulateSimpleBitrates(ui->comboBox_SimpleOutputABitrate, isOpus);

	if (!_IsSurround(speakers))
		_RestrictResetBitrates({ ui->comboBox_SimpleOutputABitrate }, 320);

	AFSettingUtils::SetComboByName(ui->comboBox_SimpleOutputABitrate, std::to_string(audioBitrate).c_str());

	_ResetEncoders();
	AFSettingUtils::SetComboByName(ui->comboBox_SimpleOutStrAEncoder, streamAudioEnc);
	qslotSimpleEncoderChanged();
	qslotUpdateSimpleRecordingEncoder();

	if (!AFSettingUtils::SetComboByValue(ui->comboBox_AdvOutAEncoder, encoderType))
		ui->comboBox_AdvOutAEncoder->setCurrentIndex(-1);

	if (meterDecayRate == VOLUME_METER_DECAY_MEDIUM)
		ui->comboBox_MeterDecayRate->setCurrentIndex(1);
	else if (meterDecayRate == VOLUME_METER_DECAY_SLOW)
		ui->comboBox_MeterDecayRate->setCurrentIndex(2);
	else
		ui->comboBox_MeterDecayRate->setCurrentIndex(0);

	ui->comboBox_PeakMeterType->setCurrentIndex(peakMeterTypeIdx);

	if (obs_audio_monitoring_available())
		_FillAudioMonitoringDevices();

	if (obs_audio_monitoring_available() && 
		!AFSettingUtils::SetComboByValue(ui->comboBox_MonitoringDevice, monDevId.toUtf8()))
		AFSettingUtils::SetInvalidValue(ui->comboBox_MonitoringDevice, monDevName.toUtf8(), monDevId.toUtf8());

#ifdef _WIN32
	ui->checkBox_DisableAudioDucking->setChecked(disableAudioDucking);
#endif
	ui->checkBox_LowLatencyBuffering->setChecked(enableLLAudioBuffering);

	_LoadAudioDevices();
	_LoadAudioSources();

	m_bLoading = false;

	const char* mode = config_get_string(basicConfig, "Output", "Mode");
	if (astrcmpi(mode, "Advanced") == 0)
		m_bIsAdvancedMode = true;
	else
		m_bIsAdvancedMode = false;
	
	if (reset) {
		if (m_bIsAdvancedMode)
			emit qsignalAdvancedModeClicked();
		else
			emit qsignalSimpleModeClicked();
	}

	ToggleOnStreaming(false);
	_SetSettingModeUi(m_bIsAdvancedMode);
}

void AFQAudioSettingAreaWidget::SaveAudioSettings()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	config_t* globalConfig = AFConfigManager::GetSingletonInstance().GetGlobal();

	QString sampleRateStr = ui->comboBox_SampleRate->currentText();
	int channelSetupIdx = ui->comboBox_ChannelSetup->currentIndex();

	if (!m_bAudioDataChanged)
		return;

	int sampleRate = 44100;
	if (sampleRateStr == "48 kHz")
		sampleRate = 48000;

	if (AFSettingUtils::WidgetChanged(ui->comboBox_SampleRate))
		config_set_uint(basicConfig, "Audio", "SampleRate", sampleRate);

	const char* channelSetup;
	switch (channelSetupIdx) {
	case 0:
		channelSetup = "Mono";
		break;
	case 1:
		channelSetup = "Stereo";
		break;
	case 2:
		channelSetup = "2.1";
		break;
	case 3:
		channelSetup = "4.0";
		break;
	case 4:
		channelSetup = "4.1";
		break;
	case 5:
		channelSetup = "5.1";
		break;
	case 6:
		channelSetup = "7.1";
		break;

	default:
		channelSetup = "Stereo";
		break;
	}

	if (AFSettingUtils::WidgetChanged(ui->comboBox_ChannelSetup))
		config_set_string(basicConfig, "Audio", "ChannelSetup", channelSetup);

	_SaveAudioBitrate();

	AFSettingUtils::SaveComboData(ui->comboBox_SimpleOutStrAEncoder, "SimpleOutput", "StreamAudioEncoder");
	AFSettingUtils::SaveComboData(ui->comboBox_AdvOutAEncoder, "AdvOut", "AudioEncoder");
	
	if (AFSettingUtils::WidgetChanged(ui->comboBox_MeterDecayRate)) {
		double meterDecayRate;
		switch (ui->comboBox_MeterDecayRate->currentIndex()) {
		case 0:
			meterDecayRate = VOLUME_METER_DECAY_FAST;
			break;
		case 1:
			meterDecayRate = VOLUME_METER_DECAY_MEDIUM;
			break;
		case 2:
			meterDecayRate = VOLUME_METER_DECAY_SLOW;
			break;
		default:
			meterDecayRate = VOLUME_METER_DECAY_FAST;
			break;
		}

		config_set_double(basicConfig, "Audio", "MeterDecayRate", meterDecayRate);

		App()->GetMainView()->GetMainWindow()->UpdateVolumeControlsDecayRate();
	}

	if (AFSettingUtils::WidgetChanged(ui->comboBox_PeakMeterType)) {
		uint32_t peakMeterTypeIdx = ui->comboBox_PeakMeterType->currentIndex();
		config_set_uint(basicConfig, "Audio", "PeakMeterType", peakMeterTypeIdx);

		App()->GetMainView()->GetMainWindow()->UpdateVolumeControlsPeakMeterType();
	}

	if (obs_audio_monitoring_available()) {
		AFSettingUtils::SaveCombo(ui->comboBox_MonitoringDevice, "Audio",
			"MonitoringDeviceName");
		AFSettingUtils::SaveComboData(ui->comboBox_MonitoringDevice, "Audio",
			"MonitoringDeviceId");
	}

	QString lastMonitoringDevice = config_get_string(basicConfig, "Audio", "MonitoringDeviceId");

	if (obs_audio_monitoring_available()) {
		QString newDevice =
			ui->comboBox_MonitoringDevice->currentData().toString();

		if (lastMonitoringDevice != newDevice) {
			obs_set_audio_monitoring_device(
					QT_TO_UTF8(ui->comboBox_MonitoringDevice->currentText()),
					QT_TO_UTF8(newDevice));

			blog(LOG_INFO,
				"Audio monitoring device:\n\tname: %s\n\tid: %s",
					QT_TO_UTF8(ui->comboBox_MonitoringDevice->currentText()),
					QT_TO_UTF8(newDevice));
		}
	}

#ifdef _WIN32
	if (AFSettingUtils::WidgetChanged(ui->checkBox_DisableAudioDucking)) {
		bool disable = ui->checkBox_DisableAudioDucking->isChecked();
		config_set_bool(globalConfig, "Audio", "DisableAudioDucking", disable);

		DisableAudioDucking(disable);
	}
#endif

	if (AFSettingUtils::WidgetChanged(ui->checkBox_LowLatencyBuffering)) {
		bool enableLLAudioBuffering = ui->checkBox_LowLatencyBuffering->isChecked();
		config_set_bool(globalConfig, "Audio", "LowLatencyAudioBuffering", enableLLAudioBuffering);
	}

	for (auto& audioSource : m_AudioSources) {
		auto source = OBSGetStrongRef(std::get<0>(audioSource));

		if (!source)
			continue;

		auto& ptmCB = std::get<1>(audioSource);
		auto& ptmSB = std::get<2>(audioSource);
		auto& pttCB = std::get<3>(audioSource);
		auto& pttSB = std::get<4>(audioSource);

		obs_source_enable_push_to_mute(source, ptmCB->isChecked());
		obs_source_set_push_to_mute_delay(source, ptmSB->value());

		obs_source_enable_push_to_talk(source, pttCB->isChecked());
		obs_source_set_push_to_talk_delay(source, pttSB->value());
	}

	auto UpdateAudioDevice = [this](bool input, QComboBox* combo,
		const char* name, int index) {
			audioUtil.ResetAudioDevice(
				input ? App()->InputAudioSource()
				: App()->OutputAudioSource(),
				QT_TO_UTF8(AFSettingUtils::GetComboData(combo)),
				AFLocaleTextManager::GetSingletonInstance().Str(name), index);
		};

	UpdateAudioDevice(false, ui->comboBox_DesktopAudioDevice1, "Basic.DesktopDevice1", 1);
	UpdateAudioDevice(false, ui->comboBox_DesktopAudioDevice2, "Basic.DesktopDevice2", 2);
	UpdateAudioDevice(true, ui->comboBox_AuxAudioDevice1, "Basic.AuxDevice1", 3);
	UpdateAudioDevice(true, ui->comboBox_AuxAudioDevice2, "Basic.AuxDevice2", 4);
	UpdateAudioDevice(true, ui->comboBox_AuxAudioDevice3, "Basic.AuxDevice3", 5);
	UpdateAudioDevice(true, ui->comboBox_AuxAudioDevice4, "Basic.AuxDevice4", 6);

	App()->GetMainView()->qslotSaveProject();
}

void AFQAudioSettingAreaWidget::ResetAudioSettings()
{
	m_bRestartNeeded = false;

	_ClearAudioSettingUi();
	LoadAudioSettings(true);
}

void AFQAudioSettingAreaWidget::ToggleOnStreaming(bool streaming)
{
	bool useVideo = obs_video_active() ? false : true;
	ui->frame_SettingMode->setEnabled(useVideo);
	ui->comboBox_SimpleOutStrAEncoder->setEnabled(useVideo);
	ui->comboBox_SampleRate->setEnabled(useVideo);
	ui->comboBox_ChannelSetup->setEnabled(useVideo);
	ui->comboBox_SimpleOutputABitrate->setEnabled(useVideo);
	ui->comboBox_AdvOutAEncoder->setEnabled(useVideo);
}

int AFQAudioSettingAreaWidget::AudioBitrate()
{
	return ui->comboBox_SimpleOutputABitrate->currentText().toInt();
}

QString AFQAudioSettingAreaWidget::GetAdvAudioEncoder()
{
	if (ui->comboBox_AdvOutAEncoder->currentIndex() < 0)
		return "";
	return ui->comboBox_AdvOutAEncoder->currentData().toString();
}

QString AFQAudioSettingAreaWidget::GetSimpleAudioEncoder()
{
	if (ui->comboBox_SimpleOutStrAEncoder->currentIndex() < 0)
		return "";
	return ui->comboBox_SimpleOutStrAEncoder->currentData().toString();
}

void AFQAudioSettingAreaWidget::ChangeSettingModeToSimple()
{
	if (m_bIsAdvancedMode)
	{
		m_bIsAdvancedMode = false;
		_SetSettingModeUi(m_bIsAdvancedMode);
	}
}

void AFQAudioSettingAreaWidget::ChangeSettingModeToAdvanced()
{
	if (!m_bIsAdvancedMode)
	{
		m_bIsAdvancedMode = true;
		_SetSettingModeUi(m_bIsAdvancedMode);
	}
}

QString AFQAudioSettingAreaWidget::CurrentAudioBitRate()
{
	return ui->comboBox_SimpleOutputABitrate->currentText();
}
#pragma endregion public func

#pragma region private func
void AFQAudioSettingAreaWidget::_ClearAudioSettingUi()
{
	ui->comboBox_SimpleOutStrAEncoder->clear();

	ui->comboBox_DesktopAudioDevice1->clear();
	ui->comboBox_DesktopAudioDevice2->clear();
	ui->comboBox_AuxAudioDevice1->clear();
	ui->comboBox_AuxAudioDevice2->clear();
	ui->comboBox_AuxAudioDevice3->clear();
	ui->comboBox_AuxAudioDevice4->clear();

	ui->comboBox_MonitoringDevice->clear();

	int nHotKeyRowCount = ui->formLayout_AudioHotkeys->rowCount();
	for (int i = nHotKeyRowCount - 1; i >= 0; i--)
	{
		QWidget* widgetToRemove = ui->formLayout_AudioHotkeys->itemAt(i, QFormLayout::FieldRole)->widget();
		ui->formLayout_AudioHotkeys->removeRow(i);
	}
}

void AFQAudioSettingAreaWidget::_ChangeLanguage()
{
	QList<QLabel*> labelList = findChildren<QLabel*>();
	QList<QCheckBox*> checkboxList = findChildren<QCheckBox*>();
	QList<QComboBox*> comboboxList = findChildren<QComboBox*>();

	foreach(QLabel * label, labelList)
	{
		label->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(label->text().toUtf8().constData())));
	}

	foreach(QCheckBox * checkbox, checkboxList)
	{
		checkbox->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(checkbox->text().toUtf8().constData())));
	}

	foreach(QComboBox* combobox, comboboxList)
	{
		int itemCount = combobox->count();
		for (int i = 0; i < itemCount; ++i) 
		{
			QString itemString = combobox->itemText(i);
			QRegularExpression rx("\\d+");

			if (rx.match(itemString).hasMatch())
				continue;

			combobox->setItemText(i, QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(itemString.toUtf8().constData())));
		}
	}
}

void AFQAudioSettingAreaWidget::_LoadAudioDevices()
{
	const char* input_id = App()->InputAudioSource();
	const char* output_id = App()->OutputAudioSource();

	obs_properties_t* input_props = obs_get_source_properties(input_id);
	obs_properties_t* output_props = obs_get_source_properties(output_id);

	if (input_props) {
		obs_property_t* inputs = obs_properties_get(input_props, "device_id");

		_LoadListValues(ui->comboBox_AuxAudioDevice1, inputs, 3);
		_LoadListValues(ui->comboBox_AuxAudioDevice2, inputs, 4);
		_LoadListValues(ui->comboBox_AuxAudioDevice3, inputs, 5);
		_LoadListValues(ui->comboBox_AuxAudioDevice4, inputs, 6);

		obs_properties_destroy(input_props);
	}

	if (output_props) {
		obs_property_t* outputs = obs_properties_get(output_props, "device_id");

		_LoadListValues(ui->comboBox_DesktopAudioDevice1, outputs, 1);
		_LoadListValues(ui->comboBox_DesktopAudioDevice2, outputs, 2);

		obs_properties_destroy(output_props);
	}
}

#define NBSP "\xC2\xA0"
void AFQAudioSettingAreaWidget::_LoadAudioSources()
{
	int nHotKeyRowCount = ui->formLayout_AudioHotkeys->rowCount();
	for (int i = nHotKeyRowCount - 1; i >= 0; i--)
	{
		QWidget* widgetToRemove = ui->formLayout_AudioHotkeys->itemAt(i, QFormLayout::FieldRole)->widget();
		ui->formLayout_AudioHotkeys->removeRow(i);
	}

	m_AudioSourceSignals.clear();
	m_AudioSources.clear();

	const char* enablePtm = AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Audio.EnablePushToMute");
	const char* ptmDelay = AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Audio.PushToMuteDelay");
	const char* enablePtt = AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Audio.EnablePushToTalk");
	const char* pttDelay = AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Audio.PushToTalkDelay");

	auto AddSource = [&](obs_source_t* source) 
	{
		if (!(obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO))
			return true;

		QVBoxLayout* vLayout = new QVBoxLayout();
		QHBoxLayout* hLayout1 = new QHBoxLayout();
		QHBoxLayout* hLayout2 = new QHBoxLayout();

		int nSpacing = ui->formLayout_AudioAdvanced->verticalSpacing();
		vLayout->setSpacing(nSpacing);
		hLayout1->setSpacing(nSpacing);
		hLayout2->setSpacing(nSpacing);

		vLayout->addLayout(hLayout1);
		vLayout->addLayout(hLayout2);

		auto ptmCB = new AFQSilentUpdateCheckBox();
		ptmCB->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		ptmCB->setFixedSize(QSize(160, 40));
		ptmCB->setText(enablePtm);
		ptmCB->setChecked(obs_source_push_to_mute_enabled(source));
		hLayout1->addWidget(ptmCB);

		QSpacerItem* hSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
		hLayout1->addItem(hSpacer1);

		auto ptmLB = new QLabel(ptmDelay);
		ptmLB->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		ptmLB->setFixedSize(QSize(170, 40));
		ptmLB->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		TruncateTextToLabelWidth(ptmLB, ptmLB->text());
		ptmLB->setStyleSheet("color: rgba(255, 255, 255, 60%);"
							"font: 400 14px;");
		hLayout1->addWidget(ptmLB);

		auto ptmSB = new AFQCustomSpinbox();
		ptmSB->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		ptmSB->setFixedSize(QSize(296, 40));
		ptmSB->setSuffix(NBSP "ms");
		ptmSB->setRange(0, INT_MAX);
		ptmSB->setValue(obs_source_get_push_to_mute_delay(source));
		hLayout1->addWidget(ptmSB);

		auto pttCB = new AFQSilentUpdateCheckBox();
		pttCB->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		pttCB->setFixedSize(QSize(160, 40));
		pttCB->setText(enablePtt);
		pttCB->setChecked(obs_source_push_to_talk_enabled(source));
		hLayout2->addWidget(pttCB);

		QSpacerItem* hSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
		hLayout2->addItem(hSpacer2);

		auto pttLB = new QLabel(ptmDelay);
		pttLB->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		pttLB->setFixedSize(QSize(170, 40));
		pttLB->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		TruncateTextToLabelWidth(pttLB, pttLB->text());
		pttLB->setStyleSheet("color: rgba(255, 255, 255, 60%);"
							"font: 400 14px;");
		hLayout2->addWidget(pttLB);

		auto pttSB = new AFQCustomSpinbox();
		pttSB->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		pttSB->setFixedSize(QSize(296, 40));
		pttSB->setSuffix(NBSP "ms");
		pttSB->setRange(0, INT_MAX);
		pttSB->setValue(obs_source_get_push_to_talk_delay(source));
		hLayout2->addWidget(pttSB);

		AFSettingUtils::HookWidget(ptmCB, this, CHECK_CHANGED, AUDIO_CHANGED);
		AFSettingUtils::HookWidget(ptmSB, this, SCROLL_CHANGED, AUDIO_CHANGED);
		AFSettingUtils::HookWidget(pttCB, this, CHECK_CHANGED, AUDIO_CHANGED);
		AFSettingUtils::HookWidget(pttSB, this, SCROLL_CHANGED, AUDIO_CHANGED);

		m_AudioSourceSignals.reserve(m_AudioSourceSignals.size() + 4);

		auto handler = obs_source_get_signal_handler(source);
		m_AudioSourceSignals.emplace_back(
			handler, "push_to_mute_changed",
			[](void* data, calldata_t* param) {
				QMetaObject::invokeMethod(
					static_cast<QObject*>(data),
					"qslotSetCheckedSilently",
					Q_ARG(bool,
						calldata_bool(param, "enabled")));
			},
			ptmCB);
		m_AudioSourceSignals.emplace_back(
			handler, "push_to_mute_delay",
			[](void* data, calldata_t* param) {
				QMetaObject::invokeMethod(
					static_cast<QObject*>(data),
					"qslotSetValueSilently",
					Q_ARG(int,
						calldata_int(param, "delay")));
			},
			ptmSB);
		m_AudioSourceSignals.emplace_back(
			handler, "push_to_talk_changed",
			[](void* data, calldata_t* param) {
				QMetaObject::invokeMethod(
					static_cast<QObject*>(data),
					"qslotSetCheckedSilently",
					Q_ARG(bool,
						calldata_bool(param, "enabled")));
			},
			pttCB);
		m_AudioSourceSignals.emplace_back(
			handler, "push_to_talk_delay",
			[](void* data, calldata_t* param) {
				QMetaObject::invokeMethod(
					static_cast<QObject*>(data),
					"qslotSetValueSilently",
					Q_ARG(int,
						calldata_int(param, "delay")));
			},
			pttSB);

		m_AudioSources.emplace_back(OBSGetWeakRef(source), ptmCB, ptmSB, pttCB, pttSB);

		QSize labelSize = ui->label_SampleRate->size();
		int spacing = ui->formLayout_AudioHotkeys->horizontalSpacing();
		labelSize.setWidth(labelSize.width() - spacing);

		auto label = new AFQSourceLabel(source);
		label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		label->setFixedSize(labelSize);
		TruncateTextToLabelWidth(label, label->text());
		label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

		connect(label, &AFQSourceLabel::Removed, [=]() {
			QMetaObject::invokeMethod(this, "qslotReloadAudioSources");
			});

		connect(label, &AFQSourceLabel::Destroyed, [=]() {
			QMetaObject::invokeMethod(this, "qslotReloadAudioSources");
			});

		ui->formLayout_AudioHotkeys->addRow(label, vLayout);
		
		return true;
	};

	using AddSource_t = decltype(AddSource);
	obs_enum_sources(
		[](void* data, obs_source_t* source) {
			auto& AddSource = *static_cast<AddSource_t*>(data);
			if (!obs_source_removed(source))
				AddSource(source);
			return true;
		},
		static_cast<void*>(&AddSource));

	if (!m_bIsAdvancedMode) {
		ui->frame_AudioHotkeys->hide();
	}
	else {
		if (ui->formLayout_AudioHotkeys->rowCount() == 0)
			ui->frame_AudioHotkeys->hide();
		else
			ui->frame_AudioHotkeys->show();
	}
}

void AFQAudioSettingAreaWidget::_LoadListValues(QComboBox* widget, obs_property_t* prop, int index)
{
	size_t count = obs_property_list_item_count(prop);

	OBSSourceAutoRelease source = obs_get_output_source(index);
	const char* deviceId = nullptr;
	OBSDataAutoRelease settings = nullptr;

	if (source) {
		settings = obs_source_get_settings(source);
		if (settings)
			deviceId = obs_data_get_string(settings, "device_id");
	}

	widget->addItem(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Audio.Disabled"), "disabled");

	for (size_t i = 0; i < count; i++) {
		const char* name = obs_property_list_item_name(prop, i);
		const char* val = obs_property_list_item_string(prop, i);
		_LoadListValue(widget, name, val);
	}

	if (deviceId) {
		QVariant var(QT_UTF8(deviceId));
		int idx = widget->findData(var);
		if (idx != -1) {
			widget->setCurrentIndex(idx);
		}
		else {
			widget->insertItem(0,
				AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Audio."
					"UnknownAudioDevice"),
				var);
			widget->setCurrentIndex(0);
		}
	}
}

void AFQAudioSettingAreaWidget::_LoadListValue(QComboBox* widget, const char* text, const char* val)
{
	widget->addItem(QT_UTF8(text), QT_UTF8(val));
}

static QString get_adv_audio_fallback(const QString& enc)
{
	const char* codec = obs_get_encoder_codec(QT_TO_UTF8(enc));

	if (codec && strcmp(codec, "aac") == 0)
		return "ffmpeg_opus";

	QString aac_default = "ffmpeg_aac";
	if (EncoderAvailable("CoreAudio_AAC"))
		aac_default = "CoreAudio_AAC";
	else if (EncoderAvailable("libfdk_aac"))
		aac_default = "libfdk_aac";

	return aac_default;
}

void AFQAudioSettingAreaWidget::_ResetEncoders(bool streamOnly)
{
	QString lastAdvAudioEnc = ui->comboBox_AdvOutAEncoder->currentData().toString();
	QString lastAudioEnc = ui->comboBox_SimpleOutStrAEncoder->currentData().toString();
	OBSDataAutoRelease settings = obs_data_create();
	const char* service_id = "rtmp_common";
	OBSServiceAutoRelease newService = obs_service_create(service_id, "temp_service", settings, nullptr);
	OBSService service = newService.Get();
	QString protocol = "RTMP";
	//

	const char** acodecs = obs_service_get_supported_audio_codecs(service);
	const char* type;
	BPtr<char*> output_vcodecs;
	BPtr<char*> output_acodecs;
	size_t idx = 0;

	if (!acodecs) {
		const char* output;

		obs_enum_output_types_with_protocol(QT_TO_UTF8(protocol),
			&output, return_first_id);
		output_acodecs = strlist_split(
			obs_get_output_supported_audio_codecs(output), ';',
			false);
		acodecs = (const char**)output_acodecs.Get();
	}

	QSignalBlocker s3(ui->comboBox_SimpleOutStrAEncoder);
	QSignalBlocker s4(ui->comboBox_AdvOutAEncoder);

	ui->comboBox_SimpleOutStrAEncoder->clear();
	ui->comboBox_AdvOutAEncoder->clear();

	while (obs_enum_encoder_types(idx++, &type)) {
		const char* name = obs_encoder_get_display_name(type);
		const char* codec = obs_get_encoder_codec(type);
		uint32_t caps = obs_get_encoder_caps(type);

		QString qName = QT_UTF8(name);
		QString qType = QT_UTF8(type);

		if (obs_get_encoder_type(type) == OBS_ENCODER_AUDIO) {
			if (ServiceSupportsCodec(acodecs, codec))
				ui->comboBox_AdvOutAEncoder->addItem(qName, qType);
		}
	}

	ui->comboBox_AdvOutAEncoder->model()->sort(0);

	#define ENCODER_STR(str) QTStr("Basic.Settings.Output.Simple.Encoder." str)
	if (ServiceSupportsEncoder(acodecs, "CoreAudio_AAC") ||
		ServiceSupportsEncoder(acodecs, "libfdk_aac") ||
		ServiceSupportsEncoder(acodecs, "ffmpeg_aac"))
		ui->comboBox_SimpleOutStrAEncoder->addItem(
			QTStr("Basic.Settings.Output.Simple.Codec.AAC.Default"),
			"aac");
	if (ServiceSupportsEncoder(acodecs, "ffmpeg_opus"))
	ui->comboBox_SimpleOutStrAEncoder->addItem(
		QTStr("Basic.Settings.Output.Simple.Codec.Opus"),
		"opus");
	#undef ENCODER_STR

	if (!lastAdvAudioEnc.isEmpty()) {
		int idx = ui->comboBox_AdvOutAEncoder->findData(lastAdvAudioEnc);
		if (idx == -1) {
			lastAdvAudioEnc =
				get_adv_audio_fallback(lastAdvAudioEnc);
			ui->comboBox_AdvOutAEncoder->setProperty("changed",
				QVariant(true));
			qslotAudioDataChanged();
		}

		idx = ui->comboBox_AdvOutAEncoder->findData(lastAdvAudioEnc);
		s4.unblock();
		ui->comboBox_AdvOutAEncoder->setCurrentIndex(idx);
	}

	if (!lastAudioEnc.isEmpty()) {
		int idx = ui->comboBox_SimpleOutStrAEncoder->findData(lastAudioEnc);
		if (idx == -1) {
			lastAudioEnc = (lastAudioEnc == "opus") ? "aac"
				: "opus";
			ui->comboBox_SimpleOutStrAEncoder->setProperty("changed",
				QVariant(true));
			qslotAudioDataChanged();
		}

		idx = ui->comboBox_SimpleOutStrAEncoder->findData(lastAudioEnc);
		s3.unblock();
		ui->comboBox_SimpleOutStrAEncoder->setCurrentIndex(idx);
	}
}

void AFQAudioSettingAreaWidget::_SetAudioSettingSignal()
{
	AFSettingUtils::HookWidget(ui->comboBox_SampleRate, this, COMBO_CHANGED, AUDIO_RESTART);
	AFSettingUtils::HookWidget(ui->comboBox_ChannelSetup, this, COMBO_CHANGED, AUDIO_RESTART);
	AFSettingUtils::HookWidget(ui->comboBox_SimpleOutputABitrate, this, COMBO_CHANGED, AUDIO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_SimpleOutStrAEncoder, this, COMBO_CHANGED, AUDIO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutAEncoder, this, COMBO_CHANGED, AUDIO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_DesktopAudioDevice1, this, COMBO_CHANGED, AUDIO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_DesktopAudioDevice2, this, COMBO_CHANGED, AUDIO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AuxAudioDevice1, this, COMBO_CHANGED, AUDIO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AuxAudioDevice2, this, COMBO_CHANGED, AUDIO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AuxAudioDevice3, this, COMBO_CHANGED, AUDIO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AuxAudioDevice4, this, COMBO_CHANGED, AUDIO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_MeterDecayRate, this, COMBO_CHANGED, AUDIO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_PeakMeterType, this, COMBO_CHANGED, AUDIO_CHANGED);
	
	if (obs_audio_monitoring_available())
		AFSettingUtils::HookWidget(ui->comboBox_MonitoringDevice, this, COMBO_CHANGED, AUDIO_CHANGED);

#ifdef _WIN32
	AFSettingUtils::HookWidget(ui->checkBox_DisableAudioDucking, this, CHECK_CHANGED, AUDIO_CHANGED);
#else
	delete ui->checkBox_DisableAudioDucking;
	ui->checkBox_DisableAudioDucking = nullptr;
#endif
	
	connect(ui->widget_Simple, &AFQHoverWidget::qsignalMouseClick, this, &AFQAudioSettingAreaWidget::qslotChangeSettingModeToSimple);
	connect(ui->widget_Advanced, &AFQHoverWidget::qsignalMouseClick, this, &AFQAudioSettingAreaWidget::qslotChangeSettingModeToAdvanced);

	connect(ui->comboBox_ChannelSetup, &QComboBox::currentIndexChanged, this, &AFQAudioSettingAreaWidget::qslotSurroundWarning);
	connect(ui->comboBox_ChannelSetup, &QComboBox::currentIndexChanged, this, &AFQAudioSettingAreaWidget::qslotSpeakerLayoutChanged);
	connect(ui->checkBox_LowLatencyBuffering, &QCheckBox::clicked, this, &AFQAudioSettingAreaWidget::qslotLowLatencyBufferingChanged);	
	
	connect(ui->comboBox_SimpleOutputABitrate, &QComboBox::currentIndexChanged, this, &AFQAudioSettingAreaWidget::qslotUpdateStreamDelayEstimate);
	connect(ui->comboBox_SimpleOutputABitrate, &QComboBox::currentIndexChanged, this, &AFQAudioSettingAreaWidget::qslotUpdateSimpleReplayBuffer);
	connect(ui->comboBox_SimpleOutputABitrate, &QComboBox::currentIndexChanged, this, &AFQAudioSettingAreaWidget::qslotUpdateSimpleRecordingEncoder);
	connect(ui->comboBox_SimpleOutStrAEncoder, &QComboBox::currentIndexChanged, this, &AFQAudioSettingAreaWidget::qslotSimpleStreamAudioEncoderChanged);
	connect(ui->comboBox_AdvOutAEncoder, &QComboBox::currentIndexChanged, this, &AFQAudioSettingAreaWidget::qslotAdvAudioEncodersChanged);

	connect(ui->comboBox_SimpleOutputABitrate, &QComboBox::currentIndexChanged, this, &AFQAudioSettingAreaWidget::qslotSimpleBitrateChanged);
	connect(ui->comboBox_SimpleOutStrAEncoder, &QComboBox::currentIndexChanged, this, &AFQAudioSettingAreaWidget::qslotSimpleEncoderChanged);
	
	m_nChannelIndex = ui->comboBox_ChannelSetup->currentIndex();
	m_nSampleRateIndex = ui->comboBox_SampleRate->currentIndex();
	m_bLlBufferingEnabled = ui->checkBox_LowLatencyBuffering->isChecked();
}

void AFQAudioSettingAreaWidget::_SetAudioSettingUi()
{
	if (!obs_audio_monitoring_available()) {
		delete ui->label_MonitoringDevice;
		ui->label_MonitoringDevice = nullptr;
		delete ui->comboBox_MonitoringDevice;
		ui->comboBox_MonitoringDevice = nullptr;
	}

	auto ReloadAudioSources = [](void* data, calldata_t* param) {
		auto settings = static_cast<AFQAudioSettingAreaWidget*>(data);
		auto source = static_cast<obs_source_t*>(
			calldata_ptr(param, "source"));

		if (!source)
			return;

		if (!(obs_source_get_output_flags(source) & OBS_SOURCE_AUDIO))
			return;

		QMetaObject::invokeMethod(settings, "qslotReloadAudioSources",
			Qt::QueuedConnection);
		};

	m_SourceCreated.Connect(obs_get_signal_handler(), "source_create", ReloadAudioSources, this);
	m_ChannelChanged.Connect(obs_get_signal_handler(), "channel_change", ReloadAudioSources, this);

	ui->comboBox_AdvOutAEncoder->setPlaceholderText(QTStr("CodecCompat.CodecPlaceholder"));
}

void AFQAudioSettingAreaWidget::_FillAudioMonitoringDevices()
{
	QComboBox* cb = ui->comboBox_MonitoringDevice;

	auto enum_devices = [](void* param, const char* name, const char* id) {
		QComboBox* cb = (QComboBox*)param;
		cb->addItem(name, id);
		return true;
		};

	cb->addItem(QTStr("Basic.Settings.Advanced.Audio.MonitoringDevice"
					".Default"),
					"default");

	obs_enum_audio_monitoring_devices(enum_devices, cb);
}

void AFQAudioSettingAreaWidget::_SetSettingModeUi(const bool isAdvanced)
{
	if (isAdvanced)
	{
		ui->widget_Simple->setProperty("modeActive", false);
		ui->widget_Advanced->setProperty("modeActive", true);
		ui->label_Simple->setProperty("modeActive", false);
		ui->label_Advanced->setProperty("modeActive", true);
	}
	else
	{
		ui->widget_Simple->setProperty("modeActive", true);
		ui->widget_Advanced->setProperty("modeActive", false);
		ui->label_Simple->setProperty("modeActive", true);
		ui->label_Advanced->setProperty("modeActive", false);
	}

	ui->label_SimpleOutStrAEncoder->setVisible(!isAdvanced);
	ui->comboBox_SimpleOutStrAEncoder->setVisible(!isAdvanced);
	ui->label_AdvOutAEncoder->setVisible(isAdvanced);
	ui->comboBox_AdvOutAEncoder->setVisible(isAdvanced);

	ui->label_DesktopAudioDevice2->setVisible(isAdvanced);
	ui->comboBox_DesktopAudioDevice2->setVisible(isAdvanced);

	ui->label_AuxAudioDevice2->setVisible(isAdvanced);
	ui->comboBox_AuxAudioDevice2->setVisible(isAdvanced);

	ui->label_AuxAudioDevice3->setVisible(isAdvanced);
	ui->comboBox_AuxAudioDevice3->setVisible(isAdvanced);

	ui->label_AuxAudioDevice4->setVisible(isAdvanced);
	ui->comboBox_AuxAudioDevice4->setVisible(isAdvanced);

	ui->frame_AudioMeters->setVisible(isAdvanced);
	ui->frame_AudioAdvanced->setVisible(isAdvanced);
	
	if(!isAdvanced)
		ui->frame_AudioHotkeys->hide();
	else {
		if (ui->formLayout_AudioHotkeys->rowCount() == 0)
			ui->frame_AudioHotkeys->hide();
		else
			ui->frame_AudioHotkeys->show();
	}

	style()->unpolish(ui->widget_Simple);
	style()->unpolish(ui->widget_Advanced);
	style()->unpolish(ui->label_Simple);
	style()->unpolish(ui->label_Advanced);

	style()->polish(ui->widget_Simple);
	style()->polish(ui->widget_Advanced);
	style()->polish(ui->label_Simple);
	style()->polish(ui->label_Advanced);
}

void AFQAudioSettingAreaWidget::_PopulateSimpleBitrates(QComboBox* box, bool opus)
{
	auto& bitrateMap = opus ? GetSimpleOpusEncoderBitrateMap()
							: GetSimpleAACEncoderBitrateMap();

	if (bitrateMap.empty())
		return;

	std::vector<std::pair<QString, QString>> pairs;
	for (auto& entry : bitrateMap)
		pairs.emplace_back(
			QString::number(entry.first),
			obs_encoder_get_display_name(entry.second.c_str()));

	QString currentBitrate = box->currentText();
	box->clear();

	for (auto& pair : pairs) {
		box->addItem(pair.first);
		box->setItemData(box->count() - 1, pair.second,
			Qt::ToolTipRole);
	}

	if (box->findData(currentBitrate) == -1) {
		int bitrate = _FindClosestAvailableAudioBitrate(
			box, currentBitrate.toInt());
		box->setCurrentText(QString::number(bitrate));
	}
	else
		box->setCurrentText(currentBitrate);
}

#define INVALID_BITRATE 10000
int AFQAudioSettingAreaWidget::_FindClosestAvailableAudioBitrate(QComboBox* box, int bitrate)
{
	QList<int> bitrates;
	int prev = 0;
	int next = INVALID_BITRATE;

	for (int i = 0; i < box->count(); i++)
		bitrates << box->itemText(i).toInt();

	for (int val : bitrates) {
		if (next > val) {
			if (val == bitrate)
				return bitrate;

			if (val < next && val > bitrate)
				next = val;
			if (val > prev && val < bitrate)
				prev = val;
		}
	}

	if (next != INVALID_BITRATE)
		return next;
	if (prev != 0)
		return prev;
	return 192;
}

bool AFQAudioSettingAreaWidget::_IsSurround(const char* speakers)
{
	static const char* surroundLayouts[] = { "2.1", "4.0", "4.1", "5.1", "7.1", nullptr };

	if (!speakers || !*speakers)
		return false;

	const char** curLayout = surroundLayouts;
	for (; *curLayout; ++curLayout) {
		if (strcmp(*curLayout, speakers) == 0) {
			return true;
		}
	}

	return false;
}


void AFQAudioSettingAreaWidget::_RestrictResetBitrates(std::initializer_list<QComboBox*> boxes, int maxbitrate)
{
	for (auto box : boxes) {
		int idx = box->currentIndex();
		int max_bitrate = _FindClosestAvailableAudioBitrate(box, maxbitrate);
		int count = box->count();
		int max_idx = box->findText(QT_UTF8(std::to_string(max_bitrate).c_str()));

		for (int i = (count - 1); i > max_idx; i--)
			box->removeItem(i);

		if (idx > max_idx) {
			int default_bitrate = _FindClosestAvailableAudioBitrate(box, maxbitrate / 2);
			int default_idx = box->findText(QT_UTF8(std::to_string(default_bitrate).c_str()));

			box->setCurrentIndex(default_idx);
			box->setProperty("changed", QVariant(true));
		}
		else {
			box->setCurrentIndex(idx);
		}
	}
}

void AFQAudioSettingAreaWidget::_SaveAudioBitrate()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();

	AFSettingUtils::SaveCombo(ui->comboBox_SimpleOutputABitrate, "SimpleOutput", "ABitrate");

	if (AFSettingUtils::WidgetChanged(ui->comboBox_SimpleOutputABitrate))
	{
		QString strBitrate = ui->comboBox_SimpleOutputABitrate->currentText();
		config_set_string(basicConfig, "AdvOut", "Track1Bitrate", QT_TO_UTF8(strBitrate));
		config_set_string(basicConfig, "AdvOut", "Track2Bitrate", QT_TO_UTF8(strBitrate));
		config_set_string(basicConfig, "AdvOut", "Track3Bitrate", QT_TO_UTF8(strBitrate));
		config_set_string(basicConfig, "AdvOut", "Track4Bitrate", QT_TO_UTF8(strBitrate));
		config_set_string(basicConfig, "AdvOut", "Track5Bitrate", QT_TO_UTF8(strBitrate));
		config_set_string(basicConfig, "AdvOut", "Track6Bitrate", QT_TO_UTF8(strBitrate));
	}
}
#pragma endregion private func

#pragma region QT Field
void AFQAudioSettingAreaWidget::qslotAudioChangedRestart()
{
	if (!m_bLoading) {
		int currentChannelIndex = ui->comboBox_ChannelSetup->currentIndex();
		int currentSampleRateIndex = ui->comboBox_SampleRate->currentIndex();
		bool currentLLAudioBufVal = ui->checkBox_LowLatencyBuffering->isChecked();

		if (currentChannelIndex != m_nChannelIndex ||
			currentSampleRateIndex != m_nSampleRateIndex ||
			currentLLAudioBufVal != m_bLlBufferingEnabled) {

			m_bRestartNeeded = true;
		}
		else {
		}

		m_bAudioDataChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalAudioDataChanged();
	}
}

void AFQAudioSettingAreaWidget::qslotAudioDataChanged()
{
	if (!m_bLoading)
	{
		m_bAudioDataChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalAudioDataChanged();
	}
}

void AFQAudioSettingAreaWidget::qslotChangeSettingModeToSimple()
{
	ChangeSettingModeToSimple();

	emit qsignalSimpleModeClicked();
}

void AFQAudioSettingAreaWidget::qslotChangeSettingModeToAdvanced()
{
	ChangeSettingModeToAdvanced();

	emit qsignalAdvancedModeClicked();
}

void AFQAudioSettingAreaWidget::qslotSimpleBitrateChanged()
{
	emit qsignalSimpleBitrateChanged(ui->comboBox_SimpleOutputABitrate->currentText().toInt());
}

void AFQAudioSettingAreaWidget::qslotSimpleEncoderChanged()
{
	emit qsignalSimpleEncoderChanged(ui->comboBox_SimpleOutStrAEncoder->currentData().toString());
}

#define MULTI_CHANNEL_WARNING "Basic.Settings.Audio.MultichannelWarning"
void AFQAudioSettingAreaWidget::qslotSurroundWarning(int idx)
{
	if (idx == m_nLastChannelSetupIdx || idx == -1)
		return;

	if (m_bLoading) {
		m_nLastChannelSetupIdx = idx;
		return;
	}

	auto& localeTextManger = AFLocaleTextManager::GetSingletonInstance();

	QString speakerLayoutQstr = ui->comboBox_ChannelSetup->itemText(idx);
	bool surround = _IsSurround(QT_TO_UTF8(speakerLayoutQstr));

	QString lastQstr = ui->comboBox_ChannelSetup->itemText(m_nLastChannelSetupIdx);
	bool wasSurround = _IsSurround(QT_TO_UTF8(lastQstr));

	if (surround && !wasSurround) 
	{
		QString warningString = localeTextManger.Str("Basic.Settings.ProgramRestart") +
								QStringLiteral("\n\n") +
								localeTextManger.Str(MULTI_CHANNEL_WARNING) +
								QStringLiteral("\n\n") +
								localeTextManger.Str(MULTI_CHANNEL_WARNING ".Confirm");

			int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
			this, "",
			warningString);

		if (result == QDialog::Rejected)
		{
			QMetaObject::invokeMethod(
				ui->comboBox_ChannelSetup, "setCurrentIndex",
				Qt::QueuedConnection,
				Q_ARG(int, m_nLastChannelSetupIdx));
			return;
		}
	}

	m_nLastChannelSetupIdx = idx;
}

void AFQAudioSettingAreaWidget::qslotSpeakerLayoutChanged(int idx)
{
	QString speakerLayoutQstr = ui->comboBox_ChannelSetup->itemText(idx);
	std::string speakerLayout = QT_TO_UTF8(speakerLayoutQstr);
	bool surround = _IsSurround(speakerLayout.c_str());
	bool isOpus = ui->comboBox_SimpleOutStrAEncoder->currentData().toString() == "opus";

	if (surround) {
		_PopulateSimpleBitrates(ui->comboBox_SimpleOutputABitrate, isOpus);

		std::string stream_encoder_id = ui->comboBox_AdvOutAEncoder->currentData()
			.toString()
			.toStdString();
	}
	else {
		_RestrictResetBitrates(
			{ ui->comboBox_SimpleOutputABitrate }, 320);

		_SaveAudioBitrate();
	}
}

#define LL_BUFFERING_WARNING "Basic.Settings.Audio.LowLatencyBufferingWarning"
void AFQAudioSettingAreaWidget::qslotLowLatencyBufferingChanged(bool checked)
{
	if (checked) {
		QString warningStr = AFLocaleTextManager::GetSingletonInstance().Str(LL_BUFFERING_WARNING) +
			QStringLiteral("\n\n") +
			AFLocaleTextManager::GetSingletonInstance().Str(LL_BUFFERING_WARNING ".Confirm");

		int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
			this, "", warningStr);

		if (result == QDialog::Rejected) {
			QMetaObject::invokeMethod(ui->checkBox_LowLatencyBuffering,
									"setChecked",
									Qt::QueuedConnection,
									Q_ARG(bool, false));
			return;
		}
	}

	QMetaObject::invokeMethod(this, "qslotAudioChangedRestart");
}

void AFQAudioSettingAreaWidget::qslotSimpleStreamAudioEncoderChanged()
{
	_PopulateSimpleBitrates(
		ui->comboBox_SimpleOutputABitrate,
		ui->comboBox_SimpleOutStrAEncoder->currentData().toString() == "opus");

	if (_IsSurround(QT_TO_UTF8(ui->comboBox_ChannelSetup->currentText())))
		return;

	_RestrictResetBitrates({ ui->comboBox_SimpleOutputABitrate }, 320);
}

void AFQAudioSettingAreaWidget::qslotAdvAudioEncodersChanged()
{
	QString streamEncoder = ui->comboBox_AdvOutAEncoder->currentData().toString();

	if (_IsSurround(QT_TO_UTF8(ui->comboBox_ChannelSetup->currentText())))
		return;
}

void AFQAudioSettingAreaWidget::qslotUpdateSimpleReplayBuffer()
{
	emit qsignalCallSimpleReplayBufferChanged();
}

void AFQAudioSettingAreaWidget::qslotUpdateSimpleRecordingEncoder()
{
	emit qsignalCallSimpleRecordingEncoderChanged();
}

void AFQAudioSettingAreaWidget::qslotUpdateStreamDelayEstimate()
{
	emit qsignalCallUpdateStreamDelayEstimate();
}
void AFQAudioSettingAreaWidget::qslotReloadAudioSources()
{
	_LoadAudioSources();
}
#pragma endregion QT Field