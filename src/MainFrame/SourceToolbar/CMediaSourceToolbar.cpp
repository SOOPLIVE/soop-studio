#include "CMediaSourceToolbar.h"
#include "ui_media-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>
#include <QToolTip>
#include <QFileInfo>

#include "qt-wrappers.hpp"

#include "Application/CApplication.h"

#include "CoreModel/Source/CSource.h"

#include "absolute-slider.hpp"

AFQMediaSourceToolbar::AFQMediaSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQMediaSourceToolbar)
{
	ui->setupUi(this);

	OBSDataAutoRelease settings = obs_source_get_settings(source_);
	bool is_local_file = obs_data_get_bool(settings, "is_local_file");
	if (!is_local_file) {
		ui->frameLocalFileControl->hide();
	}

	_SetSignals(source_);

	connect(&m_timerMedia, &QTimer::timeout, this, &AFQMediaSourceToolbar::_qslotSetSliderPosition);
	connect(&m_timerSeek, &QTimer::timeout, this, &AFQMediaSourceToolbar::_qslotSeekTimerCallback);

	connect(ui->mediaSlider, &AbsoluteSlider::sliderPressed, this, &AFQMediaSourceToolbar::_qslotMediaSliderClicked);
	connect(ui->mediaSlider, &AbsoluteSlider::absoluteSliderHovered, this, &AFQMediaSourceToolbar::_qslotMediaSliderHovered);
	connect(ui->mediaSlider, &AbsoluteSlider::sliderReleased, this, &AFQMediaSourceToolbar::_qslotMediaSliderReleased);
	connect(ui->mediaSlider, &AbsoluteSlider::sliderMoved, this, &AFQMediaSourceToolbar::_qslotMediaSliderMoved);

	connect(ui->pushButton_Restart, &QPushButton::clicked, this, &AFQMediaSourceToolbar::_qslotControlButtonClicked);
	connect(ui->pushButton_Play, &QPushButton::clicked, this, &AFQMediaSourceToolbar::_qslotControlButtonClicked);
	connect(ui->pushButton_Pause, &QPushButton::clicked, this, &AFQMediaSourceToolbar::_qslotControlButtonClicked);
	connect(ui->pushButton_Stop, &QPushButton::clicked, this, &AFQMediaSourceToolbar::_qslotStopButtonClicked);
}

AFQMediaSourceToolbar::~AFQMediaSourceToolbar()
{
	delete ui;
}

void AFQMediaSourceToolbar::_qslotControlButtonClicked()
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

void AFQMediaSourceToolbar::_qslotStopButtonClicked()
{
	_StopMedia();
}

void AFQMediaSourceToolbar::_qslotMediaSliderClicked()
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

void AFQMediaSourceToolbar::_qslotMediaSliderReleased()
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

void AFQMediaSourceToolbar::_qslotMediaSliderHovered(int val)
{
	float seconds = ((float)_GetSliderTime(val) / 1000.0f);
	QString times = _FormatSeconds((int)seconds);
	QToolTip::showText(QCursor::pos(), times, this);

	if (m_pressedSlider)
		ui->label_CurrentTime ->setText(times);
}

void AFQMediaSourceToolbar::_qslotMediaSliderMoved(int val)
{
	if (m_timerSeek.isActive()) {
		m_seek = val;
	}
}

void AFQMediaSourceToolbar::_qslotSetPlayingState()
{
	ui->mediaSlider->setEnabled(true);

	ui->pushButton_Play->hide();
	ui->pushButton_Restart->hide();
	ui->pushButton_Pause->show();

	m_prevPaused = false;

	_StartMediaTimer();
}

void AFQMediaSourceToolbar::_qslotSetPausedState()
{
	ui->pushButton_Play->show();
	ui->pushButton_Restart->hide();
	ui->pushButton_Pause->hide();

	_StopMediaTimer();
}

void AFQMediaSourceToolbar::_qslotSetRestartState()
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

void AFQMediaSourceToolbar::_qslotSetSliderPosition()
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

void AFQMediaSourceToolbar::_qslotSeekTimerCallback()
{
	if (m_lastSeek != m_seek) {
		OBSSource source = GetSource();
		if (source) {
			obs_source_media_set_time(source, _GetSliderTime(m_seek));
		}
		m_lastSeek = m_seek;
	}
}

void AFQMediaSourceToolbar::_SetSignals(OBSSource source)
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
		m_signals.emplace_back(sh, "media_file_load", FSMediaFileLoaded, this);
	}

	_RefreshControls();
}

void AFQMediaSourceToolbar::FSMediaFileLoaded(void* data, calldata_t* calldata)
{
	OBSSource source((obs_source_t*)calldata_ptr(calldata, "source"));

	int width = calldata_int(calldata, "width");
	int height = calldata_int(calldata, "height");

	QMetaObject::invokeMethod(
		qApp, [source, width, height]() {

			QString filePath;
			std::string id = obs_source_get_id(source);
			OBSDataAutoRelease settings = obs_source_get_settings(source);

			OBSSceneItemAutoRelease item = obs_scene_sceneitem_from_source(SCENE_CONTEXT.GetCurrentScene(), source);

			if (!item) 
				return;

			obs_transform_info oti;
			obs_sceneitem_get_info(item, &oti);

			uint32_t canvasW = (uint32_t)config_get_uint(ACTIVECONFIG, "Video", "BaseCX");
			uint32_t canvasH = (uint32_t)config_get_uint(ACTIVECONFIG, "Video", "BaseCY");

			const float adjustScaleW = 1.0f / 2.0f;
			const float adjustScaleH = 2.0f / 3.0f;

			if (oti.scale.x == 0.0f || oti.scale.y == 0.0f) {
				float targetScale = 1.0f;
				if (width >= height) {
					const float adjustW = canvasW * adjustScaleW;
					if (width > adjustW)
						targetScale = adjustW / (float)width;
				}
				else {
					const float adjustH = canvasH * adjustScaleH;
					if (height > adjustH)
						targetScale = adjustH / (float)height;
				}
				if (oti.scale.x != targetScale || oti.scale.y != targetScale) {
					oti.scale.x = oti.scale.y = targetScale;
					obs_sceneitem_set_info(item, &oti);
				}
			}

			bool is_changed_name = obs_data_get_bool(settings, "is_changed_name");
			if (!is_changed_name)
			{
				bool is_local_file = obs_data_get_bool(settings, "is_local_file");
				if (is_local_file) {
					const char* input = obs_data_get_string(settings, "local_file");
					filePath = QString::fromUtf8(input);
				}
				else {
					const char* input = obs_data_get_string(settings, "input");
					filePath = QString::fromUtf8(input);
				}

				if (!filePath.isEmpty()) {

					QString fileName = QFileInfo(filePath).fileName();
					QString currentName = QString::fromUtf8(obs_source_get_name(source));

					if (0 == currentName.compare(fileName))
						return;

					QString displayText = fileName;

					// check source name 
					int i = 2;
					while (true) {
						OBSSourceAutoRelease s =
							obs_get_source_by_name(QT_TO_UTF8(displayText));

						if (!s || s == source)
							break;

						displayText = QString("%1 %2").arg(fileName).arg(i++);
					}

					if (displayText != currentName) {

						std::string prevName = obs_source_get_name(source);
						std::string newName = currentName.toStdString();

						UNDO_STACK.AddActionRename(prevName, newName, source);

						obs_source_set_name(source, displayText.toUtf8().constData());
					}
				}
			}

		}, Qt::QueuedConnection);
}

QString AFQMediaSourceToolbar::_FormatSeconds(int totalSeconds)
{
	int seconds = totalSeconds % 60;
	int totalMinutes = totalSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}


int64_t AFQMediaSourceToolbar::_GetSliderTime(int val)
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

void AFQMediaSourceToolbar::_PlayMedia()
{
	OBSSource source = GetSource();
	if (source)
		obs_source_media_play_pause(source, false);
}

void AFQMediaSourceToolbar::_PauseMedia()
{
	OBSSource source = GetSource();
	if (source)
		obs_source_media_play_pause(source, true);
}

void AFQMediaSourceToolbar::_RestartMedia()
{
	OBSSource source = GetSource();
	if (source)
		obs_source_media_restart(source);
}

void AFQMediaSourceToolbar::_StopMedia()
{
	OBSSource source = GetSource();
	if (source)
		obs_source_media_stop(source);
}

void AFQMediaSourceToolbar::_RefreshControls()
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

void AFQMediaSourceToolbar::_StartMediaTimer()
{
	if (!m_timerMedia.isActive())
		m_timerMedia.start(1000);
}

void AFQMediaSourceToolbar::_StopMediaTimer()
{
	if (m_timerMedia.isActive())
		m_timerMedia.stop();
}

void AFQMediaSourceToolbar::OBSMediaStopped(void* data, calldata_t* calldata)
{
	AFQMediaSourceToolbar* toolbar = static_cast<AFQMediaSourceToolbar*>(data);
	QMetaObject::invokeMethod(toolbar, "_qslotSetRestartState");
}

void AFQMediaSourceToolbar::OBSMediaPlay(void* data, calldata_t* calldata)
{
	AFQMediaSourceToolbar* toolbar = static_cast<AFQMediaSourceToolbar*>(data);
	QMetaObject::invokeMethod(toolbar, "_qslotSetPlayingState");
}

void AFQMediaSourceToolbar::OBSMediaPause(void* data, calldata_t* calldata)
{
	AFQMediaSourceToolbar* toolbar = static_cast<AFQMediaSourceToolbar*>(data);
	QMetaObject::invokeMethod(toolbar, "_qslotSetPausedState");
}

void AFQMediaSourceToolbar::OBSMediaStarted(void* data, calldata_t* calldata)
{
	AFQMediaSourceToolbar* toolbar = static_cast<AFQMediaSourceToolbar*>(data);
	QMetaObject::invokeMethod(toolbar, "_qslotSetPlayingState");
}