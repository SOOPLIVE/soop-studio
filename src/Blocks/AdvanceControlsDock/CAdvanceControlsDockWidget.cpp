#include "CAdvanceControlsDockWidget.h"
#include "ui_advance-controls-dock.h"


#include "qt-wrapper.h"

#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Config/CConfigManager.h"
#include "platform/platform.hpp"
#include "Application/CApplication.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

AFAdvanceControlsDockWidget::AFAdvanceControlsDockWidget(QWidget *parent) :
    AFQBaseDockWidget(parent),
    ui(new Ui::AFAdvanceControlsDockWidget)
{
    ui->setupUi(this);
    
    _Initialize();
}

AFAdvanceControlsDockWidget::~AFAdvanceControlsDockWidget()
{
    delete ui;
}

void AFAdvanceControlsDockWidget::EnableReplayBuffer(bool enable)
{
    ui->widget_ReplayBufferButton->setEnabled(enable);
}

void AFAdvanceControlsDockWidget::qslotClickedReplayBuffer()
{
    _ChangeReplayBufferStyle(WidgetState::Pressed);

    AFMainFrame* main = App()->GetMainView();
    bool bufferStart = main->qslotReplayBufferClicked();

    //main->qslotPauseRecordingClicked();
}

void AFAdvanceControlsDockWidget::qslotReleasedStudioMode() {
    _ChangeStudioWidgetStyle(WidgetState::Default);
}

void  AFAdvanceControlsDockWidget::qslotReleasedReplayBuffer() {
    _ChangeReplayBufferStyle(WidgetState::Default);
}

void AFAdvanceControlsDockWidget::qslotClickedDownloadReplayBuffer()
{ 
    AFMainFrame* main = App()->GetMainView();
    main->qslotReplayBufferSave();
}

void AFAdvanceControlsDockWidget::SetStudioMode(bool isStudioMode)
{ 
    m_IsStudioMode = isStudioMode; 
    _SetStudioModeText();
    _ChangeStudioIconStyle();
}

void AFAdvanceControlsDockWidget::SetReplayBufferStartStopStyle(bool bufferStart)
{
    QString BufferText = bufferStart ? QTStr("Basic.Main.StopReplayBuffer") :
                                       QTStr("Basic.Main.StartReplayBuffer");

    ui->label_TextReplayBuffer->setText(BufferText);
    m_qDownloadReplayBuffer->setVisible(bufferStart);

    if (bufferStart)
    {
        ui->widget_ReplayBufferArea->layout()->addWidget(m_qDownloadReplayBuffer);
        ui->label_IconReplayBuffer->setStyleSheet("");
    }
    else
    {
        ui->widget_ReplayBufferArea->layout()->removeWidget(m_qDownloadReplayBuffer);
        ui->label_IconReplayBuffer->setStyleSheet("");
    }
}

void AFAdvanceControlsDockWidget::SetReplayBufferStoppingStyle()
{
    QString BufferText = QTStr("Basic.Main.StoppingReplayBuffer");
    ui->label_TextReplayBuffer->setText(BufferText);
}

void AFAdvanceControlsDockWidget::qslotClickedStudioMode()
{
    m_IsStudioMode = !m_IsStudioMode;

    _ChangeStudioWidgetStyle(WidgetState::Pressed);
    App()->GetMainView()->GetMainWindow()->qslotToggleStudioModeBlock(m_IsStudioMode);
}

void AFAdvanceControlsDockWidget::_Initialize()
{
    auto& locale = AFLocaleTextManager::GetSingletonInstance();

    m_qDownloadReplayBuffer = new QPushButton(this);
    m_qDownloadReplayBuffer->setObjectName("pushButton_DownloadReplayBuffer");
    m_qDownloadReplayBuffer->setFixedSize(50, 40);
    m_qDownloadReplayBuffer->setIconSize(QSize(24, 24));

    connect(m_qDownloadReplayBuffer, &QPushButton::clicked,
        this, &AFAdvanceControlsDockWidget::qslotClickedDownloadReplayBuffer);

    qslotReleasedStudioMode();
    qslotReleasedReplayBuffer();

    _SetStudioModeText();
    ui->label_TextReplayBuffer->setText(
      QT_UTF8(locale.Str("Basic.Main.StartReplayBuffer")));

    connect(ui->widget_StudioModeButton, &AFQBaseClickWidget::qsignalLeftClick,
            this, &AFAdvanceControlsDockWidget::qslotClickedStudioMode);
    connect(ui->widget_StudioModeButton, &AFQBaseClickWidget::qsignalLeftReleased,
            this, &AFAdvanceControlsDockWidget::qslotReleasedStudioMode);

    connect(ui->widget_ReplayBufferButton, &AFQBaseClickWidget::qsignalLeftClick,
            this, &AFAdvanceControlsDockWidget::qslotClickedReplayBuffer);
    connect(ui->widget_ReplayBufferButton, &AFQBaseClickWidget::qsignalLeftReleased,
            this, &AFAdvanceControlsDockWidget::qslotReleasedReplayBuffer);
}

void AFAdvanceControlsDockWidget::_SetStudioModeText()
{
    if (m_IsStudioMode)
        ui->label_TextStudioMode->setText(QTStr("Basic.ToggleBasicProgramMode"));
    else
        ui->label_TextStudioMode->setText(QTStr("Basic.TogglePreviewProgramMode"));
}

void AFAdvanceControlsDockWidget::_ChangeStudioWidgetStyle(WidgetState state) {
    if (state == WidgetState::Pressed) { // pressed
        ui->widget_StudioModeButton->setProperty("AdvControlWdtPressed", true);
        ui->label_TextStudioMode->setEnabled(false);
        ui->label_RightArrowImg->setEnabled(false);
    }
    else { // normal
        ui->widget_StudioModeButton->setProperty("AdvControlWdtPressed", false);
        ui->label_TextStudioMode->setEnabled(true);
        ui->label_RightArrowImg->setEnabled(true);
    }

    _ChangeStudioIconStyle();

    style()->unpolish(ui->widget_StudioModeButton);
    style()->polish(ui->widget_StudioModeButton);
}

void AFAdvanceControlsDockWidget::_ChangeStudioIconStyle()
{
    bool isPressed = ui->widget_StudioModeButton->property("AdvControlWdtPressed").toBool();

    if (isPressed) {
        if (m_IsStudioMode)
            ui->label_IconStudioMode->setProperty("AdvControlWdtStyle", "studioMode-pressed");
        else
            ui->label_IconStudioMode->setProperty("AdvControlWdtStyle", "basicMode-pressed");
    }
    else {
        if (m_IsStudioMode)
            ui->label_IconStudioMode->setProperty("AdvControlWdtStyle", "studioMode");
        else
            ui->label_IconStudioMode->setProperty("AdvControlWdtStyle", "basicMode");
    }

    style()->unpolish(ui->label_IconStudioMode);
    style()->polish(ui->label_IconStudioMode);
}

void AFAdvanceControlsDockWidget::_ChangeReplayBufferStyle(WidgetState state) {
    if (state == WidgetState::Pressed) { // pressed
        ui->widget_ReplayBufferButton->setProperty("AdvControlWdtPressed", true);
        ui->label_TextReplayBuffer->setEnabled(false);
        ui->label_IconReplayBuffer->setEnabled(false);
        ui->label_RightReplayBuffer->setEnabled(false);
    }
    else { // normal
        ui->widget_ReplayBufferButton->setProperty("AdvControlWdtPressed", false);
        ui->label_TextReplayBuffer->setEnabled(true);
        ui->label_IconReplayBuffer->setEnabled(true);
        ui->label_RightReplayBuffer->setEnabled(true);
    }
    style()->unpolish(ui->widget_ReplayBufferButton);
    style()->polish(ui->widget_ReplayBufferButton);
}