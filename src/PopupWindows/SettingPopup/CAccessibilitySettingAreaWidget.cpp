#include "CAccessibilitySettingAreaWidget.h"
#include "PopupWindows/CCustomColorDialog.h"


static inline QColor color_from_int(long long val)
{
	return QColor(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff,
		(val >> 24) & 0xff);
}

static inline long long color_to_int(QColor color)
{
	auto shift = [&](unsigned val, int shift) {
		return ((val & 0xff) << shift);
		};

	return shift(color.red(), 0) | shift(color.green(), 8) |
		shift(color.blue(), 16) | shift(color.alpha(), 24);
}

AFQAccessibilitySettingAreaWidget::AFQAccessibilitySettingAreaWidget(QWidget* parent):
	QWidget(parent), ui(new Ui::AFQAccessibilitySettingAreaWidget)
{
	ui->setupUi(this);

	_SetAccessibilityUIProp();
	_ChangeLanguage();
}

AFQAccessibilitySettingAreaWidget::~AFQAccessibilitySettingAreaWidget()
{
	delete ui;
}

void AFQAccessibilitySettingAreaWidget::_SetAccessibilityUIProp()
{
	AFSettingUtilsA::HookWidget(ui->comboBox_ColorPreset, this, COMBO_CHANGED, A11Y_CHANGED);
	connect(ui->comboBox_ColorPreset, &QComboBox::currentIndexChanged, this, &AFQAccessibilitySettingAreaWidget::qslotColorPreset);
	connect(ui->pushButton_Color1, &QPushButton::clicked, this, &AFQAccessibilitySettingAreaWidget::qslotChoose1Clicked);
	connect(ui->pushButton_Color2, &QPushButton::clicked, this, &AFQAccessibilitySettingAreaWidget::qslotChoose2Clicked);
	connect(ui->pushButton_Color3, &QPushButton::clicked, this, &AFQAccessibilitySettingAreaWidget::qslotChoose3Clicked);
	connect(ui->pushButton_Color4, &QPushButton::clicked, this, &AFQAccessibilitySettingAreaWidget::qslotChoose4Clicked);
	connect(ui->pushButton_Color5, &QPushButton::clicked, this, &AFQAccessibilitySettingAreaWidget::qslotChoose5Clicked);
	connect(ui->pushButton_Color6, &QPushButton::clicked, this, &AFQAccessibilitySettingAreaWidget::qslotChoose6Clicked);
	connect(ui->pushButton_Color7, &QPushButton::clicked, this, &AFQAccessibilitySettingAreaWidget::qslotChoose7Clicked);
	connect(ui->pushButton_Color8, &QPushButton::clicked, this, &AFQAccessibilitySettingAreaWidget::qslotChoose8Clicked);
	connect(ui->pushButton_Color9, &QPushButton::clicked, this, &AFQAccessibilitySettingAreaWidget::qslotChoose9Clicked);

}

void AFQAccessibilitySettingAreaWidget::qslotA11yDataChanged()
{
	if (!m_bLoading)
	{
		m_bA11yChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalA11yDataChanged();
	}
}

void AFQAccessibilitySettingAreaWidget::LoadAccessibilitySettings(bool presetChange)
{
	config_t* globalConfig = AFConfigManager::GetSingletonInstance().GetGlobal();

	m_bLoading = true;
	if (!presetChange) {
		preset = config_get_int(globalConfig, "Accessibility", "ColorPreset");
		
		bool block = ui->comboBox_ColorPreset->blockSignals(true);
		ui->comboBox_ColorPreset->setCurrentIndex(std::min(preset, (uint32_t)ui->comboBox_ColorPreset->count() - 1));
		ui->comboBox_ColorPreset->blockSignals(block);

		bool checked = config_get_bool(globalConfig, "Accessibility",
			"OverrideColors");
	}

	if (preset == COLOR_PRESET_DEFAULT) {
		_ResetDefaultColors();
	}
	else if (preset == COLOR_PRESET_COLOR_BLIND_1) {
		mixerGreenActive = 0x742E94;
		mixerGreen = 0x4A1A60;
		mixerYellowActive = 0x3349F9;
		mixerYellow = 0x1F2C97;
		mixerRedActive = 0xBEAC63;
		mixerRed = 0x675B28;

		selectRed = 0x3349F9;
		selectGreen = 0xFF56C9;
		selectBlue = 0xB09B44;
	}
	else if (preset == COLOR_PRESET_CUSTOM) {
		selectRed =
			config_get_int(globalConfig, "Accessibility", "SelectRed");
		selectGreen =
			config_get_int(globalConfig, "Accessibility", "SelectGreen");
		selectBlue =
			config_get_int(globalConfig, "Accessibility", "SelectBlue");

		mixerGreen =
			config_get_int(globalConfig, "Accessibility", "MixerGreen");
		mixerYellow =
			config_get_int(globalConfig, "Accessibility", "MixerYellow");
		mixerRed = config_get_int(globalConfig, "Accessibility", "MixerRed");

		mixerGreenActive = config_get_int(globalConfig, "Accessibility",
			"MixerGreenActive");
		mixerYellowActive = config_get_int(globalConfig, "Accessibility",
			"MixerYellowActive");
		mixerRedActive = config_get_int(globalConfig, "Accessibility",
			"MixerRedActive");
	}

	_UpdateAccessibilityColors();

	m_bLoading = false;
}

void AFQAccessibilitySettingAreaWidget::qslotColorPreset(int idx)
{
	preset = idx == ui->comboBox_ColorPreset->count() - 1 ? COLOR_PRESET_CUSTOM
		: idx;
	LoadAccessibilitySettings(true);
}

void AFQAccessibilitySettingAreaWidget::_ResetDefaultColors()
{
	config_t* globalConfig = AFConfigManager::GetSingletonInstance().GetGlobal();

	selectRed = config_get_default_int(globalConfig, "Accessibility", "SelectRed");
	selectGreen = config_get_default_int(globalConfig, "Accessibility", "SelectGreen");
	selectBlue = config_get_default_int(globalConfig, "Accessibility", "SelectBlue");
	mixerGreen = config_get_default_int(globalConfig, "Accessibility", "MixerGreen");
	mixerYellow = config_get_default_int(globalConfig, "Accessibility", "MixerYellow");
	mixerRed = config_get_default_int(globalConfig, "Accessibility", "MixerRed");
	mixerGreenActive = config_get_default_int(globalConfig, "Accessibility", "MixerGreenActive");
	mixerYellowActive = config_get_default_int(globalConfig, "Accessibility", "MixerYellowActive");
	mixerRedActive = config_get_default_int(globalConfig, "Accessibility", "MixerRedActive");
}

void AFQAccessibilitySettingAreaWidget::_SetDefaultColors()
{
	config_t* globalConfig = AFConfigManager::GetSingletonInstance().GetGlobal();
	config_set_default_int(globalConfig, "Accessibility", "SelectRed", selectRed);
	config_set_default_int(globalConfig, "Accessibility", "SelectGreen",
		selectGreen);
	config_set_default_int(globalConfig, "Accessibility", "SelectBlue",
		selectBlue);

	config_set_default_int(globalConfig, "Accessibility", "MixerGreen",
		mixerGreen);
	config_set_default_int(globalConfig, "Accessibility", "MixerYellow",
		mixerYellow);
	config_set_default_int(globalConfig, "Accessibility", "MixerRed", mixerRed);

	config_set_default_int(globalConfig, "Accessibility", "MixerGreenActive",
		mixerGreenActive);
	config_set_default_int(globalConfig, "Accessibility", "MixerYellowActive",
		mixerYellowActive);
	config_set_default_int(globalConfig, "Accessibility", "MixerRedActive",
		mixerRedActive);
}

void AFQAccessibilitySettingAreaWidget::_UpdateAccessibilityColors()
{
	_SetStyle(ui->label_Color1_1, selectRed);
	_SetStyle(ui->label_Color2_1, selectGreen);
	_SetStyle(ui->label_Color3_1, selectBlue);
	_SetStyle(ui->label_Color4_1, mixerGreen);
	_SetStyle(ui->label_Color5_1, mixerYellow);
	_SetStyle(ui->label_Color6_1, mixerRed);
	_SetStyle(ui->label_Color7_1, mixerGreenActive);
	_SetStyle(ui->label_Color8_1, mixerYellowActive);
	_SetStyle(ui->label_Color9_1, mixerRedActive);
}

void AFQAccessibilitySettingAreaWidget::_SetStyle(QLabel* label, uint32_t colorVal)
{
	QColor color = color_from_int(colorVal);
	color.setAlpha(255);
	QPalette palette = QPalette(color);
	label->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	label->setText(color.name(QColor::HexRgb));
	label->setPalette(palette);
	label->setStyleSheet(QString("QLabel{ border-radius: 4px; background-color: %1; color: %2; }")
								.arg(palette.color(QPalette::Window)
									.name(QColor::HexRgb))
								.arg(palette.color(QPalette::WindowText)
									.name(QColor::HexRgb)));
	label->setAlignment(Qt::AlignCenter);
}

void AFQAccessibilitySettingAreaWidget::SaveAccessibilitySettings()
{
	config_t* globalConfig = AFConfigManager::GetSingletonInstance().GetGlobal();

	config_set_int(globalConfig, "Accessibility", "ColorPreset", preset);

	config_set_int(globalConfig, "Accessibility", "SelectRed", selectRed);
	config_set_int(globalConfig, "Accessibility", "SelectGreen", selectGreen);
	config_set_int(globalConfig, "Accessibility", "SelectBlue", selectBlue);
	config_set_int(globalConfig, "Accessibility", "MixerGreen", mixerGreen);
	config_set_int(globalConfig, "Accessibility", "MixerYellow", mixerYellow);
	config_set_int(globalConfig, "Accessibility", "MixerRed", mixerRed);
	config_set_int(globalConfig, "Accessibility", "MixerGreenActive", mixerGreenActive);
	config_set_int(globalConfig, "Accessibility", "MixerYellowActive", mixerYellowActive);
	config_set_int(globalConfig, "Accessibility", "MixerRedActive", mixerRedActive);

	App()->GetMainView()->GetMainWindow()->RefreshVolumeColors();
	App()->GetMainView()->GetMainWindow()->RefreshSourceBorderColor();
}

void AFQAccessibilitySettingAreaWidget::qslotChoose1Clicked()
{
	QColor color = GetColor(selectRed, AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Accessibility.ColorOverrides.SelectRed"));

	if (!color.isValid())
		return;

	selectRed = color_to_int(color);

	preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose2Clicked()
{
	QColor color = GetColor(selectRed, AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Accessibility.ColorOverrides.SelectGreen"));

	if (!color.isValid())
		return;

	selectGreen = color_to_int(color);

	preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose3Clicked()
{
	QColor color = GetColor(selectRed, AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Accessibility.ColorOverrides.SelectBlue"));

	if (!color.isValid())
		return;

	selectBlue = color_to_int(color);

	preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose4Clicked()
{
	QColor color = GetColor(selectRed, AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Accessibility.ColorOverrides.MixerGreen"));

	if (!color.isValid())
		return;

	mixerGreen = color_to_int(color);

	preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose5Clicked()
{
	QColor color = GetColor(selectRed, AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Accessibility.ColorOverrides.MixerYellow"));

	if (!color.isValid())
		return;

	mixerYellow = color_to_int(color);

	preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose6Clicked()
{
	QColor color = GetColor(selectRed, AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Accessibility.ColorOverrides.MixerRed"));

	if (!color.isValid())
		return;

	mixerRed = color_to_int(color);

	preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose7Clicked()
{
	QColor color = GetColor(selectRed, AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Accessibility.ColorOverrides.MixerGreenActive"));

	if (!color.isValid())
		return;

	mixerGreenActive = color_to_int(color);

	preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose8Clicked()
{
	QColor color = GetColor(selectRed, AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Accessibility.ColorOverrides.MixerYellowActive"));

	if (!color.isValid())
		return;

	mixerYellowActive = color_to_int(color);

	preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose9Clicked()
{
	QColor color = GetColor(selectRed, AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Accessibility.ColorOverrides.MixerRedActive"));

	if (!color.isValid())
		return;

	mixerRedActive = color_to_int(color);

	preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}

QColor AFQAccessibilitySettingAreaWidget::GetColor(uint32_t colorVal, QString label)
{
	if (label.contains('\n')) {
		label.replace("\n", " ");
	}

	QColorDialog::ColorDialogOptions options;
#ifdef __linux__
	options |= QColorDialog::DontUseNativeDialog;
#endif

	QColor color = color_from_int(colorVal);

#ifdef _WIN32	
	return AFQCustomColorDialog::getColor(color, this, Str("CustomColorDialog.title"), options);
#else
	return QColorDialog::getColor(color, this, Str("CustomColorDialog.title"), options);
#endif
}


void AFQAccessibilitySettingAreaWidget::_ChangeLanguage()
{
	QList<QLabel*> labelList = findChildren<QLabel*>();
	QList<QComboBox*> comboboxList = findChildren<QComboBox*>();
	QList<QPushButton*> pushbuttonList = findChildren<QPushButton*>();

	foreach(QLabel * label, labelList)
	{
		QString qtranslate = QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(label->text().toUtf8().constData()));
		label->setText(qtranslate);
	}
	foreach(QPushButton* pushButton, pushbuttonList)
	{
		QString qtranslate = QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(pushButton->text().toUtf8().constData()));
		pushButton->setText(qtranslate);
	}

	foreach(QComboBox * combobox, comboboxList)
	{
		int itemCount = combobox->count();
		for (int i = 0; i < itemCount; ++i)
		{
			QString itemString = combobox->itemText(i);
			QRegularExpression rx("\\d+");

			if (rx.match(itemString).hasMatch())
				continue;

			QString qtranslate = QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(itemString.toUtf8().constData()));
			combobox->setItemText(i, qtranslate);
		}
	}
}