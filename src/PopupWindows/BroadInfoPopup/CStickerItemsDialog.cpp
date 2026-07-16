#include "CStickerItemsDialog.h"
#include "ui_sticker-items-dialog.h"

#include "Common/StudioDefine.h"

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "Utils/SOOPAPIHandler.h"

#include "CoreModel/Auth/CAuthManager.h"

#include "PopupWindows/CEmptyDialog.h"


AFQStickerItemsDialog::AFQStickerItemsDialog(QWidget* parent) :
    AFTTopBaseDialog(parent),
    ui(new Ui::AFQStickerItemsDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

    AFQBlockManager::ApplyMoveInAllArea(this);
    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);

    _Init();
}

AFQStickerItemsDialog::~AFQStickerItemsDialog() 
{
    delete ui;
}

void AFQStickerItemsDialog::UpdateUI()
{
    if (!ui)
        return;

    AFQBroadInfo* pBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    if (!pBroadInfo) {
        return;
    }

    AFItemInfo itemInfo = pBroadInfo->ItemInfo();

    AFChannelData* pSoopChannel = nullptr;
    AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannel);
    if (!pSoopChannel) {
        blog(LOG_ERROR, "sticker popup - soop channel data is null");
        return;
    }

    if (itemInfo.addManagerEndTime > 0) {
        ui->label_AddManagerInUse->show();
        ui->label_AddManagerNotUse->hide();
        ui->label_AddManagerUsagePeriod->setText(QTStr("StickerItem.Until")
            .arg(_GetFormattedTime(itemInfo.addManagerEndTime)));
    }
    else {
        ui->label_AddManagerInUse->hide();
        ui->label_AddManagerNotUse->show();
        ui->label_AddManagerUsagePeriod->setText("-");
    }

    if (itemInfo.topListEndTime > 0) {
        ui->label_TopListInUse->show();
        ui->label_TopListNotUse->hide();
        ui->label_TopListUsagePeriod->setText(QTStr("StickerItem.Until")
            .arg(_GetFormattedTime(itemInfo.topListEndTime)));
    }
    else {
        ui->label_TopListInUse->hide();
        ui->label_TopListNotUse->show();
        ui->label_TopListUsagePeriod->setText("-");
    }

    if (0 == itemInfo.haveMobileAlarmItem && false == itemInfo.useMobileAlarmItem)
    {
        ui->pushButton_UseMobileNoti->setText("-");
        ui->pushButton_UseMobileNoti->setEnabled(false);
        ui->pushButton_UseMobileNoti->setProperty("itemButton", "noItem");
        PolishStyleSheet(ui->pushButton_UseMobileNoti);
        ui->label_SendMobileNotiUsagePeriod->setText("-");
    }
    else
    {
        ui->pushButton_UseMobileNoti->setText(QTStr("StickerItem.Use"));
        ui->pushButton_UseMobileNoti->setProperty("pushButtonTheme", "type2");
        PolishStyleSheet(ui->pushButton_UseMobileNoti);
        ui->pushButton_UseMobileNoti->setEnabled(true);

        if (itemInfo.useMobileAlarmItem)
        {
            ui->label_SendMobileNotiUsagePeriod->setText(QTStr("StickerItem.Until")
                .arg(_GetFormattedTime(itemInfo.mobileAlarmItemEndTime)));
        }
        else
        {
            ui->label_SendMobileNotiUsagePeriod->setText("-");
        }
    }

    if (false == itemInfo.haveBurningItem && false == itemInfo.usingBurningItem)
    {
        ui->pushButton_UseBurningTen->setText("-");
        ui->pushButton_UseBurningTen->setEnabled(false);
        ui->pushButton_UseBurningTen->setProperty("itemButton", "noItem");
        PolishStyleSheet(ui->pushButton_UseBurningTen);
        ui->label_BurningTenMinutesUsagePeriod->setText("-");
    }
    else
    {
        ui->pushButton_UseBurningTen->setProperty("pushButtonTheme", "type2");
        PolishStyleSheet(ui->pushButton_UseBurningTen);
        ui->pushButton_UseBurningTen->setEnabled(true);

        if (itemInfo.usingBurningItem)
        {
            ui->pushButton_UseBurningTen->setText(QTStr("InUse"));
            ui->label_BurningTenMinutesUsagePeriod->setText(QTStr("StickerItem.Until")
                .arg(_GetFormattedTime(itemInfo.burningItemEndTime)));
        }
        else
        {
            ui->pushButton_UseBurningTen->setText(QTStr("StickerItem.Use"));
            ui->label_BurningTenMinutesUsagePeriod->setText("-");
        }
    }
}

void AFQStickerItemsDialog::closeEvent(QCloseEvent* event)
{
    if (m_pCefWidget)
    {
        static int panel_version = -1;
        if (panel_version == -1)
            panel_version = obs_browser_qcef_version();

        if (panel_version >= 2 && !!m_pCefWidget)
            m_pCefWidget->closeBrowser();
    }

    event->accept();
}

void AFQStickerItemsDialog::_Init() 
{
    ui->label_Title->setText(QTStr("StickerItem.Usage"));
    ui->stackedWidget->setCurrentWidget(ui->page_StickerItemsUsage);

    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQStickerItemsDialog::close);
    connect(ui->widget_ExchangeStickerItem, &AFQHoverWidget::qsignalMouseClick, this, &AFQStickerItemsDialog::_qslotClickedExchangeStickerItemButton);
    connect(ui->pushButton_UseMobileNoti, &QPushButton::clicked, this, &AFQStickerItemsDialog::_qslotClickedSendMobileAlarmButton);
    connect(ui->pushButton_UseBurningTen, &QPushButton::clicked, this, &AFQStickerItemsDialog::_qslotClickedBurnningTenButton);

    UpdateUI();
}

QString AFQStickerItemsDialog::_GetFormattedTime(int addSec)
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime newTime = currentTime.addSecs(addSec);

    QString formattedTime = newTime.toString("yy.MM.dd hh:mm");
    return formattedTime;
}

void AFQStickerItemsDialog::_qslotClickedExchangeStickerItemButton() 
{
    MAINFRAME->NavigateDefaultBrowser(QString::fromStdString(URL_STUDIO_STICKER_POPUP));
}

void AFQStickerItemsDialog::_qslotClickedSendMobileAlarmButton()
{
    AFQBroadInfo* pBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    if (!pBroadInfo) {
        return;
    }

    AFItemInfo itemInfo = pBroadInfo->ItemInfo();
    if (!itemInfo.useMobileAlarmItem) {
        MAINFRAME->NavigateDefaultBrowser(QString::fromStdString(MOBILE_ALARM_USE_PAGE));
    }
    else
    {
        QCefWidget* cefWidget = CEFMANAGER.createWidget(nullptr, MOBILE_ALARM_SEND_PAGE);
        if (cefWidget) {
            AFQEmptyDialog dialog(this);

            connect(cefWidget, SIGNAL(cefQueryRequest(const QCefQuery&)), &dialog, SLOT(qslotQueryRecieved(const QCefQuery&)));

            dialog.setTitle(QTStr("StickerItem.SendMobileNoti"));
            dialog.setFixedSize(500, 560);
            dialog.addWidget(cefWidget);
            dialog.exec();
        }
    }
}

void AFQStickerItemsDialog::_qslotClickedBurnningTenButton()
{
    AFQBroadInfo* pBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    if (!pBroadInfo) {
        return;
    }

    AFItemInfo itemInfo = pBroadInfo->ItemInfo();

    if (!itemInfo.haveBurningItem || itemInfo.burningItemKey == 0) {
        blog(LOG_ERROR, "burnning item not have");
        return;
    }

    int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this, "",  QTStr("StickerItem.BurnningTen.Use"), true, true);

    if (result == QDialog::Accepted)
    {
        QList<QVariant> values = { };
        SOOP_API_HANDLER->getAPIfromId(GET_USE_BURNNINGTEN_ITEM, values, this, "_qslotResponseUseBurnningTenAPI");
    }
}

void AFQStickerItemsDialog::_qslotResponseUseBurnningTenAPI(const QByteArray& responseData)
{
    std::string jsonString = responseData.toStdString();
    std::string err;
}

void AFQStickerItemsDialog::_qslotDataReceivedSticker(const QCefQuery& query)
{
    std::string err;
}
