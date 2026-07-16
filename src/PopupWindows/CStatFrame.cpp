#include "CStatFrame.h"
#include "ui_stat-frame.h"

#include "Application/CApplication.h"

#include "CoreModel/Statistics/CStatistics.h"


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
    QWidget(parent, flag),
    ui(new Ui::AFQStatWidget)
{
    ui->setupUi(this);

    installEventFilter(this);
    setWindowTitle(QTStr("Basic.Stats"));

    auto& statistics = STATISTICS;
    //
    connect(&statistics, &AFStatistics::qsignalNetworkState, this, &AFQStatWidget::qslotNetworkState);
    connect(&statistics, &AFStatistics::qsignalCPUState, this, &AFQStatWidget::qslotCPUState);
    connect(&statistics, &AFStatistics::qsignalDiskState, this, &AFQStatWidget::qslotDiskState);
    connect(&statistics, &AFStatistics::qsignalMemoryState, this, &AFQStatWidget::qslotMemoryState);
    connect(&statistics, &AFStatistics::qsignalFPSState, this, &AFQStatWidget::qslotFPSState);
    connect(&statistics, &AFStatistics::qsignalRenderTimeState, this, &AFQStatWidget::qslotRenderTimeState);
    connect(&statistics, &AFStatistics::qsignalSkippedFrameState, this, &AFQStatWidget::qslotSkippedFrameState);
    connect(&statistics, &AFStatistics::qsignalLaggedFrameState, this, &AFQStatWidget::qslotLaggedFrameState);

    connect(ui->pushButton_CloseBtn, &QPushButton::clicked, this, &AFQStatWidget::qslotCloseButtonTriggered);
    connect(ui->pushButton_Reset, &QPushButton::clicked, [this]() { qslotReset(); });
    connect(MAINFRAME, &AFMainFrame::qsignalRefreshTimerTick, this, &AFQStatWidget::UpdateState);

    UpdateState();
    UpdateStateIcon();

    if (MAINFRAME->IsSmallResolution())
    {
        ui->buttonBoxFrame->setMinimumHeight(0);
        ui->buttonBoxFrame->setMaximumHeight(QWIDGETSIZE_MAX);
    }

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
}

AFQStatWidget::~AFQStatWidget()
{
    delete ui;
}

void AFQStatWidget::qslotReset()
{
    STATISTICS.Reset();

    UpdateState();
    UpdateStateIcon();
}

void AFQStatWidget::qslotNetworkState(PCStatState state) 
{
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

void AFQStatWidget::qslotFPSState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_FPSState, state);
}

void AFQStatWidget::qslotRenderTimeState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_RenderTimeSState, state);
}

void AFQStatWidget::qslotSkippedFrameState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_SkippedFramesState, state);
}

void AFQStatWidget::qslotLaggedFrameState(PCStatState state) {
    AFMainFrame::SetPCStateIconStyle(ui->label_MissedFramesState, state);
}

void AFQStatWidget::qslotCloseButtonTriggered()
{
    emit qsignalCloseTriggered(ENUM_WINDOW_TYPE::StatPage);
}

void AFQStatWidget::UpdateStateIcon()
{
    auto& statistics = STATISTICS;
    //
    // Init State Icon
    PCStatState curState = statistics.GetNetworkIconState();
    qslotNetworkState(curState);
    curState = statistics.GetCPUIconState();
    qslotCPUState(curState);
    curState = statistics.GetDiskIconState();
    qslotDiskState(curState);
    curState = statistics.GetMemoryIconState();
    qslotMemoryState(curState);
    curState = statistics.GetSkippedFrameIconState();
    qslotSkippedFrameState(curState);
    curState = statistics.GetLaggedFrameIconState();
    qslotLaggedFrameState(curState);
}

void AFQStatWidget::UpdateState()
{
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
    int network = STATISTICS.GetNetworkState();
    QString str = QString::number(network) + QStringLiteral("%");
    ui->label_NetworkValue->setText(str);
}

void AFQStatWidget::_RefreshFPSText()
{
    struct obs_video_info ovi = {};
    obs_get_video_info(&ovi);
    double obsFPS = (double)ovi.fps_num / (double)ovi.fps_den;

    double fps = STATISTICS.GetCurFPS();
    QString str = QString("%1 / %2").arg(QString::number(fps, 'f', 2)).arg(QString::number(obsFPS, 'f', 2));
    str = SPACE_TEXT + str;
    ui->label_FPSValue->setText(str);
}

void AFQStatWidget::_RefreshCPUText()
{
    double usage = STATISTICS.GetCPUUsage();
    QString str = QString::number(usage, 'f', 1) + QStringLiteral("%");
    str = SPACE_TEXT + str;
    ui->label_CPUValue->setText(str);
}

void AFQStatWidget::_RefreshDiskText()
{
#define MBYTE (1024ULL * 1024ULL)
#define GBYTE (1024ULL * 1024ULL * 1024ULL)
#define TBYTE (1024ULL * 1024ULL * 1024ULL * 1024ULL)

    uint64_t numBytes = STATISTICS.GetDiskSize();
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
    long double num = STATISTICS.GetMemorySize();

    QString str = QString::number(num, 'f', 1) + QStringLiteral(" MB");
    str = SPACE_TEXT + str;
    ui->label_MemoryValue->setText(str);
}

void AFQStatWidget::_RefreshRenderingAvgTimeText()
{
    long double num = STATISTICS.GetOBSAvgFrameTime();

    QString str = QString::number(num, 'f', 1) + QStringLiteral(" ms");
    str = SPACE_TEXT + str;
    ui->label_FrameRenderingAvgValue->setText(str);
}

void AFQStatWidget::_RefreshSkippedFrameText()
{
    auto& statistics = STATISTICS;
    //
    uint32_t totalEncoded = statistics.GetTotalEncoded();
    uint32_t totalSkipped = statistics.GetTotalSkipped();
    long double skippedRate = statistics.GetSkippedFrameRate();

    QString str = QString("%1 / %2 (%3%)")
        .arg(QString::number(totalSkipped),
            QString::number(totalEncoded),
            QString::number(skippedRate, 'f', 1));
    str = SPACE_TEXT + str;

    ui->label_SkippedFramesValue->setText(str);
}

void AFQStatWidget::_RefreshLaggedFrameText()
{
    auto& statistics = STATISTICS;
    //
    uint32_t totalRendered = statistics.GetTotalRendered();
    uint32_t totalLagged = statistics.GetTotalLagged();
    long double laggedRate = statistics.GetLaggedFrameRate();

    QString str = MakeMissedFramesText(totalLagged, totalRendered, laggedRate);
    str = SPACE_TEXT + str;
    ui->label_MissedFramesValue->setText(str);
}

void AFQStatWidget::_RefreshStreamResourceText()
{
    auto& statistics = STATISTICS;
    //
    long double num = statistics.GetStreamMegabytesSent();
    long double kbps = statistics.GetStreamBitrate();

    QString str = QString("%1 MB").arg(QString::number(num, 'f', 1));
    str = SPACE_TEXT + str;
    ui->label_StreamMegabytesSentValue->setText(str);
 
    str = QString("%1 kb/s").arg(QString::number(kbps, 'f', 0));
    str = SPACE_TEXT + str;
    ui->label_StreamBitrateValue->setText(str);
 
    int total = statistics.GetStreamTotalFrame();
    int dropped = statistics.GetStreamDroppedFrame();
    num = statistics.GetStreamDroppedFrameRate();

    str = QString("%1 / %2 (%3%)")
        .arg(QString::number(dropped),
            QString::number(total),
            QString::number(num, 'f', 1));
    str = SPACE_TEXT + str;
    ui->label_StreamDroppedFramesValue->setText(str);
}

void AFQStatWidget::_RefreshRecResourceText()
{
    auto& statistics = STATISTICS;
    //
    long double num = statistics.GetRecMegabytesSent();
    long double kbps = statistics.GetRecBitrate();

    QString str = QString("%1 MB").arg(QString::number(num, 'f', 1));
    str = SPACE_TEXT + str;
    ui->label_RecMegabytesSentValue->setText(str);

    str = QString("%1 kb/s").arg(QString::number(kbps, 'f', 0));
    str = SPACE_TEXT + str;
    ui->label_RecBitrateValue->setText(str);
}