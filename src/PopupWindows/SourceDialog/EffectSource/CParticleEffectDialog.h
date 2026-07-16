#pragma once

#include "obs.hpp"
#include "UIComponent/CTopBaseWindow.h"
#include <QPointer>
#include "CParticleThumbnailWidget.h"

#define SOOP_PARTICLE_EFFECT_SOURCE_ID "soop_particle_effect_source"

static const char* g_pszSoopParticleEffectId[] = { 
    "heart_up", 
    "light",
    "flower", 
    "snow2", 
    "heart21", 
    "tv_noise", 
    "blood", 
    "ice", 
    "Beach", 
    "kkk", 
    "bubble", 
    "heart_down", 
    "circle", 
    "confetti", 
    "snowx", 
    "kiss",
    "stara", 
    "twinkle", 
    "up", 
    "zzz", 
    "starb", 
    "x-mas2",
    "valentine", 
    "autumn", 
    "Thanksgiving", 
};

namespace Ui {
    class AFQParticleEffectDialog;
}


class AFQParticleEffectDialog : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    AFQParticleEffectDialog(QWidget* parent, OBSSource source);
    ~AFQParticleEffectDialog();

protected:
    // common SOOP Source Props
    void showEvent(QShowEvent* event) override;

private:
    void _CreateParticleButtons();
    AFQParticleThumbnailWidget* _GetButtonWithId(const QString& id);
    static void _SourceRemoved(void* data, calldata_t* params);

private slots:
    void _qslotParticleThumbnailTriggered(const QString& particleId);
    void _qslotBrowserSizeClicked();
    void _qslotShutdownSourceNotVisibleChecked(bool checked);
    void _qslotInteractionClicked();
    void _qslotRefreshClicked();
    
private:
    QHash<QString, AFQParticleThumbnailWidget*> m_buttonHash;
    QPointer<AFQParticleThumbnailWidget> m_selectedButton = nullptr;

    OBSWeakSource m_weakSource = nullptr;
    using properties_delete_t = decltype(&obs_properties_destroy);
    using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;
    properties_t m_props;
    
    Ui::AFQParticleEffectDialog* ui = nullptr;

    OBSSignal removeSignal;
};