#include "CMonitorSourceToolbar.h"
#include "ui_monitor-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

AFQMonitorSourceToolbar::AFQMonitorSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQMonitorSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->comboBox_Display, &QComboBox::currentIndexChanged,
		this, &AFQMonitorSourceToolbar::_qslotMonitorCurrentIndexChanged);

	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	const char* prop_name;
#ifdef _WIN32
	prop_name = "monitor_id";
#elif __APPLE__
	prop_name = "display_uuid";
#else
	is_int = true;
	prop_name = "screen";
#endif

	UpdateSourceComboToolbarProperties(ui->comboBox_Display, source, props.get(), prop_name, false);
}

AFQMonitorSourceToolbar::~AFQMonitorSourceToolbar()
{
	delete ui;
}

void AFQMonitorSourceToolbar::_qslotMonitorCurrentIndexChanged(int idx)
{
	OBSSource source = GetSource();
	if (idx == -1 || !source) {
		return;
	}

	const char* prop_name;
#ifdef _WIN32
	prop_name = "monitor_id";
#elif __APPLE__
	prop_name = "display_uuid";
#else
	is_int = true;
	prop_name = "screen";
#endif

	SaveOldProperties(source);
	UpdateSourceComboToolbarValue(ui->comboBox_Display, source, idx, prop_name, false);
	SetUndoProperties(source);
}