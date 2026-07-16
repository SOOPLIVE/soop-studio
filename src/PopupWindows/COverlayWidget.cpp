
#include "qstandarditemmodel.h" // QStandardItemModel and QStandardItem

#include "qt-wrappers.hpp"

#include "Utils/OverlayManager.h" // QT_UTF8, QTStr and ENUM_WINDOW_TYPE

#include "MainFrame/CMainFrame.h"

template<long long get_int(obs_data_t*, const char*),
	double get_double(obs_data_t*, const char*),
	const char* get_string(obs_data_t*, const char*),
	bool get_bool(obs_data_t*, const char*)>
static QVariant from_obs_data(obs_data_t* data, const char* name,
	obs_combo_format format) // properties-view.cpp (obs-studio)
{
	switch (format) {
	case OBS_COMBO_FORMAT_INT:
		return QVariant::fromValue(get_int(data, name));
	case OBS_COMBO_FORMAT_FLOAT:
		return QVariant::fromValue(get_double(data, name));
	case OBS_COMBO_FORMAT_STRING:
		return QByteArray(get_string(data, name));
	case OBS_COMBO_FORMAT_BOOL:
		return QVariant::fromValue(get_bool(data, name));
	default:
		return QVariant();
	}
}

static QVariant from_obs_data(obs_data_t* data, const char* name,
	obs_combo_format format) // properties-view.cpp (obs-studio)
{
	return from_obs_data<obs_data_get_int, obs_data_get_double,
		obs_data_get_string, obs_data_get_bool>(data, name,
			format);
}

static QVariant from_obs_data_autoselect(obs_data_t* data, const char* name,
	obs_combo_format format) // properties-view.cpp (obs-studio)
{
	return from_obs_data<
		obs_data_get_autoselect_int, obs_data_get_autoselect_double,
		obs_data_get_autoselect_string, obs_data_get_autoselect_bool>(
			data, name, format);
}

static QVariant propertyListToQVariant(obs_property_t* prop, size_t idx) // properties-view.cpp (obs-studio)
{
	obs_combo_format format = obs_property_list_format(prop);

	QVariant var;
	if (format == OBS_COMBO_FORMAT_INT) {
		long long val = obs_property_list_item_int(prop, idx);
		var = QVariant::fromValue<long long>(val);
	}
	else if (format == OBS_COMBO_FORMAT_FLOAT) {
		double val = obs_property_list_item_float(prop, idx);
		var = QVariant::fromValue<double>(val);
	}
	else if (format == OBS_COMBO_FORMAT_STRING) {
		var = QByteArray(obs_property_list_item_string(prop, idx));
	}
	else if (format == OBS_COMBO_FORMAT_BOOL) {
		bool val = obs_property_list_item_bool(prop, idx);
		var = QVariant::fromValue<bool>(val);
	}
	return var;
}

static void AddComboItem(QComboBox* combo, obs_property_t* prop, size_t idx) // properties-view.cpp (obs-studio)
{
	const char* name = obs_property_list_item_name(prop, idx);
	QVariant var = propertyListToQVariant(prop, idx);

	combo->addItem(QT_UTF8(name), var);

	if (!obs_property_list_item_disabled(prop, idx))
		return;

	int index = combo->findText(QT_UTF8(name));
	if (index < 0)
		return;

	QStandardItemModel* model =
		dynamic_cast<QStandardItemModel*>(combo->model());
	if (!model)
		return;

	QStandardItem* item = model->item(index);
	item->setFlags(Qt::NoItemFlags);
}

static void AddList(obs_data_t* settings, obs_properties_t* props, QComboBox* combo, const char* name) // properties-view.cpp (obs-studio)
{
#if 1
	auto prop = obs_properties_get(props, name);
#endif // 1
	//const char* name = obs_property_name(prop);
	obs_combo_type type = obs_property_list_type(prop);
	obs_combo_format format = obs_property_list_format(prop);
	size_t count = obs_property_list_item_count(prop);

	QVariant value = from_obs_data(settings, name, format);

	//if (type == OBS_COMBO_TYPE_RADIO) {
	//    QButtonGroup* buttonGroup = new QButtonGroup();
	//    QFormLayout* subLayout = new QFormLayout();
	//    subLayout->setContentsMargins(0, 0, 0, 0);

	//    for (size_t idx = 0; idx < count; idx++)
	//        AddRadioItem(buttonGroup, subLayout, prop, value, idx);

	//    if (count > 0) {
	//        buttonGroup->setExclusive(true);
	//        WidgetInfo* info = new WidgetInfo(
	//            this, prop, buttonGroup->buttons()[0]);
	//        children.emplace_back(info);
	//        connect(buttonGroup, &QButtonGroup::buttonClicked, info,
	//            &WidgetInfo::ControlChanged);
	//    }

	//    QWidget* widget = new QWidget();
	//    widget->setLayout(subLayout);
	//    return widget;
	//}

	int idx = -1;

	//QComboBox* combo = new QComboBox();
	for (size_t i = 0; i < count; i++)
		AddComboItem(combo, prop, i);

	//if (type == OBS_COMBO_TYPE_EDITABLE)
	//    combo->setEditable(true);

	combo->setMaxVisibleItems(40);
	combo->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	if (format == OBS_COMBO_FORMAT_STRING &&
		type == OBS_COMBO_TYPE_EDITABLE) {
		//combo->lineEdit()->setText(value.toString());
	}
	else {
		idx = combo->findData(value);
	}

	//if (type == OBS_COMBO_TYPE_EDITABLE)
	//    return NewWidget(prop, combo, &QComboBox::editTextChanged);

	if (idx != -1)
		combo->setCurrentIndex(idx);

	if (obs_data_has_autoselect_value(settings, name)) {
		QVariant autoselect =
			from_obs_data_autoselect(settings, name, format);
		int id = combo->findData(autoselect);

		if (id != -1 && id != idx) {
			QString actual = combo->itemText(id);
			QString selected = combo->itemText(idx);
			QString combined = QT_UTF8( // QString combined = QTStr(
				"Basic.PropertiesWindow.AutoSelectFormat");
			combo->setItemText(idx,
				combined.arg(selected).arg(actual));
		}
	}

	//QAbstractItemModel* model = combo->model();
	//warning = idx != -1 &&
	//    model->flags(model->index(idx, 0)) == Qt::NoItemFlags;

	//WidgetInfo* info = new WidgetInfo(this, prop, combo);
	//connect(combo, &QComboBox::currentIndexChanged, info,
	//    &WidgetInfo::ControlChanged);
	//children.emplace_back(info);

	///* trigger a settings update if the index was not found */
	//if (count && idx == -1)
	//    info->ControlChanged();

	//return combo;
}

static void ListChanged(obs_properties_t* props, obs_data_t* settings, QComboBox* combo, const char* setting) // properties-view.cpp (obs-studio)
{
#if 1
	auto property = obs_properties_get(props, setting);
#endif // 1
	obs_combo_format format = obs_property_list_format(property);
	obs_combo_type type = obs_property_list_type(property);
	QVariant data;

	if (type == OBS_COMBO_TYPE_RADIO) {
		//QButtonGroup* group =
		//    static_cast<QAbstractButton*>(widget)->group();
		//QAbstractButton* button = group->checkedButton();
		//data = button->property("value");
	}
	else if (type == OBS_COMBO_TYPE_EDITABLE) {
		//data = static_cast<QComboBox*>(widget)->currentText().toUtf8();
	}
	else {
		//QComboBox* combo = static_cast<QComboBox*>(widget);
		int index = combo->currentIndex();
		if (index != -1)
			data = combo->itemData(index);
		else
			return;
	}

	switch (format) {
	case OBS_COMBO_FORMAT_INVALID:
		return;
	case OBS_COMBO_FORMAT_INT:
		obs_data_set_int(settings, setting, // obs_data_set_int(view->settings, setting,
			data.value<long long>());
		break;
	case OBS_COMBO_FORMAT_FLOAT:
		obs_data_set_double(settings, setting, // obs_data_set_double(view->settings, setting,
			data.value<double>());
		break;
	case OBS_COMBO_FORMAT_STRING:
		obs_data_set_string(settings, setting, // obs_data_set_string(view->settings, setting,
			data.toByteArray().constData());
		break;
	case OBS_COMBO_FORMAT_BOOL:
		obs_data_set_bool(settings, setting, // obs_data_set_bool(view->settings, setting,
			data.value<double>());
		break;
	}

	//

	obs_property_modified(property, settings);
}

#include "COverlayWidget.h"

#include <util/base.h>

void AFQOverlayWidget::_qslotWindowCurrentIndexChanged(int index)
{
	if (index != -1)
	{
		OBSPropertiesAutoDestroy props = obs_source_properties(source);
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		ListChanged(props, settings, ui->comboBox_window, "window");
		auto window = obs_data_get_string(settings, "window");
		OVERLAY_MANAGER.Window(window);
	}
}

void AFQOverlayWidget::_qslotWindowbeforeShowPopup()
{
	//blog(LOG_DEBUG, "%s (%d) : %s", __FILE__, __LINE__, __FUNCTION__);

	do
	{
		if (ui == nullptr)
		{
			blog(LOG_ERROR, "%s (%d) : ui : %d", __FILE__, __LINE__, ui);
			break;
		}

		do
		{
			if (ui->comboBox_window == nullptr)
			{
				blog(LOG_ERROR, "%s (%d) : ui->comboBox_window : %d", __FILE__, __LINE__, ui->comboBox_window);
				break;
			}

			ui->comboBox_window->blockSignals(true);
			ui->comboBox_window->clear();
			OBSDataAutoRelease settings = obs_source_get_settings(source);
			OBSPropertiesAutoDestroy props = obs_source_properties(source);
			AddList(settings, props, ui->comboBox_window, "window");
			auto item = ui->comboBox_window->itemData(0);
			if (item.toString().isEmpty() == true)
				ui->comboBox_window->setItemText(0, QTStr("Popup.LiveOverlay.ComboboxWindow.Placeholder"));
			ui->comboBox_window->blockSignals(false);
		} while (false);


	} while (false);
}

void AFQOverlayWidget::_qslotRefreshHotkey()
{
	auto text = OVERLAY_MANAGER.EditableHotkey();
	ui->pushButton_hotkeysetting->setText(text.isEmpty() == true ? QTStr("Popup.LiveOverlay.HotkeySetting") : text);
}

AFQOverlayWidget::AFQOverlayWidget() : QWidget(nullptr, Qt::FramelessWindowHint), ui(new Ui::AFQOverlayWidget)
{
	ui->setupUi(this);

	ui->pushButton_Tooltip->SetExplanationText(QTStr("Popup.LiveOverlay.Tooltip"),
		ENUM_TOOLTIP_POSITION::BottomCenter);

	connect(ui->toggleButtonOverlay, &AFQToggleButton::clicked,
		this, [this](bool checked)
		{
			OVERLAY_MANAGER.Enable(checked);
			this->ui->comboBox_window->setDisabled(!checked);
		});
	auto checked = OVERLAY_MANAGER.Enable();
	ui->toggleButtonOverlay->SetChecked(checked);

	//
	
	source = obs_source_create_private("window_capture", "overlay_window_capture", nullptr);
	OBSPropertiesAutoDestroy props = obs_source_properties(source);
	auto prop = obs_properties_get(props, "window");

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	auto window = OVERLAY_MANAGER.Window();
	obs_data_set_string(settings, "window", window);
	obs_property_modified(prop, settings);

	ui->comboBox_window->setDisabled(!checked);
	AddList(settings, props, ui->comboBox_window, "window");
	auto item = ui->comboBox_window->itemData(0);
	if (item.toString().isEmpty() == true)
		ui->comboBox_window->setItemText(0, QTStr("Popup.LiveOverlay.ComboboxWindow.Placeholder"));

	connect(ui->comboBox_window, &QComboBox::currentIndexChanged,
		this, &AFQOverlayWidget::_qslotWindowCurrentIndexChanged);
	connect(ui->comboBox_window, &AFQCustomCombobox::beforeShowPopup,
		this, &AFQOverlayWidget::_qslotWindowbeforeShowPopup);
#if 0
	auto priority = OVERLAY_MANAGER.Priority();
	obs_data_set_int(settings, "priority", priority);
	obs_property_modified(prop, settings);

	AddList(source, ui->comboBox_priority, "priority");
	connect(ui->comboBox_priority, &QComboBox::currentIndexChanged, this, &AFQOverlayWidget::qslotComboBox_PriorityCurrentIndexChanged);
#endif // 0
	//

	connect(ui->toggleButtonChat, &AFQToggleButton::clicked,
		this, [](bool checked) {OVERLAY_MANAGER.Chat(checked); });
	checked = OVERLAY_MANAGER.Chat();
	ui->toggleButtonChat->SetChecked(checked);

	connect(ui->toggleButtonBroadTime, &AFQToggleButton::clicked,
		this, [](bool checked) {OVERLAY_MANAGER.Time(checked); });
	checked = OVERLAY_MANAGER.Time();
	ui->toggleButtonBroadTime->SetChecked(checked);

	connect(ui->toggleButtonGiftCount, &AFQToggleButton::clicked,
		this, [](bool checked) {OVERLAY_MANAGER.Gift(checked); });
	checked = OVERLAY_MANAGER.Gift();
	ui->toggleButtonGiftCount->SetChecked(checked);

	connect(ui->toggleButtonUserCount, &AFQToggleButton::clicked,
		this, [](bool checked) {OVERLAY_MANAGER.User(checked); });
	checked = OVERLAY_MANAGER.User();
	ui->toggleButtonUserCount->SetChecked(checked);

	connect(ui->toggleButtonUpCount, &AFQToggleButton::clicked,
		this, [](bool checked) {OVERLAY_MANAGER.Up(checked); });
	checked = OVERLAY_MANAGER.Up();
	ui->toggleButtonUpCount->SetChecked(checked);

	//

	ui->pushButton_LockHotkeyInfo->SetExplanationText(QTStr("Popup.LiveOverlay.LockHotkeyInfo"),
		ENUM_TOOLTIP_POSITION::BottomCenter);

	ui->pushButton_hotkeysetting->setProperty("type", 5);
	_qslotRefreshHotkey();
	connect(ui->pushButton_hotkeysetting, &QPushButton::clicked, MAINFRAME, &AFMainFrame::qslotShowStudioSettingWithButtonSender);

	connect(ui->pushButton_initLayout, &QPushButton::clicked,
		this, [this]() {OVERLAY_MANAGER.Reset(this); });
}

void AFQOverlayWidget::closeEvent(QCloseEvent* event) {
	if (source)
		obs_source_release(source);

	emit qsignalCloseTriggered(ENUM_WINDOW_TYPE::SoopOverlay);
}
