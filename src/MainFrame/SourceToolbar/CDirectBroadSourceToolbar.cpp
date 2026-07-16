#include "CDirectBroadSourceToolbar.h"
#include "ui_direct-broad-source-toolbar.h"

#include <QPushButton>

#include "qt-wrappers.hpp"

#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

#include "MainFrame/CMainFrame.h"

AFQDirectBroadSourceToolbar::AFQDirectBroadSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source),
	ui(new Ui::AFQDirectBroadSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->pushButton_Refresh, &QPushButton::clicked,
		this, &AFQDirectBroadSourceToolbar::_qslotRefreshButtonClicked);
}

AFQDirectBroadSourceToolbar::~AFQDirectBroadSourceToolbar()
{
	delete ui;
}

void AFQDirectBroadSourceToolbar::SetContextBarSize(int contextBarSize)
{
	// add button text space
	if (2 == contextBarSize)
		ui->pushButton_Refresh->setText(" " + QTStr("Toolbar.RefreshMedia"));
	else
		ui->pushButton_Refresh->setText("");
}

void AFQDirectBroadSourceToolbar::_qslotRefreshButtonClicked()
{
	OBSSource source = GetSource();
	if (!source)
		return;

	int idx = SOOP_SRC_MANAGER.GetDirectBroadIdx();
	if (0 != idx) {
		SOOP_SRC_MANAGER.RequestDirectBroadOneTimeUrlWithGeoBlock(idx);
	}
}