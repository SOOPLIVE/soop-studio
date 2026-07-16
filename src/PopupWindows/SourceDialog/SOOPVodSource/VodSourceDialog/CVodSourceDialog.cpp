#include "CVodSourceDialog.h"
#include "ui_vod-source-dialog.h"

#include "qt-wrappers.hpp"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

#include "absolute-slider.hpp"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/Output/COutput.h"

#include "CVodListItem.h"
#include "CVodContentWidget.h" 
#include "CVodContentSeasonItem.h"
#include "json11.hpp"
//
#define VOD_DIALOG_CX		800
#define VOD_LISTFRAME_CX	439

AFQVodSourceDialog::AFQVodSourceDialog(QWidget* parent, 
									   obs_source_t* source) 
	: AFTTopBaseDialog(parent),
	ui(new Ui::AFQVodSourceDialog),
	weakSource(OBSGetWeakRef(source))
{
	ui->setupUi(this);

	ui->stackedWidget->setCurrentIndex(0);

	SetWidthResizeEnabled(false);

	m_sourceId = obs_source_get_id(source);
	const char* name = obs_source_get_display_name(m_sourceId.c_str());
	m_vodType = SOOP_SRC_MANAGER.GetSoopVodSourceType(m_sourceId.c_str());

	QString caption = QTStr("Popup.VodSource.Caption").arg(name);
    
#ifdef _WIN32
    ui->labelCaption->setText(caption);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    setWindowTitle(caption);
    ui->titleFrame->hide();
#endif

	QString vodList = QTStr("Popup.VodSource.VodList").arg(name);
	ui->label_VodList->setText(vodList);

	ui->vodListscrollAreaContents->SetVodDialogPtr(this);
	
	QString message;
	if (m_vodType == ANIME)
		message = QTStr("Popup.VodSource.EmptyPlayList_1").arg(name);
	else
		message = QTStr("Popup.VodSource.EmptyPlayList_2").arg(name);

	QString message_2 = QTStr("Popup.VodSource.EmptyPlayList_3");
	QString message_3 = QTStr("Popup.VodSource.EmptyPlayList_4");

	ui->vodListscrollAreaContents->SetVodDialogPtr(this);
	ui->vodListscrollAreaContents->SetVodEmtpyPlayListMessage(message_2, message_3);
	ui->labelEmptyContent->setText(message);

	QString cautionInfo = QTStr("Popup.VodSource.CautionInfoMessage").arg(name);
	ui->pushButton_VodInfo->SetExplanationText(cautionInfo, ENUM_TOOLTIP_POSITION::BottomCenter);
	ui->pushButton_Pause->hide();

	bool repeatSeries = SOOP_SRC_MANAGER.GetRepeatSeries(m_vodType);
	bool repeatVodList = SOOP_SRC_MANAGER.GetRepeatVodList(m_vodType);

	ui->pushButton_Repeat->setChecked(repeatVodList);
	ui->toggleButton_RepeatSeries->setChecked(repeatSeries);

	if (repeatSeries)
	{
		ui->pushButton_Repeat->hide();
		ui->pushButton_RepeatSeries->show();
	}
	else
	{
		ui->pushButton_Repeat->show();
		ui->pushButton_RepeatSeries->hide();
	}

	_RegisterUISignals();

	if (IsEmptyVodPlayList()) {
		ui->frameControl->hide();
	}

	QAction* sliderFoward = new QAction(this);
	sliderFoward->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(sliderFoward, &QAction::triggered, this, &AFQVodSourceDialog::qslotMediaSliderForward);
	sliderFoward->setShortcut({ Qt::Key_Right });
	addAction(sliderFoward);

	QAction* sliderBack = new QAction(this);
	sliderBack->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(sliderBack, &QAction::triggered, this, &AFQVodSourceDialog::qslotMediaSliderBackward);
	sliderBack->setShortcut({ Qt::Key_Left });
	addAction(sliderBack);

	connect(&SOOP_SRC_MANAGER, &SOOPMediaSourceManager::qsignalRefreshVODContents, this, &AFQVodSourceDialog::qslotRefreshContents);
	connect(&SOOP_SRC_MANAGER, &SOOPMediaSourceManager::qsignalRefreshVODSeasons, this, &AFQVodSourceDialog::qslotRefreshSeasons);

	ui->contentsScrollArea->installEventFilter(this);

	SOOP_SRC_MANAGER.RequestVODContents(m_vodType);
}

AFQVodSourceDialog::~AFQVodSourceDialog()
{
	delete ui;
}

void AFQVodSourceDialog::qslotCloseButtonClicked()
{
	close();
}

void AFQVodSourceDialog::qslotRefreshButtonClicked()
{
	SOOP_SRC_MANAGER.RequestVODContents(m_vodType);
}

void AFQVodSourceDialog::qslotPlayVOD(VodInfo_s info)
{
	if (AFOutputUtil::IsStreamActive()) {
		if (!MAIN_OUTPUT->IsStreamingOnlySoop()) {
			QString exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2")
				.arg(obs_source_get_display_name(m_sourceId.c_str()));
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", exceptionMsg);
			return;
		}
	}

	if (!_CheckVodEnableBroadSetting(info))
	{
		AFQCateChangeDialog dlg(this, m_sourceId.c_str());
		if (info.is_adult) {
			dlg.AddAllowedAnimeAdultCategoryInfo(info.allowed_category);
		}
		else {
			std::list<int> categorys;
			categorys.push_back(info.allowed_category);
			dlg.AddAllowedCategoryInfo(categorys);
		}

		if (QDialog::Accepted != dlg.exec())
			return;

		AFQBroadInfo* soopBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
		if (soopBroadInfo) {
			int selectedCategoryNum = dlg.GetSelectedCategory();
			soopBroadInfo->SetCategory(selectedCategoryNum);
			if (info.is_adult)
				soopBroadInfo->SetAdultOnly(info.is_adult);
			AUTH_CONTEXT.SendSoopBroadInfoSetting();
			MAINFRAME->RefreshBroadInfoDockUI(false);
		}
	}

	SOOP_SRC_MANAGER.PlayVodMedia(m_vodType, info, true);
}

void AFQVodSourceDialog::qslotStopvodButtonClicked()
{
	_StopMedia();
}

void AFQVodSourceDialog::qslotRepeatButtonClicked(bool checked)
{
	SOOP_SRC_MANAGER.SetRepeatVodList(m_vodType, checked);
}

void AFQVodSourceDialog::qslotRepeatSeriesInfoButtonClicked()
{
	AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, "", QTStr("Popup.VodSource.SeriesPlayInfo"), false, true);
}

void AFQVodSourceDialog::qslotShowContentList(bool checked)
{
	QRect geometry = this->geometry();

	const int adjustSize = VOD_DIALOG_CX - VOD_LISTFRAME_CX;
	if (checked) {
		
		ui->frameAniContents->show();

		setMinimumWidth(VOD_DIALOG_CX);
		setMaximumWidth(VOD_DIALOG_CX);

		geometry.setLeft(geometry.left() - adjustSize);
	}
	else {
		ui->frameAniContents->hide();

		setMinimumWidth(VOD_LISTFRAME_CX);
		setMaximumWidth(VOD_LISTFRAME_CX);

		geometry.setLeft(geometry.left() + adjustSize);
	}
	setGeometry(geometry);
}

void AFQVodSourceDialog::qslotContentItemClicked(int contentIdx)
{
	SOOP_SRC_MANAGER.RequestVODContentList(m_vodType, contentIdx);
}

void AFQVodSourceDialog::qslotAddContentSeasonVodItems(QString season)
{	
	if (AFOutputUtil::IsStreamActive()) {
		if (!MAIN_OUTPUT->IsStreamingOnlySoop()) {
			QString exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2")
				.arg(obs_source_get_display_name(m_sourceId.c_str()));
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", exceptionMsg);
			return;
		}
	}

	if (0 == m_curSeason.compare(season))
		return;

	bool findFirstVod = false;
	VodInfo_s firstVodInfo;
	std::vector<VodInfo_s>& vecVodLists = SOOP_SRC_MANAGER.GetSoopVodLists(m_vodType);
	std::vector<VodInfo_s>::iterator it = vecVodLists.begin();
	for (; it != vecVodLists.end(); ++it)
	{
		if (0 == season.compare((*it).seasonTitle)) {
			firstVodInfo = (*it);
			findFirstVod = true;
			break;
		}
	}
	
	// IMPORTANT: Ensure that the VOD list creation order and the VOD play sequence are correctly maintained.
	if (findFirstVod)
	{
		if (AFOutputUtil::IsStreamActive()) {
			if (!MAIN_OUTPUT->IsStreamingOnlySoop()) {
				QString exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2")
					.arg(obs_source_get_display_name(m_sourceId.c_str()));
				AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", exceptionMsg);
				return;
			}
		}

		if (!_CheckVodEnableBroadSetting(firstVodInfo))
		{
			AFQCateChangeDialog dlg(this, m_sourceId.c_str());
			if (firstVodInfo.is_adult) {
				dlg.AddAllowedAnimeAdultCategoryInfo(firstVodInfo.allowed_category);
			}
			else {
				std::list<int> categorys;
				categorys.push_back(firstVodInfo.allowed_category);
				dlg.AddAllowedCategoryInfo(categorys);
			}

			if (QDialog::Accepted != dlg.exec())
				return;

			AFQBroadInfo* soopBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
			if (soopBroadInfo) {
				int selectedCategoryNum = dlg.GetSelectedCategory();
				soopBroadInfo->SetCategory(selectedCategoryNum);
				if (firstVodInfo.is_adult)
					soopBroadInfo->SetAdultOnly(firstVodInfo.is_adult);
				AUTH_CONTEXT.SendSoopBroadInfoSetting();
				MAINFRAME->RefreshBroadInfoDockUI(false);
			}
		}

		SOOP_SRC_MANAGER.PlayVodMedia(m_vodType, firstVodInfo, true);
		_MakeVodPlayList(season);

	}
	else
	{
		_MakeVodPlayList(season);
	}
}

void AFQVodSourceDialog::qslotToggleRepeatSeries(bool checked)
{
	SOOP_SRC_MANAGER.SetRepeatSeries(m_vodType, checked);

	if (checked)
	{
		ui->pushButton_Repeat->hide();
		ui->pushButton_RepeatSeries->show();
	}
	else
	{
		ui->pushButton_Repeat->show();
		ui->pushButton_RepeatSeries->hide();
	}
}

void AFQVodSourceDialog::qslotSliderPosition()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (false == _IsValidSource(source))
		return;

	float time = (float)obs_source_media_get_time(source);
	float duration = (float)obs_source_media_get_duration(source);

	float sliderPosition;

	if (duration)
		sliderPosition = (time / duration) * (float)ui->slider_VodMedia->maximum();
	else
		sliderPosition = 0.0f;

	ui->slider_VodMedia->setValue((int)sliderPosition);

	ui->label_CurrentTime->setText(_FormatSeconds((int)(time / 1000.0f)));
	ui->label_Duration->setText(_FormatSeconds((int)(duration / 1000.0f)));
}

#ifdef _SOOP_VLC
void AFQVodSourceDialog::qslotSeekTimerCallback()
{
	if (m_lastSeek != m_seek) {

		OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
		if (_IsValidSource(source)) {
			obs_source_media_set_time(source, _GetSliderTime(m_seek));
		}
		m_lastSeek = m_seek;
	}
}
#endif // _SOOP_VLC

void AFQVodSourceDialog::qslotSeekForwardTimer()
{
	m_timerForward.stop();

	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (false == _IsValidSource(source))
		return;

	int64_t ms = obs_source_media_get_time(source) + 5000;
	if (ms >= obs_source_media_get_duration(source))
		return;

	obs_source_media_set_time(source, ms);

	qslotSliderPosition();
}

void AFQVodSourceDialog::qslotSeekBackwardTimer()
{
	m_timerBackward.stop();

	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (false == _IsValidSource(source))
		return;

	int64_t ms = obs_source_media_get_time(source) - 5000;
	if (ms <= 0)
		return;

	obs_source_media_set_time(source, ms);

	qslotSliderPosition();
}

void AFQVodSourceDialog::qslotPlayNextVodTimer()
{
	if(m_timerNextVod.isActive())
	m_timerNextVod.stop();
}

void AFQVodSourceDialog::qslotMediaSliderClicked()
{
#ifdef _SOOP_VLC
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!_IsValidSource(source))
		return;

	obs_media_state state = obs_source_media_get_state(source);

	if (state == OBS_MEDIA_STATE_PAUSED) {
		m_prevPaused = true;
	}
	else if (state == OBS_MEDIA_STATE_PLAYING) {
		m_prevPaused = false;
		_PauseMedia();
		_StopMediaTimer();
	}

	m_seek = ui->slider_VodMedia->value();
	m_timerSeek.start(100);
#else
	_StopMediaTimer();

	ui->label_CurrentTime->setText(_FormatSeconds((int)(_GetSliderTime(ui->slider_VodMedia->value()) / 1000.0f)));
#endif // _SOOP_VLC
}

void AFQVodSourceDialog::qslotMediaSliderReleased()
{
#ifdef _SOOP_VLC
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!_IsValidSource(source))
		return;

	if (m_timerSeek.isActive()) {
		m_timerSeek.stop();
		if (m_lastSeek != m_seek) {
			obs_source_media_set_time(source, _GetSliderTime(m_seek));
		}
		m_seek = m_lastSeek = -1;
	}

	if (!m_prevPaused) {
		_PlayMedia();
		_StartMediaTimer();
	}
#else
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	obs_source_media_set_time(source, _GetSliderTime(ui->slider_VodMedia->value()));

	_StartMediaTimer();
#endif // _SOOP_VLC
}

void AFQVodSourceDialog::qslotMediaSliderMoved(int val)
{
#ifdef _SOOP_VLC
	if (m_timerSeek.isActive()) {
		m_seek = val;
	}
#else
	ui->label_CurrentTime->setText(_FormatSeconds((int)(_GetSliderTime(ui->slider_VodMedia->value()) / 1000.0f)));
#endif // _SOOP_VLC
}

void AFQVodSourceDialog::qslotMediaSliderForward()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!_IsValidSource(source))
		return;

	if (m_timerBackward.isActive()) {
		m_timerBackward.stop();
	}

	if (!m_timerForward.isActive()) {
		m_timerForward.start(300);
	}
}

void AFQVodSourceDialog::qslotMediaSliderBackward()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!_IsValidSource(source))
		return;

	if (m_timerForward.isActive()) {
		m_timerForward.stop();
	}

	if (!m_timerBackward.isActive()) {
		m_timerBackward.start(300);
	}
}

void AFQVodSourceDialog::_qslotBuildVodListTick()
{
	if (m_iteratorMakeVodList == m_tmpMakeVodList.end()) {
		m_vodListBuildTimer->stop();
		ui->vodListscrollAreaContents->repaint();
		return;
	}

	VodInfo_s curVodInfo = SOOP_SRC_MANAGER.GetCurVodInfo(m_vodType);
	VodInfo_s recentlyVodInfo = SOOP_SRC_MANAGER.GetRecentlyVodInfo(m_vodType);

	int count = kBuildItemPerTick;
	while (count-- > 0 && m_iteratorMakeVodList != m_tmpMakeVodList.end()) {
		VodInfo_s& info = *m_iteratorMakeVodList;

		AFQVodListItem* item = new AFQVodListItem(ui->vodListscrollAreaContents);
		item->SetVodInfo(info);

		connect(item, &AFQVodListItem::qsignalPlayVOD, this, &AFQVodSourceDialog::qslotPlayVOD);

		if (SOOPMediaSourceManager::IsEqualVodInfo(curVodInfo, info))
			item->SetVodListItemStatus(true, obs_media_state::OBS_MEDIA_STATE_PLAYING);

		if (SOOPMediaSourceManager::IsEqualVodInfo(recentlyVodInfo, info))
			item->SetRecentyPlayVodStatus();

		m_vodListItems.push_back(item);

		item->setGeometry(VOD_PLAYLIST_ITEM_START_X, m_scrollHeight, VOD_PLAYLIST_ITEM_WIDTH, VOD_PLAYLIST_ITEM_HEIGHT);
		item->show();

		m_scrollHeight += (VOD_PLAYLIST_ITEM_HEIGHT + VOD_PLAYLIST_ITEM_SPACE);

		++m_iteratorMakeVodList;
	}
}

void AFQVodSourceDialog::_SourceRemoved(void* data, calldata_t* params)
{
	QMetaObject::invokeMethod(static_cast<AFQVodSourceDialog*>(data),
		"close");
}

void AFQVodSourceDialog::qslotBackButtonClicked()
{
	ui->stackedWidget->setCurrentIndex(0);
}

void AFQVodSourceDialog::qslotPauseVodButtonClicked()
{
	_PauseMedia();
}

void AFQVodSourceDialog::qslotPlayVodButtonClicked()
{
	VodInfo_s info = SOOP_SRC_MANAGER.GetCurVodInfo(m_vodType);
	if (0 == info.contentIdx)
		return;

	if (AFOutputUtil::IsStreamActive()) {
		if (!MAIN_OUTPUT->IsStreamingOnlySoop()) {
			QString exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2")
				.arg(obs_source_get_display_name(m_sourceId.c_str()));
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", exceptionMsg);
			return;
		}
	}

	if (!_CheckVodEnableBroadSetting(info))
	{
		if (0 == info.allowed_category)
			return;

		AFQCateChangeDialog dlg(this, m_sourceId.c_str());
		if (info.is_adult) {
			dlg.AddAllowedAnimeAdultCategoryInfo(info.allowed_category);
		}
		else {
			std::list<int> categorys;
			categorys.push_back(info.allowed_category);
			dlg.AddAllowedCategoryInfo(categorys);
		}

		if (QDialog::Accepted != dlg.exec())
			return;

		AFQBroadInfo* soopBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
		if (soopBroadInfo) {
			int selectedCategoryNum = dlg.GetSelectedCategory();
			soopBroadInfo->SetCategory(selectedCategoryNum);
			if (info.is_adult)
				soopBroadInfo->SetAdultOnly(info.is_adult);
			AUTH_CONTEXT.SendSoopBroadInfoSetting();
			MAINFRAME->RefreshBroadInfoDockUI(false);
		}
	}

	//SOOP_SRC_MANAGER.PlayVodMedia(m_vodType, info, true);

	_PlayMedia();
}

void AFQVodSourceDialog::qslotPrevVodButtonClicked()
{
	SOOP_SRC_MANAGER.PlayPrevVOD(m_vodType);
}

void AFQVodSourceDialog::qslotNextVodButtonClicked()
{
	SOOP_SRC_MANAGER.PlayNextVOD(m_vodType);
}

void AFQVodSourceDialog::qslotRefreshContents()
{
	for (auto it = m_vodContents.begin(); it != m_vodContents.end(); it++) {
		delete (*it);
	}
	m_vodContents.clear();

	std::vector<VodContentInfo_s>& contents = SOOP_SRC_MANAGER.GetSoopVodContents(m_vodType);
	for (auto it = contents.begin(); it != contents.end(); ++it) {
		VodContentInfo_s content = (*it);

		AFQVodContentWidget* item = new AFQVodContentWidget(nullptr, m_vodType);
		item->SetVodContentInfo(content);
		m_vodContents.push_back(item);

		ui->scrollAreaWidgetContents->layout()->addWidget(item);

		if (m_vodType != SOOP_VOD_TYPE::SPORT) {
			SOOP_API_HANDLER->downloadImage(content.contentImage.toStdString().c_str(), item, "qslotImageDownloaded");
		}

		connect(item, &AFQVodContentWidget::qsignalContentItemClicked, this, &AFQVodSourceDialog::qslotContentItemClicked);
	}

	if (0 == contents.size()) {
		ui->contentsScrollArea->hide();
		ui->frameEmptyContent->show();
	}
	else {
		ui->frameEmptyContent->hide();
		ui->contentsScrollArea->show();
	}

	ui->contentsScrollArea->verticalScrollBar()->setProperty("transparent", true);
	PolishStyleSheet(ui->contentsScrollArea->verticalScrollBar());

	QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->scrollAreaWidgetContents->layout());
	for (int i = 0; i < layout->count(); ++i) {
		QLayoutItem* item = layout->itemAt(i);
		if (item->spacerItem()) {
			QLayoutItem* removedItem = layout->takeAt(i);
			delete removedItem;
			break;
		}
	}
	layout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Preferred, QSizePolicy::Expanding));

}

void AFQVodSourceDialog::qslotRefreshSeasons(QString content, int contentNo, int recentlyListLoad)
{
	QString contentBack = QString(" %1").arg(content);

	for (auto it = m_vodContentSeasons.begin(); it != m_vodContentSeasons.end(); it++) {
		AFQVodContentSeasonItem* item = (*it);
		if (item)
			delete item;
	}
	m_vodContentSeasons.clear();

	QVBoxLayout* layout = ui->layoutVodContentList_2;
	for (int i = 0; i < layout->count(); ++i) {
		QLayoutItem* item = layout->itemAt(i);
		if (item->spacerItem()) {
			QLayoutItem* removedItem = layout->takeAt(i);
			delete removedItem;
			break;
		}
	}

	int seasonCount = 0;
	std::vector<QString>& seasons = SOOP_SRC_MANAGER.GetSoopVodContentSeason(m_vodType);
	for (auto it = seasons.begin(); it != seasons.end(); ++it) {
		QString season = (*it);

		AFQVodContentSeasonItem* item = new AFQVodContentSeasonItem();
		item->SetContentSeasonInfo(season);
		ui->layoutVodContentList_2->addWidget(item);

		connect(item, &AFQVodContentSeasonItem::qsignalAddContentSeasonVodList,
				this, &AFQVodSourceDialog::qslotAddContentSeasonVodItems);

		m_vodContentSeasons.push_back(item);

		seasonCount++;
	}

	int titleButtonWidth = 300;
	if (seasonCount > 1) {
		ui->label_SeriesRepeatInfo->show();
		ui->toggleButton_RepeatSeries->show();

		titleButtonWidth = 190;
	}
	else {
		ui->label_SeriesRepeatInfo->hide();
		ui->toggleButton_RepeatSeries->hide();
	}

	QFontMetrics metricsTitle(ui->pushButton_Back->font());
	QString elidedTitle = metricsTitle.elidedText(contentBack, Qt::ElideRight, titleButtonWidth);
	ui->pushButton_Back->setText(elidedTitle);
	if (0 != elidedTitle.compare(contentBack)) {
		ui->pushButton_Back->setToolTip(content);
	} else {
		ui->pushButton_Back->setToolTip("");
	}

	layout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Preferred, QSizePolicy::Expanding));

	if (1 == recentlyListLoad)
	{
		VodInfo_s recentlyVodInfo = SOOP_SRC_MANAGER.GetRecentlyVodInfo(m_vodType);
		if (!recentlyVodInfo.seasonTitle.isEmpty()) {
			if (0 != m_curSeason.compare(recentlyVodInfo.seasonTitle))
				_MakeVodPlayList(recentlyVodInfo.seasonTitle);
		}
	}
	
	m_curContentIdx = contentNo;

	ui->stackedWidget->setCurrentIndex(1);
}

void AFQVodSourceDialog::qslotRecvOBSMediaStarted()
{
	_RefreshControls();
}

void AFQVodSourceDialog::qslotRecvOBSMediaStopped()
{
	_RefreshControls();
}

void AFQVodSourceDialog::qslotRecvOBSMediaEnded()
{
	//qslotNextVodButtonClicked();
}

void AFQVodSourceDialog::qslotRecvOBSMediaPlay()
{
	_RefreshControls();
}

void AFQVodSourceDialog::qslotRecvOBSMediaPause()
{
	_RefreshControls();
}

QString AFQVodSourceDialog::_FormatSeconds(int totalSeconds)
{
	int seconds = totalSeconds % 60;
	int totalMinutes = totalSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}

int64_t AFQVodSourceDialog::_GetSliderTime(int val)
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!_IsValidSource(source))
		return 0;

	float percent = (float)val / (float)ui->slider_VodMedia->maximum();
	float duration = (float)obs_source_media_get_duration(source);
	int64_t seekTo = (int64_t)(percent * duration);

	return seekTo;
}

void AFQVodSourceDialog::_PlayMedia()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!_IsValidSource(source))
		return;

	obs_media_state state = obs_source_media_get_state(source);
	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
	case OBS_MEDIA_STATE_NONE:
		obs_source_media_restart(source);
		break;
	case OBS_MEDIA_STATE_PAUSED:
		obs_source_media_play_pause(source, false);
		break;
	}
}

void AFQVodSourceDialog::_PauseMedia()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!source)
		return;

	QString id = obs_source_get_id(source);
	if (0 != id.compare(SOOP_SRC_MANAGER.GetSoopVodSourceId(m_vodType)))
		return;

	obs_source_media_play_pause(source, true);

}

void AFQVodSourceDialog::_StopMedia()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!_IsValidSource(source))
		return;

	obs_source_media_stop(source);
}

void AFQVodSourceDialog::_StartMediaTimer()
{
	if (!m_timerMedia.isActive())
		m_timerMedia.start(1000);
}

void AFQVodSourceDialog::_StopMediaTimer()
{
	if (m_timerMedia.isActive())
		m_timerMedia.stop();
}

void AFQVodSourceDialog::_RegisterUISignals()
{
	connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotCloseButtonClicked);
	connect(ui->pushButton_Refresh, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotRefreshButtonClicked);
	connect(ui->pushButton_LeftAreaOpen, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotShowContentList);
	connect(ui->toggleButton_RepeatSeries, &AFQToggleButton::clicked, this, &AFQVodSourceDialog::qslotToggleRepeatSeries);

	connect(ui->slider_VodMedia, &AbsoluteSlider::sliderPressed, this, &AFQVodSourceDialog::qslotMediaSliderClicked);
	connect(ui->slider_VodMedia, &AbsoluteSlider::sliderReleased, this, &AFQVodSourceDialog::qslotMediaSliderReleased);
	connect(ui->slider_VodMedia, &AbsoluteSlider::sliderMoved, this, &AFQVodSourceDialog::qslotMediaSliderMoved);

#ifdef _SOOP_VLC
	connect(&m_timerSeek, &QTimer::timeout, this, &AFQVodSourceDialog::qslotSeekTimerCallback);
#endif // _SOOP_VLC

	connect(&m_timerForward, &QTimer::timeout, this, &AFQVodSourceDialog::qslotSeekForwardTimer);
	connect(&m_timerBackward, &QTimer::timeout, this, &AFQVodSourceDialog::qslotSeekBackwardTimer);
	connect(&m_timerMedia, &QTimer::timeout, this, &AFQVodSourceDialog::qslotSliderPosition);
	connect(&m_timerNextVod, &QTimer::timeout, this, &AFQVodSourceDialog::qslotPlayNextVodTimer);

	connect(ui->pushButton_Next, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotNextVodButtonClicked);
	connect(ui->pushButton_Prev, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotPrevVodButtonClicked);
	connect(ui->pushButton_Stop, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotStopvodButtonClicked);
	connect(ui->pushButton_Repeat, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotRepeatButtonClicked);
	connect(ui->pushButton_Play, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotPlayVodButtonClicked);
	connect(ui->pushButton_Pause, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotPauseVodButtonClicked);
	connect(ui->pushButton_Back, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotBackButtonClicked);
	connect(ui->pushButton_RepeatSeries, &QPushButton::clicked, this, &AFQVodSourceDialog::qslotRepeatSeriesInfoButtonClicked);
}

void AFQVodSourceDialog::_RefreshControls()
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (_IsValidSource(source)) 
	{
		uint32_t flags = 0;
		const char* id = nullptr;

		if (source) {
			flags = obs_source_get_output_flags(source);
			id = obs_source_get_unversioned_id(source);
		}

		if (!source || !(flags & OBS_SOURCE_CONTROLLABLE_MEDIA)) {
			_SetVODStopState();
			setEnabled(false);
			hide();
			return;
		}
		else {
			setEnabled(true);
			show();
		}

		obs_media_state state = obs_source_media_get_state(source);
		switch (state) {
		case OBS_MEDIA_STATE_STOPPED:
		case OBS_MEDIA_STATE_ENDED:
		case OBS_MEDIA_STATE_NONE:
			_SetVODStopState();
			break;
#ifndef _SOOP_VLC
		case OBS_MEDIA_STATE_OPENING:
#endif // !_SOOP_VLC
		case OBS_MEDIA_STATE_PLAYING:
			_SetVODPlayingState();
			break;
		case OBS_MEDIA_STATE_PAUSED:
			_SetVODPauseState();
			break;
		default:
			break;
		}
	}
}

void AFQVodSourceDialog::_SetVODPlayingState()
{
	ui->slider_VodMedia->setEnabled(true);

	ui->pushButton_Play->hide();
	ui->pushButton_Pause->show();

#ifdef _SOOP_VLC
	ui->label_Duration->setProperty("playing", true);
	ui->label_CurrentTime->setProperty("playing", true);

	PolishStyleSheet(ui->label_Duration);
	PolishStyleSheet(ui->label_CurrentTime);

	ui->pushButton_Prev->setDisabled(false);
	ui->pushButton_Next->setDisabled(false);
	ui->pushButton_Pause->setDisabled(false);
	ui->pushButton_Stop->setDisabled(false);
	ui->pushButton_Repeat->setDisabled(false);

	m_prevPaused = false;

	ui->slider_VodMedia->setDisabled(false);
#else
	ui->pushButton_Stop->setEnabled(true);
#endif // _SOOP_VLC

	_StartMediaTimer();

	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (_IsValidSource(source))
	{
		VodInfo_s info = SOOP_SRC_MANAGER.GetCurVodInfo(m_vodType);
		_SetVodListItemStatus(info);
	}
}

void AFQVodSourceDialog::_SetVODPauseState()
{
#ifndef _SOOP_VLC
	ui->slider_VodMedia->setEnabled(true);
#endif // !_SOOP_VLC
	ui->pushButton_Play->show();
	ui->pushButton_Pause->hide();
#ifndef _SOOP_VLC
	ui->pushButton_Stop->setEnabled(true);
#endif // !_SOOP_VLC

	_StartMediaTimer();
}

void AFQVodSourceDialog::_SetVODStopState()
{
#ifdef _SOOP_VLC
	ui->pushButton_Play->hide();
	ui->pushButton_Pause->show();

	ui->label_Duration->setText("00:00:00");
	ui->label_CurrentTime->setText("00:00:00");

	ui->label_Duration->setProperty("playing", false);
	ui->label_CurrentTime->setProperty("playing", false);

	PolishStyleSheet(ui->label_Duration);
	PolishStyleSheet(ui->label_CurrentTime);

	ui->pushButton_Prev->setDisabled(true);
	ui->pushButton_Next->setDisabled(true);
	ui->pushButton_Pause->setDisabled(true);
	ui->pushButton_Stop->setDisabled(true);
	ui->pushButton_Repeat->setDisabled(true);
#else
	ui->label_CurrentTime->setText("--:--:--");
	ui->label_Duration->setText("--:--:--");
	ui->pushButton_Play->show();
	ui->pushButton_Pause->hide();
	ui->pushButton_Stop->setDisabled(true);
#endif // _SOOP_VLC

	_StopMediaTimer();

	ui->slider_VodMedia->setValue(0);
	ui->slider_VodMedia->setDisabled(true);

	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (_IsValidSource(source)) {
		VodInfo_s emptyInfo = {};
		_SetVodListItemStatus(emptyInfo);
	}

	// check recently play vod
	VodInfo_s recentlyVodInfo = SOOP_SRC_MANAGER.GetRecentlyVodInfo(m_vodType);
	for (auto it = m_vodListItems.begin(); it != m_vodListItems.end(); ++it) {
		VodInfo_s info = (*it)->GetVodInfo();

		if (SOOPMediaSourceManager::IsEqualVodInfo(recentlyVodInfo, info)) {
			(*it)->SetRecentyPlayVodStatus();
			break;
		}
	}
}

bool AFQVodSourceDialog::_CheckVodEnableBroadSetting(VodInfo_s vodInfo)
{
	if (AFOutputUtil::IsStreamActive())
	{
		AFQBroadInfo* pSoopBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();

		bool adultOptionNotMatch = false;
		if (!pSoopBroadInfo->AdultOnly()) {
			adultOptionNotMatch = (vodInfo.is_adult != pSoopBroadInfo->AdultOnly());
		}

		if (adultOptionNotMatch || vodInfo.allowed_category != pSoopBroadInfo->CategoryNumber())
		{
			return false;
		}
	}

	return true;
}

bool AFQVodSourceDialog::_IsValidSource(OBSSource source)
{
	if (!source)
		return false;

	QString id = obs_source_get_id(source);
	if (0 != id.compare(SOOP_SRC_MANAGER.GetSoopVodSourceId(m_vodType)))
		return false;

	return true;
}

bool AFQVodSourceDialog::_SetVodListItemStatus(VodInfo_s vodInfo)
{
	OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
	if (!_IsValidSource(source))
		return false;

	obs_media_state state = obs_source_media_get_state(source);

	bool findItem = false;
	std::vector<AFQVodListItem*>::iterator it = m_vodListItems.begin();
	for (; it != m_vodListItems.end(); it++) {
		VodInfo_s info = (*it)->GetVodInfo();

		if (SOOPMediaSourceManager::IsEqualVodInfo(vodInfo, info))
		{
			(*it)->SetVodListItemStatus(true, state);
			findItem = true;
		}
		else
		{
			(*it)->SetVodListItemStatus(false, state);
		}
	}

	return findItem;
}

void AFQVodSourceDialog::_MakeVodPlayList(QString season)
{
	int vodIdx = 0;
	int scrollHeight = 0;
	std::vector<VodInfo_s>& vecVodLists = SOOP_SRC_MANAGER.GetSoopVodLists(m_vodType);

	ui->vodListscrollArea->verticalScrollBar()->setValue(0);

	VodInfo_s curVodInfo = SOOP_SRC_MANAGER.GetCurVodInfo(m_vodType);
	VodInfo_s recentlyVodInfo = SOOP_SRC_MANAGER.GetRecentlyVodInfo(m_vodType);

	for (auto it = m_vodListItems.begin(); it != m_vodListItems.end(); it++) {
		AFQVodListItem* item = (*it);
		if (item)
			item->deleteLater();
	}
	m_vodListItems.clear();
	m_tmpMakeVodList.clear();

	for (auto it = vecVodLists.begin(); it != vecVodLists.end(); ++it) {
		if (0 == season.compare((*it).seasonTitle)) {
			VodInfo_s info = (*it);
			m_tmpMakeVodList.push_back(info);
			vodIdx++;
			scrollHeight += (VOD_PLAYLIST_ITEM_HEIGHT + VOD_PLAYLIST_ITEM_SPACE);
		}
	}

	ui->vodListscrollAreaContents->setMinimumHeight(scrollHeight);

	m_scrollHeight = 0;
	m_iteratorMakeVodList = m_tmpMakeVodList.begin();

	if (!m_vodListBuildTimer) {
		m_vodListBuildTimer = new QTimer(this);
		connect(m_vodListBuildTimer, &QTimer::timeout, this, &AFQVodSourceDialog::_qslotBuildVodListTick);
	}

	if (m_vodListBuildTimer->isActive())
		m_vodListBuildTimer->stop();

	m_vodListBuildTimer->start(kBuildIntervalMs);

	m_curSeason = season;

	QString format = QTStr("Popup.VodSource.PlayListCount");
	QString str = QString(format).arg(vodIdx);
	ui->label_PlayListCount->setText(str);
	ui->frameControl->show();
}

bool AFQVodSourceDialog::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == ui->contentsScrollArea) {
		if (event->type() == QEvent::Enter) {
			SetScrollBarTransparent(ui->contentsScrollArea, ui->scrollAreaWidgetContents, false);
			return false;
		}
		else if (event->type() == QEvent::Leave) {
			SetScrollBarTransparent(ui->contentsScrollArea, ui->scrollAreaWidgetContents, true);
			return false;
		}
	}
	return QWidget::eventFilter(watched, event);
}

void AFQVodSourceDialog::showEvent(QShowEvent* event)
{
	if(m_ignoreShowEvent)
		return;
	//
	if (weakSource)
	{
		OBSSource source = OBSGetStrongRef(weakSource);
		signal_handler_t* handler = obs_source_get_signal_handler(source);
		if (handler) {
			removeSignal.Connect(obs_source_get_signal_handler(source), "remove",
				AFQVodSourceDialog::_SourceRemoved, this);
		}
	}

	SOOP_SRC_MANAGER.SetSoopMediaSourceProps(this);

	VodInfo_s recentlyVodInfo = SOOP_SRC_MANAGER.GetRecentlyVodInfo(m_vodType);
	if (0 != recentlyVodInfo.contentIdx && 
		m_curContentIdx != recentlyVodInfo.contentIdx) {
			SOOP_SRC_MANAGER.RequestVODContentList(m_vodType, recentlyVodInfo.contentIdx, 1);
	}
	else
	{
		if(SOOPMediaSourceManager::IsEqualVodInfo(recentlyVodInfo, VodInfo_s())) {

			for (auto it = m_vodListItems.begin(); it != m_vodListItems.end(); it++) {
				AFQVodListItem* item = (*it);
				if (item)
					item->deleteLater();
			}
			m_vodListItems.clear();
			ui->stackedWidget->setCurrentIndex(0);
			ui->vodListscrollAreaContents->repaint();
			ui->label_PlayListCount->setText(QTStr("Popup.VodSource.PlayList"));

			m_curSeason = "";
		}
		else
		{
			if (0 == m_curSeason.compare(recentlyVodInfo.seasonTitle)) {
				for (auto it = m_vodListItems.begin(); it != m_vodListItems.end(); ++it) {
					VodInfo_s info = (*it)->GetVodInfo();

					if (SOOPMediaSourceManager::IsEqualVodInfo(recentlyVodInfo, info)) {
						(*it)->SetRecentyPlayVodStatus();
						break;
					}
				}
			}
			else {
				_MakeVodPlayList(recentlyVodInfo.seasonTitle);
			}
		}
	}

	if (IsEmptyVodPlayList() &&
		m_tmpMakeVodList.empty()) {
		ui->frameControl->hide();
		ui->vodListscrollAreaContents->setMinimumHeight(0);
	}

	m_ignoreShowEvent = true;
	_RefreshControls();
	m_ignoreShowEvent = false;

	ui->contentsScrollArea->verticalScrollBar()->setProperty("transparent", true);
	PolishStyleSheet(ui->contentsScrollArea->verticalScrollBar());

	if (IsEmptyVodPlayList()) {
		ui->frameControl->hide();
	}

	resize(800, 550);
}

void AFQVodSourceDialog::hideEvent(QHideEvent* event)
{
	_SetVODStopState();

	SOOP_SRC_MANAGER.SetSoopMediaSourceProps(nullptr);
}

bool AFQVodSourceDialog::IsEmptyVodPlayList()
{
	return m_vodListItems.empty();
}

void AFQVodSourceDialog::SetSource(obs_source_t* source)
{
	weakSource = OBSGetWeakRef(source);
}
