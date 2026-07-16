#include "CVideoBalloonSourceToolbar.h"
#include "ui_videoballoon-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

#include "MainFrame/CMainFrame.h"
#include "Common/StudioDefine.h"

AFQVideoBalloonSourceToolbar::AFQVideoBalloonSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQVideoBalloonSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->pushButton_VideoLists, &QPushButton::clicked,
		this, &AFQVideoBalloonSourceToolbar::_qslotShowVideoBalloonListsButtonClicked);

	connect(ui->pushButton_Refresh, &QPushButton::clicked,
		this, &AFQVideoBalloonSourceToolbar::_qslotRefreshButtonClicked);

	OBSSource source = GetSource();
	if (!source) {
		return;
	}
}

AFQVideoBalloonSourceToolbar::~AFQVideoBalloonSourceToolbar()
{
	delete ui;
}

void AFQVideoBalloonSourceToolbar::SetContextBarSize(int contextBarSize)
{
	// add button text space
	if (2 == contextBarSize)
		ui->pushButton_Refresh->setText(" " + QTStr("Toolbar.RefreshMedia"));
	else
		ui->pushButton_Refresh->setText("");
}

void AFQVideoBalloonSourceToolbar::_qslotShowVideoBalloonListsButtonClicked()
{

}

void AFQVideoBalloonSourceToolbar::_qslotRefreshButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "refreshnocache");
	obs_property_button_clicked(p, source.Get());
}