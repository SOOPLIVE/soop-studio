#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

namespace Ui {
	class AFQSliderControlPanel;
}

class AFQSliderControlPanel : public QFrame
{

	Q_OBJECT

public:
	explicit AFQSliderControlPanel(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQSliderControlPanel();

private slots:
	void SetPlayingState();
	void SetPausedState();
	void SetRestartState();

	void ControlButtonClicked();
	void PrevListClicked();
	void NextListClicked();
	void StopButtonClicked();

	void PlayMedia();
	void PauseMedia();
	void RestartMedia();
	void StopMedia();

	void UpdateSlideCounter();

public:
	void		SetSource(OBSSource source);
	OBSSource	GetSource();

private:
	void RefreshControls();

	static void OBSMediaStopped(void* data, calldata_t* calldata);
	static void OBSMediaPlay(void* data, calldata_t* calldata);
	static void OBSMediaPause(void* data, calldata_t* calldata);
	static void OBSMediaStarted(void* data, calldata_t* calldata);
	static void OBSMediaNext(void* data, calldata_t* calldata);
	static void OBSMediaPrevious(void* data, calldata_t* calldata);

private:
	Ui::AFQSliderControlPanel* ui;

	std::vector<OBSSignal>	m_signals;
	OBSWeakSource			m_weakSource = nullptr;
};