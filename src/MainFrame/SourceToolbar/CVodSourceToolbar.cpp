#include "CVodSourceToolbar.h"
#include "ui_vod-source-toolbar.h"

#include <QPushButton>
#include "absolute-slider.hpp"

#include "MainFrame/CMainFrame.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/OBSOutput/COutput.h"

AFQVodSourceToolbar::AFQVodSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent),
	ui(new Ui::AFQVodSourceToolbar)
{
	ui->setupUi(this);

	ui->pushButton_Pause->hide();
		
	const char* id = obs_source_get_id(source);
	m_vodType = SOOP_SRC_MANAGER.GetSoopVodSourceType(id);

	connect(&m_mediaTimer, &QTimer::timeout, this, &AFQVodSourceToolbar::_qslotSliderPosition);
	connect(&m_seekTimer, &QTimer::timeout, this, &AFQVodSourceToolbar::_qslotSeekTimerCallback);

	connect(ui->mediaSlider, &AbsoluteSlider::sliderPressed, this, &AFQVodSourceToolbar::_qslotMediaSliderClicked);
	connect(ui->mediaSlider, &AbsoluteSlider::sliderReleased, this, &AFQVodSourceToolbar::_qslotMediaSliderReleased);
	connect(ui->mediaSlider, &AbsoluteSlider::sliderMoved, this, &AFQVodSourceToolbar::_qslotMediaSliderMoved);

	connect(ui->pushButton_Play, &QPushButton::clicked, this, &AFQVodSourceToolbar::qslotPlayMedia);
	connect(ui->pushButton_Pause, &QPushButton::clicked, this, &AFQVodSourceToolbar::qslotPauseMedia);
	connect(ui->pushButton_Restart, &QPushButton::clicked, this, &AFQVodSourceToolbar::qslotRestartMedia);
	connect(ui->pushButton_Prev, &QPushButton::clicked, this, &AFQVodSourceToolbar::qslotPrevMediaClicked);
	connect(ui->pushButton_Next, &QPushButton::clicked, this, &AFQVodSourceToolbar::qslotNextMediaClicked);

	RefreshControls();
}

AFQVodSourceToolbar::~AFQVodSourceToolbar()
{
	delete ui;
}

void AFQVodSourceToolbar::RefreshControls()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source)
		return;

	QString id = obs_source_get_name(source);

	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
	case OBS_MEDIA_STATE_NONE:
		_SetRestartState();
		//_SetStopState();
		break;
	case OBS_MEDIA_STATE_OPENING:
	case OBS_MEDIA_STATE_PLAYING:
		_SetPlayingState();
		break;
	case OBS_MEDIA_STATE_PAUSED:
		_SetPausedState();
		break;
	default:
		break;
	}
}

void AFQVodSourceToolbar::StartMediaTimer()
{
	if (!m_mediaTimer.isActive())
		m_mediaTimer.start(1000);
}

void AFQVodSourceToolbar::StopMediaTimer()
{
	if (m_mediaTimer.isActive())
		m_mediaTimer.stop();
}

QString FormatSeconds(int totalSeconds)
{
	int seconds = totalSeconds % 60;
	int totalMinutes = totalSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}

int64_t AFQVodSourceToolbar::_GetSliderTime(int val)
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source)
		return 0;

	float percent = (float)val / (float)ui->mediaSlider->maximum();
	float duration = (float)obs_source_media_get_duration(source);
	int64_t seekTo = (int64_t)(percent * duration);

	return seekTo;
}

void AFQVodSourceToolbar::qslotPlayMedia()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (source)
		obs_source_media_play_pause(source, false);
}

void AFQVodSourceToolbar::qslotPauseMedia()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (source) {
		obs_source_media_play_pause(source, true);
	}
}

void AFQVodSourceToolbar::qslotRestartMedia()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source)
		return;

	if (AFOutputUtil::IsStreamActive())
	{
		std::string id = obs_source_get_id(source);

		VodInfo_s info = SOOP_SRC_MANAGER.GetCurVodInfo(m_vodType);
		if (0 == info.contentIdx)
			return;

		AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
		if (!broadInfo)
			return;

		const bool adultOptionNotMatch = info.is_adult && !broadInfo->AdultOnly();
		const bool categoryNotMatch = info.allowed_category != broadInfo->CategoryNumber();

		if (adultOptionNotMatch || categoryNotMatch)
		{
			AFQCateChangeDialog dlg(MAINFRAME, id.c_str());
			if (info.is_adult) {
				dlg.AddAllowedAnimeAdultCategoryInfo(info.allowed_category);
			}
			else {
				std::list<int> categorys = { info.allowed_category };
				dlg.AddAllowedCategoryInfo(categorys);
			}

			if (QDialog::Accepted != dlg.exec())
				return;

			broadInfo->SetCategory(dlg.GetSelectedCategory());

			if (info.is_adult)
				broadInfo->SetAdultOnly(true);

			AUTH_CONTEXT.SendSoopBroadInfoSetting();
			MAINFRAME->RefreshBroadInfoDockUI(false);
		}
	}

	obs_source_media_restart(source);
}

void AFQVodSourceToolbar::qslotPrevMediaClicked()
{
	SOOP_SRC_MANAGER.PlayPrevVOD(m_vodType);
}

void AFQVodSourceToolbar::qslotNextMediaClicked()
{
	SOOP_SRC_MANAGER.PlayNextVOD(m_vodType);
}

void AFQVodSourceToolbar::_SetStopState() {
	StopMediaTimer();

	ui->pushButton_Play->show();

	ui->pushButton_Restart->hide();
	ui->pushButton_Pause->hide();

	ui->mediaSlider->setSliderPosition(0);
	ui->mediaSlider->setDisabled(true);

	ui->label_CurrentTime->setText("--:--:--");

	ui->label_Duration->setText("--:--:--");
}

void AFQVodSourceToolbar::_SetRestartState()
{
	StopMediaTimer();

	ui->pushButton_Restart->show();
	ui->pushButton_Play->hide();
	ui->pushButton_Pause->hide();

	ui->mediaSlider->setSliderPosition(0);
	ui->mediaSlider->setDisabled(true);

	ui->label_CurrentTime->setText("--:--:--");

	ui->label_Duration->setText("--:--:--");
}

void AFQVodSourceToolbar::_SetPlayingState()
{
	ui->mediaSlider->setEnabled(true);

	ui->pushButton_Play->hide();
	ui->pushButton_Restart->hide();
	ui->pushButton_Pause->show();
	StartMediaTimer();
}

void AFQVodSourceToolbar::_SetPausedState()
{
	ui->pushButton_Play->show();
	ui->pushButton_Restart->hide();
	ui->pushButton_Pause->hide();
#ifdef _SOOP_VLC
	StopMediaTimer();
#else
	ui->mediaSlider->setEnabled(true);

	StartMediaTimer();
#endif // _SOOP_VLC
}

void AFQVodSourceToolbar::qslotRecvOBSMediaStarted()
{
	RefreshControls();
}

void AFQVodSourceToolbar::qslotRecvOBSMediaStopped()
{
	RefreshControls();
}

void AFQVodSourceToolbar::qslotRecvOBSMediaEnded()
{
	RefreshControls();
}

void AFQVodSourceToolbar::qslotRecvOBSMediaPlay()
{
	RefreshControls();
}

void AFQVodSourceToolbar::qslotRecvOBSMediaPause()
{
	RefreshControls();
}

void AFQVodSourceToolbar::_qslotSliderPosition()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source) {
		return;
	}

	float time = (float)obs_source_media_get_time(source);
	float duration = (float)obs_source_media_get_duration(source);

	float sliderPosition;

	if (duration)
		sliderPosition =
		(time / duration) * (float)ui->mediaSlider->maximum();
	else
		sliderPosition = 0.0f;

	ui->mediaSlider->setValue((int)sliderPosition);
	ui->label_CurrentTime->setText(FormatSeconds((int)(time / 1000.0f)));
	ui->label_Duration->setText(FormatSeconds((int)(duration / 1000.0f)));
}

void AFQVodSourceToolbar::_qslotSeekTimerCallback()
{
	if (m_lastSeek != m_seek) {
		OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
		if (source) {
			obs_source_media_set_time(source, _GetSliderTime(m_seek));
		}
		m_lastSeek = m_seek;
	}
}

void AFQVodSourceToolbar::_qslotMediaSliderClicked()
{
#ifdef _SOOL_VLC
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source) {
		return;
	}

	obs_media_state state = obs_source_media_get_state(source);

	if (state == OBS_MEDIA_STATE_PAUSED) {
		m_prevPaused = true;
	}
	else if (state == OBS_MEDIA_STATE_PLAYING) {
		m_prevPaused = false;
		qslotPauseMedia();
		StopMediaTimer();
	}

	m_seek = ui->mediaSlider->value();

	m_seekTimer.start(100);
#else
	StopMediaTimer();

	ui->label_CurrentTime->setText(FormatSeconds((int)(_GetSliderTime(ui->mediaSlider->value()) / 1000.0f)));
#endif // _SOOL_VLC
}

void AFQVodSourceToolbar::_qslotMediaSliderReleased()
{
#ifdef _SOOP_VLC
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source)
		return;

	if (m_seekTimer.isActive()) {
		m_seekTimer.stop();
		if (m_lastSeek != m_seek) {
			obs_source_media_set_time(source, _GetSliderTime(m_seek));
		}
		m_seek = m_lastSeek = -1;
	}

	if (!m_prevPaused) {
		qslotPlayMedia();
		StartMediaTimer();
	}
#else
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	obs_source_media_set_time(source, _GetSliderTime(ui->mediaSlider->value()));

	StartMediaTimer();
#endif // _SOOP_VLC
}

void AFQVodSourceToolbar::_qslotMediaSliderMoved(int val)
{
#ifdef _SOOP_VLC
	if (m_seekTimer.isActive()) {
		m_seek = val;
	}
#endif // _SOOP_VLC
	ui->label_CurrentTime->setText(FormatSeconds((int)(_GetSliderTime(ui->mediaSlider->value()) / 1000.0f)));
}