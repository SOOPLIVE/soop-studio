#include "CSoopMediaSourceManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>

#include "platform/platform.hpp"

#include "Common/StudioDefine.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Auth/CAuthManager.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/Output/COutput.h"
#include "Blocks/SceneSourceDock/CSourceListView.h"

enum class TvLiveRequestType {
	PlayUrl = 0,
	BlindCheck = 1,
};

namespace {

	int JsonValueToInt(const QJsonValue& value, int defaultValue = 0)
	{
		if (value.isString())
			return value.toString().toInt();

		if (value.isDouble())
			return value.toInt();

		return defaultValue;
	}

	int DirectBroadStatusPriority(int status)
	{
		switch (status) {
		case 1:
			return 0; // Possible
		case 0:
			return 1; // Scheduled
		case 2:
			return 2; // Finished
		case 3:
			return 3; // Canceled
		default:
			return 4;
		}
	}

	bool LessDirectBroadInfo(const DirectBroadInfo_s& a, const DirectBroadInfo_s& b)
	{
		const int aPriority = DirectBroadStatusPriority(a.status);
		const int bPriority = DirectBroadStatusPriority(b.status);

		if (aPriority != bPriority)
			return aPriority < bPriority;

		const QTime aTime = QTime::fromString(a.start_time, "HH:mm");
		const QTime bTime = QTime::fromString(b.start_time, "HH:mm");

		if (aTime.isValid() && bTime.isValid())
			return aTime < bTime;

		if (aTime.isValid() != bTime.isValid())
			return aTime.isValid();

		return a.idx < b.idx;
	}

}

void SOOPMediaSourceManager::InitContext()
{
	_InitTvLiveLists();

	_InitVodConfigFile();

	m_tvLiveBlindTimer.setInterval(30000);
	connect(&m_tvLiveBlindTimer, &QTimer::timeout, this, &SOOPMediaSourceManager::_qslotTvSourceBlindTimer);
	connect(&m_tvLiveStartTimer, &QTimer::timeout, this, &SOOPMediaSourceManager::_qslotTvLiveStartTimer);
	connect(&m_timerNextVod, &QTimer::timeout, this, &SOOPMediaSourceManager::_qslotPlayNextVodTimer);
	connect(&m_timerDirectBroad, &QTimer::timeout, this, &SOOPMediaSourceManager::_qslotPlayDirectBroadTimer);

	RequestVODContents(SOOP_VOD_TYPE::ANIME);
	RequestVODContents(SOOP_VOD_TYPE::DRAMA);
	RequestVODContents(SOOP_VOD_TYPE::MOVIE);
	RequestVODContents(SOOP_VOD_TYPE::SPORT);
}

void SOOPMediaSourceManager::SetSoopMediaSource(OBSSource source, bool removeSource)
{
	m_signals.clear();

	if (source)
	{
		m_weakSource = OBSGetWeakRef(source);

		signal_handler_t* sh = obs_source_get_signal_handler(source);
		m_signals.emplace_back(sh, "media_play", SOOPMediaSourceManager::OBSMediaPlay, this);
		m_signals.emplace_back(sh, "media_pause", SOOPMediaSourceManager::OBSMediaPause, this);
		m_signals.emplace_back(sh, "media_stopped", SOOPMediaSourceManager::OBSMediaStopped, this);
		m_signals.emplace_back(sh, "media_started", SOOPMediaSourceManager::OBSMediaStarted, this);
		m_signals.emplace_back(sh, "media_ended", SOOPMediaSourceManager::OBSMediaEnded, this);
		m_signals.emplace_back(sh, "media_file_load", SOOPMediaSourceManager::FSMediaFileLoaded, this);

		QString id = obs_source_get_id(source);
		if (0 == id.compare("soop_directbroad_source"))
		{
			if (0 != m_currentIdx)
				RequestDirectBroadOneTimeUrl_2(m_currentIdx);
		}
		else if (0 == id.compare("soop_tv_cable_source")) {
			m_signals.emplace_back(sh, "vlc_restart_requested", SOOPMediaSourceManager::OBSVlcRestartRequested, this);
			bool requestTvLive = (m_currentCPNo != 0);

			m_tvLiveRestartPending = false;
			++m_tvLiveRefreshSequence;

			if (requestTvLive && AFOutputUtil::IsStreamActive()) {
				AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
				const int categoryNo = 390000 + m_currentCPNo;

				requestTvLive =
					broadInfo && broadInfo->CategoryNumber() == categoryNo;
			}

			if (requestTvLive)
				RequestTvLiveOneTimeUrlWithGeoBlock(m_currentCPNo);
		}
	}
	else {
		OBSSource source = GetSoopMediaSource();
		if (source) {
			std::string id = obs_source_get_id(source);
			if (0 == id.compare("soop_directbroad_source")) {
				m_currentIdx = 0;
				m_currentURL = "";
				if (m_timerDirectBroad.isActive())
					m_timerDirectBroad.stop();
			}

			if (0 == id.compare("soop_tv_cable_source")) {
				if (m_tvLiveBlindTimer.isActive())
					m_tvLiveBlindTimer.stop();

				m_currentCPNo = -1;
			}

			if (0 == id.compare("soop_anivod_source")   ||
				0 == id.compare("soop_sportvod_source") ||
				0 == id.compare("soop_dramavod_source") ||
				0 == id.compare("soop_movievod_source")) {

				SOOP_VOD_TYPE  type = GetSoopVodSourceType(id.c_str());
				SetCurVodInfo(type, VodInfo_s());

				if (m_timerNextVod.isActive())
					m_timerNextVod.stop();

				if (removeSource) {
					SetRecentlyVodInfo(type, VodInfo_s());
				}
			}
		}
		m_weakSource = nullptr;
	}
}

OBSSource SOOPMediaSourceManager::GetSoopMediaSource()
{
	return OBSGetStrongRef(m_weakSource);
}

void SOOPMediaSourceManager::SetSoopMediaSourceToolBar(QWidget* toolbar)
{
	m_pSoopSourceToolbar = toolbar;
}

void SOOPMediaSourceManager::SetSoopMediaSourceProps(QDialog* props)
{
	m_pSoopSourceProps = props;
}

static bool find_soop_source(obs_scene_t* , obs_sceneitem_t* item, void* param)
{
	obs_source_t* source = obs_sceneitem_get_source(item);

	if (!source) {
		return true; 
	}
	if (AFSourceUtil::IsSoopMediaSource(source) || 
		AFSourceUtil::IsSoopKBOSource(source) || 
		AFSourceUtil::IsSoopFootballSource(source)) {
		OBSSceneItem& found_item = *reinterpret_cast<OBSSceneItem*>(param);
		found_item = item;
		return false;
	}
	return true;
};

// need KBO check code separation
void SOOPMediaSourceManager::ForceStopSoopSource()
{
	OBSScene curScene = SCENE_CONTEXT.GetCurrentScene();

	OBSSceneItem item;
	obs_scene_enum_items(curScene, find_soop_source, &item);

	bool forceStop = false;
	if (item) {
		obs_source_t* source = obs_sceneitem_get_source(item);

		const char* id = obs_source_get_id(source);
		std::string name;
		if (AFSourceUtil::IsSoopMediaSource(source, true))
		{
			obs_media_state media_state = obs_source_media_get_state(source);
			if (media_state == OBS_MEDIA_STATE_PLAYING ||
				media_state == OBS_MEDIA_STATE_OPENING ||
				media_state == OBS_MEDIA_STATE_BUFFERING ||
				media_state == OBS_MEDIA_STATE_PAUSED)
			{
				obs_source_media_stop(source);
				StopTvLiveBlind();
				MAINFRAME->HideSourceProperties(source);
				m_currentCPNo = -1;
				m_currentIdx = 0;

				forceStop = true;
			}
			name = obs_source_get_display_name(id);
		}
		else if(AFSourceUtil::IsSoopKBOSource(source))
		{
			OBSDataAutoRelease settings = obs_source_get_settings(source);
			std::string url = obs_data_get_string(settings, "url");
			if (!url.empty()) {
				obs_data_set_string(settings, "url", "");
				obs_source_update(source, settings);

				forceStop = true;
			}
			name = QTStr("Basic.SelectedSourcePopup.KBOSource").toStdString();
		}
		else if (AFSourceUtil::IsSoopFootballSource(source))
		{
			OBSDataAutoRelease settings = obs_source_get_settings(source);
			std::string url = obs_data_get_string(settings, "url");
			if (!url.empty()) {
				obs_data_set_string(settings, "url", "");
				obs_source_update(source, settings);

				forceStop = true;
			}
			name = QTStr("Basic.SelectedSourcePopup.FootballSource").toStdString();
		}
		else {
			return;
		}

		if (forceStop)
		{
			QString topmsg = QTStr("Caution.SOOPMediaSource.DisableMessage0") .arg(name.c_str());
			QString msg = QTStr("Caution.SOOPMediaSource.DisableMessage1") .arg(name.c_str());

			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
									   "", msg, false, true, topmsg, 420, 200);
		}
	}
}

void SOOPMediaSourceManager::StopTvLiveBlind()
{
	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	if (m_tvLiveBlindTimer.isActive()) {
		m_tvLiveBlindTimer.stop();
		QString id = obs_source_get_id(source);
		if (0 != id.compare("soop_tv_cable_source"))
			return;
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		if (obs_data_get_bool(settings, "show_blind")) {
			obs_data_set_bool(settings, "show_blind", false);
			obs_data_set_bool(settings, "make_blind", false);
			obs_source_update(source, settings);
		}		
	}
}

void SOOPMediaSourceManager::RequestTvLiveOneTimeUrl(int cpNo)
{
	m_currentCPNo = cpNo;

	AFChannelData* pSoopChannel = nullptr;
	AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannel);
	if (pSoopChannel)
	{
		const char* soop_id = pSoopChannel->pAuthData->channelID.c_str();

		QList<QVariant> values = { };
		QList<int> additionalData = { cpNo, (int)TvLiveRequestType::PlayUrl };
		SOOP_API_HANDLER->postAPIfromId(GET_TVBROAD_ONETIME_URL, values,
			this, "_qslotTvSourceOneTimeUrlAPIData", additionalData);
	}
}
void SOOPMediaSourceManager::RequestTvLiveOneTimeUrlWithGeoBlock(int cpNo)
{
	const int categoryNo = 390000 + cpNo;

	QList<QVariant> queryValues;
	QList<int>      additionalData = { cpNo, categoryNo };

	SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_BROAD_GEO_BLOCK, queryValues,
		this, "_qslotTvLiveGeoBlockCheckAPIResponse", additionalData);
}

void SOOPMediaSourceManager::RequestTvLiveBlindCheck()
{
	AFChannelData* pSoopChannel = nullptr;
	AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannel);
	if (pSoopChannel)
	{
		const char* soop_id = pSoopChannel->pAuthData->channelID.c_str();

		QList<QVariant> values = { };
		QList<int> additionalData = { m_currentCPNo, (int)TvLiveRequestType::BlindCheck };
		SOOP_API_HANDLER->postAPIfromId(GET_TVBROAD_ONETIME_URL, values,
			this, "_qslotTvSourceOneTimeUrlAPIData", additionalData);
	}
}

void SOOPMediaSourceManager::RequestVODContents(int contentType)
{
	QList<QVariant> queryValues = { };
	QList<int>		additionalData = { contentType };
	SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_VOD_CONTENT_LIST, queryValues,
		this, "_qslotHandleContentListAPIData", additionalData);
}

void SOOPMediaSourceManager::RequestVODContentList(int contentType, int contentIdx, int recentlyListLoad)
{
	QList<QVariant> queryValues = { };
	QList<int>		additionalData = { contentType, contentIdx, recentlyListLoad };
	SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_VOD_LIST, queryValues,
		this, "_qslotHandleVodListAPIData", additionalData);
}

void SOOPMediaSourceManager::RequestDirectBroadList()
{
	SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_DIRECTBROAD_DATA, QList<QVariant>(),
		this, "_qslotHandleDirectBroadDataAPI");
}

void SOOPMediaSourceManager::RequestDirectBroadOneTimeUrl(int idx)
{
	QList<QVariant> queryValues = { };
	QList<int>		additionalData = { idx };
	SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_DIRECTBROAD_ONETIME_URL, queryValues,
		this, "_qslotDirectBroadOneTimeUrlDataAPI", additionalData);
}


void SOOPMediaSourceManager::RequestDirectBroadOneTimeUrl_2(int idx)
{
	QList<QVariant> queryValues = {  };
	QList<int>		additionalData = { idx };
	SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_DIRECTBROAD_ONETIME_URL, queryValues,
		this, "_qslotDirectBroadOneTimeUrlDataAPI_2", additionalData);
}

void SOOPMediaSourceManager::RequestDirectBroadOneTimeUrlWithGeoBlock(int idx)
{
	// GET_BROAD_GEO_BLOCK : {"user_nation":"KR","cate_no":["00040086","00060000", ...]}
	QList<QVariant> queryValues;
	QList<int>      additionalData = { idx };

	SOOP_API_HANDLER->getAPIfromId( SOOP_API_KEY::GET_BROAD_GEO_BLOCK, queryValues,	
		this, "_qslotDirectBroadGeoBlockForDirectAPIData", additionalData);
}

std::vector<VodContentInfo_s>& SOOPMediaSourceManager::GetSoopVodContents(SOOP_VOD_TYPE type)
{
	return m_contents[type];
}

std::vector<QString>& SOOPMediaSourceManager::GetSoopVodContentSeason(SOOP_VOD_TYPE type)
{
	return m_seasons[type];
}

std::vector<VodInfo_s>& SOOPMediaSourceManager::GetSoopVodLists(SOOP_VOD_TYPE type)
{
	return m_vodLists[type];
}


std::vector<VodInfo_s>& SOOPMediaSourceManager::GetSoopVodPlayLists(SOOP_VOD_TYPE type)
{
	return m_vodPlayLists[type];
}


bool SOOPMediaSourceManager::GetSoopVodAdultContentCheck(int category)
{
	bool audltCategory = (GetSoopVodAdultContentCheck(SOOP_VOD_TYPE::ANIME, category) ||
						  GetSoopVodAdultContentCheck(SOOP_VOD_TYPE::DRAMA, category) ||
						  GetSoopVodAdultContentCheck(SOOP_VOD_TYPE::MOVIE, category));

	return audltCategory;
}

bool SOOPMediaSourceManager::GetSoopVodAdultContentCheck(SOOP_VOD_TYPE type, int category)
{
	QString compareCateNum;
	if (SOOP_VOD_TYPE::ANIME == type) {
		compareCateNum = "0035";
	}
	else if(SOOP_VOD_TYPE::DRAMA == type) {
		compareCateNum = "0094";
	}
	else if (SOOP_VOD_TYPE::MOVIE == type) {
		compareCateNum = "0096";
	}
	else {
		return false;
	}

	QString categoryFormat = QString("%1").arg(category, 8, 10, QLatin1Char('0'));
	QString categoryCheckNum = categoryFormat.mid(0, 4);
	if (0 == categoryCheckNum.compare(compareCateNum))
	{
		bool isAdultCategory = false;
		std::vector<VodContentInfo_s> aniContents = SOOP_SRC_MANAGER.GetSoopVodContents(type);
		for (size_t idx = 0; idx < aniContents.size(); idx++)
		{
			if (category == aniContents[idx].categoryNum) {
				isAdultCategory = aniContents[idx].is_adult;
				break;
			}
		}

		if (isAdultCategory) {
			return true;
		}
	}

	return false;
}

VodInfo_s SOOPMediaSourceManager::GetCurVodInfo(SOOP_VOD_TYPE type)
{
	return m_curVodInfo[type];
}

VodInfo_s SOOPMediaSourceManager::GetCurVodInfo(const char* vod_source_id)
{
	std::string id = vod_source_id;
	if (0 == id.compare("soop_anivod_source")) {
		return m_curVodInfo[SOOP_VOD_TYPE::ANIME];
	}
	else if (0 == id.compare("soop_sportvod_source")) {
		return m_curVodInfo[SOOP_VOD_TYPE::SPORT];
	}
	else if (0 == id.compare("soop_dramavod_source")) {
		return m_curVodInfo[SOOP_VOD_TYPE::DRAMA];
	}
	else if (0 == id.compare("soop_movievod_source")) {
		return m_curVodInfo[SOOP_VOD_TYPE::MOVIE];
	}
	else {
		return VodInfo_s();
	}
}

void SOOPMediaSourceManager::SetCurVodInfo(SOOP_VOD_TYPE type, VodInfo_s info)
{
	m_curVodInfo[type] = info;
}

void SOOPMediaSourceManager::SetRecentlyVodInfo(SOOP_VOD_TYPE type, VodInfo_s info)
{
	QString vodType = QString("VodType_%1").arg(type);
	config_set_int(m_configSoopVOD, vodType.toStdString().c_str(), "content_idx", info.contentIdx);
	config_set_string(m_configSoopVOD, vodType.toStdString().c_str(), "content_title", info.contentTitle.toStdString().c_str());
	config_set_string(m_configSoopVOD, vodType.toStdString().c_str(), "season", info.seasonTitle.toStdString().c_str());
	config_set_string(m_configSoopVOD, vodType.toStdString().c_str(), "vod_title", info.vodTitle.toStdString().c_str());
	config_set_int(m_configSoopVOD, vodType.toStdString().c_str(), "vod_idx", info.vodIdx);

	config_save_safe(m_configSoopVOD, "tmp", nullptr);
}

VodInfo_s SOOPMediaSourceManager::GetRecentlyVodInfo(SOOP_VOD_TYPE type)
{
	QString vodType = QString("VodType_%1").arg(type);

	VodInfo_s info = {};
	info.contentIdx = config_get_int(m_configSoopVOD, vodType.toStdString().c_str(), "content_idx");
	info.contentTitle = config_get_string(m_configSoopVOD, vodType.toStdString().c_str(), "content_title");
	info.seasonTitle = config_get_string(m_configSoopVOD, vodType.toStdString().c_str(), "season");
	info.vodTitle = config_get_string(m_configSoopVOD, vodType.toStdString().c_str(), "vod_title");
	info.vodIdx = config_get_int(m_configSoopVOD, vodType.toStdString().c_str(), "vod_idx");

	return info;
}

SOOP_VOD_TYPE SOOPMediaSourceManager::GetSoopVodSourceType(QString id)
{
	if (0 == id.compare("soop_anivod_source"))
		return SOOP_VOD_TYPE::ANIME;
	else if (0 == id.compare("soop_sportvod_source"))
		return SOOP_VOD_TYPE::SPORT;
	else if (0 == id.compare("soop_dramavod_source"))
		return SOOP_VOD_TYPE::DRAMA;
	else if (0 == id.compare("soop_movievod_source"))
		return SOOP_VOD_TYPE::MOVIE;

	return SOOP_VOD_TYPE::NONE;
}

const char* SOOPMediaSourceManager::GetSoopVodSourceId(SOOP_VOD_TYPE type)
{
	const char* soop_source_id;
	if (SOOP_VOD_TYPE::ANIME == type)
		return "soop_anivod_source";
	else if (SOOP_VOD_TYPE::SPORT == type)
		return "soop_sportvod_source";
	else if (SOOP_VOD_TYPE::DRAMA == type)
		return "soop_dramavod_source";
	else if (SOOP_VOD_TYPE::MOVIE == type)
		return "soop_movievod_source";

	return "";
}

void SOOPMediaSourceManager::SetRepeatSeries(SOOP_VOD_TYPE type, bool repeat)
{
	m_isVodSeasonRepeat[type] = repeat;
}

void SOOPMediaSourceManager::SetRepeatVodList(SOOP_VOD_TYPE type, bool repeat)
{
	m_isVodListRepeat[type] = repeat;
}

bool SOOPMediaSourceManager::GetRepeatSeries(SOOP_VOD_TYPE type)
{
	return m_isVodSeasonRepeat[type];
}

bool SOOPMediaSourceManager::GetRepeatVodList(SOOP_VOD_TYPE type)
{
	return m_isVodListRepeat[type];
}

void SOOPMediaSourceManager::PlayVodMedia(SOOP_VOD_TYPE type, VodInfo_s info, bool selectedItem)
{
	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;
	
	m_pendingVodType = type;
	m_pendingVodInfo = info;
	m_pendingSelectedItem = selectedItem;

	if (info.allowed_category > 0) {

		QList<QVariant> queryValues;
		QList<int>      additionalData;
		additionalData.push_back(info.allowed_category);

		SOOP_API_HANDLER->getAPIfromId(
			SOOP_API_KEY::GET_BROAD_GEO_BLOCK, queryValues, this, "_qslotVodGeoBlockCheckAPIResponse", additionalData);

		return;
	}
}

void SOOPMediaSourceManager::_ApplyVodPlayAfterGeoCheck()
{
	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	if (m_pendingVodType == SOOP_VOD_TYPE::NONE)
		return;

	SOOP_VOD_TYPE type = m_pendingVodType;
	VodInfo_s     info = m_pendingVodInfo;
	bool          selectedItem = m_pendingSelectedItem;

	
	SetCurVodInfo(type, info);
	SetRecentlyVodInfo(type, info);
		
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_string(settings, "input", info.hls.toStdString().c_str());
	obs_source_update(source, settings);

	bool is_changed_name = obs_data_get_bool(settings, "is_changed_name");
	if (!is_changed_name)
	{
		QString name = QString("%1 - %2").arg(info.seasonTitle).arg(info.vodTitle);

		std::string prevName = obs_source_get_name(source);
		std::string newName = name.toStdString();

		UNDO_STACK.AddActionRename(prevName, newName, source);

		obs_source_set_name(source, name.toStdString().c_str());
	}

	if (selectedItem) {
		std::vector<VodInfo_s>& vecplayLists = GetSoopVodPlayLists(type);
		std::vector<VodInfo_s>& vecLists = GetSoopVodLists(type);

		vecplayLists = vecLists;
	}

	m_pendingVodType = SOOP_VOD_TYPE::NONE;
	m_pendingVodInfo = VodInfo_s();
	m_pendingSelectedItem = false;
}


void SOOPMediaSourceManager::_qslotVodGeoBlockCheckAPIResponse(const QByteArray& responseData, int allowedCategory)
{
	std::string jsonString = responseData.toStdString();
	std::string parseError;

	_ApplyVodPlayAfterGeoCheck();
}


void SOOPMediaSourceManager::PlayNextVOD(SOOP_VOD_TYPE type)
{
	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	VodInfo_s curVodInfo = GetCurVodInfo(type);
	VodInfo_s nextVodInfo = {};

	std::vector<VodInfo_s>& vecVodLists = GetSoopVodPlayLists(type);

	auto it = vecVodLists.begin();
	for (; it != vecVodLists.end(); ++it) {
		if (0 == (*it).hls.compare(curVodInfo.hls))
			break;
	}

	if (it != vecVodLists.end()) {
		auto nextIt = std::next(it);
		if (nextIt == vecVodLists.end()) {
			if (m_isVodSeasonRepeat[type]) {

				nextVodInfo = vecVodLists.front();

				if (m_pSoopSourceProps) {
					QMetaObject::invokeMethod(m_pSoopSourceProps, "qslotAddContentSeasonVodItems",
						Q_ARG(QString, nextVodInfo.seasonTitle));
					return;
				}
			}
			else {
				if (m_isVodListRepeat[type]) {
					nextVodInfo = FindFirstInSeason(type, curVodInfo.seasonTitle);
				}
				else {
					qslotRecvOBSMediaStoped();
					return;
				}
			}
		}
		else {
			if (m_isVodSeasonRepeat[type] && nextIt->seasonTitle != curVodInfo.seasonTitle) {

				nextVodInfo = FindFirstInSeason(type, nextIt->seasonTitle);

				if (m_pSoopSourceProps) {
					QMetaObject::invokeMethod(m_pSoopSourceProps, "qslotAddContentSeasonVodItems",
						Q_ARG(QString, nextIt->seasonTitle));
					return;
				}

			} else {

				if (nextIt->seasonTitle != curVodInfo.seasonTitle) {
					if (m_isVodListRepeat[type]) {
						nextVodInfo = FindFirstInSeason(type, curVodInfo.seasonTitle);
					}
					else {
						qslotRecvOBSMediaStoped();
						return;
					}
				}
				else {
					nextVodInfo = (*nextIt);
				}
			}
		}
	}
	PlayVodMedia(type, nextVodInfo);
}

void SOOPMediaSourceManager::PlayPrevVOD(SOOP_VOD_TYPE type)
{
	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	VodInfo_s curVodInfo = GetCurVodInfo(type);
	VodInfo_s prevVodInfo = {};

	std::vector<VodInfo_s>& vecVodLists = GetSoopVodPlayLists(type);

	auto it = vecVodLists.begin();
	for (; it != vecVodLists.end(); ++it) {
		if (0 == (*it).hls.compare(curVodInfo.hls))
			break;
	}

	if (it != vecVodLists.end()) {
		if (it == vecVodLists.begin()) {
			if (m_isVodListRepeat[type]) {

				prevVodInfo = vecVodLists.back();

				if (m_pSoopSourceProps) {
					QMetaObject::invokeMethod(m_pSoopSourceProps, "qslotAddContentSeasonVodItems",
						Q_ARG(QString, prevVodInfo.seasonTitle));
					return;
				}

			} else {
				qslotRecvOBSMediaStoped();
				return;
			}
		}
		else {
			auto prevIt = std::prev(it);
			if (m_isVodSeasonRepeat[type] && prevIt->seasonTitle != curVodInfo.seasonTitle) {
				prevVodInfo = FindLastInSeason(type, prevIt->seasonTitle);

				if (m_pSoopSourceProps) {
					QMetaObject::invokeMethod(m_pSoopSourceProps, "qslotAddContentSeasonVodItems",
						Q_ARG(QString, prevIt->seasonTitle));
					return;
				}
			}
			else {

				if (prevIt->seasonTitle != curVodInfo.seasonTitle) {
					if (m_isVodListRepeat[type]) {
						prevVodInfo = FindLastInSeason(type, curVodInfo.seasonTitle);
					}
					else {
						qslotRecvOBSMediaStoped();
						return;
					}
				}
				else {
					prevVodInfo = (*prevIt);
				}

			}
		}
	}
	PlayVodMedia(type, prevVodInfo);
}

VodInfo_s SOOPMediaSourceManager::FindFirstInSeason(SOOP_VOD_TYPE type, QString season)
{
	std::vector<VodInfo_s>& vecLists = GetSoopVodPlayLists(type);

	for (const auto& ep : vecLists) {
		if (ep.seasonTitle == season) {
			return ep;
		}
	}
	return vecLists.front();
}

VodInfo_s SOOPMediaSourceManager::FindLastInSeason(SOOP_VOD_TYPE type, QString season)
{
	std::vector<VodInfo_s>& vecLists = GetSoopVodPlayLists(type);

	for (auto it = vecLists.rbegin(); it != vecLists.rend(); ++it) {
		if (it->seasonTitle == season) {
			return *it;
		}
	}
	return vecLists.back();
}

void SOOPMediaSourceManager::_InitTvLiveLists()
{

}

void SOOPMediaSourceManager::_InitVodConfigFile()
{
	char path[512];
	int ret = GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/basic/vod_source").c_str());

	if (os_file_exists(path) == false)
		os_mkdir(path);

	std::string vodConfigPath = path;
	vodConfigPath += "/vodConfig.ini";

	ret = m_configSoopVOD.Open(vodConfigPath.c_str(), CONFIG_OPEN_ALWAYS);
}

bool SOOPMediaSourceManager::MakeTvSourceBlindImage(const QString& BasePath,
													const QString& resultPath,
													const char* program,
													const char* blindTime)
{
	QImage image(1920, 1080, QImage::Format_ARGB32);
	image.fill(Qt::white);

	QPainter painter(&image);

	QString imagePath = BasePath;
	QImage loadedImage(imagePath);

	if (!loadedImage.isNull()) {

		painter.drawImage(0, 0, loadedImage);

		QFont font;
		font.setPointSize(50);
		font.setWeight(QFont::Bold);

		painter.setFont(font);
		painter.setPen(Qt::white);
		painter.setBackgroundMode(Qt::OpaqueMode);
		painter.setBackground(QBrush(QColor(33, 33, 33)));

		painter.drawText(150, 790, QTStr("Popup.TvLive.BlindProgram"));
		painter.drawText(650, 790, program);

		painter.drawText(150, 920, QTStr("Popup.TvLive.BlindTime"));
		painter.drawText(650, 920, blindTime);

		return image.save(resultPath);
	}

	return false;
}

void SOOPMediaSourceManager::_qslotTvSourceBlindTimer()
{
	RequestTvLiveBlindCheck();
}

void SOOPMediaSourceManager::_qslotTvLiveStartTimer()
{
	if (m_tvLiveStartTimer.isActive())
		m_tvLiveStartTimer.stop();

	OBSSource source = GetSoopMediaSource();
	if (!source) {
		m_tvLiveRestartPending = false;
		return;
	}

	QString id = obs_source_get_id(source);
	if (0 != id.compare("soop_tv_cable_source")) {
		m_tvLiveRestartPending = false;
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);

	if (m_refreshblind)
	{
		obs_source_media_stop(source);

		char makeBlindImagePath[512];
		std::string fileName, resultFileName;
		GetDataFilePath("assets/blindImage/rtmpstandbytemp.png", fileName);
		resultFileName = "SOOPStudio/res/rtmpstandbytemp_Result.png";

		GetAppConfigPath(makeBlindImagePath, sizeof(makeBlindImagePath),
			resultFileName.c_str());

		QString qFilePath(makeBlindImagePath);
		QDir dir(QFileInfo(qFilePath).absolutePath());
		if (!dir.exists()) {
			dir.mkpath(".");
		}

		QString blindBaseImagePath = fileName.c_str();

		MakeTvSourceBlindImage(blindBaseImagePath, makeBlindImagePath,
			m_blindProgram.toStdString().c_str(), m_blindDuration.toStdString().c_str());

		obs_data_set_bool(settings, "show_blind", true);
		obs_data_set_bool(settings, "make_blind", true);
		obs_data_set_string(settings, "blind_image_path", makeBlindImagePath);
		obs_data_set_string(settings, "input", "");
		obs_source_update(source, settings);
		m_refreshblind = false;
	}
	else
	{
		if (!m_tvLiveUrl.isEmpty())
		{
			obs_data_set_bool(settings, "show_blind", false);
			obs_data_set_bool(settings, "make_blind", false);
			obs_data_set_string(settings, "blind_image_path", "");
			obs_data_set_string(settings, "input", m_tvLiveUrl.toStdString().c_str());
			obs_source_update(source, settings);			
		}
	}

	obs_data_set_int(settings, "cpNo", m_currentCPNo);
	m_tvLiveRestartPending = false;
	//
	auto it = m_tvLists.begin();
	for (; it != m_tvLists.end(); ++it) {
		if (m_currentCPNo == (*it).cpNo) {

			bool is_changed_name = obs_data_get_bool(settings, "is_changed_name");
			if (!is_changed_name)
			{
				QString name = QTStr("Popup.TvLive.SourceNameFormat")
					.arg((*it).cpTitle);

				std::string prevName = obs_source_get_name(source);
				std::string newName = name.toStdString();

				UNDO_STACK.AddActionRename(prevName, newName, source);

				obs_source_set_name(source, name.toStdString().c_str());
			}
			break;
		}
	}

	emit qsignalResponseTvLiveOneTimeUrl(m_currentCPNo);
}

void SOOPMediaSourceManager::_qslotTvSourceOneTimeUrlAPIData(const QByteArray& responseData, int requestcpNo, int requestType)
{
	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	QString id = obs_source_get_id(source);
	if (0 != id.compare("soop_tv_cable_source"))
		return;

	if (m_tvLiveRestartPending &&
		requestType ==
		(int)TvLiveRequestType::BlindCheck) {
		blog(LOG_INFO,
			"[SOOPMediaSourceManager] "
			"Ignore blind-check response "
			"while refreshing TV URL");
		return;
	}

	QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
	if (!jsonResponse.isObject())
		return;

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	bool show_blind = obs_data_get_bool(settings, "show_blind");
	QJsonObject jsonObj = jsonResponse.object();
	if (1 == jsonObj["BLIND"].toInt())
	{
		m_tvLiveUrl = "";
		if (!show_blind)
		{
			m_refreshblind = true;
			m_blindProgram = jsonObj["NAME"].toString();
			m_blindDuration = jsonObj["DURATION"].toString();
		}
		else {
			if (m_blindProgram != jsonObj["NAME"].toString() || m_blindDuration != jsonObj["DURATION"].toString()) {
				m_refreshblind = true;
				m_blindProgram = jsonObj["NAME"].toString();
				m_blindDuration = jsonObj["DURATION"].toString();				
			}
		}
	}
	else
	{
		if (show_blind) {
			StopTvLiveBlind();
			RequestTvLiveOneTimeUrl(requestcpNo);
			return;
		}
		m_refreshblind = false;
		m_blindProgram = "";
		m_blindDuration = "";
		m_tvLiveUrl = jsonObj["DATA"].toString();
	}

	m_currentCPNo = requestcpNo;

	if (!m_tvLiveBlindTimer.isActive())
		m_tvLiveBlindTimer.start();

	if (!m_tvLiveStartTimer.isActive())
		m_tvLiveStartTimer.start(500);
}

void SOOPMediaSourceManager::_qslotTvLiveGeoBlockCheckAPIResponse(const QByteArray& responseData, int cpNo, int categoryNo)
{
	std::string jsonString = responseData.toStdString();
	std::string err;

	SOOP_SRC_MANAGER.RequestTvLiveOneTimeUrl(cpNo);
}

void SOOPMediaSourceManager::_qslotInitAnimationCategoryInfoAPIData(const QByteArray& responseData)
{
	std::vector<VodContentInfo_s>& contents = GetSoopVodContents(SOOP_VOD_TYPE::ANIME);
	contents.clear();

	QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
	if (!jsonResponse.isArray())
		return;

	QJsonArray jsonArray = jsonResponse.array();
	for (const QJsonValue& value : jsonArray) {
		if (value.isObject()) {
			QJsonObject obj = value.toObject();

			VodContentInfo_s content = {  };
			contents.push_back(content);
		}
	}
}

void SOOPMediaSourceManager::_qslotPlayNextVodTimer()
{
	m_timerNextVod.stop();

	OBSSource source = GetSoopMediaSource();
	if (!source) {
		return;
	}

	const char* id = obs_source_get_id(source);
	SOOP_VOD_TYPE vodSourceType = GetSoopVodSourceType(id);

	PlayNextVOD(vodSourceType);
}

void SOOPMediaSourceManager::_qslotHandleContentListAPIData(const QByteArray& responseData, int contentType)
{
	std::vector<VodContentInfo_s>& contents = GetSoopVodContents((SOOP_VOD_TYPE)contentType);
	contents.clear();

	QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
	if (!jsonResponse.isArray()) {
		emit qsignalRefreshVODContents();
		return;
	}

	QJsonArray jsonArray = jsonResponse.array();
	for (const QJsonValue& value : jsonArray) {
		if (value.isObject()) {
			QJsonObject obj = value.toObject();

			VodContentInfo_s content = {  };
			contents.push_back(content);
		}
	}
	emit qsignalRefreshVODContents();
}

void SOOPMediaSourceManager::_qslotHandleVodListAPIData(const QByteArray& responseData, int contentType, int contentIdx, int recentlyListLoad)
{
	SOOP_VOD_TYPE type = (SOOP_VOD_TYPE)contentType;

	std::vector<QString>& vecSeasons = GetSoopVodContentSeason(type);
	std::vector<VodInfo_s>& vecVodLists = GetSoopVodLists(type);

	vecSeasons.clear();
	vecVodLists.clear();

	int  allowed_category = 0;
	bool is_adult_content = false;
	std::vector<VodContentInfo_s>& contents = GetSoopVodContents(type);
	auto it = contents.begin();
	for (; it != contents.end(); ++it) {
		if (contentIdx == (*it).contentIdx) {
			allowed_category = (*it).categoryNum;
			is_adult_content = (*it).is_adult;
			break;
		}
	}
	if (it == contents.end())
		return;

	QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);
	if (!jsonResponse.isObject())
		return;

	QJsonObject jsonObj = jsonResponse.object();

	QString content = jsonObj.keys().first();
	if (!jsonObj[content].isObject())
		return;

	QJsonObject contentObj = jsonObj[content].toObject();
	QStringList seasonLists = contentObj.keys();

	int idx = 0;
	for (const QString& season : seasonLists) {
		
		idx = 0;
		
		if (contentObj[season].isArray()) {

			QJsonArray array = contentObj[season].toArray();

			if(array.size() > 0)
				vecSeasons.push_back(season);

			for (const QJsonValue& value : array) {
				if (value.isObject()) {

					VodInfo_s vod = {};
					vecVodLists.push_back(vod);

					idx++;
				}
			}
		}
	}
	emit qsignalRefreshVODSeasons(content, contentIdx, recentlyListLoad);
}

void SOOPMediaSourceManager::_qslotPlayDirectBroadTimer()
{
	if (m_timerDirectBroad.isActive())
		m_timerDirectBroad.stop();

	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	QString id = obs_source_get_id(source);
	if (0 != id.compare("soop_directbroad_source"))
		return;

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_string(settings, "input", m_currentURL.toStdString().c_str());

	obs_source_update(source, settings);
}

void SOOPMediaSourceManager::_qslotHandleDirectBroadDataAPI(const QByteArray& responseData)
{
	m_dates.clear();
	m_directBroadInfos.clear();

	QJsonDocument doc = QJsonDocument::fromJson(responseData);
	if (doc.isNull() || !doc.isObject())
		return;

	QJsonObject jsonObj = doc.object();

	int result = jsonObj["RESULT"].toInt();
	if (1 != result)
		return;

	QJsonObject dataObj = jsonObj["DATA"].toObject();
	if (dataObj.isEmpty()) {
		emit qsignalRefreshDirectBroadList();
		return;
	}

	for (auto it = dataObj.constBegin(); it != dataObj.constEnd(); ++it) {
		const QString dateKey = it.key();
		const QJsonArray dateArray = it.value().toArray();

		m_dates.push_back(dateKey);

		std::vector<DirectBroadInfo_s> sortedDirectBroads;
		sortedDirectBroads.reserve(dateArray.size());

		for (const QJsonValue& value : dateArray) {
			const QJsonObject item = value.toObject();

			DirectBroadInfo_s info;
			sortedDirectBroads.push_back(info);
		}

		std::sort(
			sortedDirectBroads.begin(),
			sortedDirectBroads.end(),
			LessDirectBroadInfo
		);

		m_directBroadInfos.insert(
			m_directBroadInfos.end(),
			sortedDirectBroads.begin(),
			sortedDirectBroads.end()
		);
	}

	std::sort(m_dates.begin(), m_dates.end());

	emit qsignalRefreshDirectBroadList();
}

void SOOPMediaSourceManager::_qslotDirectBroadOneTimeUrlDataAPI(const QByteArray& responseData, int requestIdx)
{
	QJsonDocument doc = QJsonDocument::fromJson(responseData);
	if (doc.isNull() || !doc.isObject())
		return;

	QJsonObject jsonObj = doc.object();

	int result = jsonObj["RESULT"].toInt();
	if (1 != result)
		return;

	QJsonObject dataObj = jsonObj["DATA"].toObject();

	QString allowedCateLists = dataObj["allowed_cate_no"].toString();
	QString url = dataObj["url"].toString();

	std::list<int> categorys;
	QStringList categoryNums = allowedCateLists.split(',', Qt::SkipEmptyParts);
	for (QString& strCategoryNum : categoryNums) {
		int categoryNo = strCategoryNum.trimmed().toInt();
		categorys.push_back(categoryNo);
	}

	if (!m_directBroadGeoBlockedCategory.empty() && !categorys.empty()) {
		bool blocked = false;

		for (int allowedCate : categorys) {
			for (int blockedCate : m_directBroadGeoBlockedCategory) {
				if (allowedCate == blockedCate) {
					blocked = true;
					break;
				}
			}
			if (blocked)
				break;
		}

		if (blocked) {
			QString msg = QTStr("Basic.GeoBlock.Blocked");
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", msg, false, true, "", 0, 0, "type1");
			return;
		}
	}

	const char* directbroad_id = "soop_directbroad_source";

	if (AFOutputUtil::IsStreamActive())
	{
		AFQBroadInfo* soopBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
		if (soopBroadInfo)
		{
			bool findAllowedCategory = false;
			for (auto it = categorys.begin(); it != categorys.end(); ++it) {
				if (soopBroadInfo->CategoryNumber() == (*it)) {
					findAllowedCategory = true;
				}
			}

			if (!findAllowedCategory)
			{
				QWidget* parent = m_pSoopSourceProps;
				if (!m_pSoopSourceProps)
					parent = MAINFRAME;

				AFQCateChangeDialog dlg(parent, directbroad_id);
				dlg.AddAllowedCategoryInfo(categorys);

				if (QDialog::Accepted != dlg.exec())
					return;

				int selectedCategoryNum = dlg.GetSelectedCategory();
				soopBroadInfo->SetCategory(selectedCategoryNum);
				AUTH_CONTEXT.SendSoopBroadInfoSetting();
				MAINFRAME->RefreshBroadInfoDockUI(false);
			}
		}
	}

	OBSSource source = GetSoopMediaSource();
	if (source)
	{
		obs_media_state media_state = obs_source_media_get_state(source);
		if (media_state == OBS_MEDIA_STATE_PLAYING ||
			media_state == OBS_MEDIA_STATE_OPENING ||
			media_state == OBS_MEDIA_STATE_BUFFERING ||
			media_state == OBS_MEDIA_STATE_PAUSED)
		{
			obs_source_media_stop(source);
		}

		QString id = obs_source_get_id(source);
		if (0 != id.compare(directbroad_id))
			return;

		OBSDataAutoRelease settings = obs_source_get_settings(source);

		m_currentIdx = requestIdx;
		obs_data_set_string(settings, "input", url.toStdString().c_str());

		obs_source_update(source, settings);

		auto it = m_directBroadInfos.begin();
		for (; it != m_directBroadInfos.end(); ++it) {
			if ((*it).idx == requestIdx) {

				bool is_changed_name = obs_data_get_bool(settings, "is_changed_name");
				if (!is_changed_name)
				{
					std::string prevName = obs_source_get_name(source);
					std::string newName = (*it).title.toStdString().c_str();

					UNDO_STACK.AddActionRename(prevName, newName, source);

					obs_source_set_name(source, (*it).title.toStdString().c_str());
				}
				break;
			}
		}

		m_directBroadAllowedCategory = categorys;

		emit qsignalResponseDirectBroadOneTimeUrl(requestIdx, url);
	}
}

void SOOPMediaSourceManager::_qslotDirectBroadOneTimeUrlDataAPI_2(const QByteArray& responseData, int requestIdx)
{
	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	QString id = obs_source_get_id(source);
	if (0 != id.compare("soop_directbroad_source"))
		return;

	QJsonDocument doc = QJsonDocument::fromJson(responseData);
	if (doc.isNull() || !doc.isObject())
		return;

	QJsonObject jsonObj = doc.object();

	int result = jsonObj["RESULT"].toInt();
	if (1 != result)
		return;

	QJsonObject dataObj = jsonObj["DATA"].toObject();

	QString allowedCateLists = dataObj["allowed_cate_no"].toString();
	QString url = dataObj["url"].toString();

	bool findAllowedCategory = false;
	QStringList categoryNums = allowedCateLists.split(',', Qt::SkipEmptyParts);
	for (QString& strCategoryNum : categoryNums) {
		int categoryNo = strCategoryNum.trimmed().toInt();
		if (categoryNo == AUTH_CONTEXT.GetSoopBroadInfo()->CategoryNumber()) {
			findAllowedCategory = true;
			break;
		}
	}

	if (!findAllowedCategory && AFOutputUtil::IsStreamActive())
		return;

	m_currentIdx = requestIdx;
	m_currentURL = url;

	if(!m_timerDirectBroad.isActive())
		m_timerDirectBroad.start(1000);
}

void SOOPMediaSourceManager::_qslotDirectBroadGeoBlockForDirectAPIData(	const QByteArray& responseData,	int requestIdx)
{
	m_directBroadGeoBlockedCategory.clear();

	QJsonDocument doc = QJsonDocument::fromJson(responseData);
	if (doc.isNull() || !doc.isObject())
		return;

	QJsonObject obj = doc.object();

	QJsonArray cateArray = obj["cate_no"].toArray();
	for (const QJsonValue& value : cateArray) {
		QString cateStr = value.toString().trimmed();
		if (cateStr.isEmpty())
			continue;

		bool parsedOk = false;
		int cateNo = cateStr.toInt(&parsedOk);   // "00040086" -> 40086
		if (parsedOk) {
			m_directBroadGeoBlockedCategory.push_back(cateNo);
		}
	}

	RequestDirectBroadOneTimeUrl(requestIdx);
}

void SOOPMediaSourceManager::qslotRecvOBSMediaStarted()
{
	if(m_pSoopSourceProps)
		QMetaObject::invokeMethod(m_pSoopSourceProps, "qslotRecvOBSMediaStarted");

	if (m_pSoopSourceToolbar)
		QMetaObject::invokeMethod(m_pSoopSourceToolbar, "qslotRecvOBSMediaStarted");
}

void SOOPMediaSourceManager::qslotRecvOBSMediaStoped()
{
	if (m_pSoopSourceProps)
		QMetaObject::invokeMethod(m_pSoopSourceProps, "qslotRecvOBSMediaStopped");

	if (m_pSoopSourceToolbar)
		QMetaObject::invokeMethod(m_pSoopSourceToolbar, "qslotRecvOBSMediaStopped");
}

void SOOPMediaSourceManager::qslotRecvOBSMediaEnded()
{
	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	QString id = obs_source_get_id(source);
	if (0 == id.compare("soop_anivod_source") ||
		0 == id.compare("soop_sportvod_source") ||
		0 == id.compare("soop_dramavod_source") ||
		0 == id.compare("soop_movievod_source")) {
		m_timerNextVod.start(1000);
	}
	else
	{
		if (m_pSoopSourceProps)
			QMetaObject::invokeMethod(m_pSoopSourceProps, "qslotRecvOBSMediaEnded");
	}

	if (m_pSoopSourceToolbar)
		QMetaObject::invokeMethod(m_pSoopSourceToolbar, "qslotRecvOBSMediaEnded");
}

void SOOPMediaSourceManager::qslotRecvOBSMediaPlay()
{
	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	if (m_pSoopSourceProps)
		QMetaObject::invokeMethod(m_pSoopSourceProps, "qslotRecvOBSMediaPlay");

	if (m_pSoopSourceToolbar)
		QMetaObject::invokeMethod(m_pSoopSourceToolbar, "qslotRecvOBSMediaPlay");
}

void SOOPMediaSourceManager::qslotRecvOBSMediaPause()
{
	if (m_pSoopSourceProps)
		QMetaObject::invokeMethod(m_pSoopSourceProps, "qslotRecvOBSMediaPause");

	if (m_pSoopSourceToolbar)
		QMetaObject::invokeMethod(m_pSoopSourceToolbar, "qslotRecvOBSMediaPause");
}

void ResizeSceneitemToFitVideo(obs_sceneitem_t* item, int width, int height)
{
	if (!item)
		return;

	obs_transform_info oti;
	obs_sceneitem_get_info(item, &oti);

	uint32_t canvasW = (uint32_t)config_get_uint(ACTIVECONFIG, "Video", "BaseCX");
	uint32_t canvasH = (uint32_t)config_get_uint(ACTIVECONFIG, "Video", "BaseCY");

	const float adjustScaleW = 1.0f / 2.0f;
	const float adjustScaleH = 2.0f / 3.0f;

	if (oti.scale.x == 0.0f || oti.scale.y == 0.0f) {
		float targetScale = 1.0f;
		if (width >= height) {
			const float adjustW = canvasW * adjustScaleW;
			if (width > adjustW)
				targetScale = adjustW / (float)width;
		}
		else {
			const float adjustH = canvasH * adjustScaleH;
			if (height > adjustH)
				targetScale = adjustH / (float)height;
		}
		if (oti.scale.x != targetScale || oti.scale.y != targetScale) {
			oti.scale.x = oti.scale.y = targetScale;
			obs_sceneitem_set_info(item, &oti);
		}
	}
	
	// adjust
	float itemW = oti.scale.x * (float)width;
	float itemH = oti.scale.y * (float)height;

	float availW = canvasW - oti.pos.x;
	float availH = canvasH - oti.pos.y;

	if (itemW <= availW && itemH <= availH)
		return;

	if (availW <= 0.0f || availH <= 0.0f)
		return;

	float fitW = availW / itemW;
	float fitH = availH / itemH;
	float fit = std::min(1.0f, std::min(fitW, fitH));

	if (fit < 1.0f) {
		oti.scale.x *= fit;
		oti.scale.y *= fit;
		obs_sceneitem_set_info(item, &oti);
	}
}

void SOOPMediaSourceManager::qslotRecvOBSMediaGetFirstFrame(int frame_width, int frame_height)
{
	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	OBSScene curScene = SCENE_CONTEXT.GetCurrentScene();
	OBSSceneItemAutoRelease item = obs_scene_sceneitem_from_source(curScene, source);

	ResizeSceneitemToFitVideo(item, frame_width, frame_height);
}

void SOOPMediaSourceManager::OBSMediaStarted(void* data, calldata_t* calldata)
{
	SOOPMediaSourceManager* callback = static_cast<SOOPMediaSourceManager*>(data);
	if (!callback)
		return;

	QMetaObject::invokeMethod(callback, "qslotRecvOBSMediaStarted");
}

void SOOPMediaSourceManager::OBSMediaStopped(void* data, calldata_t* calldata)
{
	SOOPMediaSourceManager* callback = static_cast<SOOPMediaSourceManager*>(data);
	if (!callback)
		return;

	QMetaObject::invokeMethod(callback, "qslotRecvOBSMediaStoped");
}

void SOOPMediaSourceManager::OBSMediaEnded(void* data, calldata_t* calldata)
{
	SOOPMediaSourceManager* callback = static_cast<SOOPMediaSourceManager*>(data);
	if (!callback)
		return;

	QMetaObject::invokeMethod(callback, "qslotRecvOBSMediaEnded");
}

void SOOPMediaSourceManager::OBSMediaPlay(void* data, calldata_t* calldata)
{
	SOOPMediaSourceManager* callback = static_cast<SOOPMediaSourceManager*>(data);
	if (!callback)
		return;

	QMetaObject::invokeMethod(callback, "qslotRecvOBSMediaPlay");
}

void SOOPMediaSourceManager::OBSMediaPause(void* data, calldata_t* calldata)
{
	SOOPMediaSourceManager* callback = static_cast<SOOPMediaSourceManager*>(data);
	if (!callback)
		return;

	QMetaObject::invokeMethod(callback, "qslotRecvOBSMediaPause");
}

void SOOPMediaSourceManager::FSMediaFileLoaded(void* data, calldata_t* calldata)
{
	SOOPMediaSourceManager* callback = static_cast<SOOPMediaSourceManager*>(data);
	if (!callback)
		return;

	int width = calldata_int(calldata, "width");
	int height = calldata_int(calldata, "height");

	QMetaObject::invokeMethod(callback, "qslotRecvOBSMediaGetFirstFrame",
		Q_ARG(int, width),
		Q_ARG(int, height));
}


bool SOOPMediaSourceManager::IsEqualVodInfo(const VodInfo_s& a, const VodInfo_s& b)
{
	return (a.contentIdx == b.contentIdx) &&
		(a.contentTitle == b.contentTitle) &&
		(a.seasonTitle == b.seasonTitle) &&
		(a.vodTitle == b.vodTitle);
}

void SOOPMediaSourceManager::OBSVlcRestartRequested(void* data, calldata_t* calldata)
{
	auto* manager = static_cast<SOOPMediaSourceManager*>(data);

	if (!manager || !calldata)
		return;

	obs_source_t* signalSource = static_cast<obs_source_t*>(calldata_ptr(calldata, "source"));

	if (!signalSource)
		return;

	const char* sourceId = obs_source_get_id(signalSource);
	if (!sourceId || strcmp(sourceId, "soop_tv_cable_source") != 0) {
		return;
	}

	const char* uuid = obs_source_get_uuid(signalSource);

	QString sourceUuid = QString::fromUtf8(uuid ? uuid : "");

	int abnormalCount = (int)calldata_int(calldata, "abnormal_count");
	int videoAgeMs = (int)calldata_int(calldata, "video_age_ms");
	int audioAgeMs = (int)calldata_int(calldata, "audio_age_ms");
	int avDriftMs = (int)calldata_int(calldata, "av_drift_ms");

	QMetaObject::invokeMethod(
		manager,
		"qslotVlcRestartRequested",
		Qt::QueuedConnection,
		Q_ARG(QString, sourceUuid),
		Q_ARG(int, abnormalCount),
		Q_ARG(int, videoAgeMs),
		Q_ARG(int, audioAgeMs),
		Q_ARG(int, avDriftMs));
}

void SOOPMediaSourceManager::qslotVlcRestartRequested(
	QString sourceUuid,
	int abnormalCount,
	int videoAgeMs,
	int audioAgeMs,
	int avDriftMs)
{
	if (m_tvLiveRestartPending)
		return;

	OBSSource source = GetSoopMediaSource();
	if (!source)
		return;

	const char* sourceId = obs_source_get_id(source);

	if (!sourceId || strcmp(sourceId, "soop_tv_cable_source") != 0) {
		return;
	}

	const char* currentUuid = obs_source_get_uuid(source);

	if (!currentUuid || sourceUuid != QString::fromUtf8(currentUuid)) {
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);

	if (obs_data_get_bool(settings, "show_blind")) {
		return;
	}

	const int cpNo = (int)obs_data_get_int(settings, "cpNo");
	if (cpNo <= 0)
		return;

	m_tvLiveRestartPending = true;
	const uint64_t refreshSequence = ++m_tvLiveRefreshSequence;

	if (m_tvLiveBlindTimer.isActive())
		m_tvLiveBlindTimer.stop();

	blog(LOG_WARNING,
		"[SOOPMediaSourceManager] "
		"Request new TV live URL: "
		"name=%s, cpNo=%d, "
		"abnormal_count=%d, "
		"video_age=%d ms, "
		"audio_age=%d ms, "
		"av_drift=%d ms",
		obs_source_get_name(source),
		cpNo,
		abnormalCount,
		videoAgeMs,
		audioAgeMs,
		avDriftMs);

	RequestTvLiveOneTimeUrl(cpNo);

	QTimer::singleShot(
		30000,
		this,
		[this, refreshSequence]() {
			if (refreshSequence !=
				m_tvLiveRefreshSequence) {
				return;
			}

			if (!m_tvLiveRestartPending)
				return;

			blog(LOG_WARNING,
				"[SOOPMediaSourceManager] "
				"TV live URL refresh timeout");

			m_tvLiveRestartPending = false;

			if (!m_tvLiveBlindTimer.isActive())
				m_tvLiveBlindTimer.start();
		});
}