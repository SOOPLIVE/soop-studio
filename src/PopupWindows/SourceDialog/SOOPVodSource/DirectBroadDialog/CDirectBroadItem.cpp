#include "CDirectBroadItem.h"
#include "ui_direct-broad-item.h"

#include "qt-wrappers.hpp"

AFQDirectBroadItem::AFQDirectBroadItem(QWidget* parent) :
	QFrame(parent),
	ui(new Ui::AFQDirectBroadItem)
{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover, true);

	ui->setupUi(this);

	ui->pushButton_Stop->hide();

	connect(ui->pushButton_Play, &QPushButton::clicked, this, &AFQDirectBroadItem::_qslotPlayButtonClicked);
	connect(ui->pushButton_Stop, &QPushButton::clicked, this, &AFQDirectBroadItem::_qslotStopButtonClicked);

	connect(this, &AFQDirectBroadItem::qsignalHoverItem, ui->label_Title, &AFQElidedSlideLabel::qslotHoverButton);
	connect(this, &AFQDirectBroadItem::qsignalLeaveItem, ui->label_Title, &AFQElidedSlideLabel::qslotLeaveButton);
}

AFQDirectBroadItem::~AFQDirectBroadItem()
{
	delete ui;
}

void AFQDirectBroadItem::_qslotPlayButtonClicked()
{
	emit qsignalRequestOnetimeUrl(m_directBroadInfo.idx);
}

void AFQDirectBroadItem::_qslotStopButtonClicked()
{
	emit qsignalStopDirectBroad(m_directBroadInfo.idx);
}

void AFQDirectBroadItem::enterEvent(QEnterEvent* event)
{
	emit qsignalHoverItem(QString());

	QFrame::enterEvent(event);
}
void AFQDirectBroadItem::leaveEvent(QEvent* event)
{
	emit qsignalLeaveItem();

	QFrame::leaveEvent(event);
}

void AFQDirectBroadItem::SetDirectBroadInfo(const DirectBroadInfo_s& info)
{
	m_directBroadInfo = info;

	ui->horizontalSpacer_3->changeSize(8, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);

	QString status;
	switch (m_directBroadInfo.status) {
	case 0: 
		status = QTStr("Popup.DirectBroad.Scheduled");
		ui->horizontalSpacer_3->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
		break;
	//case 1: status = QTStr("Popup.DirectBroad.Possible");  break;
	case 2: 
		status = QTStr("Popup.DirectBroad.Finished"); 
		ui->horizontalSpacer_3->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
		break;
	}

	QString content_info = QString("%1%2").arg(m_directBroadInfo.start_time).arg(m_directBroadInfo.contents);
	ui->label_Status->setText(status);
	ui->label_Title->setText(m_directBroadInfo.title);
	ui->label_Info->setText(content_info);

	SetPropAndPolishStyleSheet(ui->label_Status, "status", m_directBroadInfo.status);
	SetPropAndPolishStyleSheet(ui->label_Title, "status", m_directBroadInfo.status);

	if (1 != m_directBroadInfo.status) {
		ui->pushButton_Play->hide();
		ui->pushButton_Stop->hide();
	}
}

void AFQDirectBroadItem::SetDirectBroadPlayingStatus(bool playing)
{
	if (playing == m_playing)
		return;

	if (playing) {
		ui->label_Status->setText(QTStr("Popup.DirectBroad.OnAir"));
		ui->pushButton_Stop->show();
		ui->pushButton_Play->hide();
	}
	else {
		QString status;
		switch (m_directBroadInfo.status) {
			case 0: status = QTStr("Popup.DirectBroad.Scheduled");  break;
			//case 1: status = QTStr("Popup.DirectBroad.Possible");  break;
			case 2: status = QTStr("Popup.DirectBroad.Finished");  break;
		}
		ui->label_Status->setText(status);
		ui->pushButton_Play->show();
		ui->pushButton_Stop->hide();
	}

	SetPropAndPolishStyleSheet(ui->frameBroadItem, "playing", playing);
	SetPropAndPolishStyleSheet(ui->label_Status, "status", m_directBroadInfo.status);

	m_playing = playing;
}