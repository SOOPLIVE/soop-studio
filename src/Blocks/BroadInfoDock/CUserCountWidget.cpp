#include "CUserCountWidget.h"
#include "ui_user-count-widget.h"

#include "Common/StudioDefine.h"

#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"

#include "MainFrame/CMainFrame.h"

AFUserCountWidget::AFUserCountWidget(QWidget* parent)
	: QWidget(parent),
	ui(new Ui::AFUserCountWidget)
{
	ui->setupUi(this);

	setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setAttribute(Qt::WA_DeleteOnClose, true);

	_Init();
}

AFUserCountWidget::~AFUserCountWidget() 
{
	if (m_showUserCountChanged) {
		config_set_bool(USERCONFIG, "BroadInfo", "HideUserCount", ui->pushButton_HideUserCount->isChecked());
		config_save_safe(USERCONFIG, "tmp", nullptr);
	}

	delete ui;
}

void AFUserCountWidget::_qslotClickedHideUserCount() 
{
	m_showUserCountChanged = true;

	bool showUserCount = ui->pushButton_HideUserCount->isChecked();
	emit qsignalHideUserCountChanged(showUserCount);
}

void AFUserCountWidget::qslotRefreshViewer(int result, QString msg)
{
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	QString info = QString::number(broadInfo->CurrentViewer());
	ui->label_MainBroadUserCount->setText(info);

	info = QString::number(broadInfo->PcViewer());
	ui->label_MainBroadPCUserCount->setText(info);

	info = QString::number(broadInfo->MobileViewer());
	ui->label_MainBroadMobileUserCount->setText(info);

	info = QString::number(broadInfo->RelayViewer());
	ui->label_RelayBroadUserCount->setText(info);

	info = QString::number(broadInfo->RelayPcViewer());
	ui->label_RelayBroadPCUserCount->setText(info);

	info = QString::number(broadInfo->RelayMobileViewer());
	ui->label_RelayBroadMobileUserCount->setText(info);

	info = QString::number(broadInfo->RelayCount());
	ui->label_OpenedRelayBroadCount->setText(info);

	info = QString::number(broadInfo->AccumulateViewer());
	ui->label_AccumulationUserCount->setText(info);
}

void AFUserCountWidget::_qslotClickedStatistics()
{
	QString url = QString::fromStdString(SOOP_BROAD_STATISTICS_URL);
	MAINFRAME->NavigateDefaultBrowser(url);
}

void AFUserCountWidget::_Init()
{
	// Apply Broad Info To UI
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	ui->pushButton_HideUserCount->SetChecked(config_get_bool(USERCONFIG, "BroadInfo", "HideUserCount"));
	
	ui->widget_RelayRoomContents->setVisible(broadInfo->RelayCount() > 0 ? true : false);
	adjustSize();

	// Explanation Text
	ui->pushButton_HideUserCount->setToolTip(QTStr("UserCountDetails.HideUserCount.ToolTip"));

	// Connect Signals
	connect(ui->pushButton_Close, &QPushButton::clicked, 
			this, &AFUserCountWidget::close);
	connect(ui->pushButton_HideUserCount, &QPushButton::clicked, 
			this, &AFUserCountWidget::_qslotClickedHideUserCount);
	connect(ui->pushButton_Statistics, &QPushButton::clicked, 
			this, &AFUserCountWidget::_qslotClickedStatistics);

	connect(broadInfo, &AFQBroadInfo::qsignalViewerResult, this, &AFUserCountWidget::qslotRefreshViewer);

	qslotRefreshViewer(1, "");
}