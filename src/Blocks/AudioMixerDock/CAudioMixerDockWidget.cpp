#include "CAudioMixerDockWidget.h"
#include "ui_audio-mixer-dock.h"

#include <QPushButton>
#include <QScrollBar>

#include <Application/CApplication.h>

#include "qt-wrappers.hpp"

#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"

#include "UIComponent/CItemWidgetHelper.h"      // For InsertQObjectByName() Func

#include "Blocks/AudioMixerDock/CVolumeControl.h"

#define AUDIO_TRACK_NUM 6
#define TRANSPARENT_PROPERTY "transparent"

AFAudioMixerWidget::AFAudioMixerWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::AFAudioMixerWidget)
{
    ui->setupUi(this);

    Init();
}

AFAudioMixerWidget::~AFAudioMixerWidget()
{
    delete ui;
}

void AFAudioMixerWidget::Init()
{
    // Context Menu
    ui->hMixerScrollArea->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->vMixerScrollArea->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->hMixerScrollArea, &QWidget::customContextMenuRequested,
            MAINFRAME, &AFMainFrame::qslotStackedMixerAreaContextMenuRequested);
    connect(ui->vMixerScrollArea, &QWidget::customContextMenuRequested,
            MAINFRAME, &AFMainFrame::qslotStackedMixerAreaContextMenuRequested);

    // Set ScrollBar Transparent
    ui->hMixerScrollArea->verticalScrollBar()->setProperty(TRANSPARENT_PROPERTY, true);
    ui->vMixerScrollArea->horizontalScrollBar()->setProperty(TRANSPARENT_PROPERTY, true);
    
    PolishStyleSheet(ui->hMixerScrollArea);
    PolishStyleSheet(ui->vMixerScrollArea);
}

void AFAudioMixerWidget::ActivateAudioSource(OBSSource source)
{
    bool vertical = config_get_bool(USERCONFIG, "BasicWindow", "VerticalVolControl");

    AFQVolControl* vol = new AFQVolControl(nullptr, source, true, vertical);
    vol->EnableSlider(!AFSourceUtil::SourceVolumeLocked(source));

    double meterDecayRate = config_get_double(ACTIVECONFIG, "Audio", "MeterDecayRate");

    vol->SetMeterDecayRate(meterDecayRate);

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
    vol->setPeakMeterType(peakMeterType);
    vol->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(vol, &QWidget::customContextMenuRequested, MAINFRAME, &AFMainFrame::qslotVolControlContextMenu);
    connect(vol, &AFQVolControl::ConfigClicked, MAINFRAME, &AFMainFrame::qslotVolControlContextMenu);

    VolControlVector& volumes = SCENE_CONTEXT.GetVolControlVector();

#pragma region _SOOP_BREAKTIME
    auto channel = obs_get_output_source_channel(source);
    if(channel == 7) // BreakTime BGM
        volumes.insert(volumes.begin(), vol); // Must Front Add
    else
        InsertQObjectByName(volumes, vol); // Sort Control  
#pragma endregion

    for (auto volume : volumes) {
        if (vertical)
            ui->horizontalLayout_MixerVertical->addWidget(volume);
        else
            ui->verticalLayout_MixerHorizontal->addWidget(volume);
    }

    for (AFQVolControl* vol : volumes)
        vol->refreshColors();
}

void AFAudioMixerWidget::SetMixerLayout(bool vertical) 
{
    if (vertical) {
        ui->stackedMixerArea->setCurrentIndex(1);
    }
    else {
        ui->stackedMixerArea->setCurrentIndex(0);
    }
}

void AFAudioMixerWidget::enterEvent(QEnterEvent* event) 
{
    SetScrollBarVisibility(false);
    QWidget::enterEvent(event);
}

void AFAudioMixerWidget::leaveEvent(QEvent* event)
{
    SetScrollBarVisibility(true);
    QWidget::leaveEvent(event);
}

void AFAudioMixerWidget::resizeEvent(QResizeEvent* event)
{
    SetScrollBarVisibility(true);
    QWidget::resizeEvent(event);
}

void AFAudioMixerWidget::SetScrollBarVisibility(bool transparent)
{
    QScrollBar* scrollbar = nullptr;
    if (ui->stackedMixerArea->currentIndex() == 0) {
        SetScrollBarTransparent(ui->hMixerScrollArea,
                                ui->hVolumeWidgets,
                                transparent, Qt::Vertical);

    } else {
        SetScrollBarTransparent(ui->vMixerScrollArea,
                                ui->vVolumeWidgets,
                                transparent, Qt::Horizontal);
    }
}