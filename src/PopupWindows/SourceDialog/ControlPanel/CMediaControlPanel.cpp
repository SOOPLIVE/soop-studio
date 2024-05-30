#include "CMediaControlPanel.h"
#include "ui_media-control-panel.h"
#include <QToolTip>

void AFQMediaControlPanel::OBSMediaStopped(void* data, calldata_t*)
{
	AFQMediaControlPanel* media = static_cast<AFQMediaControlPanel*>(data);
	QMetaObject::invokeMethod(media, "SetRestartState");
}

void AFQMediaControlPanel::OBSMediaPlay(void* data, calldata_t*)
{
	AFQMediaControlPanel* media = static_cast<AFQMediaControlPanel*>(data);
	QMetaObject::invokeMethod(media, "SetPlayingState");
}

void AFQMediaControlPanel::OBSMediaPause(void* data, calldata_t*)
{
	AFQMediaControlPanel* media = static_cast<AFQMediaControlPanel*>(data);
	QMetaObject::invokeMethod(media, "SetPausedState");
}

void AFQMediaControlPanel::OBSMediaStarted(void* data, calldata_t*)
{
	AFQMediaControlPanel* media = static_cast<AFQMediaControlPanel*>(data);
	QMetaObject::invokeMethod(media, "SetPlayingState");
}

void AFQMediaControlPanel::OBSMediaNext(void* data, calldata_t*)
{
	AFQMediaControlPanel* media = static_cast<AFQMediaControlPanel*>(data);
	QMetaObject::invokeMethod(media, "UpdateSlideCounter");
}

void AFQMediaControlPanel::OBSMediaPrevious(void* data, calldata_t*)
{
	AFQMediaControlPanel* media = static_cast<AFQMediaControlPanel*>(data);
	QMetaObject::invokeMethod(media, "UpdateSlideCounter");
}

AFQMediaControlPanel::AFQMediaControlPanel(QWidget* parent, OBSSource source) :
	QFrame(parent),
	ui(new Ui::AFQMediaControlPanel)
{
	ui->setupUi(this);

	setFocusPolicy(Qt::StrongFocus);

	connect(&m_timerMedia, &QTimer::timeout, 
			this, &AFQMediaControlPanel::SetSliderPosition);
	
	connect(&m_timerSeek, &QTimer::timeout,
			this, &AFQMediaControlPanel::SeekTimerCallback);

	connect(ui->mediaSlider, &AFQMediaSlider::mediaSliderPress,
			this, &AFQMediaControlPanel::MediaSliderClicked);
	connect(ui->mediaSlider, &AFQMediaSlider::mediaSliderHovered, 
			this, &AFQMediaControlPanel::MediaSliderHovered);
	connect(ui->mediaSlider, &AFQMediaSlider::mediaSliderRelease,
			this, &AFQMediaControlPanel::MediaSliderReleased);
	connect(ui->mediaSlider, &AFQMediaSlider::sliderMoved,
			this, &AFQMediaControlPanel::MediaSliderMoved);

	connect(ui->playButton, &QPushButton::clicked,
			this, &AFQMediaControlPanel::ControlButtonClicked);
	connect(ui->pauseButton, &QPushButton::clicked,
			this, &AFQMediaControlPanel::ControlButtonClicked);
	connect(ui->restartButton, &QPushButton::clicked,
			this, &AFQMediaControlPanel::ControlButtonClicked);
	connect(ui->stopButton, &QPushButton::clicked,
			this, &AFQMediaControlPanel::StopButtonClicked);
}

AFQMediaControlPanel::~AFQMediaControlPanel()
{
	delete ui;
}

void AFQMediaControlPanel::SetPlayingState()
{
	ui->mediaSlider->setEnabled(true);

	ui->playButton->hide();
	ui->restartButton->hide();
	ui->pauseButton->show();

	m_prevPaused = false;

	StartMediaTimer();
}


void AFQMediaControlPanel::SetPausedState()
{
	ui->playButton->show();
	ui->restartButton->hide();
	ui->pauseButton->hide();

	StopMediaTimer();
}

void AFQMediaControlPanel::SetRestartState()
{
	ui->playButton->hide();
	ui->restartButton->show();
	ui->pauseButton->hide();

	ui->mediaSlider->setValue(0);

	ui->timerLabel->setText("--:--:--");
	ui->durationLabel->setText("--:--:--");

	ui->mediaSlider->setEnabled(false);

	StopMediaTimer();
}

void AFQMediaControlPanel::SetSliderPosition()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (!source)
		return;

	float time = (float)obs_source_media_get_time(source);
	float duration = (float)obs_source_media_get_duration(source);

	float sliderPosition;

	if (duration)
		sliderPosition =
		(time / duration) * (float)ui->mediaSlider->maximum();
	else
		sliderPosition = 0.0f;

	ui->mediaSlider->setValue((int)sliderPosition);

	ui->timerLabel->setText(FormatSeconds((int)(time / 1000.0f)));

	if (!m_countDownTimer)
		ui->durationLabel->setText(
			FormatSeconds((int)(duration / 1000.0f)));
	//else
	//	ui->durationLabel->setText(
	//		QString("-") +
	//		FormatSeconds((int)((duration - time) / 1000.0f)));
}


void AFQMediaControlPanel::SeekTimerCallback()
{
	if (m_lastSeek != m_seek) {
		OBSSource source = OBSGetStrongRef(m_weakSource);
		if (source) {
			obs_source_media_set_time(source, GetSliderTime(m_seek));
		}
		m_lastSeek = m_seek;
	}
}

void AFQMediaControlPanel::ControlButtonClicked()
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

void AFQMediaControlPanel::StopButtonClicked()
{
	StopMedia();
}

void AFQMediaControlPanel::PlayMedia()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (source)
		obs_source_media_play_pause(source, false);
}

void AFQMediaControlPanel::PauseMedia()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (source)
		obs_source_media_play_pause(source, true);
}

void AFQMediaControlPanel::RestartMedia()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (source)
		obs_source_media_restart(source);
}

void AFQMediaControlPanel::StopMedia()
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (source)
		obs_source_media_stop(source);
}

void AFQMediaControlPanel::MediaSliderClicked(int value)
{
	m_pressedSlider = true;

	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (!source)
		return;

	obs_media_state state = obs_source_media_get_state(source);

	if (state == OBS_MEDIA_STATE_PAUSED) {
		m_prevPaused = true;
	}
	else if (state == OBS_MEDIA_STATE_PLAYING) {
		m_prevPaused = false;
		PauseMedia();
		StopMediaTimer();
	}

	ui->mediaSlider->setValue(value);
	m_seek = value;

	SeekTimerCallback();
	ui->timerLabel->setText(FormatSeconds((int)(GetSliderTime(m_seek) / 1000.0f)));

	m_timerSeek.start(100);
}

void AFQMediaControlPanel::MediaSliderReleased()
{
	m_pressedSlider = false;

	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (!source)
		return;

	if (m_timerSeek.isActive()) {
		m_timerSeek.stop();
		if (m_lastSeek != m_seek) {
			obs_source_media_set_time(source, GetSliderTime(m_seek));
		}

		m_seek = m_lastSeek = -1;
	}

	if (!m_prevPaused) {
		PlayMedia();
		StartMediaTimer();
	}
}

void AFQMediaControlPanel::MediaSliderHovered(int val)
{
	float seconds = ((float)GetSliderTime(val) / 1000.0f);
	QString times = FormatSeconds((int)seconds);
	QToolTip::showText(QCursor::pos(), times, this);

	if (m_pressedSlider)
		ui->timerLabel->setText(times);
}

void AFQMediaControlPanel::MediaSliderMoved(int val)
{
	if (m_timerSeek.isActive()) {
		m_seek = val;
	}
}

void AFQMediaControlPanel::SetSource(OBSSource source)
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


OBSSource AFQMediaControlPanel::GetSource()
{
	return OBSGetStrongRef(m_weakSource);
}

QString AFQMediaControlPanel::FormatSeconds(int totalSeconds)
{
	int seconds = totalSeconds % 60;
	int totalMinutes = totalSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}

int64_t AFQMediaControlPanel::GetSliderTime(int val)
{
	OBSSource source = OBSGetStrongRef(m_weakSource);
	if (!source) {
		return 0;
	}

	float percent = (float)val / (float)ui->mediaSlider->maximum();
	float duration = (float)obs_source_media_get_duration(source);
	int64_t seekTo = (int64_t)(percent * duration);

	return seekTo;
}


void AFQMediaControlPanel::StartMediaTimer()
{
	if (!m_timerMedia.isActive())
		m_timerMedia.start(1000);
}

void AFQMediaControlPanel::StopMediaTimer()
{
	if (m_timerMedia.isActive())
		m_timerMedia.stop();
}

void AFQMediaControlPanel::RefreshControls()
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

	SetSliderPosition();
}