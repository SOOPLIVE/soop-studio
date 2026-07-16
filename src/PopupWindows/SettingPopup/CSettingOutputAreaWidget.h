#pragma once

#include <QWidget>
#include "ffmpeg-utils.hpp"
#include "CSettingUtils.h"
#include "ui_setting-output-area.h"
#include "CStudioSettingDialog.h"
#include "properties-view.hpp"
#include "CoreModel/Locale/CLocaleTextManager.h"


typedef obs_properties_t* (*PropertiesReloadCallback)(void* obj);

#define ESTIMATE_STR "Basic.Settings.Output.ReplayBuffer.Estimate"
#define ESTIMATE_TOO_LARGE_STR "Basic.Settings.Output.ReplayBuffer.EstimateTooLarge"
#define ESTIMATE_UNKNOWN_STR "Basic.Settings.Output.ReplayBuffer.EstimateUnknown"


#define OUTPUTS_CHANGED &AFQOutputSettingAreaWidget::qslotOutputDataChanged

namespace Ui {
	class AFQOutputSettingAreaWidget;
}

class AFQOutputSettingAreaWidget : public QWidget
{
	Q_OBJECT

signals:
	void qsignalOutputDataChanged();
	void qsignalUpdateReplayBufferStream(bool state);
	void qsignalSimpleModeClicked();
	void qsignalAdvancedModeClicked();
	void qsignalSetAutoRemuxText(QString autoRemuxText);

public slots:
	void qslotSimpleRecordingEncoderChanged();
	void qslotSimpleReplayBufferChanged();
	void qslotUpdateStreamDelayEstimate();
	void qslotAdvReplayBufferChanged();
	int qslotSimpleOutGetSelectedAudioTracks();
	int qslotAdvOutGetSelectedAudioTracks();

private slots:
	void qslotSimpleOutputBrowseClicked();
	void qslotAdvOutRecPathBrowseClicked();
	void qslotAdvOutFFPathBrowseClicked();
	void qslotOutputDataChanged();
	void qslotOutputTypeChanged(int idx);
	void qslotAdvOutRecCheckCodecs();
	void qslotAdvOutRecEncoderCurrentIndexChanged(int idx);
	void qslotAdvOutFFIgnoreCompatStateChanged(int);
	void qslotAdvOutFFFormatCurrentIndexChanged(int idx);
	void qslotAdvOutFFAEncoderCurrentIndexChanged(int idx);
	void qslotAdvOutFFVEncoderCurrentIndexChanged(int idx);
	void qslotAdvOutFFTypeCurrentIndexChanged(int idx);
	void qslotSettingModeCurrentIndexChanged(int idx);
	void qslotAdvOutSplitFileChanged();
	void qslotAdvOutRecCheckWarnings();
	void qslotSimpleRecordingQualityChanged();
	void qslotSimpleRecordingQualityLosslessWarning(int idx);
	void qslotUpdateAutomaticReplayBufferCheckboxes();

public:
	explicit AFQOutputSettingAreaWidget(QWidget* parent = nullptr);
	~AFQOutputSettingAreaWidget();

public:
	void LoadOutputSettings(bool reset = false);
	void LoadSimpleOutputSettings();
	void LoadOutputStandardSettings(bool reset);
	void SaveOutputFormat(QComboBox* combo);
	void SaveOutputEncoder(QComboBox* combo, const char* section, const char* value);
	void SaveOutputSettings();
	void SetOutputDataChangedVal(bool changed) { m_outputChanged = changed; };
	bool OutputDataChanged() { return m_outputChanged; };
	void ResetOutputSettings();
	void AdvOutEncoderData(QString advOutVEncoder, QString advOutAEncoder);
	void SimpleOutVEncoder(QString simpleOutVEncoder);
	void SimpleOutAEncoder(QString simpleOutAEncoder);
	void SimpleOutVBitrate(int simpleVBitrate);
	void SimpleOutABitrate(int simpleABitrate);
	void StreamEncoderData(int vbitrate, const char* rateControl);
	void OutputResolution(uint32_t cx, uint32_t cy);
	void ResetDownscales(uint32_t cx, uint32_t cy, bool ignoreAllSignals = false);
	void RefreshDownscales(uint32_t cx, uint32_t cy, const QComboBox* comboBox);
	void HasStreamEncoder(bool hasEncoder) { m_hasStreamEncoderData = hasEncoder; }
	void ToggleOnStreaming(bool streaming);

	QString GetSimpleAudioRecEncoder();
	QString GetSimpleVideoRecEncoder();
	QString GetSimpleRecQuality();
	QString GetSimpleRecFormat();
	QString GetSimpleOutputPath() { return ui->lineEdit_SimpleOutputPath->text(); }

	QString GetAdvAudioRecEncoder();
	QString GetAdvVideoRecEncoder();
	QString GetAdvRecFormat();
	QString GetAdvOutRecPath() { return ui->lineEdit_AdvOutRecPath->text(); }
	QString GetAdvOutFFPath() { return ui->lineEdit_AdvOutFFRecPath->text(); }
	bool IsCustomFFmpeg() { return ui->comboBox_AdvOutRecType->currentIndex() == 1; }

	void ChangeSettingModeToSimple();
	void ChangeSettingModeToAdvanced();
	bool IsAdvancedMode() { return m_isAdvancedMode; }

private:
	void _LoadAdvancedOutputFFmpegSettings();
	void _LoadOutputRecordingEncoderProperties();
	void _LoadAdvReplayBuffer();
	void _LoadFormats();
	void _ReloadCodecs(const FFmpegFormat& format);
	void _SetAdvOutputFFmpegEnablement(FFmpegCodecType encoderType, bool enabled, bool enableEncoder);
	void _ResetEncoders(bool streamOnly);
	OBSPropertiesView* _CreateEncoderPropertyView(const char* encoder, const char* path, bool changed = false);

	void _ClearOutputSettingUi();
	void _FillSimpleRecordingValues();

	inline bool _IsCustomService() const;
	void _EventHookConnect();
	void _SaveFormat(QComboBox* combo);
	void _SaveEncoder(QComboBox* combo, const char* section, const char* value);
	int _CurrentFLVTrack();
	void _SetSettingModeUi(const bool isAdvanced);
	void _ChangeLanguage();
	//void _AdvOutRecCheckWarnings();

private:
	std::vector<FFmpegFormat> m_formats;
	Ui::AFQOutputSettingAreaWidget* ui = nullptr;
	OBSPropertiesView* m_pRecordEncoderProps = nullptr;
	bool m_hasStreamEncoderData = false;
	QString m_curAdvRecordEncoder;

	OBSService m_service;
	QString m_protocol;
	bool m_loading = true;
	bool m_outputChanged = false;
	bool m_isAdvancedMode = false;
	int m_lastSimpleRecQualityIdx = 0;

	QString m_advOutVEncoder;
	QString m_advOutAEncoder;
	QString m_simpleOutVEncoder;
	QString m_simpleOutAEncoder;
	int m_simpleOutVBitrate = 0;
	int m_simpleOutABitrate = 0;
	uint32_t m_outputCX = 0;
	uint32_t m_outputCY = 0;
	int m_vbitrate = 0;
	const char* m_rateControl = nullptr;

	static constexpr uint32_t ENCODER_HIDE_FLAGS = (OBS_ENCODER_CAP_DEPRECATED | OBS_ENCODER_CAP_INTERNAL);

	enum _SplitFileType {SPLIT_MANUAL, SPLIT_TIME, SPLIT_SIZE};
};