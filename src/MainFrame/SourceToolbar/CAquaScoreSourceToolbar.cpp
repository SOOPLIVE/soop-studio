#include "CAquaScoreSourceToolbar.h"
#include "ui_aqua-score-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

AFQAquaScoreSourceToolbar::AFQAquaScoreSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQAquaScoreSourceToolbar)
{
	ui->setupUi(this);

	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	int leftScore = obs_data_get_int(settings, "score_blue_num");
	int rightScore = obs_data_get_int(settings, "score_red_num");
	int leftSet = obs_data_get_int(settings, "score_blue_set");
	int rightSet = obs_data_get_int(settings, "score_red_set");

	ui->spinBox_LeftScore->setValue(leftScore);
	ui->spinBox_RightScore->setValue(rightScore);
	ui->spinBox_LeftSet->setValue(leftSet);
	ui->spinBox_RightSet->setValue(rightSet);

	connect(ui->pushButton_Start, &QPushButton::clicked,
		this, &AFQAquaScoreSourceToolbar::_qslotTimerStartButtonClicked);

	connect(ui->pushButton_Stop, &QPushButton::clicked,
		this, &AFQAquaScoreSourceToolbar::_qslotTimerPauseButtonClicked);

	connect(ui->pushButton_Reset, &QPushButton::clicked,
		this, &AFQAquaScoreSourceToolbar::_qslotTimerResetButtonClicked);

	connect(ui->spinBox_LeftScore, &QSpinBox::valueChanged,
		this, &AFQAquaScoreSourceToolbar::_qslotLeftScoreValueChanged);

	connect(ui->spinBox_RightScore, &QSpinBox::valueChanged,
		this, &AFQAquaScoreSourceToolbar::_qslotRightScoreValueChanged);

	connect(ui->spinBox_LeftSet, &QSpinBox::valueChanged,
		this, &AFQAquaScoreSourceToolbar::_qslotLeftSetValueChanged);

	connect(ui->spinBox_RightSet, &QSpinBox::valueChanged,
		this, &AFQAquaScoreSourceToolbar::_qslotRightSetValueChanged);
}

AFQAquaScoreSourceToolbar::~AFQAquaScoreSourceToolbar()
{
	delete ui;
}

void AFQAquaScoreSourceToolbar::_qslotTimerStartButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "score_timer_play");
	obs_property_button_clicked(p, source.Get());
}

void AFQAquaScoreSourceToolbar::_qslotTimerPauseButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "score_timer_pause");
	obs_property_button_clicked(p, source.Get());
}

void AFQAquaScoreSourceToolbar::_qslotTimerResetButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "score_timer_reset");
	obs_property_button_clicked(p, source.Get());
}

void AFQAquaScoreSourceToolbar::_qslotLeftScoreValueChanged(int value)
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_int(settings, "score_blue_num", value);

	SaveOldProperties(source);
	obs_source_update(source, settings);
	SetUndoProperties(source);
}

void AFQAquaScoreSourceToolbar::_qslotRightScoreValueChanged(int value)
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_int(settings, "score_red_num", value);

	SaveOldProperties(source);
	obs_source_update(source, settings);
	SetUndoProperties(source);
}

void AFQAquaScoreSourceToolbar::_qslotLeftSetValueChanged(int value)
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_int(settings, "score_blue_set", value);

	SaveOldProperties(source);
	obs_source_update(source, settings);
	SetUndoProperties(source);
}

void AFQAquaScoreSourceToolbar::_qslotRightSetValueChanged(int value)
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_int(settings, "score_red_set", value);

	SaveOldProperties(source);
	obs_source_update(source, settings);
	SetUndoProperties(source);
}