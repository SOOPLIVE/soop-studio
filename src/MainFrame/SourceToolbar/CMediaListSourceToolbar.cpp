#include "CMediaListSourceToolbar.h"
#include "ui_medialist-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>
#include <QToolTip>

#include "absolute-slider.hpp"

AFQMediaListSourceToolbar::AFQMediaListSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQMediaListSourceToolbar)
{
	ui->setupUi(this);

	_SetSignals(source_);

	connect(&m_timerMedia, &QTimer::timeout, this, &AFQMediaListSourceToolbar::_qslotSetSliderPosition);
	connect(&m_timerSeek, &QTimer::timeout, this, &AFQMediaListSourceToolbar::_qslotSeekTimerCallback);

	connect(ui->mediaSlider, &AbsoluteSlider::sliderPressed, this, &AFQMediaListSourceToolbar::_qslotMediaSliderClicked);
	connect(ui->mediaSlider, &AbsoluteSlider::absoluteSliderHovered, this, &AFQMediaListSourceToolbar::_qslotMediaSliderHovered);
	connect(ui->mediaSlider, &AbsoluteSlider::sliderReleased, this, &AFQMediaListSourceToolbar::_qslotMediaSliderReleased);
	connect(ui->mediaSlider, &AbsoluteSlider::sliderMoved, this, &AFQMediaListSourceToolbar::_qslotMediaSliderMoved);

	connect(ui->pushButton_Restart, &QPushButton::clicked, this, &AFQMediaListSourceToolbar::_qslotControlButtonClicked);
	connect(ui->pushButton_Play, &QPushButton::clicked, this, &AFQMediaListSourceToolbar::_qslotControlButtonClicked);
	connect(ui->pushButton_Pause, &QPushButton::clicked, this, &AFQMediaListSourceToolbar::_qslotControlButtonClicked);
	connect(ui->pushButton_Stop, &QPushButton::clicked, this, &AFQMediaListSourceToolbar::_qslotStopButtonClicked);
	connect(ui->pushButton_Prev, &QPushButton::clicked, this, &AFQMediaListSourceToolbar::_qslotPrevButtonClicked);
	connect(ui->pushButton_Next, &QPushButton::clicked, this, &AFQMediaListSourceToolbar::_qslotNextButtonClicked);

	_RefreshControls();
}

AFQMediaListSourceToolbar::~AFQMediaListSourceToolbar()
{
	delete ui;
}

void AFQMediaListSourceToolbar::_qslotControlButtonClicked()
{
	OBSSource source = GetSource();
	if (!source)
		return;

	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
		_RestartMedia();
		break;
	case OBS_MEDIA_STATE_OPENING:
	case OBS_MEDIA_STATE_PLAYING:
		_PauseMedia();
		break;
	case OBS_MEDIA_STATE_PAUSED:
		_PlayMedia();
		break;
	default:
		break;
	}
}

void AFQMediaListSourceToolbar::_qslotStopButtonClicked()
{
	_StopMedia();
}

void AFQMediaListSourceToolbar::_qslotPrevButtonClicked()
{
	OBSSource source = GetSource();
	if (source)
		obs_source_media_previous(source);
}

void AFQMediaListSourceToolbar::_qslotNextButtonClicked()
{
	OBSSource source = GetSource();
	if (source)
		obs_source_media_next(source);
}

void AFQMediaListSourceToolbar::_qslotMediaSliderClicked()
{
	m_pressedSlider = true;

	OBSSource source = GetSource();
	if (!source)
		return;

	obs_media_state state = obs_source_media_get_state(source);

	if (state == OBS_MEDIA_STATE_PAUSED) {
		m_prevPaused = true;
	}
	else if (state == OBS_MEDIA_STATE_PLAYING) {
		m_prevPaused = false;
		_PauseMedia();
		_StopMediaTimer();
	}

	m_seek = ui->mediaSlider->value();

	_qslotSeekTimerCallback();
	ui->label_CurrentTime->setText(_FormatSeconds((int)(_GetSliderTime(m_seek) / 1000.0f)));

	m_timerSeek.start(100);
}

void AFQMediaListSourceToolbar::_qslotMediaSliderReleased()
{
	m_pressedSlider = false;

	OBSSource source = GetSource();
	if (!source)
		return;

	if (m_timerSeek.isActive()) {
		m_timerSeek.stop();
		if (m_lastSeek != m_seek) {
			obs_source_media_set_time(source, _GetSliderTime(m_seek));
		}

		m_seek = m_lastSeek = -1;
	}

	if (!m_prevPaused) {
		_PlayMedia();
		_StartMediaTimer();
	}
}

void AFQMediaListSourceToolbar::_qslotMediaSliderHovered(int val)
{
	float seconds = ((float)_GetSliderTime(val) / 1000.0f);
	QString times = _FormatSeconds((int)seconds);
	QToolTip::showText(QCursor::pos(), times, this);

	if (m_pressedSlider)
		ui->label_CurrentTime ->setText(times);
}

void AFQMediaListSourceToolbar::_qslotMediaSliderMoved(int val)
{
	if (m_timerSeek.isActive()) {
		m_seek = val;
	}
}

void AFQMediaListSourceToolbar::_qslotSetPlayingState()
{
	ui->mediaSlider->setEnabled(true);

	ui->pushButton_Play->hide();
	ui->pushButton_Restart->hide();
	ui->pushButton_Pause->show();

	m_prevPaused = false;

	_StartMediaTimer();
}

void AFQMediaListSourceToolbar::_qslotSetPausedState()
{
	ui->pushButton_Play->show();
	ui->pushButton_Restart->hide();
	ui->pushButton_Pause->hide();

	_StopMediaTimer();
}

void AFQMediaListSourceToolbar::_qslotSetRestartState()
{
	ui->pushButton_Play->hide();
	ui->pushButton_Restart->show();
	ui->pushButton_Pause->hide();

	ui->mediaSlider->setValue(0);

	ui->label_CurrentTime->setText("--:--:--");
	ui->label_Duration->setText("--:--:--");

	ui->mediaSlider->setEnabled(false);

	_StopMediaTimer();
}

void AFQMediaListSourceToolbar::_qslotSetSliderPosition()
{
	OBSSource source = GetSource();
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

	ui->label_CurrentTime->setText(_FormatSeconds((int)(time / 1000.0f)));

	if (!m_countDownTimer)
		ui->label_Duration->setText(
			_FormatSeconds((int)(duration / 1000.0f)));
}

void AFQMediaListSourceToolbar::_qslotSeekTimerCallback()
{
	if (m_lastSeek != m_seek) {
		OBSSource source = GetSource();
		if (source) {
			obs_source_media_set_time(source, _GetSliderTime(m_seek));
		}
		m_lastSeek = m_seek;
	}
}

void AFQMediaListSourceToolbar::_SetSignals(OBSSource source)
{
	m_signals.clear();

	if (source)
	{
		signal_handler_t* sh = obs_source_get_signal_handler(source);
		m_signals.emplace_back(sh, "media_play", OBSMediaPlay, this);
		m_signals.emplace_back(sh, "media_pause", OBSMediaPause, this);
		m_signals.emplace_back(sh, "media_restart", OBSMediaPlay, this);
		m_signals.emplace_back(sh, "media_stopped", OBSMediaStopped, this);
		m_signals.emplace_back(sh, "media_started", OBSMediaStarted, this);
		m_signals.emplace_back(sh, "media_ended", OBSMediaStopped, this);
	}

	_RefreshControls();
}

QString AFQMediaListSourceToolbar::_FormatSeconds(int totalSeconds)
{
	int seconds = totalSeconds % 60;
	int totalMinutes = totalSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}


int64_t AFQMediaListSourceToolbar::_GetSliderTime(int val)
{
	OBSSource source = GetSource();
	if (!source) {
		return 0;
	}

	float percent = (float)val / (float)ui->mediaSlider->maximum();
	float duration = (float)obs_source_media_get_duration(source);
	int64_t seekTo = (int64_t)(percent * duration);

	return seekTo;
}

void AFQMediaListSourceToolbar::_PlayMedia()
{
	OBSSource source = GetSource();
	if (source)
		obs_source_media_play_pause(source, false);
}

void AFQMediaListSourceToolbar::_PauseMedia()
{
	OBSSource source = GetSource();
	if (source)
		obs_source_media_play_pause(source, true);
}

void AFQMediaListSourceToolbar::_RestartMedia()
{
	OBSSource source = GetSource();
	if (source)
		obs_source_media_restart(source);
}

void AFQMediaListSourceToolbar::_StopMedia()
{
	OBSSource source = GetSource();
	if (source)
		obs_source_media_stop(source);
}

void AFQMediaListSourceToolbar::_RefreshControls()
{
	OBSSource source = GetSource();

	uint32_t flags = 0;
	const char* id = nullptr;

	if (source) {
		flags = obs_source_get_output_flags(source);
		id = obs_source_get_unversioned_id(source);
	}

	if (!source || !(flags & OBS_SOURCE_CONTROLLABLE_MEDIA)) {
		_qslotSetRestartState();
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
		_qslotSetRestartState();
		break;
	case OBS_MEDIA_STATE_OPENING:
	case OBS_MEDIA_STATE_PLAYING:
		_qslotSetPlayingState();
		break;
	case OBS_MEDIA_STATE_PAUSED:
		_qslotSetPausedState();
		break;
	default:
		break;
	}

	_qslotSetSliderPosition();
}

void AFQMediaListSourceToolbar::_StartMediaTimer()
{
	if (!m_timerMedia.isActive())
		m_timerMedia.start(1000);
}

void AFQMediaListSourceToolbar::_StopMediaTimer()
{
	if (m_timerMedia.isActive())
		m_timerMedia.stop();
}

void AFQMediaListSourceToolbar::OBSMediaStopped(void* data, calldata_t* calldata)
{
	AFQMediaListSourceToolbar* toolbar = static_cast<AFQMediaListSourceToolbar*>(data);
	QMetaObject::invokeMethod(toolbar, "_qslotSetRestartState");
}

void AFQMediaListSourceToolbar::OBSMediaPlay(void* data, calldata_t* calldata)
{
	AFQMediaListSourceToolbar* toolbar = static_cast<AFQMediaListSourceToolbar*>(data);
	QMetaObject::invokeMethod(toolbar, "_qslotSetPlayingState");
}

void AFQMediaListSourceToolbar::OBSMediaPause(void* data, calldata_t* calldata)
{
	AFQMediaListSourceToolbar* toolbar = static_cast<AFQMediaListSourceToolbar*>(data);
	QMetaObject::invokeMethod(toolbar, "_qslotSetPausedState");
}

void AFQMediaListSourceToolbar::OBSMediaStarted(void* data, calldata_t* calldata)
{
	AFQMediaListSourceToolbar* toolbar = static_cast<AFQMediaListSourceToolbar*>(data);
	QMetaObject::invokeMethod(toolbar, "_qslotSetPlayingState");
}