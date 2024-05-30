#include "CStatFrame.h"
#include "ui_stat-frame.h"
#include "Application/CApplication.h"
#include "qt-wrapper.h"

#define REC_TIME_LEFT_INTERVAL 30000
#define SPACE_TEXT ": "
static QString MakeMissedFramesText(uint32_t total_lagged,
    uint32_t total_rendered, long double num)
{
    return QString("%1 / %2 (%3%)")
        .arg(QString::number(total_lagged),
            QString::number(total_rendered),
            QString::number(num, 'f', 1));
}

AFQStatWidget::AFQStatWidget(QWidget* parent, Qt::WindowFlags flag) :
    AFCQMainBaseWidget(parent, flag),
    ui(new Ui::AFQStatWidget)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, true);
    installEventFilter(this);
    setWindowTitle(QTStr("Basic.Stats"));

    m_Statistics = App()->GetStatistics();
    connect(m_Statistics, &AFStatistics::qsignalNetworkState, this, &AFQStatWidget::qslotNetworkState);
    connect(m_Statistics, &AFStatistics::qsignalCPUState, this, &AFQStatWidget::qslotCPUState);
    connect(m_Statistics, &AFStatistics::qsignalDiskState, this, &AFQStatWidget::qslotDiskState);
    connect(m_Statistics, &AFStatistics::qsignalMemoryState, this, &AFQStatWidget::qslotMemoryState);
    //connect(m_Statistics, &AFStatistics::qsignalFPSState, this, &AFQStatWidget::qslotFPSState);
    //connect(m_Statistics, &AFStatistics::qsignalRenderTimeState, this, &AFQStatWidget::qslotRenderTimeState);
    connect(m_Statistics, &AFStatistics::qsignalSkippedFrameState, this, &AFQStatWidget::qslotSkippedFrameState);
    connect(m_Statistics, &AFStatistics::qsignalLaggedFrameState, this, &AFQStatWidget::qslotLaggedFrameState);

    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQStatWidget::close);
    connect(ui->pushButton_CloseBtn, &QPushButton::clicked, this, &AFQStatWidget::close);
    connect(ui->pushButton_Reset, &QPushButton::clicked, [this]() { qslotReset(); });
    connect(App()->GetMainView(), &AFMainFrame::qsignalRefreshTimerTick,
            this, &AFQStatWidget::UpdateState);

    UpdateState();
    UpdateStateIcon();

    //OBSBasic* main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());
    //const char* geometry =
    //    config_get_string(main->Config(), "Stats", "geometry");
    //if (geometry != NULL) {
    //    QByteArray byteArray =
    //        QByteArray::fromBase64(QByteArray(geometry));
    //    restoreGeometry(byteArray);

    //    QRect windowGeometry = normalGeometry();
    //    if (!WindowPositionValid(windowGeometry)) {
    //        QRect rect =
    //            QGuiApplication::primaryScreen()->geometry();
    //        setGeometry(QStyle::alignedRect(Qt::LeftToRight,
    //            Qt::AlignCenter, size(),
    //            rect));
    //    }
    //}
    //obs_frontend_add_event_callback(OBSFrontendEvent, this);

    _ChangeLanguage();

    this->SetHeightFixed(true);
    this->SetWidthFixed(true);
}

AFQStatWidget::~AFQStatWidget()
{
    delete ui;
}

void AFQStatWidget::qslotReset()
{
    m_Statistics->Reset();

    UpdateState();
    UpdateStateIcon();
}

void AFQStatWidget::qslotNetworkState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_NetworkState, state);

    if (state == PCStatState::None)
        ui->label_NetworkValue->setText("Stand By");
    else if (state == PCStatState::Normal)
        ui->label_NetworkValue->setText("Good");
    else
        ui->label_NetworkValue->setText("Bad");
}

void AFQStatWidget::qslotCPUState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_CPUState, state);
}

void AFQStatWidget::qslotDiskState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_DiskState, state);
}

void AFQStatWidget::qslotMemoryState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_MemoryState, state);
}

void AFQStatWidget::qslotSkippedFrameState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_SkippedFramesState, state);
}

void AFQStatWidget::qslotLaggedFrameState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_MissedFramesState, state);
}

void AFQStatWidget::UpdateStateIcon()
{
    // Init State Icon
    PCStatState curState = m_Statistics->GetNetworkIconState();
    qslotNetworkState(curState);
    curState = m_Statistics->GetCPUIconState();
    qslotCPUState(curState);
    curState = m_Statistics->GetDiskIconState();
    qslotDiskState(curState);
    curState = m_Statistics->GetMemoryIconState();
    qslotMemoryState(curState);
    curState = m_Statistics->GetSkippedFrameIconState();
    qslotSkippedFrameState(curState);
    curState = m_Statistics->GetLaggedFrameIconState();
    qslotLaggedFrameState(curState);
}

void AFQStatWidget::UpdateState()
{
    if (!m_Statistics)
        return;

    OBSOutputAutoRelease strOutput = obs_frontend_get_streaming_output();
    OBSOutputAutoRelease recOutput = obs_frontend_get_recording_output();

    _RefreshFPSText();
    _RefreshCPUText();
    _RefreshDiskText();
    _RefreshMemoryText();
    _RefreshRenderingAvgTimeText();
    _RefreshSkippedFrameText();
    _RefreshLaggedFrameText();
    _RefreshStreamResourceText();
    _RefreshRecResourceText();
}

void AFQStatWidget::_ChangeLanguage()
{
    QList<QLabel*> labelList = findChildren<QLabel*>();

    foreach(QLabel * label, labelList)
    {
        label->setText(QTStr(label->text().toUtf8().constData()));
    }
}

void AFQStatWidget::_RefreshNetworkText()
{
    int network = m_Statistics->GetNetworkState();
    QString str = QString::number(network) + QStringLiteral("%");
    ui->label_NetworkValue->setText(str);
}

void AFQStatWidget::_RefreshFPSText()
{
    double curFPS = m_Statistics->GetCurFPS();

    QString str = QString::number(curFPS, 'f', 2);
    str = SPACE_TEXT + str;
    ui->label_FPSValue->setText(str);
}

void AFQStatWidget::_RefreshCPUText()
{
    double usage = m_Statistics->GetCPUUsage();
    QString str = QString::number(usage, 'f', 1) + QStringLiteral("%");
    str = SPACE_TEXT + str;
    ui->label_CPUValue->setText(str);
}

void AFQStatWidget::_RefreshDiskText()
{
#define MBYTE (1024ULL * 1024ULL)
#define GBYTE (1024ULL * 1024ULL * 1024ULL)
#define TBYTE (1024ULL * 1024ULL * 1024ULL * 1024ULL)

    uint64_t numBytes = m_Statistics->GetDiskSize();
    QString abrv = QStringLiteral(" MB");
    long double num;

    num = (long double)numBytes / (1024.0l * 1024.0l);
    if (numBytes > TBYTE) {
        num /= 1024.0l * 1024.0l;
        abrv = QStringLiteral(" TB");
    }
    else if (numBytes > GBYTE) {
        num /= 1024.0l;
        abrv = QStringLiteral(" GB");
    }

    QString str = QString::number(num, 'f', 1) + abrv;
    str = SPACE_TEXT + str;
    ui->label_DiskVaue->setText(str);
}

void AFQStatWidget::_RefreshMemoryText()
{
    long double num = m_Statistics->GetMemorySize();

    QString str = QString::number(num, 'f', 1) + QStringLiteral(" MB");
    str = SPACE_TEXT + str;
    ui->label_MemoryValue->setText(str);
}

void AFQStatWidget::_RefreshRenderingAvgTimeText()
{
    long double num = m_Statistics->GetOBSAvgFrameTime();

    QString str = QString::number(num, 'f', 1) + QStringLiteral(" ms");
    str = SPACE_TEXT + str;
    ui->label_FrameRenderingAvgValue->setText(str);
}

void AFQStatWidget::_RefreshSkippedFrameText()
{
    uint32_t totalEncoded = m_Statistics->GetTotalEncoded();
    uint32_t totalSkipped = m_Statistics->GetTotalSkipped();
    long double skippedRate = m_Statistics->GetSkippedFrameRate();

    QString str = QString("%1 / %2 (%3%)")
        .arg(QString::number(totalSkipped),
            QString::number(totalEncoded),
            QString::number(skippedRate, 'f', 1));
    str = SPACE_TEXT + str;

    ui->label_SkippedFramesValue->setText(str);
}

void AFQStatWidget::_RefreshLaggedFrameText()
{
    uint32_t totalRendered = m_Statistics->GetTotalRendered();
    uint32_t totalLagged = m_Statistics->GetTotalLagged();
    long double laggedRate = m_Statistics->GetLaggedFrameRate();

    QString str = MakeMissedFramesText(totalLagged, totalRendered, laggedRate);
    str = SPACE_TEXT + str;
    ui->label_MissedFramesValue->setText(str);
}

void AFQStatWidget::_RefreshStreamResourceText()
{
    long double num = m_Statistics->GetStreamMegabytesSent();
    long double kbps = m_Statistics->GetStreamBitrate();

    QString str = QString("%1 MB").arg(QString::number(num, 'f', 1));
    str = SPACE_TEXT + str;
    ui->label_StreamMegabytesSentValue->setText(str);
 
    str = QString("%1 kb/s").arg(QString::number(kbps, 'f', 0));
    str = SPACE_TEXT + str;
    ui->label_StreamBitrateValue->setText(str);
 
    int total = m_Statistics->GetStreamTotalFrame();
    int dropped = m_Statistics->GetStreamDroppedFrame();
    num = m_Statistics->GetStreamDroppedFrameRate();

    str = QString("%1 / %2 (%3%)")
        .arg(QString::number(dropped),
            QString::number(total),
            QString::number(num, 'f', 1));
    str = SPACE_TEXT + str;
    ui->label_StreamDroppedFramesValue->setText(str);
}

void AFQStatWidget::_RefreshRecResourceText()
{
    long double num = m_Statistics->GetRecMegabytesSent();
    long double kbps = m_Statistics->GetRecBitrate();

    QString str = QString("%1 MB").arg(QString::number(num, 'f', 1));
    str = SPACE_TEXT + str;
    ui->label_RecMegabytesSentValue->setText(str);

    str = QString("%1 kb/s").arg(QString::number(kbps, 'f', 0));
    str = SPACE_TEXT + str;
    ui->label_RecBitrateValue->setText(str);
}