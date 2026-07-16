#include "CKBOSourceToolbar.h"
#include "ui_kbo-source-toolbar.h"

#include <QPushButton>

#include "Application/CApplication.h"

AFQKBOSourceToolbar::AFQKBOSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source),
	ui(new Ui::AFQKBOSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->pushButton_Refresh, &QPushButton::clicked,
		this, &AFQKBOSourceToolbar::_qslotRefreshButtonClicked);
}

AFQKBOSourceToolbar::~AFQKBOSourceToolbar()
{
	delete ui;
}

void AFQKBOSourceToolbar::SetContextBarSize(int contextBarSize)
{
	// add button text space
	if (2 == contextBarSize)
		ui->pushButton_Refresh->setText(" " + QTStr("Toolbar.RefreshBrowse"));
	else
		ui->pushButton_Refresh->setText("");
}

void AFQKBOSourceToolbar::_qslotRefreshButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "refreshnocache");
	obs_property_button_clicked(p, source.Get());
}