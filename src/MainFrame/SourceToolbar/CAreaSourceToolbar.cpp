#include "CAreaSourceToolbar.h"
#include "ui_area-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

#include "MainFrame/CMainFrame.h"

AFQAreaSourceToolbar::AFQAreaSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQAreaSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->buttonCapture, &QPushButton::clicked,
		this, &AFQAreaSourceToolbar::qslotFindWindowButtonClicked);

	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	const char* prop_name;
	prop_name = "window";
}

AFQAreaSourceToolbar::~AFQAreaSourceToolbar()
{
	delete ui;
}

void AFQAreaSourceToolbar::qslotFindWindowButtonClicked()
{
	MAINFRAME->ShowWindowCaptureArea(GetSource());
}
