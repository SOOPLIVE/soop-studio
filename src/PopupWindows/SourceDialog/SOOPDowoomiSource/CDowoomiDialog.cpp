
#include "CDowoomiDialog.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "properties-view.hpp"
#include "UIComponent/CCustomCombobox.h"
#include "Common/StudioDefine.h"
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
AFQDowoomiDialog::AFQDowoomiDialog(QWidget* parent, OBSSource source) :
    AFTTopBaseDialog(parent, Qt::WindowFlags()),
    ui(new Ui::AFQDowoomiDialog),
    m_weakSource(OBSGetWeakRef(source)),
    m_props(obs_source_properties(source), obs_properties_destroy),
    removeSignal(obs_source_get_signal_handler(source), "remove",
        AFQDowoomiDialog::_SourceRemoved, this)
{
    ui->setupUi(this);
    
#ifdef _WIN32
    ui->titleFrame->setProperty("MoveInAllArea", true);
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif

    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);
    setAttribute(Qt::WA_DeleteOnClose);

    OBSDataAutoRelease settings = obs_source_get_settings(source);
    bool shutdown = obs_data_get_bool(settings, "shutdown");
    ui->checkbox_ShutdownSourceNotVisible->setChecked(shutdown);

    bool reroute_audio = obs_data_get_bool(settings, "reroute_audio");
    ui->checkbox_RerouteAudio->setChecked(reroute_audio);
    
    std::string source_id = obs_source_get_id(source);
    m_sourceName = obs_source_get_display_name(source_id.c_str());

    if (source_id != "soop_chat_source_chat")
        ui->widget_Toolip->setVisible(false);

    _RefreshStyleCombobox();

    int width = obs_data_get_int(settings, "width");
    int height = obs_data_get_int(settings, "height");
    ui->spinBox_Width->setValue(width);
    ui->spinBox_Height->setValue(height);

    QString settingPopup = QTStr("Dowoomi.SettingPopup").arg(m_sourceName);

    ui->button_DetailStyle->setText(settingPopup);
    ui->button_DetailStyle->setStyleSheet("background-color: transparent; color: rgb(0, 163, 255); border: 1px solid rgb(0, 163, 255);");
    PolishStyleSheet(ui->button_DetailStyle);
    
    // connect action
    connect(ui->buttonClose, &QPushButton::clicked,
        this, &AFQDowoomiDialog::close);
    
    connect(ui->button_DetailStyle, &QPushButton::clicked, this, &AFQDowoomiDialog::qslotDetailSettingClicked);
    connect(ui->combo_Style, &QComboBox::currentIndexChanged, this, &AFQDowoomiDialog::qslotStyleChanged);
    connect(ui->checkbox_ShutdownSourceNotVisible, &QCheckBox::clicked, this, &AFQDowoomiDialog::qslotShutdownSourceNotVisibleChecked);
    connect(ui->checkbox_RerouteAudio, &QCheckBox::clicked, this, &AFQDowoomiDialog::qslotFreecshotControlAudio);
    connect(ui->button_Interactive, &QPushButton::clicked, this, &AFQDowoomiDialog::qslotInteractionClicked);
    connect(ui->button_Refresh, &QPushButton::clicked, this, &AFQDowoomiDialog::qslotRefreshClicked);
    connect(ui->widget_Toolip, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiDialog::qslotHelpPopupClicked);

    // Set Source Type UI
    if (0 == strcmp(source_id.c_str(), "soop_videoballoon_source"))
        IsVideoBalloonDialog(source);
    else if (nullptr != strstr(source_id.c_str(), "soop_kbo_graphic_source_"))
        IsKBODialog(source);
    else if (nullptr != strstr(source_id.c_str(), "soop_football_graphic_source_"))
        IsFootballDialog(source);
    else if (nullptr != strstr(source_id.c_str(), "soop_commerce_source_"))
        IsCommerceDialog(source);

    if (source_id == "soop_chat_source_subtitle")
    {
        ui->comboBox_SubtitleForm->blockSignals(true);

        ui->comboBox_SubtitleForm->addItem(QTStr("Dowoomi.Subtitle.Count"));
        ui->comboBox_SubtitleForm->addItem(QTStr("Dowoomi.Subtitle.Rank"));
        ui->comboBox_SubtitleForm->addItem("MVP");

        std::string type = obs_data_get_string(settings, "type_setting");
        int subTitleType = 0;
        try {
            size_t idx;
            subTitleType = std::stoi(type, &idx);
        }
        catch (const std::invalid_argument& e) {
            subTitleType = 0;
        }
        catch (const std::out_of_range& e) {
            subTitleType = 0;
        }

        ui->comboBox_SubtitleForm->setCurrentIndex(subTitleType);

        connect(ui->comboBox_SubtitleForm, &QComboBox::currentIndexChanged, this, &AFQDowoomiDialog::qslotTypeChanged);

        ui->comboBox_SubtitleForm->blockSignals(false);
    }
    else
    {
        ui->comboBox_SubtitleForm->close();
        ui->comboBox_SubtitleForm->deleteLater();

        ui->label_SubTitleForm->close();
        ui->label_SubTitleForm->deleteLater();
    }

    connect(ui->pushButton_Size, &QPushButton::clicked, this, &AFQDowoomiDialog::qslotBrowserSizeClicked);
    connect(this, &AFQDowoomiDialog::qsignalStyleChanged, DYNAMIC_COMPOSIT, &AFMainDynamicComposit::UpdateSourceToolbar);

    setFixedSize(720, 550);
    move(x(), y()-1);
    
}

AFQDowoomiDialog::~AFQDowoomiDialog()
{
    if (m_cefPopupProperties)
        m_cefPopupProperties->close();

    m_weakSource = nullptr;
    delete ui;
}

void AFQDowoomiDialog::IsKBODialog(OBSSource source)
{
    m_isKBO = true;

    QString title = m_sourceName + " " + QTStr("Properties");
    QString settingPopup = QTStr("KBO.List");

#ifdef _WIN32
    ui->label_Title->setText(title);
#elif defined(__APPLE__)
    setWindowTitle(title);
#endif

    ui->checkbox_RerouteAudio->setVisible(false);

    ui->button_DetailStyle->setText(settingPopup);

    ui->widget_SetScore->close();
    ui->combo_Style->close();
}

void AFQDowoomiDialog::IsFootballDialog(OBSSource source)
{
    m_isFootball = true;

    QString title = m_sourceName + " " + QTStr("Properties");
    QString settingPopup = QTStr("Football.List");

#ifdef _WIN32
    ui->label_Title->setText(title);
#elif defined(__APPLE__)
    setWindowTitle(title);
#endif

    ui->checkbox_RerouteAudio->setVisible(false);

    ui->button_DetailStyle->setText(settingPopup);

    ui->widget_SetScore->close();
    ui->combo_Style->close();
}


void AFQDowoomiDialog::IsCommerceDialog(OBSSource source)
{

    m_isCommerce = true;

    QString title = m_sourceName + " " + QTStr("Properties");
    QString settingPopup = QTStr("Live.Commerce.Setting");

#ifdef _WIN32
    ui->label_Title->setText(title);
#elif defined(__APPLE__)
    setWindowTitle(title);
#endif

    ui->checkbox_RerouteAudio->setVisible(false);

    ui->button_DetailStyle->setText(settingPopup);

    ui->widget_SetScore->close();
    ui->combo_Style->close();
    ui->button_Interactive->close();
}

void AFQDowoomiDialog::IsVideoBalloonDialog(OBSSource source)
{
    m_isVideoBalloon = true; 

    QString title = m_sourceName + " " + QTStr("Properties");
    QString settingPopup = QTStr("VideoBalloon.List");

#ifdef _WIN32
    ui->label_Title->setText(title);
#elif defined(__APPLE__)
    setWindowTitle(title);
#endif

    ui->button_DetailStyle->setText(settingPopup);

    ui->widget_SetScore->close();
    ui->combo_Style->close();
    ui->label_BrowserSize->close();
    ui->button_Interactive->close();
    ui->checkbox_ShutdownSourceNotVisible->close();

    QLayoutItem* item;
    while ((item = ui->horizontalLayout_4->takeAt(0)) != nullptr)
    {
        if (QWidget* widget = item->widget())
        {
            widget->setParent(nullptr);
            widget->deleteLater();
        }
        delete item;
    }
}
//
void AFQDowoomiDialog::qslotDetailSettingClicked()
{
    if(!m_weakSource) {
        return;
    }

    if (m_isKBO || m_isFootball || m_isCommerce)
    {
        MAINFRAME->CreateSoopCefDetailProperties(OBSGetStrongRef(m_weakSource));
        close();
    }
    else if(m_isVideoBalloon)
    {
        MAINFRAME->CreateSourceProperties(OBSGetStrongRef(m_weakSource));
    }
    else
    {
        std::string source_id = obs_source_get_id(OBSGetStrongRef(m_weakSource));
        std::string prefix = "soop_chat_source_";
        std::string type;
        if (source_id.find(prefix) == 0) {
            type = source_id.substr(prefix.length());

            if (type == "c_mission") {
                type = "funding";
            }

            if (type == "subtitle")
            {
                type = "subtitle_number";
            }

            QString url = QString::fromStdString(SOOP_DASHBOARD_OVERLAY_URL).arg(type.c_str());
            MAINFRAME->NavigateDefaultBrowser(url);
        }
    }

}
void AFQDowoomiDialog::qslotStyleChanged(int)
{
    if(!m_weakSource) {
        return;
    }
    
    QVariant data = ui->combo_Style->currentData();
    QString style = ui->combo_Style->currentText();

    ui->label_Title->setText(QTStr("Dowoomi.Title").arg(m_sourceName, style));

    OBSSource source = GetSource(m_weakSource);
    obs_property_t* prop = obs_properties_get(m_props.get(), "style_setting");
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_string(settings, "style_setting", data.toByteArray().constData());
    obs_source_update(source, settings);
    obs_property_modified(prop, settings);

    std::string source_id = obs_source_get_id(source);
    emit qsignalStyleChanged(QString::fromStdString(source_id));
}
void AFQDowoomiDialog::qslotTypeChanged(int)
{
    if (!m_weakSource) {
        return;
    }

    int data = ui->comboBox_SubtitleForm->currentIndex();
    QString style = ui->comboBox_SubtitleForm->currentText();

    ui->label_Title->setText(QTStr("Dowoomi.Title").arg(m_sourceName, style));

    OBSSource source = GetSource(m_weakSource);
    obs_property_t* prop = obs_properties_get(m_props.get(), "type_setting");
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    std::string typeData = std::to_string(data);
    obs_data_set_string(settings, "type_setting", typeData.c_str());
    obs_source_update(source, settings);
    obs_property_modified(prop, settings);

    _RefreshStyleCombobox(true);
}
void AFQDowoomiDialog::qslotShutdownSourceNotVisibleChecked(bool checked)
{
    if(!m_weakSource) {
        return;
    }

    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_bool(settings, "shutdown", checked);
    obs_source_update(source, settings);
}

void AFQDowoomiDialog::qslotFreecshotControlAudio(bool checked)
{
    if (!m_weakSource) {
        return;
    }

    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_bool(settings, "reroute_audio", checked);
    obs_source_update(source, settings);
}

void AFQDowoomiDialog::qslotInteractionClicked()
{
    if(!m_weakSource) {
        return;
    }

    MAINFRAME->ShowBrowserInteractionPopup(GetSource(m_weakSource));
}

void AFQDowoomiDialog::qslotRefreshClicked()
{
    if(!m_weakSource) {
        return;
    }

    obs_property_t* prop = obs_properties_get(m_props.get(), "refreshnocache");
    obs_property_button_clicked(prop, GetSource(m_weakSource));
}
void AFQDowoomiDialog::qslotHelpPopupClicked()
{
    bool closed = true;
    if(m_dowoomiPopup)
        closed = m_dowoomiPopup->close();
    if(!closed)
        return;

    m_dowoomiPopup = new AFQDowoomiPopup(this, 0);
    m_dowoomiPopup->setAttribute(Qt::WA_DeleteOnClose, true);
    m_dowoomiPopup->setModal(false);

    setCenterPositionNotUseParent(m_dowoomiPopup, this);

    m_dowoomiPopup->show();
}

void AFQDowoomiDialog::qslotBrowserSizeClicked()
{
    int width = ui->spinBox_Width->value();
    int height = ui->spinBox_Height->value();

    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_int(settings, "width", width);
    obs_data_set_int(settings, "height", height);
    obs_source_update(source, settings);
}

void AFQDowoomiDialog::showEvent(QShowEvent* event)
{
    setFixedSize(720, 550);
}

void AFQDowoomiDialog::_CreateCefPopupProperties(obs_source_t* source)
{
    bool closed = true;
    if (m_cefPopupProperties)
        closed = m_cefPopupProperties->close();

    if (!closed)
        return;

    m_cefPopupProperties = new AFQCefPopupDialog(this, source);
    m_cefPopupProperties->setModal(false);

    m_cefPopupProperties->move(x() + this->width(), y());

    m_cefPopupProperties->show();
}

void AFQDowoomiDialog::_RefreshStyleCombobox(bool refreshNeed)
{
    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);

    QVariant data = obs_data_get_string(settings, "style_setting");
    QString dataurl = data.toString();

    obs_property_t* prop = obs_properties_get(m_props.get(), "style_setting");
    size_t count = obs_property_list_item_count(prop);
    size_t current_idx = 0;

    ui->combo_Style->blockSignals(true);
    ui->combo_Style->clear();

    for (size_t idx = 0; idx < count; idx++)
    {
        const char* name = obs_property_list_item_name(prop, idx);
        QVariant var = propertyListToQVariant(prop, idx);
        ui->combo_Style->addItem(QT_UTF8(name), var);
        QString varurl = var.toString();
        if (dataurl == varurl)
        {
            current_idx = idx;
        }
    }
    QString style_name = obs_property_list_item_name(prop, current_idx);
    ui->combo_Style->setCurrentIndex(current_idx);
    if(refreshNeed)
        qslotStyleChanged(current_idx);

    ui->combo_Style->blockSignals(false);


    QString title = QTStr("Dowoomi.Title").arg(m_sourceName, style_name);
#ifdef _WIN32
    ui->label_Title->setText(title);
#elif defined(__APPLE__)
    setWindowTitle(title);
#endif
}

void AFQDowoomiDialog::_SourceRemoved(void* data, calldata_t* params)
{
    QMetaObject::invokeMethod(static_cast<AFQDowoomiDialog*>(data),
        "close");
}