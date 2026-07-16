#include "CSettingAccessibilityAreaWidget.h"
#include "PopupWindows/CCustomColorDialog.h"

#include "MainFrame/AudioSource/CAudioSource.h"

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
}

AFQAccessibilitySettingAreaWidget::~AFQAccessibilitySettingAreaWidget()
{
	delete ui;
}

void AFQAccessibilitySettingAreaWidget::_SetAccessibilityUIProp()
{
	// HookWidget(ui->colorsGroupBox, GROUP_CHANGED, A11Y_CHANGED);
	
	AFSettingUtils::HookWidget(ui->comboBox_ColorPreset, this, COMBO_CHANGED, A11Y_CHANGED);
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
	if (!m_loading)
	{
		m_a11yChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalA11yDataChanged();
	}
}

void AFQAccessibilitySettingAreaWidget::LoadAccessibilitySettings(bool presetChange)
{
	_SmallResolution();

	auto userConfig = USERCONFIG;
	//
	m_loading = true;
	if (!presetChange) {
        m_preset = config_get_int(userConfig, "Accessibility", "ColorPreset");
		
		bool block = ui->comboBox_ColorPreset->blockSignals(true);
		ui->comboBox_ColorPreset->setCurrentIndex(std::min(m_preset, (uint32_t)ui->comboBox_ColorPreset->count() - 1));
		ui->comboBox_ColorPreset->blockSignals(block);

		bool checked = config_get_bool(userConfig, "Accessibility", "OverrideColors");

		// ui->colorsGroupBox->setChecked(checked);
	}

	if (m_preset == COLOR_PRESET_DEFAULT) {
		_ResetDefaultColors();
		//_SetDefaultColors();
	}
	else if (m_preset == COLOR_PRESET_COLOR_BLIND_1) {

        m_mixerGreenActive = 0x742E94;
        m_mixerGreen = 0x4A1A60;
        m_mixerYellowActive = 0x3349F9;
        m_mixerYellow = 0x1F2C97;
        m_mixerRedActive = 0xBEAC63;
        m_mixerRed = 0x675B28;

        m_selectRed = 0x3349F9;
        m_selectGreen = 0xFF56C9;
        m_selectBlue = 0xB09B44;

		//_SetDefaultColors();
	}
	else if (m_preset == COLOR_PRESET_CUSTOM) {
		//_SetDefaultColors();

        m_selectRed = config_get_int(userConfig, "Accessibility", "SelectRed");
        m_selectGreen = config_get_int(userConfig, "Accessibility", "SelectGreen");
        m_selectBlue = config_get_int(userConfig, "Accessibility", "SelectBlue");

        m_mixerGreen = config_get_int(userConfig, "Accessibility", "MixerGreen");
        m_mixerYellow = config_get_int(userConfig, "Accessibility", "MixerYellow");
        m_mixerRed = config_get_int(userConfig, "Accessibility", "MixerRed");

        m_mixerGreenActive = config_get_int(userConfig, "Accessibility", "MixerGreenActive");
        m_mixerYellowActive = config_get_int(userConfig, "Accessibility", "MixerYellowActive");
        m_mixerRedActive = config_get_int(userConfig, "Accessibility", "MixerRedActive");
	}

	_UpdateAccessibilityColors();

	m_loading = false;
}

void AFQAccessibilitySettingAreaWidget::qslotColorPreset(int idx)
{
    m_preset = idx == ui->comboBox_ColorPreset->count() - 1 ? COLOR_PRESET_CUSTOM
		: idx;
	LoadAccessibilitySettings(true);
}

void AFQAccessibilitySettingAreaWidget::_ResetDefaultColors()
{
	config_t* userConfig = USERCONFIG;
	//
    m_selectRed = config_get_default_int(userConfig, "Accessibility", "SelectRed");
    m_selectGreen = config_get_default_int(userConfig, "Accessibility", "SelectGreen");
    m_selectBlue = config_get_default_int(userConfig, "Accessibility", "SelectBlue");
    m_mixerGreen = config_get_default_int(userConfig, "Accessibility", "MixerGreen");
    m_mixerYellow = config_get_default_int(userConfig, "Accessibility", "MixerYellow");
    m_mixerRed = config_get_default_int(userConfig, "Accessibility", "MixerRed");
    m_mixerGreenActive = config_get_default_int(userConfig, "Accessibility", "MixerGreenActive");
    m_mixerYellowActive = config_get_default_int(userConfig, "Accessibility", "MixerYellowActive");
    m_mixerRedActive = config_get_default_int(userConfig, "Accessibility", "MixerRedActive");
}

void AFQAccessibilitySettingAreaWidget::_SetDefaultColors()
{
	config_t* userConfig = USERCONFIG;
	//
	config_set_default_int(userConfig, "Accessibility", "SelectRed", m_selectRed);
	config_set_default_int(userConfig, "Accessibility", "SelectGreen", m_selectGreen);
	config_set_default_int(userConfig, "Accessibility", "SelectBlue", m_selectBlue);

	config_set_default_int(userConfig, "Accessibility", "MixerGreen", m_mixerGreen);
	config_set_default_int(userConfig, "Accessibility", "MixerYellow", m_mixerYellow);
	config_set_default_int(userConfig, "Accessibility", "MixerRed", m_mixerRed);

	config_set_default_int(userConfig, "Accessibility", "MixerGreenActive", m_mixerGreenActive);
	config_set_default_int(userConfig, "Accessibility", "MixerYellowActive", m_mixerYellowActive);
	config_set_default_int(userConfig, "Accessibility", "MixerRedActive", m_mixerRedActive);
}

void AFQAccessibilitySettingAreaWidget::_UpdateAccessibilityColors()
{
	_SetStyle(ui->label_Color1_1, m_selectRed);
	_SetStyle(ui->label_Color2_1, m_selectGreen);
	_SetStyle(ui->label_Color3_1, m_selectBlue);
	_SetStyle(ui->label_Color4_1, m_mixerGreen);
	_SetStyle(ui->label_Color5_1, m_mixerYellow);
	_SetStyle(ui->label_Color6_1, m_mixerRed);
	_SetStyle(ui->label_Color7_1, m_mixerGreenActive);
	_SetStyle(ui->label_Color8_1, m_mixerYellowActive);
	_SetStyle(ui->label_Color9_1, m_mixerRedActive);
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
	//label->setAutoFillBackground(true);
	label->setAlignment(Qt::AlignCenter);
}

void AFQAccessibilitySettingAreaWidget::SaveAccessibilitySettings()
{
	if (!m_a11yChanged)
		return;

	config_t* userConfig = USERCONFIG;
	//
	config_set_int(userConfig, "Accessibility", "ColorPreset", m_preset);

	config_set_int(userConfig, "Accessibility", "SelectRed", m_selectRed);
	config_set_int(userConfig, "Accessibility", "SelectGreen", m_selectGreen);
	config_set_int(userConfig, "Accessibility", "SelectBlue", m_selectBlue);
	config_set_int(userConfig, "Accessibility", "MixerGreen", m_mixerGreen);
	config_set_int(userConfig, "Accessibility", "MixerYellow", m_mixerYellow);
	config_set_int(userConfig, "Accessibility", "MixerRed", m_mixerRed);
	config_set_int(userConfig, "Accessibility", "MixerGreenActive", m_mixerGreenActive);
	config_set_int(userConfig, "Accessibility", "MixerYellowActive", m_mixerYellowActive);
	config_set_int(userConfig, "Accessibility", "MixerRedActive", m_mixerRedActive);

	MAIN_AUDIOSOURCE->RefreshVolumeColors();
	DYNAMIC_COMPOSIT->RefreshSourceBorderColor();
}

void AFQAccessibilitySettingAreaWidget::qslotChoose1Clicked()
{
	QColor color = GetColor(m_selectRed, Str("Basic.Settings.Accessibility.ColorOverrides.SelectRed"));

	if (!color.isValid())
		return;

    m_selectRed = color_to_int(color);

    m_preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose2Clicked()
{
	QColor color = GetColor(m_selectGreen, Str("Basic.Settings.Accessibility.ColorOverrides.SelectGreen"));

	if (!color.isValid())
		return;

    m_selectGreen = color_to_int(color);

    m_preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose3Clicked()
{
	QColor color = GetColor(m_selectBlue, Str("Basic.Settings.Accessibility.ColorOverrides.SelectBlue"));

	if (!color.isValid())
		return;

    m_selectBlue = color_to_int(color);

    m_preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose4Clicked()
{
	QColor color = GetColor(m_mixerGreen, Str("Basic.Settings.Accessibility.ColorOverrides.MixerGreen"));

	if (!color.isValid())
		return;

    m_mixerGreen = color_to_int(color);

    m_preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose5Clicked()
{
	QColor color = GetColor(m_mixerYellow, Str("Basic.Settings.Accessibility.ColorOverrides.MixerYellow"));

	if (!color.isValid())
		return;

    m_mixerYellow = color_to_int(color);

    m_preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose6Clicked()
{
	QColor color = GetColor(m_mixerRed, Str("Basic.Settings.Accessibility.ColorOverrides.MixerRed"));

	if (!color.isValid())
		return;

    m_mixerRed = color_to_int(color);

    m_preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose7Clicked()
{
	QColor color = GetColor(m_mixerGreenActive, Str("Basic.Settings.Accessibility.ColorOverrides.MixerGreenActive"));

	if (!color.isValid())
		return;

    m_mixerGreenActive = color_to_int(color);

    m_preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose8Clicked()
{
	QColor color = GetColor(m_mixerYellowActive, Str("Basic.Settings.Accessibility.ColorOverrides.MixerYellowActive"));

	if (!color.isValid())
		return;

    m_mixerYellowActive = color_to_int(color);

    m_preset = COLOR_PRESET_CUSTOM;
	bool block = ui->comboBox_ColorPreset->blockSignals(true);
	ui->comboBox_ColorPreset->setCurrentIndex(ui->comboBox_ColorPreset->count() - 1);
	ui->comboBox_ColorPreset->blockSignals(block);

	_UpdateAccessibilityColors();

	qslotA11yDataChanged();
}
void AFQAccessibilitySettingAreaWidget::qslotChoose9Clicked()
{
	QColor color = GetColor(m_mixerRedActive, Str("Basic.Settings.Accessibility.ColorOverrides.MixerRedActive"));

	if (!color.isValid())
		return;

    m_mixerRedActive = color_to_int(color);

    m_preset = COLOR_PRESET_CUSTOM;
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
	// TODO: Revisit hang on Ubuntu with native dialog
	options |= QColorDialog::DontUseNativeDialog;
#endif

	QColor color = color_from_int(colorVal);

#ifdef _WIN32	
	return AFQCustomColorDialog::getColor(color, this, Str("CustomColorDialog.title"), options);
#else
	return QColorDialog::getColor(color, this, Str("CustomColorDialog.title"), options);
#endif
}

void AFQAccessibilitySettingAreaWidget::_SmallResolution()
{
	if (MAINFRAME->IsSmallResolution())
	{
		ui->scrollArea->setFixedWidth(796);
		ui->scrollAreaWidgetContents->setFixedWidth(796);

		ui->label_ColorSelect->setFixedWidth(263);
		ui->label_Color1->setFixedWidth(284);
		ui->label_Color2->setFixedWidth(284);
		ui->label_Color3->setFixedWidth(284);
		ui->label_Color4->setFixedWidth(284);
		ui->label_Color5->setFixedWidth(284);
		ui->label_Color6->setFixedWidth(284);
		ui->label_Color7->setFixedWidth(284);
		ui->label_Color8->setFixedWidth(284);
		ui->label_Color9->setFixedWidth(284);

		ui->label_Color1_1->setFixedWidth(210);
		ui->label_Color2_1->setFixedWidth(210);
		ui->label_Color3_1->setFixedWidth(210);
		ui->label_Color4_1->setFixedWidth(210);
		ui->label_Color5_1->setFixedWidth(210);
		ui->label_Color6_1->setFixedWidth(210);
		ui->label_Color7_1->setFixedWidth(210);
		ui->label_Color8_1->setFixedWidth(210);
		ui->label_Color9_1->setFixedWidth(210);

		ui->pushButton_Color1->setFixedWidth(210);
		ui->pushButton_Color2->setFixedWidth(210);
		ui->pushButton_Color3->setFixedWidth(210);
		ui->pushButton_Color4->setFixedWidth(210);
		ui->pushButton_Color5->setFixedWidth(210);
		ui->pushButton_Color6->setFixedWidth(210);
		ui->pushButton_Color7->setFixedWidth(210);
		ui->pushButton_Color8->setFixedWidth(210);
		ui->pushButton_Color9->setFixedWidth(210);
	}
}


void AFQAccessibilitySettingAreaWidget::_ChangeLanguage()
{
	QList<QLabel*> labelList = findChildren<QLabel*>();
	QList<QComboBox*> comboboxList = findChildren<QComboBox*>();
	QList<QPushButton*> pushbuttonList = findChildren<QPushButton*>();

	foreach(QLabel * label, labelList)
	{
		QString qtranslate = QTStr(label->text().toUtf8().constData());
		label->setText(qtranslate);
	}
	foreach(QPushButton* pushButton, pushbuttonList)
	{
		QString qtranslate = QTStr(pushButton->text().toUtf8().constData());
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

			QString qtranslate = QTStr(itemString.toUtf8().constData());
			combobox->setItemText(i, qtranslate);
		}
	}
}
