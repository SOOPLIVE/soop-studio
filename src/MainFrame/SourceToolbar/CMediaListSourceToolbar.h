#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQMediaListSourceToolbar;
}

class AFQMediaListSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT

public:
	explicit AFQMediaListSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQMediaListSourceToolbar();

private slots:
	void _qslotControlButtonClicked();
	void _qslotStopButtonClicked();

	void _qslotNextButtonClicked();
	void _qslotPrevButtonClicked();

	void _qslotMediaSliderClicked();
	void _qslotMediaSliderReleased();
	void _qslotMediaSliderHovered(int val);
	void _qslotMediaSliderMoved(int val);

	void _qslotSetPlayingState();
	void _qslotSetPausedState();
	void _qslotSetRestartState();
	void _qslotSetSliderPosition();
	void _qslotSeekTimerCallback();

private:
	void _SetSignals(OBSSource source);
	QString _FormatSeconds(int totalSeconds);
	int64_t _GetSliderTime(int val);

	void _PlayMedia();
	void _PauseMedia();
	void _RestartMedia();
	void _StopMedia();

	void _RefreshControls();
	void _StartMediaTimer();
	void _StopMediaTimer();

	static void OBSMediaStopped(void* data, calldata_t* calldata);
	static void OBSMediaPlay(void* data, calldata_t* calldata);
	static void OBSMediaPause(void* data, calldata_t* calldata);
	static void OBSMediaStarted(void* data, calldata_t* calldata);

private:
	Ui::AFQMediaListSourceToolbar* ui;

	std::vector<OBSSignal> m_signals;

	QTimer m_timerMedia;
	QTimer m_timerSeek;

	bool m_pressedSlider = false;
	int	 m_seek = 0;
	int  m_lastSeek = 0;
	bool m_prevPaused = false;
	bool m_countDownTimer = false;
};