#include "CAquaSourceToolbar.h"
#include "ui_aqua-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

#include "Application/CApplication.h"

AFQAquaSourceToolbar::AFQAquaSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQAquaSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->comboBox_Style, &QComboBox::currentIndexChanged, this, &AFQAquaSourceToolbar::_qslotCurrentIndexChanged);
	connect(ui->pushButton_Refresh, &QPushButton::clicked, this, &AFQAquaSourceToolbar::_qslotRefreshButtonClicked);

	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	const char* prop_name = "style_setting";
	UpdateSourceComboToolbarProperties(ui->comboBox_Style, source, props.get(), prop_name, false);
}

AFQAquaSourceToolbar::~AFQAquaSourceToolbar()
{
	delete ui;
}

void AFQAquaSourceToolbar::SetContextBarSize(int contextBarSize)
{
	// add button text space
	if (2 == contextBarSize)
		ui->pushButton_Refresh->setText(" " + QTStr("Toolbar.RefreshBrowse"));
	else
		ui->pushButton_Refresh->setText("");
}

void AFQAquaSourceToolbar::_qslotRefreshButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "refreshnocache");
	obs_property_button_clicked(p, source.Get());
}

void AFQAquaSourceToolbar::_qslotCurrentIndexChanged(int idx)
{
	OBSSource source = GetSource();
	if (idx == -1 || !source) {
		return;
	}

	const char* prop_name = "style_setting";

	SaveOldProperties(source);
	UpdateSourceComboToolbarValue(ui->comboBox_Style, source, idx, prop_name, false);
	SetUndoProperties(source);
}