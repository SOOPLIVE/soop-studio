#include "CAudioSourceToolbar.h"
#include "ui_audio-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

#include "MainFrame/CMainFrame.h"

AFQAudioSourceToolbar::AFQAudioSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source),
	ui(new Ui::AFQAudioSourceToolbar)
{
	ui->setupUi(this);

	QString id = obs_source_get_id(source);
	const char* prop_name = nullptr;

	if (0 == id.compare("wasapi_input_capture")) {
		ui->label_Prop->setText(QTStr("Toolbar.Window"));
		prop_name = "device_id";
	}
	else if (0 == id.compare("wasapi_output_capture")) {
		prop_name = "device_id";
	}
	else if (0 == id.compare("wasapi_process_output_capture")) {
		prop_name = "window";
	}

	UpdateSourceComboToolbarProperties(ui->comboBox_Device, source, props.get(), prop_name, false);

	connect(ui->comboBox_Device, &QComboBox::currentIndexChanged,
		this, &AFQAudioSourceToolbar::_qslotAudioListChanged);
}

AFQAudioSourceToolbar::~AFQAudioSourceToolbar()
{
	delete ui;
}


void AFQAudioSourceToolbar::_qslotAudioListChanged(int idx)
{
	OBSSource source = GetSource();
	if (idx == -1 || !source) {
		return;
	}

	const char* prop_name;
	QString id = obs_source_get_id(source);

	if (0 == id.compare("wasapi_input_capture")) {
		prop_name = "device_id";
	}
	else if (0 == id.compare("wasapi_output_capture")) {
		prop_name = "device_id";
	}
	else if (0 == id.compare("wasapi_process_output_capture")) {
		prop_name = "window";
	}

	UpdateSourceComboToolbarValue(ui->comboBox_Device, source, idx, prop_name, false);
}