#include "CSliderControlPanel.h"
#include "ui_slider-control-panel.h"

void AFQSliderControlPanel::OBSMediaStopped(void* data, calldata_t*)
{
	AFQSliderControlPanel* slider = static_cast<AFQSliderControlPanel*>(data);
	QMetaObject::invokeMethod(slider, "SetRestartState");
}

void AFQSliderControlPanel::OBSMediaPlay(void* data, calldata_t*)
{
	AFQSliderControlPanel* slider = static_cast<AFQSliderControlPanel*>(data);
	QMetaObject::invokeMethod(slider, "SetPlayingState");
}

void AFQSliderControlPanel::OBSMediaPause(void* data, calldata_t*)
{
	AFQSliderControlPanel* slider = static_cast<AFQSliderControlPanel*>(data);
	QMetaObject::invokeMethod(slider, "SetPausedState");
}

void AFQSliderControlPanel::OBSMediaStarted(void* data, calldata_t*)
{
	AFQSliderControlPanel* slider = static_cast<AFQSliderControlPanel*>(data);
	QMetaObject::invokeMethod(slider, "SetPlayingState");
}

void AFQSliderControlPanel::OBSMediaNext(void* data, calldata_t*)
{
	AFQSliderControlPanel* slider = static_cast<AFQSliderControlPanel*>(data);
	QMetaObject::invokeMethod(slider, "UpdateSlideCounter");
}

void AFQSliderControlPanel::OBSMediaPrevious(void* data, calldata_t*)
{
	AFQSliderControlPanel* slider = static_cast<AFQSliderControlPanel*>(data);
	QMetaObject::invokeMethod(slider, "UpdateSlideCounter");
}

AFQSliderControlPanel::AFQSliderControlPanel(QWidget* parent, OBSSource source) :
	QFrame(parent),
	ui(new Ui::AFQSliderControlPanel)
{
	ui->setupUi(this);

	setFocusPolicy(Qt::StrongFocus);

	connect(ui->playButton, &QPushButton::clicked,
			this, &AFQSliderControlPanel::ControlButtonClicked);
	connect(ui->pauseButton, &QPushButton::clicked,
			this, &AFQSliderControlPanel::ControlButtonClicked);
	connect(ui->restartButton, &QPushButton::clicked,
			this, &AFQSliderControlPanel::ControlButtonClicked);
	connect(ui->stopButton, &QPushButton::clicked,
			this, &AFQSliderControlPanel::StopButtonClicked);
	connect(ui->prevButton, &QPushButton::clicked,
		this, &AFQSliderControlPanel::PrevListClicked);
	connect(ui->nextButton, &QPushButton::clicked,
		this, &AFQSliderControlPanel::NextListClicked);
}

AFQSliderControlPanel::~AFQSliderControlPanel()
{
	delete ui;
}

void AFQSliderControlPanel::SetPlayingState()
{
	ui->restartButton->hide();
	ui->playButton->hide();
	ui->pauseButton->show();

	UpdateSlideCounter();

}

void AFQSliderControlPanel::SetPausedState()
{
	ui->restartButton->hide();
	ui->playButton->show();
	ui->pauseButton->hide();
}

void AFQSliderControlPanel::SetRestartState()
{
	ui->restartButton->show();
	ui->playButton->hide();
	ui->pauseButton->hide();

	ui->curIndexLabel->setText("-");
	ui->totalIndexLabel->setText("-");
}

void AFQSliderControlPanel::ControlButtonClicked()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (!source)
		return;

	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
		RestartMedia();
		break;
	case OBS_MEDIA_STATE_PLAYING:
		PauseMedia();
		break;
	case OBS_MEDIA_STATE_PAUSED:
		PlayMedia();
		break;
	default:
		break;
	}
}

void AFQSliderControlPanel::PrevListClicked()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (source) {
		obs_source_media_previous(source);
	}
}

void AFQSliderControlPanel::NextListClicked()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (source) {
		obs_source_media_next(source);
	}
}

void AFQSliderControlPanel::StopButtonClicked()
{
	StopMedia();
}

void AFQSliderControlPanel::PlayMedia()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (source)
		obs_source_media_play_pause(source, false);
}

void AFQSliderControlPanel::PauseMedia()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (source)
		obs_source_media_play_pause(source, true);
}

void AFQSliderControlPanel::RestartMedia()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (source)
		obs_source_media_restart(source);
}

void AFQSliderControlPanel::StopMedia()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (source)
		obs_source_media_stop(source);
}

void AFQSliderControlPanel::UpdateSlideCounter()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);

	if (!source)
		return;

	proc_handler_t* ph = obs_source_get_proc_handler(source);
	calldata_t cd = {};

	proc_handler_call(ph, "current_index", &cd);
	int slide = calldata_int(&cd, "current_index");

	proc_handler_call(ph, "total_files", &cd);
	int total = calldata_int(&cd, "total_files");
	calldata_free(&cd);

	if (total > 0) {
		ui->curIndexLabel->setText(QString::number(slide + 1));
		ui->totalIndexLabel->setText(QString::number(total));
	}
	else {
		ui->curIndexLabel->setText("-");
		ui->totalIndexLabel->setText("-");
	}
}

void AFQSliderControlPanel::SetSource(OBSSource source)
{
	m_signals.clear();

	if (source)
	{
		m_weakSource = OBSGetWeakRef(source);

		signal_handler_t* sh = obs_source_get_signal_handler(source);
		m_signals.emplace_back(sh, "media_play", OBSMediaPlay, this);
		m_signals.emplace_back(sh, "media_pause", OBSMediaPause, this);
		m_signals.emplace_back(sh, "media_restart", OBSMediaPlay, this);
		m_signals.emplace_back(sh, "media_stopped", OBSMediaStopped, this);
		m_signals.emplace_back(sh, "media_started", OBSMediaStarted, this);
		m_signals.emplace_back(sh, "media_ended", OBSMediaStopped, this);
		m_signals.emplace_back(sh, "media_next", OBSMediaNext, this);
		m_signals.emplace_back(sh, "media_previous", OBSMediaPrevious, this);
	}
	else {
		m_weakSource = nullptr;
	}

	RefreshControls();
}

OBSSource AFQSliderControlPanel::GetSource()
{
	return OBSGetStrongRef(m_weakSource);
}

void AFQSliderControlPanel::RefreshControls()
{
	OBSSource source;
	source = OBSGetStrongRef(m_weakSource);

	uint32_t flags = 0;
	const char* id = nullptr;

	if (source) {
		flags = obs_source_get_output_flags(source);
		id = obs_source_get_unversioned_id(source);
	}

	if (!source || !(flags & OBS_SOURCE_CONTROLLABLE_MEDIA)) {
		SetRestartState();
		setEnabled(false);
		hide();
		return;
	}
	else {
		setEnabled(true);
		show();
	}

	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
	case OBS_MEDIA_STATE_NONE:
		SetRestartState();
		break;
	case OBS_MEDIA_STATE_PLAYING:
		SetPlayingState();
		break;
	case OBS_MEDIA_STATE_PAUSED:
		SetPausedState();
		break;
	default:
		break;
	}

	UpdateSlideCounter();
}