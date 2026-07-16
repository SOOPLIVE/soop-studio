#include "CBrowserSourceToolbar.h"
#include "ui_browser-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

#include "Application/CApplication.h"

AFQBrowserSourceToolbar::AFQBrowserSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source),
	ui(new Ui::AFQBrowserSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->pushButton_Refresh, &QPushButton::clicked,
		this, &AFQBrowserSourceToolbar::qslotRefreshButtonClicked);
}

AFQBrowserSourceToolbar::~AFQBrowserSourceToolbar()
{
	delete ui;
}

void AFQBrowserSourceToolbar::SetContextBarSize(int contextBarSize)
{
	// add button text space
	if (2 == contextBarSize)
		ui->pushButton_Refresh->setText(" " + QTStr("RefreshBrowser"));
	else
		ui->pushButton_Refresh->setText("");
}

void AFQBrowserSourceToolbar::qslotRefreshButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "refreshnocache");
	obs_property_button_clicked(p, source.Get());
}