#include "CUserUpWidget.h"
#include "ui_user-up-widget.h"

#include "Common/StudioDefine.h"

#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"

AFUserUpWidget::AFUserUpWidget(QWidget* parent)
	: QWidget(parent),
	ui(new Ui::AFUserUpWidget)
{
	ui->setupUi(this);

	setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setAttribute(Qt::WA_DeleteOnClose, true);

	_Init();
}

AFUserUpWidget::~AFUserUpWidget()
{
	delete ui;
}

void AFUserUpWidget::qslotRefreshUpInfo(int result, QString msg)
{
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	QString nickLists = broadInfo->UpNickname();
	if (nickLists.isEmpty())
		return;

	QStringList splitNickLists = nickLists.split(',');

	QString recentlyUpLists;
	for (int i = 0; i < splitNickLists.size(); ++i) {
		if (0 != i) {
			recentlyUpLists += ", ";
		}
		if (0 != i && i % 3 == 0) {
			recentlyUpLists += "\n";
		}
		recentlyUpLists += splitNickLists[i];
	}

	ui->label_UpUserLists->setText(recentlyUpLists);

	adjustSize();
}

void AFUserUpWidget::_Init()
{
	// Apply Broad Info To UI
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();

	// Connect Signals
	connect(ui->pushButton_Close, &QPushButton::clicked,
		this, &AFUserUpWidget::close);

	connect(broadInfo, &AFQBroadInfo::qsignalUpResult, this, &AFUserUpWidget::qslotRefreshUpInfo);

	qslotRefreshUpInfo(1, "");
}