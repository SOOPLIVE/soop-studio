#include "CVodContentWidget.h"
#include "ui_vod-content-widget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPushButton>

#include "ViewModel/Auth/Soop/auth-soop.hpp"

#include "MainFrame/CMainFrame.h"

AFQVodContentWidget::AFQVodContentWidget(QWidget* parent, SOOP_VOD_TYPE type) :
	QFrame(parent),
	ui(new Ui::AFQVodContentWidget)

{
	setMouseTracking(true);
	setAttribute(Qt::WA_Hover, true);

	ui->setupUi(this);

	connect(ui->pushButton_GoStation, &QPushButton::clicked,
		this, &AFQVodContentWidget::qslotGoChannelButtonClicked);

	connect(this, &AFQVodContentWidget::qsignalHoverItem,
		ui->label_Info, &AFQElidedSlideLabel::qslotHoverButton);

	connect(this, &AFQVodContentWidget::qsignalLeaveItem,
		ui->label_Info, &AFQElidedSlideLabel::qslotLeaveButton);

	connect(this, &AFQVodContentWidget::qsignalHoverItem,
		ui->label_Title, &AFQElidedSlideLabel::qslotHoverButton);

	connect(this, &AFQVodContentWidget::qsignalLeaveItem,
		ui->label_Title, &AFQElidedSlideLabel::qslotLeaveButton);

	if (SOOP_VOD_TYPE::SPORT == type) {
		ui->label_Thumbnail->hide();
		ui->label_Info->hide();
	}
}

AFQVodContentWidget::~AFQVodContentWidget()
{
	delete ui;
}

void AFQVodContentWidget::mousePressEvent(QMouseEvent* event) 
{
	emit qsignalContentItemClicked(m_contentIdx);
}

void AFQVodContentWidget::enterEvent(QEnterEvent* event)
{
	emit qsignalHoverItem(QString());

	QFrame::enterEvent(event);
}

void AFQVodContentWidget::leaveEvent(QEvent* event)
{
	emit qsignalLeaveItem();

	QFrame::leaveEvent(event);
}

void AFQVodContentWidget::qslotImageDownloaded(QByteArray responseData)
{
	QPixmap pixmap;
	pixmap.loadFromData(responseData);

	ui->label_Thumbnail->setPixmap(pixmap.scaled(ui->label_Thumbnail->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}

void AFQVodContentWidget::qslotGoChannelButtonClicked()
{
	MAINFRAME->NavigateDefaultBrowser(m_vodChannelUrl);
}

void AFQVodContentWidget::SetVodContentInfo(VodContentInfo_s info)
{
	m_contentIdx = info.contentIdx;
	m_contentName = info.contentTitle;
	
	m_vodChannelUrl = QString::fromStdString(SOOP_CHANNEL_URL) + info.channelId;

	//QFontMetrics metricsTitle(ui->label_Title->font());
	//QString elidedTitle = metricsTitle.elidedText(info.contentTitle, Qt::ElideRight, ui->label_Title->width());
	ui->label_Title->setText(info.contentTitle);

	//QFontMetrics metricsInfo(ui->label_Info->font());
	//QString elidedInfo = metricsInfo.elidedText(info.contentInfo, Qt::ElideRight, ui->label_Info->width());
	ui->label_Info->setText(info.contentInfo);
}

QString AFQVodContentWidget::GetVodContentName()
{
	return m_contentName;
}