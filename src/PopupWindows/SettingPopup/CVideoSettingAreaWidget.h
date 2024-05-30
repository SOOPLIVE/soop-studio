#ifndef CVIDEOSETTINGWIDGET_H
#define CVIDEOSETTINGWIDGET_H

#include <QWidget>
#include "CSettingAreaUtils.h"
#include "PopupWindows/SourceDialog/CPropertiesView.h"
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
#pragma region QT Field
	Q_OBJECT;

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
	void qslotChangeSettingModeToSimple();
	void qslotChangeSettingModeToAdvanced();

	void qslotAdvOutEncoderCurrentIndexChanged();
	void qslotSimpleStreamingEncoderChanged();
	void qslotBaseResolutionEditTextChanged(const QString& text);
	void qslotOutputResolutionEditTextChanged(const QString& text);
	void qslotUpdateSimpleReplayBuffer();
	void qslotUpdateSimpleRecordingEncoder();
	void qslotStreamEncoderPropChanged();
	void qslotUpdateStreamDelayEstimate();
	void qslotDisableOSXVSyncClicked();

#pragma endregion QT Field

#pragma region class initializer, destructor
public:
	explicit AFQVideoSettingAreaWidget(QWidget* parent = nullptr);
	~AFQVideoSettingAreaWidget();
#pragma endregion class initializer, destructor

#pragma region public func
public:
	void LoadVideoSettings(bool reset = false);
	void SaveVideoSettings();
	void ResetVideoSettings();
	void ToggleOnStreaming(bool streaming);

	void SetVideoDataChangedVal(bool changed) { m_bVideoDataChanged = changed; };
	bool VideoDataChanged() { return m_bVideoDataChanged; };

	int VideoBitrate();
	int VideoAdvBitrate();
	const char* VideoAdvRateControl();
	QString GetAdvVideoEncoder();
	QString GetSimpleVideoEncoder();
	uint32_t GetVideoOutputCx() { return m_nOutputCX; }
	uint32_t GetVideoOutputCy() { return m_nOutputCY; }
	QComboBox* GetOutResolutionComboBox();

	void ChangeSettingModeToSimple();
	void ChangeSettingModeToAdvanced();

	void ResetDownscales(uint32_t cx, uint32_t cy, bool ignoreAllSignals = false);
#pragma endregion public func

#pragma region private func
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

	AFQPropertiesView* _CreateEncoderPropertyView(const char* encoder, const char* path, bool changed = false);

	bool _ConvertResText(const char* res, uint32_t& cx, uint32_t& cy);
	bool _ResTooHigh(uint32_t cx, uint32_t cy);
	bool _ResTooLow(uint32_t cx, uint32_t cy);
	std::string _ResString(uint32_t cx, uint32_t cy);
#pragma endregion private func

#pragma region private member var
private:
	Ui::AFQVideoSettingAreaWidget* ui;
	AFQPropertiesView* m_streamEncoderProps = nullptr;

	bool m_bLoading = true;
	bool m_bIsAdvancedMode = false;
	bool m_bVideoDataChanged = false;

	uint32_t m_nOutputCX = 0;
	uint32_t m_nOutputCY = 0;

	QString m_strCurAdvStreamEncoder;

	QString m_strCurPreset;
	QString m_strCurQSVPreset;
	QString m_strCurNVENCPreset;
	QString m_strCurAMDPreset;
	QString m_strCurAMDAV1Preset;
#pragma endregion private member var
};
#endif // CVIDEOSETTINGWIDGET_H