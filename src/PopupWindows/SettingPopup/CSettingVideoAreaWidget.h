#ifndef CVIDEOSETTINGWIDGET_H
#define CVIDEOSETTINGWIDGET_H

#include <QWidget>
#include "CSettingUtils.h"
#include "properties-view.hpp"
#include <util/lexer.h>

struct BaseLexer {
	lexer lex;

public:
	inline BaseLexer() { lexer_init(&lex); }
	inline ~BaseLexer() { lexer_free(&lex); }
	operator lexer* () { return &lex; }
};

namespace Ui
{
	class AFQVideoSettingAreaWidget;
}

class AFQVideoSettingAreaWidget : public QWidget
{
	Q_OBJECT

signals:
	void qsignalVideoDataChanged();
	void qsignalBaseResolutionChanged(int cx, int cy);
	void qsignalCallSimpleReplayBufferChanged();
	void qsignalCallSimpleRecordingEncoderChanged();
	void qsignalCallStreamEncoderPropChanged();
	void qsignalCallUpdateStreamDelayEstimate();
	void qsignalSimpleModeClicked();
	void qsignalAdvancedModeClicked();
	void qsignalSimpleEncoderChanged(QString aEncoder);
	void qsignalSimpleBitrateChanged(int aBitrate);

public slots:
	void qslotSimpleBitrateChanged();
	void qslotSimpleEncoderChanged();

private slots:
	void qslotVideoDataChanged();
	void qslotVideoChangedResolution();
	void qslotVideoChangedRestart();

	void qslotSettingModeCurrentIndexChanged(int idx);
	void qslotAdvOutEncoderCurrentIndexChanged();
	void qslotSimpleStreamingEncoderChanged();
	void qslotBaseResolutionCurrentIndexChanged();
	void qslotBaseResolutionEditTextChanged(const QString& text);
	void qslotOutputResolutionEditTextChanged(const QString& text);
	void qslotUpdateSimpleReplayBuffer();
	void qslotUpdateSimpleRecordingEncoder();
	void qslotStreamEncoderPropChanged();
	void qslotUpdateStreamDelayEstimate();
	void qslotDisableOSXVSyncClicked();

	void prepareEncoderSetting();
	void setAV1CodecProperties();

public:
	enum class ResolutionCheck
	{
		Ok,
		Wrong_BaseResolution,
		Wrong_OutResolution
	};

	explicit AFQVideoSettingAreaWidget(QWidget* parent = nullptr);
	~AFQVideoSettingAreaWidget();

public:
	void LoadVideoSettings(bool reset = false);
	void SaveVideoSettings();
	void ResetVideoSettings();
	void ToggleOnStreaming(bool streaming);
	void SetVideoDataChanged(bool changed);
	void SetVideoDataChangedVal(bool changed) { m_videoDataChanged = changed; };
	bool VideoDataChanged() { return m_videoDataChanged; };

	int VideoBitrate();
	int VideoAdvBitrate();
	const char* VideoAdvRateControl();
	QString GetAdvVideoEncoder();
	QString GetSimpleVideoEncoder();
	uint32_t GetVideoOutputCx() { return m_outputCX; }
	uint32_t GetVideoOutputCy() { return m_outputCY; }
	QComboBox* GetOutResolutionComboBox();

	void ChangeSettingModeToSimple();
	void ChangeSettingModeToAdvanced();

	bool IsValidAspectRatios();
	void HighLightResolution();

protected:
	void showEvent(QShowEvent* event);

private:
	void _ClearVideoSettingUi();

	void _ChangeLanguage();

	void _SetVideoSettingSignal();
	void _SetVideoSettingUi();
	void _SetSettingModeUi(const bool isAdvanced);

	void _LoadResolutionLists();
	void _LoadRendererList();
	void _LoadColorFormats();
	void _LoadColorSpaces();
	void _LoadColorRanges();

	void _LoadFPSData();
	void _LoadFPSCommon();
	void _LoadFPSInteger();
	void _LoadDownscaleFilters();
	
	void _LoadAdvOutputStreamingEncoderProperties();

	bool _ValidResolutions();
	void _RecalcOutputResPixels(const char* resText);
	void _ResetEncoders(bool streamOnly = false);
	void _ResetDownscales(uint32_t cx, uint32_t cy, bool ignoreAllSignals = false);

	OBSPropertiesView* _CreateEncoderPropertyView(const char* encoder, const char* path, bool changed = false);

	bool checkAspectRatio(int width, int height);
	bool convertResText(const char* res, uint32_t& cx, uint32_t& cy);
	bool resTooHigh(uint32_t cx, uint32_t cy);
	bool resTooLow(uint32_t cx, uint32_t cy);

private:
	Ui::AFQVideoSettingAreaWidget* ui = nullptr;
	OBSPropertiesView* m_pEncoderProps = nullptr;

	bool m_loading = true;
	bool m_isAdvancedMode = false;
	bool m_videoDataChanged = false;

	uint32_t m_outputCX = 0;
	uint32_t m_outputCY = 0;

	QString m_curPreset;
	QString m_curQSVPreset;
	QString m_curNVENCPreset;
	QString m_curAMDPreset;
	QString m_curAMDAV1Preset;

	ResolutionCheck m_resolutionCheck = ResolutionCheck::Ok;
};
#endif // CVIDEOSETTINGWIDGET_H
