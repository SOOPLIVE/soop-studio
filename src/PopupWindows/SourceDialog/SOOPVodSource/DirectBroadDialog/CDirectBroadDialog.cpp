#include "CDirectBroadDialog.h"
#include "ui_direct-broad-dialog.h"

#include <QTime>
#include <QDate>
#include <QDateTime>
#include <QScrollBar>

#include "qt-wrappers.hpp"

#include "CDirectBroadItem.h"

#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

AFQDirectBroadDialog::AFQDirectBroadDialog(QWidget* parent, obs_source_t* source):
	AFTTopBaseDialog(parent), 
	ui(new Ui::AFQDirectBroadDialog),
	removeSignal(obs_source_get_signal_handler(source), "remove",
		AFQDirectBroadDialog::_SourceRemoved, this)
{
	ui->setupUi(this);
	
#ifdef __APPLE__
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    setWindowTitle(QTStr("Popup.DirectBroadProps"));
    ui->titleFrame->hide();
#endif

	SetWidthResizeEnabled(false);

	SetPropAndPolishStyleSheet(ui->pushButton_All, "selected", 1);

	connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQDirectBroadDialog::_qslotCloseButtonClicked);
	connect(ui->pushButton_PrevDate, &QPushButton::clicked, this, &AFQDirectBroadDialog::_qslotPrevDateButtonClicked);
	connect(ui->pushButton_NextDate, &QPushButton::clicked, this, &AFQDirectBroadDialog::_qslotNextDateButtonClicked);
	connect(ui->pushButton_All, &QPushButton::clicked, this, &AFQDirectBroadDialog::_qslotCategoryAllButtonClicked);
	connect(ui->pushButton_Sport, &QPushButton::clicked, this, &AFQDirectBroadDialog::_qslotCategorySportButtonClicked);
	connect(ui->pushButton_ESport, &QPushButton::clicked, this, &AFQDirectBroadDialog::_qslotCategoryESportButtonClicked);
	connect(ui->pushButton_OfficialBroad, &QPushButton::clicked, this, &AFQDirectBroadDialog::_qslotCategoryOfficialBroadButtonClicked);
	connect(ui->pushButton_Refresh, &QPushButton::clicked, this, &AFQDirectBroadDialog::_qslotRefreshDirectBroadListButtonClicked);

	connect(&SOOP_SRC_MANAGER, &SOOPMediaSourceManager::qsignalRefreshDirectBroadList, this, &AFQDirectBroadDialog::qslotRefreshDirectBroadList);
	connect(&SOOP_SRC_MANAGER, &SOOPMediaSourceManager::qsignalResponseDirectBroadOneTimeUrl, this, &AFQDirectBroadDialog::qslotResponseDirectBroadOnetimeUrl);

	SOOP_SRC_MANAGER.RequestDirectBroadList();
}

AFQDirectBroadDialog::~AFQDirectBroadDialog()
{
	SOOP_SRC_MANAGER.SetSoopMediaSourceProps(nullptr);

	delete ui;
}

namespace {
	void ClearLayout(QLayout* layout)
	{
		if (!layout)
			return;

		while (QLayoutItem* item = layout->takeAt(0)) {
			if (QWidget* widget = item->widget()) {
				widget->hide();
				widget->deleteLater();
			}
			else if (QLayout* childLayout = item->layout()) {
				ClearLayout(childLayout);
				delete childLayout;
			}

			delete item;
		}
	}
}

void AFQDirectBroadDialog::_qslotCloseButtonClicked()
{
	close();
}

void AFQDirectBroadDialog::_qslotPrevDateButtonClicked()
{
	std::vector<QString> dateList = SOOP_SRC_MANAGER.GetDirectBroadDateList();
	auto it = dateList.begin();
	for (; it != dateList.end(); ++it) {
		if (0 == (*it).compare(m_selectDate)) 
		{
			if (it != dateList.begin()) {
				--it;
			}
			break;
		}
	}

	QString prevDate = (*it);

	_SetDirectBroadList(prevDate);

	SetPropAndPolishStyleSheet(ui->pushButton_All, "selected", 1);
	SetPropAndPolishStyleSheet(ui->pushButton_Sport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_ESport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_OfficialBroad, "selected", 0);
}

void AFQDirectBroadDialog::_qslotNextDateButtonClicked()
{
	std::vector<QString> dateList = SOOP_SRC_MANAGER.GetDirectBroadDateList();
	auto it = dateList.begin();
	for (; it != dateList.end(); ++it) {
		if (0 == (*it).compare(m_selectDate)) {
			++it;
			break;
		}
	}

	if (it == dateList.end())
		--it;

	QString nextDate = (*it);

	_SetDirectBroadList(nextDate);

	SetPropAndPolishStyleSheet(ui->pushButton_All, "selected", 1);
	SetPropAndPolishStyleSheet(ui->pushButton_Sport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_ESport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_OfficialBroad, "selected", 0);
}

void AFQDirectBroadDialog::_qslotCategoryAllButtonClicked()
{
	SetPropAndPolishStyleSheet(ui->pushButton_All, "selected", 1);
	SetPropAndPolishStyleSheet(ui->pushButton_Sport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_ESport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_OfficialBroad, "selected", 0);

	_SetDirectBroadList(m_selectDate);
}

void AFQDirectBroadDialog::_qslotCategorySportButtonClicked()
{
	SetPropAndPolishStyleSheet(ui->pushButton_All, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_Sport, "selected", 1);
	SetPropAndPolishStyleSheet(ui->pushButton_ESport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_OfficialBroad, "selected", 0);

	PolishStyleSheet(ui->pushButton_All);
	PolishStyleSheet(ui->pushButton_Sport);
	PolishStyleSheet(ui->pushButton_ESport);
	PolishStyleSheet(ui->pushButton_OfficialBroad);

	_SetDirectBroadList(m_selectDate,0);
}

void AFQDirectBroadDialog::_qslotCategoryESportButtonClicked()
{
	SetPropAndPolishStyleSheet(ui->pushButton_All, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_Sport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_ESport, "selected", 1);
	SetPropAndPolishStyleSheet(ui->pushButton_OfficialBroad, "selected", 0);

	_SetDirectBroadList(m_selectDate, 1);
}

void AFQDirectBroadDialog::_qslotCategoryOfficialBroadButtonClicked()
{
	SetPropAndPolishStyleSheet(ui->pushButton_All, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_Sport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_ESport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_OfficialBroad, "selected", 1);

	_SetDirectBroadList(m_selectDate, 2);
}

void AFQDirectBroadDialog::_qslotRefreshDirectBroadListButtonClicked()
{
	SOOP_SRC_MANAGER.RequestDirectBroadList();
}

void AFQDirectBroadDialog::qslotRefreshDirectBroadList()
{
	QDateTime currentDateTime = QDateTime::currentDateTime();
	QString formattedTime = currentDateTime.toString("yyyy-MM-dd");

	_SetDirectBroadList(formattedTime);

	SetPropAndPolishStyleSheet(ui->pushButton_All, "selected", 1);
	SetPropAndPolishStyleSheet(ui->pushButton_Sport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_ESport, "selected", 0);
	SetPropAndPolishStyleSheet(ui->pushButton_OfficialBroad, "selected", 0);
}

void AFQDirectBroadDialog::qslotResponseDirectBroadOnetimeUrl(int requestIdx, QString url)
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source)
		return;

	QString id = obs_source_get_id(source);
	if (0 != id.compare("soop_directbroad_source"))
		return;

	auto it = m_directBroadItems.begin();
	for (; it != m_directBroadItems.end(); ++it) {
		AFQDirectBroadItem* item = (*it);
		if (!item)
			continue;

		DirectBroadInfo_s info = item->GetDirectBroadInfo();
		item->SetDirectBroadPlayingStatus(requestIdx == info.idx);
	}
}

void AFQDirectBroadDialog::qslotRequestOneTimeUrl(int idx)
{
	SOOP_SRC_MANAGER.RequestDirectBroadOneTimeUrlWithGeoBlock(idx);
}

void AFQDirectBroadDialog::qslotRequestStopDirectBroad(int idx)
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source)
		return;

	QString id = obs_source_get_id(source);
	if (0 != id.compare("soop_directbroad_source"))
		return;

	obs_source_media_stop(source);

	auto it = m_directBroadItems.begin();
	for (; it != m_directBroadItems.end(); ++it) {
		const DirectBroadInfo_s info = (*it)->GetDirectBroadInfo();
		if (info.idx == idx) {
			(*it)->SetDirectBroadPlayingStatus(false);
			break;
		}
	}
}

void AFQDirectBroadDialog::showEvent(QShowEvent* event)
{
	SOOP_SRC_MANAGER.SetSoopMediaSourceProps(this);

	resize(360, 550);
}

void AFQDirectBroadDialog::_SourceRemoved(void* data, calldata_t* params)
{
	QMetaObject::invokeMethod(static_cast<AFQDirectBroadDialog*>(data),
		"close");
}

void AFQDirectBroadDialog::_SetDirectBroadList(QString date, int category)
{
	QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents->layout());
	
	if (!layout)
		return;

	QWidget* contents = ui->scrollAreaWidgetContents;
	QWidget* viewport = ui->scrollArea->viewport();

	contents->setUpdatesEnabled(false);
	viewport->setUpdatesEnabled(false);

	const int scrollValue = ui->scrollArea->verticalScrollBar()->value();

	ClearLayout(layout);
	m_directBroadItems.clear();

	obs_media_state media_state = OBS_MEDIA_STATE_NONE;
	int currentDirectBroadIdx = SOOP_SRC_MANAGER.GetDirectBroadIdx();

	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if(source)
		media_state = obs_source_media_get_state(source);

	QStringList dateParts = date.split('-');
	if (dateParts.size() != 3) {
		contents->setUpdatesEnabled(true);
		viewport->setUpdatesEnabled(true);
		return;
	}

	int year = dateParts.at(0).toInt();
	int month = dateParts.at(1).toInt();
	int day = dateParts.at(2).toInt();

	QString monthString = QString("%1").arg(month, 2, 10, QChar('0'));
	QString dayString = QString("%1").arg(day, 2, 10, QChar('0'));

	QString formattedDateStr = date;
	QDate resultDate(year, month, day);
	if (resultDate.isValid()) {
		QLocale koreanLocale(QLocale::Korean, QLocale::SouthKorea);
		QString dayOfWeek = koreanLocale.dayName(resultDate.dayOfWeek(), QLocale::ShortFormat);
		formattedDateStr = QString::number(year) + "." + monthString + "." +
			dayString + " (" + dayOfWeek + ")";
	}

	ui->label_Date->setText(formattedDateStr);

	int addedItemCount = 0;
	QPointer<AFQDirectBroadItem> playingItem = nullptr;

	std::vector<DirectBroadInfo_s> directBroadList = SOOP_SRC_MANAGER.GetDirectBroadList();
	for (const DirectBroadInfo_s& info : directBroadList) {
		if (info.broad_date != date)
			continue;

		if (category != -1 && info.category != category)
			continue;

		// cancel directbroad pass
		if (info.status == 3)
			continue;

		AFQDirectBroadItem* item = new AFQDirectBroadItem(this);
		item->SetDirectBroadInfo(info);

		if (OBS_MEDIA_STATE_PLAYING == media_state ||
			OBS_MEDIA_STATE_OPENING == media_state)
		{
			if (info.idx == currentDirectBroadIdx) {
				item->SetDirectBroadPlayingStatus(true);
				playingItem = item;
			}
		}

		connect(item, &AFQDirectBroadItem::qsignalRequestOnetimeUrl, this, &AFQDirectBroadDialog::qslotRequestOneTimeUrl);
		connect(item, &AFQDirectBroadItem::qsignalStopDirectBroad, this, &AFQDirectBroadDialog::qslotRequestStopDirectBroad);

		layout->addWidget(item);

		addedItemCount++;

		m_directBroadItems.push_back(item);
	}

	if (0 == addedItemCount) {
		QLabel* emptyMessage = new QLabel(this);
		emptyMessage->setFixedHeight(100);
		emptyMessage->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		emptyMessage->setText(QTStr("Popup.DirectBroad.EmptyBroadList"));
		layout->addWidget(emptyMessage);
	}

	layout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Preferred, QSizePolicy::Expanding));

	m_selectDate = date;

	layout->invalidate();
	ui->scrollAreaWidgetContents->adjustSize();

	ui->scrollArea->verticalScrollBar()->setValue(scrollValue);

	contents->setUpdatesEnabled(true);
	viewport->setUpdatesEnabled(true);

	if (playingItem) {
		QTimer::singleShot(0, this, [this, playingItem]() {
			if (playingItem)
				ui->scrollArea->ensureWidgetVisible(playingItem, 0, 20);
			});
	}

	ui->scrollAreaWidgetContents->update();
	ui->scrollArea->viewport()->update();
}
