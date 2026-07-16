#include "CTvBroadDialog.h"
#include "ui_tv-broad-dialog.h"

#include <QTime>
#include <QDateTime>

#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/Output/COutput.h"

#include "CTvBroadItem.h"

AFQTvBroadDialog::AFQTvBroadDialog(QWidget* parent, obs_source_t* source):
	AFTTopBaseDialog(parent),
	ui(new Ui::AFQTvBroadDialog),
	removeSignal(obs_source_get_signal_handler(source), "remove",
		AFQTvBroadDialog::_SourceRemoved, this)
{
	ui->setupUi(this);

	ui->pushButton_Refresh->hide();

#ifdef __APPLE__
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    setWindowTitle(QTStr("Popup.TvLive.Caption"));
    ui->titleFrame->hide();
#endif
    
	SetWidthResizeEnabled(false);

	connect(ui->pushButton_Close, &QPushButton::clicked,this, &AFQTvBroadDialog::_qslotCloseButtonClicked);

	connect(&SOOP_SRC_MANAGER, &SOOPMediaSourceManager::qsignalResponseTvLiveOneTimeUrl,
			this, &AFQTvBroadDialog::qslotResponseTvLiveOnetimeUrl);

	const int nCurrentCPNo = SOOP_SRC_MANAGER.GetTvLiveCPNo();


	obs_media_state media_state = obs_source_media_get_state(source);
	bool isPlayingState = (media_state == OBS_MEDIA_STATE_PLAYING ||
							media_state == OBS_MEDIA_STATE_OPENING ||
							media_state == OBS_MEDIA_STATE_BUFFERING ||
							media_state == OBS_MEDIA_STATE_PAUSED);

	std::vector<TvLiveInfo_s>& tvLists = SOOP_SRC_MANAGER.GetTvLiveLists();
	for (auto it = tvLists.begin(); it != tvLists.end(); ++it) {
		AFQTvBroadItem* item = new AFQTvBroadItem(ui->scrollAreaWidgetContents);
		item->SetTvBroadInfo((*it).cpNo, (*it).cpTitle);

		if (nCurrentCPNo == (*it).cpNo && isPlayingState)
			item->SetTvBroadPlayingStatus(true);

		connect(item, &AFQTvBroadItem::qsignalRequestOnetimeUrl, this, &AFQTvBroadDialog::qslotTvLiveItemClicked);
		connect(item, &AFQTvBroadItem::qsignalStopTvBroad, this, &AFQTvBroadDialog::qslotRequestStopTvBroad);
		ui->scrollAreaLayout->addWidget(item);

		m_tvBroadItems.push_back(item);
	}

	ui->scrollAreaLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));
}

AFQTvBroadDialog::~AFQTvBroadDialog()
{
	delete ui;
}

void AFQTvBroadDialog::_qslotCloseButtonClicked()
{
	close();
}

void AFQTvBroadDialog::qslotResponseTvLiveOnetimeUrl(int cpNo)
{
	auto it = m_tvBroadItems.begin();
	for (; it != m_tvBroadItems.end(); ++it) {
		AFQTvBroadItem* item = (*it);
		if (!item)
			continue;

		int cpNo_ = item->GetTvBroadCpNo();
		item->SetTvBroadPlayingStatus(cpNo == cpNo_);
	}
}

void AFQTvBroadDialog::qslotTvLiveItemClicked(int cpNo)
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source)
		return;

	QString id = obs_source_get_id(source);
	if (0 != id.compare("soop_tv_cable_source"))
		return;

	if (AFOutputUtil::IsStreamActive())
	{
		if (!MAIN_OUTPUT->IsStreamingOnlySoop()) {
			QString exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2")
				.arg(obs_source_get_display_name(id.toStdString().c_str()));
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", exceptionMsg);
			return;
		}
		
		AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
		const int categoryNo = 390000 + cpNo;
		if (!broadInfo)
			return;

		if (broadInfo->CategoryNumber() != categoryNo)
		{
			std::list<int> categorys;
			categorys.push_back(categoryNo);

			AFQCateChangeDialog dlg(this, id.toStdString().c_str());
			dlg.AddAllowedCategoryInfo(categorys);

			if (QDialog::Accepted != dlg.exec())
				return;

			int selectedCategoryNum = dlg.GetSelectedCategory();
			broadInfo->SetCategory(selectedCategoryNum);
			AUTH_CONTEXT.SendSoopBroadInfoSetting();
			MAINFRAME->RefreshBroadInfoDockUI(false);
		}
	}

	if (source) {
		obs_media_state media_state = obs_source_media_get_state(source);
		if (media_state == OBS_MEDIA_STATE_PLAYING ||
			media_state == OBS_MEDIA_STATE_OPENING ||
			media_state == OBS_MEDIA_STATE_BUFFERING ||
			media_state == OBS_MEDIA_STATE_PAUSED)
		{
			obs_source_media_stop(source);
		}
	}

	const int categoryNo = 390000 + cpNo;

	QList<QVariant> queryValues;
	QList<int>      additionalData = { cpNo, categoryNo };

	SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_BROAD_GEO_BLOCK, queryValues,
								   this, "_qslotTvLiveGeoBlockCheckAPIResponse", additionalData);
}

void AFQTvBroadDialog::qslotRequestStopTvBroad(int cpNo)
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source)
		return;

	QString id = obs_source_get_id(source);
	if (0 != id.compare("soop_tv_cable_source"))
		return;

	auto it = m_tvBroadItems.begin();
	for (; it != m_tvBroadItems.end(); ++it) {
		AFQTvBroadItem* item = (*it);
		if (!item)
			continue;

		int cpNo_ = item->GetTvBroadCpNo();
		if (cpNo_ == cpNo) {
			obs_source_media_stop(source);
			SOOP_SRC_MANAGER.StopTvLiveBlind();			
			item->SetTvBroadPlayingStatus(false);			
		}
	}
}

void AFQTvBroadDialog::showEvent(QShowEvent* event)
{
	resize(width(), 550);
}

void AFQTvBroadDialog::_SourceRemoved(void* data, calldata_t* params)
{
	QMetaObject::invokeMethod(static_cast<AFQTvBroadDialog*>(data),
		"close");
}

void AFQTvBroadDialog::_qslotTvLiveGeoBlockCheckAPIResponse( const QByteArray& responseData, int cpNo, int categoryNo)
{
	std::string jsonString = responseData.toStdString();
	std::string err;

	SOOP_SRC_MANAGER.RequestTvLiveOneTimeUrl(cpNo);
}
