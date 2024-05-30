#include "CSourceProperties.h"
#include "ui_source-properties.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif


#include <QPushButton>
#include <QSlider>

#include "qt-wrapper.h"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#include "CPropertiesView.h"

#include "CoreModel/Source/CSource.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Config/CConfigManager.h"

#include "UIComponent/CMessageBox.h"

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

#ifdef _WIN32
    const char* text_source_id = "text_gdiplus";
#else
    const char* text_source_id = "text_ft2_source";
#endif

    obs_source_t* txtSource =
        obs_source_create_private(text_source_id, name, settings);

    return txtSource;
}


static void CreateTransitionScene(OBSSource scene, const char* text,
    uint32_t color)
{
    OBSDataAutoRelease settings = obs_data_create();
    obs_data_set_int(settings, "width", obs_source_get_width(scene));
    obs_data_set_int(settings, "height", obs_source_get_height(scene));
    obs_data_set_int(settings, "color", color);

    OBSSourceAutoRelease colorBG = obs_source_create_private(
        "color_source", "background", settings);

    obs_scene_add(obs_scene_from_source(scene), colorBG);

    OBSSourceAutoRelease label =
        CreateLabel(text, obs_source_get_height(scene));
    obs_sceneitem_t* item =
        obs_scene_add(obs_scene_from_source(scene), label);

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

AFQSourceProperties::AFQSourceProperties(QWidget *parent, OBSSource source_) :
    AFQRoundedDialogBase((QDialog*)parent, Qt::WindowFlags(), false),
    //QDialog(parent),
    ui(new Ui::AFQSourceProperties),
    source(source_),
    m_signalRemoved(obs_source_get_signal_handler(source), "remove",
                    AFQSourceProperties::SourceRemoved, this),
    m_signalRenamed(obs_source_get_signal_handler(source), "rename",
                    AFQSourceProperties::SourceRenamed, this),
    m_signalUpdateProperties(obs_source_get_signal_handler(source), "update_properties",
                    AFQSourceProperties::UpdateProperties, this),
    oldSettings(obs_data_create())
{
    AFConfigManager& config = AFConfigManager::GetSingletonInstance();
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

    int cy = (int)config_get_int(config.GetGlobal(), "PropertiesWindow", "cy");

    enum obs_source_type type = obs_source_get_type(source);

    ui->setupUi(this);

    SetWidthFixed(true);

    QPushButton* resetButton = ui->buttonBox->button(QDialogButtonBox::RestoreDefaults);
    ChangeStyleSheet(resetButton, STYLESHEET_RESET_BUTTON);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();
    
    QSize size = this->size();
    if (cy > 400)
        resize(size.width(), cy);

    /* The OBSData constructor increments the reference once */
    obs_data_release(oldSettings);

    OBSDataAutoRelease nd_settings = obs_source_get_settings(source);
    obs_data_apply(oldSettings, nd_settings);

    view = new AFQPropertiesView(
                nd_settings.Get(), source,
                (PropertiesReloadCallback)obs_source_properties,
                (PropertiesUpdateCallback) nullptr, // No special handling required for undo/redo
                (PropertiesVisualUpdateCb)obs_source_update, 200);
    view->setMinimumHeight(150);

    ui->propertiesLayout->addWidget(view);

    if (type == OBS_SOURCE_TYPE_TRANSITION) {
        connect(view, &AFQPropertiesView::PropertiesRefreshed, this,
                &AFQSourceProperties::qSlotAddPreviewButton);
    }

    view->show();
  
    installEventFilter(CreateShortcutFilter());

    const char* name = obs_source_get_name(source);
    QString propsWindowName = locale.Str("Basic.PropertiesWindow");
    propsWindowName = propsWindowName.arg(QT_UTF8(name));
    setWindowTitle(propsWindowName);

    ui->labelTitle->setText(propsWindowName);

    auto addDrawCallback = [this]() {
        obs_display_add_draw_callback(ui->preview->GetDisplay(),
            AFQSourceProperties::DrawPreview,
            this);
    };

    auto addTransitionDrawCallback = [this]() {
        obs_display_add_draw_callback(
            ui->preview->GetDisplay(),
            AFQSourceProperties::DrawTransitionPreview, this);
    };

    uint32_t caps = obs_source_get_output_flags(source);
    bool drawable_type = type == OBS_SOURCE_TYPE_INPUT ||
        type == OBS_SOURCE_TYPE_SCENE;
    bool drawable_preview = (caps & OBS_SOURCE_VIDEO) != 0;

    if (drawable_preview && drawable_type) {
        ui->preview->show();
        connect(ui->preview, &AFQTDisplay::qsignalDisplayCreated,
            addDrawCallback);
    }
    else if (type == OBS_SOURCE_TYPE_TRANSITION) {

        sourceA =
            obs_source_create_private("scene", "sourceA", nullptr);
        sourceB =
            obs_source_create_private("scene", "sourceB", nullptr);

        uint32_t colorA = 0xFFB26F52;
        uint32_t colorB = 0xFF6FB252;

        CreateTransitionScene(sourceA.Get(), "A", colorA);
        CreateTransitionScene(sourceB.Get(), "B", colorB);

        OBSDataAutoRelease settings = obs_source_get_settings(source);

        sourceClone = obs_source_create_private(
            obs_source_get_id(source), "clone", settings);

        obs_source_inc_active(sourceClone);
        obs_transition_set(sourceClone, sourceA);

        auto updateCallback = [=]() {
            OBSDataAutoRelease settings =
                obs_source_get_settings(source);
            obs_source_update(sourceClone, settings);

            obs_transition_clear(sourceClone);
            obs_transition_set(sourceClone, sourceA);
            obs_transition_force_stop(sourceClone);

            m_direction = true;
        };

        connect(view, &AFQPropertiesView::Changed, updateCallback);

        ui->preview->show();
        connect(ui->preview, &AFQTDisplay::qsignalDisplayCreated,
                addTransitionDrawCallback);

    }
    else {
        ui->preview->hide();
        ui->windowSplitter->setMinimumHeight(0);
        
        this->resize(size.width(),310);
    }

    // UI Setting
    connect(ui->closeButton, &QPushButton::clicked, this, &AFQSourceProperties::qSlotCloseButtonClicked);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &AFQSourceProperties::qSlotButtonBoxClicked);

    m_buttonOK = ui->buttonBox->button(QDialogButtonBox::Ok);
    m_buttonCancel = ui->buttonBox->button(QDialogButtonBox::Cancel);
    m_buttonRestore = ui->buttonBox->button(QDialogButtonBox::RestoreDefaults);
}

AFQSourceProperties::~AFQSourceProperties()
{
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


void AFQSourceProperties::qSlotButtonBoxClicked(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);
    if (val == QDialogButtonBox::AcceptRole) {
        std::string scene_uuid = obs_source_get_uuid(AFSourceUtil::GetCurrentSource());

        // OBSBasicProperties::on_buttonBox_clicked
        auto undo_redo = [scene_uuid](const std::string& data) {
            OBSDataAutoRelease settings = obs_data_create_from_json(data.c_str());
            OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(settings, "undo_uuid"));
            obs_source_reset_settings(source, settings);

            obs_source_update_properties(source);

            OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());

            AFMainFrame* main = App()->GetMainView();
            main->GetMainWindow()->SetCurrentScene(scene_source.Get(), true);
        };

        OBSDataAutoRelease new_settings = obs_data_create();
        OBSDataAutoRelease curr_settings = obs_source_get_settings(source);
        obs_data_apply(new_settings, curr_settings);
        obs_data_set_string(new_settings, "undo_uuid", obs_source_get_uuid(source));
        obs_data_set_string(oldSettings, "undo_uuid", obs_source_get_uuid(source));

        std::string undo_data(obs_data_get_json(oldSettings));
        std::string redo_data(obs_data_get_json(new_settings));

        AFMainFrame* main = App()->GetMainView();
        if (undo_data.compare(redo_data) != 0)
        {
            main->m_undo_s.AddAction(QTStr("Undo.Properties").arg(obs_source_get_name(source)),
                                     undo_redo, undo_redo, undo_data, redo_data);
        }

        m_acceptClicked = true;
        close();

        if (view->DeferUpdate())
            view->UpdateSettings();

        if (AFSourceUtil::ShouldShowContextPopup(source))
            main->GetMainWindow()->ShowOrUpdateContextPopup(source);
    }
    else if (val == QDialogButtonBox::RejectRole) {
        qSlotCloseButtonClicked();
    }
    else if (val == QDialogButtonBox::ResetRole) {
        if (!ConfirmReset(this))
            return;

        OBSDataAutoRelease settings = obs_source_get_settings(source);
        obs_data_clear(settings);

        if (!view->DeferUpdate())
            obs_source_update(source, nullptr);

        view->ReloadProperties();
    }
}


void AFQSourceProperties::qSlotCloseButtonClicked()
{
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_clear(settings);

    if (view->DeferUpdate())
        obs_data_apply(settings, oldSettings);
    else
        obs_source_update(source, oldSettings);

    close();
}

void AFQSourceProperties::qSlotAddPreviewButton()
{
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

    QPushButton* playButton =
        new QPushButton(locale.Str("PreviewTransition"), view);

    playButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    playButton->setFixedHeight(40);

    VScrollArea* area = view;
    QFormLayout* formLayout = (QFormLayout*)area->widget()->layout();
    formLayout->addRow(playButton);

    connect(playButton, &QPushButton::clicked, this, &AFQSourceProperties::qSlotStartTransPreview);
}


void AFQSourceProperties::qSlotStartTransPreview()
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
	int duration = sceneContext.GetCurDuraition();

	OBSSource start;
	OBSSource end;

	if (m_direction) {
		start = sourceA;
		end = sourceB;
	}
	else {
		start = sourceB;
		end = sourceA;
	}

	obs_transition_set(sourceClone, start);
	obs_transition_start(sourceClone, OBS_TRANSITION_MODE_AUTO,
		duration, end);
	m_direction = !m_direction;

	start = nullptr;
	end = nullptr;
}

void AFQSourceProperties::qslotSetWindowTitle(QString title)
{
    ui->labelTitle->setText(title);
}

bool AFQSourceProperties::nativeEvent(const QByteArray& eventType, void* message,
                                      qintptr* result)
{
#ifdef _WIN32
    const MSG& msg = *static_cast<MSG*>(message);
    switch (msg.message) {
    case WM_MOVE:
        for (AFQTDisplay* const display :
            findChildren<AFQTDisplay*>()) {
                display->OnMove();
        }
        break;
    case WM_DISPLAYCHANGE:
        for (AFQTDisplay* const display :
            findChildren<AFQTDisplay*>()) {
                display->OnDisplayChange();
        }
    }
#else
    UNUSED_PARAMETER(message);
#endif

    return false;
}

void AFQSourceProperties::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);

    Cleanup();
}

void AFQSourceProperties::reject()
{
    Cleanup();
    done(0);
}

void AFQSourceProperties::SourceRemoved(void* data, calldata_t* params)
{
    QMetaObject::invokeMethod(static_cast<AFQSourceProperties*>(data),
                              "close");
}

void AFQSourceProperties::SourceRenamed(void* data, calldata_t* params)
{
    AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

    const char* name = calldata_string(params, "new_name");

    QString propsWindowName = locale.Str("Basic.PropertiesWindow");
    propsWindowName = propsWindowName.arg(QT_UTF8(name));

    QMetaObject::invokeMethod(static_cast<AFQSourceProperties*>(data),
                              "qslotSetWindowTitle", Q_ARG(QString, propsWindowName));
}

void AFQSourceProperties::UpdateProperties(void* data, calldata_t*)
{
    QMetaObject::invokeMethod(static_cast<AFQSourceProperties*>(data)->view,
        "ReloadProperties");
}

// display-helper.hpp line23
static inline void GetScaleAndCenterPos_Temp(int baseCX, int baseCY, int windowCX,
    int windowCY, int& x, int& y,
    float& scale)
{
    double windowAspect, baseAspect;
    int newCX, newCY;

    windowAspect = double(windowCX) / double(windowCY);
    baseAspect = double(baseCX) / double(baseCY);

    if (windowAspect > baseAspect) {
        scale = float(windowCY) / float(baseCY);
        newCX = int(double(windowCY) * baseAspect);
        newCY = windowCY;
    }
    else {
        scale = float(windowCX) / float(baseCX);
        newCX = windowCX;
        newCY = int(float(windowCX) / baseAspect);
    }

    x = windowCX / 2 - newCX / 2;
    y = windowCY / 2 - newCY / 2;
}

void AFQSourceProperties::DrawPreview(void* data, uint32_t cx, uint32_t cy)
{
    AFQSourceProperties* window = static_cast<AFQSourceProperties*>(data);

    if (!window->source)
        return;

    uint32_t sourceCX = std::max(obs_source_get_width(window->source), 1u);
    uint32_t sourceCY = std::max(obs_source_get_height(window->source), 1u);

    int x, y;
    int newCX, newCY;
    float scale;

    GetScaleAndCenterPos_Temp(sourceCX, sourceCY, cx, cy, x, y, scale);

    newCX = int(scale * float(sourceCX));
    newCY = int(scale * float(sourceCY));

    gs_viewport_push();
    gs_projection_push();
    const bool previous = gs_set_linear_srgb(true);

    gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
    gs_set_viewport(x, y, newCX, newCY);
    obs_source_video_render(window->source);

    gs_set_linear_srgb(previous);
    gs_projection_pop();
    gs_viewport_pop();
}


void AFQSourceProperties::DrawTransitionPreview(void* data, uint32_t cx, uint32_t cy)
{
    AFQSourceProperties* window = static_cast<AFQSourceProperties*>(data);

    if (!window->sourceClone)
        return;

    uint32_t sourceCX = std::max(obs_source_get_width(window->sourceClone), 1u);
    uint32_t sourceCY = std::max(obs_source_get_height(window->sourceClone), 1u);

    int x, y;
    int newCX, newCY;
    float scale;

    GetScaleAndCenterPos_Temp(sourceCX, sourceCY, cx, cy, x, y, scale);

    newCX = int(scale * float(sourceCX));
    newCY = int(scale * float(sourceCY));

    gs_viewport_push();
    gs_projection_push();
    gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
    gs_set_viewport(x, y, newCX, newCY);

    obs_source_video_render(window->sourceClone);

    gs_projection_pop();
    gs_viewport_pop();
}

void AFQSourceProperties::Cleanup()
{
    AFConfigManager& config = AFConfigManager::GetSingletonInstance();

    config_set_int(config.GetGlobal(), "PropertiesWindow", "cy", height());

    obs_display_remove_draw_callback(ui->preview->GetDisplay(),
                                     AFQSourceProperties::DrawPreview, this);
    obs_display_remove_draw_callback(
            ui->preview->GetDisplay(),
            AFQSourceProperties::DrawTransitionPreview, this);
}
