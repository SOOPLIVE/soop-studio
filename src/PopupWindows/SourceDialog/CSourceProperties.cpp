#include "CSourceProperties.h"
#include "ui_source-properties.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include <QPushButton>
#include <QSlider>
#include <qformlayout.h>

#include "qt-wrappers.hpp"
#include "display-helpers.hpp"

#include "Application/CApplication.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"
#include "Blocks/CBlockManager.h"

#include "Common/MathMiscUtils.h"

#include "CoreModel/Source/CSource.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "properties-view.hpp"

#include "UIComponent/CMessageBox.h"

#define COLOR_SOURCE_A  0xFFB26F52
#define COLOR_SOURCE_B  0xFF6FB252

#define CY_NOT_VIDEO_SOURCE 310

static obs_source_t* CreateLabel(const char* name, size_t h)
{
    OBSDataAutoRelease settings = obs_data_create();
    OBSDataAutoRelease font = obs_data_create();

    std::string text;
    text += "   ";
    text += name;
    text += " ";

#if defined(_WIN32)
    obs_data_set_string(font, "face", "Arial");
#elif defined(__APPLE__)
    obs_data_set_string(font, "face", "Helvetica");
#else
    obs_data_set_string(font, "face", "Monospace");
#endif
    obs_data_set_int(font, "flags", 1); // Bold text
    obs_data_set_int(font, "size", std::min(int(h), 300));

    obs_data_set_obj(settings, "font", font);
    obs_data_set_string(settings, "text", text.c_str());
    obs_data_set_bool(settings, "outline", false);
    //obs_data_set_int(settings, "opacity", 50);

#ifdef _WIN32
    const char* text_source_id = "text_gdiplus";
#else
    const char* text_source_id = "text_ft2_source";
#endif

    obs_source_t* txtSource = obs_source_create_private(text_source_id, name, settings);

    return txtSource;
}

static void CreateTransitionScene(OBSSource scene, const char* text, uint32_t color)
{
    OBSDataAutoRelease settings = obs_data_create();
    obs_data_set_int(settings, "width", obs_source_get_width(scene));
    obs_data_set_int(settings, "height", obs_source_get_height(scene));
    obs_data_set_int(settings, "color", color);

    OBSSourceAutoRelease colorBG = obs_source_create_private("color_source", "background", settings);

    obs_scene_add(obs_scene_from_source(scene), colorBG);

    OBSSourceAutoRelease label = CreateLabel(text, obs_source_get_height(scene));
    obs_sceneitem_t* item = obs_scene_add(obs_scene_from_source(scene), label);

    vec2 size;
    vec2_set(&size, obs_source_get_width(scene),
#ifdef _WIN32
        obs_source_get_height(scene));
#else
        obs_source_get_height(scene) * 0.8);
#endif

    obs_sceneitem_set_bounds(item, &size);
    obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
}

AFQSourceProperties::AFQSourceProperties(QWidget *parent, OBSSource source) :
    AFTTopBaseDialog(parent, Qt::WindowFlags()),
    ui(new Ui::AFQSourceProperties),
    m_obsSource(source),
    m_signalRemoved(obs_source_get_signal_handler(m_obsSource), "remove", AFQSourceProperties::_SourceRemoved, this),
    m_signalRenamed(obs_source_get_signal_handler(m_obsSource), "rename", AFQSourceProperties::_SourceRenamed, this),
    m_dataOldSetting(obs_data_create())
{
    int cy = (int)config_get_int(APPCONFIG, "PropertiesWindow", "cy");

    enum obs_source_type type = obs_source_get_type(m_obsSource);
    m_sourceType = type;

    ui->setupUi(this);

    repaint();
    QApplication::processEvents();

#ifdef __APPLE__
    ui->titleWidget->hide();
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
#endif

    SetWidthResizeEnabled(false);

    QPushButton* resetButton = ui->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    resetButton->setObjectName("resetButton");
    ChangeStyleSheet(resetButton, STYLESHEET_RESET_BUTTON); // Need qss separation

    ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();
    
    QSize size = this->size();
    if (cy > 400)
        resize(size.width(), cy);

    /* The OBSData constructor increments the reference once */
    obs_data_release(m_dataOldSetting);

    OBSDataAutoRelease nd_settings = obs_source_get_settings(m_obsSource);
    obs_data_apply(m_dataOldSetting, nd_settings);

    m_pViewProps = new OBSPropertiesView( nd_settings.Get(), m_obsSource,
                                         (PropertiesReloadCallback)obs_source_properties,
                                         (PropertiesUpdateCallback) nullptr, // No special handling required for undo/redo
                                         (PropertiesVisualUpdateCb)obs_source_update, 200);
    m_pViewProps->setMinimumHeight(150);

    ui->propertiesLayout->addWidget(m_pViewProps);

    if (type == OBS_SOURCE_TYPE_TRANSITION) {
        connect(m_pViewProps, &OBSPropertiesView::PropertiesRefreshed, this, &AFQSourceProperties::qslotAddPreviewButton);
    }

    m_pViewProps->show();
  
    installEventFilter(CreateShortcutFilter());

    const char* name = obs_source_get_name(m_obsSource);
    QString propsWindowName = Str("Basic.PropertiesWindow");
    propsWindowName = propsWindowName.arg(QT_UTF8(name));
    setWindowTitle(propsWindowName);
    ui->labelTitle->setText(propsWindowName);

    obs_source_inc_showing(m_obsSource);

    m_signalUpdateProperties.Connect(obs_source_get_signal_handler(m_obsSource), "update_properties",
                                     AFQSourceProperties::_UpdateProperties, this);

    /*auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(ui->preview->GetDisplay(), OBSBasicProperties::DrawPreview, this);
    };
    auto addTransitionDrawCallback = [this]() {
        obs_display_add_draw_callback(ui->preview->GetDisplay(), OBSBasicProperties::DrawTransitionPreview,
                          this);
    };*/
    uint32_t caps = obs_source_get_output_flags(m_obsSource);
    bool drawable_type = (type == OBS_SOURCE_TYPE_INPUT ||
                          type == OBS_SOURCE_TYPE_SCENE);
    bool drawable_preview = (caps & OBS_SOURCE_NOT_DRAW_PREVIEW) == 0 &&
                            (caps & OBS_SOURCE_VIDEO) != 0;

    if (drawable_preview && drawable_type) {
        ui->preview->show();
        connect(ui->preview, &AFQTDisplay::qsignalDisplayCreated, this, &AFQSourceProperties::qslotAddDrawCallback);
    }
    else if (type == OBS_SOURCE_TYPE_TRANSITION) {
        m_obsSourceA = obs_source_create_private("scene", "sourceA", nullptr);
        m_obsSourceB = obs_source_create_private("scene", "sourceB", nullptr);

        CreateTransitionScene(m_obsSourceA.Get(), "A", COLOR_SOURCE_A);
        CreateTransitionScene(m_obsSourceB.Get(), "B", COLOR_SOURCE_B);

        /**
         * The cloned source is made from scratch, rather than using
         * obs_source_duplicate, as the stinger transition would not
         * play correctly otherwise.
         */

        OBSDataAutoRelease settings = obs_source_get_settings(m_obsSource);

        m_obsSourceClone = obs_source_create_private(obs_source_get_id(m_obsSource), "clone", settings);

        obs_source_inc_active(m_obsSourceClone);
        obs_transition_set(m_obsSourceClone, m_obsSourceA);

        connect(m_pViewProps, &OBSPropertiesView::Changed, this, &AFQSourceProperties::qslotUpdatePropsCallback);

        ui->preview->show();
        connect(ui->preview, &AFQTDisplay::qsignalDisplayCreated, this, &AFQSourceProperties::qslotAddTransitionDrawCallback);
    }
    else {
        ui->preview->hide();
        ui->windowSplitter->setMinimumHeight(0);
        
        this->resize(size.width(), CY_NOT_VIDEO_SOURCE);
    }

    // ButtonBox UI Setting
    ui->closeButton->setObjectName("closeButton");
    auto* okBtn = ui->buttonBox->button(QDialogButtonBox::Ok);
    if (okBtn)
        okBtn->setObjectName("okButton");

    auto* cancelBtn = ui->buttonBox->button(QDialogButtonBox::Cancel);
    if (cancelBtn)
        cancelBtn->setObjectName("cancelButton");

    connect(ui->closeButton, &QPushButton::clicked, this, &AFQSourceProperties::qslotCloseButtonClicked);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &AFQSourceProperties::qslotButtonBoxClicked);
}

AFQSourceProperties::~AFQSourceProperties()
{
    if (m_obsSourceClone) {
        obs_source_dec_active(m_obsSourceClone);
    }
    obs_source_dec_showing(m_obsSource);

    delete ui;
}

static bool ConfirmReset(QWidget* parent)
{
    int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                            parent,
                                            Str("ConfirmRemove.Title"),
                                            Str("ConfirmReset.Text"));
    return (result == QDialog::Accepted);
}


void AFQSourceProperties::qslotButtonBoxClicked(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);
    if (val == QDialogButtonBox::AcceptRole) {
        std::string scene_uuid = obs_source_get_uuid(SCENE_CONTEXT.GetCurrentSceneSource());

        // OBSBasicProperties::on_buttonBox_clicked
        auto undo_redo = [scene_uuid](const std::string& data) {
            OBSDataAutoRelease settings = obs_data_create_from_json(data.c_str());
            OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(settings, "undo_uuid"));
            obs_source_reset_settings(source, settings);

            obs_source_update_properties(source);

            OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());

            DYNAMIC_COMPOSIT->SetCurrentScene(scene_source.Get(), true);
        };

        OBSDataAutoRelease new_settings = obs_data_create();
        OBSDataAutoRelease curr_settings = obs_source_get_settings(m_obsSource);
        obs_data_apply(new_settings, curr_settings);
        obs_data_set_string(new_settings, "undo_uuid", obs_source_get_uuid(m_obsSource));
        obs_data_set_string(m_dataOldSetting, "undo_uuid", obs_source_get_uuid(m_obsSource));

        std::string undo_data(obs_data_get_json(m_dataOldSetting));
        std::string redo_data(obs_data_get_json(new_settings));

        if (undo_data.compare(redo_data) != 0)
        {
            UNDO_STACK.AddAction(QTStr("Undo.Properties").arg(obs_source_get_name(m_obsSource)),
                                 undo_redo, undo_redo, undo_data, redo_data);
        }

        m_acceptClicked = true;
        close();

        if (m_pViewProps->DeferUpdate())
            m_pViewProps->UpdateSettings();

        auto now_settings = obs_source_get_settings(m_obsSource);

        std::string source_id = obs_source_get_id(m_obsSource);
        obs_data_set_string(now_settings, "source_id", source_id.c_str());

        MAINFRAME->UpdateContextToolBarDeferred();
    }
    else if (val == QDialogButtonBox::RejectRole) {
        qslotCloseButtonClicked();
    }
    else if (val == QDialogButtonBox::ResetRole) {
        if (!ConfirmReset(this))
            return;

        OBSDataAutoRelease settings = obs_source_get_settings(m_obsSource);
        obs_data_clear(settings);

        if (!m_pViewProps->DeferUpdate())
            obs_source_update(m_obsSource, nullptr);

        m_pViewProps->ReloadProperties();
    }
}

void AFQSourceProperties::qslotCloseButtonClicked()
{
    OBSDataAutoRelease settings = obs_source_get_settings(m_obsSource);
    obs_data_clear(settings);

    if (m_pViewProps->DeferUpdate())
        obs_data_apply(settings, m_dataOldSetting);
    else
        obs_source_update(m_obsSource, m_dataOldSetting);

    close();
}

void AFQSourceProperties::qslotAddPreviewButton()
{
    QPushButton* playButton = new QPushButton(Str("PreviewTransition"), this);

    playButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    playButton->setFixedHeight(40);
    playButton->setProperty("pushButtonTheme", "type4");

    VScrollArea* area = m_pViewProps;
    QFormLayout* formLayout = (QFormLayout*)area->widget()->layout();

    QLabel* label = new QLabel("");
    formLayout->addRow(label, playButton);

    connect(playButton, &QPushButton::clicked, this, &AFQSourceProperties::qslotStartTransPreview);
}

void AFQSourceProperties::qslotStartTransPreview()
{
	int duration = SCENE_CONTEXT.GetCurDuraition();

	OBSSource start;
	OBSSource end;

	if (m_direction) {
		start = m_obsSourceA;
		end = m_obsSourceB;
	}
	else {
		start = m_obsSourceB;
		end = m_obsSourceA;
	}

	obs_transition_set(m_obsSourceClone, start);
	obs_transition_start(m_obsSourceClone, OBS_TRANSITION_MODE_AUTO, duration, end);
	m_direction = !m_direction;

	start = nullptr;
	end = nullptr;
}

void AFQSourceProperties::qslotSetWindowTitle(QString title)
{
    ui->labelTitle->setText(title);
}

void AFQSourceProperties::qslotAddDrawCallback()
{
    obs_display_add_draw_callback(ui->preview->GetDisplay(),
                                  AFQSourceProperties::_DrawPreview, this);
}

void AFQSourceProperties::qslotAddTransitionDrawCallback()
{
    obs_display_add_draw_callback(
        ui->preview->GetDisplay(),
        AFQSourceProperties::_DrawTransitionPreview, this);
}

void AFQSourceProperties::qslotUpdatePropsCallback()
{
    OBSDataAutoRelease settings =
        obs_source_get_settings(m_obsSource);
    obs_source_update(m_obsSourceClone, settings);

    obs_transition_clear(m_obsSourceClone);
    obs_transition_set(m_obsSourceClone, m_obsSourceA);
    obs_transition_force_stop(m_obsSourceClone);

    m_direction = true;
}

void AFQSourceProperties::_SourceRemoved(void* data, calldata_t* params)
{
    QMetaObject::invokeMethod(static_cast<AFQSourceProperties*>(data), "close");
}

void AFQSourceProperties::_SourceRenamed(void* data, calldata_t* params)
{
    const char* name = calldata_string(params, "new_name");

    QString propsWindowName = Str("Basic.PropertiesWindow");
    propsWindowName = propsWindowName.arg(QT_UTF8(name));

    QMetaObject::invokeMethod(static_cast<AFQSourceProperties*>(data),
                              "qslotSetWindowTitle", Q_ARG(QString, propsWindowName));
}

void AFQSourceProperties::_UpdateProperties(void* data, calldata_t*)
{
    QMetaObject::invokeMethod(static_cast<AFQSourceProperties*>(data)->m_pViewProps, "ReloadProperties");
}

void AFQSourceProperties::_DrawPreview(void* data, uint32_t cx, uint32_t cy)
{
    AFQSourceProperties* window = static_cast<AFQSourceProperties*>(data);

    if (!window->m_obsSource)
        return;

    uint32_t sourceCX = std::max(obs_source_get_width(window->m_obsSource), 1u);
    uint32_t sourceCY = std::max(obs_source_get_height(window->m_obsSource), 1u);

    int x, y;
    int newCX, newCY;
    float scale;

    GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

    newCX = int(scale * float(sourceCX));
    newCY = int(scale * float(sourceCY));

    gs_viewport_push();
    gs_projection_push();
    const bool previous = gs_set_linear_srgb(true);

    gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
    gs_set_viewport(x, y, newCX, newCY);
    obs_source_video_render(window->m_obsSource);

    gs_set_linear_srgb(previous);
    gs_projection_pop();
    gs_viewport_pop();
}

void AFQSourceProperties::_DrawTransitionPreview(void* data, uint32_t cx, uint32_t cy)
{
    AFQSourceProperties* window = static_cast<AFQSourceProperties*>(data);

    if (!window->m_obsSourceClone)
        return;

    uint32_t sourceCX = std::max(obs_source_get_width(window->m_obsSourceClone), 1u);
    uint32_t sourceCY = std::max(obs_source_get_height(window->m_obsSourceClone), 1u);

    int x, y;
    int newCX, newCY;
    float scale;

    GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

    newCX = int(scale * float(sourceCX));
    newCY = int(scale * float(sourceCY));

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
    gs_set_viewport(x, y, newCX, newCY);

    obs_source_video_render(window->m_obsSourceClone);

    gs_projection_pop();
    gs_viewport_pop();
}

void AFQSourceProperties::_Cleanup()
{
    config_set_int(APPCONFIG, "PropertiesWindow", "cy", height());

    obs_display_remove_draw_callback(ui->preview->GetDisplay(), AFQSourceProperties::_DrawPreview, this);
    obs_display_remove_draw_callback(ui->preview->GetDisplay(), AFQSourceProperties::_DrawTransitionPreview, this);
}

void AFQSourceProperties::CloseSourcePropertise()
{
    qslotCloseButtonClicked();
}

bool AFQSourceProperties::nativeEvent(const QByteArray& eventType, void* message,
                                      qintptr* result)
{
#ifdef _WIN32
    const MSG& msg = *static_cast<MSG*>(message);
    switch (msg.message) {
    case WM_MOVE:
        for (AFQTDisplay* const display : findChildren<AFQTDisplay*>()) {
                display->OnMove();
        }
        break;
    case WM_DISPLAYCHANGE:
        for (AFQTDisplay* const display : findChildren<AFQTDisplay*>()) {
                display->OnDisplayChange();
        }
    }
#else
    UNUSED_PARAMETER(message);
#endif

    return AFTTopBaseDialog::nativeEvent(eventType, message, result);
}

void AFQSourceProperties::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);

    _Cleanup();
}

void AFQSourceProperties::reject()
{
    _Cleanup();
    done(0);
}

void AFQSourceProperties::showEvent(QShowEvent* event)
{
    if(MAINFRAME->IsSmallResolution())
        resize(720, 550);
}
