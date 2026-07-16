#include "CDowoomiScoreDialog.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Locale/CLocaleTextManager.h"

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
AFQDowoomiScoreDialog::AFQDowoomiScoreDialog(QWidget *parent, OBSSource source)
    : AFTTopBaseDialog(parent, Qt::WindowFlags()),
    ui(new Ui::AFQDowoomiScoreDialog),
    m_weakSource(OBSGetWeakRef(source)), 
    m_props(obs_source_properties(source), obs_properties_destroy),
    removeSignal(obs_source_get_signal_handler(source), "remove",
        AFQDowoomiScoreDialog::_SourceRemoved, this)
{
    ui->setupUi(this);
    
#ifdef _WIN32
    ui->titleFrame->setProperty("MoveInAllArea", true);
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Dialog|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif

    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);
    setAttribute(Qt::WA_DeleteOnClose);

    QString locale = QString::fromStdString(LOCALE_CONTEXT.GetCurrentLocaleStr());
    if (locale != "ko-KR") {
        ui->label_ThemeSimpleImg->setProperty("lang", "");
        PolishStyleSheet(ui->label_ThemeSimpleImg);

        ui->label_ThemeGameImg->setProperty("lang", "");
        PolishStyleSheet(ui->label_ThemeGameImg);

        ui->label_ThemeSportImg->setProperty("lang", "");
        PolishStyleSheet(ui->label_ThemeSportImg);
    }

    // initialize ui
    ui->spinBox_LeftScore->setMinimum(0);
    ui->spinBox_LeftScore->setMaximum(99);
    ui->spinBox_RightScore->setMinimum(0);
    ui->spinBox_RightScore->setMaximum(99);
    ui->spinBox_LeftSet->setMinimum(0);
    ui->spinBox_LeftSet->setMaximum(9);
    ui->spinBox_RightSet->setMinimum(0);
    ui->spinBox_RightSet->setMaximum(9);

    OBSDataAutoRelease settings = obs_source_get_settings(source);

    int width = obs_data_get_int(settings, "width");
    int height = obs_data_get_int(settings, "height");
    ui->spinBox_Width->setValue(width);
    ui->spinBox_Height->setValue(height);

    obs_property_t* prop = obs_properties_get(m_props.get(), "refresh_score_info");
    if (prop)
    {
        obs_property_button_clicked(prop, GetSource(m_weakSource));
    }
    

    _GetScoreInfo();

    std::string source_id = obs_source_get_id(source);
    QString source_name = obs_source_get_display_name(source_id.c_str());

#ifdef _WIN32
    ui->label_Title->setText(QTStr("Dowoomi.Score.Title").arg(source_name));
#elif defined(__APPLE__)
    setWindowTitle(QTStr("Dowoomi.Score.Title").arg(source_name));
#endif

    connect(ui->buttonClose, &QPushButton::clicked,
        this, &AFQDowoomiScoreDialog::close);

    connect(ui->button_ClearAll, &QPushButton::clicked, this, &AFQDowoomiScoreDialog::qslotClearAll);
    connect(ui->lineEdit_LeftName, &QLineEdit::textChanged, this, &AFQDowoomiScoreDialog::qslotNameChanged);
    connect(ui->lineEdit_RightName, &QLineEdit::textChanged, this, &AFQDowoomiScoreDialog::qslotNameChanged);
    connect(ui->spinBox_LeftScore, &QSpinBox::valueChanged, this, &AFQDowoomiScoreDialog::qslotScoreChanged);
    connect(ui->spinBox_RightScore, &QSpinBox::valueChanged, this, &AFQDowoomiScoreDialog::qslotScoreChanged);
    connect(ui->checkbox_UseSet, &QCheckBox::clicked, this, &AFQDowoomiScoreDialog::qslotUseSet);
    connect(ui->spinBox_LeftSet, &QSpinBox::valueChanged, this, &AFQDowoomiScoreDialog::qslotSetChanged);
    connect(ui->spinBox_RightSet, &QSpinBox::valueChanged, this, &AFQDowoomiScoreDialog::qslotSetChanged);
    connect(ui->checkbox_UseTimer, &QCheckBox::clicked, this, &AFQDowoomiScoreDialog::qslotUseTimer);
    connect(ui->pushButton_Play, &QPushButton::clicked, this, &AFQDowoomiScoreDialog::qslotTimerPlay);
    connect(ui->pushButton_Pause, &QPushButton::clicked, this, &AFQDowoomiScoreDialog::qslotTimerPause);
    connect(ui->pushButton_Reset, &QPushButton::clicked, this, &AFQDowoomiScoreDialog::qslotTimerReset);
    connect(ui->widget_ThemeSimple, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiScoreDialog::qslotSetThema);
    connect(ui->widget_ThemeGame, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiScoreDialog::qslotSetThema);
    connect(ui->widget_ThemeSport, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiScoreDialog::qslotSetThema);
    connect(ui->widget_Toolip, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiScoreDialog::qslotHelpPopupClicked);
    connect(ui->pushButton_Size, &QPushButton::clicked, this, &AFQDowoomiScoreDialog::qslotBrowserSizeClicked);

    _SetupAutoRefresh();
}


AFQDowoomiScoreDialog::~AFQDowoomiScoreDialog()
{
    delete ui;
}
//
void AFQDowoomiScoreDialog::qslotClearAll()
{
    if(!m_weakSource) {
        return;
    }

    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    //int thema_idx = obs_data_get_int(settings, "skin_idx");
    //AFQHoverWidget* widget = nullptr;
    //if(1 == thema_idx)
    //    widget = ui->widget_ThemeSimple;
    //else if(2 == thema_idx)
    //    widget = ui->widget_ThemeGame;
    //else if(3 == thema_idx)
    //    widget = ui->widget_ThemeSport;
    //widget->setProperty("IsSelected", false);
    //PolishStyleSheet(widget);
    //

    obs_property_t* prop = obs_properties_get(m_props.get(), "score_clear");
    obs_property_button_clicked(prop, source);
    //
    _GetScoreInfo();
}
void AFQDowoomiScoreDialog::qslotNameChanged(const QString& text)
{
    if(!m_weakSource) {
        return;
    }

    OBSDataAutoRelease settings = obs_source_get_settings(GetSource(m_weakSource));
    QWidget* senderWidget = qobject_cast<QWidget*>(sender());
    if(ui->lineEdit_LeftName == senderWidget) {
        obs_data_set_string(settings, "score_blue_name", QT_TO_UTF8(text));
    } else if(ui->lineEdit_RightName == senderWidget) {
        obs_data_set_string(settings, "score_red_name", QT_TO_UTF8(text));
    }
    _UpdateScoreInfo();
}
void AFQDowoomiScoreDialog::qslotScoreChanged(int score)
{
    if(!m_weakSource) {
        return;
    }

    OBSDataAutoRelease settings = obs_source_get_settings(GetSource(m_weakSource));
    QWidget* senderWidget = qobject_cast<QWidget*>(sender());
    if(ui->spinBox_LeftScore == senderWidget) {
        obs_data_set_int(settings, "score_blue_num", score);
    } else if(ui->spinBox_RightScore == senderWidget) {
        obs_data_set_int(settings, "score_red_num", score);
    }
    _UpdateScoreInfo();
}
void AFQDowoomiScoreDialog::qslotUseSet(bool checked)
{
    if(!m_weakSource) {
        return;
    }

    ui->widget_SetScore->setEnabled(checked);

    OBSSource source = GetSource(m_weakSource);
    // 
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_bool(settings, "score_set", checked);
    _UpdateScoreInfo();
}
void AFQDowoomiScoreDialog::qslotSetChanged(int set)
{
    if(!m_weakSource) {
        return;
    }

    OBSDataAutoRelease settings = obs_source_get_settings(GetSource(m_weakSource));
    QWidget* senderWidget = qobject_cast<QWidget*>(sender());
    if(ui->spinBox_LeftSet == senderWidget) {
        obs_data_set_int(settings, "score_blue_set", set);
    } else if(ui->spinBox_RightSet == senderWidget) {
        obs_data_set_int(settings, "score_red_set", set);
    }
    _UpdateScoreInfo();
}
void AFQDowoomiScoreDialog::qslotUseTimer(bool checked)
{
    if(!m_weakSource) {
        return;
    }

    ui->widget_Time->setEnabled(checked);

    OBSDataAutoRelease settings = obs_source_get_settings(GetSource(m_weakSource));
    obs_data_set_bool(settings, "score_timer", checked);
    _UpdateScoreInfo();
}
void AFQDowoomiScoreDialog::qslotTimerPlay()
{
    if(!m_weakSource) {
        return;
    }

    obs_property_t* prop = obs_properties_get(m_props.get(), "score_timer_play");
    obs_property_button_clicked(prop, GetSource(m_weakSource));
}
void AFQDowoomiScoreDialog::qslotTimerPause()
{
    if(!m_weakSource) {
        return;
    }

    obs_property_t* prop = obs_properties_get(m_props.get(), "score_timer_pause");
    obs_property_button_clicked(prop, GetSource(m_weakSource));
}
void AFQDowoomiScoreDialog::qslotTimerReset()
{
    if(!m_weakSource) {
        return;
    }

    obs_property_t* prop = obs_properties_get(m_props.get(), "score_timer_reset");
    obs_property_button_clicked(prop, GetSource(m_weakSource));
}
void AFQDowoomiScoreDialog::qslotSetThema()
{
    if(!m_weakSource) {
        return;
    }

    OBSDataAutoRelease settings = obs_source_get_settings(GetSource(m_weakSource));
    int old_thema = obs_data_get_int(settings, "skin_idx");
    AFQHoverWidget* widget = nullptr;
    if(1 == old_thema)
        widget = ui->widget_ThemeSimple;
    else if(2 == old_thema)
        widget = ui->widget_ThemeGame;
    else if(3 == old_thema)
        widget = ui->widget_ThemeSport;
    widget->setProperty("IsSelected", false);
    PolishStyleSheet(widget);

    // new thema fill outline
    widget = reinterpret_cast<AFQHoverWidget*>(sender());
    int index = 1;
    if(ui->widget_ThemeSimple == widget)
        index = 1;
    else if(ui->widget_ThemeGame == widget)
        index = 2;
    else if(ui->widget_ThemeSport == widget)
        index = 3;
    widget->setProperty("IsSelected", true);
    PolishStyleSheet(widget);
    //
    
    obs_data_set_int(settings, "skin_idx", index);
    _UpdateScoreInfo();
}

void AFQDowoomiScoreDialog::qslotHelpPopupClicked()
{
    bool closed = true;
    if(m_dowoomiPopup)
        closed = m_dowoomiPopup->close();
    if(!closed)
        return;

    m_dowoomiPopup = new AFQDowoomiPopup(this, 1);
    m_dowoomiPopup->setAttribute(Qt::WA_DeleteOnClose, true);
    m_dowoomiPopup->setModal(false);

    setCenterPositionNotUseParent(m_dowoomiPopup, this);

    m_dowoomiPopup->show();
}

void AFQDowoomiScoreDialog::qslotBrowserSizeClicked()
{
    int width = ui->spinBox_Width->value();
    int height = ui->spinBox_Height->value();

    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_int(settings, "width", width);
    obs_data_set_int(settings, "height", height);
    obs_source_update(source, settings);
}

void AFQDowoomiScoreDialog::_GetScoreInfo()
{
    if(!m_weakSource) {
        return;
    }

    // get settings
    OBSDataAutoRelease settings = obs_source_get_settings(GetSource(m_weakSource));

    QString name_ = QT_UTF8(obs_data_get_string(settings, "score_blue_name"));
    ui->lineEdit_LeftName->setText(name_);
    
    name_ = QT_UTF8(obs_data_get_string(settings, "score_red_name"));
    ui->lineEdit_RightName->setText(name_);

    int score_ = obs_data_get_int(settings, "score_blue_num");
    ui->spinBox_LeftScore->setValue(score_);

    score_ = obs_data_get_int(settings, "score_red_num");
    ui->spinBox_RightScore->setValue(score_);

    bool use_set = obs_data_get_bool(settings, "score_set");
    ui->checkbox_UseSet->setChecked(use_set);
    ui->widget_SetScore->setEnabled(use_set);

    int set_ = obs_data_get_int(settings, "score_blue_set");
    ui->spinBox_LeftSet->setValue(set_);

    set_ = obs_data_get_int(settings, "score_red_set");
    ui->spinBox_RightSet->setValue(set_);

    //if(!use_set) {
    //    ui->spinBox_LeftSet->setEnabled(false);
    //    ui->spinBox_RightSet->setEnabled(false);
    //}

    bool use_timer = obs_data_get_bool(settings, "score_timer");
    ui->checkbox_UseTimer->setChecked(use_timer);
    ui->widget_Time->setEnabled(use_timer);

    //if(!use_timer) {
    //    ui->pushButton_Play->setEnabled(false);
    //    ui->pushButton_Pause->setEnabled(false);
    //    ui->pushButton_Reset->setEnabled(false);
    //}

    int thema_idx = obs_data_get_int(settings, "skin_idx");
    AFQHoverWidget* widget = nullptr;
    if(1 == thema_idx)
        widget = ui->widget_ThemeSimple;
    else if(2 == thema_idx)
        widget = ui->widget_ThemeGame;
    else if(3 == thema_idx)
        widget = ui->widget_ThemeSport;
    widget->setProperty("IsSelected", true);
    PolishStyleSheet(widget);
}

void AFQDowoomiScoreDialog::_UpdateScoreInfo()
{
    if(!m_weakSource) {
        return;
    }
    
    obs_property_t* prop = obs_properties_get(m_props.get(), "update_score_info");
    obs_property_button_clicked(prop, GetSource(m_weakSource));
}

void AFQDowoomiScoreDialog::_SourceRemoved(void* data, calldata_t* params)
{
    QMetaObject::invokeMethod(static_cast<AFQVodSourceDialog*>(data),
        "close");
}

void AFQDowoomiScoreDialog::_SetupAutoRefresh()
{
    m_refreshTimer = new QTimer(this);

    connect(m_refreshTimer, &QTimer::timeout, this, &AFQDowoomiScoreDialog::_HandleAutoRefresh);

    const int intervalMS = 5 * 1000;
    m_refreshTimer->start(intervalMS);
}

void AFQDowoomiScoreDialog::_HandleAutoRefresh()
{
    if (!m_weakSource) {
        return;
    }

    obs_property_t* prop = obs_properties_get(m_props.get(), "refresh_score_info");
    if (prop) {
        obs_property_button_clicked(prop, GetSource(m_weakSource));

        _GetScoreInfo();
    }
}
