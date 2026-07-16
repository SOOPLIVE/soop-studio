#include "CParticleEffectDialog.h"
#include "ui_particle-effect-dialog.h"

#include "qt-wrappers.hpp"

#include "Blocks/CBlockManager.h"

#include "MainFrame/CMainFrame.h"

#define SETTING_PARTICLE_EFFECT_ID "effect_id"

static OBSSource GetSource(OBSWeakSource weakSource) {
    return OBSGetStrongRef(weakSource);
}

AFQParticleEffectDialog::AFQParticleEffectDialog(QWidget* parent, OBSSource _source) 
    : AFTTopBaseDialog(parent, Qt::WindowFlags()),
    ui(new Ui::AFQParticleEffectDialog),
    m_weakSource(OBSGetWeakRef(_source)),
    m_props(obs_source_properties(_source), obs_properties_destroy),
    removeSignal(obs_source_get_signal_handler(_source), "remove",
        AFQParticleEffectDialog::_SourceRemoved, this)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

#ifdef _WIN32
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    ui->titleFrame->hide();
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    setWindowTitle(QTStr("Effect.Particle.Props.Caption"));
#endif
    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);

    _CreateParticleButtons();

    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);

    int width = obs_data_get_int(settings, "width");
    int height = obs_data_get_int(settings, "height");
    ui->spinBox_Width->setValue(width);
    ui->spinBox_Height->setValue(height);


    bool shutdown = obs_data_get_bool(settings, "shutdown");
    ui->checkbox_ShutdownSourceNotVisible->setChecked(shutdown);

    connect(ui->buttonClose, &QPushButton::clicked,
        this, &AFQParticleEffectDialog::hide);

    connect(ui->pushButton_Size, &QPushButton::clicked, 
            this, &AFQParticleEffectDialog::_qslotBrowserSizeClicked);
    connect(ui->checkbox_ShutdownSourceNotVisible, &QCheckBox::clicked, 
            this, &AFQParticleEffectDialog::_qslotShutdownSourceNotVisibleChecked);
    connect(ui->pushButton_Interactive, &QPushButton::clicked,
            this, &AFQParticleEffectDialog::_qslotInteractionClicked);
    connect(ui->pushButton_Refresh, &QPushButton::clicked,
            this, &AFQParticleEffectDialog::_qslotRefreshClicked);
}

AFQParticleEffectDialog::~AFQParticleEffectDialog() 
{
    delete ui;

    qDeleteAll(m_buttonHash);
    m_buttonHash.clear();
}

void AFQParticleEffectDialog::showEvent(QShowEvent* event)
{
    resize(720, 550);
}

void AFQParticleEffectDialog::_CreateParticleButtons()
{
    if (!m_weakSource)
        return;

    m_selectedButton = nullptr;
    qDeleteAll(m_buttonHash);
    m_buttonHash.clear();

    constexpr int cols = 4;
    int row = 0;
    int col = 0;

    // Get obs data
    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    QString appliedParticleId = QString::fromUtf8(obs_data_get_string(settings, SETTING_PARTICLE_EFFECT_ID));

    // Create buttons
    size_t arrSize = sizeof(g_pszSoopParticleEffectId) / sizeof(g_pszSoopParticleEffectId[0]);
    for (int idx = 0; idx < arrSize; idx++)
    {
        QString particleId = QString::fromUtf8(g_pszSoopParticleEffectId[idx]);
        AFQParticleThumbnailWidget* button = new AFQParticleThumbnailWidget(this, particleId);
        button->setFixedSize(105, 80);
        m_buttonHash.insert(particleId, button);
        
        if (particleId == appliedParticleId) 
        {
            button->SetChecked(true);
            m_selectedButton = button;
        }
        
        connect(button, &AFQParticleThumbnailWidget::qsignalParticleThumbnailClicked,
                this, &AFQParticleEffectDialog::_qslotParticleThumbnailTriggered);

        row = idx / cols;
        col = idx % cols;

        ui->gridLayout_Particles->addWidget(button, row, col);
    }
}

AFQParticleThumbnailWidget* AFQParticleEffectDialog::_GetButtonWithId(const QString& id)
{
    if (id.isEmpty() || m_buttonHash.isEmpty())
        return nullptr;

    auto it = m_buttonHash.find(id);
    return (it != m_buttonHash.end()) ? it.value() : nullptr;
}

void AFQParticleEffectDialog::_SourceRemoved(void* data, calldata_t* params)
{
    QMetaObject::invokeMethod(static_cast<AFQParticleEffectDialog*>(data),
        "close");

}

void AFQParticleEffectDialog::_qslotParticleThumbnailTriggered(const QString& particleId)
{
    if (!m_weakSource) {
        return;
    }

    // Update ui
    if (m_selectedButton)
        m_selectedButton->SetChecked(false);
    m_selectedButton = _GetButtonWithId(particleId);

    // Update obs data
    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_property_t* property = obs_properties_get(m_props.get(), SETTING_PARTICLE_EFFECT_ID);

    auto it = m_buttonHash.find(particleId);
    if (it != m_buttonHash.end()) {
        obs_data_set_string(settings, SETTING_PARTICLE_EFFECT_ID, QT_TO_UTF8(particleId));
    }
    else {
        obs_data_set_string(settings, SETTING_PARTICLE_EFFECT_ID, "none");
    }
    
    obs_source_update(source, settings);
    obs_property_modified(property, settings);
}


void AFQParticleEffectDialog::_qslotBrowserSizeClicked()
{
    int width = ui->spinBox_Width->value();
    int height = ui->spinBox_Height->value();

    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_int(settings, "width", width);
    obs_data_set_int(settings, "height", height);
    obs_source_update(source, settings);
}

void AFQParticleEffectDialog::_qslotShutdownSourceNotVisibleChecked(bool checked)
{
    if (!m_weakSource) {
        return;
    }

    OBSSource source = GetSource(m_weakSource);
    OBSDataAutoRelease settings = obs_source_get_settings(source);
    obs_data_set_bool(settings, "shutdown", checked);
    obs_source_update(source, settings);
}

void AFQParticleEffectDialog::_qslotInteractionClicked()
{
    if (!m_weakSource) {
        return;
    }

    MAINFRAME->ShowBrowserInteractionPopup(GetSource(m_weakSource));
}

void AFQParticleEffectDialog::_qslotRefreshClicked()
{
    if (!m_weakSource) {
        return;
    }

    obs_property_t* prop = obs_properties_get(m_props.get(), "refreshnocache");
    obs_property_button_clicked(prop, GetSource(m_weakSource));
}