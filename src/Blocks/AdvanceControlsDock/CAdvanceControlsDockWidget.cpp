#include "CAdvanceControlsDockWidget.h"
#include "ui_advance-controls-dock.h"

#include "CoreModel/OBSOutput/COutput.h"

#include "MainFrame/CMainFrame.h"

AFAdvanceControlsWidget::AFAdvanceControlsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFAdvanceControlsWidget)
{
    ui->setupUi(this);
    
    Initialize();
}

AFAdvanceControlsWidget::~AFAdvanceControlsWidget()
{
    delete ui;
}

void AFAdvanceControlsWidget::EnableReplayBuffer(bool enable)
{
    ui->replayBufferWidget_3->setEnabled(enable);
}

void AFAdvanceControlsWidget::ReplayBufferButtonClicked()
{
    bool bufferStart = MAINFRAME->qslotReplayBufferClicked();
}


void  AFAdvanceControlsWidget::ReplayBufferReleased() 
{
    ui->replayBufferWidget->setEnabled(true);
}

void AFAdvanceControlsWidget::SaveReplayBufferButtonClicked()
{ 
    ui->saveReplayButton->setEnabled(false);
    MAINFRAME->qslotReplayBufferSave();
}

void AFAdvanceControlsWidget::SaveReplayBufferButtonEnabled()
{
    ui->saveReplayButton->setEnabled(AFOutputUtil::IsReplayBufferActive());
}

void AFAdvanceControlsWidget::SetReplayBufferStartStopStyle(bool bufferStart)
{
    QString BufferText = bufferStart ? QTStr("ReplayBuffer.Stop") :
                                       QTStr("ReplayBuffer.Start");

    ui->replayBufferButton->setText(BufferText);
    ui->replayBufferButton->setEnabled(true);
    ui->saveReplayButton->setEnabled(bufferStart);
}

void AFAdvanceControlsWidget::SetReplayBufferStoppingStyle()
{
    ui->replayBufferButton->setText(QTStr("ReplayBuffer.Stopping"));
    ui->replayBufferButton->setEnabled(false);
    ui->saveReplayButton->setEnabled(false);
}

void AFAdvanceControlsWidget::Initialize()
{
    ui->replayBufferLabel->setText(QTStr("ReplayBuffer"));
    ui->saveReplayButton->setText(QTStr("ReplayBuffer.Download"));

    connect(ui->replayBufferButton, &QPushButton::clicked,
            this, &AFAdvanceControlsWidget::ReplayBufferButtonClicked);

    connect(ui->saveReplayButton, &QPushButton::clicked,
        this, &AFAdvanceControlsWidget::SaveReplayBufferButtonClicked);

    ui->saveReplayButton->setEnabled(AFOutputUtil::IsReplayBufferActive());
    connect(MAINFRAME, &AFMainFrame::qsignalReplayBufferSaved, 
        this, &AFAdvanceControlsWidget::SaveReplayBufferButtonEnabled);
}