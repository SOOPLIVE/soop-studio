#include "CDowoomiMoodCheckDialog.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Source/CSource.h"

//
static OBSSource GetSource(OBSWeakSource weakSource) {
    return OBSGetStrongRef(weakSource);
}
AFQDowoomiMoodCheckDialog::AFQDowoomiMoodCheckDialog(QWidget* parent, OBSSource source) :
    AFTTopBaseDialog(parent, Qt::WindowFlags()),
    ui(new Ui::AFQDowoomiMoodCheckDialog),
    m_weakSource(OBSGetWeakRef(source)),
    m_props(obs_source_properties(source), obs_properties_destroy),
    removeSignal(obs_source_get_signal_handler(source), "remove",
        AFQDowoomiMoodCheckDialog::_SourceRemoved, this)
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

    //bool reroute_audio = obs_data_get_bool(settings, "reroute_audio");
    //ui->checkBox_RerouteAudio->setChecked(reroute_audio);
   
    _GetThemeInfo();

    std::string source_id = obs_source_get_id(source);
    m_sourceName = obs_source_get_display_name(source_id.c_str());

    int width = obs_data_get_int(settings, "width");
    int height = obs_data_get_int(settings, "height");
    ui->spinBox_Width->setValue(width);
    ui->spinBox_Height->setValue(height);

    QString title = QTStr("Dowoomi.MoodCheck.Title");//m_sourceName;
    QString settingPopup = QTStr("Dowoomi.SettingPopup").arg(m_sourceName);

#ifdef _WIN32
    ui->label_Title->setText(title);
#elif defined(__APPLE__)
    setWindowTitle(title);
#endif


    // connect action
    connect(ui->buttonClose, &QPushButton::clicked,
        this, &AFQDowoomiMoodCheckDialog::close);
    
    connect(ui->checkbox_ShutdownSourceNotVisible, &QCheckBox::clicked, this, &AFQDowoomiMoodCheckDialog::qslotShutdownSourceNotVisibleChecked);
    //connect(ui->checkBox_RerouteAudio, &QCheckBox::clicked, this, &AFQDowoomiMoodCheckDialog::qslotFreecshotControlAudio);

    connect(ui->widget_basic_theme, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiMoodCheckDialog::qslotSetThemeClicked);
    connect(ui->widget_talk_balloon_theme, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiMoodCheckDialog::qslotSetThemeClicked);
    connect(ui->widget_expression_theme, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiMoodCheckDialog::qslotSetThemeClicked);
    connect(ui->widget_sticky_notes_theme, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiMoodCheckDialog::qslotSetThemeClicked);
    connect(ui->widget_simple_theme, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiMoodCheckDialog::qslotSetThemeClicked);
    connect(ui->widget_pixel_theme, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiMoodCheckDialog::qslotSetThemeClicked); 
    connect(ui->widget_award_led_theme, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiMoodCheckDialog::qslotSetThemeClicked);
    connect(ui->widget_award_led_blue_theme, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiMoodCheckDialog::qslotSetThemeClicked);
    connect(ui->widget_award_led_pink_theme, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiMoodCheckDialog::qslotSetThemeClicked);
    connect(ui->widget_award_led_gold_theme, &AFQHoverWidget::qsignalMouseClick, this, &AFQDowoomiMoodCheckDialog::qslotSetThemeClicked);

    connect(ui->button_Interactive, &QPushButton::clicked, this, &AFQDowoomiMoodCheckDialog::qslotInteractionClicked);
    connect(ui->button_Refresh, &QPushButton::clicked, this, &AFQDowoomiMoodCheckDialog::qslotRefreshClicked);

    connect(ui->pushButton_Size, &QPushButton::clicked, this, &AFQDowoomiMoodCheckDialog::qslotBrowserSizeClicked);
}
AFQDowoomiMoodCheckDialog::~AFQDowoomiMoodCheckDialog()
{
    m_weakSource = nullptr;
    delete ui;
}
//
void AFQDowoomiMoodCheckDialog::qslotShutdownSourceNotVisibleChecked(bool checked)
{
    if(!m_weakSource) {
        return;
    }

    OBSSource source = OBSGetStrongRef(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_bool(settings, "shutdown", checked);
    obs_source_update(source, settings);
}

void AFQDowoomiMoodCheckDialog::qslotFreecshotControlAudio(bool checked)
{
    if (!m_weakSource) {
        return;
    }

    OBSSource source = OBSGetStrongRef(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_bool(settings, "reroute_audio", checked);
    obs_source_update(source, settings);
}

void AFQDowoomiMoodCheckDialog::qslotSetThemeClicked()
{
    OBSSource source = OBSGetStrongRef(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(GetSource(m_weakSource));
    int old_theme = obs_data_get_int(settings, "mood_check_idx");
    auto theme_info = tMOOD_CHECK_THEME_INFO();

    AFQHoverWidget* widget = nullptr;
    switch (old_theme) {
    case basic_theme:
        widget = ui->widget_basic_theme;            
        break;

    case sticker_notes_theme:
        widget = ui->widget_sticky_notes_theme;
        break;

    case talk_balloon_theme:
        widget = ui->widget_talk_balloon_theme;
        break;

    case simple_theme:
        widget = ui->widget_simple_theme;
        break;

    case expression_theme:
        widget = ui->widget_expression_theme;
        break;

    case pixel_theme:
        widget = ui->widget_pixel_theme;
        break;

    case award_led_theme:
        widget = ui->widget_award_led_theme;
        break;

    case award_led_blue_theme:
        widget = ui->widget_award_led_blue_theme;
        break;

    case award_led_pink_theme:
        widget = ui->widget_award_led_pink_theme;
        break;

    case award_led_gold_theme:
        widget = ui->widget_award_led_gold_theme;
        break;

    default:        // basic_theme
        widget = ui->widget_basic_theme;
        break;
    }
    widget->setProperty("IsSelected", false);
    PolishStyleSheet(widget);

    // new theme fill outline
    widget = reinterpret_cast<AFQHoverWidget*>(sender());
    int index = 1;
    std::string new_theme = "";
    if (ui->widget_basic_theme == widget) {
        theme_info = tMOOD_CHECK_THEME_INFO(basic_theme, "default", "default");
    }
    else if (ui->widget_sticky_notes_theme == widget) {
        theme_info = tMOOD_CHECK_THEME_INFO(sticker_notes_theme, "postit", "post_it");
    }
    else if (ui->widget_talk_balloon_theme == widget) {
        theme_info = tMOOD_CHECK_THEME_INFO(talk_balloon_theme, "balloon", "speech_bubble");
    }
    else if (ui->widget_simple_theme == widget) {
        theme_info = tMOOD_CHECK_THEME_INFO(simple_theme, "simple", "simple");
    }
    else if (ui->widget_expression_theme == widget) {
        theme_info = tMOOD_CHECK_THEME_INFO(expression_theme, "expression", "emotion");
    }
    else if (ui->widget_pixel_theme == widget) {
        theme_info = tMOOD_CHECK_THEME_INFO(pixel_theme, "pixel", "pixel");
    }
    else if (ui->widget_award_led_theme == widget) {
        theme_info = tMOOD_CHECK_THEME_INFO(award_led_theme, "award/purple", "led");
    }
    else if (ui->widget_award_led_blue_theme == widget) {
        theme_info = tMOOD_CHECK_THEME_INFO(award_led_blue_theme, "award/blue", "led_blue");
    }
    else if (ui->widget_award_led_pink_theme == widget) {
        theme_info = tMOOD_CHECK_THEME_INFO(award_led_pink_theme, "award/pink", "led_pink");
    }
    else if (ui->widget_award_led_gold_theme == widget) {
        theme_info = tMOOD_CHECK_THEME_INFO(award_led_gold_theme, "award/yellow", "led_gold");
    }
    else {
        theme_info = tMOOD_CHECK_THEME_INFO(basic_theme, "default", "default");
    }

    AUTH_CONTEXT.SetMinsimCheckThemeInfo(theme_info.theme_log_type);
        
    widget->setProperty("IsSelected", true);
    PolishStyleSheet(widget);

    obs_data_set_int(settings, "mood_check_idx", theme_info.theme_index);

    // update
    obs_data_set_string(settings, "theme", theme_info.theme_type.c_str());


    obs_source_update(source, settings);

    AFQSceneListItem* clickedItem = SCENE_CONTEXT.GetCurSelectedSceneItem();

    obs_source_t* src = obs_scene_get_source(clickedItem->GetScene());

    MAINFRAME->SetCurrentScene(src);

    SceneItemVector& sceneItemVector = SCENE_CONTEXT.GetSceneItemVector();

    struct FindMinsimChk { bool found = false; int nCnt = 0; } findMinsimChk;
    FindMinsimChk findInfo;

    obs_scene_enum_items(clickedItem->GetScene(), [](obs_scene_t*, obs_sceneitem_t* item, void* param)->bool {
        auto* f = static_cast<FindMinsimChk*>(param);
        auto src = obs_sceneitem_get_source(item);
        const char* id = obs_source_get_id(src);
        if (0 == strcmp(id, "soop_chat_source_mood_check")) {
            bool bVisible = obs_source_showing(src);
            if (bVisible) {
                f->found = true;
                f->nCnt += 1;
            }
        }
        return true;
    }, &findInfo);
}

void AFQDowoomiMoodCheckDialog::qslotInteractionClicked()
{
    if(!m_weakSource) {
        return;
    }

    MAINFRAME->ShowBrowserInteractionPopup(OBSGetStrongRef(m_weakSource));
}
void AFQDowoomiMoodCheckDialog::qslotRefreshClicked()
{
    if(!m_weakSource) {
        return;
    }

    obs_property_t* prop = obs_properties_get(m_props.get(), "refreshnocache");
    obs_property_button_clicked(prop, OBSGetStrongRef(m_weakSource));
}

void AFQDowoomiMoodCheckDialog::qslotBrowserSizeClicked()
{
    int width = ui->spinBox_Width->value();
    int height = ui->spinBox_Height->value();

    OBSSource source = OBSGetStrongRef(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);

    obs_scene_t* scene = SCENE_CONTEXT.GetCurrentScene();
    OBSSceneItemAutoRelease item = obs_scene_sceneitem_from_source(scene, source);
    obs_sceneitem_defer_update_begin(item);
    obs_transform_info info;
    obs_sceneitem_get_info(item, &info);
    vec2_set(&info.bounds, width, height);
    info.bounds_type = OBS_BOUNDS_STRETCH;
    obs_sceneitem_set_info(item, &info);
    obs_sceneitem_defer_update_end(item);

    obs_data_set_int(settings, "width", width);
    obs_data_set_int(settings, "height", height);

    obs_source_update(source, settings);
}

void AFQDowoomiMoodCheckDialog::_GetThemeInfo()
{
    if (!m_weakSource) {
        return;
    }

    // get settings
    auto settings = obs_source_get_settings(GetSource(m_weakSource));

    int theme_index = obs_data_get_int(settings, "mood_check_idx");
    AFQHoverWidget* widget = nullptr;
    switch (theme_index) {
    case basic_theme:
        widget = ui->widget_basic_theme;        
        break;

    case sticker_notes_theme:
        widget = ui->widget_sticky_notes_theme;
        break;

    case talk_balloon_theme:
        widget = ui->widget_talk_balloon_theme;
        break;

    case simple_theme:
        widget = ui->widget_simple_theme;
        break;

    case expression_theme:
        widget = ui->widget_expression_theme;
        break;

    case pixel_theme:
        widget = ui->widget_pixel_theme;
        break;

    case award_led_theme:
        widget = ui->widget_award_led_theme;
        break;

    case award_led_blue_theme:
        widget = ui->widget_award_led_blue_theme;
        break;

    case award_led_pink_theme:
        widget = ui->widget_award_led_pink_theme;
        break;

    case award_led_gold_theme:
        widget = ui->widget_award_led_gold_theme;
        break;

    default:        // basic_theme
        widget = ui->widget_basic_theme;
        break;
    }

    widget->setProperty("IsSelected", true);
    PolishStyleSheet(widget);
}

void AFQDowoomiMoodCheckDialog::_UpdateScoreInfo()
{
    if (!m_weakSource) {
        return;
    }
}

void AFQDowoomiMoodCheckDialog::_SourceRemoved(void* data, calldata_t* params)
{
    QMetaObject::invokeMethod(static_cast<AFQDowoomiMoodCheckDialog*>(data),
        "close");
}