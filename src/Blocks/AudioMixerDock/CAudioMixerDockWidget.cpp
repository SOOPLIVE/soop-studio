#include <QPushButton>
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "UIComponent/CCustomPushbutton.h"
#include "UIComponent/CNameDialog.h"
#include "CAudioMixerDockWidget.h"
#include "ui_audio-mixer-dock.h"
#include "qt-wrapper.h"
#include <Application/CApplication.h>
#include "UIComponent/CMessageBox.h"

#define AUDIO_TRACK_NUM 6

#pragma region QT Field, CTOR/DTOR
AFAudioMixerDockWidget::AFAudioMixerDockWidget(QWidget* parent) :
    AFQBaseDockWidget(parent),
    ui(new Ui::AFAudioMixerDockWidget)
{
    ui->setupUi(this);
}

AFAudioMixerDockWidget::~AFAudioMixerDockWidget()
{
    delete ui;
}