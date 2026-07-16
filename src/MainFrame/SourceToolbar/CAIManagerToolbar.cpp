#include "CAIManagerToolbar.h"
#include "ui_aimanager-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

#include "MainFrame/CMainFrame.h"
#include "CoreModel/Source/CSource.h"

AFQAIManagerToolbar::AFQAIManagerToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source),
	ui(new Ui::AFQAIManagerToolbar)
{
	ui->setupUi(this);

	connect(ui->pushButton_Refresh, &QPushButton::clicked,
		this, &AFQAIManagerToolbar::qslotRefreshButtonClicked);

	connect(ui->pushButton_RemoveSource, &QPushButton::clicked,
		this, &AFQAIManagerToolbar::qslotRemoveSource);

}

AFQAIManagerToolbar::~AFQAIManagerToolbar()
{
	delete ui;
}

void AFQAIManagerToolbar::SetContextBarSize(int contextBarSize)
{
	// add button text space
	if (2 == contextBarSize)
		ui->pushButton_Refresh->setText(" " + QTStr("RefreshBrowser"));
	else
		ui->pushButton_Refresh->setText("");
}

void AFQAIManagerToolbar::qslotRemoveSource()
{
	AFSourceUtil::RemoveSourceItems(SCENE_CONTEXT.GetCurrentScene());
}

void AFQAIManagerToolbar::qslotRefreshButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "refreshnocache");
	obs_property_button_clicked(p, source.Get());
}