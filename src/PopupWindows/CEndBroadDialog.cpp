#include "CEndBroadDialog.h"
#include "ui_end-broad-dialog.h"

#include <QDialogButtonBox>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "UIComponent/CMessageBox.h"

AFQEndBroadDialog::AFQEndBroadDialog(QWidget *parent, bool studioEnd) :
    AFTTopBaseDialog(parent),
    ui(new Ui::AFQEndBroadDialog)
{
    ui->setupUi(this);

#ifdef _WIN32
#elif __APPLE__
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);
    ui->widget_Top->hide();
#endif

    m_isStudioEnd = studioEnd;
    if (m_isStudioEnd)
    {
#ifdef _WIN32
        ui->label_EndBroadTitle->setText(QTStr("Output.EndStudioOnBroad.Title"));
#elif __APPLE__
        setWindowTitle(QTStr("Output.EndStudioOnBroad.Title"));
#endif
        ui->label_EndBroadInfoTitle->setText(QTStr("Output.EndStudioOnBroad.Info"));
    }

    ui->pushButton_Cancel->setProperty("pushButtonTheme", "type4");

    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);

    AFQBlockManager::ApplyMoveInAllArea(this);
}

AFQEndBroadDialog::~AFQEndBroadDialog()
{
    delete ui;
}

void AFQEndBroadDialog::qslotRefreshWaitTime(int t1, QString t2)
{
    UNUSED_PARAMETER(t1);
    UNUSED_PARAMETER(t2);

    int broadWaitTimeType = AUTH_CONTEXT.GetSoopBroadInfo()->BroadWaitingTime() / 10;
    ui->comboBox_EndBroadDelayTime->setCurrentIndex(broadWaitTimeType);
}

void AFQEndBroadDialog::qslotWaitTimeTimerTimeout()
{
    ResetBroadEndUI();

    accept();
}

void AFQEndBroadDialog::qslotBroadCloseAPIResponsed(const QByteArray& responseData)
{
    ResetBroadEndUI();

    accept();
}
//
void AFQEndBroadDialog::EndBroadInfoInit(bool ReplayAvailable)
{
    m_waitTimeTimer = new QTimer(this);
    m_waitTimeTimer->setSingleShot(true);
    m_waitTimeTimer->setInterval(1500);
    connect(m_waitTimeTimer, &QTimer::timeout, this, &AFQEndBroadDialog::qslotWaitTimeTimerTimeout);

    if (m_wheelMovie == nullptr)
    {
        std::string absPath;
        GetDataFilePath("assets", absPath);
        QString gifPath = QString("%1/mainview/broad-spinner-black.gif").arg(absPath.data());
        m_wheelMovie = new QMovie(gifPath, QByteArray(), this);
    }
    QSize s = ui->pushButton_EndBroad->rect().size();
    connect(m_wheelMovie, &QMovie::frameChanged, [=] {
        ui->pushButton_EndBroad->setIcon(m_wheelMovie->currentPixmap());
        ui->pushButton_EndBroad->setIconSize(s);
        });

    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    if (!broadInfo) {
        return;
    }

	int broadWaitTimeType = broadInfo->BroadWaitingTime() / 10;
    m_replayAvailable = ReplayAvailable;
    ui->comboBox_EndBroadDelayTime->addItem(QTStr("None"));

    QString min = QTStr("Minutes");

    if (broadWaitTimeType < 0 || broadWaitTimeType > 2) {
        broadWaitTimeType = 0;
    }

    ui->comboBox_EndBroadDelayTime->addItem(QString::number(10) + min);
    ui->comboBox_EndBroadDelayTime->addItem(QString::number(20) + min);

    ui->comboBox_EndBroadDelayTime->setCurrentIndex(broadWaitTimeType);

    std::string locale = LOCALE_CONTEXT.GetCurrentLocaleStr();

    if (locale == "ko-KR")
        ui->widget_EndBroad->setFixedHeight(80);
    else
        ui->widget_EndBroad->setFixedHeight(130);

    if (m_replayAvailable)
    {
        setFixedSize(480, 670);

        std::string broadTitle = broadInfo->Title();
        ui->lineEdit_EndBroadReplay->setText(QString::fromStdString(broadTitle));

        if (!broadInfo->SubscribeBroad())
        {
            ui->widget_SubscribeBroadSaveVodTip->hide();
            setFixedSize(480, 640);
        }
    }
    else
    {
        setFixedSize(480, 500);

        ui->widget_EndBroadReplay->hide();
        ui->widget_SubscribeBroadSaveVodTip->hide();
    }

    if (MAINFRAME->IsSmallResolution())
    {
        if(height() > 550)
            setFixedHeight(550);
    }

    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQEndBroadDialog::reject);
    ui->pushButton_Close->setProperty("buttonType", "closeButton");

    connect(ui->pushButton_EndBroad, &QPushButton::clicked, this, &AFQEndBroadDialog::_qslotEndBroadButtonClicked);
    connect(ui->pushButton_Cancel, &QPushButton::clicked, this, &AFQEndBroadDialog::reject);

    ui->pushButton_ReplayException->SetExplanationText(QTStr("Output.EndBroad.VodExceptionTooltip"));
    ui->pushButton_ReplayException->setProperty("buttonType", "questionmarkButton");
    PolishStyleSheet(ui->pushButton_ReplayException);
}
void AFQEndBroadDialog::ResetBroadEndUI()
{
    m_waitTimeTimer->stop();
    m_wheelMovie->stop();

    ui->pushButton_EndBroad->setText(QTStr("Exit"));
    ui->pushButton_EndBroad->setEnabled(true);
    ui->pushButton_EndBroad->setIcon(QIcon());
}
//
void AFQEndBroadDialog::_qslotEndBroadButtonClicked()
{
    m_broadWaitTime = ui->comboBox_EndBroadDelayTime->currentIndex() * 10;

    std::string responseData;
    AFQBroadInfo* soopBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    if(soopBroadInfo) {

        ui->pushButton_EndBroad->setEnabled(false);
        m_wheelMovie->start();

        if(m_replayAvailable)
        {
            if(ui->lineEdit_EndBroadReplay->text() == "") {
                QString missing = QTStr("Need.Value").arg(QTStr("Title"));
                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                    "", missing, false, true);

                ui->pushButton_EndBroad->setEnabled(true);
                ui->pushButton_EndBroad->setIcon(QPixmap());
                m_wheelMovie->stop();
                return;
            }

            std::string prevBroadTitle = soopBroadInfo->Title();
            QString vodTitle = ui->lineEdit_EndBroadReplay->text();
            if(0 != prevBroadTitle.compare(vodTitle.toStdString()))
            {
                std::string userId;
                AUTH_CONTEXT.GetChannelID(PLATFORM_SOOP, userId);

                QList<QVariant> queryValues = { };
                responseData = SOOP_API_HANDLER->getAPIfromId(GET_BROADTITLE_ENABLE, queryValues);
            }

            const int isWaitBroad = (m_broadWaitTime != 0 ? 1 : 0);
            QList<QVariant> values = { };
            bool request = SOOP_API_HANDLER->postAPIfromId(POST_KR_CLOSE_REQUEST, values, this, "qslotBroadCloseAPIResponsed", {}, true);
            if(!request) {
                accept();
                return;
            }
        }

        m_waitTimeTimer->start();
    }
}

void AFQEndBroadDialog::showEvent(QShowEvent* event)
{
    resize(width(), height() + 1);

    QRect midRect = MAIN_BLOCKMANAGER->GetMidGeometry(this->size());
    QRect adjustRect;
    MAIN_BLOCKMANAGER->AdjustPositionOutSideFullScreen(midRect, adjustRect);

    move(adjustRect.x(), adjustRect.y());
}