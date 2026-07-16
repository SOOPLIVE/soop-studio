#include "CMissionDonationRankDialog.h"
#include "ui_mission-donation-rank-dialog.h"
#include "Application/CApplication.h"

//
static OBSSource GetSource(OBSWeakSource weakSource) {
	return OBSGetStrongRef(weakSource);
}
//
AFQMissionDonationRankDialog::AFQMissionDonationRankDialog(QWidget* parent, OBSSource source, int type)
	: AFTTopBaseDialog(parent),
	ui(new Ui::AFQMissionDonationRankDialog),
	m_weakSource(OBSGetWeakRef(source)), 
	m_props(obs_source_properties(source), obs_properties_destroy),
	removeSignal(obs_source_get_signal_handler(source), "remove",
		AFQMissionDonationRankDialog::_SourceRemoved, this)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);
	AFQBlockManager::ApplyMoveInAllArea(this);
	
	if(1 == type) {
		ui->checkBox_ShowDonorsOnly->setVisible(false);
		QPoint pos = ui->checkBox_ExitOnInvisible->pos();
		pos.setY(pos.y() - 30);
		ui->checkBox_ExitOnInvisible->move(pos);
		//
		ui->label_WindowTitle->setText(QTStr("Mission.Properties.JoinUsers"));
	}

	_Init();
}

AFQMissionDonationRankDialog::~AFQMissionDonationRankDialog()
{
	delete ui;
}

static void qslotCloseButtonClicked()
{
}
void AFQMissionDonationRankDialog::_Init()
{
	if(!m_weakSource) {
		return;
	}

	OBSSource source = GetSource(m_weakSource);
	OBSDataAutoRelease settings = obs_source_get_settings(source);

	bool checked = obs_data_get_bool(settings, "shutdown");
	ui->checkBox_ExitOnInvisible->setChecked(checked);
	checked = obs_data_get_bool(settings, "my_funding_list");
	ui->checkBox_ShowDonorsOnly->setChecked(checked);

	bool reroute_audio = obs_data_get_bool(settings, "reroute_audio");
	ui->checkBox_RerouteAudio->setChecked(reroute_audio);

	int width = obs_data_get_int(settings, "width");
	int height = obs_data_get_int(settings, "height");
	ui->spinBox_Width->setValue(width);
	ui->spinBox_Height->setValue(height);

	// Set Current Values
#if 0
	ui->checkBox_ShowDonorsOnly->setChecked();
	ui->checkBox_ExitOnInvisible->setChecked();
#endif

	// Connect Events
	connect(ui->pushButton_Close, &QPushButton::clicked,
			this, &AFQMissionDonationRankDialog::close);
	
	connect(ui->pushButton_Refresh, &QPushButton::clicked,
			this, &AFQMissionDonationRankDialog::_qslotClickedRefresh);
	connect(ui->checkBox_ShowDonorsOnly, &QCheckBox::clicked,
			this, &AFQMissionDonationRankDialog::_qslotShowDonorsOnlyClicked);
	connect(ui->checkBox_ExitOnInvisible, &QCheckBox::clicked,
			this, &AFQMissionDonationRankDialog::_qslotExitOnInvisibleClicked);

	connect(ui->pushButton_Size, &QPushButton::clicked,
			this, &AFQMissionDonationRankDialog::_qslotBrowserSizeClicked);

}

void AFQMissionDonationRankDialog::_SourceRemoved(void* data, calldata_t* params)
{
	QMetaObject::invokeMethod(static_cast<AFQVodSourceDialog*>(data),
		"close");
}

void AFQMissionDonationRankDialog::_qslotClickedRefresh()
{
	if(!m_weakSource) {
		return;
	}
	
	OBSSource source = GetSource(m_weakSource);
	obs_property_t* prop = obs_properties_get(m_props.get(), "refreshnocache");
	obs_property_button_clicked(prop, source);
	// current setting request
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	bool checked = ui->checkBox_ExitOnInvisible->isChecked();
	obs_data_set_bool(settings, "shutdown", checked);
	checked = ui->checkBox_ShowDonorsOnly->isChecked();
	obs_data_set_bool(settings, "my_funding_list", checked);
	prop = obs_properties_get(m_props.get(), "request_query");
	obs_property_button_clicked(prop, source);
}

void AFQMissionDonationRankDialog::_qslotShowDonorsOnlyClicked(bool checked)
{
	if(!m_weakSource) {
		return;
	}

	OBSSource source = GetSource(m_weakSource);
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_bool(settings, "my_funding_list", checked);
	obs_property_t* prop = obs_properties_get(m_props.get(), "request_query");
	obs_property_button_clicked(prop, source);
}

void AFQMissionDonationRankDialog::_qslotExitOnInvisibleClicked(bool checked)
{
	if(!m_weakSource) {
		return;
	}
	
	OBSSource source = GetSource(m_weakSource);
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_bool(settings, "shutdown", checked);
	obs_property_t* prop = obs_properties_get(m_props.get(), "request_query");
	obs_property_button_clicked(prop, source);
}

void AFQMissionDonationRankDialog::_qslotBrowserSizeClicked()
{
	int width = ui->spinBox_Width->value();
	int height = ui->spinBox_Height->value();

	OBSSource source = GetSource(m_weakSource);
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_int(settings, "width", width);
	obs_data_set_int(settings, "height", height);
	obs_source_update(source, settings);
}

void AFQMissionDonationRankDialog::showEvent(QShowEvent* event)
{
	if (MAINFRAME->IsSmallResolution())
	{
		setFixedHeight(550);
		resize(this->width(), 550);
	}

	QRect adjusted;
	MAIN_BLOCKMANAGER->AdjustPositionOutSideFullScreen(this->geometry(), adjusted);
	setGeometry(adjusted);
}
