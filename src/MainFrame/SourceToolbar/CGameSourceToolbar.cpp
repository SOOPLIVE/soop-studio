#include "CGameSourceToolbar.h"
#include "ui_game-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

#include "qt-wrappers.hpp"

#include "CoreModel/Source/CSource.h"

AFQGameCaptureSourceToolbar::AFQGameCaptureSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQGameCaptureSourceToolbar)
{
	obs_property_t* p;
	int cur_idx;

	ui->setupUi(this);

	obs_module_t* mod = obs_get_module("win-capture");
	if (!mod)
		return;

	OBSDataAutoRelease settings = obs_source_get_settings(source_);
	std::string cur_mode = obs_data_get_string(settings, "capture_mode");
	std::string cur_window = obs_data_get_string(settings, "window");

	ui->comboBox_Mode->blockSignals(true);
	p = obs_properties_get(props.get(), "capture_mode");
	cur_idx = FillPropertyCombo(ui->comboBox_Mode, p, cur_mode);
	ui->comboBox_Mode->setCurrentIndex(cur_idx);
	ui->comboBox_Mode->blockSignals(false);

	ui->comboBox_Window->blockSignals(true);
	p = obs_properties_get(props.get(), "window");
	cur_idx = FillPropertyCombo(ui->comboBox_Window, p, cur_window);
	ui->comboBox_Window->setCurrentIndex(cur_idx);
	ui->comboBox_Window->blockSignals(false);

	if (cur_idx != -1 && obs_property_list_item_disabled(p, cur_idx)) {
		SetComboItemEnabled(ui->comboBox_Window, cur_idx, false);
	}

	connect(ui->comboBox_Mode, &QComboBox::currentIndexChanged,
			this, &AFQGameCaptureSourceToolbar::_qslotModeCurrentIndexChanged);


	connect(ui->comboBox_Window, &QComboBox::currentIndexChanged,
			this, &AFQGameCaptureSourceToolbar::_qslotWindowCurrentIndexChanged);

	connect(ui->pushButton_FitScreen, &QPushButton::clicked,
		this, &AFQGameCaptureSourceToolbar::_qslotFitScreenSourceClicked);

	UpdateWindowVisibility();
}

AFQGameCaptureSourceToolbar::~AFQGameCaptureSourceToolbar()
{
	delete ui;
}

void AFQGameCaptureSourceToolbar::UpdateWindowVisibility()
{
	QString mode = ui->comboBox_Mode->currentData().toString();
	bool is_window = (mode == "window");
	ui->label_Window->setVisible(is_window);
	ui->comboBox_Window->setVisible(is_window);
}


void AFQGameCaptureSourceToolbar::_qslotModeCurrentIndexChanged(int idx)
{
	OBSSource source = GetSource();
	if (idx == -1 || !source) {
		return;
	}

	QString id = ui->comboBox_Mode->itemData(idx).toString();

	SaveOldProperties(source);
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "capture_mode", QT_TO_UTF8(id));
	obs_source_update(source, settings);
	SetUndoProperties(source);

	UpdateWindowVisibility();
}

void AFQGameCaptureSourceToolbar::_qslotWindowCurrentIndexChanged(int idx)
{
	OBSSource source = GetSource();
	if (idx == -1 || !source) {
		return;
	}

	QString id = ui->comboBox_Window->itemData(idx).toString();

	SaveOldProperties(source);
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "window", QT_TO_UTF8(id));
	obs_source_update(source, settings);
	SetUndoProperties(source);
}

void AFQGameCaptureSourceToolbar::_qslotFitScreenSourceClicked()
{
	AFSourceUtil::FitSourceToScreenFromMenu(OBS_BOUNDS_SCALE_INNER);
}