#include "CResourceExtension.h"
#include "ui_resource-extension.h"

#include "Application/CApplication.h"

AFResourceExtension::AFResourceExtension(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFResourceExtension)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
}

AFResourceExtension::~AFResourceExtension()
{
    disconnect(ui->pushButton_Stat, &QPushButton::clicked,
        this, &AFResourceExtension::qsignalStatWindowTriggered);

    disconnect(App()->GetMainView(), &AFMainFrame::qsignalRefreshTimerTick,
        this, &AFResourceExtension::qslotResourceUpdateTimerTick);

    m_Statistics = nullptr;
    //
    delete ui;
}

void AFResourceExtension::qslotResourceUpdateTimerTick()
{
    _RefreshCPUText();
    _RefreshDiskText();
    _RefreshMemoryText();
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

void AFResourceExtension::ResourceExtensionInit()
{
    m_Statistics = App()->GetStatistics();

    connect(ui->pushButton_Stat, &QPushButton::clicked,
        this, &AFResourceExtension::qsignalStatWindowTriggered);
    connect(App()->GetMainView(), &AFMainFrame::qsignalRefreshTimerTick,
        this, &AFResourceExtension::qslotResourceUpdateTimerTick);
    connect(m_Statistics, &AFStatistics::qsignalCPUState, 
        this, &AFResourceExtension::qslotCPUState);
    connect(m_Statistics, &AFStatistics::qsignalDiskState, 
        this, &AFResourceExtension::qslotDiskState);
    connect(m_Statistics, &AFStatistics::qsignalMemoryState, 
        this, &AFResourceExtension::qslotMemoryState);
    connect(m_Statistics, &AFStatistics::qsignalNetworkState, 
        this, &AFResourceExtension::qslotNetworkState);

    // Set State Icon
    PCStatState stateState = m_Statistics->GetCPUIconState();
    qslotCPUState(stateState);
    stateState = m_Statistics->GetDiskIconState();
    qslotDiskState(stateState);
    stateState = m_Statistics->GetMemoryIconState();
    qslotMemoryState(stateState);
    stateState = m_Statistics->GetNetworkIconState();
    qslotNetworkState(stateState);

    // Set State
    _RefreshCPUText();
    _RefreshDiskText();
    _RefreshMemoryText();
}

void AFResourceExtension::_RefreshCPUText()
{
    QString text;
    text += QString::number(m_Statistics->GetCPUUsage(), 'f', 1) + QString("%");

    ui->label_CpuValue->setText(text);
}

void AFResourceExtension::_RefreshDiskText()
{
    uint64_t num_bytes = m_Statistics->GetDiskSize();
    double gigBytes = (double)num_bytes / (1024 * 1024 * 1024);
    QString text;
    text += QString::number(gigBytes, 'f', 1) + QString("GB");

    ui->label_DiskValue->setText(text);
}

void AFResourceExtension::_RefreshMemoryText()
{
    long double num = (long double)m_Statistics->GetMemorySize();

    QString str = QString::number(num, 'f', 1) + QStringLiteral("MB");
    ui->label_MemoryValue->setText(str);
}

void AFResourceExtension::_RefreshNetworkText()
{
    int network = m_Statistics->GetNetworkState();
    QString str = QString::number(network) + QStringLiteral("%");
    ui->label_NetworkValue->setText(str);
}