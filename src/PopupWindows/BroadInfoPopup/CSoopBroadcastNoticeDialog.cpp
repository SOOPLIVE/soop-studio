#include "CSoopBroadcastNoticeDialog.h"
#include "ui_soop-broadcast-notice-dialog.h"

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Config/CConfigManager.h"

AFQSoopBroadcastNoticeDialog::AFQSoopBroadcastNoticeDialog(QWidget* parent) :
    AFTTopBaseDialog(parent),
    ui(new Ui::AFQSoopBroadcastNoticeDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

#ifdef _WIN32
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowTitle(QTStr("Caution.SOOPManner"));
    setWindowFlags(Qt::Dialog|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif
    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);

    _Init();
}

AFQSoopBroadcastNoticeDialog::~AFQSoopBroadcastNoticeDialog()
{
    delete ui;
}

void AFQSoopBroadcastNoticeDialog::_qslotClickedResponsibilityCheckBox(bool checked)
{
    ui->pushButton_Ok->setEnabled(checked);
}

void AFQSoopBroadcastNoticeDialog::_qslotClickedOkButtonBox()
{
    if (ui->checkBox_HideForWeek->isChecked())
    {
        // Save Date
        QDate currentDate = QDateTime::currentDateTime().date();
        currentDate = currentDate.addDays(7);

        QString strCurrentDate = currentDate.toString("yyyy-MM-dd");
        config_set_string(APPCONFIG, "BroadInfo", SOOP_BROADCAST_NOTICE_DATE_CHECK, QT_TO_UTF8(strCurrentDate));
        config_save_safe(APPCONFIG, "tmp", nullptr);
    }

    accept();
    close();
}

void AFQSoopBroadcastNoticeDialog::showEvent(QShowEvent* event)
{
    QRect midRect = MAIN_BLOCKMANAGER->GetMidGeometry(this->size());
    QRect adjustRect;
    MAIN_BLOCKMANAGER->AdjustPositionOutSideFullScreen(midRect, adjustRect);

    move(adjustRect.x(), adjustRect.y());
}

void AFQSoopBroadcastNoticeDialog::_Init()
{
    connect(ui->checkBox_Responsibility, &QCheckBox::clicked,
        this, &AFQSoopBroadcastNoticeDialog::_qslotClickedResponsibilityCheckBox);
    connect(ui->pushButton_Close, &QPushButton::clicked,
        this, &AFQSoopBroadcastNoticeDialog::close);
    connect(ui->pushButton_Ok, &QPushButton::clicked,
        this, &AFQSoopBroadcastNoticeDialog::_qslotClickedOkButtonBox);

    ui->pushButton_Ok->setEnabled(false);
}
