#include <sstream>
#include "CVideoSettingAreaWidget.h"
#include "ui_video-setting-area.h"
#include "platform/platform.hpp"

#include "../Common/SettingsMiscDef.h"
#include "../CoreModel/Output/COutput.inl"
#include "../CoreModel/Config/CArgOption.h"
#define VIDEO_CHANGED   &AFQVideoSettingAreaWidget::qslotVideoDataChanged
#define VIDEO_RES       &AFQVideoSettingAreaWidget::qslotVideoChangedResolution
#define VIDEO_RESTART   &AFQVideoSettingAreaWidget::qslotVideoChangedRestart

#pragma region class initializer, destructor
AFQVideoSettingAreaWidget::AFQVideoSettingAreaWidget(QWidget* parent) :
	ui(new Ui::AFQVideoSettingAreaWidget)
{
	ui->setupUi(this);

	_SetVideoSettingUi();
	_SetVideoSettingSignal();
	_ChangeLanguage();
}

AFQVideoSettingAreaWidget::~AFQVideoSettingAreaWidget()
{
	delete ui;
}
#pragma endregion class initializer, destructor

#pragma region QT Field
void AFQVideoSettingAreaWidget::qslotVideoDataChanged()
{
	if (!m_bLoading) 
	{
		m_bVideoDataChanged = true;
		sender()->setProperty("changed", QVariant(true));
		
		emit qsignalVideoDataChanged();
	}
}

void AFQVideoSettingAreaWidget::qslotVideoChangedResolution()
{
	if (!m_bLoading && _ValidResolutions()) 
	{
		m_bVideoDataChanged = true;
		sender()->setProperty("changed", QVariant(true));
		
		emit qsignalVideoDataChanged();
	}
}

void AFQVideoSettingAreaWidget::qslotVideoChangedRestart()
{
	if (!m_bLoading)
	{
		m_bVideoDataChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalVideoDataChanged();
	}
}

void AFQVideoSettingAreaWidget::qslotChangeSettingModeToSimple()
{
	ChangeSettingModeToSimple();

	emit qsignalSimpleModeClicked();
}

void AFQVideoSettingAreaWidget::qslotChangeSettingModeToAdvanced()
{
	ChangeSettingModeToAdvanced();

	emit qsignalAdvancedModeClicked();
}

void AFQVideoSettingAreaWidget::qslotSimpleBitrateChanged()
{
	QString strBitrate = ui->comboBox_SimpleOutputVBitrate->currentText();
	strBitrate.replace(" Kbps", "");

	emit qsignalSimpleBitrateChanged(strBitrate.toInt());
}

void AFQVideoSettingAreaWidget::qslotSimpleEncoderChanged()
{
	emit qsignalSimpleEncoderChanged(ui->comboBox_SimpleOutStrEncoder->currentData().toString());
}

void AFQVideoSettingAreaWidget::qslotAdvOutEncoderCurrentIndexChanged()
{
	QString encoder = AFSettingUtils::GetComboData(ui->comboBox_AdvOutEncoder);
	if (!m_bLoading) {
		bool loadSettings = encoder == m_strCurAdvStreamEncoder;

		delete m_streamEncoderProps;
		m_streamEncoderProps = nullptr;

		m_streamEncoderProps = _CreateEncoderPropertyView(
							QT_TO_UTF8(encoder),
							loadSettings ? "streamEncoder.json" : nullptr, true);
		m_streamEncoderProps->setSizePolicy(QSizePolicy::Preferred,
											QSizePolicy::Minimum);
		ui->verticalLayout_AdvOutEncoderProps->addWidget(m_streamEncoderProps);

		connect(m_streamEncoderProps, &AFQPropertiesView::Changed, this, &AFQVideoSettingAreaWidget::qslotStreamEncoderPropChanged);
	}

	ui->checkBox_AdvOutUseRescale->setVisible(true);
	ui->comboBox_AdvOutRescale->setVisible(true);
}

void AFQVideoSettingAreaWidget::qslotSimpleStreamingEncoderChanged()
{
	QString encoder = ui->comboBox_SimpleOutStrEncoder->currentData().toString();
	QString preset;
	const char* defaultPreset = nullptr;

	ui->label_SimpleOutPreset->setVisible(true);
	ui->comboBox_SimpleOutPreset->setVisible(true);
	ui->comboBox_SimpleOutPreset->clear();

	if (encoder == SIMPLE_ENCODER_QSV ||
		encoder == SIMPLE_ENCODER_QSV_AV1) {
		ui->comboBox_SimpleOutPreset->addItem("speed", "speed");
		ui->comboBox_SimpleOutPreset->addItem("balanced", "balanced");
		ui->comboBox_SimpleOutPreset->addItem("quality", "quality");

		defaultPreset = "balanced";
		preset = m_strCurQSVPreset;

	}
	else if (encoder == SIMPLE_ENCODER_NVENC ||
		encoder == SIMPLE_ENCODER_NVENC_HEVC ||
		encoder == SIMPLE_ENCODER_NVENC_AV1) {

		const char* name = get_simple_output_encoder(QT_TO_UTF8(encoder));
		obs_properties_t* props = obs_get_encoder_properties(name);

		obs_property_t* p = obs_properties_get(props, "preset2");
		size_t num = obs_property_list_item_count(p);
		for (size_t i = 0; i < num; i++) {
			const char* name = obs_property_list_item_name(p, i);
			const char* val = obs_property_list_item_string(p, i);

			ui->comboBox_SimpleOutPreset->addItem(QT_UTF8(name), val);
		}

		obs_properties_destroy(props);

		defaultPreset = "default";
		preset = m_strCurNVENCPreset;

	}
	else if (encoder == SIMPLE_ENCODER_AMD ||
		encoder == SIMPLE_ENCODER_AMD_HEVC) {
		ui->comboBox_SimpleOutPreset->addItem("Speed", "speed");
		ui->comboBox_SimpleOutPreset->addItem("Balanced", "balanced");
		ui->comboBox_SimpleOutPreset->addItem("Quality", "quality");

		defaultPreset = "balanced";
		preset = m_strCurAMDPreset;
	}
	else if (encoder == SIMPLE_ENCODER_APPLE_H264
#ifdef ENABLE_HEVC
		|| encoder == SIMPLE_ENCODER_APPLE_HEVC
#endif
		) {
		ui->comboBox_SimpleOutPreset->setVisible(false);
		ui->label_SimpleOutPreset->setVisible(false);

	}
	else if (encoder == SIMPLE_ENCODER_AMD_AV1) {
		ui->comboBox_SimpleOutPreset->addItem("Speed", "speed");
		ui->comboBox_SimpleOutPreset->addItem("Balanced", "balanced");
		ui->comboBox_SimpleOutPreset->addItem("Quality", "quality");
		ui->comboBox_SimpleOutPreset->addItem("High Quality", "highQuality");

		defaultPreset = "balanced";
		preset = m_strCurAMDAV1Preset;
	}
	else {

#define PRESET_STR(val) \
	QString(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Output.EncoderPreset." val)).arg(val)
		ui->comboBox_SimpleOutPreset->addItem(PRESET_STR("ultrafast"), "ultrafast");
		ui->comboBox_SimpleOutPreset->addItem("superfast", "superfast");
		ui->comboBox_SimpleOutPreset->addItem(PRESET_STR("veryfast"), "veryfast");
		ui->comboBox_SimpleOutPreset->addItem("faster", "faster");
		ui->comboBox_SimpleOutPreset->addItem(PRESET_STR("fast"), "fast");
#undef PRESET_STR

		if (ui->comboBox_SimpleOutPreset->findData(m_strCurPreset) == -1) {
			ui->comboBox_SimpleOutPreset->addItem(m_strCurPreset, m_strCurPreset);
			QStandardItemModel* model =
				qobject_cast<QStandardItemModel*>(
					ui->comboBox_SimpleOutPreset->model());
			QStandardItem* item =
				model->item(model->rowCount() - 1);
			item->setEnabled(false);
		}

		defaultPreset = "veryfast";
		preset = m_strCurPreset;
	}

	int idx = ui->comboBox_SimpleOutPreset->findData(QVariant(preset));
	if (idx == -1)
		idx = ui->comboBox_SimpleOutPreset->findData(QVariant(defaultPreset));

	ui->comboBox_SimpleOutPreset->setCurrentIndex(idx);
}

void AFQVideoSettingAreaWidget::qslotBaseResolutionEditTextChanged(const QString& text)
{
	if (!m_bLoading && _ValidResolutions()) 
	{
		QString baseResolution = text;
		uint32_t cx, cy;

		_ConvertResText(QT_TO_UTF8(baseResolution), cx, cy);

		std::tuple<int, int> aspect = AspectRatio(cx, cy);

		ResetDownscales(cx, cy);
		emit qsignalBaseResolutionChanged(cx, cy);
	}
}

void AFQVideoSettingAreaWidget::qslotOutputResolutionEditTextChanged(const QString& text)
{
	if (!m_bLoading) 
	{
		_RecalcOutputResPixels(QT_TO_UTF8(text));
		_LoadDownscaleFilters();
	}
}

void AFQVideoSettingAreaWidget::qslotUpdateSimpleReplayBuffer()
{
	emit qsignalCallSimpleReplayBufferChanged();
}

void AFQVideoSettingAreaWidget::qslotUpdateSimpleRecordingEncoder()
{
	emit qsignalCallSimpleRecordingEncoderChanged();
}

void AFQVideoSettingAreaWidget::qslotStreamEncoderPropChanged()
{
	emit qsignalCallStreamEncoderPropChanged();
}

void AFQVideoSettingAreaWidget::qslotUpdateStreamDelayEstimate()
{
	emit qsignalCallUpdateStreamDelayEstimate();
}

void AFQVideoSettingAreaWidget::qslotDisableOSXVSyncClicked() 
{
#ifdef __APPLE__
	if (!m_bLoading) {
		bool disable = ui->checkBox_DisableOSXVSync->isChecked();
		ui->checkBox_ResetOSXVSync->setEnabled(disable);
	}
#endif
}
#pragma endregion QT Field

#pragma region public func
void AFQVideoSettingAreaWidget::LoadVideoSettings(bool reset)
{
	m_bLoading = true;
	
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();

	int videoBitrate;
	const char* streamEnc;
	const char* preset;
	const char* videoColorFormat;
	const char* videoColorSpace;
	const char* videoColorRange;
	uint32_t sdrWhiteLevel;
	uint32_t hdrNominalPeakLevel;

	videoBitrate = config_get_uint(basicConfig, "SimpleOutput", "VBitrate");
	streamEnc = config_get_string(basicConfig, "SimpleOutput", "StreamEncoder");
	preset = config_get_string(basicConfig, "SimpleOutput", "Preset");
	videoColorFormat = config_get_string(basicConfig, "Video", "ColorFormat");
	videoColorSpace = config_get_string(basicConfig, "Video", "ColorSpace");
	videoColorRange = config_get_string(basicConfig, "Video", "ColorRange");
	sdrWhiteLevel = (uint32_t)config_get_uint(basicConfig, "Video", "SdrWhiteLevel");
	hdrNominalPeakLevel = (uint32_t)config_get_uint(basicConfig, "Video", "HdrNominalPeakLevel");

	bool rescale = config_get_bool(basicConfig, "AdvOut", "Rescale");
	const char* qsvPreset = config_get_string(basicConfig, "SimpleOutput", "QSVPreset");
	const char* nvPreset = config_get_string(basicConfig, "SimpleOutput", "NVENCPreset2");
	const char* amdPreset = config_get_string(basicConfig, "SimpleOutput", "AMDPreset");
	const char* amdAV1Preset = config_get_string(basicConfig, "SimpleOutput", "AMDAV1Preset");
	const char* rescaleRes = config_get_string(basicConfig, "AdvOut", "RescaleRes");

	m_strCurPreset = preset;
	m_strCurQSVPreset = qsvPreset;
	m_strCurNVENCPreset = nvPreset;
	m_strCurAMDPreset = amdPreset;
	m_strCurAMDAV1Preset = amdAV1Preset;

	ui->checkBox_AdvOutUseRescale->setChecked(rescale);
	ui->comboBox_AdvOutRescale->setEnabled(rescale);
	ui->comboBox_AdvOutRescale->setCurrentText(rescaleRes);

	_ResetEncoders();
	_LoadResolutionLists();
	_LoadFPSData();
	_LoadDownscaleFilters();
	_LoadAdvOutputStreamingEncoderProperties();
	_LoadRendererList();
	_LoadColorFormats();
	_LoadColorSpaces();
	_LoadColorRanges();

	std::string strBitrate = std::to_string(videoBitrate) + " Kbps";
	const char* charBitrate = strBitrate.c_str();

	AFSettingUtils::SetComboByName(ui->comboBox_SimpleOutputVBitrate, charBitrate);
	AFSettingUtils::SetComboByValue(ui->comboBox_SimpleOutStrEncoder, streamEnc);
	
	qslotSimpleEncoderChanged();
	qslotUpdateSimpleRecordingEncoder();
	qslotSimpleStreamingEncoderChanged();

	AFSettingUtils::SetComboByValue(ui->comboBox_ColorFormat, videoColorFormat);
	AFSettingUtils::SetComboByValue(ui->comboBox_ColorSpace, videoColorSpace);
	AFSettingUtils::SetComboByValue(ui->comboBox_ColorRange, videoColorRange);
	ui->spinBox_SdrWhiteLevel->setValue(sdrWhiteLevel);
	ui->spinBox_HdrNominalPeakLevel->setValue(hdrNominalPeakLevel);

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

void AFQVideoSettingAreaWidget::SaveVideoSettings()
{
	if (!m_bVideoDataChanged)
		return;

	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	config_t* globalConfig = AFConfigManager::GetSingletonInstance().GetGlobal();

	QString baseResolution = ui->comboBox_BaseResolution->currentText();
	QString outputResolution = ui->comboBox_OutputResolution->currentText();
	int fpsType = ui->comboBox_FpsType->currentIndex();
	uint32_t cx = 0, cy = 0;

	if (AFSettingUtils::WidgetChanged(ui->comboBox_BaseResolution) &&
		_ConvertResText(QT_TO_UTF8(baseResolution), cx, cy)) {
		config_set_uint(basicConfig, "Video", "BaseCX", cx);
		config_set_uint(basicConfig, "Video", "BaseCY", cy);
	}

	if (AFSettingUtils::WidgetChanged(ui->comboBox_OutputResolution) &&
		_ConvertResText(QT_TO_UTF8(outputResolution), cx, cy)) {
		config_set_uint(basicConfig, "Video", "OutputCX", cx);
		config_set_uint(basicConfig, "Video", "OutputCY", cy);
	}

	if (AFSettingUtils::WidgetChanged(ui->comboBox_FpsType))
		config_set_uint(basicConfig, "Video", "FPSType", fpsType);

	AFSettingUtils::SaveCombo(ui->comboBox_FpsCommon, "Video", "FPSCommon");
	AFSettingUtils::SaveDoubleSpinBox(ui->doubleSpinBox_FpsInteger, "Video", "FPSInt");
	AFSettingUtils::SaveComboData(ui->comboBox_DownscaleFilter, "Video", "ScaleType");

	QString strBitrate = ui->comboBox_SimpleOutputVBitrate->currentText();
	strBitrate.replace(" Kbps", "");
	config_set_uint(basicConfig, "SimpleOutput", "VBitrate", strBitrate.toInt());

	AFSettingUtils::SaveComboData(ui->comboBox_SimpleOutStrEncoder, "SimpleOutput", "StreamEncoder");

	QString encoder = ui->comboBox_SimpleOutStrEncoder->currentData().toString();
	const char* presetType;

	if (encoder == SIMPLE_ENCODER_QSV)
		presetType = "QSVPreset";
	else if (encoder == SIMPLE_ENCODER_QSV_AV1)
		presetType = "QSVPreset";
	else if (encoder == SIMPLE_ENCODER_NVENC)
		presetType = "NVENCPreset2";
	else if (encoder == SIMPLE_ENCODER_NVENC_AV1)
		presetType = "NVENCPreset2";
#ifdef ENABLE_HEVC
	else if (encoder == SIMPLE_ENCODER_AMD_HEVC)
		presetType = "AMDPreset";
	else if (encoder == SIMPLE_ENCODER_NVENC_HEVC)
		presetType = "NVENCPreset2";
#endif
	else if (encoder == SIMPLE_ENCODER_AMD)
		presetType = "AMDPreset";
	else if (encoder == SIMPLE_ENCODER_AMD_AV1)
		presetType = "AMDAV1Preset";
	else if (encoder == SIMPLE_ENCODER_APPLE_H264
#ifdef ENABLE_HEVC
		|| encoder == SIMPLE_ENCODER_APPLE_HEVC
#endif
		)
		presetType = "ApplePreset";
	else
		presetType = "Preset";

	AFSettingUtils::SaveComboData(ui->comboBox_SimpleOutPreset, "SimpleOutput", presetType);
	m_strCurAdvStreamEncoder = AFSettingUtils::GetComboData(ui->comboBox_AdvOutEncoder);
	AFSettingUtils::SaveComboData(ui->comboBox_AdvOutEncoder, "AdvOut", "Encoder");
	AFSettingUtils::WriteJsonData(m_streamEncoderProps, "streamEncoder.json");
	AFSettingUtils::SaveCheckBox(ui->checkBox_AdvOutUseRescale, "AdvOut", "Rescale");
	AFSettingUtils::SaveCombo(ui->comboBox_AdvOutRescale, "AdvOut", "RescaleRes");

#ifdef _WIN32
	if (AFSettingUtils::WidgetChanged(ui->comboBox_Renderer))
		config_set_string(globalConfig, "Video", "Renderer", QT_TO_UTF8(ui->comboBox_Renderer->currentText()));
#elif __APPLE__
	if (AFSettingUtils::WidgetChanged(ui->checkBox_DisableOSXVSync)) {
		bool disable = ui->checkBox_DisableOSXVSync->isChecked();
		config_set_bool(globalConfig, "Video",
			"DisableOSXVSync", disable);
		EnableOSXVSync(!disable);
	}
	if (AFSettingUtils::WidgetChanged(ui->checkBox_ResetOSXVSync))
		config_set_bool(globalConfig, "Video",
			"ResetOSXVSyncOnExit",
			ui->checkBox_ResetOSXVSync->isChecked());
#endif

	AFSettingUtils::SaveComboData(ui->comboBox_ColorFormat, "Video", "ColorFormat");
	AFSettingUtils::SaveComboData(ui->comboBox_ColorSpace, "Video", "ColorSpace");
	AFSettingUtils::SaveComboData(ui->comboBox_ColorRange, "Video", "ColorRange");
	AFSettingUtils::SaveSpinBox(ui->spinBox_SdrWhiteLevel, "Video", "SdrWhiteLevel");
	AFSettingUtils::SaveSpinBox(ui->spinBox_HdrNominalPeakLevel, "Video", "HdrNominalPeakLevel");
}

void AFQVideoSettingAreaWidget::ResetVideoSettings()
{
	_ClearVideoSettingUi();
	LoadVideoSettings(true);
}

void AFQVideoSettingAreaWidget::ToggleOnStreaming(bool streaming)
{
	bool useVideo = obs_video_active() ? false : true;
	ui->frame_SettingMode->setEnabled(useVideo);
	ui->frame_VideoGeneral->setEnabled(useVideo);
	ui->frame_PropertiesFrame->setEnabled(useVideo);
	ui->frame_VideoEncoder->setEnabled(useVideo);
	ui->frame_VideoRenderer->setEnabled(useVideo);
	ui->label_SimpleOutStrEncoder->setEnabled(useVideo);
	ui->comboBox_SimpleOutStrEncoder->setEnabled(useVideo);
}

int AFQVideoSettingAreaWidget::VideoBitrate()
{
	QString value = ui->comboBox_SimpleOutputVBitrate->currentText();
	QStringList list = value.split(" ");
	return list[0].toInt();
}

int AFQVideoSettingAreaWidget::VideoAdvBitrate()
{
	if (m_streamEncoderProps == nullptr)
	{
		return 0;
	}

	OBSData settings = m_streamEncoderProps->GetSettings();
	int vBitrate = (int)obs_data_get_int(settings, "bitrate");

	return vBitrate;
}

const char* AFQVideoSettingAreaWidget::VideoAdvRateControl()
{
	if (m_streamEncoderProps == nullptr)
	{
		return "";
	}

	OBSData settings = m_streamEncoderProps->GetSettings();
	const char* cRateControl = obs_data_get_string(settings, "rate_control");

	return cRateControl;
}

QString AFQVideoSettingAreaWidget::GetAdvVideoEncoder()
{
	if (ui->comboBox_AdvOutEncoder->currentIndex() < 0)
		return "";
	return ui->comboBox_AdvOutEncoder->currentData().toString();
}

QString AFQVideoSettingAreaWidget::GetSimpleVideoEncoder()
{
	if (ui->comboBox_SimpleOutStrEncoder->currentIndex() < 0)
		return "";
	return ui->comboBox_SimpleOutStrEncoder->currentData().toString();
}
#pragma endregion public func

#pragma region private func
void AFQVideoSettingAreaWidget::_ClearVideoSettingUi()
{
	ui->comboBox_BaseResolution->clear();
	ui->comboBox_OutputResolution->clear();
	ui->comboBox_DownscaleFilter->clear();
	ui->comboBox_SimpleOutStrEncoder->clear();
	ui->comboBox_SimpleOutPreset->clear();
	ui->comboBox_AdvOutEncoder->clear();
	ui->comboBox_AdvOutRescale->clear();

	if (m_streamEncoderProps != nullptr)
	{
		delete m_streamEncoderProps;
		m_streamEncoderProps = nullptr;
	}

	QLayoutItem* item;
	while ((item = ui->verticalLayout_AdvOutEncoderProps->takeAt(0)) != NULL)
	{
		delete item->widget();
		delete item;
	}

#ifdef _WIN32
	ui->comboBox_Renderer->clear();
#endif
	ui->comboBox_ColorFormat->clear();
	ui->comboBox_ColorSpace->clear();
	ui->comboBox_ColorRange->clear();
}

void AFQVideoSettingAreaWidget::_ChangeLanguage()
{
	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();

	QList<QLabel*> labelList = findChildren<QLabel*>();
	QList<QComboBox*> comboboxList = findChildren<QComboBox*>();

	foreach(QLabel * label, labelList)
	{
		label->setText(QT_UTF8(localeTextManager.Str(label->text().toUtf8().constData())));
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

			combobox->setItemText(i, QT_UTF8(localeTextManager.Str(itemString.toUtf8().constData())));
		}
	}
}

void AFQVideoSettingAreaWidget::_SetVideoSettingSignal()
{
	AFSettingUtils::HookWidget(ui->comboBox_BaseResolution, this, CBEDIT_CHANGED, VIDEO_RES);
	AFSettingUtils::HookWidget(ui->comboBox_OutputResolution, this, CBEDIT_CHANGED, VIDEO_RES);
	AFSettingUtils::HookWidget(ui->comboBox_SimpleOutputVBitrate, this, COMBO_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_SimpleOutStrEncoder, this, COMBO_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_SimpleOutPreset, this, COMBO_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_DownscaleFilter, this, COMBO_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_FpsType, this, COMBO_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_FpsCommon, this, COMBO_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->doubleSpinBox_FpsInteger, this, DSCROLL_CHANGED, VIDEO_CHANGED);
	
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutEncoder, this, COMBO_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_AdvOutRescale, this, CBEDIT_CHANGED, VIDEO_CHANGED);

#ifdef _WIN32
	AFSettingUtils::HookWidget(ui->comboBox_Renderer, this, COMBO_CHANGED, VIDEO_RESTART);
#elif __APPLE__
	AFSettingUtils::HookWidget(ui->checkBox_DisableOSXVSync, this, CHECK_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_ResetOSXVSync, this, CHECK_CHANGED, VIDEO_CHANGED);
#endif

	AFSettingUtils::HookWidget(ui->comboBox_ColorFormat, this, COMBO_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_ColorSpace, this, COMBO_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_ColorRange, this, COMBO_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_SdrWhiteLevel, this, SCROLL_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->spinBox_HdrNominalPeakLevel, this, SCROLL_CHANGED, VIDEO_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_AdvOutUseRescale, this, CHECK_CHANGED, VIDEO_CHANGED);

	connect(ui->comboBox_AdvOutEncoder, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotAdvOutEncoderCurrentIndexChanged);
	connect(ui->comboBox_FpsType, &QComboBox::currentIndexChanged, ui->stackedWidget_FpsPage, &QStackedWidget::setCurrentIndex);
	connect(ui->comboBox_SimpleOutStrEncoder, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotSimpleStreamingEncoderChanged);
	connect(ui->comboBox_SimpleOutStrEncoder, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotUpdateSimpleRecordingEncoder);
	connect(ui->comboBox_SimpleOutputVBitrate, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotUpdateSimpleReplayBuffer);
	connect(ui->comboBox_SimpleOutputVBitrate, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotUpdateSimpleRecordingEncoder);
	connect(ui->comboBox_SimpleOutputVBitrate, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotUpdateStreamDelayEstimate);
	connect(ui->comboBox_BaseResolution, &QComboBox::editTextChanged, this, &AFQVideoSettingAreaWidget::qslotBaseResolutionEditTextChanged);
	connect(ui->comboBox_OutputResolution, &QComboBox::editTextChanged, this, &AFQVideoSettingAreaWidget::qslotOutputResolutionEditTextChanged);
	
	connect(ui->widget_Simple, &AFQHoverWidget::qsignalMouseClick, this, &AFQVideoSettingAreaWidget::qslotChangeSettingModeToSimple);
	connect(ui->widget_Advanced, &AFQHoverWidget::qsignalMouseClick, this, &AFQVideoSettingAreaWidget::qslotChangeSettingModeToAdvanced);

	connect(ui->comboBox_SimpleOutputVBitrate, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotSimpleBitrateChanged);
	connect(ui->comboBox_SimpleOutStrEncoder, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotSimpleEncoderChanged);
	connect(ui->checkBox_AdvOutUseRescale, &QCheckBox::toggled, ui->comboBox_AdvOutRescale, &QComboBox::setEnabled);

#ifdef __APPLE__
	connect(ui->checkBox_DisableOSXVSync, &QCheckBox::toggled, this, &AFQVideoSettingAreaWidget::qslotDisableOSXVSyncClicked);
#endif
}

void AFQVideoSettingAreaWidget::_SetVideoSettingUi()
{
	QRegularExpression rx("\\d{1,5}x\\d{1,5}");
	QValidator* validator = new QRegularExpressionValidator(rx, this);
	ui->comboBox_BaseResolution->lineEdit()->setValidator(validator);
	ui->comboBox_OutputResolution->lineEdit()->setValidator(validator);
	ui->comboBox_AdvOutRescale->lineEdit()->setValidator(validator);
}

void AFQVideoSettingAreaWidget::_SetSettingModeUi(const bool isAdvanced)
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

	ui->label_BaseResolution->setVisible(isAdvanced);
	ui->comboBox_BaseResolution->setVisible(isAdvanced);

	ui->label_DownscaleFilter->setVisible(isAdvanced);
	ui->comboBox_DownscaleFilter->setVisible(isAdvanced);

	ui->label_SimpleOutputVBitrate->setVisible(!isAdvanced);
	ui->comboBox_SimpleOutputVBitrate->setVisible(!isAdvanced);

	ui->label_SimpleOutStrEncoder->setVisible(!isAdvanced);
	ui->comboBox_SimpleOutStrEncoder->setVisible(!isAdvanced);

	ui->label_SimpleOutPreset->setVisible(!isAdvanced);
	ui->comboBox_SimpleOutPreset->setVisible(!isAdvanced);

	ui->frame_VideoEncoder->setVisible(isAdvanced);
	ui->frame_PropertiesFrame->setVisible(isAdvanced);
	ui->frame_VideoRenderer->setVisible(isAdvanced);

	style()->unpolish(ui->widget_Simple);
	style()->unpolish(ui->widget_Advanced);
	style()->unpolish(ui->label_Simple);
	style()->unpolish(ui->label_Advanced);

	style()->polish(ui->widget_Simple);
	style()->polish(ui->widget_Advanced);
	style()->polish(ui->label_Simple);
	style()->polish(ui->label_Advanced);
}

void AFQVideoSettingAreaWidget::_LoadResolutionLists()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();

	uint32_t cx = config_get_uint(basicConfig, "Video", "BaseCX");
	uint32_t cy = config_get_uint(basicConfig, "Video", "BaseCY");
	uint32_t out_cx = config_get_uint(basicConfig, "Video", "OutputCX");
	uint32_t out_cy = config_get_uint(basicConfig, "Video", "OutputCY");

	ui->comboBox_BaseResolution->clear();

	auto addRes = [this](int cx, int cy) {
		QString res = _ResString(cx, cy).c_str();

		if (ui->comboBox_BaseResolution->findText(res) == -1)
			ui->comboBox_BaseResolution->addItem(res);
		};

	for (QScreen* screen : QGuiApplication::screens()) {
		QSize as = screen->size();
		uint32_t as_width = as.width();
		uint32_t as_height = as.height();

		as_width = round(as_width * screen->devicePixelRatio());
		as_height = round(as_height * screen->devicePixelRatio());

		addRes(as_width, as_height);
	}

	addRes(1920, 1080);
	addRes(1280, 720);

	std::string outputResString = _ResString(out_cx, out_cy);
	
	ui->comboBox_BaseResolution->lineEdit()->setText(_ResString(cx, cy).c_str());

	_RecalcOutputResPixels(outputResString.c_str());
	ResetDownscales(cx, cy);
	emit qsignalBaseResolutionChanged(cx, cy);

	ui->comboBox_OutputResolution->lineEdit()->setText(outputResString.c_str());

	std::tuple<int, int> aspect = AspectRatio(cx, cy);
}

void AFQVideoSettingAreaWidget::_LoadRendererList()
{
	config_t* globalConfig = AFConfigManager::GetSingletonInstance().GetGlobal();

#ifdef _WIN32
	const char* renderer = config_get_string(globalConfig, "Video", "Renderer");

	ui->comboBox_Renderer->addItem(QT_UTF8("Direct3D 11"));

	AFArgOption* pTmpArgOption = AFConfigManager::GetSingletonInstance().GetArgOption();

	if (pTmpArgOption->GetAllowOpenGL() || strcmp(renderer, "OpenGL") == 0)
		ui->comboBox_Renderer->addItem(QT_UTF8("OpenGL"));

	int idx = ui->comboBox_Renderer->findText(QT_UTF8(renderer));
	if (idx == -1)
		idx = 0;

	ui->comboBox_Renderer->setCurrentIndex(idx);

	delete ui->checkBox_DisableOSXVSync;
	delete ui->checkBox_ResetOSXVSync;
	ui->checkBox_DisableOSXVSync = nullptr;
	ui->checkBox_ResetOSXVSync = nullptr;
#elif __APPLE__
	bool disableOSXVSync = config_get_bool(globalConfig, "Video",
		"DisableOSXVSync");
	bool resetOSXVSync = config_get_bool(globalConfig, "Video",
		"ResetOSXVSyncOnExit");
	ui->checkBox_DisableOSXVSync->setChecked(disableOSXVSync);
	ui->checkBox_ResetOSXVSync->setChecked(resetOSXVSync);
	ui->checkBox_ResetOSXVSync->setEnabled(disableOSXVSync);

	delete ui->comboBox_Renderer;
	delete ui->label_Renderer;
	ui->label_Renderer = nullptr;
	ui->comboBox_Renderer = nullptr;
#endif
}

#define CF_NV12_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorFormat.NV12")
#define CF_I420_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorFormat.I420")
#define CF_I444_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorFormat.I444")
#define CF_P010_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorFormat.P010")
#define CF_I010_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorFormat.I010")
#define CF_P216_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorFormat.P216")
#define CF_P416_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorFormat.P416")
#define CF_BGRA_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorFormat.BGRA")
void AFQVideoSettingAreaWidget::_LoadColorFormats()
{
	ui->comboBox_ColorFormat->addItem(CF_NV12_STR, "NV12");
	ui->comboBox_ColorFormat->addItem(CF_I420_STR, "I420");
	ui->comboBox_ColorFormat->addItem(CF_I444_STR, "I444");
	ui->comboBox_ColorFormat->addItem(CF_P010_STR, "P010");
	ui->comboBox_ColorFormat->addItem(CF_I010_STR, "I010");
	ui->comboBox_ColorFormat->addItem(CF_P216_STR, "P216");
	ui->comboBox_ColorFormat->addItem(CF_P416_STR, "P416");
	ui->comboBox_ColorFormat->addItem(CF_BGRA_STR, "RGB");
}

#define CS_SRGB_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorSpace.sRGB")
#define CS_709_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorSpace.709")
#define CS_601_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorSpace.601")
#define CS_2100PQ_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorSpace.2100PQ")
#define CS_2100HLG_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorSpace.2100HLG")
void AFQVideoSettingAreaWidget::_LoadColorSpaces()
{
	ui->comboBox_ColorSpace->addItem(CS_SRGB_STR, "sRGB");
	ui->comboBox_ColorSpace->addItem(CS_709_STR, "709");
	ui->comboBox_ColorSpace->addItem(CS_601_STR, "601");
	ui->comboBox_ColorSpace->addItem(CS_2100PQ_STR, "2100PQ");
	ui->comboBox_ColorSpace->addItem(CS_2100HLG_STR, "2100HLG");
}

#define CS_PARTIAL_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorRange.Partial")
#define CS_FULL_STR AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced.Video.ColorRange.Full")
void AFQVideoSettingAreaWidget::_LoadColorRanges()
{
	ui->comboBox_ColorRange->addItem(CS_PARTIAL_STR, "Partial");
	ui->comboBox_ColorRange->addItem(CS_FULL_STR, "Full");
}

void AFQVideoSettingAreaWidget::_LoadFPSData()
{
	_LoadFPSCommon();
	_LoadFPSInteger();

	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	uint32_t fpsType = config_get_uint(basicConfig, "Video", "FPSType");

	if (fpsType > 2)
		fpsType = 0;

	ui->comboBox_FpsType->setCurrentIndex(fpsType);
	ui->stackedWidget_FpsPage->setCurrentIndex(fpsType);
}

void AFQVideoSettingAreaWidget::_LoadFPSCommon()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	const char* val = config_get_string(basicConfig, "Video", "FPSCommon");
	
	int idx = ui->comboBox_FpsCommon->findText(val);

	if (idx == -1)
		idx = 4;
	
	ui->comboBox_FpsCommon->setCurrentIndex(idx);
}

void AFQVideoSettingAreaWidget::_LoadFPSInteger()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	uint32_t val = config_get_uint(basicConfig, "Video", "FPSInt");

	ui->doubleSpinBox_FpsInteger->setValue(val);
}

void AFQVideoSettingAreaWidget::_LoadDownscaleFilters()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();
	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();

	QString downscaleFilter = ui->comboBox_DownscaleFilter->currentData().toString();

	if (downscaleFilter.isEmpty())
		downscaleFilter = config_get_string(basicConfig, "Video", "ScaleType");

	ui->comboBox_DownscaleFilter->clear();

	if (ui->comboBox_BaseResolution->currentText() == ui->comboBox_OutputResolution->currentText()) 
	{
		ui->comboBox_DownscaleFilter->setEnabled(false);
		ui->comboBox_DownscaleFilter->addItem(
			QTStr("Basic.Settings.Video.DownscaleFilter.Unavailable"),
			downscaleFilter);
	}
	else 
	{
		ui->comboBox_DownscaleFilter->setEnabled(true);		
		ui->comboBox_DownscaleFilter->addItem(
			QTStr("Basic.Settings.Video.DownscaleFilter.Bilinear"),
			QT_UTF8("bilinear"));
		ui->comboBox_DownscaleFilter->addItem(
			QTStr("Basic.Settings.Video.DownscaleFilter.Area"),
			QT_UTF8("area"));

		ui->comboBox_DownscaleFilter->addItem(
			QTStr("Basic.Settings.Video.DownscaleFilter.Bicubic"),
			QT_UTF8("bicubic"));

		ui->comboBox_DownscaleFilter->addItem(
			QTStr("Basic.Settings.Video.DownscaleFilter.Lanczos"),
			QT_UTF8("lanczos"));

		if (downscaleFilter == "bilinear")
			ui->comboBox_DownscaleFilter->setCurrentIndex(0);
		else if (downscaleFilter == "lanczos")
			ui->comboBox_DownscaleFilter->setCurrentIndex(3);
		else if (downscaleFilter == "area")
			ui->comboBox_DownscaleFilter->setCurrentIndex(1);
		else
			ui->comboBox_DownscaleFilter->setCurrentIndex(2);
	}
}

void AFQVideoSettingAreaWidget::_LoadAdvOutputStreamingEncoderProperties()
{
	config_t* basicConfig = AFConfigManager::GetSingletonInstance().GetBasic();

	const char* type = config_get_string(basicConfig, "AdvOut", "Encoder");

	if (m_streamEncoderProps != nullptr) 
	{
		delete m_streamEncoderProps;
		m_streamEncoderProps = nullptr;
	}

	m_streamEncoderProps = _CreateEncoderPropertyView(type, "streamEncoder.json");
	m_streamEncoderProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	
	ui->verticalLayout_AdvOutEncoderProps->addWidget(m_streamEncoderProps);

	connect(m_streamEncoderProps, &AFQPropertiesView::Changed, this, &AFQVideoSettingAreaWidget::qslotStreamEncoderPropChanged);
	qslotStreamEncoderPropChanged();

	m_strCurAdvStreamEncoder = type;

	if (!AFSettingUtils::SetComboByValue(ui->comboBox_AdvOutEncoder, type)) {
		uint32_t caps = obs_get_encoder_caps(type);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			const char* name = obs_encoder_get_display_name(type);

			ui->comboBox_AdvOutEncoder->insertItem(0, QT_UTF8(name),
				QT_UTF8(type));

			AFSettingUtils::SetComboByValue(ui->comboBox_AdvOutEncoder, type);
		}
	}

	emit qsignalCallUpdateStreamDelayEstimate();
}

bool AFQVideoSettingAreaWidget::_ValidResolutions()
{
	QString baseRes = ui->comboBox_BaseResolution->lineEdit()->text();
	uint32_t cx, cy;

	if (!_ConvertResText(QT_TO_UTF8(baseRes), cx, cy)) 
	{
		return false;
	}

	bool lockedOutRes = !ui->comboBox_OutputResolution->isEditable();
	if (!lockedOutRes) {
		QString outRes = ui->comboBox_OutputResolution->lineEdit()->text();
		if (!_ConvertResText(QT_TO_UTF8(outRes), cx, cy)) {
			return false;
		}
	}

	return true;
}

void AFQVideoSettingAreaWidget::_RecalcOutputResPixels(const char* resText)
{
	uint32_t newCX;
	uint32_t newCY;

	if (_ConvertResText(resText, newCX, newCY) && newCX && newCY) 
	{
		m_nOutputCX = newCX;
		m_nOutputCY = newCY;

		std::tuple<int, int> aspect = AspectRatio(m_nOutputCX, m_nOutputCY);
	}
}

void AFQVideoSettingAreaWidget::_ResetEncoders(bool streamOnly)
{
	QString lastAdvVideoEnc = ui->comboBox_AdvOutEncoder->currentData().toString();
	QString lastVideoEnc = ui->comboBox_SimpleOutStrEncoder->currentData().toString();
	OBSDataAutoRelease settings = obs_data_create();
	const char* service_id = "rtmp_common";
	OBSServiceAutoRelease newService = obs_service_create(service_id, "temp_service", settings, nullptr);
	OBSService service = newService.Get();
	QString protocol = "RTMP";
	
	const char** vcodecs = obs_service_get_supported_video_codecs(service);
	const char* type;
	BPtr<char*> output_vcodecs;
	size_t idx = 0;

	if (!vcodecs) {
		const char* output;

		obs_enum_output_types_with_protocol(QT_TO_UTF8(protocol),
			&output, return_first_id);
		output_vcodecs = strlist_split(
			obs_get_output_supported_video_codecs(output), ';',
			false);
		vcodecs = (const char**)output_vcodecs.Get();
	}

	QSignalBlocker s1(ui->comboBox_SimpleOutStrEncoder);
	QSignalBlocker s2(ui->comboBox_AdvOutEncoder);

	ui->comboBox_SimpleOutStrEncoder->clear();
	ui->comboBox_AdvOutEncoder->clear();

	while (obs_enum_encoder_types(idx++, &type)) {
		const char* name = obs_encoder_get_display_name(type);
		const char* codec = obs_get_encoder_codec(type);
		uint32_t caps = obs_get_encoder_caps(type);

		QString qName = QT_UTF8(name);
		QString qType = QT_UTF8(type);

		if (obs_get_encoder_type(type) == OBS_ENCODER_VIDEO) {
			if ((caps & ENCODER_HIDE_FLAGS) != 0)
				continue;

			if (ServiceSupportsCodec(vcodecs, codec))
				ui->comboBox_AdvOutEncoder->addItem(qName, qType);
		}
	}

	ui->comboBox_AdvOutEncoder->model()->sort(0);

	#define ENCODER_STR(str) QTStr("Basic.Settings.Output.Simple.Encoder." str)

	ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Software"),
		QString(SIMPLE_ENCODER_X264));
	#ifdef _WIN32
	if (ServiceSupportsEncoder(vcodecs, "obs_qsv11"))
	ui->comboBox_SimpleOutStrEncoder->addItem(
		ENCODER_STR("Hardware.QSV.H264"),
		QString(SIMPLE_ENCODER_QSV));
	if (ServiceSupportsEncoder(vcodecs, "obs_qsv11_av1"))
	ui->comboBox_SimpleOutStrEncoder->addItem(
		ENCODER_STR("Hardware.QSV.AV1"),
		QString(SIMPLE_ENCODER_QSV_AV1));
	#endif
	if (ServiceSupportsEncoder(vcodecs, "ffmpeg_nvenc"))
	ui->comboBox_SimpleOutStrEncoder->addItem(
		ENCODER_STR("Hardware.NVENC.H264"),
		QString(SIMPLE_ENCODER_NVENC));
	if (ServiceSupportsEncoder(vcodecs, "jim_av1_nvenc"))
	ui->comboBox_SimpleOutStrEncoder->addItem(
		ENCODER_STR("Hardware.NVENC.AV1"),
		QString(SIMPLE_ENCODER_NVENC_AV1));
	#ifdef ENABLE_HEVC
	if (ServiceSupportsEncoder(vcodecs, "h265_texture_amf"))
	ui->comboBox_SimpleOutStrEncoder->addItem(
		ENCODER_STR("Hardware.AMD.HEVC"),
		QString(SIMPLE_ENCODER_AMD_HEVC));
	if (ServiceSupportsEncoder(vcodecs, "ffmpeg_hevc_nvenc"))
	ui->comboBox_SimpleOutStrEncoder->addItem(
		ENCODER_STR("Hardware.NVENC.HEVC"),
		QString(SIMPLE_ENCODER_NVENC_HEVC));
	#endif
	if (ServiceSupportsEncoder(vcodecs, "h264_texture_amf"))
	ui->comboBox_SimpleOutStrEncoder->addItem(
		ENCODER_STR("Hardware.AMD.H264"),
		QString(SIMPLE_ENCODER_AMD));
	if (ServiceSupportsEncoder(vcodecs, "av1_texture_amf"))
	ui->comboBox_SimpleOutStrEncoder->addItem(
		ENCODER_STR("Hardware.AMD.AV1"),
		QString(SIMPLE_ENCODER_AMD_AV1));
	#ifdef __APPLE__
	if (ServiceSupportsEncoder(
		vcodecs, "com.apple.videotoolbox.videoencoder.ave.avc")
	#ifndef __aarch64__
		&& os_get_emulation_status() == true
	#endif
		) {
		if (__builtin_available(macOS 13.0, *)) {
			ui->comboBox_SimpleOutStrEncoder->addItem(
				ENCODER_STR("Hardware.Apple.H264"),
				QString(SIMPLE_ENCODER_APPLE_H264));
		}
	}
	#ifdef ENABLE_HEVC
	if (ServiceSupportsEncoder(
		vcodecs, "com.apple.videotoolbox.videoencoder.ave.hevc")
	#ifndef __aarch64__
		&& os_get_emulation_status() == true
	#endif
		) {
		if (__builtin_available(macOS 13.0, *)) {
			ui->comboBox_SimpleOutStrEncoder->addItem(
				ENCODER_STR("Hardware.Apple.HEVC"),
				QString(SIMPLE_ENCODER_APPLE_HEVC));
		}
	}
	#endif
	#endif
	#undef ENCODER_STR

	if (!lastAdvVideoEnc.isEmpty()) {
		int idx = ui->comboBox_AdvOutEncoder->findData(lastAdvVideoEnc);
		if (idx == -1) {
			lastAdvVideoEnc = AFSettingUtils::GetAdvFallback(lastAdvVideoEnc);
			ui->comboBox_AdvOutEncoder->setProperty("changed",
				QVariant(true));
			qslotVideoDataChanged();
		}

		idx = ui->comboBox_AdvOutEncoder->findData(lastAdvVideoEnc);
		s2.unblock();
		ui->comboBox_AdvOutEncoder->setCurrentIndex(idx);
	}

	if (!lastVideoEnc.isEmpty()) {
		int idx = ui->comboBox_SimpleOutStrEncoder->findData(lastVideoEnc);
		if (idx == -1) {
			lastVideoEnc = AFSettingUtils::GetSimpleFallback(lastVideoEnc);
			ui->comboBox_SimpleOutStrEncoder->setProperty("changed",
				QVariant(true));
			qslotVideoDataChanged();
		}

		idx = ui->comboBox_SimpleOutStrEncoder->findData(lastVideoEnc);
		s1.unblock();
		ui->comboBox_SimpleOutStrEncoder->setCurrentIndex(idx);
	}
}

static const double vals[] = { 1.0, 1.25, (1.0 / 0.75), 1.5,  (1.0 / 0.6), 1.75,
				  2.0, 2.25, 2.5,          2.75, 3.0 };

static const size_t numVals = sizeof(vals) / sizeof(double);

QComboBox* AFQVideoSettingAreaWidget::GetOutResolutionComboBox() {
	return ui->comboBox_AdvOutRescale;
}

void AFQVideoSettingAreaWidget::ChangeSettingModeToSimple()
{
	if (m_bIsAdvancedMode)
	{
		m_bIsAdvancedMode = false;
		_SetSettingModeUi(m_bIsAdvancedMode);
	}
}

void AFQVideoSettingAreaWidget::ChangeSettingModeToAdvanced()
{
	if (!m_bIsAdvancedMode)
	{
		m_bIsAdvancedMode = true;
		_SetSettingModeUi(m_bIsAdvancedMode);
	}
}

void AFQVideoSettingAreaWidget::ResetDownscales(uint32_t cx, uint32_t cy, bool ignoreAllSignals)
{
	QString advRescale;
	QString oldOutputRes;
	std::string bestScale;
	int bestPixelDiff = 0x7FFFFFFF;
	uint32_t out_cx = m_nOutputCX;
	uint32_t out_cy = m_nOutputCY;

	advRescale = ui->comboBox_AdvOutRescale->lineEdit()->text();

	bool lockedOutputRes = !ui->comboBox_OutputResolution->isEditable();

	if (!lockedOutputRes) {
		ui->comboBox_OutputResolution->blockSignals(true);
		ui->comboBox_OutputResolution->clear();
	}
	if (ignoreAllSignals) {
		ui->comboBox_AdvOutRescale->blockSignals(true);
	}

	ui->comboBox_AdvOutRescale->clear();

	if (!out_cx || !out_cy) {
		out_cx = cx;
		out_cy = cy;
		oldOutputRes = ui->comboBox_BaseResolution->lineEdit()->text();
	}
	else {
		oldOutputRes =
			QString::number(out_cx) + "x" + QString::number(out_cy);
	}

	for (size_t idx = 0; idx < numVals; idx++) {
		uint32_t downscaleCX = uint32_t(double(cx) / vals[idx]);
		uint32_t downscaleCY = uint32_t(double(cy) / vals[idx]);
		uint32_t outDownscaleCX = uint32_t(double(out_cx) / vals[idx]);
		uint32_t outDownscaleCY = uint32_t(double(out_cy) / vals[idx]);

		downscaleCX &= 0xFFFFFFFC;
		downscaleCY &= 0xFFFFFFFE;
		outDownscaleCX &= 0xFFFFFFFE;
		outDownscaleCY &= 0xFFFFFFFE;

		std::string res = _ResString(downscaleCX, downscaleCY);
		std::string outRes = _ResString(outDownscaleCX, outDownscaleCY);
		if (!lockedOutputRes)
			ui->comboBox_OutputResolution->addItem(res.c_str());
		ui->comboBox_AdvOutRescale->addItem(outRes.c_str());
		
		int newPixelCount = int(downscaleCX * downscaleCY);
		int oldPixelCount = int(out_cx * out_cy);
		int diff = abs(newPixelCount - oldPixelCount);

		if (diff < bestPixelDiff) {
			bestScale = res;
			bestPixelDiff = diff;
		}
	}

	std::string res = _ResString(cx, cy);

	if (!lockedOutputRes) {
		float baseAspect = float(cx) / float(cy);
		float outputAspect = float(out_cx) / float(out_cy);
		bool closeAspect = close_float(baseAspect, outputAspect, 0.01f);

		if (closeAspect) {
			ui->comboBox_OutputResolution->lineEdit()->setText(oldOutputRes);
			qslotOutputResolutionEditTextChanged(oldOutputRes);
		}
		else {
			ui->comboBox_OutputResolution->lineEdit()->setText(
				bestScale.c_str());
			qslotOutputResolutionEditTextChanged(bestScale.c_str());
		}

		ui->comboBox_OutputResolution->blockSignals(false);

		if (!closeAspect) {
			ui->comboBox_OutputResolution->setProperty("changed",
				QVariant(true));
			m_bVideoDataChanged = true;
		}
	}

	if (advRescale.isEmpty())
		advRescale = res.c_str();

	ui->comboBox_AdvOutRescale->lineEdit()->setText(advRescale);

	if (ignoreAllSignals) {
		ui->comboBox_AdvOutRescale->blockSignals(false);
	}
}

AFQPropertiesView* AFQVideoSettingAreaWidget::_CreateEncoderPropertyView(const char* encoder, const char* path, bool changed)
{
	OBSDataAutoRelease settings = obs_encoder_defaults(encoder);
	AFQPropertiesView* view;

	if (path) {
		char encoderJsonPath[512];
		int ret = AFConfigManager::GetSingletonInstance().GetProfilePath(encoderJsonPath,
			sizeof(encoderJsonPath), path);
		if (ret > 0) {
			obs_data_t* data = obs_data_create_from_json_file_safe(
				encoderJsonPath, "bak");
			obs_data_apply(settings, data);
			obs_data_release(data);
		}
	}

	view = new AFQPropertiesView(
		settings.Get(), encoder, 
		(PropertiesReloadCallback)obs_get_encoder_properties,
		ui->label_AdvOutEncoder->minimumWidth());

	view->SetScrollDisabled();
	view->SetPropertiesViewWidth(ui->frame_VideoEncoder->width());
	view->SetLayoutMargin(0, 0, 0, 0);
	view->SetFormSpacing(ui->formLayout_VideoEncoder->horizontalSpacing(), ui->formLayout_VideoEncoder->verticalSpacing());
	
	view->SetLabelFixedSize(ui->label_AdvOutEncoder->minimumSize());
	view->SetComboBoxFixedSize(ui->comboBox_AdvOutEncoder->maximumSize());
	view->SetLineEditFixedSize(ui->comboBox_AdvOutEncoder->maximumSize());
	view->SetSpinBoxFixedSize(ui->comboBox_AdvOutEncoder->maximumSize());
	view->SetCheckBoxFixedHeight(18);

	view->setFrameShape(QFrame::NoFrame);
	view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	view->setProperty("changed", QVariant(changed));
	view->ReloadProperties();
	
	QObject::connect(view, &AFQPropertiesView::Changed, this, &AFQVideoSettingAreaWidget::qslotVideoDataChanged);

	return view;
}

bool AFQVideoSettingAreaWidget::_ConvertResText(const char* res, uint32_t& cx, uint32_t& cy)
{
	BaseLexer lex;
	base_token token;

	lexer_start(lex, res);

	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != BASETOKEN_DIGIT)
		return false;

	cx = std::stoul(token.text.array);

	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (strref_cmpi(&token.text, "x") != 0)
		return false;

	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != BASETOKEN_DIGIT)
		return false;

	cy = std::stoul(token.text.array);

	if (lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;

	if (_ResTooHigh(cx, cy) || _ResTooLow(cx, cy)) {
		cx = cy = 0;
		return false;
	}

	return true;
}

bool AFQVideoSettingAreaWidget::_ResTooHigh(uint32_t cx, uint32_t cy)
{
	return cx > 16384 || cy > 16384;
}

bool AFQVideoSettingAreaWidget::_ResTooLow(uint32_t cx, uint32_t cy)
{
	return cx < 32 || cy < 32;
}

std::string AFQVideoSettingAreaWidget::_ResString(uint32_t cx, uint32_t cy)
{
	std::stringstream res;
	res << cx << "x" << cy;
	return res.str();
}
#pragma endregion private func