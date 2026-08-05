#include "CBlockManager.h"

#include "Application/CApplication.h"
#include "Common/StudioDefine.h"

#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include "platform/platform.hpp"


#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"
#include "CoreModel/Auth/CAuthManager.h"

#include "MainFrame/Output/COutput.h"

#define OVERLAY_POPUP_SIZE_WIDTH	550
#define OVERLAY_POPUP_SIZE_HEIGHT	560

#define MISSION_PAGE_SIZE_WIDTH		820
#define MISSION_PAGE_SIZE_HEIGHT	760
// (820 x 720)

#define VOTE_PAGE_SIZE_WIDTH		515
#define VOTE_PAGE_SIZE_HEIGHT		568

#define EXTENSION_PAGE_MIN_SIZE_WIDTH 540
#define EXTENSION_PAGE_MIN_SIZE_HEIGHT 950

#define SAVVYREACTION_PAGE_SIZE_WIDTH		770
#define SAVVYREACTION_PAGE_SIZE_HEIGHT		885

#define AQUA_CONTROL_PAGE_MIN_SIZE_WIDTH 436
#define AQUA_CONTROL_PAGE_MIN_SIZE_HEIGHT 800

#define BREAKTIME_POPUP_SIZE_WIDTH	560
#define BREAKTIME_POPUP_SIZE_HEIGHT	794

// cef popup url


// for mission
enum MissionType {
	Challenge = 0,
	BattleJoinUsers,
	BattleFundingRank
};
struct MissionInfo {
	const char* id = nullptr;
	QString display_name;
};
const char* mission_id[] = {
	"soop_mission_source_challenge",
	"soop_mission_source_battle_joinusers",
	"soop_mission_source_battle_fundingrank",
};
const int mission_count = sizeof(mission_id) / sizeof(mission_id[0]);


void AFQBlockManager::_SetBlockBaseInfoExtra(QMap<int, const char*>& tooltipmap)
{
	//Overlay
	BlockBaseInfo OverlayInfo;
	OverlayInfo.minSize = QSize(OVERLAY_POPUP_SIZE_WIDTH, OVERLAY_POPUP_SIZE_HEIGHT);
	OverlayInfo.maxSize = QSize(OVERLAY_POPUP_SIZE_WIDTH, OVERLAY_POPUP_SIZE_HEIGHT);
	OverlayInfo.windowTitle = "Block.Tooltip.LiveOverlay";
	OverlayInfo.isCef = false;
	OverlayInfo.showLeft = false;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::SoopOverlay, OverlayInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::SoopOverlay, "Block.Tooltip.LiveOverlay");

	//Mission
	BlockBaseInfo MissionInfo;
	MissionInfo.minSize = QSize(MISSION_PAGE_SIZE_WIDTH, MISSION_PAGE_SIZE_HEIGHT);
	MissionInfo.maxSize = QSize(MISSION_PAGE_SIZE_WIDTH, MISSION_PAGE_SIZE_HEIGHT);
	MissionInfo.windowTitle = "Block.Tooltip.Mission";
	MissionInfo.isCef = true;
	MissionInfo.showLeft = false;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::Mission, MissionInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::Mission, "Block.Tooltip.Mission");

	//Vote
	BlockBaseInfo VoteInfo;
	VoteInfo.minSize = QSize(VOTE_PAGE_SIZE_WIDTH, VOTE_PAGE_SIZE_HEIGHT);
	VoteInfo.maxSize = QSize(VOTE_PAGE_SIZE_WIDTH, VOTE_PAGE_SIZE_HEIGHT);
	VoteInfo.windowTitle = "Block.Tooltip.Vote";
	VoteInfo.isCef = true;
	VoteInfo.showLeft = false;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::Vote, VoteInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::Vote, "Block.Tooltip.Vote");

	//Savvy Reaction
	BlockBaseInfo SavvyReactionInfo;
	SavvyReactionInfo.minSize = QSize(SAVVYREACTION_PAGE_SIZE_WIDTH, SAVVYREACTION_PAGE_SIZE_HEIGHT);
	SavvyReactionInfo.maxSize = QSize(SAVVYREACTION_PAGE_SIZE_WIDTH, SAVVYREACTION_PAGE_SIZE_HEIGHT);
	SavvyReactionInfo.windowTitle = "Block.Tooltip.SavvyReaction";
	SavvyReactionInfo.isCef = true;
	SavvyReactionInfo.showLeft = false;
	SavvyReactionInfo.parentOnPopup = ParentTypes::MainWindow;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::SAVVYReaction, SavvyReactionInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::SAVVYReaction, "Block.Tooltip.SavvyReaction");

	//Aqua Remote Control
	BlockBaseInfo AquaRemoteControlInfo;
	AquaRemoteControlInfo.minSize = QSize(AQUA_CONTROL_PAGE_MIN_SIZE_WIDTH, AQUA_CONTROL_PAGE_MIN_SIZE_HEIGHT);
	//AquaRemoteControlInfo.maxSize = QSize(AQUA_CONTROL_PAGE_MIN_SIZE_WIDTH, AQUA_CONTROL_PAGE_MIN_SIZE_HEIGHT);
	AquaRemoteControlInfo.windowTitle = "Block.Tooltip.AquaRemoteControl";
	AquaRemoteControlInfo.isCef = true;
	AquaRemoteControlInfo.showLeft = false;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::AquaControl, AquaRemoteControlInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::AquaControl, "Block.Tooltip.AquaRemoteControl");

	//BreakTime
	BlockBaseInfo BreakTimeInfo;
	BreakTimeInfo.minSize = QSize(BREAKTIME_POPUP_SIZE_WIDTH, BREAKTIME_POPUP_SIZE_HEIGHT);
	BreakTimeInfo.maxSize = QSize(BREAKTIME_POPUP_SIZE_WIDTH, BREAKTIME_POPUP_SIZE_HEIGHT);
	BreakTimeInfo.windowTitle = "Block.Tooltip.Breaktime";
	BreakTimeInfo.isCef = false;
	BreakTimeInfo.showLeft = false;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::Breaktime, BreakTimeInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::Breaktime, "Block.Tooltip.Breaktime");
}

void AFQBlockManager::qslotCefPopupDataReceived(const QCefQuery& query)
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
}

static bool FindVideoBalloonSource(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
	OBSSceneItem& destItem = *reinterpret_cast<OBSSceneItem*>(param);

	obs_source_t* source = obs_sceneitem_get_source(item);
	const char* id = obs_source_get_id(source);

	bool findSource = false;
	if (0 == strcmp(id, "soop_videoballoon_source")) {
		findSource = true;
	}

	if (findSource) {
		destItem = item;
		return false;
	}

	return true;
};

void AFQBlockManager::qslotExtensionDataReceived(const QCefQuery& query)
{

}

//
bool AFQBlockManager::_CreateCefPopup(ENUM_WINDOW_TYPE type)
{
	QWidget* findWidget = nullptr;
	FindBlock(type, findWidget);

	if(findWidget)
		return true;

	std::string url;
	bool success = _GetCefPopupURL(type, url);
	if(!success)
		return false;

	QCefWidget* cefWidget = CEFMANAGER.createWidget(nullptr, url);
	if(!cefWidget)
		return false;
	
	AddBlock(type, cefWidget);

	return true;
}

void AFQBlockManager::qslotCreateAfterCefBrowser(int type)
{
	ENUM_WINDOW_TYPE windowType = (ENUM_WINDOW_TYPE)type;

	if (windowType == ENUM_WINDOW_TYPE::AquaControl)
	{
		AFChannelData* pSoopChannel = nullptr;
		AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannel);
		if (pSoopChannel) {
			QList<QVariant> values = { };
			SOOP_API_HANDLER->postAPIfromId(SOOP_API_KEY::POST_AQUA_REMOTE_CONTROL, values,
											this, "qslotReceiveAquaRemoteControlUrl");
		}
	}
}

void AFQBlockManager::qslotLoadEndCefBrowser(int type)
{
	if (type == ENUM_WINDOW_TYPE::SoopChat)
	{
		AFChannelData* pSoopChannelData = nullptr;
		AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannelData);
		if (!pSoopChannelData) {
			return;
		}
		if (0 != MAIN_OUTPUT->GetStreamingOutputRef()) {
			if (pSoopChannelData->isStreaming) {
				QTimer::singleShot(500, this, [this]() {
					SendBroadState(true);
					});
			}
		}
	}
	else if (type == ENUM_WINDOW_TYPE::Extensions)
	{
		QWidget* widget = nullptr;
		FindBlock(ENUM_WINDOW_TYPE::Extensions, widget);
		if (nullptr == widget)
			return;

		QCefWidget* cefWidget = reinterpret_cast<QCefWidget*>(widget);
		cefWidget->executeJavaScript("window.postMessage({cmd : 'init',  data : { state : 'BROADING' }});");
	}
}

void AFQBlockManager::qslotReceiveAquaRemoteControlUrl(const QByteArray& responseData)
{
	std::string jsonString = responseData.toStdString();

	std::string err;
	err.clear();
}

bool AFQBlockManager::_GetCefPopupURL(ENUM_WINDOW_TYPE type, std::string& url)
{
	switch(type)
	{
		case ENUM_WINDOW_TYPE::Mission:
			url = URL_MISSION_MAIN;
			break;

		case ENUM_WINDOW_TYPE::Vote:
			url = URL_STUDIO_VOTETOOL;
			break;

		case ENUM_WINDOW_TYPE::Extensions:
			url = URL_EXTENSION_LIST;
			break;
		case ENUM_WINDOW_TYPE::SAVVYReaction:
		{
			AFChannelData* pSoopChannel = nullptr;
			AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannel);
			QString qUrl = QString::fromStdString(URL_SAVVY_REACTION) + pSoopChannel->pAuthData->channelID.c_str();
			url = qUrl.toStdString();
		}			
			break;
		case ENUM_WINDOW_TYPE::AquaControl:
			url = URL_AQUA_REMOTE_CONTROL;
			break;
		default:
			url.clear();
			break;
	}
	return !url.empty();
}

void AFQBlockManager::_AddMissionSource(int type, std::string url)
{
	if(type < 0 ||
	   type > mission_count)
		return;

	if (type == 0)
	{
		emit qsignalAddSource("soop_chat_source_c_mission");
		return;
	}

	//
	const char* id = mission_id[type];
	QString displayText = AFSourceUtil::GetPlaceHodlerText(id);


	obs_transform_info* pInfo = nullptr;
	if (AFSourceUtil::IsBrowserSizeStretch(id))
	{
		obs_transform_info itemInfo;
		vec2_set(&itemInfo.pos, 0, 0);
		vec2_set(&itemInfo.scale, 1.0f, 1.0f);
		vec2_set(&itemInfo.bounds, 960, 540);

		itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
		itemInfo.rot = 0.0f;
		itemInfo.bounds_type = OBS_BOUNDS_STRETCH;
		itemInfo.bounds_alignment = OBS_ALIGN_CENTER;

		pInfo = &itemInfo;
	}

	OBSSource newSource;
	if(!AFSourceUtil::AddNewSource(MAINFRAME, id, displayText.toStdString().c_str(), true, newSource, pInfo))
		return;

	OBSDataAutoRelease settings = obs_source_get_settings(newSource);

	obs_data_set_string(settings, "url", url.c_str());
	obs_source_update(newSource, settings);

	if(AFSourceUtil::ShouldShowProperties(newSource))
	{
		MAINFRAME->CreateSourceProperties(newSource);
	}

	AFSourceUtil::SetUndoRedoAddSource(id, displayText.toStdString().c_str(), true);
}

void AFQBlockManager::_OpenSavvySoopFolder()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(GetDefaultVideoSavePath().c_str()));
}

void AFQBlockManager::_DownloadSavvyReactionVideo(std::string strUrl,
												  std::string strReactionType,
												  std::string strStyleType,
												  int nReactionIndex)
{
	AFChannelData* pSoopChannel = nullptr;
	AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannel);
	time_t now = time(0);
	struct tm* cur_time = nullptr;
	cur_time = localtime(&now);

	ListReactionInfo data;
	data.remoteUrl = strUrl.c_str();
	data.localPath = QString::asprintf("%s\\[%s]_%s_%s_%s_%02d%02d%02d%02d%02d.mp4",
									   GetDefaultVideoSavePath().c_str(),
									   QTStr("Savvy.Reaction.FileName").toStdString().c_str(),
									   pSoopChannel->pAuthData->channelID.c_str(),
									   strReactionType.c_str(),
									   strStyleType.c_str(),
									   cur_time->tm_mon + 1,
									   cur_time->tm_mday,
									   cur_time->tm_hour,
									   cur_time->tm_min,
									   cur_time->tm_sec);
	data.reactionIndex = nReactionIndex;

	std::lock_guard<std::mutex> lock(m_listMutex);
	ListReactionType listType = { strReactionType.c_str(), strStyleType.c_str() };
	m_listReactionData.push_back(data);
	m_listReactionType.push_back(listType);
	
	if (m_listReactionData.size() == 1) {
		_StartDownload();
	}
}

void AFQBlockManager::_StartDownload()
{
	if (!m_listReactionData.empty()) {
		ListReactionInfo data = m_listReactionData.front();
		QUrl url(data.remoteUrl);
		QNetworkRequest request(url);
		m_netManager.get(request);
	}
}

void AFQBlockManager::_DownloadFinished(QNetworkReply* reply)
{
	ListReactionInfo data;
	QString localPath;
	
	std::lock_guard<std::mutex> lock(m_listMutex);
	data = m_listReactionData.front();
	m_listReactionData.pop_front();

	localPath = data.localPath;

	if (reply->error() == QNetworkReply::NoError) {
		QDir dir(GetDefaultVideoSavePath().c_str());
		if (!dir.exists()) {
			dir.mkpath(".");
		}
		QFile file(localPath);
		if (file.open(QIODevice::WriteOnly)) {
			file.write(reply->readAll());
			file.close();
			
			_AddReactionListSource(localPath.toStdString());

			QWidget* widget = nullptr;
			FindBlock(ENUM_WINDOW_TYPE::SAVVYReaction, widget);
			if (nullptr != widget) {
				QCefWidget* cefWidget = reinterpret_cast<QCefWidget*>(widget);
				std::string script = "window.handleFreecshotQuery({ result: 'true', reactionIdx: " + std::to_string(data.reactionIndex) + "});";
				cefWidget->executeJavaScript(script);
			}
		}
	}
	else {
		blog(LOG_ERROR, "Failed to download file");
	}
	reply->deleteLater();

	if (!m_listReactionData.empty()) {
	}
		_StartDownload();
}

obs_source_t* AFQBlockManager::_IsExistingReactionListSource()
{
	obs_source_t* reactionSource = nullptr;
	const char* id = "ffmpeg_list_source";
	const char* sourcename = "AI 리액션 동영상 리스트";

	auto findReactionSource = [id, sourcename, &reactionSource](obs_source_t* scene_source) {
		obs_scene_t* scene = obs_scene_from_source(scene_source);

		OBSSceneItem item;
		obs_scene_enum_items(scene, [](obs_scene_t* scene, obs_sceneitem_t* item, void* param) {
			auto* reactionSourcePtr = reinterpret_cast<obs_source_t**>(param);
			obs_source_t* source = obs_sceneitem_get_source(item);
			const char* _id = obs_source_get_id(source);
			const char* _name = obs_source_get_name(source);

			if (strcmp(_id, "ffmpeg_list_source") == 0 && strcmp(_name, "AI 리액션 동영상 리스트") == 0) {
				*reactionSourcePtr = source;
				return false;
			}
			return true;
		}, &reactionSource);

		return reactionSource == nullptr;
	};

	using FindReactionSource_t = decltype(findReactionSource);
	obs_enum_scenes([](void* data, obs_source_t* source) {
		return (*reinterpret_cast<FindReactionSource_t*>(data))(source);
	}, &findReactionSource);

	return reactionSource;
}

void AFQBlockManager::_AddReactionListSource(std::string url)
{
	obs_source_t* reactionSource = _IsExistingReactionListSource();
	if (nullptr == reactionSource) {
		if (!m_firstAddReaction)
			return;

		OBSDataAutoRelease settings = obs_data_create();
		obs_data_set_string(settings, "current_file_name", url.c_str());
		obs_data_set_bool(settings, "looping", true);
		OBSDataArrayAutoRelease items = obs_data_get_array(settings, "playlist");
		if (nullptr == items)
			items = obs_data_array_create();

		obs_data_t* item = obs_data_create();
		obs_data_set_string(item, "value", url.c_str());
		obs_data_set_bool(item, "selected", false);
		obs_data_set_bool(item, "hidden", false);
		obs_data_array_push_back(items, item);
		obs_data_set_array(settings, "playlist", items);
		obs_data_release(item);

		OBSSource reactionListSource = obs_source_create("ffmpeg_list_source", "AI 리액션 동영상 리스트", settings, nullptr);

		obs_source_t* scene = obs_frontend_get_current_scene();
		if (!scene) {
			blog(LOG_ERROR, "Failed to get current scene");
			return;
		}		
		obs_scene_t* scene_data = obs_scene_from_source(scene);
		obs_sceneitem_t* scene_item = obs_scene_add(scene_data, reactionListSource);
		obs_source_release(scene);

		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		obs_transform_info transform;
		vec2_set(&transform.pos, 0, 0);
		vec2_set(&transform.scale, 1.0f, 1.0f);
		vec2_set(&transform.bounds, ovi.base_width, ovi.base_height);
		transform.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
		transform.rot = 0.0f;
		transform.bounds_type = OBS_BOUNDS_SCALE_INNER;
		transform.bounds_alignment = OBS_ALIGN_CENTER;
		obs_sceneitem_set_info(scene_item, &transform);		

		obs_source_media_play_pause(reactionListSource, false);
	} else {
		/*
		 2024-08-21: derrod
		 * docs,libobs: Remove/internalize deprecated addref functions
		 These have been deprecated for external users since 27.2 (early 2022)
		 and only two are still in use internally.
		*/
		//obs_source_addref(reactionSource);
		OBSDataAutoRelease settings = obs_source_get_settings(reactionSource);
		
		OBSDataArrayAutoRelease items = obs_data_get_array(settings, "playlist");
		if (nullptr == items)
			items = obs_data_array_create();
		obs_data_t* item = obs_data_create();
		obs_data_set_string(item, "value", url.c_str());
		obs_data_set_bool(item, "selected", false);
		obs_data_set_bool(item, "hidden", false);
		obs_data_array_push_back(items, item);
		obs_data_set_array(settings, "playlist", items);
		obs_data_release(item);
		obs_source_update(reactionSource, settings);
		
		obs_source_release(reactionSource);
	}

	m_firstAddReaction = false;
}
