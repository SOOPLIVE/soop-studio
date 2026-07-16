#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQSlideShowSourceToolbar;
}

class AFQSlideShowSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT

public:
	explicit AFQSlideShowSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQSlideShowSourceToolbar();

private slots:
	void _qslotSetRestartState();
	void _qslotSetPlayingState();
	void _qslotSetPausedState();
	void _qslotUpdateSlideCounter();

	void _qslotRestartButtonClicked();
	void _qslotPlayPauseButtonClicked();
	void _qslotStopButtonClicked();
	void _qslotPrevButtonClicked();
	void _qslotNextButtonClicked();

private:
	void _SetSignals(OBSSource source);
	void _SetShortCut();

	void _PlaySlideShow();
	void _PauseSlideShow();
	void _RefreshControls();

	static void OBSMediaStopped(void* data, calldata_t* calldata);
	static void OBSMediaPlay(void* data, calldata_t* calldata);
	static void OBSMediaPause(void* data, calldata_t* calldata);
	static void OBSMediaStarted(void* data, calldata_t* calldata);
	static void OBSMediaNext(void* data, calldata_t* calldata);
	static void OBSMediaPrevious(void* data, calldata_t* calldata);

private:
	Ui::AFQSlideShowSourceToolbar* ui = nullptr;

	std::vector<OBSSignal> m_signals;
};