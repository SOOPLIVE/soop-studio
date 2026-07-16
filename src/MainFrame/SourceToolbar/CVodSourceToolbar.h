#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"
#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQVodSourceToolbar;
}

class AFQVodSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT

public:
	explicit AFQVodSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQVodSourceToolbar();

	void RefreshControls();
	void StartMediaTimer();
	void StopMediaTimer();

public slots:
	void qslotPlayMedia();
	void qslotPauseMedia();
	void qslotRestartMedia();
	void qslotPrevMediaClicked();
	void qslotNextMediaClicked();

	// common SOOP Source Props
	void qslotRecvOBSMediaStarted();
	void qslotRecvOBSMediaStopped();
	void qslotRecvOBSMediaEnded();
	void qslotRecvOBSMediaPlay();
	void qslotRecvOBSMediaPause();

private slots:
	void _qslotSliderPosition();
	void _qslotSeekTimerCallback();
	void _qslotMediaSliderClicked();
	void _qslotMediaSliderReleased();
	void _qslotMediaSliderMoved(int val);

private:	
	void _SetPlayingState();
	void _SetPausedState();
	void _SetStopState();
	void _SetRestartState();


	int64_t _GetSliderTime(int val);

private:
	Ui::AFQVodSourceToolbar* ui;

	SOOP_VOD_TYPE m_vodType = SOOP_VOD_TYPE::NONE;

	QTimer m_mediaTimer;
	QTimer m_seekTimer;

	int	 m_seek;
	int  m_lastSeek;
};