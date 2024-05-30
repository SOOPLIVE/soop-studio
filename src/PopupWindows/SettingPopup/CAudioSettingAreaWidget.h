#ifndef CAUDIOSETTINGWIDGET_H
#define CAUDIOSETTINGWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include "CSettingAreaUtils.h"

#include "../CoreModel/Audio/CAudio.h"


#define VOLUME_METER_DECAY_FAST 23.53
#define VOLUME_METER_DECAY_MEDIUM 11.76
#define VOLUME_METER_DECAY_SLOW 8.57

namespace Ui
{
	class AFQAudioSettingAreaWidget;
}

class AFQSilentUpdateCheckBox : public QCheckBox {
	Q_OBJECT

public slots:
	void qslotSetCheckedSilently(bool checked)
	{
		bool blocked = blockSignals(true);
		setChecked(checked);
		blockSignals(blocked);
	}
};

class AFQAudioSettingAreaWidget : public QWidget
{
#pragma region QT Field
	Q_OBJECT;

signals:
	void qsignalAudioDataChanged();
	void qsignalCallSimpleReplayBufferChanged();
	void qsignalCallSimpleRecordingEncoderChanged();
	void qsignalCallUpdateStreamDelayEstimate();
	void qsignalSimpleModeClicked();
	void qsignalAdvancedModeClicked();
	void qsignalSimpleEncoderChanged(QString aEncoder);
	void qsignalSimpleBitrateChanged(int aBitrate);

public slots:
	void qslotSimpleBitrateChanged();
	void qslotSimpleEncoderChanged();

private slots:
	void qslotAudioChangedRestart();
	void qslotAudioDataChanged();

	void qslotChangeSettingModeToSimple();
	void qslotChangeSettingModeToAdvanced();

	void qslotSurroundWarning(int idx);
	void qslotSpeakerLayoutChanged(int idx);
	void qslotLowLatencyBufferingChanged(bool checked);
	void qslotSimpleStreamAudioEncoderChanged();
	void qslotAdvAudioEncodersChanged();
	void qslotUpdateSimpleReplayBuffer();
	void qslotUpdateSimpleRecordingEncoder();
	void qslotUpdateStreamDelayEstimate();
	void qslotReloadAudioSources();
#pragma endregion QT Field

#pragma region class initializer, destructor
public:
	explicit AFQAudioSettingAreaWidget(QWidget* parent = nullptr);
	~AFQAudioSettingAreaWidget();
#pragma endregion class initializer, destructor

#pragma region public func
public:
	void LoadAudioSettings(bool reset = false);
	void SaveAudioSettings();
	void ResetAudioSettings();
	void ToggleOnStreaming(bool streaming);

	QString CurrentAudioBitRate();
	bool CheckAudioRestartRequired() { return m_bRestartNeeded; }

	void SetAudioDataChangedVal(bool changed) {m_bAudioDataChanged = changed; };
	bool AudioDataChanged() { return m_bAudioDataChanged; };
	int AudioBitrate();
	QString GetAdvAudioEncoder();
	QString GetSimpleAudioEncoder();

	void ChangeSettingModeToSimple();
	void ChangeSettingModeToAdvanced();
#pragma endregion public func

#pragma region private func
private:
	void _ClearAudioSettingUi();

	void _ChangeLanguage();

	void _LoadAudioDevices();
	void _LoadAudioSources();
	void _LoadListValues(QComboBox* widget, obs_property_t* prop, int index);
	void _LoadListValue(QComboBox* widget, const char* text, const char* val);

	void _ResetEncoders(bool streamOnly = false);

	void _SetAudioSettingSignal();
	void _SetAudioSettingUi();

	void _FillAudioMonitoringDevices();
	void _SetSettingModeUi(const bool isAdvanced);

	void _PopulateSimpleBitrates(QComboBox* box, bool opus);
	int _FindClosestAvailableAudioBitrate(QComboBox* box, int bitrate);
	bool _IsSurround(const char* speakers);
	void _RestrictResetBitrates(std::initializer_list<QComboBox*> boxes, int maxbitrate);

	void _SaveAudioBitrate();
#pragma endregion private func

#pragma region private member var
private:
	Ui::AFQAudioSettingAreaWidget* ui;

	using AudioSource_t =
		std::tuple<OBSWeakSource, QPointer<QCheckBox>,
		QPointer<QSpinBox>, QPointer<QCheckBox>,
		QPointer<QSpinBox>>;

	std::vector<AudioSource_t> m_AudioSources;
	std::vector<OBSSignal> m_AudioSourceSignals;
	OBSSignal m_SourceCreated;
	OBSSignal m_ChannelChanged;

	bool m_bLoading = true;
	bool m_bIsAdvancedMode = false;
	bool m_bAudioDataChanged = false;
	bool m_bRestartNeeded = false;

	int m_nSampleRateIndex = 0;
	int m_nChannelIndex = 0;
	bool m_bLlBufferingEnabled = false;

	int m_nLastChannelSetupIdx = 0;

	AFAudioUtil		audioUtil;
#pragma endregion private member var
};
#endif // CAUDIOSETTINGWIDGET_H