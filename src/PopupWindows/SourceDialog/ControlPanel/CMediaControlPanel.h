#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

namespace Ui {
	class AFQMediaControlPanel;
}

class AFQMediaControlPanel : public QFrame
{
	Q_OBJECT

public:
	explicit AFQMediaControlPanel(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQMediaControlPanel();

private slots:
	void SetPlayingState();
	void SetPausedState();
	void SetRestartState();
	void SetSliderPosition();
	void SeekTimerCallback();

	void ControlButtonClicked();
	void StopButtonClicked();

	void PlayMedia();
	void PauseMedia();
	void RestartMedia();
	void StopMedia();

	void MediaSliderClicked(int value);
	void MediaSliderReleased();
	void MediaSliderHovered(int val);
	void MediaSliderMoved(int val);

public:
	void		SetSource(OBSSource source);
	OBSSource	GetSource();

private:
	QString FormatSeconds(int totalSeconds);
	int64_t GetSliderTime(int val);


	void StartMediaTimer();
	void StopMediaTimer();
	void RefreshControls();

	static void OBSMediaStopped(void* data, calldata_t* calldata);
	static void OBSMediaPlay(void* data, calldata_t* calldata);
	static void OBSMediaPause(void* data, calldata_t* calldata);
	static void OBSMediaStarted(void* data, calldata_t* calldata);
	static void OBSMediaNext(void* data, calldata_t* calldata);
	static void OBSMediaPrevious(void* data, calldata_t* calldata);

private:
	std::vector<OBSSignal>	m_signals;
	OBSWeakSource			m_weakSource = nullptr;
	QTimer					m_timerMedia;
	QTimer					m_timerSeek;

	bool m_pressedSlider = false;
	int	 m_seek;
	int  m_lastSeek;
	bool m_prevPaused = false;
	bool m_countDownTimer = false;

	Ui::AFQMediaControlPanel* ui;
};