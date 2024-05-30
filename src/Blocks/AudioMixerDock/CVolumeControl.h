#pragma once

#include <obs.hpp>
#include <QWidget>
#include <QPaintEvent>
#include <QMutex>
#include <QTimer>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

#include "UIComponent/CCustomMenu.h"

class AFQVolumeMeterTimer;

class AFQMuteCheckBox : public QCheckBox {
	Q_OBJECT
public:
	AFQMuteCheckBox(QWidget* parent = nullptr) : QCheckBox(parent)
	{
		setTristate(true);
	}
protected:
	/* While we need it to be tristate internally, we don't want a user being
	 * able to manually get into the partial state. */
	void nextCheckState() override
	{
		if (checkState() != Qt::Checked)
			setCheckState(Qt::Checked);
		else
			setCheckState(Qt::Unchecked);
	}
};

class AFQVolumeMeter : public QWidget {
	Q_OBJECT


	friend class AFQVolControl;
private:
	obs_volmeter_t* obs_volmeter;
	static std::weak_ptr<AFQVolumeMeterTimer> updateTimer;
	std::shared_ptr<AFQVolumeMeterTimer> updateTimerRef;

	inline void resetLevels();
	inline void doLayout();
	inline bool detectIdle(uint64_t ts);
	inline void calculateBallistics(uint64_t ts,
									qreal timeSinceLastRedraw = 0.0);
	inline void calculateBallisticsForChannel(int channelNr, uint64_t ts,
											  qreal timeSinceLastRedraw);

	inline int convertToInt(float number);
	void paintInputMeter(QPainter& painter, int x, int y, int width,
						 int height, float peakHold);
	void paintHMeter(QPainter& painter, int x, int y, int width, int height,
					 float magnitude, float peak, float peakHold);
	void paintVMeter(QPainter& painter, int x, int y, int width, int height,
					 float magnitude, float peak, float peakHold);
	void paintHTicks(QPainter& painter, int x, int y, int width);
	void paintVTicks(QPainter& painter, int x, int y, int height);

	QMutex dataMutex;

	bool recalculateLayout = true;

	uint64_t currentLastUpdateTime = 0;
	float m_currentMaxPeak = -M_INFINITE;
	float currentMagnitude[MAX_AUDIO_CHANNELS];
	float currentPeak[MAX_AUDIO_CHANNELS];
	float currentInputPeak[MAX_AUDIO_CHANNELS];

	int displayNrAudioChannels = 0;
	float displayMagnitude[MAX_AUDIO_CHANNELS];
	float displayPeak[MAX_AUDIO_CHANNELS];
	float displayPeakHold[MAX_AUDIO_CHANNELS];
	uint64_t displayPeakHoldLastUpdateTime[MAX_AUDIO_CHANNELS];
	float displayInputPeakHold[MAX_AUDIO_CHANNELS];
	uint64_t displayInputPeakHoldLastUpdateTime[MAX_AUDIO_CHANNELS];

	QFont tickFont;
	QColor backgroundNominalColor;
	QColor backgroundWarningColor;
	QColor backgroundErrorColor;
	QColor foregroundNominalColor;
	QColor foregroundWarningColor;
	QColor foregroundErrorColor;

	QColor backgroundNominalColorDisabled;
	QColor backgroundWarningColorDisabled;
	QColor backgroundErrorColorDisabled;
	QColor foregroundNominalColorDisabled;
	QColor foregroundWarningColorDisabled;
	QColor foregroundErrorColorDisabled;

	QColor clipColor;
	QColor magnitudeColor;
	QColor majorTickColor;
	QColor minorTickColor;

	int meterThickness;
	qreal meterFontScaling;

	qreal minimumLevel;
	qreal warningLevel;
	qreal errorLevel;
	qreal clipLevel;
	qreal minimumInputLevel;
	qreal peakDecayRate;
	qreal magnitudeIntegrationTime;
	qreal peakHoldDuration;
	qreal inputPeakHoldDuration;

	QColor p_backgroundNominalColor;
	QColor p_backgroundWarningColor;
	QColor p_backgroundErrorColor;
	QColor p_foregroundNominalColor;
	QColor p_foregroundWarningColor;
	QColor p_foregroundErrorColor;

	uint64_t lastRedrawTime = 0;
	int channels = 0;
	bool clipping = false;
	bool vertical;
	bool muted = false;
	bool m_bMeterTickEnabled = false;

public:
	explicit AFQVolumeMeter(QWidget* parent = nullptr,
		obs_volmeter_t* obs_volmeter = nullptr,
		bool vertical = false);
	~AFQVolumeMeter();

	void setLevels(const float magnitude[MAX_AUDIO_CHANNELS],
		const float peak[MAX_AUDIO_CHANNELS],
		const float inputPeak[MAX_AUDIO_CHANNELS]);
	QRect getBarRect() const;
	bool needLayoutChange();

	QColor getBackgroundNominalColor() const;
	void setBackgroundNominalColor(QColor c);
	QColor getBackgroundWarningColor() const;
	void setBackgroundWarningColor(QColor c);
	QColor getBackgroundErrorColor() const;
	void setBackgroundErrorColor(QColor c);
	QColor getForegroundNominalColor() const;
	void setForegroundNominalColor(QColor c);
	QColor getForegroundWarningColor() const;
	void setForegroundWarningColor(QColor c);
	QColor getForegroundErrorColor() const;
	void setForegroundErrorColor(QColor c);

	QColor getBackgroundNominalColorDisabled() const;
	void setBackgroundNominalColorDisabled(QColor c);
	QColor getBackgroundWarningColorDisabled() const;
	void setBackgroundWarningColorDisabled(QColor c);
	QColor getBackgroundErrorColorDisabled() const;
	void setBackgroundErrorColorDisabled(QColor c);
	QColor getForegroundNominalColorDisabled() const;
	void setForegroundNominalColorDisabled(QColor c);
	QColor getForegroundWarningColorDisabled() const;
	void setForegroundWarningColorDisabled(QColor c);
	QColor getForegroundErrorColorDisabled() const;
	void setForegroundErrorColorDisabled(QColor c);

	QColor getClipColor() const;
	void setClipColor(QColor c);
	QColor getMagnitudeColor() const;
	void setMagnitudeColor(QColor c);
	QColor getMajorTickColor() const;
	void setMajorTickColor(QColor c);
	QColor getMinorTickColor() const;
	void setMinorTickColor(QColor c);
	int getMeterThickness() const;
	void setMeterThickness(int v);
	qreal getMeterFontScaling() const;
	void setMeterFontScaling(qreal v);
	qreal getMinimumLevel() const;
	void setMinimumLevel(qreal v);
	qreal getWarningLevel() const;
	void setWarningLevel(qreal v);
	qreal getErrorLevel() const;
	void setErrorLevel(qreal v);
	qreal getClipLevel() const;
	void setClipLevel(qreal v);
	qreal getMinimumInputLevel() const;
	void setMinimumInputLevel(qreal v);
	qreal getPeakDecayRate() const;
	void setPeakDecayRate(qreal v);
	qreal getMagnitudeIntegrationTime() const;
	void setMagnitudeIntegrationTime(qreal v);
	qreal getPeakHoldDuration() const;
	void setPeakHoldDuration(qreal v);
	qreal getInputPeakHoldDuration() const;
	void setInputPeakHoldDuration(qreal v);
	void setPeakMeterType(enum obs_peak_meter_type peakMeterType);
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void wheelEvent(QWheelEvent* event) override;
	void SetMeterTickEnabled(bool enable) { m_bMeterTickEnabled = enable; }
	float GetCurrentMaxPeak() { return m_currentMaxPeak; }

protected:
	void paintEvent(QPaintEvent* event) override;
	void changeEvent(QEvent* e) override;
};

class AFQVolumeMeterTimer : public QTimer {
	Q_OBJECT

public:
	inline AFQVolumeMeterTimer() : QTimer() {}

	void AddVolControl(AFQVolumeMeter* meter);
	void RemoveVolControl(AFQVolumeMeter* meter);

protected:
	void timerEvent(QTimerEvent* event) override;
	QList<AFQVolumeMeter*> volumeMeters;
};

class AFQVolControl : public QWidget {
	Q_OBJECT

private:
	OBSSource source;
	QLabel* nameLabel;
	QLabel* volLabel;
	QPointer<QCheckBox> lockIcon;

	QSlider* slider;
	AFQMuteCheckBox* mute;
	QPushButton* config = nullptr;
	OBSFader obs_fader;
	OBSVolMeter obs_volmeter;

	AFQCustomMenu* contextMenu;
	AFQVolumeMeter* volMeter;

	QPointer<QTimer> m_UpdatePeakTextTimer;

	QString m_strSourceName = "";

	bool m_bValueChangedByUser = true;

	static void OBSVolumeChanged(void* param, float db);
	static void OBSVolumeLevel(void* data,
							   const float magnitude[MAX_AUDIO_CHANNELS],
							   const float peak[MAX_AUDIO_CHANNELS],
							   const float inputPeak[MAX_AUDIO_CHANNELS]);
	static void OBSVolumeMuted(void* data, calldata_t* calldata);
	static void OBSMixersOrMonitoringChanged(void* data, calldata_t*);

	void EmitConfigClicked();

private slots:
	void VolumeChanged();
	void VolumeMuted(bool muted);
	void MixersOrMonitoringChanged();

	void SliderChanged(int vol);
	void updateText();
	void qslotUpdateCurrentDbText();

	void SetConfigButtonReleased();
	void SetConfigButtonPressed();

public slots:
	void SetMuted();

signals:
	void ConfigClicked();

public:
	explicit AFQVolControl(QWidget* parent, OBSSource source, bool showConfig = false,
		bool vertical = false);
	~AFQVolControl();

	inline obs_source_t* GetSource() const { return source; }

	QString GetName() const;
	void SetName(const QString& newName);

	void SetMeterDecayRate(qreal q);
	void setPeakMeterType(enum obs_peak_meter_type peakMeterType);

	void EnableSlider(bool enable);
	void SetContextMenu(AFQCustomMenu* cm);

	void refreshColors();

	void ChangeVolume(int volume);
	void ChangeMuteState();

	bool IsMuted();
	int GetVolume();
	float GetCurrentPeak();

	void ValueChangedByUser(bool byUser) { m_bValueChangedByUser = byUser; }

protected:
	virtual void showEvent(QShowEvent* event) override;
	virtual void hideEvent(QHideEvent* event) override;
	virtual void resizeEvent(QResizeEvent* event) override;
};