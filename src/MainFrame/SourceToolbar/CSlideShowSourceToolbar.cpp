#include "CSlideShowSourceToolbar.h"
#include "ui_slideshow-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>
#include <QToolTip>

AFQSlideShowSourceToolbar::AFQSlideShowSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQSlideShowSourceToolbar)
{
	ui->setupUi(this);

	_SetSignals(source_);
	
	connect(ui->pushButton_Pause, &QPushButton::clicked, 
		this, &AFQSlideShowSourceToolbar::_qslotPlayPauseButtonClicked);

	connect(ui->pushButton_Play, &QPushButton::clicked,
		this, &AFQSlideShowSourceToolbar::_qslotPlayPauseButtonClicked);

	connect(ui->pushButton_Restart, &QPushButton::clicked,
		this, &AFQSlideShowSourceToolbar::_qslotRestartButtonClicked);

	connect(ui->pushButton_Stop, &QPushButton::clicked,
		this, &AFQSlideShowSourceToolbar::_qslotStopButtonClicked);

	connect(ui->pushButton_Prev, &QPushButton::clicked,
		this, &AFQSlideShowSourceToolbar::_qslotPrevButtonClicked);

	connect(ui->pushButton_Next, &QPushButton::clicked,
		this, &AFQSlideShowSourceToolbar::_qslotNextButtonClicked);

	_SetShortCut();
}

AFQSlideShowSourceToolbar::~AFQSlideShowSourceToolbar()
{
	delete ui;
}

void AFQSlideShowSourceToolbar::_qslotSetRestartState()
{
	ui->pushButton_Play->hide();
	ui->pushButton_Pause->hide();
	ui->pushButton_Restart->show();

	ui->label_CurrentIdx->setText("-");
	ui->label_TotalIdx->setText("-");
}

void AFQSlideShowSourceToolbar::_qslotSetPlayingState()
{
	ui->pushButton_Play->hide();
	ui->pushButton_Restart->hide();
	ui->pushButton_Pause->show();

	_qslotUpdateSlideCounter();
}

void AFQSlideShowSourceToolbar::_qslotSetPausedState()
{
	ui->pushButton_Restart->hide();
	ui->pushButton_Pause->hide();
	ui->pushButton_Play->show();
}

void AFQSlideShowSourceToolbar::_qslotUpdateSlideCounter()
{
	OBSSource source = GetSource();

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
		ui->label_CurrentIdx->setText(QString::number(slide + 1));
		ui->label_TotalIdx->setText(QString::number(total));
	}
	else {
		ui->label_CurrentIdx->setText("-");
		ui->label_TotalIdx->setText("-");
	}
}


void AFQSlideShowSourceToolbar::_qslotRestartButtonClicked()
{
	OBSSource source = GetSource();
	if (source) {
		obs_source_media_restart(source);
	}
}

void AFQSlideShowSourceToolbar::_qslotPlayPauseButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
		_qslotRestartButtonClicked();
		break;
	case OBS_MEDIA_STATE_PLAYING:
		_PauseSlideShow();
		break;
	case OBS_MEDIA_STATE_PAUSED:
		_PlaySlideShow();
		break;
	default:
		break;
	}
}


void AFQSlideShowSourceToolbar::_qslotStopButtonClicked()
{
	OBSSource source = GetSource();
	if (source) {
		obs_source_media_stop(source);
	}
}

void AFQSlideShowSourceToolbar::_qslotPrevButtonClicked()
{
	OBSSource source = GetSource();
	if (source) {
		obs_source_media_previous(source);
	}
}

void AFQSlideShowSourceToolbar::_qslotNextButtonClicked()
{
	OBSSource source = GetSource();
	if (source) {
		obs_source_media_next(source);
	}
}

void AFQSlideShowSourceToolbar::_PlaySlideShow()
{
	OBSSource source = GetSource();
	if (source) {
		obs_source_media_play_pause(source, false);
	}
}

void AFQSlideShowSourceToolbar::_PauseSlideShow()
{
	OBSSource source = GetSource();
	if (source) {
		obs_source_media_play_pause(source, true);
	}
}

void AFQSlideShowSourceToolbar::_RefreshControls()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
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

	_qslotUpdateSlideCounter();
}

void AFQSlideShowSourceToolbar::_SetSignals(OBSSource source)
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
		m_signals.emplace_back(sh, "media_next", OBSMediaNext, this);
		m_signals.emplace_back(sh, "media_previous", OBSMediaPrevious, this);
	}

	_RefreshControls();
}

void AFQSlideShowSourceToolbar::_SetShortCut()
{
	QAction* restartAction = new QAction(this);
	restartAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	restartAction->setShortcut({ Qt::Key_R });
	connect(restartAction, &QAction::triggered, this,
		&AFQSlideShowSourceToolbar::_qslotRestartButtonClicked);
	addAction(restartAction);

	QAction* playPause = new QAction(this);
	playPause->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(playPause, &QAction::triggered, this,
		&AFQSlideShowSourceToolbar::_qslotPlayPauseButtonClicked);
	playPause->setShortcut({ Qt::Key_Space });
	addAction(playPause);
}

void AFQSlideShowSourceToolbar::OBSMediaStopped(void* data, calldata_t* calldata)
{
	AFQSlideShowSourceToolbar* slideToolbar = static_cast<AFQSlideShowSourceToolbar*>(data);
	QMetaObject::invokeMethod(slideToolbar, "_qslotSetRestartState");
}

void AFQSlideShowSourceToolbar::OBSMediaPlay(void* data, calldata_t* calldata)
{
	AFQSlideShowSourceToolbar* slideToolbar = static_cast<AFQSlideShowSourceToolbar*>(data);
	QMetaObject::invokeMethod(slideToolbar, "_qslotSetPlayingState");
}

void AFQSlideShowSourceToolbar::OBSMediaPause(void* data, calldata_t* calldata)
{
	AFQSlideShowSourceToolbar* slideToolbar = static_cast<AFQSlideShowSourceToolbar*>(data);
	QMetaObject::invokeMethod(slideToolbar, "_qslotSetPausedState");
}

void AFQSlideShowSourceToolbar::OBSMediaStarted(void* data, calldata_t* calldata)
{
	AFQSlideShowSourceToolbar* slideToolbar = static_cast<AFQSlideShowSourceToolbar*>(data);
	QMetaObject::invokeMethod(slideToolbar, "_qslotSetPlayingState");
}

void AFQSlideShowSourceToolbar::OBSMediaNext(void* data, calldata_t* calldata)
{
	AFQSlideShowSourceToolbar* slideToolbar = static_cast<AFQSlideShowSourceToolbar*>(data);
	QMetaObject::invokeMethod(slideToolbar, "_qslotUpdateSlideCounter");
}

void AFQSlideShowSourceToolbar::OBSMediaPrevious(void* data, calldata_t* calldata)
{
	AFQSlideShowSourceToolbar* slideToolbar = static_cast<AFQSlideShowSourceToolbar*>(data);
	QMetaObject::invokeMethod(slideToolbar, "_qslotUpdateSlideCounter");
}