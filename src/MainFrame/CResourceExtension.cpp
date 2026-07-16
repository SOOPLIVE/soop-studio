#include "CResourceExtension.h"
#include "ui_resource-extension.h"

#include "Application/CApplication.h"

#include "CoreModel/Statistics/CStatistics.h"

#include "MainFrame/CMainFrame.h"

AFResourceExtension::AFResourceExtension(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFResourceExtension)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);
}

AFResourceExtension::~AFResourceExtension()
{
    disconnect(ui->pushButton_Stat, &QPushButton::clicked, this, &AFResourceExtension::qsignalStatWindowTriggered);
    disconnect(MAINFRAME, &AFMainFrame::qsignalRefreshTimerTick, this, &AFResourceExtension::qslotResourceUpdateTimerTick);
    //
    delete ui;
}

void AFResourceExtension::qslotResourceUpdateTimerTick()
{
    _RefreshCPUText();
    _RefreshDiskText();
    _RefreshMemoryText();
    //_RefreshNetworkText();

    _RefreshFPSText();
}

void AFResourceExtension::qslotCPUState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_CpuIcon, state);
}

void AFResourceExtension::qslotDiskState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_DiskIcon, state);
}

void AFResourceExtension::qslotMemoryState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_MemoryIcon, state);
}

void AFResourceExtension::qslotNetworkState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_NetworkIcon, state);

    if (state == PCStatState::None)
        ui->label_NetworkValue->setText("Stand By");
    else if (state == PCStatState::Normal)
        ui->label_NetworkValue->setText("Good");
    else
        ui->label_NetworkValue->setText("Bad");
}

void AFResourceExtension::qslotFPSState(PCStatState state)
{
    AFMainFrame::SetPCStateIconStyle(ui->label_FPSIcon, state);
}

void AFResourceExtension::ResourceExtensionInit()
{
    auto& statistics = STATISTICS;
    connect(ui->pushButton_Stat, &QPushButton::clicked, this, &AFResourceExtension::qsignalStatWindowTriggered);
    connect(MAINFRAME, &AFMainFrame::qsignalRefreshTimerTick, this, &AFResourceExtension::qslotResourceUpdateTimerTick);
    connect(&statistics, &AFStatistics::qsignalCPUState, this, &AFResourceExtension::qslotCPUState);
    connect(&statistics, &AFStatistics::qsignalDiskState, this, &AFResourceExtension::qslotDiskState);
    connect(&statistics, &AFStatistics::qsignalMemoryState, this, &AFResourceExtension::qslotMemoryState);
    connect(&statistics, &AFStatistics::qsignalNetworkState, this, &AFResourceExtension::qslotNetworkState);

    // Set State Icon
    PCStatState stateState = statistics.GetCPUIconState();
    qslotCPUState(stateState);
    stateState = statistics.GetDiskIconState();
    qslotDiskState(stateState);
    stateState = statistics.GetMemoryIconState();
    qslotMemoryState(stateState);
    stateState = statistics.GetNetworkIconState();
    qslotNetworkState(stateState);

    // Set State
    _RefreshCPUText();
    _RefreshDiskText();
    _RefreshMemoryText();
    connect(&statistics, &AFStatistics::qsignalFPSState, this, &AFResourceExtension::qslotFPSState);
    _RefreshFPSText();
    stateState = statistics.GetNetworkIconState();
    qslotFPSState(stateState);
}

void AFResourceExtension::_RefreshCPUText()
{
    QString text;
    text += QString::number(STATISTICS.GetCPUUsage(), 'f', 1) + QString("%");

    ui->label_CpuValue->setText(text);
}

void AFResourceExtension::_RefreshDiskText()
{
    uint64_t num_bytes = STATISTICS.GetDiskSize();
    double gigBytes = (double)num_bytes / (1024 * 1024 * 1024);
    QString text;
    text += QString::number(gigBytes, 'f', 1) + QString("GB");

    ui->label_DiskValue->setText(text);
}

void AFResourceExtension::_RefreshMemoryText()
{
    long double num = (long double)STATISTICS.GetMemorySize();

    QString str = QString::number(num, 'f', 1) + QStringLiteral("MB");
    ui->label_MemoryValue->setText(str);
}

void AFResourceExtension::_RefreshNetworkText()
{
    int network = STATISTICS.GetNetworkState();
    QString str = QString::number(network) + QStringLiteral("%");
    ui->label_NetworkValue->setText(str);
}

void AFResourceExtension::_RefreshFPSText()
{
    struct obs_video_info ovi = {};
    obs_get_video_info(&ovi);
    double obsFPS = (double)ovi.fps_num / (double)ovi.fps_den;

    double fps = STATISTICS.GetCurFPS();
    QString str = QString("%1 / %2").arg(QString::number(fps, 'f', 2)).arg(QString::number(obsFPS, 'f', 2));
    ui->label_FPSValue->setText(str);
    //ui->label_F
}
