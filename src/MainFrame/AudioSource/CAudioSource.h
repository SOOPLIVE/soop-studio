#pragma once

#include <qwidget.h>
#include <QPointer>
#include <QTimer>
#include <qgraphicseffect.h>

#include "MainFrame/CMainFrame.h"

class CMainAudioSource : public QObject
{
	Q_OBJECT

public:
	CMainAudioSource(QObject* parent);
	~CMainAudioSource();

	void qslotCloseVolumeSlider();
	void qslotStopVolumeTimer();
	void qslotMainAudioValueChanged(float volume);
	void qslotCloseMicSlider();
	void qslotStopMicTimer();
	void qslotMainMicValueChanged(float volume);
	void qslotShowVolumeSlider();
	void qslotShowMicSlider();
	void qslotSetVolumeMute();
	void qslotSetMicMute();
	void qslotSetAudioPeakValue();
	void qslotSetMicPeakValue();

public:
	void SetupMainFrameAudioUI(QWidget* parent);
	void DestoryMainFrameAudioUI();

	void ClearVolumeControls();
	void RefreshVolumeColors();
	void UpdateVolumeControlsDecayRate();
	void UpdateVolumeControlsPeakMeterType();

	bool IsAudioMuted();
	bool IsMicMuted();

private:
	void _SetMainAudioVolumePeak();
	void _SetMainMicVolumePeak();

	void _SetAudioVolumeSliderSignalsBlock(bool block);
	void _SetMicVolumeSliderSignalsBlock(bool block);
	void _SetAudioSliderEnabled(bool enabled);
	void _SetMicSliderEnabled(bool enabled);
	void _SetAudioButtonMute(bool mute);
	void _SetMicButtonMute(bool mute);

private:
	// MainFrame UI
	float m_output = (float)-96; // dB
	QPointer<AFQSliderFrame> m_volumeSliderFrame = nullptr;
	float m_input = (float)-96; // dB
	QPointer<AFQSliderFrame> m_micSliderFrame = nullptr;
	QPointer<QGraphicsOpacityEffect> m_effectAudioUI = nullptr;
	QPointer<QGraphicsOpacityEffect> m_effectMicUI = nullptr;

	QPointer<QTimer> m_volumeTimer;
	QPointer<QTimer> m_micTimer;
	QPointer<QTimer> m_audioPeakUpdateTimer;
	QPointer<QTimer> m_micPeakUpdateTimer;
};
