#include "CCommerceSourceToolbar.h"
#include "ui_commerce-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

#include <MainFrame/CMainFrame.h>

AFQCommerceSourceToolbar::AFQCommerceSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQCommerceSourceToolbar)
{
	ui->setupUi(this);

	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	connect(ui->pushButton_Refresh, &QPushButton::clicked, 
		this, &AFQCommerceSourceToolbar::_qslotBrowserRefreshClicked);


	connect(ui->pushButton_CommerceSetting, &QPushButton::clicked,
		this, &AFQCommerceSourceToolbar::_qslotCommerceSettingClicked);
}

AFQCommerceSourceToolbar::~AFQCommerceSourceToolbar()
{
	delete ui;
}

void AFQCommerceSourceToolbar::SetContextBarSize(int contextBarSize)
{
	// add button text space
	if (2 == contextBarSize)
		ui->pushButton_Refresh->setText(" " + QTStr("Toolbar.RefreshBrowse"));
	else
		ui->pushButton_Refresh->setText("");
}

void AFQCommerceSourceToolbar::_qslotBrowserRefreshClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "refreshnocache");
	obs_property_button_clicked(p, source.Get());
}


void AFQCommerceSourceToolbar::_qslotCommerceSettingClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	MAINFRAME->CreateSoopCefDetailProperties(source);
}