#include "CHotkeySettingAreaWidget.h"
#include "ui_hotkey-setting-area.h"

#include <QTimer>
#include <QScrollbar>

#include "include/qt-wrapper.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/OBSOutput/SBasicOutputHandler.h"

#include "UIComponent/CCustomPushbutton.h"
#include "UIComponent/CHotkeyComponent.h"
#include "UIComponent/CSourceLabel.h"


#define HOTKEY_MARGIN	10
#define HOTKEY_FOCUS_CHANGED     &AFQHotkeySettingAreaWidget::qslotHotkeyFocusChanged

template<typename Func>
static inline void LayoutHotkey(AFQHotkeySettingAreaWidget* hotkeyArea, obs_hotkey_id id, obs_hotkey_t* key,
	Func&& fun,
	const std::map<obs_hotkey_id, std::vector<obs_key_combination_t>>& keys)
{	
	auto* label = new AFQHotkeyLabel;
	label->setFixedSize(200, 40);
	label->setWordWrap(true);
	QString text = Str(obs_hotkey_get_description(key));

	label->setProperty("fullName", text);
	TruncateLabel(label, text);

	AFHotkeyWidget* hw = nullptr;

	auto combos = keys.find(id);
	if (combos == std::end(keys))
		hw = new AFHotkeyWidget(hotkeyArea, id, obs_hotkey_get_name(key),
			hotkeyArea);
	else
		hw = new AFHotkeyWidget(hotkeyArea, id, obs_hotkey_get_name(key),
			hotkeyArea, combos->second);

	QObject::connect(hw, &AFHotkeyWidget::qsignalAddRemoveEdit,
		hotkeyArea, &AFQHotkeySettingAreaWidget::qslotHotkeyAreaScrollChanged);

	hw->m_Label = label;
	label->m_Widget = hw;

	fun(key, label, hw);
}

#define HOTKEY_AREA_LABEL_QSS "QLabel { color: rgba(255, 255, 255, 80%);		\
										font-size: 18px;		\
										font-style: normal;	\
										font-weight: 700;		\
										line-height: normal;	\
										}"

static QLabel* makeLabel(QString text)
{
	QLabel* label = new QLabel(text);
	label->setFixedHeight(40);
	label->setStyleSheet(HOTKEY_AREA_LABEL_QSS);
	return label;
}

template<typename Func, typename T>
static QLabel* makeLabel(T& t, Func&& getName)
{
	QLabel* label = new QLabel(getName(t));
	label->setFixedHeight(40);
	label->setStyleSheet(HOTKEY_AREA_LABEL_QSS);
	return label;
}

template<typename Func>
static QLabel* makeLabel(const OBSSource& source, Func&&)
{
	AFQSourceLabel* label = new AFQSourceLabel(source);
	label->setAlignment(Qt::AlignLeft);
	label->setFixedHeight(40);
	label->setStyleSheet(HOTKEY_AREA_LABEL_QSS);
	QString name = QT_UTF8(obs_source_get_name(source));
	TruncateLabel(label, name, 50);

	return label;
}

template<typename Func, typename T>
static inline void AddHotkeys(
	QFormLayout& layout, Func&& getName,
	std::vector<std::tuple<T, QPointer<QLabel>, QPointer<QWidget>>>& hotkeys)
{
	if (hotkeys.empty())
		return;

	using tuple_type = std::tuple<T, QPointer<QLabel>, QPointer<QWidget>>;

	stable_sort(begin(hotkeys), end(hotkeys),
		[&](const tuple_type& a, const tuple_type& b) {
			const auto& o_a = std::get<0>(a);
			const auto& o_b = std::get<0>(b);
			return o_a != o_b &&
				std::string(getName(o_a)) < getName(o_b);
		});

	std::string prevName;
	for (const auto& hotkey : hotkeys) {
		const auto& o = std::get<0>(hotkey);
		const char* name = getName(o);
		if (prevName != name) {
			prevName = name;
			layout.setItem(layout.rowCount(),
						   QFormLayout::SpanningRole,
						   new QSpacerItem(0, 10));

			QLabel* tlabel = makeLabel(o, getName);
			layout.addRow(tlabel);
		}

		QLabel* hlabel = std::get<1>(hotkey);
		AFQHotkeyLabel* hHotkeyLabel = qobject_cast<AFQHotkeyLabel*>(hlabel);
		hHotkeyLabel->SetHotkeyAreaName(QT_UTF8(name));
		auto widget = std::get<2>(hotkey);
		layout.addRow(hlabel, widget);
	}
}

AFQHotkeySettingAreaWidget::AFQHotkeySettingAreaWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQHotkeySettingAreaWidget)
{
    ui->setupUi(this);
	connect(ui->lineEdit_Filter, &QLineEdit::textChanged, 
		this, &AFQHotkeySettingAreaWidget::qslotHotkeySearchByName);
	connect(ui->lineEdit_FilterByHotkey, &AFQHotkeyEdit::qsignalKeyChanged,
		this, &AFQHotkeySettingAreaWidget::qslotHotkeySearchByKey);
	connect(ui->pushButton_Filter, &QPushButton::clicked,
		this, &AFQHotkeySettingAreaWidget::qslotHotkeyNameResetClicked);
	connect(ui->pushButton_FilterByHotkey, &QPushButton::clicked,
		this, &AFQHotkeySettingAreaWidget::qslotHotkeyComboResetClicked);
	connect(ui->scrollArea_Hotkeys->verticalScrollBar(), &QScrollBar::valueChanged,
		this, &AFQHotkeySettingAreaWidget::qslotCheckScrollValue);


#define ADD_HOTKEY_FOCUS_TYPE(s)      \
	ui->comboBox_HotkeyFocusBehavior->addItem( \
		Str("Basic.Settings.Advanced.Hotkeys." s), s)

	ADD_HOTKEY_FOCUS_TYPE("NeverDisableHotkeys");
	ADD_HOTKEY_FOCUS_TYPE("DisableHotkeysInFocus");
	ADD_HOTKEY_FOCUS_TYPE("DisableHotkeysOutOfFocus");

#undef ADD_HOTKEY_FOCUS_TYPE

	auto ReloadHotkeys = [](void* data, calldata_t*) {
		auto settings = static_cast<AFQHotkeySettingAreaWidget*>(data);
		QMetaObject::invokeMethod(settings, "ReloadHotkeys");
	};
	m_HotkeyRegisteredOBSSignal.Connect(obs_get_signal_handler(), "hotkey_register",
		ReloadHotkeys, this);

	auto ReloadHotkeysIgnore = [](void* data, calldata_t* param) {
		auto settings = static_cast<AFQHotkeySettingAreaWidget*>(data);
		auto key =
			static_cast<obs_hotkey_t*>(calldata_ptr(param, "key"));
		QMetaObject::invokeMethod(settings, "ReloadHotkeys",
			Q_ARG(obs_hotkey_id,
				obs_hotkey_get_id(key)));
	};
	m_HotkeyUnregisteredOBSSignal.Connect(obs_get_signal_handler(),
		"hotkey_unregister", ReloadHotkeysIgnore,
		this);


	AFSettingUtils::HookWidget(ui->comboBox_HotkeyFocusBehavior, this, COMBO_CHANGED, HOTKEY_FOCUS_CHANGED);
	
}

AFQHotkeySettingAreaWidget::~AFQHotkeySettingAreaWidget()
{
    delete ui;
}

void AFQHotkeySettingAreaWidget::qslotHotkeyFocusChanged()
{
	if (!m_bLoading) {
		SetHotkeysDataChangedVal(true);
		sender()->setProperty("changed", QVariant(true));
		emit qsignalHotkeyChanged();
	}
}

void AFQHotkeySettingAreaWidget::qslotHotkeyAreaScrollChanged()
{
	_RefreshHotkeyArea();
}

void AFQHotkeySettingAreaWidget::qslotHotkeySearchByName(const QString text)
{
	SearchHotkeys(text, ui->lineEdit_FilterByHotkey->key);
}

void AFQHotkeySettingAreaWidget::qslotHotkeySearchByKey(obs_key_combination_t combo)
{
	SearchHotkeys(ui->lineEdit_Filter->text(), combo);
}

void AFQHotkeySettingAreaWidget::qslotHotkeyNameResetClicked()
{
	ui->lineEdit_Filter->setText("");
}

void AFQHotkeySettingAreaWidget::qslotHotkeyComboResetClicked()
{
	ui->lineEdit_FilterByHotkey->qslotResetKey();
}

void AFQHotkeySettingAreaWidget::qslotMoveToPosition()
{
	QMap<int, QPushButton*>::iterator iter;
	for (iter = m_mapSectionButton.begin(); iter != m_mapSectionButton.end(); ++iter){
		iter.value()->setChecked(false);
	}

	AFQCustomPushbutton* AreaButton = reinterpret_cast<AFQCustomPushbutton*>(sender());
	int position = AreaButton->ButtonKeyValue();

	m_bSectionButtonClicked = true;
	ui->scrollArea_Hotkeys->verticalScrollBar()->setValue(position);
	m_bSectionButtonClicked = false;

	AreaButton->setChecked(true);
}

void AFQHotkeySettingAreaWidget::qslotCheckScrollValue(int value)
{
	if (m_bSectionButtonClicked) {
		m_bSectionButtonClicked = false;
		return;
	}

	int height = value + ui->scrollAreaWidgetContents_Hotkeys->visibleRegion().boundingRect().height();

	QMap<int, QPushButton*>::iterator iter = m_mapSectionButton.begin();
	for (; iter != m_mapSectionButton.end(); ++iter) {
		iter.value()->setChecked(false);
	}

	QMapIterator<int, QPushButton*> iter_reverse(m_mapSectionButton);
	iter_reverse.toBack();
	while (iter_reverse.hasPrevious()) {
		iter_reverse.previous();
		if (iter_reverse.key() < height) {
			break;
		}
	}
	iter_reverse.value()->setChecked(true);

}

void AFQHotkeySettingAreaWidget::qslotHotkeyChanged()
{
	using namespace std;
	if (m_bLoading)
		return;

	m_bHotkeysDataChanged =
		any_of(begin(m_vHotkeyWiget), end(m_vHotkeyWiget),
			[](const HOTKEY_WIDGET_PAIR& hotkey) {
				const auto& hw = *hotkey.second;
				return hw.Changed();
			});
	if (m_bHotkeysDataChanged)
		emit qsignalHotkeyChanged();
}


#include <unordered_map>
#include <vector>
#include "obs-hotkey.h"

namespace std {
	template<> struct hash<obs_key_combination_t> {
		size_t operator()(obs_key_combination_t value) const
		{
			size_t h1 = hash<uint32_t>{}(value.modifiers);
			size_t h2 = hash<int>{}(value.key);

			h2 ^= h1 + 0x9e3779b9 + (h2 << 6) + (h2 >> 2);
			return h2;
		}
	};
}

bool AFQHotkeySettingAreaWidget::qslotScanDuplicateHotkeys(QFormLayout* layout)
{
	typedef struct assignment {
		AFQHotkeyLabel* label;
		AFQHotkeyEdit* edit;
	} assignment;

	std::unordered_map<obs_key_combination_t, std::vector<assignment>> assignments;
	std::vector<AFQHotkeyLabel*> items;
	bool hasDupes = false;

	for (int i = 0; i < layout->rowCount(); i++) {
		auto label = layout->itemAt(i, QFormLayout::LabelRole);
		if (!label)
			continue;
		AFQHotkeyLabel* item =
			qobject_cast<AFQHotkeyLabel*>(label->widget());
		if (!item)
			continue;

		items.push_back(item);

		for (auto& edit : item->m_Widget->m_Edits) {
			edit->hasDuplicate = false;

			if (obs_key_combination_is_empty(edit->key))
				continue;

			for (assignment& assign : assignments[edit->key]) {
				if (item->m_PairPartner == assign.label)
					continue;

				assign.edit->hasDuplicate = true;
				edit->hasDuplicate = true;
				hasDupes = true;
			}

			assignments[edit->key].push_back({ item, edit });
		}
	}

	for (auto* item : items)
		for (auto& edit : item->m_Widget->m_Edits)
			edit->UpdateDuplicationState();

	return true;
}

void AFQHotkeySettingAreaWidget::UpdateFocusBehaviorComboBox() {
    const char* hotkeyFocusType =
        config_get_string(AFConfigManager::GetSingletonInstance().GetGlobal(),
                          "General", "HotkeyFocusType");
    int idx =
        ui->comboBox_HotkeyFocusBehavior->findData(QT_UTF8(hotkeyFocusType));
    if (idx != -1) {
        ui->comboBox_HotkeyFocusBehavior->setCurrentIndex(idx);
    }
}

void AFQHotkeySettingAreaWidget::LoadHotkeysSettings(obs_hotkey_id ignoreKey)
{
	m_bLoading = true;
	m_vHotkeyWiget.clear();
	m_mapSectionButton.clear();
	m_listSectionPosY.clear();
	m_iAreaTrace = 0;

	UpdateFocusBehaviorComboBox();

	if (ui->formLayout_Hotkey->rowCount() > 0) {
		QLayoutItem* forDeletion = ui->formLayout_Hotkey->takeAt(0);
		forDeletion->widget()->deleteLater();
		delete forDeletion;
	}
	ui->lineEdit_Filter->blockSignals(true);
	ui->lineEdit_FilterByHotkey->blockSignals(true);
	ui->lineEdit_Filter->setText("");
	ui->lineEdit_FilterByHotkey->qslotResetKey();
	ui->lineEdit_Filter->blockSignals(false);
	ui->lineEdit_FilterByHotkey->blockSignals(false);

	using keys_t = std::map<obs_hotkey_id, std::vector<obs_key_combination_t>>;
	keys_t keys;
	obs_enum_hotkey_bindings(
		[](void* data, size_t, obs_hotkey_binding_t* binding) {
			auto& keys = *static_cast<keys_t*>(data);

			keys[obs_hotkey_binding_get_hotkey_id(binding)]
				.emplace_back(
					obs_hotkey_binding_get_key_combination(
						binding));

			return true;
		},
		&keys);

	QFormLayout* hotkeysLayout = new QFormLayout();
	hotkeysLayout->setVerticalSpacing(10);
	hotkeysLayout->setHorizontalSpacing(0);
	hotkeysLayout->setContentsMargins(0, 0, 0, 0);
	hotkeysLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	hotkeysLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignTrailing |
		Qt::AlignVCenter);
	auto hotkeyChildWidget = new QWidget();
	hotkeyChildWidget->setLayout(hotkeysLayout);
	ui->formLayout_Hotkey->addRow(hotkeyChildWidget);

	using namespace std;
	using encoders_elem_t =
		tuple<OBSEncoder, QPointer<QLabel>, QPointer<QWidget>>;
	using outputs_elem_t =
		tuple<OBSOutput, QPointer<QLabel>, QPointer<QWidget>>;
	using services_elem_t =
		tuple<OBSService, QPointer<QLabel>, QPointer<QWidget>>;
	using sources_elem_t =
		tuple<OBSSource, QPointer<QLabel>, QPointer<QWidget>>;
	vector<encoders_elem_t> encoders;
	vector<outputs_elem_t> outputs;
	vector<services_elem_t> services;
	vector<sources_elem_t> scenes;
	vector<sources_elem_t> sources;

	vector<obs_hotkey_id> pairIds;
	map<obs_hotkey_id, pair<obs_hotkey_id, AFQHotkeyLabel*>> pairLabels;

	using std::move;

	auto HandleEncoder = [&](void* registerer, AFQHotkeyLabel* label,
		AFHotkeyWidget* hw) {
			auto weak_encoder =
				static_cast<obs_weak_encoder_t*>(registerer);
			auto encoder = OBSGetStrongRef(weak_encoder);

			if (!encoder)
				return true;

			encoders.emplace_back(std::move(encoder), label, hw);
			return false;
	};

	auto HandleOutput = [&](void* registerer, AFQHotkeyLabel* label,
		AFHotkeyWidget* hw) {
			auto weak_output = static_cast<obs_weak_output_t*>(registerer);
			auto output = OBSGetStrongRef(weak_output);

			if (!output)
				return true;

			outputs.emplace_back(std::move(output), label, hw);
			return false;
	};

	auto HandleService = [&](void* registerer, AFQHotkeyLabel* label,
		AFHotkeyWidget* hw) {
			auto weak_service =
				static_cast<obs_weak_service_t*>(registerer);
			auto service = OBSGetStrongRef(weak_service);

			if (!service)
				return true;

			services.emplace_back(std::move(service), label, hw);
			return false;
	};

	auto HandleSource = [&](void* registerer, AFQHotkeyLabel* label,
		AFHotkeyWidget* hw) {
			auto weak_source = static_cast<obs_weak_source_t*>(registerer);
			auto source = OBSGetStrongRef(weak_source);

			if (!source)
				return true;

			if (obs_scene_from_source(source))
				scenes.emplace_back(source, label, hw);
			else if (obs_source_get_name(source) != NULL)
				sources.emplace_back(source, label, hw);

			return false;
	};

	auto RegisterHotkey = [&](obs_hotkey_t* key, AFQHotkeyLabel* label,
		AFHotkeyWidget* hw) {
			auto registerer_type = obs_hotkey_get_registerer_type(key);
			void* registerer = obs_hotkey_get_registerer(key);

			obs_hotkey_id partner = obs_hotkey_get_pair_partner_id(key);
			if (partner != OBS_INVALID_HOTKEY_ID) {
				pairLabels.emplace(obs_hotkey_get_id(key),
					make_pair(partner, label));
				pairIds.push_back(obs_hotkey_get_id(key));
			}

			using std::move;
			switch (registerer_type) {
			case OBS_HOTKEY_REGISTERER_FRONTEND:
				hotkeysLayout->addRow(label, hw);
				label->SetHotkeyAreaName(Str("Basic.Settings.General"));
				break;

			case OBS_HOTKEY_REGISTERER_ENCODER:
				if (HandleEncoder(registerer, label, hw))
					return;
				break;

			case OBS_HOTKEY_REGISTERER_OUTPUT:
				if (HandleOutput(registerer, label, hw))
					return;
				break;

			case OBS_HOTKEY_REGISTERER_SERVICE:
				if (HandleService(registerer, label, hw))
					return;
				break;

			case OBS_HOTKEY_REGISTERER_SOURCE:
				if (HandleSource(registerer, label, hw))
					return;
				break;
			}

			label->SetRegisterType(registerer_type);

			m_vHotkeyWiget.emplace_back(
				registerer_type == OBS_HOTKEY_REGISTERER_FRONTEND, hw);
			connect(hw, &AFHotkeyWidget::qsignalKeyChanged, this, [=]() {
				qslotHotkeyChanged();
				qslotScanDuplicateHotkeys(hotkeysLayout);
				});
			connect(hw, &AFHotkeyWidget::qsignalSearchKey,
				[=](obs_key_combination_t combo) {
					ui->lineEdit_Filter->setText("");
					ui->lineEdit_FilterByHotkey->qslotHandleNewKey(combo);
					ui->lineEdit_FilterByHotkey->qsignalKeyChanged(combo);
				});
	};

	bool bUseFrontHotkey = true;
	if (bUseFrontHotkey) {
		QLabel* tlabel = makeLabel(Str("Basic.Settings.General"));
		hotkeysLayout->addRow(tlabel);
	}

	auto data =
		make_tuple(RegisterHotkey, std::move(keys), ignoreKey, this);
	using data_t = decltype(data);
	obs_enum_hotkeys(
		[](void* data, obs_hotkey_id id, obs_hotkey_t* key) {
			data_t& d = *static_cast<data_t*>(data);
			if (id != get<2>(d))
				LayoutHotkey(get<3>(d), id, key, get<0>(d),
					get<1>(d));
			return true;
		},
		&data);

	for (auto keyId : pairIds) {
		auto data1 = pairLabels.find(keyId);
		if (data1 == end(pairLabels))
			continue;

		auto& label1 = data1->second.second;
		if (label1->m_PairPartner)
			continue;

		auto data2 = pairLabels.find(data1->second.first);
		if (data2 == end(pairLabels))
			continue;

		auto& label2 = data2->second.second;
		if (label2->m_PairPartner)
			continue;

		QString tt = QT_UTF8(Str("Basic.Settings.Hotkeys.Pair"));
		auto name1 = label1->text();
		auto name2 = label2->text();

		auto Update = [&](AFQHotkeyLabel* label, const QString& name,
			AFQHotkeyLabel* other,
			const QString& otherName) {
				QString string =
					other->property("fullName").value<QString>();

				if (string.isEmpty() || string.isNull())
					string = otherName;

				label->setToolTip(tt.arg(string));
				label->setText(name + " *");
				label->m_PairPartner = other;
		};
		Update(label1, name1, label2, name2);
		Update(label2, name2, label1, name1);
	}

	AddHotkeys(*hotkeysLayout, obs_output_get_name, outputs);
	AddHotkeys(*hotkeysLayout, obs_source_get_name, scenes);
	AddHotkeys(*hotkeysLayout, obs_source_get_name, sources);
	AddHotkeys(*hotkeysLayout, obs_encoder_get_name, encoders);
	AddHotkeys(*hotkeysLayout, obs_service_get_name, services);

	qslotScanDuplicateHotkeys(hotkeysLayout);

	_RefreshHotkeyArea();

	QTimer::singleShot(1, this, &AFQHotkeySettingAreaWidget::unsetCursor);
	m_bLoading = false;
	m_bHotkeysLoaded = true;
}

void AFQHotkeySettingAreaWidget::ReloadHotkeys(obs_hotkey_id ignoreKey)
{
	if (!m_bHotkeysLoaded)
		return;
	LoadHotkeysSettings(ignoreKey);
}

void AFQHotkeySettingAreaWidget::SaveHotkeysSettings()
{
	if (AFSettingUtils::WidgetChanged(ui->comboBox_HotkeyFocusBehavior)) 
	{
		QString str = AFSettingUtils::GetComboData(ui->comboBox_HotkeyFocusBehavior);
		config_set_string(AFConfigManager::GetSingletonInstance().GetGlobal(), "General",
			"HotkeyFocusType", QT_TO_UTF8(str));
	}

	const auto& config = AFConfigManager::GetSingletonInstance().GetBasic();

	using namespace std;

	std::vector<obs_key_combination> combinations;
    bool anyUpdate = false;
        
	for (auto& hotkey : m_vHotkeyWiget) {
		auto& hw = *hotkey.second;
		if (!hw.Changed())
			continue;

		hw.Save(combinations);

		if (!hotkey.first)
			continue;

		anyUpdate = true;
		OBSDataArrayAutoRelease array = obs_hotkey_save(hw.m_hotkeyId);
		OBSDataAutoRelease data = obs_data_create();
		obs_data_set_array(data, "bindings", array);
		const char* json = obs_data_get_json(data);
		config_set_string(config, "Hotkeys", hw.m_sName.c_str(), json);
	};

    if (anyUpdate) {
        AFConfigManager::GetSingletonInstance().SafeSaveBasic();
    }
}

void AFQHotkeySettingAreaWidget::ClearHotkeyValues() {
    for (HOTKEY_WIDGET_PAIR& hotkeyPair : m_vHotkeyWiget) {
        AFHotkeyWidget& hotkey = *hotkeyPair.second;
        hotkey.Clear();
    }
}

void AFQHotkeySettingAreaWidget::SearchHotkeys(const QString& text, obs_key_combination_t filterCombo)
{
	if (ui->formLayout_Hotkey->rowCount() == 0)
		return;

	std::vector<obs_key_combination_t> combos;
	bool showHotkey;
	ui->scrollArea_Hotkeys->ensureVisible(0, 0);

	QLayoutItem* hotkeysItem = ui->formLayout_Hotkey->itemAt(0);
	QWidget* hotkeys = hotkeysItem->widget();
	if (!hotkeys)
		return;

	QFormLayout* hotkeysLayout =
		qobject_cast<QFormLayout*>(hotkeys->layout());
	hotkeysLayout->setEnabled(false);

	QString needle = text.toLower();

	for (int i = 0; i < hotkeysLayout->rowCount(); i++) {
		auto label = hotkeysLayout->itemAt(i, QFormLayout::LabelRole);
		if (!label)
			continue;

		AFQHotkeyLabel* item =
			qobject_cast<AFQHotkeyLabel*>(label->widget());
		if (!item)
			continue;

		QString fullname = item->property("fullName").value<QString>();

		showHotkey = needle.isEmpty() ||
			fullname.toLower().contains(needle);

		if (showHotkey && !obs_key_combination_is_empty(filterCombo)) {
			showHotkey = false;

			item->m_Widget->GetCombinations(combos);
			for (auto combo : combos) {
				if (combo == filterCombo) {
					showHotkey = true;
					break;
				}
			}
		}

		label->widget()->setVisible(showHotkey);

		auto field = hotkeysLayout->itemAt(i, QFormLayout::FieldRole);
		if (field)
			field->widget()->setVisible(showHotkey);
	}
	hotkeysLayout->setEnabled(true);
}

void AFQHotkeySettingAreaWidget::_RefreshHotkeyArea()
{
	_AdjustHotkeySectionScroll();

	_MakeHotkeySectionButtons();
}


void AFQHotkeySettingAreaWidget::_AdjustHotkeySectionScroll()
{
	m_listSectionPosY.clear();
	m_iAreaTrace = 0;

	QString hotkeyAreaPrev = Str("Basic.Settings.General");
	m_listSectionPosY.append(HOTKEY_SECTION_INFO(hotkeyAreaPrev, m_iAreaTrace));

	m_iAreaTrace =+ 50;

	QLayoutItem* hotkeysItem = ui->formLayout_Hotkey->itemAt(0);
	QWidget* hotkeys = hotkeysItem->widget();
	if (!hotkeys)
		return;

	QFormLayout* hotkeysLayout =
		qobject_cast<QFormLayout*>(hotkeys->layout());

	int rowCount = hotkeysLayout->rowCount();
	for (int i = 0; i < rowCount; i++) {
		auto label = hotkeysLayout->itemAt(i, QFormLayout::LabelRole);
		auto field = hotkeysLayout->itemAt(i, QFormLayout::FieldRole);
		if (!label || !field)
			continue;

		AFQHotkeyLabel* item = qobject_cast<AFQHotkeyLabel*>(label->widget());
		if (!item)
			continue;

		AFHotkeyWidget* widget = qobject_cast<AFHotkeyWidget*>(field->widget());
		if (!widget)
			continue;

		QString hotkeyArea = item->GetHotkeyAreaName();
		if (0 != hotkeyAreaPrev.compare(hotkeyArea)) {
			m_iAreaTrace += 20;
			m_listSectionPosY.append(HOTKEY_SECTION_INFO(hotkeyArea, m_iAreaTrace));
			m_iAreaTrace += 50;

			hotkeyAreaPrev = hotkeyArea;
		}
		m_iAreaTrace += (50 * widget->GetEditCount());
	}
}

inline void clearLayout(QLayout* layout) {
	while (QLayoutItem* item = layout->takeAt(0)) {
		if (QWidget* widget = item->widget()) {
			delete widget;
		}
		else if (QLayout* childLayout = item->layout()) {
			clearLayout(childLayout);
		}
	}
}

void AFQHotkeySettingAreaWidget::_MakeHotkeySectionButtons()
{
	if (ui->scrollAreaWidgetContents_Scene->layout() != nullptr)
	{
		while (QLayoutItem* item = ui->scrollAreaWidgetContents_Scene->layout()->takeAt(0))
		{
			if (QWidget* widget = item->widget())
				widget->deleteLater();

			delete item;
		}
	}

	QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents_Scene->layout());
	clearLayout(layout);

	QMap<int, QPushButton*>::iterator iter = m_mapSectionButton.begin();
	for (; iter != m_mapSectionButton.end(); ++iter) {
		QPushButton* sectionButton = iter.value();
		delete sectionButton;
	}
	m_mapSectionButton.clear();

	for (int idx = 0; idx < m_listSectionPosY.count(); idx++)
	{
		AFQCustomPushbutton* areaButton = new AFQCustomPushbutton(this);
		areaButton->setCheckable(true);
		if (idx == 0)
			areaButton->setChecked(true);
		else
			areaButton->setChecked(false);

		areaButton->setFixedWidth(180);
		areaButton->setFixedHeight(38);
		areaButton->setObjectName("pushButton_AreaButton");

		HOTKEY_SECTION_INFO pair = m_listSectionPosY[idx];

		QFontMetrics metrics(areaButton->font());
		QString elidedText = metrics.elidedText(pair.first, Qt::ElideRight, 140);
		areaButton->setText(elidedText);
		if (0 != elidedText.compare(pair.first)) {
			areaButton->setToolTip(pair.first);
		}
		areaButton->SetButtonKeyValue(pair.second);
		connect(areaButton, &QPushButton::clicked, this, &AFQHotkeySettingAreaWidget::qslotMoveToPosition);

		m_mapSectionButton.insert(pair.second, areaButton);
		layout->insertWidget(idx, areaButton);
	}
	layout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
}