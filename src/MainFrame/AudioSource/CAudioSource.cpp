#include "CAudioSource.h"

#include "ui_aneta-main-frame.h"
#include "platform/platform.hpp"

#include "CoreModel/Scene/CSceneContext.h"

CMainAudioSource::CMainAudioSource(QObject* parent) :
    QObject(parent)
{

}

CMainAudioSource::~CMainAudioSource()
{

}

void CMainAudioSource::qslotCloseVolumeSlider()
{
    int volumeSize = m_volumeSliderFrame->VolumeSize();
    bool volumechecked = m_volumeSliderFrame->ButtonIsChecked();

    disconnect(m_audioPeakUpdateTimer, &QTimer::timeout, 
               this, &CMainAudioSource::qslotSetAudioPeakValue);
    m_volumeSliderFrame->hide();
    m_audioPeakUpdateTimer->stop();
}

void CMainAudioSource::qslotStopVolumeTimer()
{
    if(m_volumeTimer)
        m_volumeTimer->stop();
}

void CMainAudioSource::qslotMainAudioValueChanged(float volume)
{
    m_output = volume * (float)96; // -96 ~ 0 (db)
    auto mul = obs_db_to_mul(m_output);
    soop_set_master_output_volume(mul);
}

void CMainAudioSource::qslotCloseMicSlider()
{
    int volumeSize = m_micSliderFrame->VolumeSize();
    bool micchecked = m_micSliderFrame->ButtonIsChecked();

    disconnect(m_micPeakUpdateTimer, &QTimer::timeout, 
               this, &CMainAudioSource::qslotSetMicPeakValue);
    m_micSliderFrame->hide();
    m_micPeakUpdateTimer->stop();
}

void CMainAudioSource::qslotStopMicTimer()
{
    if(m_micTimer)
        m_micTimer->stop();
}

void CMainAudioSource::qslotMainMicValueChanged(float volume)
{
    m_input = volume * (float)96; // -96 ~ 0 (db)
    auto mul = obs_db_to_mul(m_input);
    soop_set_master_input_volume(mul);
}

void CMainAudioSource::qslotShowVolumeSlider()
{
    if (!MAINFRAME_UI->widget_Volume->isEnabled())
        return;

    m_volumeTimer = new QTimer(this);
    m_volumeTimer->setInterval(300);
    m_volumeTimer->setSingleShot(true);

    m_audioPeakUpdateTimer = new QTimer(this);
    m_audioPeakUpdateTimer->setInterval(50);

    connect(m_volumeTimer, &QTimer::timeout, 
            this, &CMainAudioSource::qslotCloseVolumeSlider);
    connect(m_audioPeakUpdateTimer, &QTimer::timeout, 
            this, &CMainAudioSource::qslotSetAudioPeakValue);

    QPoint globalPos = MAINFRAME_UI->widget_Volume->mapToGlobal(QPoint(0, 0));

    int posX = globalPos.x() - 6;
    int posY = globalPos.y() - m_volumeSliderFrame->height() + 30;

    globalPos.setX(posX);
    globalPos.setY(posY);
    m_volumeSliderFrame->move(globalPos);

    m_volumeSliderFrame->show();
    m_volumeTimer->start();
    m_audioPeakUpdateTimer->start();
}

void CMainAudioSource::qslotShowMicSlider()
{
    if (!MAINFRAME_UI->widget_Mic->isEnabled())
        return;

    m_micTimer = new QTimer(this);
    m_micTimer->setInterval(300);
    m_micTimer->setSingleShot(true);

    m_micPeakUpdateTimer = new QTimer(this);
    m_micPeakUpdateTimer->setInterval(50);

    connect(m_micTimer, &QTimer::timeout, 
            this, &CMainAudioSource::qslotCloseMicSlider);
    connect(m_micPeakUpdateTimer, &QTimer::timeout, 
            this, &CMainAudioSource::qslotSetMicPeakValue);

#if 0
    qreal ratio = 1;
    QScreen* screen = MAINFRAME->screen();
    if (screen)
        ratio = screen->devicePixelRatio();
#endif

    QPoint globalPos = MAINFRAME_UI->widget_Mic->mapToGlobal(QPoint(0, 0));

    int posX = globalPos.x() - 6; // 6: Slider frame left margin
    int posY = globalPos.y() - m_micSliderFrame->height() + 30; // 30: Slider frame button size + bottom margin

    globalPos.setX(posX); 
    globalPos.setY(posY);
    m_micSliderFrame->move(globalPos);

    m_micSliderFrame->show();
    m_micTimer->start();
    m_micPeakUpdateTimer->start();
}

void CMainAudioSource::qslotSetVolumeMute()
{
	bool isMuted = IsAudioMuted();
    _SetAudioButtonMute(!isMuted);
}

void CMainAudioSource::qslotSetMicMute()
{
	bool isMuted = IsMicMuted();
    _SetMicButtonMute(!isMuted);
}

void CMainAudioSource::qslotSetAudioPeakValue()
{
    _SetMainAudioVolumePeak();
}

void CMainAudioSource::qslotSetMicPeakValue()
{
    _SetMainMicVolumePeak();
}

void CMainAudioSource::SetupMainFrameAudioUI(QWidget* parent)
{
    config_t* appConfig = APPCONFIG;
    // output audio
    m_volumeSliderFrame = new AFQSliderFrame(parent);
    m_volumeSliderFrame->InitSliderFrame("assets/mainview/default/soundvolume.svg", true, 4096);

    connect(m_volumeSliderFrame, &AFQSliderFrame::qsignalMouseLeave, this, &CMainAudioSource::qslotCloseVolumeSlider);
    connect(m_volumeSliderFrame, &AFQSliderFrame::qsignalMouseEnterSlider, this, &CMainAudioSource::qslotStopVolumeTimer);
    connect(m_volumeSliderFrame, &AFQSliderFrame::qsignalMuteButtonClicked, this, &CMainAudioSource::qslotSetVolumeMute);
    bool savedMute = config_get_bool(appConfig, "Audio", "MainAudioMute");
    _SetAudioButtonMute(savedMute);
    connect(m_volumeSliderFrame, &AFQSliderFrame::qsignalVolumeChanged, this, &CMainAudioSource::qslotMainAudioValueChanged);
    int savedVolume = config_get_int(appConfig, "Audio", "MainAudioVolume");
    m_volumeSliderFrame->SetVolumeSize(savedVolume);

    // input audio

    m_micSliderFrame = new AFQSliderFrame(parent);
    m_micSliderFrame->setWindowFlag(Qt::WindowStaysOnTopHint);
    m_micSliderFrame->InitSliderFrame("assets/mainview/default/micvolume.svg", true, 4096);

    connect(m_micSliderFrame, &AFQSliderFrame::qsignalMouseLeave, this, &CMainAudioSource::qslotCloseMicSlider);
    connect(m_micSliderFrame, &AFQSliderFrame::qsignalMouseEnterSlider, this, &CMainAudioSource::qslotStopMicTimer);
    connect(m_micSliderFrame, &AFQSliderFrame::qsignalMuteButtonClicked, this, &CMainAudioSource::qslotSetMicMute);
    savedMute = config_get_bool(appConfig, "Audio", "MainMicMute");
    _SetMicButtonMute(savedMute);
    connect(m_micSliderFrame, &AFQSliderFrame::qsignalVolumeChanged, this, &CMainAudioSource::qslotMainMicValueChanged);
    savedVolume = config_get_int(appConfig, "Audio", "MainMicVolume");
    m_micSliderFrame->SetVolumeSize(savedVolume);
    
    // connect slot MainFrame UI 
    connect(MAINFRAME_UI->widget_Volume, &AFQHoverWidget::qsignalHoverEnter, this, &CMainAudioSource::qslotShowVolumeSlider);
    connect(MAINFRAME_UI->widget_Mic, &AFQHoverWidget::qsignalHoverEnter, this, &CMainAudioSource::qslotShowMicSlider);
}

void CMainAudioSource::DestoryMainFrameAudioUI()
{
    config_t* appConfig = APPCONFIG;
    //
    if (m_volumeSliderFrame) {
        // save main audio volume
        config_set_int(appConfig, "Audio", "MainAudioVolume", m_volumeSliderFrame->VolumeSize());
        config_set_bool(appConfig, "Audio", "MainAudioMute", IsAudioMuted());
    }

    if (m_micSliderFrame) {
        // save main mic volume
        config_set_int(appConfig, "Audio", "MainMicVolume", m_micSliderFrame->VolumeSize());
        config_set_bool(appConfig, "Audio", "MainMicMute", IsMicMuted());
    }

    config_save_safe(appConfig, "tmp", nullptr);

    m_volumeSliderFrame = nullptr;
    m_micSliderFrame = nullptr;
}

void CMainAudioSource::ClearVolumeControls()
{
    VolControlVector& volumes = SCENE_CONTEXT.GetVolControlVector();
    for (AFQVolControl* vol : volumes)
        delete vol;

    volumes.clear();
}

void CMainAudioSource::RefreshVolumeColors()
{
    VolControlVector& volumes = SCENE_CONTEXT.GetVolControlVector();
    for (AFQVolControl* vol : volumes)
        vol->refreshColors();
}

void CMainAudioSource::UpdateVolumeControlsDecayRate()
{
    double meterDecayRate = config_get_double(ACTIVECONFIG, "Audio", "MeterDecayRate");

    VolControlVector& volumes = SCENE_CONTEXT.GetVolControlVector();
    for (size_t i = 0; i < volumes.size(); i++)
        volumes[i]->SetMeterDecayRate(meterDecayRate);
}

void CMainAudioSource::UpdateVolumeControlsPeakMeterType()
{
    uint32_t peakMeterTypeIdx = config_get_uint(ACTIVECONFIG, "Audio", "PeakMeterType");

    enum obs_peak_meter_type peakMeterType;
    switch (peakMeterTypeIdx) {
    case 0:
        peakMeterType = SAMPLE_PEAK_METER;
        break;
    case 1:
        peakMeterType = TRUE_PEAK_METER;
        break;
    default:
        peakMeterType = SAMPLE_PEAK_METER;
        break;
    }

    VolControlVector& volumes = SCENE_CONTEXT.GetVolControlVector();
    for (size_t i = 0; i < volumes.size(); i++)
        volumes[i]->setPeakMeterType(peakMeterType);
}

bool CMainAudioSource::IsAudioMuted()
{
    if (m_volumeSliderFrame)
        return m_volumeSliderFrame->IsVolumeMuted();
    return true;
}

bool CMainAudioSource::IsMicMuted()
{
    if (m_micSliderFrame)
        return m_micSliderFrame->IsVolumeMuted();
    return true;
}

void CMainAudioSource::_SetMainAudioVolumePeak()
{
    auto maxPeak = (float)-96;

    VolControlVector& volumes = SCENE_CONTEXT.GetVolControlVector();
    for (size_t i = 0; i < volumes.size(); i++)
    {
        if (obs_source_muted(volumes[i]->GetSource()))
            continue;

        auto type = obs_source_get_monitoring_type(volumes[i]->GetSource());
        if (type == obs_monitoring_type::OBS_MONITORING_TYPE_MONITOR_ONLY)
            continue;

        auto channel = obs_get_output_source_channel(volumes[i]->GetSource());
        if ((channel > 2) && (channel < 7))
            continue;
        
        maxPeak = maxPeak > volumes[i]->GetCurrentPeak() ? maxPeak : volumes[i]->GetCurrentPeak();
    }

    if (m_volumeSliderFrame)
    {
        if (m_volumeSliderFrame->isVisible())
        {
            maxPeak += m_output;
            if (maxPeak > (float)0)
                maxPeak = (float)0;
            else if (maxPeak < (float)-96)
                maxPeak = (float)-96;

            m_volumeSliderFrame->SetVolumePeak(maxPeak);
        }
    }
}

void CMainAudioSource::_SetMainMicVolumePeak()
{
    auto maxPeak = (float)-96;

    VolControlVector& volumes = SCENE_CONTEXT.GetVolControlVector();
    for (size_t i = 0; i < volumes.size(); i++)
    {
        if (obs_source_muted(volumes[i]->GetSource()))
            continue;

        auto type = obs_source_get_monitoring_type(volumes[i]->GetSource());
        if (type == obs_monitoring_type::OBS_MONITORING_TYPE_MONITOR_ONLY) 
            continue;

        auto channel = obs_get_output_source_channel(volumes[i]->GetSource());
        if ((channel < 3) || (channel > 6)) 
            continue;

        maxPeak = maxPeak > volumes[i]->GetCurrentPeak() ? maxPeak : volumes[i]->GetCurrentPeak();
    }

    if (m_micSliderFrame)
    {
        if (m_micSliderFrame->isVisible())
        {
            maxPeak += m_input;
            if (maxPeak > (float)0)
                maxPeak = (float)0;
            else if (maxPeak < (float)-96)
                maxPeak = (float)-96;

            m_micSliderFrame->SetVolumePeak(maxPeak);
        }
    }
}

void CMainAudioSource::_SetAudioVolumeSliderSignalsBlock(bool block)
{
    if (m_volumeSliderFrame)
        m_volumeSliderFrame->BlockSliderSignal(block);
}

void CMainAudioSource::_SetMicVolumeSliderSignalsBlock(bool block)
{
    if (m_micSliderFrame)
        m_micSliderFrame->BlockSliderSignal(block);
}

void CMainAudioSource::_SetAudioSliderEnabled(bool enabled)
{
    if (m_volumeSliderFrame)
        m_volumeSliderFrame->SetVolumeSliderEnabled(enabled);
}

void CMainAudioSource::_SetMicSliderEnabled(bool enabled)
{
    if (m_micSliderFrame)
        m_micSliderFrame->SetVolumeSliderEnabled(enabled);
}

void CMainAudioSource::_SetAudioButtonMute(bool mute)
{
    if (m_volumeSliderFrame) {
        m_volumeSliderFrame->SetButtonProperty(mute ? "soundVolumeMute" : "soundVolume");

        MAINFRAME_UI->widget_Volume->setProperty("mute", mute);
        MAINFRAME_UI->widget_Volume->style()->unpolish(MAINFRAME_UI->widget_Volume);
        MAINFRAME_UI->widget_Volume->style()->polish(MAINFRAME_UI->widget_Volume);

        m_volumeSliderFrame->SetVolumeMuted(mute);
    }

    MAINFRAME->OnSoopEvent(mute ?
                           soop_frontend_type::SOOP_FRONTEND_EVENT_TOOGLE_MAIN_VOLUME_OFF :
                           soop_frontend_type::SOOP_FRONTEND_EVENT_TOOGLE_MAIN_VOLUME_ON, nullptr);

    soop_set_master_output_muted(mute);
}

void CMainAudioSource::_SetMicButtonMute(bool mute)
{
    if (m_micSliderFrame) {
        m_micSliderFrame->SetButtonProperty(mute ? "micVolumeMute" : "micVolume");
        
        MAINFRAME_UI->widget_Mic->setProperty("mute", mute);
        MAINFRAME_UI->widget_Mic->style()->unpolish(MAINFRAME_UI->widget_Mic);
        MAINFRAME_UI->widget_Mic->style()->polish(MAINFRAME_UI->widget_Mic);

        m_micSliderFrame->SetVolumeMuted(mute);
    }

    MAINFRAME->OnSoopEvent(mute ?
                           soop_frontend_type::SOOP_FRONTEND_EVENT_TOOGLE_MAIN_MIC_OFF :
                           soop_frontend_type::SOOP_FRONTEND_EVENT_TOOGLE_MAIN_MIC_ON, nullptr);

    soop_set_master_input_muted(mute);
}
