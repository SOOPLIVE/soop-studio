#include "CAquaSubtitleDialog.h"

#include <qmovie.h>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "Common/ColorMiscUtils.h"

#include "Application/CApplication.h"

#include <PopupWindows/CCustomColorDialog.h>

//
static OBSSource GetSource(OBSWeakSource weakSource) {
    return OBSGetStrongRef(weakSource);
}
static QVariant propertyListToQVariant(obs_property_t* prop, size_t idx) {
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

//
AFQAquaSubtitleDialog::AFQAquaSubtitleDialog(QWidget *parent, OBSSource source)
    : AFTTopBaseDialog(parent, Qt::WindowFlags()),
    ui(new Ui::AFQAquaSubtitleDialog),
    m_weakSource(OBSGetWeakRef(source)),
    m_props(obs_source_properties(source), obs_properties_destroy),
    removeSignal(obs_source_get_signal_handler(source), "remove",
        AFQAquaSubtitleDialog::_SourceRemoved, this)
{
    ui->setupUi(this);
    
#ifdef _WIN32
    ui->titleFrame->setProperty("MoveInAllArea", true);
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Dialog|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif

    _InitUIComponent();

    _RefreshStyle();

    obs_property_t* prop = obs_properties_get(m_props.get(), "get_anime_subtitle_info");
    obs_property_button_clicked(prop, GetSource(m_weakSource));

    _RefreshSubtitleInfo(true);
    QTimer::singleShot(500, this, [this](){
        _RefreshSubtitleInfo(true);
        });

    //

    connect(ui->pushButton_SubTitleSetting, &QPushButton::clicked, 
        this, &AFQAquaSubtitleDialog::_qslotSubTitleSettingButtonClicked);

    connect(ui->pushButton_DetailSetting, &QPushButton::clicked,
        this, &AFQAquaSubtitleDialog::_qslotDetailSettingButtonClicked);

    connect(ui->comboBox_Style, &QComboBox::currentIndexChanged,
        this, &AFQAquaSubtitleDialog::_qslotStyleIndexChanged);

    //
    connect(ui->pushButton_Close, &QPushButton::clicked,
        this, &AFQAquaSubtitleDialog::close);

    connect(ui->buttonBox, &QDialogButtonBox::clicked,
        this, &AFQAquaSubtitleDialog::_qslotClickedButtonBox);
}

AFQAquaSubtitleDialog::~AFQAquaSubtitleDialog()
{
    delete ui;
}

void AFQAquaSubtitleDialog::_qslotSubTitleSettingButtonClicked()
{
    ui->pushButton_SubTitleSetting->setChecked(true);
    ui->pushButton_DetailSetting->setChecked(false);
    ui->stackedWidget->setCurrentIndex(0);
}

void AFQAquaSubtitleDialog::_qslotDetailSettingButtonClicked()
{
    ui->pushButton_DetailSetting->setChecked(true);
    ui->pushButton_SubTitleSetting->setChecked(false);
    ui->stackedWidget->setCurrentIndex(1);
}


void AFQAquaSubtitleDialog::_qslotFontTransparencySliderChanged(int value)
{
    ui->spinBox_FontTransparency->setValue(value);
}

void AFQAquaSubtitleDialog::_qslotFontTransparencySpinboxChanged(int value)
{
    ui->slider_FontTransparency->setValue(value);
}

void AFQAquaSubtitleDialog::_qslotFontOutlinSizeSliderChanged(int value)
{
    ui->spinBox_OutlineSize->setValue(value);
}

void AFQAquaSubtitleDialog::_qslotFontOutlinSizeSpinboxChanged(int value)
{
    ui->slider_OutlineSize->setValue(value);
}

void AFQAquaSubtitleDialog::_qslotBackgroundTransparencySliderChanged(int value)
{
    ui->spinBox_BackgroundColor->setValue(value);
}

void AFQAquaSubtitleDialog::_qslotBackgroundTransparencySpinboxChanged(int value)
{
    ui->slider_BackgroundColor->setValue(value);
}

void AFQAquaSubtitleDialog::_qslotAnimationSpeedSliderChanged(int value)
{
    ui->spinBox_AnimationSpeed->setValue(value);
}

void AFQAquaSubtitleDialog::_qslotAnimationSpeedSpinboxChanged(int value)
{
    ui->slider_AnimationSpeed->setValue(value);
}

void AFQAquaSubtitleDialog::_qslotAnimationRepeatChanged(int value)
{
    bool useAnimation = (0 == value);
    _SetAnimationEnableUI(useAnimation);
}

void AFQAquaSubtitleDialog::_qslotAnimeThemeButtonClicked(int index)
{
    OBSDataAutoRelease settings = obs_source_get_settings(GetSource(m_weakSource));
    const int currentThemeIdx = obs_data_get_int(settings, "subtitle_theme");
    if (currentThemeIdx != index) {
        _SetAnimeSubtitleThemeUI(index);
        _SetFontOptionEnableUI(index);
    } 
    else {
        _RefreshSubtitleInfo();
    }
}

void AFQAquaSubtitleDialog::_qslotStyleIndexChanged(int index)
{
    if (!m_weakSource) {
        return;
    } 

    OBSSource source = GetSource(m_weakSource);
    obs_property_t* prop = obs_properties_get(m_props.get(), "style_setting");
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_int(settings, "style_index", index);
    obs_source_update(source, settings);
    obs_property_modified(prop, settings);

    QTimer::singleShot(500, this, [this]() {
        _RefreshSubtitleInfo(true);
        });
}

void AFQAquaSubtitleDialog::_qslotClickedButtonBox(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

    if (val == QDialogButtonBox::AcceptRole)
    {
        _UpdateAnimeSubtitleInfo();
        accept();
    }
    else if(val == QDialogButtonBox::ApplyRole)
    {
        if (!m_weakSource)
            return;

        _UpdateAnimeSubtitleInfo();
        return;
    }
    else if (val == QDialogButtonBox::ResetRole)
    {
        int result = AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            this, QT_UTF8(""), QTStr("Aqua.AnimeSubtitle.Reset.Message"), QTStr("Reset"));

        if (result == QDialog::Accepted) {
            _ResetAnimeSubtitleInfo();
            _UpdateAnimeSubtitleInfo();
        }
        return;
    }
    close();
}


void AFQAquaSubtitleDialog::_qslotFontColorChanged()
{
    QColorDialog::ColorDialogOptions options;

#ifdef __linux__
    // TODO: Revisit hang on Ubuntu with native dialog
    options |= QColorDialog::DontUseNativeDialog;
#endif

#ifdef _WIN32	
    QColor color = AFQCustomColorDialog::getColor(m_fontColor, this, Str("CustomColorDialog.title"), options);
#else
    color = QColorDialog::getColor(color, view, Str("CustomColorDialog.title"), options);
#endif
    if (!color.isValid())
        return;

    _SetColorLabel(ui->label_FontColor, color);

    m_fontColor = color;
}

void AFQAquaSubtitleDialog::_qslotOutlineColorChanged()
{
    QColorDialog::ColorDialogOptions options;

#ifdef __linux__
    // TODO: Revisit hang on Ubuntu with native dialog
    options |= QColorDialog::DontUseNativeDialog;
#endif

#ifdef _WIN32	
    QColor color = AFQCustomColorDialog::getColor(m_outlineColor, this, Str("CustomColorDialog.title"), options);
#else
    color = QColorDialog::getColor(color, view, Str("CustomColorDialog.title"), options);
#endif
    if (!color.isValid())
        return;

    _SetColorLabel(ui->label_OutlineColor, color);

    m_outlineColor = color;
}

void AFQAquaSubtitleDialog::_qslotBackgroundColorChanged()
{
    QColorDialog::ColorDialogOptions options;

#ifdef __linux__
    // TODO: Revisit hang on Ubuntu with native dialog
    options |= QColorDialog::DontUseNativeDialog;
#endif

#ifdef _WIN32	
    QColor color = AFQCustomColorDialog::getColor(m_backgroundColor, this, Str("CustomColorDialog.title"), options);
#else
    color = QColorDialog::getColor(color, view, Str("CustomColorDialog.title"), options);
#endif
    if (!color.isValid())
        return;

    _SetColorLabel(ui->label_BackgroundColor, color);

    m_backgroundColor = color;
}

void AFQAquaSubtitleDialog::_InitUIComponent()
{
    if (!m_weakSource)
        return;

    QPushButton* resetButton = ui->buttonBox->button(QDialogButtonBox::Reset);
    ChangeStyleSheet(resetButton, STYLESHEET_RESET_BUTTON);
    ChangeStyleSheet(ui->pushButton_FontColor, STYLESHEET_RESET_BUTTON);
    ChangeStyleSheet(ui->pushButton_OutlineColor, STYLESHEET_RESET_BUTTON);
    ChangeStyleSheet(ui->pushButton_BackgroundColor, STYLESHEET_RESET_BUTTON);

    OBSSource source = GetSource(m_weakSource);
    const char* name = obs_source_get_name(source);

    ui->label_Title->setText(name);

    ui->stackedWidget->setCurrentIndex(0);

    m_themeButtons = {
        ui->widget_Theme_0, ui->widget_Theme_1, ui->widget_Theme_2, ui->widget_Theme_3,
        ui->widget_Theme_4, ui->widget_Theme_5, ui->widget_Theme_6, ui->widget_Theme_7
    };

    m_themeButtonsChecked = {
        ui->label_CheckedTheme_0, ui->label_CheckedTheme_1, ui->label_CheckedTheme_2, ui->label_CheckedTheme_3,
        ui->label_CheckedTheme_4, ui->label_CheckedTheme_5, ui->label_CheckedTheme_6, ui->label_CheckedTheme_7
    };

    QList<QLabel*> themeGIFs = {
        ui->label_ThemeGIF_0, ui->label_ThemeGIF_1, ui->label_ThemeGIF_2, ui->label_ThemeGIF_3,
        ui->label_ThemeGIF_4, ui->label_ThemeGIF_5, ui->label_ThemeGIF_6, ui->label_ThemeGIF_7
    };

    for (int idx = 0; idx < m_themeButtons.size(); ++idx) {

        AFQHoverWidget* widget = m_themeButtons.at(idx);
        connect(widget, &AFQHoverWidget::qsignalMouseClick,
            this, [this, idx]() {
                _qslotAnimeThemeButtonClicked(idx);
            });

        std::string absPath;
        GetDataFilePath("assets", absPath);
        QString gifPath = QString("%1/Popup/dowoomi/animation_subtitle/subtitle_theme_%2.gif").
            arg(absPath.data()).arg(idx);

        QLabel* gifLabel = themeGIFs.at(idx);
        gifLabel->setAlignment(Qt::AlignCenter);
        gifLabel->setScaledContents(true);
        
        QMovie* movie = new QMovie(gifPath, QByteArray(), gifLabel);
        gifLabel->setMovie(movie);
        movie->start();
    }

    ui->plainTextEdit->setMaxLength(400);
    ui->plainTextEdit_2->setMaxLength(400);

    connect(ui->slider_FontTransparency, &QSlider::valueChanged,
        this, &AFQAquaSubtitleDialog::_qslotFontTransparencySliderChanged);

    connect(ui->spinBox_FontTransparency, &QSpinBox::valueChanged,
        this, &AFQAquaSubtitleDialog::_qslotFontTransparencySpinboxChanged);

    connect(ui->slider_OutlineSize, &QSlider::valueChanged,
        this, &AFQAquaSubtitleDialog::_qslotFontOutlinSizeSliderChanged);

    connect(ui->spinBox_OutlineSize, &QSpinBox::valueChanged,
        this, &AFQAquaSubtitleDialog::_qslotFontOutlinSizeSpinboxChanged);

    connect(ui->slider_BackgroundColor, &QSlider::valueChanged,
        this, &AFQAquaSubtitleDialog::_qslotBackgroundTransparencySliderChanged);

    connect(ui->spinBox_BackgroundColor, &QSpinBox::valueChanged,
        this, &AFQAquaSubtitleDialog::_qslotBackgroundTransparencySpinboxChanged);

    connect(ui->slider_AnimationSpeed, &QSlider::valueChanged,
        this, &AFQAquaSubtitleDialog::_qslotAnimationSpeedSliderChanged);

    connect(ui->spinBox_AnimationSpeed, &QSpinBox::valueChanged,
        this, &AFQAquaSubtitleDialog::_qslotAnimationSpeedSpinboxChanged);

    connect(ui->pushButton_FontColor, &QPushButton::clicked, 
        this, &AFQAquaSubtitleDialog::_qslotFontColorChanged);

    connect(ui->pushButton_OutlineColor, &QPushButton::clicked,
        this, &AFQAquaSubtitleDialog::_qslotOutlineColorChanged);

    connect(ui->pushButton_BackgroundColor, &QPushButton::clicked,
        this, &AFQAquaSubtitleDialog::_qslotBackgroundColorChanged);

    connect(ui->comboBox_UseAnimation, &QComboBox::currentIndexChanged,
        this, &AFQAquaSubtitleDialog::_qslotAnimationRepeatChanged);
}


void AFQAquaSubtitleDialog::_SetColorLabel(QLabel* colorLabel, QColor color)
{
	if (!colorLabel)
		return;

	colorLabel->setEnabled(false);

	QColor::NameFormat format = QColor::HexRgb;
	color.setAlpha(255);

	QPalette palette = QPalette(color);
	colorLabel->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	colorLabel->setText(color.name(format));
	colorLabel->setPalette(palette);
	colorLabel->setStyleSheet(
		QString("QLabel { background-color :%1; color: %2; }")
		.arg(palette.color(QPalette::Window).name(format))
		.arg(palette.color(QPalette::WindowText).name(format)));
	colorLabel->setAutoFillBackground(true);
	colorLabel->setAlignment(Qt::AlignCenter);
}

void AFQAquaSubtitleDialog::_SetThemeButtonWidget(int themeIdx)
{
    for (int i = 0; i < m_themeButtons.size(); ++i) {
        bool isActive = (i == themeIdx);
        m_themeButtons[i]->setProperty("checked", isActive);
        PolishStyleSheet(m_themeButtons[i]);

        m_themeButtonsChecked[i]->setVisible(isActive);
    }

    if (2 == themeIdx || 5 == themeIdx)
        ui->frame_SubText->show();
    else
        ui->frame_SubText->hide();

    m_checkedThemeIdx = themeIdx;
}

void AFQAquaSubtitleDialog::_RefreshStyle()
{
    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);

    QVariant data = obs_data_get_string(settings, "style_setting");
    QString dataurl = data.toString();

    obs_property_t* prop = obs_properties_get(m_props.get(), "style_setting");
    size_t count = obs_property_list_item_count(prop);

    ui->comboBox_Style->blockSignals(true);
    ui->comboBox_Style->clear();

    for (size_t idx = 0; idx < count; idx++)
    {
        QString name = QString("%1 %2").arg(QTStr("Style")).arg(idx + 1);
        QVariant var = propertyListToQVariant(prop, idx);
        ui->comboBox_Style->addItem(name, var);
        QString varurl = var.toString();
    }

    int style_index = obs_data_get_int(settings, "style_index");
    ui->comboBox_Style->setCurrentIndex(style_index);
    ui->comboBox_Style->blockSignals(false);
}

void AFQAquaSubtitleDialog::_RefreshSubtitleInfo(bool init)
{
    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);

    const int subtitle_theme = obs_data_get_int(settings, "subtitle_theme");
    _SetThemeButtonWidget(subtitle_theme);
    _SetFontOptionEnableUI(subtitle_theme);

    const std::string fontFace = obs_data_get_string(settings, "font_face");
    ui->comboBox_FontFace->setCurrentText(fontFace.c_str());

    if (init) {
        const int fontSize = obs_data_get_int(settings, "font_size");
        ui->spinBox_FontSize->setValue(fontSize);
    }

    const bool fontBold = obs_data_get_bool(settings, "font_bold");
    ui->checkBox_FontBold->setChecked(fontBold);

    const int fontAlignType = obs_data_get_int(settings, "font_align_type");
    ui->comboBox_FontAlignType->setCurrentIndex(fontAlignType - 1);

    std::string fontColor = obs_data_get_string(settings, "font_color");
    m_fontColor = QColor(fontColor.c_str());
    _SetColorLabel(ui->label_FontColor, m_fontColor);

    int fontTransparency = obs_data_get_int(settings, "font_transparency");
    ui->slider_FontTransparency->setValue(fontTransparency);
    ui->spinBox_FontTransparency->setValue(fontTransparency);

    bool useOutline = obs_data_get_bool(settings, "font_outline_display");
    ui->checkBox_Outline->setChecked(useOutline);

    std::string outlineColor = obs_data_get_string(settings, "font_outline_color");
    m_outlineColor = QColor(outlineColor.c_str());
    _SetColorLabel(ui->label_OutlineColor, m_outlineColor);

    int outlineSize = obs_data_get_int(settings, "font_outline_size");
    ui->slider_OutlineSize->setValue(outlineSize);
    ui->spinBox_OutlineSize->setValue(outlineSize);

    bool useBackground = obs_data_get_bool(settings, "display_background");
    ui->checkBox_Background->setChecked(useBackground);

    std::string BackgroundColor = obs_data_get_string(settings, "background_color");
    m_backgroundColor = QColor(BackgroundColor.c_str());
    _SetColorLabel(ui->label_BackgroundColor, m_backgroundColor);

    int backgroundTransparency = obs_data_get_int(settings, "background_transparency");
    ui->slider_BackgroundColor->setValue(100 - backgroundTransparency);
    ui->spinBox_BackgroundColor->setValue(100 - backgroundTransparency);

    bool useAnimation = obs_data_get_bool(settings, "use_animation");
    ui->comboBox_UseAnimation->setCurrentIndex(useAnimation == true ? 0 : 1);
    _SetAnimationEnableUI(useAnimation);

    int animationSpeed = obs_data_get_int(settings, "animation_speed");
    ui->slider_AnimationSpeed->setValue(animationSpeed);
    ui->spinBox_AnimationSpeed->setValue(animationSpeed);


    if (init) {
        QString subTitleText = QString::fromStdString(obs_data_get_string(settings, "subtitle_text"));
        subTitleText.replace("\\n", "\n");
        if (subTitleText.compare("애니메이션 자막을\n입력해주세요.") == 0) // Server Default
            subTitleText = QTStr("Aqua.AnimeSubtitle.Text.Default"); // UI Default

        const std::string subtitle_text = subTitleText.toStdString();

        size_t pos = subtitle_text.find("$$");
        if (pos != std::string::npos)
        {
            std::string part1 = subtitle_text.substr(0, pos);
            std::string part2 = subtitle_text.substr(pos + 2);
            ui->plainTextEdit->setPlainText(QString::fromStdString(part1));
            ui->plainTextEdit_2->setPlainText(QString::fromStdString(part2));
        }
        else
        {
            ui->plainTextEdit->setPlainText(QString::fromStdString(subtitle_text));
            ui->plainTextEdit_2->clear();
        }
    }
}

void AFQAquaSubtitleDialog::_SetFontOptionEnableUI(int themeIdx)
{
    bool enableFontOption = !(themeIdx == 3 || themeIdx == 4 || themeIdx == 7);

    ui->checkBox_Outline->setEnabled(enableFontOption);
    ui->label_OutlineColor->setEnabled(enableFontOption);
    ui->pushButton_OutlineColor->setEnabled(enableFontOption);
    ui->slider_OutlineSize->setEnabled(enableFontOption);
    ui->spinBox_OutlineSize->setEnabled(enableFontOption);

    ui->slider_FontTransparency->setEnabled(enableFontOption);
    ui->spinBox_FontTransparency->setEnabled(enableFontOption);
    ui->pushButton_FontColor->setEnabled(enableFontOption);

    if (enableFontOption) {
        _SetColorLabel(ui->label_FontColor, m_fontColor);
        _SetColorLabel(ui->label_OutlineColor, m_outlineColor);
    }
    else {
        QColor disableColor = m_outlineColor.darker(140);
        _SetColorLabel(ui->label_OutlineColor, disableColor);

        QColor disableFontColor = m_fontColor.darker(140);
        _SetColorLabel(ui->label_FontColor, disableFontColor);
    }
}

void AFQAquaSubtitleDialog::_SetAnimationEnableUI(int useAnimation)
{
    ui->slider_AnimationSpeed->setEnabled(useAnimation);
    ui->spinBox_AnimationSpeed->setEnabled(useAnimation);
}

void AFQAquaSubtitleDialog::_ResetAnimeSubtitleInfo()
{
    _SetAnimeSubtitleThemeUI(m_checkedThemeIdx, true);

    return;
}

void AFQAquaSubtitleDialog::_SetAnimeSubtitleThemeUI(int themeIdx, bool reset)
{
    auto setFontFaceByName = [&](const QString& family) {
        int idx = ui->comboBox_FontFace->findText(family, Qt::MatchFixedString);
        if (idx < 0) idx = ui->comboBox_FontFace->findText(family, Qt::MatchContains);
        if (idx >= 0) ui->comboBox_FontFace->setCurrentIndex(idx);
    };

    bool useOutline = false;
    bool bold = false;
    int alignType = 2;
    QColor fontColor(255, 255, 255);
    QColor outlineColor(0, 0, 0);
    QColor bgColor(0, 0, 0);
    int outlineSize = 2;
    int fontTransparency = 100;
    bool useBg = false;
    int animSpeed = 50;
    QString fontFace = QStringLiteral("나눔고딕");

    switch (themeIdx) {
    case 0:
    case 1:
        useOutline = true;
        alignType = 2;
        break;

    case 2:
        useOutline = true;
        alignType = 2;
        break;

    case 3:
        useOutline = false;
        alignType = 2;
        break;

    case 4:
        bold = true;
        useOutline = false;
        alignType = 1;
        break;

    case 5:
        useOutline = false;
        alignType = 2;
        break;

    case 6:
        fontFace = QStringLiteral("쿠키런 글꼴");
        bold = true;
        fontColor = QColor(0, 0, 0);
        useOutline = false;
        alignType = 2;
        break;

    case 7:
        fontFace = QStringLiteral("쿠키런 글꼴");
        bold = true;
        fontColor = QColor(255, 255, 255);
        useOutline = false;
        alignType = 1;
        break;

    default:
        useOutline = true;
        alignType = 2;
        break;
    }

    setFontFaceByName(fontFace);
    ui->checkBox_FontBold->setChecked(bold);

    int alignIndex = (alignType == 2) ? 1 : 0;
    ui->comboBox_FontAlignType->setCurrentIndex(alignIndex);

    if(reset)
        ui->spinBox_FontSize->setValue(35);

    m_fontColor = fontColor;
    _SetColorLabel(ui->label_FontColor, m_fontColor);

    ui->slider_FontTransparency->setValue(fontTransparency);
    ui->spinBox_FontTransparency->setValue(fontTransparency);

    ui->checkBox_Outline->setChecked(useOutline);
    m_outlineColor = outlineColor;
    _SetColorLabel(ui->label_OutlineColor, m_outlineColor);
    ui->slider_OutlineSize->setValue(outlineSize);
    ui->spinBox_OutlineSize->setValue(outlineSize);

    ui->checkBox_Background->setChecked(useBg);
    m_backgroundColor = bgColor;
    _SetColorLabel(ui->label_BackgroundColor, m_backgroundColor);
    ui->slider_BackgroundColor->setValue(0);
    ui->spinBox_BackgroundColor->setValue(0);

    ui->comboBox_UseAnimation->setCurrentIndex(0);
    ui->slider_AnimationSpeed->setValue(animSpeed);
    ui->spinBox_AnimationSpeed->setValue(animSpeed);

    const bool useSubText = (themeIdx == 2 || themeIdx == 5);
    if (ui->plainTextEdit->toPlainText().trimmed().isEmpty() || reset) {
        ui->plainTextEdit->setPlainText(QTStr("Aqua.AnimeSubtitle.Text.Default"));
    }

    if (useSubText) {
        if (ui->plainTextEdit_2->toPlainText().trimmed().isEmpty() || reset) {
            ui->plainTextEdit_2->setPlainText(QTStr("Aqua.AnimeSubtitle.Text.SubDefault"));
        }
    }

    _SetThemeButtonWidget(themeIdx);
}

void AFQAquaSubtitleDialog::_UpdateAnimeSubtitleInfo()
{
    if (!m_weakSource) {
        return;
    }

    OBSDataAutoRelease settings = obs_source_get_settings(GetSource(m_weakSource));

    QString text = ui->plainTextEdit->toPlainText();

    if (2 == m_checkedThemeIdx || 5 == m_checkedThemeIdx) {
        QString subText = ui->plainTextEdit_2->toPlainText();
        text = QString("%1$$%2").arg(text).arg(subText);
    }

    obs_data_set_int(settings, "subtitle_theme", m_checkedThemeIdx);

    QString escapedText = text;
    if (escapedText.compare(QTStr("Aqua.AnimeSubtitle.Text.Default")) == 0) // UI Default
        escapedText = "애니메이션 자막을\n입력해주세요."; // Server Default
    
    obs_data_set_string(settings, "subtitle_text", escapedText.toStdString().c_str());

    QString fontFace = ui->comboBox_FontFace->currentText();
    obs_data_set_string(settings, "font_face", fontFace.toStdString().c_str());

    int fontSize = ui->spinBox_FontSize->value();
    obs_data_set_int(settings, "font_size", fontSize);

    bool fontBold = ui->checkBox_FontBold->isChecked();
    obs_data_set_bool(settings, "font_bold", fontBold);

    int fontAlignType = ui->comboBox_FontAlignType->currentIndex() + 1;
    obs_data_set_int(settings, "font_align_type", fontAlignType);

    std::string fontColor = m_fontColor.name(QColor::HexRgb).toStdString();
    obs_data_set_string(settings, "font_color", fontColor.c_str());

    int fontTransparency = ui->slider_FontTransparency->value();
    obs_data_set_int(settings, "font_transparency", fontTransparency);

    bool useOutline = ui->checkBox_Outline->isChecked();
    obs_data_set_bool(settings, "font_outline_display", useOutline);

    std::string outlineColor = m_outlineColor.name(QColor::HexRgb).toStdString();
    obs_data_set_string(settings, "font_outline_color", outlineColor.c_str());

    int outlineSize = ui->slider_OutlineSize->value();
    obs_data_set_int(settings, "font_outline_size", outlineSize);

    bool useBackground = ui->checkBox_Background->isChecked();
    obs_data_set_bool(settings, "display_background", useBackground);

    std::string BackgroundColor = m_backgroundColor.name(QColor::HexRgb).toStdString();
    obs_data_set_string(settings, "background_color", BackgroundColor.c_str());

    int backgroundTransparency = 100 - ui->slider_BackgroundColor->value();
    obs_data_set_int(settings, "background_transparency", backgroundTransparency);

    bool useAnimation = ui->comboBox_UseAnimation->currentIndex() == 0 ? true : false;
    obs_data_set_bool(settings, "use_animation", useAnimation);

    int animationSpeed = ui->slider_AnimationSpeed->value();
    obs_data_set_int(settings, "animation_speed", animationSpeed);

    obs_property_t* prop = obs_properties_get(m_props.get(), "update_anime_subtitle_info");
    obs_property_button_clicked(prop, GetSource(m_weakSource));
}

void AFQAquaSubtitleDialog::_SourceRemoved(void* data, calldata_t* params)
{
    QMetaObject::invokeMethod(static_cast<AFQAquaSubtitleDialog*>(data),
        "close");
}