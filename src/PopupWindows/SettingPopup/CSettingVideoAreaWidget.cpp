#include "CSettingVideoAreaWidget.h"
#include "ui_setting-video-area.h"
#include <sstream>

#include <QScrollBar>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "Common/SettingsMiscDef.h"

#include "Application/CApplication.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Encoder/CEncoder.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/OBSOutput/COBSOutputContext.h"
#include "CoreModel/Profile/CProfile.h"

#define VIDEO_CHANGED   &AFQVideoSettingAreaWidget::qslotVideoDataChanged
#define VIDEO_RES       &AFQVideoSettingAreaWidget::qslotVideoChangedResolution
#define VIDEO_RESTART   &AFQVideoSettingAreaWidget::qslotVideoChangedRestart

AFQVideoSettingAreaWidget::AFQVideoSettingAreaWidget(QWidget* parent) :
	ui(new Ui::AFQVideoSettingAreaWidget)
{
	ui->setupUi(this);

	_SetVideoSettingUi();
	_SetVideoSettingSignal();
}

AFQVideoSettingAreaWidget::~AFQVideoSettingAreaWidget()
{
	delete ui;
}

void AFQVideoSettingAreaWidget::qslotVideoDataChanged()
{
	if (!m_loading) 
	{
		m_videoDataChanged = true;
		sender()->setProperty("changed", QVariant(true));
		
		emit qsignalVideoDataChanged();
	}
}

void AFQVideoSettingAreaWidget::qslotVideoChangedResolution()
{
	if (!m_loading && _ValidResolutions()) 
	{
		m_videoDataChanged = true;
		sender()->setProperty("changed", QVariant(true));
		
		emit qsignalVideoDataChanged();
	}
}

void AFQVideoSettingAreaWidget::qslotVideoChangedRestart()
{
	if (!m_loading)
	{
		m_videoDataChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalVideoDataChanged();
	}
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


void AFQVideoSettingAreaWidget::qslotSettingModeCurrentIndexChanged(int idx)
{
	if (0 == idx) {			// Simple
		ChangeSettingModeToSimple();
		emit qsignalSimpleModeClicked();
	}
	else if (1 == idx) {	// Advanced
		ChangeSettingModeToAdvanced();
		emit qsignalAdvancedModeClicked();
	}	
}

void AFQVideoSettingAreaWidget::qslotAdvOutEncoderCurrentIndexChanged()
{
	QString encoder = AFSettingUtils::GetComboData(ui->comboBox_AdvOutEncoder);
	if (!m_loading) {

		if (m_pEncoderProps) {
			delete m_pEncoderProps;
			m_pEncoderProps = nullptr;
		}

		m_pEncoderProps = _CreateEncoderPropertyView(QT_TO_UTF8(encoder), "streamEncoder.json", true);
		m_pEncoderProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
		ui->verticalLayout_AdvOutEncoderProps->addWidget(m_pEncoderProps);

		connect(m_pEncoderProps, &OBSPropertiesView::Changed, this, &AFQVideoSettingAreaWidget::qslotStreamEncoderPropChanged);
		connect(m_pEncoderProps, &OBSPropertiesView::PropertiesRefreshed, this, &AFQVideoSettingAreaWidget::setAV1CodecProperties);
	}
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
		preset = m_curQSVPreset;

	}
	else if (encoder == SIMPLE_ENCODER_NVENC ||
		encoder == SIMPLE_ENCODER_NVENC_HEVC ||
		encoder == SIMPLE_ENCODER_NVENC_AV1) {

		const char* name = get_simple_output_encoder(QT_TO_UTF8(encoder));
		const bool isFFmpegEncoder = strncmp(name, "ffmpeg_", 7) == 0;
		obs_properties_t* props = obs_get_encoder_properties(name);

		obs_property_t* p = obs_properties_get(props, isFFmpegEncoder ? "preset2" : "preset");
		size_t num = obs_property_list_item_count(p);
		for (size_t i = 0; i < num; i++) {
			name = obs_property_list_item_name(p, i);
			const char* val = obs_property_list_item_string(p, i);

			ui->comboBox_SimpleOutPreset->addItem(QT_UTF8(name), val);
		}

		obs_properties_destroy(props);

		defaultPreset = "default";
		preset = m_curNVENCPreset;

	}
	else if (encoder == SIMPLE_ENCODER_AMD ||
		encoder == SIMPLE_ENCODER_AMD_HEVC) {
		ui->comboBox_SimpleOutPreset->addItem("Speed", "speed");
		ui->comboBox_SimpleOutPreset->addItem("Balanced", "balanced");
		ui->comboBox_SimpleOutPreset->addItem("Quality", "quality");

		defaultPreset = "balanced";
		preset = m_curAMDPreset;
	}
#ifdef __APPLE__
	else if (encoder == SIMPLE_ENCODER_APPLE_H264
#ifdef ENABLE_HEVC
		|| encoder == SIMPLE_ENCODER_APPLE_HEVC
#endif // ENABLE_HEVC
		)
	{
		ui->comboBox_SimpleOutPreset->setVisible(false);
		ui->label_SimpleOutPreset->setVisible(false);

	}
#endif // __APPLE__
	else if (encoder == SIMPLE_ENCODER_AMD_AV1) {
		ui->comboBox_SimpleOutPreset->addItem("Speed", "speed");
		ui->comboBox_SimpleOutPreset->addItem("Balanced", "balanced");
		ui->comboBox_SimpleOutPreset->addItem("Quality", "quality");
		ui->comboBox_SimpleOutPreset->addItem("High Quality", "highQuality");

		defaultPreset = "balanced";
		preset = m_curAMDAV1Preset;
	}
	else {

#define PRESET_STR(val) \
	QString(Str("Basic.Settings.Output.EncoderPreset." val)).arg(val)
		ui->comboBox_SimpleOutPreset->addItem(PRESET_STR("ultrafast"), "ultrafast");
		ui->comboBox_SimpleOutPreset->addItem("superfast", "superfast");
		ui->comboBox_SimpleOutPreset->addItem(PRESET_STR("veryfast"), "veryfast");
		ui->comboBox_SimpleOutPreset->addItem("faster", "faster");
		ui->comboBox_SimpleOutPreset->addItem(PRESET_STR("fast"), "fast");
#undef PRESET_STR

		/* Users might have previously selected a preset which is no
		 * longer available in simple mode. Make sure we don't mess
		 * with their setups without them knowing. */
		if (ui->comboBox_SimpleOutPreset->findData(m_curPreset) == -1) {
			ui->comboBox_SimpleOutPreset->addItem(m_curPreset, m_curPreset);
			QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->comboBox_SimpleOutPreset->model());
			QStandardItem* item = model->item(model->rowCount() - 1);
			item->setEnabled(false);
		}

		defaultPreset = "veryfast";
		preset = m_curPreset;
	}

	int idx = ui->comboBox_SimpleOutPreset->findData(QVariant(preset));
	if (idx == -1)
		idx = ui->comboBox_SimpleOutPreset->findData(QVariant(defaultPreset));

	ui->comboBox_SimpleOutPreset->setCurrentIndex(idx);
}

void AFQVideoSettingAreaWidget::qslotBaseResolutionCurrentIndexChanged()
{
	QString baseResolution = ui->comboBox_BaseResolution->currentText();
	qslotBaseResolutionEditTextChanged(baseResolution);
}

void AFQVideoSettingAreaWidget::qslotBaseResolutionEditTextChanged(const QString& text)
{
	if (!m_loading && _ValidResolutions()) 
	{
		QString baseResolution = text;
		uint32_t cx, cy;

		convertResText(QT_TO_UTF8(baseResolution), cx, cy);

		std::tuple<int, int> aspect = AFEncoderUtil::AspectRatio(cx, cy);

		_ResetDownscales(cx, cy);

		ui->comboBox_OutputResolution->setCurrentText(text);

		emit qsignalBaseResolutionChanged(cx, cy);
	}
}

void AFQVideoSettingAreaWidget::qslotOutputResolutionEditTextChanged(const QString& text)
{
	if (!m_loading) 
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
	if (!m_loading) {
		bool disable = ui->checkBox_DisableOSXVSync->isChecked();
		ui->checkBox_ResetOSXVSync->setEnabled(disable);
	}
#endif // __APPLE__
}

void AFQVideoSettingAreaWidget::prepareEncoderSetting()
{
	QString encoder = (m_isAdvancedMode ? GetAdvVideoEncoder() : GetSimpleVideoEncoder());
	bool isAV1Codec = AFEncoderUtil::isAV1Codec(encoder.toStdString().c_str());
	//
	int count = ui->comboBox_BaseResolution->count();
	QString curResolution = ui->comboBox_BaseResolution->currentText();
	uint32_t cx = 0, cy = 0;
	convertResText(QT_TO_UTF8(curResolution), cx, cy);
	bool is1440pResolution = (cx != av1Resolution.width || cy != av1Resolution.height ? false : true);
	//
	bool loadResolution = false;
	bool changed = false;
	QString resolution;
	if(isAV1Codec) {
		if(1 != count)
			loadResolution = true;
		//
		if(!is1440pResolution) {
			resolution = AFSettingUtils::ResString(av1Resolution.width, av1Resolution.height).c_str();
			changed = true;
		}
	} else {
		if(1 == count)
			loadResolution = true;
		//
		bool allowed1440p = AUTH_CONTEXT.GetSoopBroadInfo()->Allow1440P();
		if(!allowed1440p && is1440pResolution) {
			resolution = AFSettingUtils::ResString(defResolution.width, defResolution.height).c_str();
			changed = true;
		}
	}
	//
	if(changed) {
		ui->comboBox_BaseResolution->setCurrentText(resolution.toStdString().c_str());
		SaveVideoSettings();
	}
	if(loadResolution) {
		ui->comboBox_BaseResolution->blockSignals(true);
		ui->comboBox_SimpleOutputVBitrate->blockSignals(true);
		//
		_LoadResolutionLists();
		// bitrate
		uint32_t bitrate = config_get_uint(ACTIVECONFIG, "SimpleOutput", "VBitrate");
		std::string strBitrate = std::to_string(bitrate) + " Kbps";
		AFSettingUtils::SetComboByName(ui->comboBox_SimpleOutputVBitrate, strBitrate.c_str());
		//
		ui->comboBox_BaseResolution->blockSignals(false);
		ui->comboBox_SimpleOutputVBitrate->blockSignals(false);
	}
	// keyframe
	if(isAV1Codec) {
		OBSData settings = AFProfileUtil::GetDataFromJsonFile("streamEncoder.json");
		obs_data_set_int(settings, "keyint_sec", 2);
		AFProfileUtil::SetDataToJsonFile("streamEncoder.json", settings);
	}
}

void AFQVideoSettingAreaWidget::setAV1CodecProperties()
{
	if(m_isAdvancedMode) {
		if(!m_pEncoderProps)
			return;
		//
		QFormLayout* formLayout = m_pEncoderProps->getFormLayout();
		if(!formLayout)
			return;

		QLayout* layout = formLayout->findChild<QHBoxLayout*>("layout_keyint_sec");
		if(!layout)
			return;

		QSpinBox* keyintBox = nullptr;
		for(int i = 0; i < layout->count(); i++) {
			QLayoutItem* item = layout->itemAt(i);
			if(!item || !item->widget())
				continue;
			//
			if(item->widget()->objectName() == "keyint_sec") {
				keyintBox = qobject_cast<QSpinBox*>(item->widget());
			}
		}

		if(keyintBox) {
			QString encoder = GetAdvVideoEncoder();
			bool isAV1Codec = AFEncoderUtil::isAV1Codec(encoder.toStdString().c_str());
			bool changed = (keyintBox->isReadOnly() != isAV1Codec);
			if(!changed)
				return;
			//
			if(isAV1Codec) {
				keyintBox->setValue(2);
			}
			keyintBox->setReadOnly(isAV1Codec);
		}
	}
}

void AFQVideoSettingAreaWidget::LoadVideoSettings(bool reset)
{
	auto config = ACTIVECONFIG;
	//
	m_loading = true;

	int videoBitrate;
	const char* streamEnc;
	const char* preset;
	const char* videoColorFormat;
	const char* videoColorSpace;
	const char* videoColorRange;
	uint32_t sdrWhiteLevel;
	uint32_t hdrNominalPeakLevel;

	videoBitrate = config_get_uint(config, "SimpleOutput", "VBitrate");
	streamEnc = config_get_string(config, "SimpleOutput", "StreamEncoder");
	preset = config_get_string(config, "SimpleOutput", "Preset");
	videoColorFormat = config_get_string(config, "Video", "ColorFormat");
	videoColorSpace = config_get_string(config, "Video", "ColorSpace");
	videoColorRange = config_get_string(config, "Video", "ColorRange");
	sdrWhiteLevel = (uint32_t)config_get_uint(config, "Video", "SdrWhiteLevel");
	hdrNominalPeakLevel = (uint32_t)config_get_uint(config, "Video", "HdrNominalPeakLevel");

	bool rescale = config_get_bool(config, "AdvOut", "Rescale");
	const char* qsvPreset = config_get_string(config, "SimpleOutput", "QSVPreset");
	const char* nvPreset = config_get_string(config, "SimpleOutput", "NVENCPreset2");
	const char* amdPreset = config_get_string(config, "SimpleOutput", "AMDPreset");
	const char* amdAV1Preset = config_get_string(config, "SimpleOutput", "AMDAV1Preset");
	const char* rescaleRes = config_get_string(config, "AdvOut", "RescaleRes");

	m_curPreset = preset;
	m_curQSVPreset = qsvPreset;
	m_curNVENCPreset = nvPreset;
	m_curAMDPreset = amdPreset;
	m_curAMDAV1Preset = amdAV1Preset;

	// Rescale
	ui->checkBox_AdvOutUseRescale->setChecked(rescale);
	ui->comboBox_AdvOutRescale->setEnabled(rescale);
	ui->comboBox_AdvOutRescale->setCurrentText(rescaleRes);

	ui->comboBox_OutputResolution->setEnabled(false);


	_ResetEncoders();
	_LoadResolutionLists();
	_LoadFPSData();
	_LoadDownscaleFilters();
	_LoadAdvOutputStreamingEncoderProperties();
	_LoadRendererList();
	_LoadColorFormats();
	_LoadColorSpaces();
	_LoadColorRanges();

	// Bitrate
	std::string strBitrate = std::to_string(videoBitrate) + " Kbps";
	const char* charBitrate = strBitrate.c_str();

	AFSettingUtils::SetComboByName(ui->comboBox_SimpleOutputVBitrate, charBitrate);

	// Encoder
	AFSettingUtils::SetComboByValue(ui->comboBox_SimpleOutStrEncoder, streamEnc);
	qslotSimpleEncoderChanged();
	qslotUpdateSimpleRecordingEncoder();

	// Preset
	qslotSimpleStreamingEncoderChanged();
	//
	prepareEncoderSetting();

	// Video
	AFSettingUtils::SetComboByValue(ui->comboBox_ColorFormat, videoColorFormat);
	AFSettingUtils::SetComboByValue(ui->comboBox_ColorSpace, videoColorSpace);
	AFSettingUtils::SetComboByValue(ui->comboBox_ColorRange, videoColorRange);
	ui->spinBox_SdrWhiteLevel->setValue(sdrWhiteLevel);
	ui->spinBox_HdrNominalPeakLevel->setValue(hdrNominalPeakLevel);

	m_loading = false;

	const char* mode = config_get_string(config, "Output", "Mode");
	if (astrcmpi(mode, "Advanced") == 0)
		m_isAdvancedMode = true;
	else
		m_isAdvancedMode = false;
	
	if (reset) {
		if (m_isAdvancedMode)
			emit qsignalAdvancedModeClicked();
		else
			emit qsignalSimpleModeClicked();
	}

	ToggleOnStreaming(false);
	_SetSettingModeUi(m_isAdvancedMode);

	ui->comboBox_OutputResolution->lineEdit()->setReadOnly(true);
	ui->comboBox_AdvOutRescale->lineEdit()->setReadOnly(true);
}

void AFQVideoSettingAreaWidget::SaveVideoSettings()
{
	if (!m_videoDataChanged)
		return;

	auto config = ACTIVECONFIG;
	//
	QString baseResolution = ui->comboBox_BaseResolution->currentText();
	QString outputResolution = ui->comboBox_OutputResolution->currentText();
	int fpsType = ui->comboBox_FpsType->currentIndex();
	uint32_t cx = 0, cy = 0;

	if (AFSettingUtils::WidgetChanged(ui->comboBox_BaseResolution) &&
		convertResText(QT_TO_UTF8(baseResolution), cx, cy)) {
		config_set_uint(config, "Video", "BaseCX", cx);
		config_set_uint(config, "Video", "BaseCY", cy);
	}

	if (AFSettingUtils::WidgetChanged(ui->comboBox_OutputResolution) &&
		convertResText(QT_TO_UTF8(outputResolution), cx, cy)) {
		config_set_uint(config, "Video", "OutputCX", cx);
		config_set_uint(config, "Video", "OutputCY", cy);
	}

	if (AFSettingUtils::WidgetChanged(ui->comboBox_FpsType))
		config_set_uint(config, "Video", "FPSType", fpsType);

	AFSettingUtils::SaveCombo(ui->comboBox_FpsCommon, "Video", "FPSCommon");
	AFSettingUtils::SaveDoubleSpinBox(ui->doubleSpinBox_FpsInteger, "Video", "FPSInt");
	AFSettingUtils::SaveComboData(ui->comboBox_DownscaleFilter, "Video", "ScaleType");

	// Bitrate
	QString strBitrate = ui->comboBox_SimpleOutputVBitrate->currentText();
	strBitrate.replace(" Kbps", "");
	config_set_uint(config, "SimpleOutput", "VBitrate", strBitrate.toInt());

	// Encoder
	AFSettingUtils::SaveComboData(ui->comboBox_SimpleOutStrEncoder, "SimpleOutput", "StreamEncoder");

	// Preset
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
#endif // ENABLE_HEVC
	else if (encoder == SIMPLE_ENCODER_AMD)
		presetType = "AMDPreset";
	else if (encoder == SIMPLE_ENCODER_AMD_AV1)
		presetType = "AMDAV1Preset";
#ifdef __APPLE__
	else if (encoder == SIMPLE_ENCODER_APPLE_H264
#ifdef ENABLE_HEVC
		|| encoder == SIMPLE_ENCODER_APPLE_HEVC
#endif // ENABLE_HEVC
		)
		/* The Apple encoders don't have presets like the other encoders
		 do. This only exists to make sure that the x264 preset doesn't
		 get overwritten with empty data. */
		presetType = "ApplePreset";
#endif // __APPLE__
	else
		presetType = "Preset";

	AFSettingUtils::SaveComboData(ui->comboBox_SimpleOutPreset, "SimpleOutput", presetType);

	// Encoder
	AFSettingUtils::SaveComboData(ui->comboBox_AdvOutEncoder, "AdvOut", "Encoder");
	AFSettingUtils::WriteJsonData(m_pEncoderProps, "streamEncoder.json");

	AFSettingUtils::SaveCheckBox(ui->checkBox_AdvOutUseRescale, "AdvOut", "Rescale");
	AFSettingUtils::SaveCombo(ui->comboBox_AdvOutRescale, "AdvOut", "RescaleRes");

	// Video
#ifdef _WIN32
	if (AFSettingUtils::WidgetChanged(ui->comboBox_Renderer))
		config_set_string(APPCONFIG, "Video", "Renderer", QT_TO_UTF8(ui->comboBox_Renderer->currentText()));
#elif __APPLE__
	if (AFSettingUtils::WidgetChanged(ui->checkBox_DisableOSXVSync)) {
		bool disable = ui->checkBox_DisableOSXVSync->isChecked();
		config_set_bool(APPCONFIG, "Video", "DisableOSXVSync", disable);
		EnableOSXVSync(!disable);
	}
	if (AFSettingUtils::WidgetChanged(ui->checkBox_ResetOSXVSync))
		config_set_bool(APPCONFIG, "Video", "ResetOSXVSyncOnExit", ui->checkBox_ResetOSXVSync->isChecked());
#endif // _WIN32

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

void AFQVideoSettingAreaWidget::SetVideoDataChanged(bool changed)
{
	if (m_pEncoderProps)
		m_pEncoderProps->setProperty("changed", QVariant(changed));
	m_videoDataChanged = changed;
}

int AFQVideoSettingAreaWidget::VideoBitrate()
{
	QString value = ui->comboBox_SimpleOutputVBitrate->currentText();
	QStringList list = value.split(" ");
	return list[0].toInt();
}

int AFQVideoSettingAreaWidget::VideoAdvBitrate()
{
	if (m_pEncoderProps == nullptr)
		return 0;

	OBSData settings = m_pEncoderProps->GetSettings();
	int vBitrate = (int)obs_data_get_int(settings, "bitrate");

	return vBitrate;
}

const char* AFQVideoSettingAreaWidget::VideoAdvRateControl()
{
	if (m_pEncoderProps == nullptr)
		return "";

	OBSData settings = m_pEncoderProps->GetSettings();
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

void AFQVideoSettingAreaWidget::showEvent(QShowEvent* event)
{
	ui->verticalLayout_ScrollArea->update();
	QSize x = ui->scrollAreaWidgetContents->size();
	QWidget::showEvent(event);
}

void AFQVideoSettingAreaWidget::_ClearVideoSettingUi()
{
	ui->comboBox_BaseResolution->clear();
	ui->comboBox_OutputResolution->clear();
	ui->comboBox_DownscaleFilter->clear();
	ui->comboBox_SimpleOutStrEncoder->clear();
	ui->comboBox_SimpleOutPreset->clear();
	ui->comboBox_AdvOutEncoder->clear();
	ui->comboBox_AdvOutRescale->clear();

	if (m_pEncoderProps != nullptr) {
		delete m_pEncoderProps;
		m_pEncoderProps = nullptr;
	}

	QLayoutItem* item;
	while ((item = ui->verticalLayout_AdvOutEncoderProps->takeAt(0)) != NULL)
	{
		delete item->widget();
		delete item;
	}

#ifdef _WIN32
	ui->comboBox_Renderer->clear();
#endif // _WIN32
	ui->comboBox_ColorFormat->clear();
	ui->comboBox_ColorSpace->clear();
	ui->comboBox_ColorRange->clear();
}

void AFQVideoSettingAreaWidget::_ChangeLanguage()
{
	QList<QLabel*> labelList = findChildren<QLabel*>();
	QList<QComboBox*> comboboxList = findChildren<QComboBox*>();

	foreach(QLabel * label, labelList)
	{
		label->setText(QTStr(label->text().toUtf8().constData()));
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

			combobox->setItemText(i, QTStr(itemString.toUtf8().constData()));
		}
	}
}

void AFQVideoSettingAreaWidget::_SetVideoSettingSignal()
{
	AFSettingUtils::HookWidget(ui->comboBox_BaseResolution, this, COMBO_CHANGED, VIDEO_RES);
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
#endif // _WIN32

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
	
	connect(ui->comboBox_BaseResolution, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotBaseResolutionCurrentIndexChanged);
	connect(ui->comboBox_OutputResolution, &QComboBox::editTextChanged, this, &AFQVideoSettingAreaWidget::qslotOutputResolutionEditTextChanged);
	
	connect(ui->comboBox_SettingMode, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotSettingModeCurrentIndexChanged);

	connect(ui->comboBox_SimpleOutputVBitrate, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotSimpleBitrateChanged);
	connect(ui->comboBox_SimpleOutStrEncoder, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::qslotSimpleEncoderChanged);
	connect(ui->checkBox_AdvOutUseRescale, &QCheckBox::toggled, ui->comboBox_AdvOutRescale, &QComboBox::setEnabled);

	// for av1
	connect(ui->comboBox_SimpleOutStrEncoder, &QComboBox::activated, this, &AFQVideoSettingAreaWidget::prepareEncoderSetting);
	connect(ui->comboBox_AdvOutEncoder, &QComboBox::activated, this, &AFQVideoSettingAreaWidget::prepareEncoderSetting);
	// switch mode
	connect(ui->comboBox_SettingMode, &QComboBox::currentIndexChanged, this, &AFQVideoSettingAreaWidget::prepareEncoderSetting);

#ifdef __APPLE__
	connect(ui->checkBox_DisableOSXVSync, &QCheckBox::toggled, this, &AFQVideoSettingAreaWidget::qslotDisableOSXVSyncClicked);
#endif // __APPLE__
}

void AFQVideoSettingAreaWidget::_SetVideoSettingUi()
{
	if (MAINFRAME->IsSmallResolution())
	{
		ui->scrollArea->setFixedWidth(796);
		ui->scrollAreaWidgetContents->setFixedWidth(796);
	}

	QRegularExpression rx("\\d{1,5}x\\d{1,5}");
	QValidator* validator = new QRegularExpressionValidator(rx, this);
	ui->comboBox_OutputResolution->lineEdit()->setValidator(validator);
	ui->comboBox_AdvOutRescale->lineEdit()->setValidator(validator);

	ui->label_BaseResolution->setText(QTStr("Basic.Settings.Video.Resolution"));

	ui->checkBox_AdvOutUseRescale->setChecked(false);
	ui->frame_EncorderRescale->hide();

	if (ui->formLayout_VideoEncoder->rowCount() > 2) {
		ui->formLayout_VideoEncoder->removeRow(2);
	}

	ui->stackedWidget_FpsType->setCurrentIndex(1);
	ui->comboBox_BaseResolution->setEditable(false);

	ui->label_OutputResolution->setVisible(false);
	ui->comboBox_OutputResolution->setVisible(false);

	ui->label_DownscaleFilter->setVisible(false);
	ui->comboBox_DownscaleFilter->setVisible(false);
}

void AFQVideoSettingAreaWidget::_SetSettingModeUi(const bool isAdvanced)
{
	// Advanced
	if (isAdvanced)
	{
		config_set_int(ACTIVECONFIG, "AdvOut", "FFVBitrate", VideoAdvBitrate());

		if (m_pEncoderProps) {
			if (m_pEncoderProps->getAdvancedBitrateComboBox()) {
				std::string strBitrate = std::to_string(VideoAdvBitrate()) + " Kbps";
				const char* charBitrate = strBitrate.c_str();
				AFSettingUtils::SetComboByName(m_pEncoderProps->getAdvancedBitrateComboBox(), charBitrate);
			}
		}
		ui->comboBox_SettingMode->setCurrentIndex(1);
	}
	// Simple
	else
	{
		std::string strBitrate = std::to_string(VideoBitrate()) + " Kbps";
		const char* charBitrate = strBitrate.c_str();
		AFSettingUtils::SetComboByName(ui->comboBox_SimpleOutputVBitrate, charBitrate);

		ui->comboBox_SettingMode->setCurrentIndex(0);
	}

	ui->label_SimpleOutputVBitrate->setVisible(!isAdvanced);
	ui->comboBox_SimpleOutputVBitrate->setVisible(!isAdvanced);

	ui->label_SimpleOutStrEncoder->setVisible(!isAdvanced);
	ui->comboBox_SimpleOutStrEncoder->setVisible(!isAdvanced);

	ui->label_SimpleOutPreset->setVisible(!isAdvanced);
	ui->comboBox_SimpleOutPreset->setVisible(!isAdvanced);

	ui->frame_VideoEncoder->setVisible(isAdvanced);
	ui->frame_PropertiesFrame->setVisible(isAdvanced);
	ui->frame_VideoRenderer->setVisible(isAdvanced);
}

void AFQVideoSettingAreaWidget::_LoadResolutionLists()
{
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();

	auto config = ACTIVECONFIG;
	//
	uint32_t cx = config_get_uint(config, "Video", "BaseCX");
	uint32_t cy = config_get_uint(config, "Video", "BaseCY");
	uint32_t out_cx = config_get_uint(config, "Video", "OutputCX");
	uint32_t out_cy = config_get_uint(config, "Video", "OutputCY");

	ui->comboBox_BaseResolution->clear();
	ui->comboBox_SimpleOutputVBitrate->clear();

	bool allowed1440p = broadInfo->Allow1440P();
	bool allowedAV1 = broadInfo->AllowAV1();
	bool isAV1Codec = false;
	if(allowedAV1) {
		QString encoder = (m_isAdvancedMode ? GetAdvVideoEncoder() : GetSimpleVideoEncoder());
		isAV1Codec = AFEncoderUtil::isAV1Codec(encoder.toStdString().c_str());
	}
	//
	auto addResolution = [this](int cx, int cy) {
		QString res = AFSettingUtils::ResString(cx, cy).c_str();

		if (ui->comboBox_BaseResolution->findText(res) == -1)
			ui->comboBox_BaseResolution->addItem(res);
	};
	for(auto resolution : resolutionList)
	{
		if(isAV1Codec) {
			if(!resolution.is1440p) continue;
		} else
			if(resolution.is1440p && !allowed1440p)
				continue;
		//
		addResolution(resolution.width, resolution.height);
	}
	//
	auto addBitrate = [this](int bitrate) {
		QString tmp = QString("%1 Kbps").arg(bitrate);

		if(ui->comboBox_SimpleOutputVBitrate->findText(tmp) == -1)
			ui->comboBox_SimpleOutputVBitrate->addItem(tmp);
	};
	for(auto bitrate : bitrateList)
	{
		if(isAV1Codec) {
			if(!bitrate.is1440p) continue;
		} else
			if(bitrate.is1440p && !allowed1440p)
				continue;
		//
		addBitrate(bitrate.bitrate);
	}

	std::string outputResString = AFSettingUtils::ResString(out_cx, out_cy);
	ui->comboBox_BaseResolution->setCurrentText(AFSettingUtils::ResString(cx, cy).c_str());

	_RecalcOutputResPixels(outputResString.c_str());
	_ResetDownscales(cx, cy);

	emit qsignalBaseResolutionChanged(cx, cy);

	ui->comboBox_OutputResolution->lineEdit()->setText(outputResString.c_str());
}

void AFQVideoSettingAreaWidget::_LoadRendererList()
{
#ifdef _WIN32
	const char* renderer = config_get_string(APPCONFIG, "Video", "Renderer");

	ui->comboBox_Renderer->addItem(QT_UTF8("Direct3D 11"));

	if (ARGOPTION.GetAllowOpenGL() || strcmp(renderer, "OpenGL") == 0)
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
	bool disableOSXVSync = config_get_bool(APPCONFIG, "Video", "DisableOSXVSync");
	bool resetOSXVSync = config_get_bool(APPCONFIG, "Video", "ResetOSXVSyncOnExit");
	ui->checkBox_DisableOSXVSync->setChecked(disableOSXVSync);
	ui->checkBox_ResetOSXVSync->setChecked(resetOSXVSync);
	ui->checkBox_ResetOSXVSync->setEnabled(disableOSXVSync);

	delete ui->comboBox_Renderer;
	delete ui->label_Renderer;
	ui->label_Renderer = nullptr;
	ui->comboBox_Renderer = nullptr;
#endif // _WIN32
}

#define CF_NV12_STR Str("Basic.Settings.Advanced.Video.ColorFormat.NV12")
#define CF_I420_STR Str("Basic.Settings.Advanced.Video.ColorFormat.I420")
#define CF_I444_STR Str("Basic.Settings.Advanced.Video.ColorFormat.I444")
#define CF_P010_STR Str("Basic.Settings.Advanced.Video.ColorFormat.P010")
#define CF_I010_STR Str("Basic.Settings.Advanced.Video.ColorFormat.I010")
#define CF_P216_STR Str("Basic.Settings.Advanced.Video.ColorFormat.P216")
#define CF_P416_STR Str("Basic.Settings.Advanced.Video.ColorFormat.P416")
#define CF_BGRA_STR Str("Basic.Settings.Advanced.Video.ColorFormat.BGRA")
void AFQVideoSettingAreaWidget::_LoadColorFormats()
{
	ui->comboBox_ColorFormat->addItem(CF_NV12_STR, "NV12");
	ui->comboBox_ColorFormat->addItem(CF_I420_STR, "I420");
	ui->comboBox_ColorFormat->addItem(CF_I444_STR, "I444");
	ui->comboBox_ColorFormat->addItem(CF_P010_STR, "P010");
	ui->comboBox_ColorFormat->addItem(CF_I010_STR, "I010");
	ui->comboBox_ColorFormat->addItem(CF_P216_STR, "P216");
	ui->comboBox_ColorFormat->addItem(CF_P416_STR, "P416");
	ui->comboBox_ColorFormat->addItem(CF_BGRA_STR, "RGB"); // Avoid config break
}

#define CS_SRGB_STR		Str("Basic.Settings.Advanced.Video.ColorSpace.sRGB")
#define CS_709_STR		Str("Basic.Settings.Advanced.Video.ColorSpace.709")
#define CS_601_STR		Str("Basic.Settings.Advanced.Video.ColorSpace.601")
#define CS_2100PQ_STR	Str("Basic.Settings.Advanced.Video.ColorSpace.2100PQ")
#define CS_2100HLG_STR	Str("Basic.Settings.Advanced.Video.ColorSpace.2100HLG")
void AFQVideoSettingAreaWidget::_LoadColorSpaces()
{
	ui->comboBox_ColorSpace->addItem(CS_SRGB_STR, "sRGB");
	ui->comboBox_ColorSpace->addItem(CS_709_STR, "709");
	ui->comboBox_ColorSpace->addItem(CS_601_STR, "601");
	ui->comboBox_ColorSpace->addItem(CS_2100PQ_STR, "2100PQ");
	ui->comboBox_ColorSpace->addItem(CS_2100HLG_STR, "2100HLG");
}

#define CS_PARTIAL_STR	Str("Basic.Settings.Advanced.Video.ColorRange.Partial")
#define CS_FULL_STR		Str("Basic.Settings.Advanced.Video.ColorRange.Full")
void AFQVideoSettingAreaWidget::_LoadColorRanges()
{
	ui->comboBox_ColorRange->addItem(CS_PARTIAL_STR, "Partial");
	ui->comboBox_ColorRange->addItem(CS_FULL_STR, "Full");
}

void AFQVideoSettingAreaWidget::_LoadFPSData()
{
	_LoadFPSCommon();
	_LoadFPSInteger();

	//uint32_t fpsType = config_get_uint(config, "Video", "FPSType");

	//if (fpsType > 2)
	//	fpsType = 0;

	ui->comboBox_FpsType->setCurrentIndex(0);
	ui->stackedWidget_FpsPage->setCurrentIndex(0);
}

void AFQVideoSettingAreaWidget::_LoadFPSCommon()
{
	const char* val = config_get_string(ACTIVECONFIG, "Video", "FPSCommon");
	
	int idx = ui->comboBox_FpsCommon->findText(val);
	if (idx == -1)
		idx = 4;
	
	ui->comboBox_FpsCommon->setCurrentIndex(idx);
}

void AFQVideoSettingAreaWidget::_LoadFPSInteger()
{
	uint32_t val = config_get_uint(ACTIVECONFIG, "Video", "FPSInt");

	ui->doubleSpinBox_FpsInteger->setValue(val);
}

void AFQVideoSettingAreaWidget::_LoadDownscaleFilters()
{
	QString downscaleFilter = ui->comboBox_DownscaleFilter->currentData().toString();

	if (downscaleFilter.isEmpty())
		downscaleFilter = config_get_string(ACTIVECONFIG, "Video", "ScaleType");

	ui->comboBox_DownscaleFilter->clear();

	if (ui->comboBox_BaseResolution->currentText() == ui->comboBox_OutputResolution->currentText()) 
	{
		ui->comboBox_DownscaleFilter->setEnabled(false);
		ui->comboBox_DownscaleFilter->addItem(QTStr("Basic.Settings.Video.DownscaleFilter.Unavailable"), downscaleFilter);
	}
	else 
	{
		ui->comboBox_DownscaleFilter->setEnabled(true);		
		ui->comboBox_DownscaleFilter->addItem(QTStr("Basic.Settings.Video.DownscaleFilter.Bilinear"), QT_UTF8("bilinear"));
		ui->comboBox_DownscaleFilter->addItem(QTStr("Basic.Settings.Video.DownscaleFilter.Area"), QT_UTF8("area"));

		ui->comboBox_DownscaleFilter->addItem(QTStr("Basic.Settings.Video.DownscaleFilter.Bicubic"), QT_UTF8("bicubic"));

		ui->comboBox_DownscaleFilter->addItem(QTStr("Basic.Settings.Video.DownscaleFilter.Lanczos"), QT_UTF8("lanczos"));

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
	const char* type = config_get_string(ACTIVECONFIG, "AdvOut", "Encoder");

	if(m_pEncoderProps) {
		delete m_pEncoderProps;
	}

	m_pEncoderProps = _CreateEncoderPropertyView(type, "streamEncoder.json");
	m_pEncoderProps->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	
	ui->verticalLayout_AdvOutEncoderProps->addWidget(m_pEncoderProps);

	connect(m_pEncoderProps, &OBSPropertiesView::Changed, this, &AFQVideoSettingAreaWidget::qslotStreamEncoderPropChanged);
	connect(m_pEncoderProps, &OBSPropertiesView::PropertiesRefreshed, this, &AFQVideoSettingAreaWidget::setAV1CodecProperties);
	qslotStreamEncoderPropChanged();

	if (!AFSettingUtils::SetComboByValue(ui->comboBox_AdvOutEncoder, type)) {
		uint32_t caps = obs_get_encoder_caps(type);
		if ((caps & ENCODER_HIDE_FLAGS) != 0) {
			QString encName = QT_UTF8(obs_encoder_get_display_name(type));
			if(caps & OBS_ENCODER_CAP_DEPRECATED)
				encName += " (" + QTStr("Deprecated") + ")";

			ui->comboBox_AdvOutEncoder->insertItem(0, encName, QT_UTF8(type));
			AFSettingUtils::SetComboByValue(ui->comboBox_AdvOutEncoder, type);
		}
	}

	emit qsignalCallUpdateStreamDelayEstimate();
}

bool AFQVideoSettingAreaWidget::_ValidResolutions()
{
	QString baseRes = ui->comboBox_BaseResolution->currentText();
	uint32_t cx, cy;

	if (!convertResText(QT_TO_UTF8(baseRes), cx, cy)) {
		return false;
	}

	bool lockedOutRes = !ui->comboBox_OutputResolution->isEditable();
	if (!lockedOutRes) {
		QString outRes = ui->comboBox_OutputResolution->lineEdit()->text();
		if (!convertResText(QT_TO_UTF8(outRes), cx, cy)) {
			return false;
		}
	}

	return true;
}

void AFQVideoSettingAreaWidget::_RecalcOutputResPixels(const char* resText)
{
	uint32_t newCX;
	uint32_t newCY;

	if (convertResText(resText, newCX, newCY) && newCX && newCY) {
		m_outputCX = newCX;
		m_outputCY = newCY;

		std::tuple<int, int> aspect = AFEncoderUtil::AspectRatio(m_outputCX, m_outputCY);
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
	//
	
	const char** vcodecs = obs_service_get_supported_video_codecs(service);
	const char* type;
	BPtr<char*> output_vcodecs;
	size_t idx = 0;

	if (!vcodecs /*|| IsCustomService()*/) {
		const char* output;

		obs_enum_output_types_with_protocol(QT_TO_UTF8(protocol), &output, AFEncoderUtil::return_first_id);
		output_vcodecs = strlist_split(obs_get_output_supported_video_codecs(output), ';', false);
		vcodecs = (const char**)output_vcodecs.Get();
	}

	QSignalBlocker s1(ui->comboBox_SimpleOutStrEncoder);
	QSignalBlocker s2(ui->comboBox_AdvOutEncoder);

	/* ------------------------------------------------- */
	/* clear encoder lists                               */

	ui->comboBox_SimpleOutStrEncoder->clear();
	ui->comboBox_AdvOutEncoder->clear();

	/* ------------------------------------------------- */
	/* load advanced stream/recording encoders           */

	while (obs_enum_encoder_types(idx++, &type)) {
		const char* name = obs_encoder_get_display_name(type);
		const char* codec = obs_get_encoder_codec(type);
		uint32_t caps = obs_get_encoder_caps(type);

#ifndef ENABLE_HEVC
		if(0 == strcmp(codec, "hevc"))
			continue;
#endif // ENABLE_HEVC

		QString qName = QT_UTF8(name);
		QString qType = QT_UTF8(type);
		
		if (obs_get_encoder_type(type) == OBS_ENCODER_VIDEO) {
			if ((caps & ENCODER_HIDE_FLAGS) != 0) {
				continue;
			}

			if (AFEncoderUtil::isAV1Codec(type)) {
				auto broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
				if (!broadInfo->AllowAV1())
					continue;
			}

			// Skip AV1 software codec
			if(0 == strcmp(name, "SVT-AV1")
#ifndef _DEBUG
			   || 0 == strcmp(name, "AOM AV1")
#endif // _DEBUG
			   ) {
				continue;
			}

			if (AFEncoderUtil::ServiceSupportsCodec(vcodecs, codec)) {
				ui->comboBox_AdvOutEncoder->addItem(qName, qType);
			}
		}
	}

	ui->comboBox_AdvOutEncoder->model()->sort(0);

	/* ------------------------------------------------- */
	/* load simple stream encoders                       */

	#define ENCODER_STR(str) QTStr("Basic.Settings.Output.Simple.Encoder." str)

	ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Software"), QString(SIMPLE_ENCODER_X264));
	#ifdef _WIN32
	if (AFEncoderUtil::ServiceSupportsEncoder(vcodecs, "obs_qsv11"))
	ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Hardware.QSV.H264"), QString(SIMPLE_ENCODER_QSV));

	if (AFEncoderUtil::ServiceSupportsEncoder(vcodecs, "obs_qsv11_av1"))
		ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Hardware.QSV.AV1"), QString(SIMPLE_ENCODER_QSV_AV1));
	#endif // _WIN32
	if (AFEncoderUtil::ServiceSupportsEncoder(vcodecs, "ffmpeg_nvenc"))
	ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Hardware.NVENC.H264"), QString(SIMPLE_ENCODER_NVENC));
	if (AFEncoderUtil::ServiceSupportsEncoder(vcodecs, "obs_nvenc_av1_tex"))
		ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Hardware.NVENC.AV1"), QString(SIMPLE_ENCODER_NVENC_AV1));
	#ifdef ENABLE_HEVC
	if (AFEncoderUtil::ServiceSupportsEncoder(vcodecs, "h265_texture_amf"))
	ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Hardware.AMD.HEVC"), QString(SIMPLE_ENCODER_AMD_HEVC));
	if (AFEncoderUtil::ServiceSupportsEncoder(vcodecs, "ffmpeg_hevc_nvenc"))
	ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Hardware.NVENC.HEVC"), QString(SIMPLE_ENCODER_NVENC_HEVC));
	#endif // ENABLE_HEVC
	if (AFEncoderUtil::ServiceSupportsEncoder(vcodecs, "h264_texture_amf"))
	ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Hardware.AMD.H264"), QString(SIMPLE_ENCODER_AMD));

	if (AFEncoderUtil::ServiceSupportsEncoder(vcodecs, "av1_texture_amf"))
		ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Hardware.AMD.AV1"), QString(SIMPLE_ENCODER_AMD_AV1));
	/* Preprocessor guard required for the macOS version check */
	#ifdef __APPLE__
	if (AFEncoderUtil::ServiceSupportsEncoder(vcodecs, "com.apple.videotoolbox.videoencoder.ave.avc")
	#ifndef __aarch64__
		&& os_get_emulation_status() == true
	#endif // __aarch64__
		) {
		if (__builtin_available(macOS 13.0, *)) {
			ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Hardware.Apple.H264"), QString(SIMPLE_ENCODER_APPLE_H264));
		}
	}
	#ifdef ENABLE_HEVC
	if (AFEncoderUtil::ServiceSupportsEncoder(vcodecs, "com.apple.videotoolbox.videoencoder.ave.hevc")
	#ifndef __aarch64__
		&& os_get_emulation_status() == true
	#endif // __aarch64__
		) {
		if (__builtin_available(macOS 13.0, *)) {
			ui->comboBox_SimpleOutStrEncoder->addItem(ENCODER_STR("Hardware.Apple.HEVC"), QString(SIMPLE_ENCODER_APPLE_HEVC));
		}
	}
	#endif // ENABLE_HEVC
	#endif // __APPLE__
	#undef ENCODER_STR

	/* ------------------------------------------------- */
	/* Find fallback encoders                            */

	if (!lastAdvVideoEnc.isEmpty()) {
		int findIdx = ui->comboBox_AdvOutEncoder->findData(lastAdvVideoEnc);
		if (findIdx == -1) {
			lastAdvVideoEnc = AFSettingUtils::get_adv_fallback(lastAdvVideoEnc);
			ui->comboBox_AdvOutEncoder->setProperty("changed", QVariant(true));
			qslotVideoDataChanged();
		}

		findIdx = ui->comboBox_AdvOutEncoder->findData(lastAdvVideoEnc);
		s2.unblock();
		ui->comboBox_AdvOutEncoder->setCurrentIndex(findIdx);
	}

	if (!lastVideoEnc.isEmpty()) {
		int findIdx = ui->comboBox_SimpleOutStrEncoder->findData(lastVideoEnc);
		if (findIdx == -1) {
			lastVideoEnc = AFSettingUtils::get_simple_fallback(lastVideoEnc);
			ui->comboBox_SimpleOutStrEncoder->setProperty("changed", QVariant(true));
			qslotVideoDataChanged();
		}

		findIdx = ui->comboBox_SimpleOutStrEncoder->findData(lastVideoEnc);
		s1.unblock();
		ui->comboBox_SimpleOutStrEncoder->setCurrentIndex(findIdx);
	}
}

void AFQVideoSettingAreaWidget::_ResetDownscales(uint32_t cx, uint32_t cy, bool ignoreAllSignals)
{
	QString advRescale;
	QString oldOutputRes;

	std::string bestScale;
	int bestPixelDiff = 0x7FFFFFFF;
	uint32_t out_cx = m_outputCX;
	uint32_t out_cy = m_outputCY;

	advRescale = ui->comboBox_AdvOutRescale->lineEdit()->text();

	bool lockedOutputRes = !ui->comboBox_OutputResolution->isEditable();

	if (!lockedOutputRes) {
		ui->comboBox_OutputResolution->blockSignals(true);
		ui->comboBox_OutputResolution->clear();
	}
	if (ignoreAllSignals) {
		ui->comboBox_AdvOutRescale->blockSignals(true);
		//ui->advOutRecRescale->blockSignals(true);
		//ui->advOutFFRescale->blockSignals(true);
	}

	ui->comboBox_AdvOutRescale->clear();
	//ui->advOutRecRescale->clear();
	//ui->advOutFFRescale->clear();

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

		std::string res = AFSettingUtils::ResString(downscaleCX, downscaleCY);
		std::string outRes = AFSettingUtils::ResString(outDownscaleCX, outDownscaleCY);
		if (!lockedOutputRes)
			ui->comboBox_OutputResolution->addItem(res.c_str());
		ui->comboBox_AdvOutRescale->addItem(outRes.c_str());

		/* always try to find the closest output resolution to the
		 * previously set output resolution */
		int newPixelCount = int(downscaleCX * downscaleCY);
		int oldPixelCount = int(out_cx * out_cy);
		int diff = abs(newPixelCount - oldPixelCount);

		if (diff < bestPixelDiff) {
			bestScale = res;
			bestPixelDiff = diff;
		}
	}

	std::string res = AFSettingUtils::ResString(cx, cy);

	if (!lockedOutputRes) {
		float baseAspect = float(cx) / float(cy);
		float outputAspect = float(out_cx) / float(out_cy);
		bool closeAspect = close_float(baseAspect, outputAspect, 0.01f);

		if (closeAspect) {
			ui->comboBox_OutputResolution->lineEdit()->setText(oldOutputRes);
			qslotOutputResolutionEditTextChanged(oldOutputRes);
		}
		else {
			ui->comboBox_OutputResolution->lineEdit()->setText(bestScale.c_str());
			qslotOutputResolutionEditTextChanged(bestScale.c_str());
		}

		ui->comboBox_OutputResolution->blockSignals(false);

		if (!closeAspect) {
			if (!m_loading) {
				ui->comboBox_OutputResolution->setProperty("changed", QVariant(true));
				m_videoDataChanged = true;
			}
		}
	}

	if (advRescale.isEmpty())
		advRescale = res.c_str();

	ui->comboBox_AdvOutRescale->lineEdit()->setText(advRescale);

	if (ignoreAllSignals) {
		ui->comboBox_AdvOutRescale->blockSignals(false);
	}
}

QComboBox* AFQVideoSettingAreaWidget::GetOutResolutionComboBox() {
	return ui->comboBox_AdvOutRescale;
}

void AFQVideoSettingAreaWidget::ChangeSettingModeToSimple()
{
	if (m_isAdvancedMode)
	{
		m_isAdvancedMode = false;
		_SetSettingModeUi(m_isAdvancedMode);
	}
}

void AFQVideoSettingAreaWidget::ChangeSettingModeToAdvanced()
{
	if (!m_isAdvancedMode)
	{
		m_isAdvancedMode = true;
		_SetSettingModeUi(m_isAdvancedMode);
	}
}

bool AFQVideoSettingAreaWidget::IsValidAspectRatios()
{
	QString baseRes = ui->comboBox_BaseResolution->currentText();
	uint32_t cx, cy;

	// BaseResolution
	if (!convertResText(QT_TO_UTF8(baseRes), cx, cy))
	{
		m_resolutionCheck = ResolutionCheck::Wrong_BaseResolution;
		return false;
	}

	if (!checkAspectRatio(cx, cy))
	{
		m_resolutionCheck = ResolutionCheck::Wrong_BaseResolution;
		return false;
	}
	//

	// OutResolution
	bool lockedOutRes = !ui->comboBox_OutputResolution->isEditable();
	if (!lockedOutRes) {
		QString outRes = ui->comboBox_OutputResolution->lineEdit()->text();
		if (!convertResText(QT_TO_UTF8(outRes), cx, cy)) {
			m_resolutionCheck = ResolutionCheck::Wrong_OutResolution;
			return false;
		}
	}

	if (!checkAspectRatio(cx, cy))
	{
		m_resolutionCheck = ResolutionCheck::Wrong_OutResolution;
		return false;
	}
	//

	m_resolutionCheck = ResolutionCheck::Ok;

	return true;
}

void AFQVideoSettingAreaWidget::HighLightResolution()
{
	if (m_resolutionCheck == ResolutionCheck::Ok)
		return;

	// Move ScrollBar To Top
	ui->scrollArea->verticalScrollBar()->setValue(0);

	// Focus Line Edit
	if (m_resolutionCheck == ResolutionCheck::Wrong_BaseResolution)
	{
		ui->comboBox_BaseResolution->lineEdit()->setFocus();
		ui->comboBox_BaseResolution->lineEdit()->selectAll();
	}
	else // ResolutionCheck::Wrong_OutResolution
	{
		ui->comboBox_OutputResolution->lineEdit()->setFocus();
		ui->comboBox_OutputResolution->lineEdit()->selectAll();
	}

	m_resolutionCheck = ResolutionCheck::Ok;
}

OBSPropertiesView* AFQVideoSettingAreaWidget::_CreateEncoderPropertyView(const char* encoder, const char* path, bool changed)
{
	OBSDataAutoRelease settings = obs_encoder_defaults(encoder);
	OBSPropertiesView* view;

	if (strcmp(encoder, "obs_qsv11_v2") == 0) {
		obs_data_set_int(settings, "bframes", 0);
	}
	else if (0 != strstr(encoder, "nvenc")) {
		obs_data_set_int(settings, "bf", 0);
	}
	obs_data_set_int(settings, "keyint_sec", 2);

	if (path) {
		char encoderJsonPath[512];
		int ret = GetProfilePath(encoderJsonPath, sizeof(encoderJsonPath), path);
		if (ret > 0) {
			obs_data_t* data = obs_data_create_from_json_file_safe(encoderJsonPath, "bak");
			obs_data_apply(settings, data);
			obs_data_release(data);
		}
	}

	view = new OBSPropertiesView(settings.Get(), encoder,
								 (PropertiesReloadCallback)obs_get_encoder_properties,
								 ui->label_AdvOutEncoder->minimumWidth(), true);

	view->setLayoutMargin(0, 0, 0, 0);
	view->setFormSpacing(0, 16);
	view->setCheckBoxFixedHeight(18);
	view->setFrameShape(QFrame::NoFrame);
	view->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	view->setProperty("changed", QVariant(changed));
	
	QObject::connect(view, &OBSPropertiesView::Changed, this, &AFQVideoSettingAreaWidget::qslotVideoDataChanged);

	return view;
}


#define ASPECT_RATIO_EPSILON 0.2
static std::vector<std::pair<int, int>> standardAspectRatios = {
	{16, 9},  // Common widescreen aspect ratio
	{4, 3},   // Legacy display aspect ratio
	{21, 9},  // Ultrawide display aspect ratio
	{1, 1},   // Square aspect ratio
	{3, 2},   // Common laptop and tablet aspect ratio
	{2, 1},   // Wide cinematic aspect ratio
	{16, 10}  // Common productivity monitor aspect ratio
};

static bool isNearlyEqual(double a, double b, double epsilon) {
	return std::abs(a - b) < epsilon;
}

bool AFQVideoSettingAreaWidget::checkAspectRatio(int width, int height)
{
	if (width < 1 || height < 1)
		return false;

	double aspectRatio_WH = static_cast<double>(width) / height;
	double aspectRatio_HW = static_cast<double>(height) / width;

	for (const auto& ratio : standardAspectRatios) {
		double standardRatio = static_cast<double>(ratio.first) / ratio.second;
		if (isNearlyEqual(aspectRatio_WH, standardRatio, ASPECT_RATIO_EPSILON) ||
			isNearlyEqual(aspectRatio_HW, standardRatio, ASPECT_RATIO_EPSILON)) {
			return true;
		}
	}
	return false;
}

bool AFQVideoSettingAreaWidget::convertResText(const char* res, uint32_t& cx, uint32_t& cy)
{
	BaseLexer lex;
	base_token token;

	lexer_start(lex, res);

	/* parse width */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != BASETOKEN_DIGIT)
		return false;

	cx = std::stoul(token.text.array);

	/* parse 'x' */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (strref_cmpi(&token.text, "x") != 0)
		return false;

	/* parse height */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != BASETOKEN_DIGIT)
		return false;

	cy = std::stoul(token.text.array);

	/* shouldn't be any more tokens after this */
	if (lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;

	if (resTooHigh(cx, cy) || resTooLow(cx, cy)) {
		cx = cy = 0;
		return false;
	}

	return true;
}

bool AFQVideoSettingAreaWidget::resTooHigh(uint32_t cx, uint32_t cy)
{
	return cx > 16384 || cy > 16384;
}

bool AFQVideoSettingAreaWidget::resTooLow(uint32_t cx, uint32_t cy)
{
	return cx < 32 || cy < 32;
}