#include "CMissionChallengeDialog.h"
#include "ui_mission-challenge-dialog.h"

#include "Application/CApplication.h"


struct ListItem {
	const char* name;
	const char* data;
};
ListItem theme_list[] = {
	{"Mission.Challenge.Theme.Default", "default"},
	{"Mission.Challenge.Theme.Simple", "simple"},
	{"Mission.Challenge.Theme.Wings", "wings"},
	{"Mission.Challenge.Theme.Ribbon", "ribbon"},
	{"Mission.Challenge.Theme.Flower", "flower"},
	{"Mission.Challenge.Theme.Sports", "sports"},
	{"Mission.Challenge.Theme.PostIt", "notepad"}
};
const int theme_count = sizeof(theme_list)/sizeof(theme_list[0]);
ListItem sort_list[] = {
	{"Mission.Challenge.SortOrder.Latest", "regDate"},
	{"Mission.Challenge.SortOrder.NearDeadline", "expireDate"},
	{"Mission.Challenge.SortOrder.HighestReward", "balloonCnt"}
};
const int sort_count = sizeof(sort_list)/sizeof(sort_list[0]);

static OBSSource GetSource(OBSWeakSource weakSource) {
	return OBSGetStrongRef(weakSource);
}
//
AFQMissionChallengeDialog::AFQMissionChallengeDialog(QWidget* parent, OBSSource source)
	: AFTTopBaseDialog(parent),
	ui(new Ui::AFQMissionChallengeDialog),
	m_weakSource(OBSGetWeakRef(source)),
	m_props(obs_source_properties(source), obs_properties_destroy)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);
	AFQBlockManager::ApplyMoveInAllArea(this);

	_Init();
}

AFQMissionChallengeDialog::~AFQMissionChallengeDialog()
{
	delete ui;
}
//


void AFQMissionChallengeDialog::_qslotClickedRefresh()
{
	if(!m_weakSource) {
		return;
	}
	
	OBSSource source = GetSource(m_weakSource);
	obs_property_t* prop = obs_properties_get(m_props.get(), "refreshnocache");
	obs_property_button_clicked(prop, source);
}

void AFQMissionChallengeDialog::_qslotThemeChanged(int)
{
	QVariant data = ui->comboBox_Theme->currentData();
	QString theme = ui->comboBox_Theme->currentText();

	_RequestStringQuery("theme", data.toByteArray().constData());
}

void AFQMissionChallengeDialog::_qslotSortOrderChanged(int)
{
	QVariant data = ui->comboBox_SortOrder->currentData();
	QString sort_order = ui->comboBox_SortOrder->currentText();

	_RequestStringQuery("sort_type", data.toByteArray().constData());
}

void AFQMissionChallengeDialog::_qslotOpacitySliderChanged(int)
{
	int opacity = ui->horizontalSlider_Opacity->value();

	ui->spinBox_Opacity->blockSignals(true);
	ui->spinBox_Opacity->setValue(opacity);
	ui->spinBox_Opacity->blockSignals(false);

	_RequestIntQuery("transparency", opacity);
}

void AFQMissionChallengeDialog::_qslotOpacitySpinBoxChanged(int)
{
	int opacity = ui->spinBox_Opacity->value();

	ui->horizontalSlider_Opacity->blockSignals(true);
	ui->horizontalSlider_Opacity->setValue(opacity);
	ui->horizontalSlider_Opacity->blockSignals(false);

	_RequestIntQuery("transparency", opacity);
}

void AFQMissionChallengeDialog::_qslotVolumeMuteClicked(bool checked)
{
	_RequestBoolQuery("mute", checked);
}

void AFQMissionChallengeDialog::_qslotVolumeSliderChanged(int)
{
	int volume = ui->horizontalSlider_Volume->value();

	ui->spinBox_Volume->blockSignals(true);
	ui->spinBox_Volume->setValue(volume);
	ui->spinBox_Volume->blockSignals(false);

	_RequestIntQuery("volume", volume);
}

void AFQMissionChallengeDialog::_qslotVolumeSpinBoxChanged(int)
{
	int volume = ui->spinBox_Volume->value();

	ui->horizontalSlider_Volume->blockSignals(true);
	ui->horizontalSlider_Volume->setValue(volume);
	ui->horizontalSlider_Volume->blockSignals(false);

	_RequestIntQuery("volume", volume);
}

void AFQMissionChallengeDialog::_qslotShowActiveOnlyClicked(bool checked)
{
	_RequestBoolQuery("only_progress_flag", checked);
}

void AFQMissionChallengeDialog::_qslotExitOnInvisibleClicked(bool checked)
{
	_RequestBoolQuery("shutdown", checked);
}

void AFQMissionChallengeDialog::_qslotBrowserSizeClicked()
{
	int width = ui->spinBox_Width->value();
	int height = ui->spinBox_Height->value();

	OBSSource source = GetSource(m_weakSource);
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_int(settings, "width", width);
	obs_data_set_int(settings, "height", height);
	obs_source_update(source, settings);
}

//

void AFQMissionChallengeDialog::_Init()
{
	if(!m_weakSource) {
		return;
	}

	OBSSource source = GetSource(m_weakSource);
	OBSDataAutoRelease settings = obs_source_get_settings(source);

	// Theme
	size_t current_idx = 0;
	std::string theme = obs_data_get_string(settings, "theme");
	ui->comboBox_Theme->blockSignals(true);
	for(int idx = 0; idx < theme_count; idx++) {
		ListItem item = theme_list[idx];
		//
		ui->comboBox_Theme->addItem(QTStr(item.name), item.data);
		if(0 == theme.compare(item.data))
			current_idx = idx;
	}
	ui->comboBox_Theme->setCurrentIndex(current_idx);
	ui->comboBox_Theme->blockSignals(false);

	// Sort Order
	current_idx = 0;
	std::string sort = obs_data_get_string(settings, "sort_type");
	ui->comboBox_SortOrder->blockSignals(true);
	for(int idx = 0; idx < sort_count; idx++) {
		ListItem item = sort_list[idx];
		//
		ui->comboBox_SortOrder->addItem(QTStr(item.name), item.data);
		if(0 == sort.compare(item.data))
			current_idx = idx;
	}
	ui->comboBox_SortOrder->setCurrentIndex(current_idx);
	ui->comboBox_SortOrder->blockSignals(false);

	bool checked = obs_data_get_bool(settings, "shutdown");
	ui->checkBox_ExitOnInvisible->setChecked(checked);
	checked = obs_data_get_bool(settings, "only_progress_flag");
	ui->checkBox_ShowActiveMissionOnly->setChecked(checked);

	bool reroute_audio = obs_data_get_bool(settings, "reroute_audio");
	ui->checkBox_RerouteAudio->setChecked(reroute_audio);

	int value = obs_data_get_int(settings, "transparency");
	ui->spinBox_Opacity->setMinimum(30);
	ui->spinBox_Opacity->setMaximum(100);
	ui->spinBox_Opacity->setValue(value);
	ui->horizontalSlider_Opacity->setValue(value);
	//
	checked = obs_data_get_bool(settings, "mute");
	ui->checkBox_Mute->setChecked(checked);
	value = obs_data_get_int(settings, "volume");
	ui->spinBox_Volume->setMinimum(0);
	ui->spinBox_Volume->setMaximum(100);
	ui->spinBox_Volume->setValue(value);
	ui->horizontalSlider_Volume->setValue(value);

	int width = obs_data_get_int(settings, "width");
	int height = obs_data_get_int(settings, "height");
	ui->spinBox_Width->setValue(width);
	ui->spinBox_Height->setValue(height);

	// Set Current Values
#if 0
	// Theme
	int idx = ui->comboBox_Theme->findData(QVariant(theme));
	if (idx == -1)
		idx = 0;
	ui->comboBox_Theme->setCurrentIndex(idx);

	// Sort Order
	idx = ui->comboBox_SortOrder->findData(QVariant(sortOrder));
	if (idx == -1)
		idx = 0;
	ui->comboBox_SortOrder->setCurrentIndex(idx);

	// Opacity
	ui->horizontalSlider_Opacity->setValue();
	ui->spinBox_Opacity->setValue();

	// Volume
	ui->pushButton_Volume->setChecked();
	ui->horizontalSlider_Volume->setValue();
	ui->spinBox_Volume->setValue();
	
	// CheckBox
	ui->checkBox_ShowActiveMissionOnly->setChecked();
	ui->checkBox_ExitOnInvisible->setChecked();
#endif
	
	// Connect Events
	connect(ui->pushButton_Close, &QPushButton::clicked,
			this, &AFQMissionChallengeDialog::close);

	connect(ui->pushButton_Refresh, &QPushButton::clicked,
			this, &AFQMissionChallengeDialog::_qslotClickedRefresh);
	connect(ui->comboBox_Theme, &QComboBox::currentIndexChanged,
			this, &AFQMissionChallengeDialog::_qslotThemeChanged);
	connect(ui->comboBox_SortOrder, &QComboBox::currentIndexChanged,
			this, &AFQMissionChallengeDialog::_qslotSortOrderChanged);

	connect(ui->horizontalSlider_Opacity, &QSlider::valueChanged,
			this, &AFQMissionChallengeDialog::_qslotOpacitySliderChanged);
	connect(ui->spinBox_Opacity, &QSpinBox::valueChanged,
			this, &AFQMissionChallengeDialog::_qslotOpacitySpinBoxChanged);

	connect(ui->checkBox_Mute, &QCheckBox::clicked,
			this, &AFQMissionChallengeDialog::_qslotVolumeMuteClicked);
	connect(ui->horizontalSlider_Volume, &QSlider::valueChanged,
			this, &AFQMissionChallengeDialog::_qslotVolumeSliderChanged);
	connect(ui->spinBox_Volume, &QSpinBox::valueChanged,
			this, &AFQMissionChallengeDialog::_qslotVolumeSpinBoxChanged);

	connect(ui->checkBox_ShowActiveMissionOnly, &QCheckBox::clicked,
			this, &AFQMissionChallengeDialog::_qslotShowActiveOnlyClicked);
	connect(ui->checkBox_ExitOnInvisible, &QCheckBox::clicked,
			this, &AFQMissionChallengeDialog::_qslotExitOnInvisibleClicked);

	connect(ui->pushButton_Size, &QPushButton::clicked, 
		this, &AFQMissionChallengeDialog::_qslotBrowserSizeClicked);
}

void AFQMissionChallengeDialog::_RequestStringQuery(const char* name, const char* value)
{
	if(!m_weakSource) {
		return;
	}

	OBSSource source = GetSource(m_weakSource);
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_string(settings, name, value);
	obs_property_t* prop = obs_properties_get(m_props.get(), "request_query");
	obs_property_button_clicked(prop, source);
}
void AFQMissionChallengeDialog::_RequestBoolQuery(const char* name, const bool value)
{
	if(!m_weakSource) {
		return;
	}
	
	OBSSource source = GetSource(m_weakSource);
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_bool(settings, name, value);
	obs_property_t* prop = obs_properties_get(m_props.get(), "request_query");
	obs_property_button_clicked(prop, source);
}
void AFQMissionChallengeDialog::_RequestIntQuery(const char* name, const int value)
{
	if(!m_weakSource) {
		return;
	}
	
	OBSSource source = GetSource(m_weakSource);
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_int(settings, name, value);
	obs_property_t* prop = obs_properties_get(m_props.get(), "request_query");
	obs_property_button_clicked(prop, source);
}