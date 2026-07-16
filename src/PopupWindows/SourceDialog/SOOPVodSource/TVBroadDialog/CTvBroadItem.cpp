#include "CTvBroadItem.h"
#include "ui_tv-broad-item.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

#include "MainFrame/CMainFrame.h"

AFQTvBroadItem::AFQTvBroadItem(QWidget* parent) :
	QFrame(parent),
	ui(new Ui::AFQTvBroadItem)
{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover, true);

	ui->setupUi(this);

	ui->pushButton_Pause->hide();

	connect(ui->pushButton_Pause, &QPushButton::clicked, this, &AFQTvBroadItem::_qslotStopButtonClicked);
	connect(ui->pushButton_Play, &QPushButton::clicked, this, &AFQTvBroadItem::_qslotPlayButtonClicked);

	connect(this, &AFQTvBroadItem::qsignalHoverItem, ui->label_Title, &AFQElidedSlideLabel::qslotHoverButton);
	connect(this, &AFQTvBroadItem::qsignalLeaveItem, ui->label_Title, &AFQElidedSlideLabel::qslotLeaveButton);

	SetTvBroadPlayingStatus(false);
}

AFQTvBroadItem::~AFQTvBroadItem()
{
	delete ui;
}

void AFQTvBroadItem::_qslotPlayButtonClicked()
{
	emit qsignalRequestOnetimeUrl(m_cpNo);
}

void AFQTvBroadItem::_qslotStopButtonClicked()
{
	emit qsignalStopTvBroad(m_cpNo);
}

void AFQTvBroadItem::enterEvent(QEnterEvent* event)
{
	emit qsignalHoverItem(QString());

	QFrame::enterEvent(event);
}
void AFQTvBroadItem::leaveEvent(QEvent* event)
{
	emit qsignalLeaveItem();

	QFrame::leaveEvent(event);
}

void AFQTvBroadItem::SetTvBroadInfo(int cpNo, QString cpTitle)
{
	m_cpNo = cpNo;
	ui->label_Title->setText(cpTitle);
}

int AFQTvBroadItem::GetTvBroadCpNo()
{
	return m_cpNo;
}

void AFQTvBroadItem::SetTvBroadPlayingStatus(bool playing)
{
	//if (playing == m_playing)
	//	return;

	if (playing) {
		ui->label_Status->setText(QTStr("Popup.TvLive.OnAir"));
		ui->label_Status->show();
		ui->pushButton_Pause->show();
		ui->pushButton_Play->hide();
	}
	else {
		ui->label_Status->hide();
		ui->pushButton_Play->show();
		ui->pushButton_Pause->hide();
	}

	SetPropAndPolishStyleSheet(ui->frameBroadItem, "playing", playing);
	SetPropAndPolishStyleSheet(ui->label_Title, "playing", playing);
	SetPropAndPolishStyleSheet(ui->label_Status, "status", (playing == true ? 1 : 2 ));

	m_playing = playing;
}