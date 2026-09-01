//#include "properties-view.hpp"

#include <QFormLayout>
#include <QScrollBar>
#include <QLabel>
#include <QCheckBox>
#include <QFont>
#include <QFontDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QStandardItem>
#include <QFileDialog>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QMenu>
#include <QMessageBox>
#include <QStackedWidget>
#include <QDir>
#include <QGroupBox>
#include <QObject>
#include <QDesktopServices>
#include <QUuid>

#include "double-slider.hpp"
#include "spinbox-ignorewheel.hpp"
#include "moc_properties-view.cpp"
#include "properties-view.moc.hpp"

#include "qt-wrappers.hpp"
#include "plain-text-edit.hpp"
#include "slider-ignorewheel.hpp"
#include "icon-label.hpp"
#include <cstdlib>
#include <initializer_list>
#include <obs-data.h>
#include <obs.h>
#include <qtimer.h>
#include <string>
#include <obs-frontend-api.h>
#include <util/windows/window-helpers.h>

#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Profile/CProfile.h"
#include "CoreModel/Source/CSource.h"
#include "CoreModel/Encoder/CEncoder.h"

#include "UIComponent/CCustomSpinbox.h"
#include "UIComponent/CCustomCombobox.h"
#include "UIComponent/CCustomDoubleSpinbox.h"
#include "UIComponent/CCustomMenu.h"

#include "PopupWindows/CCustomColorDialog.h"
#include "PopupWindows/CCustomFontDialog.h"

static inline QColor color_from_int(long long val)
{
	return QColor(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24) & 0xff);
}

static inline long long color_to_int(QColor color)
{
	auto shift = [&](unsigned val, int shift) {
		return ((val & 0xff) << shift);
	};
	
	return shift(color.red(), 0) | shift(color.green(), 8) | shift(color.blue(), 16) | shift(color.alpha(), 24);
}

static inline void clearLayout(QLayout* layout) {
	while(QLayoutItem* item = layout->takeAt(0)) {
		if(QWidget* widget = item->widget()) {
			delete widget;
		} else if(QLayout* childLayout = item->layout()) {
			clearLayout(childLayout);
		}
	}
}

namespace {

	struct frame_rate_tag {
		enum tag_type {
			SIMPLE,
			RATIONAL,
			USER,
		} type = SIMPLE;
		const char* val = nullptr;

		frame_rate_tag() = default;

		explicit frame_rate_tag(tag_type type) : type(type) {}

		explicit frame_rate_tag(const char* val) : type(USER), val(val) {}

		static frame_rate_tag simple() { return frame_rate_tag {SIMPLE}; }
		static frame_rate_tag rational() { return frame_rate_tag {RATIONAL}; }
	};

	struct common_frame_rate {
		const char* fps_name;
		media_frames_per_second fps;
	};

} // namespace

Q_DECLARE_METATYPE(frame_rate_tag);
Q_DECLARE_METATYPE(media_frames_per_second);

void OBSPropertiesView::ReloadProperties()
{
	OBSSource source = getSourceObject();
	if(m_NotUpdateFFMPEGLIST && source &&
	   (strcmp(obs_source_get_id(source), "ffmpeg_list_source") == 0)) {
		return;
	}

	if(weakObj || rawObj) {
		OBSObject strongObj = GetOBSObject();
		void* obj = strongObj ? strongObj.Get() : rawObj;
		if(obj)
			properties.reset(reloadCallback(obj));
	} else {
		properties.reset(reloadCallback((void*)type.c_str()));
		obs_properties_apply_settings(properties.get(), settings);
	}

	uint32_t flags = obs_properties_get_flags(properties.get());
	deferUpdate = enableDefer && (flags & OBS_PROPERTIES_DEFER_UPDATE) != 0;

	if(source) {
		const char* sourceId = obs_source_get_id(source);
		if(isExistAdvanceMode(sourceId))
			setAdvancedPropertiesById(sourceId, m_advancedProps);

		if(0 == strcmp(sourceId, "window_area_capture")) {
			createAreaCaptureProp(source);
		}
	}

	RefreshProperties();
}

#define NO_PROPERTIES_STRING Str("Basic.PropertiesWindow.NoProperties")

void OBSPropertiesView::RefreshProperties()
{
	int h, v, hend, vend;
	GetScrollPos(h, v, hend, vend);

	widget->setUpdatesEnabled(false);

	children.clear();
	clearLayout(m_formLayout);

	{
        if(m_layoutMargin[0] > -1 && m_layoutMargin[1] > -1 && m_layoutMargin[2] > -1 && m_layoutMargin[3] > -1)
            m_formLayout->setContentsMargins(m_layoutMargin[0], m_layoutMargin[1], m_layoutMargin[2], m_layoutMargin[3]);
        if(m_formVSpacing > -1)
            m_formLayout->setVerticalSpacing(m_formVSpacing);
        if(m_formHSpacing > -1)
            m_formLayout->setHorizontalSpacing(m_formHSpacing);
	}

	obs_property_t* property = obs_properties_first(properties.get());
	bool hasNoProperties = !property;

	OBSSource source = getSourceObject();
	if(AFSourceUtil::ShouldShowContextPopupFromProps(source) && !hasNoProperties)
		addControlPanelButton(m_formLayout);

	while(property) {
		AddProperty(property, m_formLayout);
		obs_property_next(&property);
	}

	if(isHideAdvancedProperties() && !hasNoProperties)
		addAdvancedPropButton(m_formLayout);

	setScrollPos(h, v, hend, vend);

	lastFocused.clear();
	if(lastWidget) {
		lastWidget->setFocus(Qt::OtherFocusReason);
		lastWidget = nullptr;
	}

	if(hasNoProperties) {
		QLabel* noPropertiesLabel = new QLabel(Str("Basic.PropertiesWindow.NoProperties"));
		m_formLayout->addWidget(noPropertiesLabel);
	}

	widget->setLayout(m_formLayout);
	widget->setUpdatesEnabled(true);

	emit PropertiesRefreshed();
}

void OBSPropertiesView::showAdvancedProperties()
{
	OBSSource source = getSourceObject();
	const char* sourceId = obs_source_get_id(source);

	m_advancedProps = !m_advancedProps;

	if(isExistAdvanceMode(sourceId)) {
		setAdvancedPropertiesById(sourceId, m_advancedProps);
		config_set_bool(APPCONFIG, "PropertiesView-Mode", sourceId, m_advancedProps);
	}

	RefreshProperties();
}

void OBSPropertiesView::showControlPanel()
{
	OBSSource source = getSourceObject();
	const char* sourceId = obs_source_get_id(source);
	if(0 == strcmp(sourceId, "browser_source")) {
		MAINFRAME->ShowBrowserInteractionPopup(source);
		return;
	}

	//MAINFRAME->ShowOrUpdateContextPopup(source);
}

void OBSPropertiesView::SetScrollPos(int h, int v, int old_hend, int old_vend)
{
	QScrollBar* scroll = horizontalScrollBar();
	if(scroll) {
		int hend = scroll->maximum() + scroll->pageStep();
		scroll->setValue(h * hend / old_hend);
	}

	scroll = verticalScrollBar();
	if(scroll) {
		int vend = scroll->maximum() + scroll->pageStep();
		scroll->setValue(v * vend / old_vend);
	}
}

void OBSPropertiesView::GetScrollPos(int& h, int& v, int& hend, int& vend)
{
	h = v = 0;

	QScrollBar* scroll = horizontalScrollBar();
	if(scroll) {
		h = scroll->value();
		hend = scroll->maximum() + scroll->pageStep();
	}

	scroll = verticalScrollBar();
	if(scroll) {
		v = scroll->value();
		vend = scroll->maximum() + scroll->pageStep();
	}
}

OBSPropertiesView::OBSPropertiesView(OBSData settings_, obs_object_t* obj,
									 PropertiesReloadCallback reloadCallback,
									 PropertiesUpdateCallback callback_,
									 PropertiesVisualUpdateCb visUpdateCb_,
									 int minSize_)
	: VScrollArea(nullptr),
	properties(nullptr, obs_properties_destroy),
	settings(settings_),
	weakObj(obs_object_get_weak_object(obj)),
	reloadCallback(reloadCallback),
	callback(callback_),
	visUpdateCb(visUpdateCb_),
	minSize(minSize_),
	m_advancedProps(false)
{
	setFrameShape(QFrame::NoFrame);

	createPropsMainWidget();

	QMetaObject::invokeMethod(this, "ReloadProperties", Qt::QueuedConnection);
}

OBSPropertiesView::OBSPropertiesView(OBSData settings_, void* obj,
									 PropertiesReloadCallback reloadCallback,
									 PropertiesUpdateCallback callback_,
									 PropertiesVisualUpdateCb visUpdateCb_,
									 int minSize_)
	: VScrollArea(nullptr),
	properties(nullptr, obs_properties_destroy),
	settings(settings_),
	rawObj(obj),
	reloadCallback(reloadCallback),
	callback(callback_),
	visUpdateCb(visUpdateCb_),
	minSize(minSize_),
	m_advancedProps(false)
{
	setFrameShape(QFrame::NoFrame);

	createPropsMainWidget();

	QMetaObject::invokeMethod(this, "ReloadProperties", Qt::QueuedConnection);
}

OBSPropertiesView::OBSPropertiesView(OBSData settings_, const char* type_,
									 PropertiesReloadCallback reloadCallback_,
									 int minSize_, bool isEncorderProps)
	: VScrollArea(nullptr),
	properties(nullptr, obs_properties_destroy),
	settings(settings_),
	type(type_),
	reloadCallback(reloadCallback_),
	minSize(minSize_),
	m_advancedProps(false),
	m_isEncorderProps(isEncorderProps)
{
	setFrameShape(QFrame::NoFrame);

	createPropsMainWidget();

	QMetaObject::invokeMethod(this, "ReloadProperties", Qt::QueuedConnection);
}

OBSPropertiesView::~OBSPropertiesView()
{
}

void OBSPropertiesView::SetDisabled(bool disabled)
{
	for(auto child : findChildren<QWidget*>()) {
		child->setDisabled(disabled);
	}
}

void OBSPropertiesView::resizeEvent(QResizeEvent* event)
{
	VScrollArea::resizeEvent(event);
}

template<typename Sender, typename SenderParent, typename... Args>
QWidget* OBSPropertiesView::NewWidget(obs_property_t* prop, Sender* widget, void (SenderParent::* signal)(Args...))
{
	const char* long_desc = obs_property_long_description(prop);

	WidgetInfo* info = new WidgetInfo(this, prop, widget);
	QObject::connect(widget, signal, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);

	widget->setToolTip(QT_UTF8(long_desc));
	return widget;
}

QWidget* OBSPropertiesView::AddCheckbox(obs_property_t* prop)
{
	const char* name = obs_property_name(prop);
	const char* desc = obs_property_description(prop);
	const char* long_desc = obs_property_long_description(prop);
	bool val = obs_data_get_bool(settings, name);

	QCheckBox* checkbox = new QCheckBox(QT_UTF8(desc));
	checkbox->setCheckState(val ? Qt::Checked : Qt::Unchecked);

	if(m_checkBoxFixedHeight > -1)
		checkbox->setFixedHeight(m_checkBoxFixedHeight);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	QWidget* widget = NewWidget(prop, checkbox, &QCheckBox::checkStateChanged);
#else
	QWidget* widget = NewWidget(prop, checkbox, &QCheckBox::stateChanged);
#endif

	/*if(!long_desc) {
		return widget;
	}

	QString file = !obs_frontend_is_theme_dark() ? ":/res/images/help.svg" : ":/res/images/help_light.svg";

	IconLabel* help = new IconLabel(checkbox);
	help->setIcon(QIcon(file));
	help->setToolTip(long_desc);

#ifdef __APPLE__
	checkbox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif

	widget = new QWidget();
	QHBoxLayout* layout = new QHBoxLayout(widget);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setAlignment(Qt::AlignLeft);
	layout->setSpacing(0);

	layout->addWidget(checkbox);
	layout->addWidget(help);
	widget->setLayout(layout);*/

	return widget;
}

QWidget* OBSPropertiesView::AddText(obs_property_t* prop, QFormLayout* layout, QLabel*& label)
{
	const char* name = obs_property_name(prop);
	const char* val = obs_data_get_string(settings, name);
	bool monospace = obs_property_text_monospace(prop);
	obs_text_type type = obs_property_text_type(prop);

	if(type == OBS_TEXT_MULTILINE) {
		OBSPlainTextEdit* edit = new OBSPlainTextEdit(this, monospace);
		edit->setFixedHeight(80);
		edit->setPlainText(QT_UTF8(val));
		edit->setTabStopDistance(40);
		return NewWidget(prop, edit, &OBSPlainTextEdit::textChanged);
		
	} else if(type == OBS_TEXT_PASSWORD) {
		QLayout* subLayout = new QHBoxLayout();
		QLineEdit* edit = new QLineEdit();
		QPushButton* show = new QPushButton();

		show->setText(Str("Show"));
		show->setCheckable(true);
		edit->setText(QT_UTF8(val));
		edit->setEchoMode(QLineEdit::Password);

		subLayout->addWidget(edit);
		subLayout->addWidget(show);

		WidgetInfo* info = new WidgetInfo(this, prop, edit);
		connect(show, &QAbstractButton::toggled, info, &WidgetInfo::TogglePasswordText);
		connect(show, &QAbstractButton::toggled, [=](bool hide) {
			show->setText(hide ? Str("Hide") : Str("Show"));
		});
		children.emplace_back(info);

		label = new QLabel(QT_UTF8(obs_property_description(prop)));
		layout->addRow(label, subLayout);

		edit->setToolTip(QT_UTF8(obs_property_long_description(prop)));

		connect(edit, &QLineEdit::textEdited, info, &WidgetInfo::ControlChanged);
		return nullptr;

	} else if(type == OBS_TEXT_INFO) {
		QString desc = QT_UTF8(obs_property_description(prop));
		const char* long_desc = obs_property_long_description(prop);
		obs_text_info_type info_type = obs_property_text_info_type(prop);

		QLabel* info_label = new QLabel(QT_UTF8(val));

		if(info_label->text().isEmpty() && long_desc == NULL) {
			label = nullptr;
			info_label->setText(desc);
		} else
			label = new QLabel(desc);

		if(long_desc != NULL && !info_label->text().isEmpty()) {
			/*QString file = !obs_frontend_is_theme_dark() ? ":/res/images/help.svg"
				: ":/res/images/help_light.svg";
			QString lStr = "<html>%1 <img src='%2' style=' \
				vertical-align: bottom; ' /></html>";*/

			QString file = !true
				? ":/res/images/help.svg"
				: ":/res/images/help_light.svg";
			QString lStr = "<html>%1 <img src='%2' style=' \
				vertical-align: bottom; ' /></html>";

			info_label->setText(lStr.arg(info_label->text(), file));
			info_label->setToolTip(QT_UTF8(long_desc));

		} else if(long_desc != NULL) {
			info_label->setText(QT_UTF8(long_desc));
		}

		info_label->setMinimumHeight(40);
		info_label->setOpenExternalLinks(true);
		info_label->setWordWrap(obs_property_text_info_word_wrap(prop));

		if(info_type == OBS_TEXT_INFO_WARNING) {
			//info_label->setProperty("class", "text-warning");
			info_label->setObjectName("warningLabel");
		} else if(info_type == OBS_TEXT_INFO_ERROR) {
			//info_label->setProperty("class", "text-danger");
			info_label->setObjectName("errorLabel");
		}

		if(label)
			label->setObjectName(info_label->objectName());

		WidgetInfo* info = new WidgetInfo(this, prop, info_label);
		children.emplace_back(info);

		if(type != OBS_TEXT_INFO ||
		   info_type == OBS_TEXT_INFO_WARNING ||
		   info_type == OBS_TEXT_INFO_ERROR) {
			layout->addRow(label, info_label);
		} else { // type == OBS_TEXT_INFO
			if (label)
				layout->addRow(label, info_label);
			else
				layout->addRow(info_label);
		}

		return nullptr;
	}

	QLineEdit* edit = new QLineEdit();
	edit->setFixedHeight(40);

    //if(m_lineEditFixedSize.width() > -1 && m_lineEditFixedSize.height() > -1)
    //    edit->setFixedSize(m_lineEditFixedSize);

	edit->setText(QT_UTF8(val));
	edit->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	return NewWidget(prop, edit, &QLineEdit::textEdited);
}

void OBSPropertiesView::AddPath(obs_property_t* prop, QFormLayout* layout, QLabel** label)
{
	const char* name = obs_property_name(prop);
	const char* val = obs_data_get_string(settings, name);
	QLayout* subLayout = new QHBoxLayout();
	QLineEdit* edit = new QLineEdit();
	QPushButton* button = new QPushButton(Str("Browse"));
	button->setFixedSize(92, 40);

	if(!obs_property_enabled(prop)) {
		edit->setEnabled(false);
		button->setEnabled(false);
	}

	edit->setFixedHeight(40);
	edit->setText(QT_UTF8(val));
	edit->setReadOnly(true);
	edit->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	subLayout->addWidget(edit);
	subLayout->addWidget(button);

	WidgetInfo* info = new WidgetInfo(this, prop, edit);
	connect(button, &QPushButton::clicked, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);

	*label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(*label, subLayout);
}

void OBSPropertiesView::AddInt(obs_property_t* prop, QFormLayout* layout, QLabel** label)
{
	obs_number_type type = obs_property_int_type(prop);
	QLayout* subLayout = new QHBoxLayout();

	const char* name = obs_property_name(prop);
	int val = (int)obs_data_get_int(settings, name);
	QSpinBox* spin = new AFQCustomSpinbox();
	spin->setObjectName(name);
	//
	QString layoutName = QString("layout_%1").arg(name);
	subLayout->setObjectName(layoutName);

    //if(m_spinBoxFixedSize.width() > -1 && m_spinBoxFixedSize.height() > -1)
    //    spin->setFixedSize(m_spinBoxFixedSize);

    //if(m_spinBoxFixedSize.width() > -1 && m_spinBoxFixedSize.height() > -1)
    //    spin->setFixedSize(m_spinBoxFixedSize);

	spin->setFixedHeight(40);
	spin->setEnabled(obs_property_enabled(prop));

	int minVal = obs_property_int_min(prop);
	int maxVal = obs_property_int_max(prop);
	int stepVal = obs_property_int_step(prop);
	const char* suffix = obs_property_int_suffix(prop);

	spin->setMinimum(minVal);
	spin->setMaximum(maxVal);
	spin->setSingleStep(stepVal);
	spin->setValue(val);
	spin->setToolTip(QT_UTF8(obs_property_long_description(prop)));
	spin->setSuffix(QT_UTF8(suffix));

	WidgetInfo* info = new WidgetInfo(this, prop, spin);
	children.emplace_back(info);

	if(type == OBS_NUMBER_SLIDER) {
		QSlider* slider = new SliderIgnoreScroll();
		slider->setMinimum(minVal);
		slider->setMaximum(maxVal);
		slider->setPageStep(stepVal);
		slider->setValue(val);
		slider->setOrientation(Qt::Horizontal);
		slider->setEnabled(obs_property_enabled(prop));
		subLayout->addWidget(slider);

		connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
		connect(spin, &QSpinBox::valueChanged, slider, &QSlider::setValue);
	}

	connect(spin, &QSpinBox::valueChanged, info, &WidgetInfo::ControlChanged);

	subLayout->addWidget(spin);

	*label = new QLabel(QT_UTF8(obs_property_description(prop)));
	qDebug() << QT_UTF8(obs_property_description(prop));
	layout->addRow(*label, subLayout);
}

void OBSPropertiesView::AddFloat(obs_property_t* prop, QFormLayout* layout, QLabel** label)
{
	obs_number_type type = obs_property_float_type(prop);
	QLayout* subLayout = new QHBoxLayout();

	const char* name = obs_property_name(prop);
	double val = obs_data_get_double(settings, name);
	AFQCustomDoubleSpinbox* spin = new AFQCustomDoubleSpinbox();
	spin->setFixedHeight(40);

	if(!obs_property_enabled(prop))
		spin->setEnabled(false);

	double minVal = obs_property_float_min(prop);
	double maxVal = obs_property_float_max(prop);
	double stepVal = obs_property_float_step(prop);
	const char* suffix = obs_property_float_suffix(prop);

	if(stepVal < 1.0) {
		constexpr int sane_limit = 8;
		const int decimals = std::min<int>(log10(1.0 / stepVal) + 0.99, sane_limit);
		if(decimals > spin->decimals())
			spin->setDecimals(decimals);
	}

	spin->setMinimum(minVal);
	spin->setMaximum(maxVal);
	spin->setSingleStep(stepVal);
	spin->setValue(val);
	spin->setToolTip(QT_UTF8(obs_property_long_description(prop)));
	spin->setSuffix(QT_UTF8(suffix));

	WidgetInfo* info = new WidgetInfo(this, prop, spin);
	children.emplace_back(info);

	if(type == OBS_NUMBER_SLIDER) {
		DoubleSlider* slider = new DoubleSlider();
		slider->setDoubleConstraints(minVal, maxVal, stepVal, val);
		slider->setOrientation(Qt::Horizontal);
		slider->setEnabled(obs_property_enabled(prop));
		subLayout->addWidget(slider);

		connect(slider, &DoubleSlider::doubleValChanged, spin, &QDoubleSpinBox::setValue);
		connect(spin, &QDoubleSpinBox::valueChanged, slider, &DoubleSlider::setDoubleVal);
	}

	connect(spin, &QDoubleSpinBox::valueChanged, info, &WidgetInfo::ControlChanged);

	subLayout->addWidget(spin);

	*label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(*label, subLayout);
}

static QVariant propertyListToQVariant(obs_property_t* prop, size_t idx)
{
	obs_combo_format format = obs_property_list_format(prop);

	QVariant var;
	if(format == OBS_COMBO_FORMAT_INT) {
		long long val = obs_property_list_item_int(prop, idx);
		var = QVariant::fromValue<long long>(val);
	} else if(format == OBS_COMBO_FORMAT_FLOAT) {
		double val = obs_property_list_item_float(prop, idx);
		var = QVariant::fromValue<double>(val);
	} else if(format == OBS_COMBO_FORMAT_STRING) {
		var = QByteArray(obs_property_list_item_string(prop, idx));
	} else if(format == OBS_COMBO_FORMAT_BOOL) {
		bool val = obs_property_list_item_bool(prop, idx);
		var = QVariant::fromValue<bool>(val);
	}
	return var;
}

static void AddComboItem(QComboBox* combo, obs_property_t* prop, size_t idx)
{
	const char* name = obs_property_list_item_name(prop, idx);
	QVariant var = propertyListToQVariant(prop, idx);

	combo->addItem(QT_UTF8(name), var);

	if(!obs_property_list_item_disabled(prop, idx))
		return;

	int index = combo->findText(QT_UTF8(name));
	if(index < 0)
		return;

	QStandardItemModel* model = dynamic_cast<QStandardItemModel*>(combo->model());
	if(!model)
		return;

	QStandardItem* item = model->item(index);
	item->setFlags(Qt::NoItemFlags);
}

static void AddRadioItem(QButtonGroup* buttonGroup, QFormLayout* layout, obs_property_t* prop, QVariant value, size_t idx)
{
	const char* name = obs_property_list_item_name(prop, idx);

	QVariant var = propertyListToQVariant(prop, idx);
	QRadioButton* button = new QRadioButton(name);
	button->setChecked(value == var);
	button->setProperty("value", var);
	buttonGroup->addButton(button);
	layout->addRow(button);
}

template<long long get_int(obs_data_t*, const char*),
		 double get_double(obs_data_t*, const char*),
		 const char* get_string(obs_data_t*, const char*),
	     bool get_bool(obs_data_t*, const char*)>
static QVariant from_obs_data(obs_data_t* data, const char* name, obs_combo_format format)
{
	switch(format) {
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

static QVariant from_obs_data(obs_data_t* data, const char* name, obs_combo_format format)
{
	return from_obs_data<obs_data_get_int, obs_data_get_double, obs_data_get_string, obs_data_get_bool>(data, name, format);
}

static QVariant from_obs_data_autoselect(obs_data_t* data, const char* name, obs_combo_format format)
{
	return from_obs_data<obs_data_get_autoselect_int, obs_data_get_autoselect_double,
		obs_data_get_autoselect_string, obs_data_get_autoselect_bool>(data, name, format);
}

QWidget* OBSPropertiesView::AddList(obs_property_t* prop, bool& warning, bool group)
{
	const char* name = obs_property_name(prop);
	obs_combo_type type = obs_property_list_type(prop);
	obs_combo_format format = obs_property_list_format(prop);
	size_t count = obs_property_list_item_count(prop);

	QVariant value = from_obs_data(settings, name, format);

	if(type == OBS_COMBO_TYPE_RADIO) {
		QButtonGroup* buttonGroup = new QButtonGroup();
		QFormLayout* subLayout = new QFormLayout();
		subLayout->setContentsMargins(0, 0, 0, 0);

		for(size_t idx = 0; idx < count; idx++)
			AddRadioItem(buttonGroup, subLayout, prop, value, idx);

		if(count > 0) {
			buttonGroup->setExclusive(true);
			WidgetInfo* info = new WidgetInfo(this, prop, buttonGroup->buttons()[0]);
			children.emplace_back(info);
			connect(buttonGroup, &QButtonGroup::buttonClicked, info, &WidgetInfo::ControlChanged);
		}

		QWidget* widget = new QWidget();
		widget->setLayout(subLayout);
		return widget;
	}

	int idx = -1;

	QComboBox* combo = new AFQCustomCombobox();

	for(size_t i = 0; i < count; i++)
		AddComboItem(combo, prop, i);

	if(type == OBS_COMBO_TYPE_EDITABLE)
		combo->setEditable(true);

	combo->setFixedHeight(40);
	combo->setMaxVisibleItems(15);
	combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	combo->view()->setTextElideMode(Qt::TextElideMode::ElideRight);
	combo->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	if(format == OBS_COMBO_FORMAT_STRING &&
		type == OBS_COMBO_TYPE_EDITABLE) {
		combo->lineEdit()->setText(value.toString());
	} else {
		idx = combo->findData(value);
	}

	if(type == OBS_COMBO_TYPE_EDITABLE)
		return NewWidget(prop, combo, &QComboBox::editTextChanged);

	if(idx != -1)
		combo->setCurrentIndex(idx);

	if(obs_data_has_autoselect_value(settings, name)) {
		QVariant autoselect = from_obs_data_autoselect(settings, name, format);
		int id = combo->findData(autoselect);

		if(id != -1 && id != idx) {
			QString actual = combo->itemText(id);
			QString selected = combo->itemText(idx);
			QString combined = Str("Basic.PropertiesWindow.AutoSelectFormat");
			combo->setItemText(idx, combined.arg(selected).arg(actual));
		}
	}

	QAbstractItemModel* model = combo->model();
	warning = idx != -1 && model->flags(model->index(idx, 0)) == Qt::NoItemFlags;

	WidgetInfo* info = new WidgetInfo(this, prop, combo);
	connect(combo, &QComboBox::currentIndexChanged, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);

	/* trigger a settings update if the index was not found */
	if(count && idx == -1)
		info->ControlChanged();

	return combo;
}

static void NewButton(QLayout* layout, WidgetInfo* info, const char* themeIcon, void (WidgetInfo::* method)())
{
	QPushButton* button = new QPushButton();
	//button->setProperty("class", "btn-tool " + QString(themeIcon));
	button->setProperty("themeID", themeIcon);
	button->setFlat(true);
	button->setProperty("toolButton", true);
	button->setFixedSize(QSize(24, 24));
	button->setIconSize(QSize(24, 24));

	QObject::connect(button, &QPushButton::clicked, info, method);

	layout->addWidget(button);
}

void OBSPropertiesView::AddEditableList(obs_property_t* prop, QFormLayout* layout, QLabel*& label)
{
	const char* name = obs_property_name(prop);
	OBSDataArrayAutoRelease array = obs_data_get_array(settings, name);
	QListWidget* list = new QListWidget();
	size_t count = obs_data_array_count(array);

	if(!obs_property_enabled(prop))
		list->setEnabled(false);

	list->setSortingEnabled(false);
	list->setSelectionMode(QAbstractItemView::ExtendedSelection);
	list->setToolTip(QT_UTF8(obs_property_long_description(prop)));
	list->setSpacing(1);

	for(size_t i = 0; i < count; i++) {
		OBSDataAutoRelease item = obs_data_array_item(array, i);
		list->addItem(QT_UTF8(obs_data_get_string(item, "value")));
		QListWidgetItem* const list_item = list->item((int)i);
		list_item->setSelected(obs_data_get_bool(item, "selected"));
		list_item->setHidden(obs_data_get_bool(item, "hidden"));
		QString uuid = QT_UTF8(obs_data_get_string(item, "uuid"));
		/* for backwards compatibility */
		if(uuid.isEmpty()) {
			uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
			obs_data_set_string(item, "uuid", uuid.toUtf8());
		}
		list_item->setData(Qt::UserRole, uuid);
	}

	WidgetInfo* info = new WidgetInfo(this, prop, list);

	list->setDragDropMode(QAbstractItemView::InternalMove);
	connect(list->model(), &QAbstractItemModel::rowsMoved, [info]() {
		info->EditableListChanged();
	});

	QVBoxLayout* sideLayout = new QVBoxLayout();
	sideLayout->setContentsMargins(0, 0, 0, 0);
	NewButton(sideLayout, info, "addIconSmall", &WidgetInfo::EditListAdd);
	NewButton(sideLayout, info, "removeIconSmall", &WidgetInfo::EditListRemove);
	NewButton(sideLayout, info, "configIconSmall", &WidgetInfo::EditListEdit);
	NewButton(sideLayout, info, "upArrowIconSmall", &WidgetInfo::EditListUp);
	NewButton(sideLayout, info, "downArrowIconSmall", &WidgetInfo::EditListDown);
	sideLayout->addStretch(0);

	list->adjustSize();

	QHBoxLayout* subLayout = new QHBoxLayout();
	subLayout->addWidget(list);
	subLayout->addLayout(sideLayout);

	children.emplace_back(info);

	label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(label, subLayout);
}

QWidget* OBSPropertiesView::AddButton(obs_property_t* prop, QLabel*& label)
{
	const char* desc = obs_property_description(prop);

	QPushButton* button = new QPushButton(QT_UTF8(desc));
	button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
	button->setFixedHeight(40);

	const char* label_text = obs_property_label_text(prop);
	if(label_text) {
		label = new QLabel(QT_UTF8(label_text));
	}

	return NewWidget(prop, button, &QPushButton::clicked);
}

void OBSPropertiesView::AddColorInternal(obs_property_t* prop, QFormLayout* layout, QLabel*& label, bool supportAlpha)
{
	QPushButton* button = new QPushButton;
	QLabel* colorLabel = new QLabel;
	const char* name = obs_property_name(prop);
	long long val = obs_data_get_int(settings, name);
	QColor color = color_from_int(val);
	QColor::NameFormat format;

	if(!obs_property_enabled(prop)) {
		button->setEnabled(false);
		colorLabel->setEnabled(false);
	}

	button->setFixedHeight(40);
	button->setText(Str("Basic.PropertiesWindow.SelectColor"));
	button->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	if(supportAlpha) {
		format = QColor::HexArgb;
	} else {
		format = QColor::HexRgb;
		color.setAlpha(255);
	}

	QPalette palette = QPalette(color);
	colorLabel->setFixedHeight(40);
	colorLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	colorLabel->setText(color.name(format));
	colorLabel->setPalette(palette);
	colorLabel->setStyleSheet(QString("background-color :%1; color: %2;")
					  .arg(palette.color(QPalette::Window).name(format))
					  .arg(palette.color(QPalette::WindowText).name(format)));
	colorLabel->setAutoFillBackground(true);
	colorLabel->setAlignment(Qt::AlignCenter);
	colorLabel->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	QHBoxLayout* subLayout = new QHBoxLayout;
	subLayout->setContentsMargins(0, 0, 0, 0);

	subLayout->addWidget(colorLabel);
	subLayout->addWidget(button);

	WidgetInfo* info = new WidgetInfo(this, prop, colorLabel);
	connect(button, &QPushButton::clicked, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);

	label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(label, subLayout);
}

void OBSPropertiesView::AddColor(obs_property_t* prop, QFormLayout* layout, QLabel*& label)
{
	AddColorInternal(prop, layout, label, false);
}

void OBSPropertiesView::AddColorAlpha(obs_property_t* prop, QFormLayout* layout, QLabel*& label)
{
	AddColorInternal(prop, layout, label, true);
}

void MakeQFont(obs_data_t* font_obj, QFont& font, bool limit = false)
{
	const char* face = obs_data_get_string(font_obj, "face");
	const char* style = obs_data_get_string(font_obj, "style");
	int size = (int)obs_data_get_int(font_obj, "size");
	uint32_t flags = (uint32_t)obs_data_get_int(font_obj, "flags");

	if(face) {
		font.setFamily(face);
		font.setStyleName(style);
	}

	if(size) {
		if(limit) {
			int max_size = font.pointSize();
			if(max_size < 28)
				max_size = 28;
			if(size > max_size)
				size = max_size;
		}
		font.setPointSize(size);
	}

	if(flags & OBS_FONT_BOLD)
		font.setBold(true);
	if(flags & OBS_FONT_ITALIC)
		font.setItalic(true);
	if(flags & OBS_FONT_UNDERLINE)
		font.setUnderline(true);
	if(flags & OBS_FONT_STRIKEOUT)
		font.setStrikeOut(true);
}

void OBSPropertiesView::AddFont(obs_property_t* prop, QFormLayout* layout, QLabel*& label)
{
	const char* name = obs_property_name(prop);
	OBSDataAutoRelease font_obj = obs_data_get_obj(settings, name);
	const char* face = obs_data_get_string(font_obj, "face");
	const char* style = obs_data_get_string(font_obj, "style");
	QPushButton* button = new QPushButton;
	QLabel* fontLabel = new QLabel;
	QFont font;

	if(!obs_property_enabled(prop)) {
		button->setEnabled(false);
		fontLabel->setEnabled(false);
	}

	font = fontLabel->font();
	MakeQFont(font_obj, font, true);

	//button->setProperty("themeID", "settingsButtons");
	button->setFixedHeight(40);
	button->setText(Str("Basic.PropertiesWindow.SelectFont"));
	button->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	fontLabel->setFixedHeight(40);
	fontLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	fontLabel->setFont(font);
	fontLabel->setText(QString("%1 %2").arg(face, style));
	fontLabel->setAlignment(Qt::AlignCenter);
	fontLabel->setToolTip(QT_UTF8(obs_property_long_description(prop)));
	fontLabel->setObjectName("fontLabel");

	QHBoxLayout* subLayout = new QHBoxLayout;
	subLayout->setContentsMargins(0, 0, 0, 0);

	subLayout->addWidget(fontLabel);
	subLayout->addWidget(button);

	WidgetInfo* info = new WidgetInfo(this, prop, fontLabel);
	connect(button, &QPushButton::clicked, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);

	label = new QLabel(QT_UTF8(obs_property_description(prop)));
	layout->addRow(label, subLayout);
}

namespace std {

	template<> struct default_delete<obs_data_t> {
		void operator()(obs_data_t* data) { obs_data_release(data); }
	};

	template<> struct default_delete<obs_data_item_t> {
		void operator()(obs_data_item_t* item) { obs_data_item_release(&item); }
	};

} // namespace std

template<typename T> static double make_epsilon(T val)
{
	return val * 0.00001;
}

static bool matches_range(media_frames_per_second& match, media_frames_per_second fps, const frame_rate_range_t& pair)
{
	auto val = media_frames_per_second_to_frame_interval(fps);
	auto max_ = media_frames_per_second_to_frame_interval(pair.first);
	auto min_ = media_frames_per_second_to_frame_interval(pair.second);

	if(min_ <= val && val <= max_) {
		match = fps;
		return true;
	}

	return false;
}

static bool matches_ranges(media_frames_per_second& best_match, media_frames_per_second fps,
						   const frame_rate_ranges_t& fps_ranges, bool exact = false)
{
	auto convert_fn = media_frames_per_second_to_frame_interval;
	auto val = convert_fn(fps);
	auto epsilon = make_epsilon(val);

	bool match = false;
	auto best_dist = std::numeric_limits<double>::max();
	for(auto& pair : fps_ranges) {
		auto max_ = convert_fn(pair.first);
		auto min_ = convert_fn(pair.second);
		/*blog(LOG_INFO, "%lg <= %lg <= %lg? %s %s %s",
				min_, val, max_,
				fabsl(min_ - val) < epsilon ? "true" : "false",
				min_ <= val && val <= max_  ? "true" : "false",
				fabsl(min_ - val) < epsilon ? "true" :
				"false");*/

		if(matches_range(best_match, fps, pair))
			return true;

		if(exact)
			continue;

		auto min_dist = fabsl(min_ - val);
		auto max_dist = fabsl(max_ - val);
		if(min_dist < epsilon && min_dist < best_dist) {
			best_match = pair.first;
			match = true;
			continue;
		}

		if(max_dist < epsilon && max_dist < best_dist) {
			best_match = pair.second;
			match = true;
			continue;
		}
	}

	return match;
}

static media_frames_per_second make_fps(uint32_t num, uint32_t den)
{
	media_frames_per_second fps {};
	fps.numerator = num;
	fps.denominator = den;
	return fps;
}

static const common_frame_rate common_fps[] = {
	{"240", {240, 1}},         {"144", {144, 1}},
	{"120", {120, 1}},         {"119.88", {120000, 1001}},
	{"60", {60, 1}},           {"59.94", {60000, 1001}},
	{"50", {50, 1}},           {"48", {48, 1}},
	{"30", {30, 1}},           {"29.97", {30000, 1001}},
	{"25", {25, 1}},           {"24", {24, 1}},
	{"23.976", {24000, 1001}},
};

static void UpdateSimpleFPSSelection(OBSFrameRatePropertyWidget* fpsProps, const media_frames_per_second* current_fps)
{
	if(!current_fps || !media_frames_per_second_is_valid(*current_fps)) {
		fpsProps->simpleFPS->setCurrentIndex(0);
		return;
	}

	auto combo = fpsProps->simpleFPS;
	auto num = combo->count();
	for(int i = 0; i < num; i++) {
		auto variant = combo->itemData(i);
		if(!variant.canConvert<media_frames_per_second>())
			continue;

		auto fps = variant.value<media_frames_per_second>();
		if(fps != *current_fps)
			continue;

		combo->setCurrentIndex(i);
		return;
	}

	combo->setCurrentIndex(0);
}

static void AddFPSRanges(std::vector<common_frame_rate>& items, const frame_rate_ranges_t& ranges)
{
	auto InsertFPS = [&](media_frames_per_second fps) {
		auto fps_val = media_frames_per_second_to_fps(fps);

		auto end_ = end(items);
		auto i = begin(items);
		for(; i != end_; i++) {
			auto i_fps_val = media_frames_per_second_to_fps(i->fps);
			if(fabsl(i_fps_val - fps_val) < 0.01)
				return;

			if(i_fps_val > fps_val)
				continue;

			break;
		}

		items.insert(i, {nullptr, fps});
	};

	for(auto& range : ranges) {
		InsertFPS(range.first);
		InsertFPS(range.second);
	}
}

static QWidget* CreateSimpleFPSValues(OBSFrameRatePropertyWidget* fpsProps, bool& selected, const media_frames_per_second* current_fps)
{
	auto widget = new QWidget {};
	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto layout = new QVBoxLayout {};
	layout->setContentsMargins(0, 0, 0, 0);

	auto items = std::vector<common_frame_rate> {};
	items.reserve(sizeof(common_fps) / sizeof(common_frame_rate));

	auto combo = fpsProps->simpleFPS = new QComboBox();
	combo->setFixedHeight(40);

	combo->addItem("", QVariant::fromValue(make_fps(0, 0)));
	for(const auto& fps : common_fps) {
		media_frames_per_second best_match {};
		if(!matches_ranges(best_match, fps.fps, fpsProps->fps_ranges))
			continue;

		items.push_back({fps.fps_name, best_match});
	}

	AddFPSRanges(items, fpsProps->fps_ranges);

	for(const auto& item : items) {
		auto var = QVariant::fromValue(item.fps);
		auto name = item.fps_name ? QString(item.fps_name)
			: QString("%1").arg(media_frames_per_second_to_fps(item.fps));
		combo->addItem(name, var);

		bool select = current_fps && *current_fps == item.fps;
		if(select) {
			combo->setCurrentIndex(combo->count() - 1);
			selected = true;
		}
	}

	layout->addWidget(combo, 0, Qt::AlignTop);
	widget->setLayout(layout);

	return widget;
}

static void UpdateRationalFPSWidgets(OBSFrameRatePropertyWidget* fpsProps, const media_frames_per_second* current_fps)
{
	if(!current_fps || !media_frames_per_second_is_valid(*current_fps)) {
		fpsProps->numEdit->setValue(0);
		fpsProps->denEdit->setValue(0);
		return;
	}

	auto combo = fpsProps->fpsRange;
	auto num = combo->count();
	for(int i = 0; i < num; i++) {
		auto variant = combo->itemData(i);
		if(!variant.canConvert<size_t>())
			continue;

		auto idx = variant.value<size_t>();
		if(fpsProps->fps_ranges.size() < idx)
			continue;

		media_frames_per_second match {};
		if(!matches_range(match, *current_fps, fpsProps->fps_ranges[idx]))
			continue;

		combo->setCurrentIndex(i);
		break;
	}

	fpsProps->numEdit->setValue(current_fps->numerator);
	fpsProps->denEdit->setValue(current_fps->denominator);
}

static QWidget* CreateRationalFPS(OBSFrameRatePropertyWidget* fpsProps, bool& selected, const media_frames_per_second* current_fps)
{
	auto widget = new QWidget {};
	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto layout = new QFormLayout {};
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	auto str = QTStr("Basic.PropertiesView.FPS.ValidFPSRanges");
	auto rlabel = new QLabel {str};

	auto combo = fpsProps->fpsRange = new QComboBox();
	auto convert_fps = media_frames_per_second_to_fps;
	//auto convert_fi  = media_frames_per_second_to_frame_interval;

	combo->setFixedHeight(40);

	for(size_t i = 0; i < fpsProps->fps_ranges.size(); i++) {
		auto& pair = fpsProps->fps_ranges[i];
		combo->addItem(QString {"%1 - %2"}.arg(convert_fps(pair.first)).arg(convert_fps(pair.second)),
				   QVariant::fromValue(i));

		media_frames_per_second match;
		if(!current_fps || !matches_range(match, *current_fps, pair))
			continue;

		combo->setCurrentIndex(combo->count() - 1);
		selected = true;
	}

	layout->addRow(rlabel, combo);

	auto num_edit = fpsProps->numEdit = new AFQCustomSpinbox {};
	auto den_edit = fpsProps->denEdit = new AFQCustomSpinbox {};

	num_edit->setFixedHeight(40);
	den_edit->setFixedHeight(40);

	num_edit->setRange(0, INT_MAX);
	den_edit->setRange(0, INT_MAX);

	if(current_fps) {
		num_edit->setValue(current_fps->numerator);
		den_edit->setValue(current_fps->denominator);
	}

	layout->addRow(QTStr("Basic.Settings.Video.Numerator"), num_edit);
	layout->addRow(QTStr("Basic.Settings.Video.Denominator"), den_edit);

	widget->setLayout(layout);

	return widget;
}

static OBSFrameRatePropertyWidget* CreateFrameRateWidget(obs_property_t* prop, bool& warning, const char* option,
														 media_frames_per_second* current_fps, frame_rate_ranges_t& fps_ranges)
{
	auto widget = new OBSFrameRatePropertyWidget {};
	auto hlayout = new QHBoxLayout {};
	hlayout->setContentsMargins(0, 0, 0, 0);

	swap(widget->fps_ranges, fps_ranges);

	auto combo = widget->modeSelect = new QComboBox();
	combo->addItem(QTStr("Basic.PropertiesView.FPS.Simple"), QVariant::fromValue(frame_rate_tag::simple()));
	combo->addItem(QTStr("Basic.PropertiesView.FPS.Rational"), QVariant::fromValue(frame_rate_tag::rational()));

	combo->setFixedHeight(40);

	combo->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	auto num = obs_property_frame_rate_options_count(prop);
	if(num)
		combo->insertSeparator(combo->count());

	bool option_found = false;
	for(size_t i = 0; i < num; i++) {
		auto name = obs_property_frame_rate_option_name(prop, i);
		auto desc = obs_property_frame_rate_option_description(prop, i);
		combo->addItem(desc, QVariant::fromValue(frame_rate_tag {name}));

		if(!name || !option || std::string(name) != option)
			continue;

		option_found = true;
		combo->setCurrentIndex(combo->count() - 1);
	}

	hlayout->addWidget(combo, 0, Qt::AlignTop);

	auto stack = widget->modeDisplay = new QStackedWidget {};

	bool match_found = option_found;
	auto AddWidget = [&](decltype(CreateRationalFPS) func) {
		bool selected = false;
		stack->addWidget(func(widget, selected, current_fps));

		if(match_found || !selected)
			return;

		match_found = true;

		stack->setCurrentIndex(stack->count() - 1);
		combo->setCurrentIndex(stack->count() - 1);
	};

	AddWidget(CreateSimpleFPSValues);
	AddWidget(CreateRationalFPS);
	stack->addWidget(new QWidget {});

	if(option_found)
		stack->setCurrentIndex(stack->count() - 1);
	else if(!match_found) {
		int idx = current_fps ? 1 : 0; // Rational for "unsupported"
		// Simple as default
		stack->setCurrentIndex(idx);
		combo->setCurrentIndex(idx);
		warning = true;
	}

	hlayout->addWidget(stack, 0, Qt::AlignTop);

	auto label_area = widget->labels = new QWidget {};
	label_area->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto vlayout = new QVBoxLayout {};
	vlayout->setContentsMargins(0, 0, 0, 0);

	auto fps_label = widget->currentFPS = new QLabel {"FPS: 22"};
	auto time_label = widget->timePerFrame = new QLabel {"Frame Interval: 0.123 ms"};
	auto min_label = widget->minLabel = new QLabel {"Min FPS:\n 1/1"};
	auto max_label = widget->maxLabel = new QLabel {"Max FPS:\n 2/1"};

	min_label->setHidden(true);
	max_label->setHidden(true);

	auto flags = Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard;
	min_label->setTextInteractionFlags(flags);
	max_label->setTextInteractionFlags(flags);

	vlayout->addWidget(fps_label);
	vlayout->addWidget(time_label);
	vlayout->addWidget(min_label);
	vlayout->addWidget(max_label);
	label_area->setLayout(vlayout);

	hlayout->addWidget(label_area, 0, Qt::AlignTop);

	widget->setLayout(hlayout);

	return widget;
}

static void UpdateMinMaxLabels(OBSFrameRatePropertyWidget* w)
{
	auto Hide = [&](bool hide) {
		w->minLabel->setHidden(hide);
		w->maxLabel->setHidden(hide);
	};

	auto variant = w->modeSelect->currentData();
	if(!variant.canConvert<frame_rate_tag>() || variant.value<frame_rate_tag>().type != frame_rate_tag::RATIONAL) {
		Hide(true);
		return;
	}

	variant = w->fpsRange->currentData();
	if(!variant.canConvert<size_t>()) {
		Hide(true);
		return;
	}

	auto idx = variant.value<size_t>();
	if(idx >= w->fps_ranges.size()) {
		Hide(true);
		return;
	}

	Hide(false);

	auto min = w->fps_ranges[idx].first;
	auto max = w->fps_ranges[idx].second;

	w->minLabel->setText(QString("Min FPS:\n %1/%2")
						 .arg(min.numerator)
						 .arg(min.denominator));
	w->maxLabel->setText(QString("Max FPS:\n %1/%2")
						 .arg(max.numerator)
						 .arg(max.denominator));
}

static void UpdateFPSLabels(OBSFrameRatePropertyWidget* w)
{
	UpdateMinMaxLabels(w);

	std::unique_ptr<obs_data_item_t> obj {obs_data_item_byname(w->settings, w->name)};

	media_frames_per_second fps {};
	media_frames_per_second* valid_fps = nullptr;
	if(obs_data_item_get_autoselect_frames_per_second(obj.get(), &fps, nullptr) ||
		obs_data_item_get_frames_per_second(obj.get(), &fps, nullptr))
		valid_fps = &fps;

	const char* option = nullptr;
	obs_data_item_get_frames_per_second(obj.get(), nullptr, &option);

	if(!valid_fps) {
		w->currentFPS->setHidden(true);
		w->timePerFrame->setHidden(true);
		if(!option) {
			//w->warningLabel->setProperty("class", "text-danger");
			w->warningLabel->setObjectName("errorLabel");
		}

		return;
	}

	w->currentFPS->setHidden(false);
	w->timePerFrame->setHidden(false);

	media_frames_per_second match {};
	if(!option && !matches_ranges(match, *valid_fps, w->fps_ranges, true)) {
		//w->warningLabel->setProperty("class", "text-danger");
		w->warningLabel->setObjectName("errorLabel");
	} else {
		//w->warningLabel->setProperty("class", "");
		w->warningLabel->setObjectName("");
	}

	auto convert_to_fps = media_frames_per_second_to_fps;
	auto convert_to_frame_interval = media_frames_per_second_to_frame_interval;

	w->currentFPS->setText(QString("FPS: %1").arg(convert_to_fps(*valid_fps)));
	w->timePerFrame->setText(QString("Frame Interval: %1 ms").arg(convert_to_frame_interval(*valid_fps) * 1000));
}

void OBSPropertiesView::AddFrameRate(obs_property_t* prop, bool& warning, QFormLayout* layout, QLabel*& label)
{
	const char* name = obs_property_name(prop);
	bool enabled = obs_property_enabled(prop);
	std::unique_ptr<obs_data_item_t> obj {obs_data_item_byname(settings, name)};

	const char* option = nullptr;
	obs_data_item_get_frames_per_second(obj.get(), nullptr, &option);

	media_frames_per_second fps {};
	media_frames_per_second* valid_fps = nullptr;
	if(obs_data_item_get_frames_per_second(obj.get(), &fps, nullptr))
		valid_fps = &fps;

	frame_rate_ranges_t fps_ranges;
	size_t num = obs_property_frame_rate_fps_ranges_count(prop);
	fps_ranges.reserve(num);
	for(size_t i = 0; i < num; i++)
		fps_ranges.emplace_back(obs_property_frame_rate_fps_range_min(prop, i),
								obs_property_frame_rate_fps_range_max(prop, i));

	auto widget = CreateFrameRateWidget(prop, warning, option, valid_fps, fps_ranges);
	auto info = new WidgetInfo(this, prop, widget);

	widget->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	widget->name = name;
	widget->settings = settings;

	widget->modeSelect->setEnabled(enabled);
	widget->simpleFPS->setEnabled(enabled);
	widget->fpsRange->setEnabled(enabled);
	widget->numEdit->setEnabled(enabled);
	widget->denEdit->setEnabled(enabled);

	label = widget->warningLabel = new QLabel {obs_property_description(prop)};

	layout->addRow(label, widget);

	children.emplace_back(info);

	UpdateFPSLabels(widget);

	auto stack = widget->modeDisplay;
	auto combo = widget->modeSelect;

	stack->setToolTip(QT_UTF8(obs_property_long_description(prop)));
	combo->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	auto comboIndexChanged = static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
	connect(combo, comboIndexChanged, stack, [=](int index) {
		bool out_of_bounds = index >= stack->count();
		auto idx = out_of_bounds ? stack->count() - 1 : index;
		stack->setCurrentIndex(idx);

		if(widget->updating)
			return;

		UpdateFPSLabels(widget);
		emit info->ControlChanged();
	});

	connect(widget->simpleFPS, comboIndexChanged, [=](int) {
		if(widget->updating)
			return;

		emit info->ControlChanged();
	});

	connect(widget->fpsRange, comboIndexChanged, [=](int) {
		if(widget->updating)
			return;

		UpdateFPSLabels(widget);
	});

	auto sbValueChanged = static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged);
	connect(widget->numEdit, sbValueChanged, [=](int) {
		if(widget->updating)
			return;

		emit info->ControlChanged();
	});

	connect(widget->denEdit, sbValueChanged, [=](int) {
		if(widget->updating)
			return;

		emit info->ControlChanged();
	});
}

void OBSPropertiesView::AddGroup(obs_property_t* prop, QFormLayout* layout)
{
	const char* name = obs_property_name(prop);
	bool val = obs_data_get_bool(settings, name);
	const char* desc = obs_property_description(prop);
	enum obs_group_type type = obs_property_group_type(prop);

	// Create GroupBox
	QGroupBox* groupBox = new QGroupBox(QT_UTF8(desc));
	groupBox->setCheckable(type == OBS_GROUP_CHECKABLE);
	groupBox->setChecked(groupBox->isCheckable() ? val : true);
	groupBox->setAccessibleName("group");
	groupBox->setEnabled(obs_property_enabled(prop));

	// Create Layout and build content
	QFormLayout* subLayout = new QFormLayout();
	subLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	subLayout->setContentsMargins(0, 0, 0, 0);
	subLayout->setSpacing(10);
	groupBox->setLayout(subLayout);

	obs_properties_t* content = obs_property_group_content(prop);
	obs_property_t* el = obs_properties_first(content);
	while(el != nullptr) {
		AddProperty(el, subLayout, true);
		obs_property_next(&el);
	}

	// Insert into UI
	layout->setWidget(layout->rowCount(), QFormLayout::ItemRole::SpanningRole, groupBox);

	// Register Group Widget
	WidgetInfo* info = new WidgetInfo(this, prop, groupBox);
	children.emplace_back(info);

	// Signals
	connect(groupBox, &QGroupBox::toggled, info, &WidgetInfo::ControlChanged);
}

void OBSPropertiesView::AddProperty(obs_property_t* property, QFormLayout* layout, bool group)
{
	const char* name = obs_property_name(property);
	obs_property_type type = obs_property_get_type(property);

	if(!obs_property_visible(property))
		return;

	QLabel* label = nullptr;
	QWidget* widget = nullptr;
	bool warning = false;

	// Change Video Encorder Property (bitrate) 
	bool changeBitrateWidget = false;
	if(m_isEncorderProps) {
		if(0 == strcmp("bitrate", name) && type == OBS_PROPERTY_INT) {
			changeBitrateWidget = true;
		}
	}
	//}

	if(!changeBitrateWidget)
	{
		switch(type) {
			case OBS_PROPERTY_INVALID:
				return;
			case OBS_PROPERTY_BOOL:
				widget = AddCheckbox(property);
				break;
			case OBS_PROPERTY_INT:
				AddInt(property, layout, &label);
				break;
			case OBS_PROPERTY_FLOAT:
				AddFloat(property, layout, &label);
				break;
			case OBS_PROPERTY_TEXT:
				widget = AddText(property, layout, label);
				break;
			case OBS_PROPERTY_PATH:
				AddPath(property, layout, &label);
				break;
			case OBS_PROPERTY_LIST:
				widget = AddList(property, warning, group);
				break;
			case OBS_PROPERTY_COLOR:
				AddColor(property, layout, label);
				break;
			case OBS_PROPERTY_FONT:
				AddFont(property, layout, label);
				break;
			case OBS_PROPERTY_BUTTON:
				widget = AddButton(property, label);
				break;
			case OBS_PROPERTY_EDITABLE_LIST:
				AddEditableList(property, layout, label);
				break;
			case OBS_PROPERTY_FRAME_RATE:
				AddFrameRate(property, warning, layout, label);
				break;
			case OBS_PROPERTY_GROUP:
				AddGroup(property, layout);
				break;
			case OBS_PROPERTY_COLOR_ALPHA:
				AddColorAlpha(property, layout, label);
				break;

				// freecshot plus prop
			case OBS_PROPERTY_IMAGE_BUTTON_GROUP:
				AddImageButtonGroup(property, layout, label);
				break;
		}
	} else
	{
		widget = addEncorderBitrateList(property);
		m_advancedBitrateComboBox = reinterpret_cast<QComboBox*>(widget);
	}

	if(!widget && !label)
		return;

	if(!label && type != OBS_PROPERTY_BOOL &&
		type != OBS_PROPERTY_BUTTON && type != OBS_PROPERTY_GROUP)
		label = new QLabel(QT_UTF8(obs_property_description(property)));

	if(label) {
		if(minSize) {
			label->setFixedSize(minSize, 40);
			label->setMinimumWidth(minSize);
			label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		}

		if(!obs_property_enabled(property))
			label->setEnabled(false);
	}

	if(!widget)
		return;

	if(!obs_property_enabled(property))
		widget->setEnabled(false);

	layout->addRow(label, widget);


	if(!lastFocused.empty())
		if(lastFocused.compare(name) == 0)
			lastWidget = widget;
}

void OBSPropertiesView::SignalChanged()
{
	emit Changed();
}

static bool FrameRateChangedVariant(const QVariant& variant,
									media_frames_per_second& fps,
									obs_data_item_t*& obj,
									const media_frames_per_second* valid_fps)
{
	if(!variant.canConvert<media_frames_per_second>())
		return false;

	fps = variant.value<media_frames_per_second>();
	if(valid_fps && fps == *valid_fps)
		return false;

	obs_data_item_set_frames_per_second(&obj, fps, nullptr);
	return true;
}

static bool FrameRateChangedCommon(OBSFrameRatePropertyWidget* w, obs_data_item_t*& obj,
				   const media_frames_per_second* valid_fps)
{
	media_frames_per_second fps {};
	if(!FrameRateChangedVariant(w->simpleFPS->currentData(), fps, obj, valid_fps))
		return false;

	UpdateRationalFPSWidgets(w, &fps);
	return true;
}

static bool FrameRateChangedRational(OBSFrameRatePropertyWidget* w, obs_data_item_t*& obj, const media_frames_per_second* valid_fps)
{
	auto num = w->numEdit->value();
	auto den = w->denEdit->value();

	auto fps = make_fps(num, den);
	if(valid_fps && media_frames_per_second_is_valid(fps) && fps == *valid_fps)
		return false;

	obs_data_item_set_frames_per_second(&obj, fps, nullptr);
	UpdateSimpleFPSSelection(w, &fps);
	return true;
}

static bool FrameRateChanged(QWidget* widget, const char* name, OBSData& settings)
{
	auto w = qobject_cast<OBSFrameRatePropertyWidget*>(widget);
	if(!w)
		return false;

	auto variant = w->modeSelect->currentData();
	if(!variant.canConvert<frame_rate_tag>())
		return false;

	auto StopUpdating = [&](void*) {
		w->updating = false;
	};
	std::unique_ptr<void, decltype(StopUpdating)> signalGuard(static_cast<void*>(w), StopUpdating);
	w->updating = true;

	if(!obs_data_has_user_value(settings, name))
		obs_data_set_obj(settings, name, nullptr);

	std::unique_ptr<obs_data_item_t> obj {obs_data_item_byname(settings, name)};
	auto obj_ptr = obj.get();
	auto CheckObj = [&]() {
		if(!obj_ptr)
			obj.release();
	};

	const char* option = nullptr;
	obs_data_item_get_frames_per_second(obj.get(), nullptr, &option);

	media_frames_per_second fps {};
	media_frames_per_second* valid_fps = nullptr;
	if(obs_data_item_get_frames_per_second(obj.get(), &fps, nullptr))
		valid_fps = &fps;

	auto tag = variant.value<frame_rate_tag>();
	switch(tag.type) {
		case frame_rate_tag::SIMPLE:
			if(!FrameRateChangedCommon(w, obj_ptr, valid_fps))
				return false;
			break;

		case frame_rate_tag::RATIONAL:
			if(!FrameRateChangedRational(w, obj_ptr, valid_fps))
				return false;
			break;

		case frame_rate_tag::USER:
			if(tag.val && option && strcmp(tag.val, option) == 0)
				return false;

			obs_data_item_set_frames_per_second(&obj_ptr, {}, tag.val);
			break;
	}

	UpdateFPSLabels(w);
	CheckObj();
	return true;
}

void WidgetInfo::BoolChanged(const char* setting)
{
	QCheckBox* checkbox = static_cast<QCheckBox*>(widget);
	obs_data_set_bool(view->settings, setting, checkbox->checkState() == Qt::Checked);
}

void WidgetInfo::IntChanged(const char* setting)
{
	QSpinBox* spin = static_cast<QSpinBox*>(widget);
	obs_data_set_int(view->settings, setting, spin->value());
}

void WidgetInfo::FloatChanged(const char* setting)
{
	QDoubleSpinBox* spin = static_cast<QDoubleSpinBox*>(widget);
	obs_data_set_double(view->settings, setting, spin->value());
}

void WidgetInfo::TextChanged(const char* setting)
{
	obs_text_type type = obs_property_text_type(property);

	if(type == OBS_TEXT_MULTILINE) {
		OBSPlainTextEdit* edit = static_cast<OBSPlainTextEdit*>(widget);
		obs_data_set_string(view->settings, setting, QT_TO_UTF8(edit->toPlainText()));
		return;
	}

	QLineEdit* edit = static_cast<QLineEdit*>(widget);
	obs_data_set_string(view->settings, setting, QT_TO_UTF8(edit->text()));
}

bool WidgetInfo::PathChanged(const char* setting)
{
	const char* desc = obs_property_description(property);
	obs_path_type type = obs_property_path_type(property);
	const char* filter = obs_property_path_filter(property);
	const char* default_path = obs_property_path_default_path(property);

	QLineEdit* edit = static_cast<QLineEdit*>(widget);

	QString startDir = edit->text();
	if(startDir.isEmpty())
		startDir = default_path;

	QString path;

	if(type == OBS_PATH_DIRECTORY)
		path = SelectDirectory(view, QT_UTF8(desc), startDir);
	else if(type == OBS_PATH_FILE)
		path = OpenFile(view, QT_UTF8(desc), startDir, QT_UTF8(filter));
	else if(type == OBS_PATH_FILE_SAVE)
		path = SaveFile(view, QT_UTF8(desc), startDir, QT_UTF8(filter));

#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	widget->window()->raise();
#endif

	if(path.isEmpty())
		return false;

	edit->setText(path);
	obs_data_set_string(view->settings, setting, QT_TO_UTF8(path));
	return true;
}

void WidgetInfo::ListChanged(const char* setting)
{
	obs_combo_format format = obs_property_list_format(property);
	obs_combo_type type = obs_property_list_type(property);
	QVariant data;

	if(type == OBS_COMBO_TYPE_RADIO) {
		QButtonGroup* group = static_cast<QAbstractButton*>(widget)->group();
		QAbstractButton* button = group->checkedButton();
		data = button->property("value");
	} else if(type == OBS_COMBO_TYPE_EDITABLE) {
		data = static_cast<QComboBox*>(widget)->currentText().toUtf8();
	} else {
		QComboBox* combo = static_cast<QComboBox*>(widget);
		int index = combo->currentIndex();
		if(index != -1)
			data = combo->itemData(index);
		else
			return;
	}

	switch(format) {
		case OBS_COMBO_FORMAT_INVALID:
			return;
		case OBS_COMBO_FORMAT_INT:
			obs_data_set_int(view->settings, setting, data.value<long long>());
			break;
		case OBS_COMBO_FORMAT_FLOAT:
			obs_data_set_double(view->settings, setting, data.value<double>());
			break;
		case OBS_COMBO_FORMAT_STRING:
			obs_data_set_string(view->settings, setting, data.toByteArray().constData());
			break;
		case OBS_COMBO_FORMAT_BOOL:
			obs_data_set_bool(view->settings, setting, data.value<double>());
			break;
	}
}

bool WidgetInfo::ColorChangedInternal(const char* setting, bool supportAlpha)
{
	const char* desc = obs_property_description(property);
	long long val = obs_data_get_int(view->settings, setting);
	QColor color = color_from_int(val);
	QColor::NameFormat format;

	QColorDialog::ColorDialogOptions options;

	if(supportAlpha) {
		options |= QColorDialog::ShowAlphaChannel;
	}

#ifdef __linux__
	// TODO: Revisit hang on Ubuntu with native dialog
	options |= QColorDialog::DontUseNativeDialog;
#endif

	//color = QColorDialog::getColor(color, view, QT_UTF8(desc), options);
#ifdef _WIN32	
	color = AFQCustomColorDialog::getColor(color, view, Str("CustomColorDialog.title"), options);
#else
	color = QColorDialog::getColor(color, view, Str("CustomColorDialog.title"), options);
#endif

#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	widget->window()->raise();
#endif

	if(!color.isValid())
		return false;

	if(supportAlpha) {
		format = QColor::HexArgb;
	} else {
		color.setAlpha(255);
		format = QColor::HexRgb;
	}

	QLabel* label = static_cast<QLabel*>(widget);
	label->setText(color.name(format));
	QPalette palette = QPalette(color);
	label->setPalette(palette);
	label->setStyleSheet(QString("background-color :%1; color: %2;")
					 .arg(palette.color(QPalette::Window).name(format))
					 .arg(palette.color(QPalette::WindowText).name(format)));

	obs_data_set_int(view->settings, setting, color_to_int(color));

	return true;
}

bool WidgetInfo::ColorChanged(const char* setting)
{
	return ColorChangedInternal(setting, false);
}

bool WidgetInfo::ColorAlphaChanged(const char* setting)
{
	return ColorChangedInternal(setting, true);
}

bool WidgetInfo::FontChanged(const char* setting)
{
	OBSDataAutoRelease font_obj = obs_data_get_obj(view->settings, setting);
	bool success;
	uint32_t flags;
	QFont font;

	QFontDialog::FontDialogOptions options;

#ifndef _WIN32
	options = QFontDialog::DontUseNativeDialog;
#endif

	if(!font_obj) {
		QFont initial;
		font = AFQCustomFontDialog::getFont(&success, initial, view, QTStr("Basic.PropertiesWindow.SelectFont.WindowTitle"), options);
	} else {
		MakeQFont(font_obj, font);
		font = AFQCustomFontDialog::getFont(&success, font, view, QTStr("Basic.PropertiesWindow.SelectFont.WindowTitle"), options);
	}

	if(!success)
		return false;

	font_obj = obs_data_create();

	obs_data_set_string(font_obj, "face", QT_TO_UTF8(font.family()));
	obs_data_set_string(font_obj, "style", QT_TO_UTF8(font.styleName()));
	obs_data_set_int(font_obj, "size", font.pointSize());
	flags = font.bold() ? OBS_FONT_BOLD : 0;
	flags |= font.italic() ? OBS_FONT_ITALIC : 0;
	flags |= font.underline() ? OBS_FONT_UNDERLINE : 0;
	flags |= font.strikeOut() ? OBS_FONT_STRIKEOUT : 0;
	obs_data_set_int(font_obj, "flags", flags);

	QLabel* label = static_cast<QLabel*>(widget);
	QFont labelFont;
	MakeQFont(font_obj, labelFont, true);
	label->setFont(labelFont);
	label->setText(QString("%1 %2").arg(font.family(), font.styleName()));

	obs_data_set_obj(view->settings, setting, font_obj);
	return true;
}

void WidgetInfo::GroupChanged(const char* setting)
{
	QGroupBox* groupbox = static_cast<QGroupBox*>(widget);
	obs_data_set_bool(view->settings, setting, groupbox->isCheckable() ? groupbox->isChecked() : true);
}

void WidgetInfo::EditableListChanged()
{
	const char* setting = obs_property_name(property);
	QListWidget* list = reinterpret_cast<QListWidget*>(widget);
	OBSDataArrayAutoRelease array = obs_data_array_create();

	for(int i = 0; i < list->count(); i++) {
		QListWidgetItem* item = list->item(i);
		OBSDataAutoRelease arrayItem = obs_data_create();
		obs_data_set_string(arrayItem, "value", QT_TO_UTF8(item->text()));
		obs_data_set_string(arrayItem, "uuid", QT_TO_UTF8(item->data(Qt::UserRole).toString()));
		obs_data_set_bool(arrayItem, "selected", item->isSelected());
		obs_data_set_bool(arrayItem, "hidden", item->isHidden());
		obs_data_array_push_back(array, arrayItem);
	}

	obs_data_set_array(view->settings, setting, array);

	ControlChanged();
}

void WidgetInfo::ButtonClicked()
{
	obs_button_type type = obs_property_button_type(property);
	const char* savedUrl = obs_property_button_url(property);

	if(type == OBS_BUTTON_URL && strcmp(savedUrl, "") != 0) {
		QUrl url(savedUrl, QUrl::StrictMode);
		if(url.isValid() && (url.scheme().compare("http") == 0 || url.scheme().compare("https") == 0)) {
			QString msg(Str("Basic.PropertiesView.UrlButton.Text"));
			msg += "\n\n";
			msg += QString(Str("Basic.PropertiesView.UrlButton.Text.Url")).arg(savedUrl);

			/*QMessageBox::StandardButton button =
				OBSMessageBox::question(view->window(), QTStr("Basic.PropertiesView.UrlButton.OpenUrl"),
							msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if(button == QMessageBox::Yes)
				QDesktopServices::openUrl(url);*/
		}
		return;
	}

	OBSObject strongObj = view->GetOBSObject();
	void* obj = strongObj ? strongObj.Get() : view->rawObj;
	if(obs_property_button_clicked(property, obj)) {
		QMetaObject::invokeMethod(view, "RefreshProperties", Qt::QueuedConnection);
	}
}

void WidgetInfo::ImageButtonGroupItemClicked()
{
	OBSObject strongObj = view->GetOBSObject();
	void* obj = strongObj ? strongObj.Get() : view->rawObj;
	if(fs_property_image_button_group_item_clicked(property, obj)) {
		QMetaObject::invokeMethod(view, "RefreshProperties", Qt::QueuedConnection);
	}
}

void WidgetInfo::TogglePasswordText(bool show)
{
	reinterpret_cast<QLineEdit*>(widget)->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
}

void WidgetInfo::ControlChanged()
{
	const char* setting = obs_property_name(property);
	obs_property_type type = obs_property_get_type(property);

	if(!recently_updated) {
		old_settings_cache = obs_data_create();
		obs_data_apply(old_settings_cache, view->settings);
		obs_data_release(old_settings_cache);
	}

	switch(type) {
		case OBS_PROPERTY_INVALID:
			return;
		case OBS_PROPERTY_BOOL:
			BoolChanged(setting);
			break;
		case OBS_PROPERTY_INT:
			IntChanged(setting);
			break;
		case OBS_PROPERTY_FLOAT:
			FloatChanged(setting);
			break;
		case OBS_PROPERTY_TEXT:
			TextChanged(setting);
			break;
		case OBS_PROPERTY_LIST:
			ListChanged(setting);
			break;
		case OBS_PROPERTY_BUTTON:
			ButtonClicked();
			return;
		case OBS_PROPERTY_COLOR:
			if(!ColorChanged(setting))
				return;
			break;
		case OBS_PROPERTY_FONT:
			if(!FontChanged(setting))
				return;
			break;
		case OBS_PROPERTY_PATH:
			if(!PathChanged(setting))
				return;
			break;
		case OBS_PROPERTY_EDITABLE_LIST:
			break;
		case OBS_PROPERTY_FRAME_RATE:
			if(!FrameRateChanged(widget, setting, view->settings))
				return;
			break;
		case OBS_PROPERTY_GROUP:
			GroupChanged(setting);
			break;

		case OBS_PROPERTY_IMAGE_BUTTON_GROUP_ITEM:
			ImageButtonGroupItemClicked();
			break;

		case OBS_PROPERTY_COLOR_ALPHA:
			if(!ColorAlphaChanged(setting))
				return;
			break;
	}

	NotifyUpdateProperty(setting);
}
void WidgetInfo::EncorderBitrateChanged()
{
	const char* setting = obs_property_name(property);

	if(!recently_updated) {
		old_settings_cache = obs_data_create();
		obs_data_apply(old_settings_cache, view->settings);
		obs_data_release(old_settings_cache);
	}

	QVariant data;
	QComboBox* combo = static_cast<QComboBox*>(widget);
	int index = combo->currentIndex();
	if(index != -1)
		data = combo->itemData(index);
	else
		return;

	obs_data_set_int(view->settings, setting, data.value<long long>());

	NotifyUpdateProperty(setting);
}
void WidgetInfo::NotifyUpdateProperty(const char* propName)
{
	if(!recently_updated) {
		recently_updated = true;
		update_timer = new QTimer;
		connect(update_timer, &QTimer::timeout,
			[this, &ru = recently_updated]() {
			OBSObject strongObj = view->GetOBSObject();
			void* obj = strongObj ? strongObj.Get()
				: view->rawObj;
			if(obj && view->callback && !view->deferUpdate) {
				view->callback(obj, old_settings_cache, view->settings);
			}

			ru = false;
		});
		connect(update_timer, &QTimer::timeout, &QTimer::deleteLater);
		update_timer->setSingleShot(true);
	}

	if(update_timer) {
		update_timer->stop();
		update_timer->start(500);
	} else {
		blog(LOG_DEBUG, "No update timer or no callback!");
	}

	if(view->visUpdateCb && !view->deferUpdate) {
		OBSObject strongObj = view->GetOBSObject();
		void* obj = strongObj ? strongObj.Get() : view->rawObj;
		if(obj)
			view->visUpdateCb(obj, view->settings);
	}

	view->SignalChanged();

	if(obs_property_modified(property, view->settings)) {
		view->lastFocused = propName;
		QMetaObject::invokeMethod(view, "RefreshProperties", Qt::QueuedConnection);
	}
}

class EditableItemDialog : public QDialog {
	QLineEdit* edit;
	QString filter;
	QString default_path;

	void BrowseClicked()
	{
		QString curPath = QFileInfo(edit->text()).absoluteDir().path();

		if(curPath.isEmpty())
			curPath = default_path;

		/*QString path = OpenFile(this, QTStr("Browse"), curPath, filter);
		if(path.isEmpty())
			return;

		edit->setText(path);*/
	}

public:
	EditableItemDialog(QWidget* parent,
					   const QString& text,
					   bool browse,
					   const char* filter_ = nullptr,
					   const char* default_path_ = nullptr)
		: QDialog(parent),
		filter(QT_UTF8(filter_)),
		default_path(QT_UTF8(default_path_))
	{
		QHBoxLayout* topLayout = new QHBoxLayout();
		QVBoxLayout* mainLayout = new QVBoxLayout();

		edit = new QLineEdit();
		edit->setText(text);
		topLayout->addWidget(edit);
		topLayout->setAlignment(edit, Qt::AlignVCenter);

		if(browse) {
			/*QPushButton* browseButton = new QPushButton(QTStr("Browse"));
			topLayout->addWidget(browseButton);
			topLayout->setAlignment(browseButton, Qt::AlignVCenter);

			connect(browseButton, &QPushButton::clicked, this, &EditableItemDialog::BrowseClicked);*/
		}

		QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok | QDialogButtonBox::Cancel;

		QDialogButtonBox* buttonBox = new QDialogButtonBox(buttons);
		buttonBox->setCenterButtons(true);

		mainLayout->addLayout(topLayout);
		mainLayout->addWidget(buttonBox);

		setLayout(mainLayout);
		resize(QSize(400, 80));

		connect(buttonBox, &QDialogButtonBox::accepted, this, &EditableItemDialog::accept);
		connect(buttonBox, &QDialogButtonBox::rejected, this, &EditableItemDialog::reject);
	}

	inline QString GetText() const { return edit->text(); }
};

void WidgetInfo::EditListAdd()
{
	enum obs_editable_list_type type = obs_property_editable_list_type(property);

	if(type == OBS_EDITABLE_LIST_TYPE_STRINGS) {
		EditListAddText();
		return;
	}

	/* Files and URLs */
	AFQCustomMenu popup(view->window());

	QAction* action;

	action = new QAction(Str("Basic.PropertiesWindow.AddFiles"), this);
	connect(action, &QAction::triggered, this, &WidgetInfo::EditListAddFiles);
	popup.addAction(action);

	action = new QAction(Str("Basic.PropertiesWindow.AddDir"), this);
	connect(action, &QAction::triggered, this, &WidgetInfo::EditListAddDir);
	popup.addAction(action);

	if(type == OBS_EDITABLE_LIST_TYPE_FILES_AND_URLS) {
		action = new QAction(Str("Basic.PropertiesWindow.AddURL"), this);
		connect(action, &QAction::triggered, this, &WidgetInfo::EditListAddText);
		popup.addAction(action);
	}

	popup.exec(QCursor::pos());
}

void WidgetInfo::EditListAddText()
{
	QListWidget* list = reinterpret_cast<QListWidget*>(widget);
	const char* desc = obs_property_description(property);

	EditableItemDialog dialog(widget->window(), QString(), false);
	QString title = Str("Basic.PropertiesWindow.AddEditableListEntry");
	title = title.arg(QT_UTF8(desc));

	dialog.setWindowTitle(title);
	if(dialog.exec() == QDialog::Rejected)
		return;

	QString text = dialog.GetText();
	if(text.isEmpty())
		return;

	/*QListWidgetItem* item = new QListWidgetItem(text);
	item->setData(Qt::UserRole, QUuid::createUuid().toString(QUuid::WithoutBraces));
	list->addItem(item);*/

	list->addItem(text);
	EditableListChanged();
}

void WidgetInfo::EditListAddFiles()
{
	QListWidget* list = reinterpret_cast<QListWidget*>(widget);
	const char* desc = obs_property_description(property);
	const char* filter = obs_property_editable_list_filter(property);
	const char* default_path = obs_property_editable_list_default_path(property);

	QString title = Str("Basic.PropertiesWindow.AddEditableListFiles");
	title = title.arg(QT_UTF8(desc));

	void* ptr = view->GetOBSObject();
	OBSSource source = static_cast<obs_source_t*>(ptr);
	if(source) {
		if(strcmp(obs_source_get_id(source), "ffmpeg_list_source") == 0) {
			view->m_NotUpdateFFMPEGLIST = true;
		}
	}

	QStringList files = OpenFiles(list, title, QT_UTF8(default_path), QT_UTF8(filter));
#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	widget->window()->raise();
#endif

	if(files.count() == 0)
		return;

	for(QString file : files) {
		QListWidgetItem* item = new QListWidgetItem(file);
		item->setData(Qt::UserRole, QUuid::createUuid().toString(QUuid::WithoutBraces));
		list->addItem(item);
	}

	EditableListChanged();

	if(source) {
		if(strcmp(obs_source_get_id(source), "ffmpeg_list_source") == 0) {
			view->m_NotUpdateFFMPEGLIST = true;
		}
	}
}

void WidgetInfo::EditListAddDir()
{
	QListWidget* list = reinterpret_cast<QListWidget*>(widget);
	const char* desc = obs_property_description(property);
	const char* default_path = obs_property_editable_list_default_path(property);

	QString title = Str("Basic.PropertiesWindow.AddEditableListDir");
	title = title.arg(QT_UTF8(desc));

	void* ptr = view->GetOBSObject();
	OBSSource source = static_cast<obs_source_t*>(ptr);
	if(source) {
		if(strcmp(obs_source_get_id(source), "ffmpeg_list_source") == 0) {
			view->m_NotUpdateFFMPEGLIST = true;
		}
	}

	QString dir = SelectDirectory(list, title, QT_UTF8(default_path));
//#ifdef __APPLE__
	// TODO: Revisit when QTBUG-42661 is fixed
	widget->window()->raise();
//#endif

	if(dir.isEmpty())
		return;

	/*QListWidgetItem* item = new QListWidgetItem(dir);
	item->setData(Qt::UserRole, QUuid::createUuid().toString(QUuid::WithoutBraces));
	list->addItem(item);*/
	list->addItem(dir);
	EditableListChanged();

	if(source) {
		if(strcmp(obs_source_get_id(source), "ffmpeg_list_source") == 0) {
			view->m_NotUpdateFFMPEGLIST = true;
		}
	}
}

void WidgetInfo::EditListRemove()
{
	QListWidget* list = reinterpret_cast<QListWidget*>(widget);
	QList<QListWidgetItem*> items = list->selectedItems();

	for(QListWidgetItem* item : items)
		delete item;
	EditableListChanged();
}

void WidgetInfo::EditListEdit()
{
	QListWidget* list = reinterpret_cast<QListWidget*>(widget);
	enum obs_editable_list_type type = obs_property_editable_list_type(property);
	const char* desc = obs_property_description(property);
	const char* filter = obs_property_editable_list_filter(property);
	QList<QListWidgetItem*> selectedItems = list->selectedItems();

	if(!selectedItems.count())
		return;

	QListWidgetItem* item = selectedItems[0];

	if(type == OBS_EDITABLE_LIST_TYPE_FILES) {
		QDir pathDir(item->text());
		QString path;

		if(pathDir.exists())
			path = SelectDirectory(list, Str("Browse"), item->text());
		else
			path = OpenFile(list, Str("Browse"), item->text(), QT_UTF8(filter));

		if(path.isEmpty())
			return;

		item->setText(path);
		EditableListChanged();
		return;
	}

	EditableItemDialog dialog(widget->window(), item->text(), type != OBS_EDITABLE_LIST_TYPE_STRINGS, filter);
	QString title = Str("Basic.PropertiesWindow.EditEditableListEntry");
	title = title.arg(QT_UTF8(desc));
	dialog.setWindowTitle(title);
	if(dialog.exec() == QDialog::Rejected)
		return;

	QString text = dialog.GetText();
	if(text.isEmpty())
		return;

	item->setText(text);
	EditableListChanged();
}

void WidgetInfo::EditListUp()
{
	QListWidget* list = reinterpret_cast<QListWidget*>(widget);
	int lastItemRow = -1;

	for(int i = 0; i < list->count(); i++) {
		QListWidgetItem* item = list->item(i);
		if(!item->isSelected())
			continue;

		int row = list->row(item);

		if((row - 1) != lastItemRow) {
			lastItemRow = row - 1;
			list->takeItem(row);
			list->insertItem(lastItemRow, item);
			item->setSelected(true);
		} else {
			lastItemRow = row;
		}
	}

	EditableListChanged();
}

void WidgetInfo::EditListDown()
{
	QListWidget* list = reinterpret_cast<QListWidget*>(widget);
	int lastItemRow = list->count();

	for(int i = list->count() - 1; i >= 0; i--) {
		QListWidgetItem* item = list->item(i);
		if(!item->isSelected())
			continue;

		int row = list->row(item);

		if((row + 1) != lastItemRow) {
			lastItemRow = row + 1;
			list->takeItem(row);
			list->insertItem(lastItemRow, item);
			item->setSelected(true);
		} else {
			lastItemRow = row;
		}
	}

	EditableListChanged();
}

OBSSource OBSPropertiesView::getSourceObject()
{
	void* ptr = GetOBSObject().Get();
	OBSSource source = static_cast<obs_source_t*>(ptr);

	return source;
}
void OBSPropertiesView::createPropsMainWidget()
{
	OBSSource source = getSourceObject();
	if(source) {
		const char* sourceId = obs_source_get_id(source);
		m_advancedProps = config_get_bool(APPCONFIG, "PropertiesView-Mode", sourceId);
	}

	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	widget = new QWidget();
	widget->setObjectName("PropertiesContainer");

	m_formLayout = new QFormLayout(widget);
	m_formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	m_formLayout->setContentsMargins(24, 24, 24, 24);
	m_formLayout->setSpacing(10);
	m_formLayout->setLabelAlignment(Qt::AlignRight);

	widget->setLayout(m_formLayout);

	QSizePolicy mainPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	setWidgetResizable(true);
	setWidget(widget);
	setSizePolicy(mainPolicy);
}
void OBSPropertiesView::getScrollPos(int& h, int& v, int& hend, int& vend)
{
	h = v = 0;

	QScrollBar* scroll = horizontalScrollBar();
	if(scroll) {
		h = scroll->value();
		hend = scroll->maximum() + scroll->pageStep();
	}

	scroll = verticalScrollBar();
	if(scroll) {
		v = scroll->value();
		vend = scroll->maximum() + scroll->pageStep();
	}
}
void OBSPropertiesView::setScrollPos(int h, int v, int old_hend, int old_vend)
{
	QScrollBar* scroll = horizontalScrollBar();
	if(scroll) {
		int hend = scroll->maximum() + scroll->pageStep();
		scroll->setValue(h * hend / old_hend);
	}

	scroll = verticalScrollBar();
	if(scroll) {
		int vend = scroll->maximum() + scroll->pageStep();
		scroll->setValue(v * vend / old_vend);
	}
}

void OBSPropertiesView::createAreaCaptureProp(OBSSource source)
{
	QString captureWindow;
	bool is_desktop = obs_data_get_bool(settings, "desktop_monitor");
	if (is_desktop)
	{
		const int monitorIdx = obs_data_get_int(settings, "monitor");
		const QList<QScreen*> screens = QGuiApplication::screens();

		if (monitorIdx >= 0 && monitorIdx < screens.size()) {
			captureWindow =
				QString("[Desktop]: %1")
				.arg(screens[monitorIdx]->name());
		}
		else {
			captureWindow = "[Desktop]";
		}
	}
	else
	{
		std::string window = obs_data_get_string(settings, "window");

		char* className = nullptr;
		char* title = nullptr;
		char* executable = nullptr;
		ms_build_window_strings(window.c_str(), &className, &title, &executable);

		captureWindow = QString("[%1]: %2")
			.arg(executable ? executable : "")
			.arg(title ? title : "");

		bfree(className);
		bfree(title);
		bfree(executable);
	}

	obs_property_t* p = obs_properties_get(properties.get(), "capture_window");
	if (p) {

		obs_property_set_long_description(p, captureWindow.toStdString().c_str());
	}

	p = obs_properties_add_button2(
		properties.get(),
		"capture_button",
		QTStr("WindowAreaCapture.CaptureButton").toStdString().c_str(),
		[](obs_properties_t* props,
			obs_property_t* property,
			void* private_data) -> bool {

				OBSPropertiesView* view =
					static_cast<OBSPropertiesView*>(private_data);

				if (!view)
					return false;

				OBSSource source = view->getSourceObject();
				if (!source)
					return false;

				QPointer<OBSPropertiesView> safeView(view);

				MAINFRAME->ShowWindowCaptureArea(
					source,
					[safeView](const std::optional<WindowCaptureAreaResult>& captureResult) {
						if (!safeView)
							return;

						if (captureResult) {
							safeView->ReloadProperties();
						}
					});

				return false;
		}, this);

	obs_property_set_label_text(p, QTStr("WindowAreaCapture.CaptureText").toStdString().c_str());

	obs_properties_add_bool(properties.get(), "cursor", QTStr("WindowAreaCapture.CaptureCursor").toStdString().c_str());
}
// ===============================
// Add Custom Props Widget 
// ( Used SOOP Studio Source )
// ===============================
QWidget* OBSPropertiesView::addEncorderBitrateList(obs_property_t* prop)
{
	bool allowed1440p = false;
	bool allowedAV1 = false;
	//

	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	if(broadInfo) {
		allowed1440p = broadInfo->Allow1440P();
		allowedAV1 = broadInfo->AllowAV1();
	}
	bool isAV1Codec = (allowedAV1 ? AFEncoderUtil::isAV1Codec(type.c_str()) : false);
	//
	const char* name = obs_property_name(prop);
	int val = (int)obs_data_get_int(settings, name);

	OBSData streamEncSettings = AFProfileUtil::GetDataFromJsonFile("streamEncoder.json");
	int vCurBitrate = obs_data_get_int(streamEncSettings, "bitrate");
	int vPreBitrate = vCurBitrate;
	if(!allowed1440p && vCurBitrate > 8000) {
		vCurBitrate = 8000;
	} else if(isAV1Codec && vCurBitrate <= 8000) {
		vCurBitrate = 16000;
	}

	QComboBox* combo = new AFQCustomCombobox();
	auto addBitrate = [combo](int bitrate) {
		QString tmp = QString("%1 Kbps").arg(bitrate);

		if(combo->findText(tmp) == -1)
			combo->addItem(tmp, bitrate);
	};
	for(auto bitrate : bitrateList)
	{
		if(isAV1Codec) {
			if(!bitrate.is1440p) continue;
		} else
			if(bitrate.is1440p && !allowed1440p)
				continue;
		//
		addBitrate(bitrate.bitrate);
	}

	combo->setFixedHeight(40);
	combo->setMaxVisibleItems(15);
	combo->view()->setTextElideMode(Qt::TextElideMode::ElideRight);
	combo->setToolTip(QT_UTF8(obs_property_long_description(prop)));

	QVariant value;
	if(0 == vCurBitrate) {
		value = from_obs_data(settings, name, OBS_COMBO_FORMAT_INT);
	} else {
		value = QVariant::fromValue<long long>(vCurBitrate);
	}

	int idx = combo->findData(value);
	if(idx != -1)
		combo->setCurrentIndex(idx);

	WidgetInfo* info = new WidgetInfo(this, prop, combo);
	connect(combo, &QComboBox::currentIndexChanged, info, &WidgetInfo::EncorderBitrateChanged);

	children.emplace_back(info);

	if(combo->count() && idx == -1)
		info->EncorderBitrateChanged();

	if(vPreBitrate != vCurBitrate)
		info->EncorderBitrateChanged();

	return combo;
}

void OBSPropertiesView::AddImageButtonGroupItem(obs_property_t* prop, QGridLayout* layout, QButtonGroup* buttonGroup, int r, int c, bool checked)
{
	static const char* temp_qss = R"(QPushButton {
										background-color:#36383E;
									    border: 2px solid transparent;
									    border-radius: 4px;
									    padding: 0px;
									}
									QPushButton:checked {
									    border: 2px solid #0182FF;
									}
									)";


	const char* name = obs_property_name(prop);
	const char* desc = obs_property_description(prop);

	const char* image_path = fs_property_image_button_group_item_path(prop);
	int width = fs_property_image_button_group_item_width(prop);
	int height = fs_property_image_button_group_item_height(prop);

	QPushButton* button = new QPushButton();
	QIcon icon(image_path);
	button->setCheckable(true);
	button->setIcon(icon);
	button->setIconSize(QSize(width-4, height-4));
	button->setFixedSize(width, height);
	button->setStyleSheet(temp_qss);
	button->setChecked(checked);

	buttonGroup->addButton(button);

	layout->addWidget(button, r, c);

	WidgetInfo* info = new WidgetInfo(this, prop, button);
	connect(button, &QPushButton::clicked, info, &WidgetInfo::ControlChanged);
	children.emplace_back(info);
}

void OBSPropertiesView::AddImageButtonGroup(obs_property_t* prop, QFormLayout* layout, QLabel*& label)
{
	label = new QLabel(obs_property_description(prop));

	const char* name = obs_property_name(prop);
	int val = (int)obs_data_get_int(settings, name);

	QWidget* container = new QWidget();
	QHBoxLayout* outer = new QHBoxLayout(container);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->setSpacing(0);

	QWidget* gridWidget = new QWidget();
	QGridLayout* gridLayout = new QGridLayout(gridWidget);
	gridLayout->setContentsMargins(0, 0, 0, 0);

	int colums = fs_property_image_button_group_columns(prop);
	if(colums <= 0)
		colums = 1;

	gridLayout->setHorizontalSpacing(10);

	obs_properties_t* content = fs_property_image_button_group_content(prop);
	obs_property_t* el = obs_properties_first(content);

	QButtonGroup* buttonGroup = new QButtonGroup(container);
	buttonGroup->setExclusive(true);

	int idx = 0;
	while(el != nullptr) {
		const int r = idx / colums;
		const int c = idx % colums;
		bool checked = (val == idx);

		AddImageButtonGroupItem(el, gridLayout, buttonGroup, r, c, checked);
		obs_property_next(&el);
		idx++;
	}

	outer->addWidget(gridWidget);
	outer->addStretch(1);

	layout->addRow(label, container);

	WidgetInfo* info = new WidgetInfo(this, prop, container);
	children.emplace_back(info);
}

// ===============================
// PropertiesView UI
// (advanced mode, fixed widget size )
// ===============================
void OBSPropertiesView::addControlPanelButton(QFormLayout* layout)
{
	QIcon icon;
	QString buttonText;

	OBSSource source = getSourceObject();
	const char* sourceId = obs_source_get_id(source);

	if(0 != strcmp(sourceId, "browser_source"))
		return;

	LoadIconFromABSPath("assets/source-props/ic_source_interact.svg", icon);
	buttonText = Str("Interact");

	QPushButton* controlButton = new QPushButton(this);
	controlButton->setObjectName("controlButton");
	controlButton->setIcon(icon);
	controlButton->setIconSize(QSize(24, 24));
	controlButton->setText(buttonText);
	controlButton->setFixedHeight(40);
	controlButton->setIcon(icon);

	connect(controlButton, &QPushButton::clicked, this, &OBSPropertiesView::showControlPanel);

	layout->addRow(controlButton);
}

void OBSPropertiesView::addAdvancedPropButton(QFormLayout* layout)
{
	QFrame* advanceLine = new QFrame;
	advanceLine->setObjectName("advanceLine");
	advanceLine->setFixedHeight(1);

	QPushButton* advancedButton = new QPushButton;
	advancedButton->setObjectName("advancedButton");
	advancedButton->setLayoutDirection(Qt::RightToLeft);

	if(m_advancedProps) {
		advancedButton->setProperty("spread", true);
		advancedButton->setText("Simple ");
	} else {
		advancedButton->setProperty("spread", false);
		advancedButton->setText("Advanced ");
	}

	PolishStyleSheet(advancedButton);

	advancedButton->setFixedHeight(40);

	connect(advancedButton, &QPushButton::clicked, this, &OBSPropertiesView::showAdvancedProperties);

	layout->addRow(advanceLine);
	layout->addRow(advancedButton);
}
bool OBSPropertiesView::isHideAdvancedProperties()
{
	OBSSource source = getSourceObject();
	QString sourceId = obs_source_get_id(source);

	if(0 == sourceId.compare("game_capture"))
		return true;

	if(0 == sourceId.compare("window_capture"))
		return true;

	if(0 == sourceId.compare("dshow_input"))
		return true;

	if(0 == sourceId.compare("ffmpeg_source"))
		return true;

	if(0 == sourceId.compare("text_gdiplus_v2"))
		return true;

	if(0 == sourceId.compare("browser_source"))
		return true;

	if(0 == sourceId.compare("ffmpeg_list_source"))
		return true;

	return false;
}

bool OBSPropertiesView::isExistAdvanceMode(QString id)
{
	if(0 == id.compare("game_capture"))
		return true;

	if(0 == id.compare("window_capture"))
		return true;

	if(0 == id.compare("dshow_input"))
		return true;

	if(0 == id.compare("ffmpeg_source"))
		return true;

	if(0 == id.compare("text_gdiplus_v2"))
		return true;

	if(0 == id.compare("browser_source"))
		return true;

	if(0 == id.compare("ffmpeg_list_source"))
		return true;

	return false;
}
void OBSPropertiesView::setAdvancedPropertiesById(QString id, bool showAdvancedProps)
{
	if(0 == id.compare("game_capture"))
		setGameCaptureProps(showAdvancedProps);

	if(0 == id.compare("window_capture"))
		setWindowCaptureProps(showAdvancedProps);

	if(0 == id.compare("dshow_input"))
		setDshowInputProps(showAdvancedProps);

	if(0 == id.compare("ffmpeg_source"))
		setFFmpegSourceProps(showAdvancedProps);

	if(0 == id.compare("ffmpeg_list_source"))
		setFFmpegListSourceProps(showAdvancedProps);

	if(0 == id.compare("text_gdiplus_v2"))
		setTextGdiPlusProps(showAdvancedProps);

	if(0 == id.compare("browser_source"))
		setBrowserSourceProps(showAdvancedProps);
}

#define set_visible_prop(val, show)										\
	do {																\
		obs_property_t* p = obs_properties_get(properties.get(), val);	\
		obs_property_set_visible(p, show);								\
	} while (false)

void OBSPropertiesView::setGameCaptureProps(bool showProp)
{
	const char* capture_mode = obs_data_get_string(settings, "capture_mode");
	if(0 == strcmp(capture_mode, "window")) {
		obs_property_t* p = obs_properties_get(properties.get(), "window");
		obs_property_modified(p, settings);
	}

	set_visible_prop("sli_compatibility", showProp);
	set_visible_prop("allow_transparency", showProp);
	set_visible_prop("limit_framerate", showProp);
	set_visible_prop("capture_cursor", showProp);
	set_visible_prop("anti_cheat_hook", showProp);
	set_visible_prop("capture_overlays", showProp);
	set_visible_prop("hook_rate", showProp);
	set_visible_prop("rgb10a2_space", showProp);
}
void OBSPropertiesView::setWindowCaptureProps(bool showProp)
{
	set_visible_prop("method", showProp);
	set_visible_prop("priority", showProp);
}
void OBSPropertiesView::setWindowAreaCaptureProps(bool showProp)
{
	set_visible_prop("method", showProp);
	set_visible_prop("priority", showProp);
}
void OBSPropertiesView::setDshowInputProps(bool showProp)
{
	set_visible_prop("activate", showProp);
	set_visible_prop("video_config", showProp);
	set_visible_prop("xbar_config", showProp);
	set_visible_prop("deactivate_when_not_showing", showProp);
	set_visible_prop("res_type", showProp);
	set_visible_prop("resolution", showProp);
	set_visible_prop("frame_interval", showProp);
	set_visible_prop("video_format", showProp);
	set_visible_prop("color_space", showProp);
	set_visible_prop("color_range", showProp);
	set_visible_prop("buffering", showProp);
	set_visible_prop("flip_vertically", showProp);
	set_visible_prop("autorotation", showProp);
	set_visible_prop("hw_decode", showProp);
	set_visible_prop("audio_output_mode", showProp);
	set_visible_prop("use_custom_audio_device", showProp);

	std::string id = GetChannelId(PLATFORM_SOOP);
	bool enableLimitFrame = false;
	for(int i = 0; i < sizeof(limitFrameUserList) / sizeof(limitFrameUserList[0]); i++)
	{
		if(0 == id.compare(limitFrameUserList[i])) {
			enableLimitFrame = true;
			break;
		}
	}

	if(enableLimitFrame) {
		set_visible_prop("limit_frame", showProp);
		set_visible_prop("use_limit_frame", showProp);
	}

	bool useCustomAudio = obs_data_get_bool(settings, "use_custom_audio_device");
	bool showAudioDeviceIdProp = useCustomAudio && showProp;
	set_visible_prop("audio_device_id", showAudioDeviceIdProp);
}
void OBSPropertiesView::setFFmpegSourceProps(bool showProp)
{
	set_visible_prop("restart_on_activate", showProp);
	set_visible_prop("hw_decode", showProp);
	set_visible_prop("clear_on_media_end", showProp);
	set_visible_prop("close_when_inactive", showProp);
	set_visible_prop("color_range", showProp);
	set_visible_prop("linear_alpha", showProp);
	set_visible_prop("ffmpeg_options", showProp);
}
void OBSPropertiesView::setFFmpegListSourceProps(bool showProp)
{
	set_visible_prop("current_file_name", false);
	set_visible_prop("restart_on_activate", showProp);
	set_visible_prop("hw_decode", showProp);
	set_visible_prop("clear_on_media_end", showProp);
	set_visible_prop("close_when_inactive", showProp);
	set_visible_prop("color_range", showProp);
	set_visible_prop("linear_alpha", showProp);
	set_visible_prop("ffmpeg_options", showProp);
}
void OBSPropertiesView::setTextGdiPlusProps(bool showProp)
{
	set_visible_prop("antialiasing", showProp);
	set_visible_prop("transform", showProp);
	set_visible_prop("vertical", showProp);
	set_visible_prop("opacity", showProp);
	set_visible_prop("gradient", showProp);
	set_visible_prop("bk_opacity", showProp);
	set_visible_prop("align", showProp);
	set_visible_prop("valign", showProp);

	set_visible_prop("outline", showProp);
	bool outline_data = obs_data_get_bool(settings, "outline");
	if(outline_data) {
		set_visible_prop("outline_size", showProp);
		set_visible_prop("outline_color", showProp);
		set_visible_prop("outline_opacity", showProp);
	}

	set_visible_prop("chatlog", showProp);
	bool chatlog_data = obs_data_get_bool(settings, "chatlog");
	if(chatlog_data) {
		set_visible_prop("chatlog_lines", showProp);
	}

	set_visible_prop("extents", showProp);
	bool extents_data = obs_data_get_bool(settings, "extents");
	if(extents_data) {
		set_visible_prop("extents_cx", showProp);
		set_visible_prop("extents_cy", showProp);
		set_visible_prop("extents_wrap", showProp);
	}
}
void OBSPropertiesView::setBrowserSourceProps(bool showProp)
{
	set_visible_prop("reroute_audio", showProp);
	set_visible_prop("fps_custom", showProp);

	bool fps_custom = obs_data_get_bool(settings, "fps_custom");
	if(fps_custom)
		set_visible_prop("fps", showProp);

	set_visible_prop("css", showProp);
	set_visible_prop("shutdown", showProp);
	set_visible_prop("restart_when_active", showProp);
	set_visible_prop("webpage_control_level", showProp);
	set_visible_prop("refreshnocache", showProp);
}