#include "CVodContentSeasonItem.h"
#include "ui_vod-content-season.h"

AFQVodContentSeasonItem::AFQVodContentSeasonItem(QWidget* parent) :
	QFrame(parent),
	ui(new Ui::AFQVodContentSeasonItem)
{
	ui->setupUi(this);

	connect(ui->pushButton_AddSeason, &QPushButton::clicked, 
			this, &AFQVodContentSeasonItem::_qslotAddContentSeasonVodList);

}

AFQVodContentSeasonItem::~AFQVodContentSeasonItem()
{
	delete ui;
}

void AFQVodContentSeasonItem::_qslotAddContentSeasonVodList()
{
	emit qsignalAddContentSeasonVodList(m_seasonTitle);
}

void AFQVodContentSeasonItem::SetContentSeasonInfo(QString title)
{
	m_seasonTitle = title;
	ui->label_Season->setText(title);
}