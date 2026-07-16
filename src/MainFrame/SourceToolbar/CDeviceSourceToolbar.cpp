#include "CDeviceSourceToolbar.h"
#include "ui_device-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "PopupWindows/SourceDialog/EffectSource/CSplitEffectDialog.h"

AFQDeviceSourceToolbar::AFQDeviceSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source, true),
	ui(new Ui::AFQDeviceSourceToolbar)
{
	ui->setupUi(this);

	OBSDataAutoRelease settings = obs_source_get_settings(source);

#ifdef _WIN32

	m_active = obs_data_get_bool(settings, "active");
	ui->pushButton_Active->setText(m_active ? QTStr("Disable") : QTStr("Enable"));

	std::string video_device_id = obs_data_get_string(settings,"video_device_id");
	if (0 == video_device_id.compare("YYCam Pro:")) {
		ui->pushButton_VideoConfig->setVisible(false);
	}

	connect(ui->pushButton_VideoConfig, &QPushButton::clicked, this, &AFQDeviceSourceToolbar::_qslotVideoConfigClicked);
	connect(ui->pushButton_Active, &QPushButton::clicked, this, &AFQDeviceSourceToolbar::_qslotVideoDeviceActivateClicked);

#elif defined(__APPLE__)
	ui->pushButton_VideoConfig->hide();
	ui->pushButton_Active->hide();
#endif

	_InitSplitFilterUI();
}

AFQDeviceSourceToolbar::~AFQDeviceSourceToolbar()
{
	delete ui;
}


void AFQDeviceSourceToolbar::_qslotVideoConfigClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	if(!props)
		props = properties_t(obs_source_properties(source), obs_properties_destroy);

	obs_property_t* prop = obs_properties_get(props.get(), "video_config");
	obs_property_button_clicked(prop, source.Get());
}


void AFQDeviceSourceToolbar::_qslotVideoDeviceActivateClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	bool now_active = obs_data_get_bool(settings, "active");

	bool desyncedSetting = now_active != m_active;

	m_active = !m_active;

	QString text = m_active ? QTStr("Disable") : QTStr("Enable");
	ui->pushButton_Active->setText(text);

	if (desyncedSetting) {
		return;
	}

	calldata_t cd = {};
	calldata_set_bool(&cd, "active", m_active);
	proc_handler_t* ph = obs_source_get_proc_handler(source);
	proc_handler_call(ph, "activate", &cd);
	calldata_free(&cd);
}

void AFQDeviceSourceToolbar::_qslotSplitFilterToggle(bool checked)
{
	OBSSource source = GetSource();
	if (!source)
		return;

	bool notExistFilter = false;
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	QString splitFilterName = obs_data_get_string(settings, SAVED_SPLIT_FILTER_NAME);
	if (splitFilterName.isEmpty())
	{
		notExistFilter = true;
	}

	if (notExistFilter)
	{
		splitFilterName = "soop_split_shader_filter";

		const int MAX_ATTEMPTS = 100;
		int attempt = 0;

		OBSSourceAutoRelease existingFilter = obs_source_get_filter_by_name(source, QT_TO_UTF8(splitFilterName));
		while (existingFilter && attempt < MAX_ATTEMPTS) {
			splitFilterName += "_";
			existingFilter = obs_source_get_filter_by_name(source, QT_TO_UTF8(splitFilterName));
			++attempt;
		}

		if (attempt == MAX_ATTEMPTS) {
			qWarning() << "Failed to find an available split filter name";
			return;
		}

		obs_data_set_string(settings, SAVED_SPLIT_FILTER_NAME, QT_TO_UTF8(splitFilterName));
	}

	OBSSourceAutoRelease filter = obs_source_get_filter_by_name(source, QT_TO_UTF8(splitFilterName));
	if (!filter) {
		filter = obs_source_create(SPLIT_FILTER_ID, QT_TO_UTF8(splitFilterName), nullptr, nullptr);
		if (!filter)
			return;
		obs_source_filter_add(source, filter);
	}

	OBSDataAutoRelease filterData = obs_source_get_settings(filter);
	if (!filterData)
		return;

	if (notExistFilter)
	{
		AFQSplitEffectDialog::SetFilterData(AFQSplitEffectDialog::SplitType::TwoWay, source, true);
	}
	else
	{
		if (checked) {
			const char* savedSplitType = obs_data_get_string(filterData, PRE_SPLIT_TYPE_PROPERTY);
			AFQSplitEffectDialog::SetFilterData(AFQSplitEffectDialog::GetPresetType(savedSplitType), source, true);
		}
		else {
			const char* savedSplitType = obs_data_get_string(filterData, PRESET_PROPERTY);
			obs_data_set_string(filterData, PRE_SPLIT_TYPE_PROPERTY, savedSplitType);
			AFQSplitEffectDialog::SetFilterData(AFQSplitEffectDialog::SplitType::None, source, true);
		}
	}

	emit qsignalSplitFilterActived();
}

void AFQDeviceSourceToolbar::_qslotSplitFilterSettingsClicked()
{
	OBSSource source = GetSource();
	if (!source)
		return;

	MAINFRAME->CreateSplitEffectPopup(source);
}

void AFQDeviceSourceToolbar::SetSplitToggleButtonState(bool checked)
{
	if (ui->pushButton_SplitFilter->isChecked() == checked)
		return;

	ui->pushButton_SplitFilter->SetChecked(checked);
}


void AFQDeviceSourceToolbar::RefreshSplitToogleButtonState()
{
	OBSSource source = GetSource();
	if (!source)
		return;

	bool splitFilterApplied = false;

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	const char* splitFilterName = obs_data_get_string(settings, SAVED_SPLIT_FILTER_NAME);
	if (splitFilterName && *splitFilterName != '\0')
	{
		OBSSourceAutoRelease filter = obs_source_get_filter_by_name(source, splitFilterName);
		if (filter) {
			OBSDataAutoRelease filterData = obs_source_get_settings(filter);
			QString splitType = obs_data_get_string(filterData, PRESET_PROPERTY);
			if (0 != splitType.compare("preset_default")) {
				splitFilterApplied = true;
			}
		}
	}

	SetSplitToggleButtonState(splitFilterApplied);
}

void AFQDeviceSourceToolbar::_InitSplitFilterUI()
{
	//
	RefreshSplitToogleButtonState();

	connect(ui->pushButton_SplitFilter, &AFQToggleButton::clicked,
		this, &AFQDeviceSourceToolbar::_qslotSplitFilterToggle);

	connect(ui->pushButton_SplitFilterSettings, &QPushButton::clicked,
		this, &AFQDeviceSourceToolbar::_qslotSplitFilterSettingsClicked);
}