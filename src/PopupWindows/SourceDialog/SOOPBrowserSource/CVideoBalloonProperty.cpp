#include "CVideoBalloonProperty.h"
#include "ui_videoballoon-source.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "Common/StudioDefine.h"
#include "UIComponent/CCustomCombobox.h"

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
AFQVideoBalloonProps::AFQVideoBalloonProps(QWidget* parent, OBSSource source) :
    AFTTopBaseDialog(parent, Qt::WindowFlags()),
    ui(new Ui::AFQVideoBalloonProps),
    m_weakSource(OBSGetWeakRef(source)),
    m_props(obs_source_properties(source), obs_properties_destroy),
    removeSignal(obs_source_get_signal_handler(source), "remove",
        AFQVideoBalloonProps::_SourceRemoved, this)
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

#ifdef _WIN32
#elif defined(__APPLE__)
    setWindowTitle(QTStr("VideoBalloon"));
#endif

    ui->button_DetailStyle->setStyleSheet("background-color: transparent; color: rgb(0, 163, 255); border: 1px solid rgb(0, 163, 255);");
    PolishStyleSheet(ui->button_DetailStyle);
    
    OBSDataAutoRelease settings = obs_source_get_settings(source);

    bool reroute_audio = obs_data_get_bool(settings, "reroute_audio");
    ui->checkBox_RerouteAudio->setChecked(reroute_audio);

    // connect action
    connect(ui->buttonClose, &QPushButton::clicked,
        this, &AFQVideoBalloonProps::close);
    
    connect(ui->checkBox_RerouteAudio, &QPushButton::clicked, 
        this, &AFQVideoBalloonProps::qslotFreecShotControlAudioClicked);

    connect(ui->button_DetailStyle, &QPushButton::clicked, this, &AFQVideoBalloonProps::qslotDetailSettingClicked);
    connect(ui->button_Refresh, &QPushButton::clicked, this, &AFQVideoBalloonProps::qslotRefreshClicked);

}
AFQVideoBalloonProps::~AFQVideoBalloonProps()
{
    m_weakSource = nullptr;
    delete ui;
}

void AFQVideoBalloonProps::qslotDetailSettingClicked()
{

}

void AFQVideoBalloonProps::qslotFreecShotControlAudioClicked(bool checked)
{
    if (!m_weakSource) {
        return;
    }

    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_bool(settings, "reroute_audio", checked);
    obs_source_update(source, settings);
}

void AFQVideoBalloonProps::qslotRefreshClicked()
{
    if(!m_weakSource) {
        return;
    }

    obs_property_t* prop = obs_properties_get(m_props.get(), "refreshnocache");
    obs_property_button_clicked(prop, GetSource(m_weakSource));
}

void AFQVideoBalloonProps::_SourceRemoved(void* data, calldata_t* params)
{
    QMetaObject::invokeMethod(static_cast<AFQVodSourceDialog*>(data),
        "close");
}