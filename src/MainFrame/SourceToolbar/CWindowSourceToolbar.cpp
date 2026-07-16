#include "CWindowSourceToolbar.h"
#include "ui_window-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

AFQWindowSourceToolbar::AFQWindowSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQWindowSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->comboBox_Window, &QComboBox::currentIndexChanged,
		this, &AFQWindowSourceToolbar::_qslotWindowCurrentIndexChanged);

	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	const char* prop_name;
#ifdef _WIN32
	prop_name = "window";
#elif __APPLE__
	prop_name = "display_uuid";
#else
	is_int = true;
	prop_name = "screen";
#endif

	UpdateSourceComboToolbarProperties(ui->comboBox_Window, source, props.get(), prop_name, false);
}

AFQWindowSourceToolbar::~AFQWindowSourceToolbar()
{
	delete ui;
}

void AFQWindowSourceToolbar::_qslotWindowCurrentIndexChanged(int idx)
{
	OBSSource source = GetSource();
	if (idx == -1 || !source) {
		return;
	}

	const char* prop_name;
#ifdef _WIN32
	prop_name = "window";
#elif __APPLE__
	prop_name = "display_uuid";
#else
	is_int = true;
	prop_name = "screen";
#endif

	SaveOldProperties(source);
	UpdateSourceComboToolbarValue(ui->comboBox_Window, source, idx, prop_name, false);
	SetUndoProperties(source);
}