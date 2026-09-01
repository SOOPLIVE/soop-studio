#include "CBlockManager.h"

#include <QPropertyAnimation>
#include <QComboBox>
#include <QSpinBox>
#include <QClipboard>
#include <QImageReader>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "Application/CApplication.h"

#include "Common/StudioDefine.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Auth/CAuthManager.h"

#include "MainFrame/Output/COutput.h"
#include "MainFrame/SceneSource/CMainSceneSource.h"
#include "MainFrame/CChannelSlideWidget.h"

#include "Blocks/CDockTitle.h"
#include "Blocks/SceneSourceDock/CSceneSourceDockWidget.h"
#include "Blocks/AudioMixerDock/CAudioMixerDockWidget.h"
#include "Blocks/AdvanceControlsDock/CAdvanceControlsDockWidget.h"
#include "Blocks/SceneControlDock/CSceneControlDockWidget.h"
#include "Blocks/BroadInfoDock/CBroadInfoDockWidget.h"
#include "Blocks/SceneControlDock/CProjector.h"

#include "PopupWindows/SettingPopup/CAddStreamWidget.h"
#include "PopupWindows/CStatFrame.h"
#include "PopupWindows/COverlayWidget.h"
#include "PopupWindows/BroadInfoPopup/CInstantVODSaverWidget.h"
#include "PopupWindows/BroadInfoPopup/CVodAutoUploadNoticeDialog.h"
#include "PopupWindows/BroadInfoPopup/CVodAutoUploadNoticeDialog.h"
#include "PopupWindows/BroadInfoPopup/CSoopBroadcastNoticeDialog.h"

#include "UIComponent/CMessageBox.h"
#include "UIComponent/CBasicToggleButton.h"

#include "Utils/soop-imageprinter.hpp"
#include "Utils/BreaktimeManager.h"

#define BLOCK_AREA_MARGIN 12

#define DOCK_MIN_SIZE_HEIGHT 156
#define SCENE_SOURCE_MIN_SIZE_WIDTH  260 //422
#define AUDIO_MIXER_MIN_SIZE_WIDTH 140
#define BROADINFO_MIN_SIZE_WIDTH 285
#define SCENE_CONTROL_WIN_MIN_SIZE_WIDTH 720
#define SCENE_CONTROL_WIN_MIN_SIZE_HEIGHT 486
#define ADVANCE_CONTROL_FIX_SIZE_WIDTH 360
#define ADVANCE_CONTROL_FIX_SIZE_HEIGHT 208
#define BROWSER_COLLECTION_FIX_SIZE_WIDTH 622
#define BROWSER_COLLECTION_FIX_SIZE_HEIGHT 720
#define STAT_FIX_SIZE_WIDTH 602
#define STAT_FIX_SIZE_HEIGHT 632
#define INSTANT_VOD_FIX_SIZE_WIDTH 480
#define INSTANT_VOD_FIX_SIZE_HEIGHT 390

#define SOOP_CHAT_MIN_SIZE_WIDTH 422
#define SOOP_CHAT__MIN_SIZE_HEIGHT 550

#define CUSTOM_BROWSER_MIN_WIDTH 370
#define CUSTOM_BROWSER_MIN_HEIGHT 352

#define EVENT_BANNER_IMAGE "assets/Popup/event/event.png"
#define EVENT_BANNER_NAVIGATE_URL "https://welcome.sooplive.com"

#define EVENT_BANNER_WIDTH 458
#define EVENT_BANNER_HEIGHT 463

#define OPEN_EXTRA_BROWSER_POPUP_STUDIO_QUICKVIEW_GIFT_MODAL_LAYER "StudioQuickviewGiftModalLayer"
#define OPEN_EXTRA_BROWSER_POPUP_STUDIO_RANDOM_GIFT_MODAL_LAYER "StudioRandomGiftModalLayer"
#include <regex>

std::string UnEscapeString(const std::string& input) {
	std::string unescape;
	unescape.reserve(input.size());

	for(size_t i = 0; i < input.length(); ++i) {
		if(input[i] == '\\' && i + 1 < input.length()) {
			switch(input[i + 1]) {
				case '\"': unescape.push_back('\"'); break;
				case '\\': unescape.push_back('\\'); break;
				default:
					unescape.push_back(input[i + 1]);
					break;
			}
			++i;
		} else {
			unescape.push_back(input[i]);
		}
	}
	// erase first \"
	size_t offset = unescape.find_first_of('\"');
	if(unescape.substr(offset, 1) == "\"") {
		unescape.erase(offset, 1);
	}
	// erase last \"
	offset = unescape.find_last_of('\"');
	if(unescape.substr(offset, 1) == "\"") {
		unescape.erase(offset, 1);
	}
	return unescape;
}

std::string createCloseChatCefScript(const std::string& key) {

	std::string data = "{ \"cmd\":\"CLOSE\", \"data\":\"{}\", \"from\":\"" + key + "\" }";
	return createRequestCefScript("broadcast", "function", data);
}

std::string createBroadStartChatCefScript() {

	std::string script = R"({ "cmd":"BROAD_STATE", "data":"{ \"state\" : \"start\" }" })";
	std::string data = EscapeForJavaScript(script);

	return createRequestCefScript("main", "function", data);
}

AFQBlockManager::AFQBlockManager(QWidget* parent) : QObject(parent)
{
	connect(&m_netManager, &QNetworkAccessManager::finished, this, &AFQBlockManager::_DownloadFinished);

	m_cefQueryThreadRunning = true;
	m_cefWorkerThread = std::thread(&AFQBlockManager::_cefWorkerLoop, this);
}

AFQBlockManager::~AFQBlockManager()
{
	m_cefQueryThreadRunning = false;
	m_cefQueryCV.notify_one();
	if (m_cefWorkerThread.joinable())
		m_cefWorkerThread.join();
}

void AFQBlockManager::_SetBlockBaseInfo()
{
	QMap<int, const char*> tooltipmap;

	/////////////////////////////// COMMON Blocks ////////////////////////////////////////
	//SceneSource
	BlockBaseInfo SceneSourceInfo;
	SceneSourceInfo.transformable = true;
	SceneSourceInfo.minSize = QSize(SCENE_SOURCE_MIN_SIZE_WIDTH, DOCK_MIN_SIZE_HEIGHT);
	SceneSourceInfo.windowTitle = "Basic.SceneSourceDock.Title";
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::SceneSource, SceneSourceInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::SceneSource, "Block.Tooltip.SceneSourceList");

	//AudioMixer
	BlockBaseInfo AudioMixerInfo;
	AudioMixerInfo.transformable = true;
	AudioMixerInfo.minSize = QSize(AUDIO_MIXER_MIN_SIZE_WIDTH, DOCK_MIN_SIZE_HEIGHT);
	AudioMixerInfo.windowTitle = "Block.Tooltip.AudioMixer";
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::AudioMixer, AudioMixerInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::AudioMixer, "Block.Tooltip.AudioMixer");

	//CHAT
	BlockBaseInfo SoopChatInfo;
	SoopChatInfo.transformable = true;
	SoopChatInfo.minSize = QSize(SOOP_CHAT_MIN_SIZE_WIDTH, SOOP_CHAT__MIN_SIZE_HEIGHT);
	SoopChatInfo.windowTitle = "Block.Tooltip.Chat";
	SoopChatInfo.isCef = true;
	SoopChatInfo.showRight = true;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::SoopChat, SoopChatInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::SoopChat, "Block.Tooltip.Chat");

	BlockBaseInfo TwitchChatInfo;
	TwitchChatInfo.transformable = true;
	TwitchChatInfo.minSize = QSize(SCENE_SOURCE_MIN_SIZE_WIDTH, DOCK_MIN_SIZE_HEIGHT);
	TwitchChatInfo.windowTitle = "Block.Tooltip.Chat.Twitch";
	TwitchChatInfo.isCef = true;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::TwitchChat, TwitchChatInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::TwitchChat, "Block.Tooltip.Chat.Twitch");

	BlockBaseInfo YoutubeChatInfo;
	YoutubeChatInfo.transformable = false;
	YoutubeChatInfo.minSize = QSize(SCENE_SOURCE_MIN_SIZE_WIDTH, DOCK_MIN_SIZE_HEIGHT);
	YoutubeChatInfo.windowTitle = "Block.Tooltip.Chat.Youtube";
	YoutubeChatInfo.isCef = true;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::YoutubeChat, YoutubeChatInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::YoutubeChat, "Block.Tooltip.Chat.Youtube");

	/////////////////////////////// COMMON Blocks ////////////////////////////////////////

	/////////////////////////////// KR Blocks ////////////////////////////////////////
	// Broad Info Dock
	BlockBaseInfo BroadInfo;
	BroadInfo.transformable = true;
	BroadInfo.minSize = QSize(BROADINFO_MIN_SIZE_WIDTH, DOCK_MIN_SIZE_HEIGHT);
	BroadInfo.windowTitle = "BroadInfo";
	//BroadInfo.needQuestionMark = true;
	//BroadInfo.questionMarkToolTip = QTStr("Block.Tooltip.BroadInfo");
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::BroadInfo, BroadInfo);
	tooltipmap.insert(ENUM_WINDOW_TYPE::BroadInfo, "BroadInfo");


	/////////////////////////////// KR Blocks ////////////////////////////////////////

	///////////////////////////// OTHER Blocks //////////////////////////////////////
	//AdvanceControl ( ReplayBuffer )
	BlockBaseInfo AdvanceControlInfo;
	AdvanceControlInfo.minSize = QSize(ADVANCE_CONTROL_FIX_SIZE_WIDTH, ADVANCE_CONTROL_FIX_SIZE_HEIGHT);
	AdvanceControlInfo.maxSize = QSize(ADVANCE_CONTROL_FIX_SIZE_WIDTH, ADVANCE_CONTROL_FIX_SIZE_HEIGHT);
	AdvanceControlInfo.windowTitle = "ReplayBuffer";
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::AdvanceControls, AdvanceControlInfo);

	//SceneControl
	BlockBaseInfo SceneControlInfo;
	SceneControlInfo.minSize = QSize(SCENE_CONTROL_WIN_MIN_SIZE_WIDTH, SCENE_CONTROL_WIN_MIN_SIZE_HEIGHT);
	SceneControlInfo.windowTitle = "Basic.Settings.General.Multiview";
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::SceneControl, SceneControlInfo);

	//CustomBrowserCollection
	BlockBaseInfo BrowserCollectionInfo;
	BrowserCollectionInfo.windowTitle = "Basic.MainMenu.Addon.CustomBrowserDocks";
	BrowserCollectionInfo.minSize = QSize(BROWSER_COLLECTION_FIX_SIZE_WIDTH, BROWSER_COLLECTION_FIX_SIZE_HEIGHT);
	BrowserCollectionInfo.showLeft = false;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::CustomBrowserCollection, BrowserCollectionInfo);

	//Stat
	BlockBaseInfo StatInfo;
	StatInfo.windowTitle = "Basic.Stats";
	StatInfo.minSize = QSize(STAT_FIX_SIZE_WIDTH, STAT_FIX_SIZE_HEIGHT);
	StatInfo.maxSize = QSize(STAT_FIX_SIZE_WIDTH, STAT_FIX_SIZE_HEIGHT);
	StatInfo.showLeft = false;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::StatPage, StatInfo);

	//Events with 1 image : size = imagesize + title
	BlockBaseInfo EventImageInfo;
	EventImageInfo.windowTitle = "Basic.Event.Title";
	EventImageInfo.minSize = QSize(EVENT_BANNER_WIDTH, EVENT_BANNER_HEIGHT);
	EventImageInfo.maxSize = QSize(EVENT_BANNER_WIDTH, EVENT_BANNER_HEIGHT);
	EventImageInfo.showLeft = false;
	EventImageInfo.parentOnPopup = ParentTypes::MainView;
	m_blockBaseInfoMap.insert(ENUM_WINDOW_TYPE::EventBannerImage, EventImageInfo);

	///////////////////////////// OTHER Blocks //////////////////////////////////////
}

void AFQBlockManager::qslotSwitchBlockWindowType(bool toPopup, int type)
{
	ENUM_WINDOW_TYPE val = static_cast<ENUM_WINDOW_TYPE>(type);

	if (toPopup)
		_ToPopup(val);
	else
		_ToDock(val);
}


void AFQBlockManager::qslotSwitchCustomBrowser(bool toPopup, QString uuid)
{
	if (toPopup)
		_ToPopup(uuid);
	else
		_ToDock(uuid);
}

void AFQBlockManager::qslotClosePopup(int type)
{
	QWidget* block;

	ENUM_WINDOW_TYPE val = static_cast<ENUM_WINDOW_TYPE>(type);

	if (FindBlock(val, block))
	{

		if (type > ENUM_WINDOW_TYPE::BLOCKITER)
		{
			if (block)
			{
				block->close();
				block->deleteLater();
			}

			m_blockMap.remove(val);
		}
		else
		{
			block->setParent(nullptr);
			block->hide();
		}

		if (_ClosePopup(val) || _CloseDock(val, true))
		{
			_ChangeBlockUse(false, val);
		}
	}
}

void AFQBlockManager::qslotCloseSoopChat(bool switchFrame)
{
	qslotClosePopup(ENUM_WINDOW_TYPE::SoopChat);
	if (!switchFrame)
	{
		foreach(const QString & key, m_extraBrowserPopups.keys()) {
			AFQBorderPopupBaseWidget* extraPopup = m_extraBrowserPopups.value(key);
			if (extraPopup) {
				if (extraPopup->GetIsChatPopup()) {
					if (!extraPopup->GetIsHidePopup())
					{
						extraPopup->close();
						m_extraBrowserPopups.remove(key);
					}
					else
					{
						extraPopup->hide();
					}
				}
			}
		}
	}
}

void AFQBlockManager::qslotClosePopupBySender()
{
	AFTTopBaseWidget* widget = reinterpret_cast<AFTTopBaseWidget*>(sender());
	
	ENUM_WINDOW_TYPE key = ENUM_WINDOW_TYPE::ENDOFINDEX;

	QMapIterator<ENUM_WINDOW_TYPE, AFTTopBaseWidget*> iter(m_popupWidgets);
	while (iter.hasNext()) {
		iter.next();
		if (iter.value() == widget) {
			key = iter.key();
		}
	}

	if(key != ENUM_WINDOW_TYPE::ENDOFINDEX)
		m_popupWidgets.remove(key);
}

void AFQBlockManager::qslotHideDock(int type)
{
	QWidget* block;

	ENUM_WINDOW_TYPE val = static_cast<ENUM_WINDOW_TYPE>(type);

	if (FindBlock(val, block))
	{
		ENUM_WINDOW_TYPE enumType = static_cast<ENUM_WINDOW_TYPE>(type);
		AFQBaseDockWidget* outDock = nullptr;
		
		if (GetDock(enumType, outDock))
		{
			if (type > ENUM_WINDOW_TYPE::BLOCKITER)
			{
				if (block)
				{
					block->close();
					block->deleteLater();
				}

				m_blockMap.remove(val);
			}
			else
			{
				block->setParent(nullptr);
				block->hide();
			}

			_CloseDock(enumType, false);
		}

		_ChangeBlockUse(false, val);

		if (val == ENUM_WINDOW_TYPE::SoopChat)
		{
			foreach(const QString & key, m_extraBrowserPopups.keys()) {
				AFQBorderPopupBaseWidget* extraPopup = m_extraBrowserPopups.value(key);
				if (extraPopup) {
					if (extraPopup->GetIsChatPopup()) {
						if (!extraPopup->GetIsHidePopup())
						{
							extraPopup->close();
							m_extraBrowserPopups.remove(key);
						}
						else
						{
							extraPopup->hide();
						}
					}
				}
			}
		}
	}
	DYNAMIC_COMPOSIT->CheckDocksState();
}


void AFQBlockManager::qslotChangeSceneDoubleClick()
{
	//Double Click Transition Hide
	return;
	//Double Click Transition Hide

	bool doubleClickSwitch = config_get_bool(USERCONFIG, "BasicWindow", "TransitionOnDoubleClick");

	if (doubleClickSwitch)
	{
		if (STATEAPP.IsPreviewProgramMode() == false)
			return;

		emit qsignalTransitionTriggered();
	}
}

void AFQBlockManager::qslotCloseCustom(QString key, int type)
{
	QWidget* widget = nullptr;

	switch(type)
	{
	case CustomTypes::CustomBrowser:
		if (m_browserWidgetMap.contains(key))
		{
			widget = m_browserWidgetMap.value(key);
			m_browserWidgetMap.remove(key);

			if(!m_browserDocks.contains(key))
				MAINFRAME->CustomBrowserStateChanged(key, false);
		}
		else
			blog(LOG_WARNING, "Close refused - No key found");
		break;
	case CustomTypes::Projector:
		if (m_projectorWidgetMap.contains(key))
		{
			widget = m_projectorWidgetMap.value(key);
			m_projectorWidgetMap.remove(key);
		}
		else
			blog(LOG_WARNING, "Close refused - No key found");
		break;
	case CustomTypes::ExtraBrowser:
		if (m_extraBrowserPopups.contains(key))
		{
			widget = m_extraBrowserPopups.value(key);
			m_extraBrowserPopups.remove(key);

			std::string strCloseScript = createCloseChatCefScript(key.toStdString().c_str());

			// chat cef popup : send close script 
			QWidget* chatwidget = nullptr;
			bool isfind = FindBlock(ENUM_WINDOW_TYPE::SoopChat, chatwidget);
			if (isfind) {
				QCefWidget* cefWidget = reinterpret_cast<QCefWidget*>(chatwidget);
				cefWidget->executeJavaScript(strCloseScript);
			}

			foreach(const QString & key, m_extraBrowserPopups.keys()) {
				AFQBorderPopupBaseWidget* extraPopup = m_extraBrowserPopups.value(key);
				if (extraPopup) {
					if (extraPopup->GetIsChatPopup()) {
						extraPopup->ExecuteScript(strCloseScript);
					}
				}
			}
		}
		else
			blog(LOG_WARNING, "Close refused - No key found");
		break;
	}
	
	if (widget)
	{
		widget->close();
		widget->deleteLater();
		widget = nullptr;
	}
}


void AFQBlockManager::qslotHideCustom(QString key, int type)
{
	if (CustomTypes::ExtraBrowser == type)
	{
		std::string strCloseScript = createCloseChatCefScript(key.toStdString().c_str());

		// chat cef popup : send close script 
		QWidget* chatwidget = nullptr;
		bool isfind = FindBlock(ENUM_WINDOW_TYPE::SoopChat, chatwidget);
		if (isfind) {
			QCefWidget* cefWidget = reinterpret_cast<QCefWidget*>(chatwidget);
			cefWidget->executeJavaScript(strCloseScript);
		}

		foreach(const QString & key, m_extraBrowserPopups.keys()) {
			AFQBorderPopupBaseWidget* extraPopup = m_extraBrowserPopups.value(key);
			if (extraPopup) {
				if (extraPopup->GetIsChatPopup()) {
					extraPopup->ExecuteScript(strCloseScript);
				}
			}
		}

	}
}

void AFQBlockManager::qslotLockDock(bool lock)
{
	QDockWidget::DockWidgetFeatures features =
		lock ? QDockWidget::NoDockWidgetFeatures
		: (QDockWidget::DockWidgetClosable |
			QDockWidget::DockWidgetMovable |
			QDockWidget::DockWidgetFloatable);

	QDockWidget::DockWidgetFeatures mainFeatures = features;
	mainFeatures &= ~QDockWidget::QDockWidget::DockWidgetClosable;

	foreach(AFQBaseDockWidget* dock, m_dockWidgets)
		dock->setFeatures(mainFeatures);
}

void AFQBlockManager::qslotDockTitleRefreshClicked(int blockType)
{
	QWidget* widget = nullptr;
	bool isFind = FindBlock((ENUM_WINDOW_TYPE)blockType, widget);
	if (isFind) {
		QCefWidget* cefWidget = reinterpret_cast<QCefWidget*>(widget);
		cefWidget->reloadPage();
	}
}

void AFQBlockManager::qslotToggleMenuCustomBrowser(bool show)
{
	QAction* custombrowserAction = reinterpret_cast<QAction*>(sender());

	QString uuid = custombrowserAction->property("uuid").toString();
	//
	if (show)
	{
		int dockCount = m_browserDocks.count() + m_browserWidgetMap.count();

		if (dockCount >= 10)
		{
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
				"", QTStr("CustomBrowser.Open.Denied"));

			custombrowserAction->setChecked(false);
			return;
		}

		for (int i = 0; i < m_customBrowserInfoVector.size(); i++) {
			if (uuid == m_customBrowserInfoVector[i].Uuid) {
				OpenCustomBrowserDock(m_customBrowserInfoVector[i], 0, true);
				m_customBrowserInfoVector[i].isOpen = true;
				break;
			}
		}
	}
	else
	{
		if (m_browserDocks.contains(uuid))
		{
			QWidget* widget = m_browserDocks.value(uuid);
			if (widget)
			{
				widget->close();
				delete widget;
				widget = nullptr;

				m_browserDocks.remove(uuid);
				return;
			}
		}

		if (m_browserWidgetMap.contains(uuid))
		{
			QWidget* widget = m_browserWidgetMap.value(uuid);
			if (widget)
			{
				widget->close();
				delete widget;
				widget = nullptr;

				m_browserWidgetMap.remove(uuid);
				return;
			}
		}
	}
}

void AFQBlockManager::qslotCloseUuidDock(QString uuid, bool closeCef)
{
	if (m_browserDocks.contains(uuid))
	{
		AFQBaseDockWidget* widget = m_browserDocks.value(uuid);
		if (widget)
		{
			widget->SetCloseCef(closeCef);
			widget->close();
			delete widget;
			widget = nullptr;

			m_browserDocks.remove(uuid);

			if (!m_browserWidgetMap.contains(uuid))
				MAINFRAME->CustomBrowserStateChanged(uuid, false);

			DYNAMIC_COMPOSIT->CheckDocksState();
			return;
		}
	}
}

void AFQBlockManager::qslotCefQueryReceived(int blockType, const QCefQuery& query)
{
	if (ENUM_WINDOW_TYPE::SoopChat == blockType) {
		qslotSoopChatDataReceived(query);
	}
	else if (ENUM_WINDOW_TYPE::Mission == blockType ||
			ENUM_WINDOW_TYPE::SAVVYReaction == blockType) {
		qslotCefPopupDataReceived(query);
	}
	else if (ENUM_WINDOW_TYPE::Extensions == blockType) {
		qslotExtensionDataReceived(query);
	}
	else {
		qslotSoopChatDataReceived(query);
	}
}

void AFQBlockManager::qslotSoopChatDataReceived(const QCefQuery& query)
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
	if (!senderWidget) {
		return;
	}

	{
		std::lock_guard<std::mutex> lock(m_cefQueryMutex);
		m_cefQueryQueue.push({ senderWidget, query });
	}
	m_cefQueryCV.notify_one();
}

void AFQBlockManager::qslotShowSceneControlDockTriggered()
{
	AFQBorderPopupBaseWidget* popup = nullptr;
	MakePopup(ENUM_WINDOW_TYPE::SceneControl, popup);
}

void AFQBlockManager::qslotProjectorFullScreenTriggered()
{
	AFQProjector* projector = reinterpret_cast<AFQProjector*>(sender());
	QString uuid = projector->property("uuid").toString();
	if (m_projectorWidgetMap.contains(uuid))
	{
		AFDockTitle* dockTitle = m_projectorWidgetMap[uuid]->findChild<AFDockTitle*>();
		if(dockTitle)
			dockTitle->hide();
	}
}

void AFQBlockManager::qslotProjectorWindowTriggered()
{
	AFQProjector* projector = reinterpret_cast<AFQProjector*>(sender());
	QString uuid = projector->property("uuid").toString();
	if (m_projectorWidgetMap.contains(uuid))
	{
		AFDockTitle* dockTitle = m_projectorWidgetMap[uuid]->findChild<AFDockTitle*>();
		if(dockTitle)
			dockTitle->show();
	}
}

bool AFQBlockManager::InitPopups()
{
	connect(MAINFRAME, &AFMainFrame::qsignalMainShowEventTriggered,
		this, &AFQBlockManager::qslotShowAllOpenedBlocksOnExecute);

	m_extraBrowserPopups.clear();

	_SetBlockBaseInfo();

	QMap<int, const char*> tooltipmap;
	_SetBlockBaseInfoExtra(tooltipmap);

	_LoadCustomBrowserList();
	_CreateCustomBrowserBlock();

	_CreateSceneSourceBlock();
	_CreateAudioMixer();
	_CreateBroadInfo();

	CreatePlatformPage();
	_CreateYoutubeChat("","");

	_LoadBlock();

	return true;
}

void AFQBlockManager::FinPopups()
{
	_SaveBlock();
	
	_DeleteYoutubePage();
	DeletePlatformPage();

	_SaveCustomBrowserBlock();

	_ClearAllBlocks();

	foreach(AFQBorderPopupBaseWidget * projector, m_projectorWidgetMap)
	{
		if (projector)
		{
			projector->close();
			projector->deleteLater();
		}
	}
	m_projectorWidgetMap.clear();

	foreach(AFQBorderPopupBaseWidget* extra, m_extraBrowserPopups)
	{
		if (extra)
		{
			extra->close();
			extra->deleteLater();
		}
	}
	m_extraBrowserPopups.clear();
}

void AFQBlockManager::CreatePlatformPage(std::string platform, bool reset)
{
	if (platform == PLATFORM_SOOP)
		CreateSoopPage();
	else if (platform == PLATFORM_TWITCH)
	{
		CreateTwitchPage(reset);
	}
	else if (platform == "")
	{	
		CreateTwitchPage(true);
		CreateSoopPage();
	}
}

void AFQBlockManager::DeletePlatformPage(std::string platform)
{
	if (platform == PLATFORM_SOOP)
		_DeleteSoopPage();
	else if (platform == PLATFORM_TWITCH)
		_DeleteTwitchPage();
	else if (platform == "")
	{
		_DeleteSoopPage();
		_DeleteTwitchPage();
	}
}

bool AFQBlockManager::AddBlock(ENUM_WINDOW_TYPE type, QWidget* contents)
{
	if (m_blockMap.contains(type))
		return false;

	m_blockMap.insert(type, contents);

	return true;
}

bool AFQBlockManager::FindBlock(ENUM_WINDOW_TYPE type, QWidget*& outBlock) const
{
	if (m_blockMap.contains(type))
	{
		outBlock = m_blockMap.value(type);
		return true;
	}
	return false;
}

void AFQBlockManager::ReloadCustomBrowserList(QVector<CUSTOMBROWSERINFO> info,	QList<QString> deletedUuid)
{
	m_customBrowserInfoVector = info;

	foreach (QString uuid, deletedUuid)
	{
		qslotCloseUuidDock(uuid, true);
		qslotCloseCustom(uuid, CustomTypes::CustomBrowser);
		for (int i = 0; i < m_customBrowserInfoVector.size(); i++)
		{
			if (uuid == m_customBrowserInfoVector[i].Uuid)
			{
				m_customBrowserInfoVector.remove(i);
				break;
			}
		}
	}

	int newCount = 0;

	foreach (CUSTOMBROWSERINFO info, m_customBrowserInfoVector)
	{
		if (info.isOpen)
		{
			if (OpenCustomBrowserDock(info, newCount))
			{
				newCount++;
				continue;
			}
		}
		else
		{
			qslotCloseUuidDock(info.Uuid, true);
			continue;
		}

		foreach(AFQBorderPopupBaseWidget * check, m_browserWidgetMap)
		{
			QString uuid = check->property("uuid").toString();
			if (uuid == info.Uuid)
			{
				QList<AFDockTitle*> checktitleList = check->findChildren<AFDockTitle*>();
				if (checktitleList.count() > 0)
				{
					checktitleList[0]->setWindowTitle(info.Name);
					check->setWindowTitle(info.Name);
				}
				check->SetUrl(info.Url.toStdString());
			}
		}

		foreach(AFQBaseDockWidget * check, m_browserDocks)
		{
			QString uuid = check->property("uuid").toString();
			if (uuid == info.Uuid)
			{
				QList<AFDockTitle*> checktitleList = check->findChildren<AFDockTitle*>();
				if (checktitleList.count() > 0)
				{
					checktitleList[0]->setWindowTitle(info.Name);
					check->setWindowTitle(info.Name);
				}

				check->setObjectName(info.Name + "_extra");
				check->SetUrl(info.Url.toStdString());
			}
		}
	}
	DYNAMIC_COMPOSIT->CheckDocksState();
}


bool AFQBlockManager::MakeDock(ENUM_WINDOW_TYPE type, AFQBaseDockWidget*& outDock, bool showDock)
{
	//DockWidget Move Resize Dont Work on Transparent
	if (GetDock(type, outDock))
	{
		outDock->setVisible(showDock);
		outDock->raise();
		outDock->setFocus(Qt::OtherFocusReason);

		_ChangeBlockUse(showDock, type);

		DYNAMIC_COMPOSIT->CheckDocksState();
		return false;
	}

	AFQBorderPopupBaseWidget* popup = nullptr;
	if (GetPopup(type, popup))
	{
		popup->raise();
		return false;
	}

	AFQBlockManager::BlockBaseInfo baseInfo;
	bool registeredInBlock = _GetBlockBaseInfo(type, baseInfo);
	if (!registeredInBlock || !baseInfo.transformable)
		return false;
	
	outDock = new AFQBaseDockWidget(type, DYNAMIC_COMPOSIT);
	QMetaEnum BlockTypeEnum = QMetaEnum::fromType<WindowTypes>();
	const char* key = BlockTypeEnum.valueToKey(type);
	QString keystring = QString(key);
	outDock->setObjectName(keystring);
	connect(outDock, &AFQBaseDockWidget::qsignalCloseDock, this, &AFQBlockManager::qslotHideDock);

	QWidget* block;
	if (FindBlock(type, block))
	{
		AFDockTitle* dockTitle = new AFDockTitle(outDock);
		_BasicSetTitleWidget(type, dockTitle, false);

		dockTitle->setParent(outDock);
		block->setParent(outDock);

		outDock->setWindowTitle(QTStr(baseInfo.windowTitle));

		QSize miniSize = QSize(baseInfo.minSize.width(), DOCK_MIN_SIZE_HEIGHT);
		outDock->setMinimumSize(miniSize);
		outDock->setMaximumSize(baseInfo.maxSize);

		outDock->setTitleBarWidget(dockTitle);

		if (baseInfo.isCef)
		{
			if (auto cef = reinterpret_cast<QCefWidget*>(block))
			{
				outDock->AddCefWidget(cef);
				connect(cef, SIGNAL(cefCreateAfter()),
					outDock, SLOT(qslotReceivedCreateAfterCefBrowser()));

				connect(cef, SIGNAL(cefLoadEnd()),
					outDock, SLOT(qslotReceivedLoadEndCefBrowser()));

				connect(outDock, &AFQBaseDockWidget::qsignalCreateAfterCefBrowser,
					this, &AFQBlockManager::qslotCreateAfterCefBrowser);

				connect(outDock, &AFQBaseDockWidget::qsignalLoadEndCefBrowser,
					this, &AFQBlockManager::qslotLoadEndCefBrowser);
			}
			else
				outDock->setWidget(block);
		}
		else
		{
			outDock->setWidget(block);
		}

		if (type == ENUM_WINDOW_TYPE::SoopChat)
		{
			connect(block, SIGNAL(cefQueryRequest(const QCefQuery&)),
				outDock, SLOT(qslotDataReceivedFromBrowserToDock(const QCefQuery&)));

			connect(outDock, &AFQBaseDockWidget::qsignalDataFromBrowser,
				this, &AFQBlockManager::qslotCefQueryReceived);

			dockTitle->ShowRefreshButton();
			connect(dockTitle, &AFDockTitle::qsignalRefreshButton,
				this, &AFQBlockManager::qslotDockTitleRefreshClicked);
		}

		outDock->SetStyle();
		outDock->setAllowedAreas(Qt::DockWidgetArea::AllDockWidgetAreas);

		DYNAMIC_COMPOSIT->addDockWidget(Qt::BottomDockWidgetArea, outDock);
		//outDock->setFloating(true);

		QApplication::processEvents();

		outDock->setVisible(showDock);
		outDock->raise();
		outDock->setFocus(Qt::OtherFocusReason);

		m_dockWidgets.insert(type, outDock);
		_ChangeBlockUse(showDock, type);

		DYNAMIC_COMPOSIT->CheckDocksState();
		return true;
	}

	DYNAMIC_COMPOSIT->CheckDocksState();
	return false;
}

bool AFQBlockManager::MakePopup(int type, AFQBorderPopupBaseWidget*& outPopup, bool toPopup, bool showPopup, bool hasParent, QWidget* parent)
{
	ENUM_WINDOW_TYPE val = static_cast<ENUM_WINDOW_TYPE>(type);

	if (!toPopup)
	{
		AFQBaseDockWidget* outDock = nullptr;
		if (GetDock(val, outDock))
			if(outDock)
				if (outDock->isVisible())
				{
					outDock->raise();
					outDock->setFocus(Qt::OtherFocusReason);
					return false;
				}
				else
				{
					_ToPopup(val);
					return false;
				}
	}

	AFQBlockManager::BlockBaseInfo baseInfo;

	QWidget* parentWindow = nullptr;
	QSize minimum = QSize(0,0);
	QSize maximum = QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
	bool transparentWindow = false;
	Qt::WindowFlags flag = Qt::Window;

	if (_GetBlockBaseInfo(val, baseInfo))
	{
		if (hasParent)
		{
			parentWindow = parent;
		}
		else
		{
			switch (baseInfo.parentOnPopup)
			{
			case AFQBlockManager::ParentTypes::NoParent:
				parentWindow = nullptr;
				break;
			case AFQBlockManager::ParentTypes::MainView:
				parentWindow = MAINFRAME;
				break;
			case AFQBlockManager::ParentTypes::MainWindow:
				parentWindow = DYNAMIC_COMPOSIT;
				break;
			default:
				parentWindow = nullptr;
				break;
			}
		}

		minimum = baseInfo.minSize;
		maximum = baseInfo.maxSize;
		transparentWindow = baseInfo.transparent;
		flag = baseInfo.flags;
	}

	return _MakePopup(val, outPopup, minimum, maximum, transparentWindow, 
		baseInfo.showLeft, baseInfo.showRight, parentWindow, flag, showPopup, baseInfo.isCef);
}

bool AFQBlockManager::GetPopup(ENUM_WINDOW_TYPE type, AFQBorderPopupBaseWidget*& outPopup) const
{
	if (m_popupWidgets.contains(type))
	{
		outPopup = reinterpret_cast<AFQBorderPopupBaseWidget*>(m_popupWidgets.value(type));
		return true;
	}

	return false;
}

bool AFQBlockManager::GetDock(ENUM_WINDOW_TYPE type, AFQBaseDockWidget*& outDock) const
{
	if (m_dockWidgets.contains(type))
	{
		outDock = m_dockWidgets.value(type);
		return true;
	}

	return false;
}

void AFQBlockManager::RaiseAllPopup()
{
	foreach(QWidget * block, m_popupWidgets)
	{
		if (block->isMinimized())
			block->showNormal();

		block->raise();
		block->activateWindow();
		block->setFocus();
	}
}

void AFQBlockManager::ResetDockUI(bool visible)
{
	AFQBaseDockWidget* broadInfoDock = nullptr;
	_ResetBlockToDock(ENUM_WINDOW_TYPE::BroadInfo, broadInfoDock);

	if (broadInfoDock && AUTH_CONTEXT.IsSoopRegistered())
	{
		broadInfoDock->setFloating(false);
		DYNAMIC_COMPOSIT->addDockWidget(Qt::BottomDockWidgetArea, broadInfoDock);
		broadInfoDock->setVisible(visible);
	}

	AFQBaseDockWidget* sceneSourceDock = nullptr;
	_ResetBlockToDock(ENUM_WINDOW_TYPE::SceneSource, sceneSourceDock);
	sceneSourceDock->setFloating(false);

	AFQBaseDockWidget* audioDock = nullptr;
	_ResetBlockToDock(ENUM_WINDOW_TYPE::AudioMixer, audioDock);
	audioDock->setFloating(false);

	DYNAMIC_COMPOSIT->addDockWidget(Qt::BottomDockWidgetArea, sceneSourceDock);

	if(broadInfoDock && AUTH_CONTEXT.IsSoopRegistered())
		DYNAMIC_COMPOSIT->splitDockWidget(broadInfoDock, sceneSourceDock, Qt::Vertical);

	DYNAMIC_COMPOSIT->addDockWidget(Qt::BottomDockWidgetArea, audioDock);

	sceneSourceDock->setVisible(visible);
	audioDock->setVisible(visible);
}

void AFQBlockManager::CloseAllBlocks(QList<ENUM_WINDOW_TYPE> exclusions)
{
	_CloseAllDocks(exclusions);
	_CloseAllPopups(exclusions);

	foreach(AFQBorderPopupBaseWidget * projector, m_projectorWidgetMap)
	{
		if (projector)
		{
			projector->close();
			projector->deleteLater();
		}
	}
	m_projectorWidgetMap.clear();

	if (!exclusions.contains(ENUM_WINDOW_TYPE::SoopChat))
	{
		foreach(AFQBorderPopupBaseWidget* extra, m_extraBrowserPopups)
		{
			if (extra)
			{
				extra->close();
				extra->deleteLater();
			}
		}
	}
	
	m_extraBrowserPopups.clear();

	MAINFRAME->CustomBrowserStateAllChanged(false);

	foreach(AFQBaseDockWidget* extra, m_browserDocks)
	{
		if (extra)
		{
			extra->close();
			extra->deleteLater();
		}
	}
	m_browserDocks.clear();

	foreach(QWidget * widget, m_browserWidgetMap)
	{
		if (widget->isWidgetType())
		{
			widget->close();
			widget->deleteLater();
			widget = nullptr;
		}
	}

	m_browserWidgetMap.clear();

	DYNAMIC_COMPOSIT->CheckDocksState();
}

void AFQBlockManager::InitDefaultDock()
{
	AFQBaseDockWidget* broadInfoDock = nullptr;
	if (AUTH_CONTEXT.IsSoopRegistered())
	{
		_ResetBlockToDock(ENUM_WINDOW_TYPE::BroadInfo, broadInfoDock);
		if (broadInfoDock)
		{
			broadInfoDock->setFloating(false);
			DYNAMIC_COMPOSIT->addDockWidget(Qt::BottomDockWidgetArea, broadInfoDock);
			broadInfoDock->show();
			
		}
	}

	AFQBaseDockWidget* sceneSourceDock = nullptr;
	_ResetBlockToDock(ENUM_WINDOW_TYPE::SceneSource, sceneSourceDock);
	if (sceneSourceDock)
	{
		sceneSourceDock->setFloating(false);
		DYNAMIC_COMPOSIT->addDockWidget(Qt::BottomDockWidgetArea, sceneSourceDock);
		sceneSourceDock->show();
	}

	AFQBaseDockWidget* audioMixerDock = nullptr;
	if (!AUTH_CONTEXT.IsSoopRegistered())
	{
		_ResetBlockToDock(ENUM_WINDOW_TYPE::AudioMixer, audioMixerDock);
		if (audioMixerDock)
		{
			audioMixerDock->setFloating(false);
			DYNAMIC_COMPOSIT->addDockWidget(Qt::BottomDockWidgetArea, audioMixerDock);
			audioMixerDock->show();
		}
	}

	QList<QDockWidget*> docks;
	QList<int> HorizontalSizes;
	QList<int> VerticalSizes;
	if (AUTH_CONTEXT.IsSoopRegistered()) {
		docks.append(broadInfoDock);
		HorizontalSizes.append(408);
		VerticalSizes.append(230);
	}

	docks.append(sceneSourceDock);
	HorizontalSizes.append(408);
	VerticalSizes.append(230);

	if (!AUTH_CONTEXT.IsSoopRegistered()) {
		docks.append(audioMixerDock);
		HorizontalSizes.append(408);
		VerticalSizes.append(230);
	}

	DYNAMIC_COMPOSIT->resizeDocks(docks, HorizontalSizes, Qt::Horizontal);
	DYNAMIC_COMPOSIT->resizeDocks(docks, VerticalSizes, Qt::Vertical);

	DYNAMIC_COMPOSIT->CheckDocksState();
}

QString AFQBlockManager::MakeProjector(int MonitorNum)
{
	AFQProjector* project = nullptr;
	QString sourceName;
	if (_CreateCurrentSceneProjector(project, sourceName, MonitorNum))
	{
		QString uuid = QUuid::createUuid().toString();

		AFQBorderPopupBaseWidget* projectWidget = 
			new AFQBorderPopupBaseWidget(uuid, CustomTypes::Projector, nullptr, Qt::Window);

		projectWidget->setMinimumSize(480, 310);
		projectWidget->resize(480, 310);
		projectWidget->setWindowTitle(sourceName);

		bool isFullScreen = MonitorNum >= 0 ? true : false;

		if (isFullScreen)
		{
			QScreen* screen = QGuiApplication::screens()[MonitorNum];
			projectWidget->move(screen->geometry().x(), screen->geometry().y());
		}
		else
		{
			QRect midPosition = GetMidGeometry(projectWidget->size());
			projectWidget->move(midPosition.x(), midPosition.y());
		}

		connect(project, &AFQProjector::qsignalFullScreenProjector,
			this, &AFQBlockManager::qslotProjectorFullScreenTriggered);
		connect(project, &AFQProjector::qsignalWindowProjector, 
			this, &AFQBlockManager::qslotProjectorWindowTriggered);

		AFDockTitle* dockTitle = new AFDockTitle(projectWidget);

		QString titleName = QString("%1 - %2").arg(QTStr("SceneWindow")).arg(sourceName);
		dockTitle->InitializeCustom(titleName, uuid, true);

		if (isFullScreen)
			dockTitle->hide();

		projectWidget->AddWidget(dockTitle);
		projectWidget->AddWidget(project);
		ApplyMoveInAllArea(projectWidget);
		
		projectWidget->SetDockContentsMargin(0, 0, 0, 0);

		//Show right after move doesn't change monitor
		if (isFullScreen) {
			projectWidget->showFullScreen();
		}
		else
			projectWidget->show();
			
		m_projectorWidgetMap.insert(uuid, projectWidget);
		return uuid;
	}
	return QString();
}

void AFQBlockManager::OpenCustomBrowserBlock(CUSTOMBROWSERINFO info, int overlapCount, bool firstRun)
{
	if (m_browserWidgetMap.contains(info.Uuid))
		return;

	AFQBorderPopupBaseWidget* customWidget = 
		new AFQBorderPopupBaseWidget(info.Uuid, CustomTypes::CustomBrowser);

	customWidget->setMinimumSize(QSize(CUSTOM_BROWSER_MIN_WIDTH, CUSTOM_BROWSER_MIN_HEIGHT));
	customWidget->setWindowTitle(info.Name);

	AFDockTitle* dockTitle = new AFDockTitle(customWidget);
	dockTitle->InitializeCustom(info.Name, info.Uuid, true, true);
	dockTitle->SetToggleWindowToDockButton(true);

	connect(dockTitle, &AFDockTitle::qsignalToggleCustomDock,
		this, &AFQBlockManager::qslotSwitchCustomBrowser);

	customWidget->AddWidget(dockTitle);

	QCefWidget* cefWidget = CEFMANAGER.createWidget(customWidget, info.Url.toStdString());
	static int panel_version = -1;
	if (panel_version == -1) {
		panel_version = obs_browser_qcef_version();
	}
	if (cefWidget && panel_version >= 1)
		cefWidget->allowAllPopups(true);

	customWidget->AddCefWidget(cefWidget, info.Url.toStdString());

	QRect browserGeo;

	if (overlapCount == -1)
	{
		browserGeo = QRect(info.x, info.y, info.width, info.height);
	}
	else
	{
		browserGeo = QRect(MAINFRAME->pos().x() - CUSTOM_BROWSER_MIN_WIDTH,
			MAINFRAME->pos().y() + overlapCount * dockTitle->height(),
			CUSTOM_BROWSER_MIN_WIDTH,
			CUSTOM_BROWSER_MIN_HEIGHT);
	}

	QRect adjustRect;
	AdjustPositionOutSideFullScreen(browserGeo, adjustRect);
	customWidget->setGeometry(adjustRect);
	ApplyMoveInAllArea(customWidget);
	//QApplication::processEvents();
	if (firstRun)
		customWidget->hide();
	else
		customWidget->show();

	m_browserWidgetMap.insert(info.Uuid, customWidget);
}

bool AFQBlockManager::OpenCustomBrowserDock(CUSTOMBROWSERINFO info, int overlapCount, bool firstRun)
{
	if (m_browserWidgetMap.contains(info.Uuid) || m_browserDocks.contains(info.Uuid))
		return false;

	AFQBaseDockWidget* customWidget =
		new AFQBaseDockWidget(info.Uuid, CustomTypes::CustomBrowser, DYNAMIC_COMPOSIT);

	QString bId(info.Uuid.isEmpty() ? QUuid::createUuid().toString() : info.Uuid);
	bId.replace(QRegularExpression("[{}-]"), "");
	customWidget->setProperty("uuid", bId);
	customWidget->setObjectName(info.Name + "_extra");
	customWidget->setMinimumSize(QSize(AUDIO_MIXER_MIN_SIZE_WIDTH, DOCK_MIN_SIZE_HEIGHT));

	customWidget->setAllowedAreas(Qt::AllDockWidgetAreas);

	DYNAMIC_COMPOSIT->addDockWidget(Qt::RightDockWidgetArea, customWidget);

	AFDockTitle* dockTitle = new AFDockTitle(customWidget);
	dockTitle->InitializeCustom(info.Name, info.Uuid, false, false, true);
	dockTitle->SetIsPopup(false);
	connect(dockTitle, &AFDockTitle::qsignalToggleCustomDock,
		this, &AFQBlockManager::qslotSwitchCustomBrowser);

	customWidget->setWindowTitle(info.Name);

	QCefWidget* cefWidget = CEFMANAGER.createWidget(nullptr, info.Url.toStdString());

	static int panel_version = -1;
	if (panel_version == -1) {
		panel_version = obs_browser_qcef_version();
	}
	if (cefWidget && panel_version >= 1)
		cefWidget->allowAllPopups(true);

	customWidget->setTitleBarWidget(dockTitle);

	customWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	customWidget->titleBarWidget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

	customWidget->AddCefWidget(cefWidget, info.Url.toStdString());
	customWidget->SetStyle();
	customWidget->adjustSize();

	m_browserDocks.insert(info.Uuid, customWidget);


	QRect browserGeo;
	if (firstRun) {
		customWidget->setFloating(true);
		if (overlapCount == -1)
		{
			browserGeo = QRect(info.x, info.y, info.width, info.height);
		}
		else
		{
			browserGeo = QRect(MAINFRAME->pos().x() - CUSTOM_BROWSER_MIN_WIDTH,
				MAINFRAME->pos().y() + overlapCount * dockTitle->height(),
				CUSTOM_BROWSER_MIN_WIDTH,
				CUSTOM_BROWSER_MIN_HEIGHT);
		}

		QRect adjustRect;
		AdjustPositionOutSideFullScreen(browserGeo, adjustRect);
		customWidget->setGeometry(adjustRect);

		customWidget->setVisible(true);
	}
	return true;
}

void AFQBlockManager::ChangeCustomBrowserOpenState(QString uuid, bool isOpen)
{
	for (int i = 0; i < m_customBrowserInfoVector.size(); i++) {
		if (uuid == m_customBrowserInfoVector[i].Uuid) {
			m_customBrowserInfoVector[i].isOpen = isOpen;
			break;
		}
	}
}

void AFQBlockManager::ChangeAllCustomBrowserOpenState(bool isOpen)
{
	for (int i = 0; i < m_customBrowserInfoVector.size(); i++) 
			m_customBrowserInfoVector[i].isOpen = isOpen;
	
}

void AFQBlockManager::ShowAllCustomBrowser()
{
	foreach(QDockWidget * w, m_browserDocks)
		w->show();
}

void AFQBlockManager::OpenExtraBrowserPopup(QString key, QString title, std::string url, QSize size, bool allowedHide, QWidget* parent)
{
	AFQBorderPopupBaseWidget* extraBrowser = nullptr;

	if (m_extraBrowserPopups.contains(key))
	{
		if (allowedHide) {
			 extraBrowser = m_extraBrowserPopups.value(key);
			if (extraBrowser && extraBrowser->GetIsHidePopup()) {
				extraBrowser->show();
			}
		}
	} 
	else 
	{
		extraBrowser = new AFQBorderPopupBaseWidget(key, CustomTypes::ExtraBrowser, nullptr, Qt::Window);
		extraBrowser->setMinimumSize(QSize(size.width(), size.height()));
		extraBrowser->SetIsChatPopup(true);
		extraBrowser->SetIsHidePopup(allowedHide);
		extraBrowser->setWindowTitle(title);

		std::smatch match;
		std::regex id_pattern(R"([?&]id=([^&?#]+))");
		auto needQuestionMark = false;
		QString questionMarkToolTip;
		if (std::regex_search(url, match, id_pattern)) {
			std::string id = match[1].str();
			//if (id == OPEN_EXTRA_BROWSER_POPUP_STUDIO_SUBSCRIPTION_GIFT_MODAL_LAYER)
			//    questionMarkToolTip = QTStr("Block.Tooltip.Subscription");
			//else
			if (id == OPEN_EXTRA_BROWSER_POPUP_STUDIO_QUICKVIEW_GIFT_MODAL_LAYER)
				questionMarkToolTip = QTStr("Block.Tooltip.QuickView");
			else if (id == OPEN_EXTRA_BROWSER_POPUP_STUDIO_RANDOM_GIFT_MODAL_LAYER)
				questionMarkToolTip = QTStr("Block.Tooltip.Random");

			if (questionMarkToolTip.length())
				needQuestionMark = true;
		}

		QString uuid = QUuid::createUuid().toString();
		AFDockTitle* dockTitle = new AFDockTitle(extraBrowser);
		dockTitle->InitializeCustom(title, uuid, false, false, false, needQuestionMark, questionMarkToolTip);
		dockTitle->SetHidePopup(allowedHide);

		QCefWidget* cefWidget = CEFMANAGER.createWidget(extraBrowser, url);
		extraBrowser->AddWidget(dockTitle);
		extraBrowser->AddCefWidget(cefWidget, url);
		cefWidget->setProperty("key", key);
		cefWidget->hide();

		// Cef Query Event
		connect(extraBrowser, &AFQBorderPopupBaseWidget::qsignalDataFromBrowser,
			this, &AFQBlockManager::qslotCefQueryReceived);

		connect(cefWidget, SIGNAL(cefQueryRequest(const QCefQuery&)),
			extraBrowser, SLOT(qslotDataReceivedFromBrowser(const QCefQuery&)));

		ApplyMoveInAllArea(extraBrowser);
		m_extraBrowserPopups.insert(key, extraBrowser);

		cefWidget->show();
	}

	if (!extraBrowser)
		return;

	QRect adjustRect;
	QRect browserGeo = QRect(MAINFRAME->x() + MAINFRAME->width(), MAINFRAME->y(),
		extraBrowser->width(), extraBrowser->height());
	QRect parentRect = MAINFRAME->geometry();

	if (parent)
	{
		bool attachToRight = false;

		if (!m_dockWidgets.contains(ENUM_WINDOW_TYPE::SoopChat))
		{
			attachToRight = true;
		}
		else
		{
			QDockWidget* outDock = m_dockWidgets.value(ENUM_WINDOW_TYPE::SoopChat);
			attachToRight = (outDock && outDock->isFloating());
		}

		if (attachToRight)
		{
			parentRect = parent->geometry();
			browserGeo = QRect(parent->x() + parent->width(),
				parent->y(),
				extraBrowser->width(),
				extraBrowser->height());
		}

		
		ReversePositionOutSideFullScreen(parentRect, browserGeo, adjustRect);
	}
	else
		adjustRect = parentRect;

	QRect adjustOutsideRect;
	AdjustPositionOutSideFullScreen(adjustRect, adjustOutsideRect);

	extraBrowser->setGeometry(adjustOutsideRect);
	extraBrowser->resize(size);
	extraBrowser->show();
}

void AFQBlockManager::ExecuteBrowserScript(QString key, const std::string& script)
{
	if (m_extraBrowserPopups.contains(key))
		m_extraBrowserPopups.value(key)->ExecuteScript(script);
}

void AFQBlockManager::ApplyBroadInfoToUI()
{
	auto& authManager = AUTH_CONTEXT;
	//
	if (authManager.IsSoopRegistered())
		authManager.RequestBroadInfoAPI();

	QWidget* outBlock = nullptr;
	if (!FindBlock(ENUM_WINDOW_TYPE::BroadInfo, outBlock))
		return;

	AFBroadInfoDockWidget* broadInfoWidget = qobject_cast<AFBroadInfoDockWidget*>(outBlock);
	if (broadInfoWidget)
		broadInfoWidget->LoadBroadInfoUI();
}

bool AFQBlockManager::CreateCustomBrowserListMenu(AFQCustomMenu*& menu)
{
	for (int i = 0; i < m_customBrowserInfoVector.size(); ++i)
	{
		QAction* customBrowser = new QAction(menu);
		customBrowser->setProperty("uuid", m_customBrowserInfoVector[i].Uuid);

		QFontMetrics metricsTitle(m_customBrowserInfoVector[i].Name);
		int titleWidth = 180;

		QString elidedTitle = metricsTitle.elidedText(m_customBrowserInfoVector[i].Name, Qt::ElideRight, titleWidth);

		customBrowser->setText(elidedTitle);
		customBrowser->setCheckable(true);
		bool isOpened = m_browserWidgetMap.contains(m_customBrowserInfoVector[i].Uuid) 
			|| m_browserDocks.contains(m_customBrowserInfoVector[i].Uuid);

		customBrowser->setChecked(isOpened);
		m_customBrowserInfoVector[i].isOpen = isOpened;

		connect(customBrowser, &QAction::triggered, this, &AFQBlockManager::qslotToggleMenuCustomBrowser);
		menu->addAction(customBrowser);
	}

	return true;
}

bool AFQBlockManager::_GetBlockBaseInfo(ENUM_WINDOW_TYPE type, AFQBlockManager::BlockBaseInfo& info)
{
	if (!m_blockBaseInfoMap.contains(type))
		return false;

	info = m_blockBaseInfoMap.value(type);

	return true;
}

bool AFQBlockManager::_MakePopup(ENUM_WINDOW_TYPE type, AFQBorderPopupBaseWidget*& outPopup,
	QSize min, QSize max, bool transparent, bool showLeft, bool showRight, QWidget* parent, 
	Qt::WindowFlags flags, bool showPopup, bool isCef)
{
	if (GetPopup(type, outPopup))
	{
		QApplication::processEvents();
		outPopup->show();
		outPopup->activateWindow();
		outPopup->raise();
		return true;
	}

	bool widthEnable = true;
	bool heightEnable = true;
	if (min.width() == max.width())
		widthEnable = false;

	if (min.height() == max.height())
		heightEnable = false;

	QWidget* block;
	if (!FindBlock(type, block))
	{
		if (!_CreateNeededBlock(type, block))
			return false;
	}

	outPopup = new AFQBorderPopupBaseWidget(type, parent, flags, widthEnable, heightEnable);
	outPopup->setAttribute(Qt::WA_DeleteOnClose);

	QCefWidget* cefWidget = reinterpret_cast<QCefWidget*>(block);
	connect(cefWidget, SIGNAL(cefCreateAfter()),
		outPopup, SLOT(qslotReceivedCreateAfterCefBrowser()));

	connect(cefWidget, SIGNAL(cefLoadEnd()),
		outPopup, SLOT(qslotReceivedLoadEndCefBrowser()));

	connect(outPopup, &AFQBorderPopupBaseWidget::qsignalCreateAfterCefBrowser,
		this, &AFQBlockManager::qslotCreateAfterCefBrowser);

	connect(outPopup, &AFQBorderPopupBaseWidget::qsignalLoadEndCefBrowser,
		this, &AFQBlockManager::qslotLoadEndCefBrowser);

	if (type == ENUM_WINDOW_TYPE::SoopChat)
		connect(outPopup, &AFQBorderPopupBaseWidget::qsignalCloseChatEventTriggered,
		this, &AFQBlockManager::qslotCloseSoopChat);
	else
		connect(outPopup, &AFQBorderPopupBaseWidget::qsignalCloseEventTriggered,
			this, &AFQBlockManager::qslotClosePopup);
		

	QSize MinSize = min;
	QSize MaxSize = max;

	if (MAINFRAME->IsSmallResolution())
	{
		switch (type)
		{
		case ENUM_WINDOW_TYPE::SAVVYReaction:
		case ENUM_WINDOW_TYPE::Mission:
			MinSize.setHeight(550);
			MaxSize.setHeight(550);
			MinSize.setWidth(800);
			MaxSize.setWidth(800);
			break;
		case ENUM_WINDOW_TYPE::Extensions:
		case ENUM_WINDOW_TYPE::StatPage:
		case ENUM_WINDOW_TYPE::AquaControl:
			MinSize.setHeight(550);
			MaxSize.setHeight(550);
			break;
		}
	}

	outPopup->setMinimumSize(MinSize);
	outPopup->setMaximumSize(MaxSize);

	bool HasMinMax = widthEnable || heightEnable;

    AFDockTitle* dockTitle = new AFDockTitle(outPopup);
    _BasicSetTitleWidget(type, dockTitle, true, HasMinMax);


	QMetaEnum BlockTypeEnum = QMetaEnum::fromType<ENUM_WINDOW_TYPE>();
	QString blockType = BlockTypeEnum.valueToKey(type);

#ifdef _DEBUG
	DWORD pid = outPopup->GetPid();
	QString pidStr = "[PID] " + blockType + " pid: " + QString::number(pid);
	blog(LOG_INFO, pidStr.toStdString().c_str());
#endif

    dockTitle->setProperty("dock", false);
    dockTitle->setProperty("dockable", false);
    PolishStyleSheet(dockTitle);

    AFQBlockManager::BlockBaseInfo baseInfo;
    if (_GetBlockBaseInfo(type, baseInfo)){
        outPopup->setWindowTitle(QTStr(baseInfo.windowTitle));
    }

	QWidget* dockContents = block->findChild<QWidget*>("dockContainer");
	if (dockContents) {
		dockContents->setProperty("dock", false);
		PolishStyleSheet(dockContents);
	}
    
#ifndef __APPLE__
	dockTitle->setParent(outPopup);
	block->setParent(outPopup);

	outPopup->AddWidget(dockTitle);
#endif

	bool cef = false;
	if (isCef)
	{
		QCefWidget* cefCheck = reinterpret_cast<QCefWidget*>(block);
		if (cefCheck)
		{
			bool isRefreshButton = false;
			cefCheck->m_cefPopupType = type;

			switch (type)
			{
			case ENUM_WINDOW_TYPE::Extensions:
			case ENUM_WINDOW_TYPE::SoopChat:
				isRefreshButton = true;
				break;

			case ENUM_WINDOW_TYPE::Mission:
			case ENUM_WINDOW_TYPE::SAVVYReaction:
			default:
				break;
			}

			connect(block, SIGNAL(cefQueryRequest(const QCefQuery&)),
				outPopup, SLOT(qslotDataReceivedFromBrowser(const QCefQuery&)));

			connect(outPopup, &AFQBorderPopupBaseWidget::qsignalDataFromBrowser, this, &AFQBlockManager::qslotCefQueryReceived);

			if (isRefreshButton) {
				dockTitle->ShowRefreshButton();
				connect(dockTitle, &AFDockTitle::qsignalRefreshButton, this, &AFQBlockManager::qslotDockTitleRefreshClicked);
			}

			outPopup->AddCefWidget(cefCheck);
			cef = true;
		}
	}

	if(!cef)
		outPopup->AddWidget(block);

	ApplyMoveInAllArea(outPopup);

	//Check Right First -> Left -> Mid
	if (showRight)
	{
		int posX = MAINFRAME->pos().x() + MAINFRAME->width();
		int posY = MAINFRAME->pos().y();

		AFQBaseDockWidget* outDock = nullptr;

		QRect dockPopupPosition = QRect(posX, posY, outPopup->width(), outPopup->height());
		QRect adjustRect;

		if (GetDock(type, outDock) && outDock->isFloating())
			adjustRect = outDock->geometry();
		else
			AdjustPositionOutSideFullScreen(dockPopupPosition, adjustRect, true);

		outPopup->setGeometry(adjustRect);
	}
	else if (showLeft)
	{
		int posX = MAINFRAME->pos().x() - outPopup->width();
		int posY = MAINFRAME->pos().y();

		AFQBaseDockWidget* outDock = nullptr;

		QRect dockPopupPosition = QRect(posX, posY, outPopup->width(), outPopup->height());
		QRect adjustRect;

		if (GetDock(type, outDock) && outDock->isFloating())
			adjustRect = outDock->geometry();
		else
			AdjustPositionOutSideFullScreen(dockPopupPosition, adjustRect, true);

		outPopup->setGeometry(adjustRect);
	}
	else if(!showRight && !showLeft)
	{
		QRect adjustRect;
		QRect dockPopupPosition = GetMidGeometry(outPopup->size());
		AdjustPositionOutSideFullScreen(dockPopupPosition, adjustRect, true);
		outPopup->setGeometry(adjustRect);
	}

	if (type == ENUM_WINDOW_TYPE::SoopChat ||
		type == ENUM_WINDOW_TYPE::TwitchChat ||
		type == ENUM_WINDOW_TYPE::YoutubeChat)
	{
		if (!MAINFRAME->isMaximized())
		{
			outPopup->SetIsMagnetPopup(true);
			outPopup->RefreshMagnetOffset(outPopup->pos());
			outPopup->MoveMagnet(outPopup->pos());
		}
	}

	block->show();

	if (showPopup)
	{
		if (type == ENUM_WINDOW_TYPE::SoopChat ||
			type == ENUM_WINDOW_TYPE::TwitchChat ||
			type == ENUM_WINDOW_TYPE::YoutubeChat)
			outPopup->setGeometry(outPopup->x(), outPopup->y(), outPopup->width(), MAINFRAME->frameGeometry().height());
		outPopup->show();
		outPopup->activateWindow();
		outPopup->raise();
	}
	else
	{
		outPopup->hide();
	}

	QApplication::processEvents();
	
	m_popupWidgets.insert(type, outPopup);
	_ChangeBlockUse(true, type);
	
	MAINFRAME->OnSoopEvent(SOOP_FRONTEND_EVENT_STATE_LNBMENU_ON, &type);

	return true;
}

bool AFQBlockManager::_CreateCurrentSceneProjector(AFQProjector*& projector, QString& sceneName, int monitor)
{
	OBSSource source = OBSSource(obs_scene_get_source(SCENE_CONTEXT.GetCurrentScene()));
	sceneName = obs_source_get_name(source);

	projector = new AFQProjector(nullptr, source, monitor, ProjectorType::Scene);
	if (projector)
		return true;

	return false;
}

bool AFQBlockManager::_BasicSetTitleWidget(ENUM_WINDOW_TYPE type, AFDockTitle*& outTitle, bool popupmode, bool fixed)
{
	AFQBlockManager::BlockBaseInfo baseInfo;
	if (_GetBlockBaseInfo(type, baseInfo))
	{

		outTitle->Initialize(baseInfo.transformable, Str(baseInfo.windowTitle), type,
			baseInfo.needQuestionMark,
			baseInfo.questionMarkToolTip);
		
		AFQBorderPopupBaseWidget* w = qobject_cast<AFQBorderPopupBaseWidget*>(outTitle->parent());

		connect(outTitle, &AFDockTitle::qsignalToggleDock,
			this, &AFQBlockManager::qslotSwitchBlockWindowType);

		if (type == ENUM_WINDOW_TYPE::SceneSource)
		{
			connect(outTitle, &AFDockTitle::qsignalTransitionScenePopup,
				MAINFRAME, &AFMainFrame::qslotTransitionSceneTriggered);
		}
		else if (type == ENUM_WINDOW_TYPE::AudioMixer) 
		{
			connect(outTitle, &AFDockTitle::qsignalAdvAudioMixerPopup,
				MAINFRAME, &AFMainFrame::qslotAdvAudioPropertiesTriggered);
		}

		outTitle->SetToggleWindowToDockButton(popupmode);
		if (popupmode)
			outTitle->MinMaxButton(fixed);

		return true;
	}
	return false;
}
void AFQBlockManager::_ChangeBlockUse(bool used, ENUM_WINDOW_TYPE checkType)
{
	bool enableFavoriteMenu = true;

	QString buttonType = "";
	switch (checkType)
	{
	case ENUM_WINDOW_TYPE::SoopChat:
		buttonType = SERVICE_CHAT;
		break;


	case ENUM_WINDOW_TYPE::TwitchChat:
	case ENUM_WINDOW_TYPE::YoutubeChat:
		buttonType = SERVICE_CHAT;
		enableFavoriteMenu = false;
		break;

	case ENUM_WINDOW_TYPE::BroadInfo:
		buttonType = SERVICE_BROADINFO;
		enableFavoriteMenu = false;
		break;
	case ENUM_WINDOW_TYPE::SoopOverlay:
		buttonType = SERVICE_OVERLAY;
		break;
	case ENUM_WINDOW_TYPE::SubTitle:
		buttonType = SERVICE_SUBTITLE;
		break;
	case ENUM_WINDOW_TYPE::Mission:
		buttonType = SERVICE_MISSION;
		break;
	case ENUM_WINDOW_TYPE::Vote:
		buttonType = SERVICE_VOTE;
		break;
	case ENUM_WINDOW_TYPE::AquaControl:
		buttonType = SERVICE_AQUA_CONTROL;
		break;
	case ENUM_WINDOW_TYPE::Extensions:
		buttonType = SERVICE_EXTENSIONS;
		break;
	case ENUM_WINDOW_TYPE::Breaktime:
		buttonType = SERVICE_BREAKTIME;
		break;
	}

	emit qsignalBlockVisible(used, checkType, enableFavoriteMenu);

	if (enableFavoriteMenu) {
		QString menuId;
		auto& items = MAINFRAME->GetLnbMenuItems();
		for (auto& item : items) {
			if (0 == item.menuLog.compare(buttonType)) {
				item.active = used;
				menuId = item.menuId;
				break;
			}
		}
		MAINFRAME->UpdateFavoirteLnbMenus(menuId);
	}
}

void AFQBlockManager::_ToPopup(QString uuid)
{
	if (m_browserDocks.contains(uuid))
	{
		if (m_browserWidgetMap.contains(uuid))
		{
			m_browserWidgetMap.value(uuid)->raise();
			return;
		}

		AFQBaseDockWidget* foundDock = m_browserDocks.value(uuid);
		
		AFDockTitle* dockTitle = qobject_cast<AFDockTitle*>(foundDock->titleBarWidget());
		QRect browserGeo = foundDock->geometry();

		AFQBorderPopupBaseWidget* customWidget =
			new AFQBorderPopupBaseWidget(uuid, CustomTypes::CustomBrowser);

		customWidget->setMinimumSize(QSize(AUDIO_MIXER_MIN_SIZE_WIDTH, DOCK_MIN_SIZE_HEIGHT));

		dockTitle->SetToggleWindowToDockButton(true);
		customWidget->AddWidget(dockTitle);
		
		if (foundDock->IsCefDock())
		{
			QCefWidget* content = foundDock->GetCefWidget();
			customWidget->AddCefWidget(content);
		}
		else
		{
			QWidget* content = foundDock->widget();
			customWidget->AddWidget(content);
		}

		if (foundDock->isFloating())
		{
			customWidget->setGeometry(browserGeo);
		}
		else
		{
			browserGeo = QRect(MAINFRAME->pos().x() - CUSTOM_BROWSER_MIN_WIDTH,
				MAINFRAME->pos().y(),
				CUSTOM_BROWSER_MIN_WIDTH,
				CUSTOM_BROWSER_MIN_HEIGHT);

			QRect adjustRect;
			AdjustPositionOutSideFullScreen(browserGeo, adjustRect);
			customWidget->setGeometry(adjustRect);
		}
		customWidget->setWindowTitle(dockTitle->GetLabelText());
		customWidget->show();

		m_browserWidgetMap.insert(uuid, customWidget);

		foundDock->SetCloseCef(false);
		foundDock->close();
		foundDock->deleteLater();
		m_browserDocks.remove(uuid);

		if (DYNAMIC_COMPOSIT)
			DYNAMIC_COMPOSIT->CheckDocksState();
		
	}
}

void AFQBlockManager::_ToDock(QString uuid)
{
	if (m_browserWidgetMap.contains(uuid))
	{
		if (m_browserDocks.contains(uuid))
			return;

		AFQBorderPopupBaseWidget* foundPopup = m_browserWidgetMap[uuid];
		QRect browserGeo = foundPopup->geometry();

		AFQBaseDockWidget* customWidget =
			new AFQBaseDockWidget(uuid, CustomTypes::CustomBrowser, DYNAMIC_COMPOSIT);

		AFDockTitle* dockTitle = foundPopup->findChild<AFDockTitle*>();
		if (!dockTitle)
		{
			CUSTOMBROWSERINFO foundInfo;
			bool found = false;
			foreach(CUSTOMBROWSERINFO info, m_customBrowserInfoVector)
			{
				if (info.Uuid == uuid)
				{
					foundInfo = info;
					customWidget->setObjectName(info.Name + "_extra");
					found = true;
				}
			}

			if (found)
			{
				dockTitle = new AFDockTitle(customWidget);
				dockTitle->InitializeCustom(foundInfo.Name, foundInfo.Uuid, false, true);

				connect(dockTitle, &AFDockTitle::qsignalToggleCustomDock,
					this, &AFQBlockManager::qslotSwitchCustomBrowser);
			}
			else
				return;
		}
		else
		{
			foreach(CUSTOMBROWSERINFO info, m_customBrowserInfoVector)
			{
				if (info.Uuid == uuid)
				{
					customWidget->setObjectName(info.Name + "_extra");
				}
			}
		}

		dockTitle->SetToggleWindowToDockButton(false);
		customWidget->setTitleBarWidget(dockTitle);

		if (foundPopup->IsCef())
		{
			QCefWidget* content = foundPopup->CefWidget();
			if (!content)
				return;
			customWidget->AddCefWidget(content);
		}
		else
		{
			QWidget* content = foundPopup->ContentWidget();
			if (!content)
				return;
			customWidget->setWidget(content);
		}

		customWidget->setAllowedAreas(Qt::DockWidgetArea::AllDockWidgetAreas);
		customWidget->setMinimumSize(QSize(AUDIO_MIXER_MIN_SIZE_WIDTH, DOCK_MIN_SIZE_HEIGHT));

		if (DYNAMIC_COMPOSIT)
		{
			DYNAMIC_COMPOSIT->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, customWidget);
			DYNAMIC_COMPOSIT->CheckDocksState();
		}

		customWidget->show();

		m_browserDocks.insert(uuid, customWidget);

		foundPopup->close();
		foundPopup->deleteLater();
		m_browserWidgetMap.remove(uuid);
	}
}

void AFQBlockManager::AdjustPositionOutSideFullScreen(QRect windowGeometry, QRect& outAdjustGeometry, bool checkMainScreen)
{
	QList<QScreen*> screens = QGuiApplication::screens();
	QRect retVal = windowGeometry;

	QScreen* targetScreen = nullptr;
	int maxIntersectionArea = 0;

	if (!checkMainScreen)
	{
		for (QScreen* screen : screens) {
			QRect screenGeometry = screen->geometry();
			QRect intersected = windowGeometry & screenGeometry;
			int area = intersected.width() * intersected.height();
			if (area > maxIntersectionArea) {
				maxIntersectionArea = area;
				targetScreen = screen;
			}
		}
	}

	if (!targetScreen && MAINFRAME) {
		QWindow* parentWindow = MAINFRAME->windowHandle();
		if (parentWindow) {
			targetScreen = parentWindow->screen();
		}
	}

	if (!targetScreen) {
		QPoint mousePos = QCursor::pos();
		targetScreen = QGuiApplication::screenAt(mousePos);
	}

	if (!targetScreen) {
		outAdjustGeometry = retVal;
		return;
	}

	QRect screenRect = targetScreen->geometry();
	int orgWidth = windowGeometry.width();
	int orgHeight = windowGeometry.height();

	if (retVal.left() < screenRect.left()) {
		retVal.moveLeft(screenRect.left());
	}
	else if (retVal.right() > screenRect.right()) {
		retVal.moveRight(screenRect.right());
		retVal.moveLeft(retVal.right() - orgWidth);
	}

	if (retVal.top() < screenRect.top()) {
		retVal.moveTop(screenRect.top());
	}
	else if (retVal.bottom() > screenRect.bottom()) {
		retVal.moveBottom(screenRect.bottom());
		retVal.moveTop(retVal.bottom() - orgHeight);
	}

	outAdjustGeometry = retVal;
}

void AFQBlockManager::ReversePositionOutSideFullScreen(QRect parentGeometry, QRect windowGeometry, QRect& outAdjustGeometry)
{
	QList<QScreen*> screens = QGuiApplication::screens();
	QRect retVal = windowGeometry;

	QRect fullScreenRect;
	for (QScreen* screen : screens) {
		fullScreenRect = fullScreenRect.united(screen->geometry());
	}

	uint32_t orgWidth = windowGeometry.width();
	uint32_t orgHeight = windowGeometry.height();

	if (windowGeometry.right() >= fullScreenRect.right())
	{
		retVal.setRight(parentGeometry.x());
		retVal.setLeft(parentGeometry.x() - orgWidth);
	}

	if (windowGeometry.top() <= fullScreenRect.top())
	{
		retVal.setTop(fullScreenRect.top());
		retVal.setBottom(orgHeight);
	}
	else if (windowGeometry.bottom() >= fullScreenRect.bottom())
	{
		retVal.setBottom(fullScreenRect.bottom());
		retVal.setTop(fullScreenRect.bottom() - orgHeight);
	}

	outAdjustGeometry = retVal;
}

//TopBaseWindow
void AFQBlockManager::ApplyMoveInAllArea(QObject* applyWidget)
{
	applyWidget->setProperty("MoveInAllArea", true);
	QObjectList objects = applyWidget->children();

	//qDebug() << applyWidget->objectName();
	foreach(QObject * object, objects)
	{
		//qDebug() << "object: " << object->objectName() << "class: " << object->metaObject()->className();

		if (QWidget* widget = qobject_cast<QWidget*>(object)) {
			if (App()->CheckClickableWidget(object, true))
			{
				//qDebug() << "Skip: " << object->objectName();
			}
			else
			{
				//qDebug() << "setProperty: " << object->objectName();
				object->setProperty("MoveInAllArea", true);
				ApplyMoveInAllArea(object);
			}
		}
	}
}

QRect AFQBlockManager::GetMidGeometry(QSize popupSize)
{
	int posX = MAINFRAME->pos().x() + (MAINFRAME->width() / 2) - (popupSize.width() / 2);
	int posY = MAINFRAME->pos().y() + (MAINFRAME->height() / 2) - (popupSize.height() / 2);

	QRect midPosition = QRect(posX, posY, popupSize.width(), popupSize.height());

	return midPosition;
}

void AFQBlockManager::CreateSoopPage(bool force, bool reCreate)
{
	//Soop Page No Reset : Logout->Restart Program
	if (reCreate)
		_DeleteSoopPage();

	_CreateSoopChat(force);
}

void AFQBlockManager::CreateTwitchPage(bool reset)
{
	/*if (reset)
		_DeleteTwitchPage();*/

	_CreateTwitchChat();
}

void AFQBlockManager::ShowVodSplit(QWidget* parent, QString prevTitle)
{
	if (!MAINFRAME->CheckSplitVodAvailable())
		return;

	if (!prevTitle.isEmpty())
	{
		bool showTitleChange = config_get_bool(USERCONFIG, "BroadInfo", "NotifyOnTitleChange");
		if (!showTitleChange)
			return;
	}
	
	AFQInstantVodSaverWidget* VodSaverWidget = new AFQInstantVodSaverWidget(parent, prevTitle);
	VodSaverWidget->exec();

	if (VodSaverWidget->VodSaveSuccess())
		MAINFRAME->SplitVodSaved();
	

	VodSaverWidget->deleteLater();
}

void AFQBlockManager::ShowVodAutoUploadNotice()
{
	AFQVodAutoUploadNoticeDialog* notice = new AFQVodAutoUploadNoticeDialog(MAINFRAME);
	notice->exec();
}

bool AFQBlockManager::ShowBroadcastNotice()
{
	AFQSoopBroadcastNoticeDialog* notice = new AFQSoopBroadcastNoticeDialog(MAINFRAME);
	return notice->exec();
}

void AFQBlockManager::SendBroadState(bool broadstart)
{
	if (!broadstart)
		return;

	QWidget* chatwidget = nullptr;
	bool isfind = FindBlock(ENUM_WINDOW_TYPE::SoopChat, chatwidget);
	if (isfind) {
		std::string strBroadStartScript = createBroadStartChatCefScript();

		QCefWidget* cefWidget = reinterpret_cast<QCefWidget*>(chatwidget);
		cefWidget->executeJavaScript(strBroadStartScript);
	}
}

void AFQBlockManager::SendScriptToChatBrowser(QString& script)
{
	QWidget* chatwidget = nullptr;
	bool isfind = FindBlock(ENUM_WINDOW_TYPE::SoopChat, chatwidget);
	if (isfind)
	{
		if (chatwidget)
		{
			QCefWidget* cefWidget = reinterpret_cast<QCefWidget*>(chatwidget);
			std::string requestScript = createRequestCefScript("main", "script", script.toStdString());
			cefWidget->executeJavaScript(requestScript);
		}
	}
}

void AFQBlockManager::FinishEditTitle()
{
	QWidget* broadwidget = nullptr;
	bool isfind = FindBlock(ENUM_WINDOW_TYPE::BroadInfo, broadwidget);
	if (isfind)
	{
		if (broadwidget)
		{
			AFBroadInfoDockWidget* broadinfoWidget = reinterpret_cast<AFBroadInfoDockWidget*>(broadwidget);
			broadinfoWidget->qslotFinishEditingBroadTitle();
		}
	}
}

void AFQBlockManager::DoubleCheckPosition(ENUM_WINDOW_TYPE type)
{
	AFQBorderPopupBaseWidget* popup = nullptr;

	QMetaEnum BlockTypeEnum = QMetaEnum::fromType<ENUM_WINDOW_TYPE>();
	bool isfind = GetPopup(type, popup);
	if (isfind)
	{
		const char* key = BlockTypeEnum.valueToKey(type);

		QPoint movePoint = popup->pos();

		//Check Position
		QString typeCheck = QString("%1CheckRect").arg(key);
		QString strPoint = QString(config_get_string(USERCONFIG, "BasicWindow", typeCheck.toUtf8().constData()));

		QStringList parts = strPoint.split('.');

		//double check rect
		QPoint checkPoint;
		if (parts.size() == 4) {
			checkPoint.setX(parts[0].toInt());
			checkPoint.setY(parts[1].toInt());
		}

		if (movePoint.y() != checkPoint.y())
		{
			movePoint.setY(checkPoint.y());
			popup->move(movePoint);
		}

		QString typeMax = QString("%1Max").arg(key);
	    bool isMax = config_get_bool(USERCONFIG, "BasicWindow", typeMax.toUtf8().constData());
		if (isMax)
			popup->showMaximized();
	}
}

void AFQBlockManager::RestoreMagnetPopup()
{
	AFQBorderPopupBaseWidget* popup = nullptr;

	QMetaEnum BlockTypeEnum = QMetaEnum::fromType<ENUM_WINDOW_TYPE>();
	bool isfind = GetPopup(ENUM_WINDOW_TYPE::SoopChat, popup);
	if (isfind)
	{
		popup->SetIsMagnetPopup(false);

		const char* key = BlockTypeEnum.valueToKey(ENUM_WINDOW_TYPE::SoopChat);

		const char* PopupPosition = config_get_string(USERCONFIG, "BasicWindow", key);

		if ((PopupPosition != nullptr) && (PopupPosition[0] != '\0'))
		{
			QByteArray posbyteArray = QByteArray::fromBase64(QByteArray(PopupPosition));
			popup->restoreGeometry(posbyteArray);
		}
	}

	isfind = GetPopup(ENUM_WINDOW_TYPE::TwitchChat, popup);
	if (isfind)
	{
		popup->SetIsMagnetPopup(false);

		const char* key = BlockTypeEnum.valueToKey(ENUM_WINDOW_TYPE::TwitchChat);

		const char* PopupPosition = config_get_string(USERCONFIG, "BasicWindow", key);

		if ((PopupPosition != nullptr) && (PopupPosition[0] != '\0'))
		{
			QByteArray posbyteArray = QByteArray::fromBase64(QByteArray(PopupPosition));
			popup->restoreGeometry(posbyteArray);
		}
	}

	isfind = GetPopup(ENUM_WINDOW_TYPE::YoutubeChat, popup);
	if (isfind)
	{
		popup->SetIsMagnetPopup(false);

		const char* key = BlockTypeEnum.valueToKey(ENUM_WINDOW_TYPE::YoutubeChat);

		const char* PopupPosition = config_get_string(USERCONFIG, "BasicWindow", key);

		if ((PopupPosition != nullptr) && (PopupPosition[0] != '\0'))
		{
			QByteArray posbyteArray = QByteArray::fromBase64(QByteArray(PopupPosition));
			popup->restoreGeometry(posbyteArray);
		}
	}
}

void AFQBlockManager::ResetMagnet(ENUM_WINDOW_TYPE type)
{
	AFQBorderPopupBaseWidget* popup = nullptr;
	bool isfind = GetPopup(type, popup);
	if (isfind)
	{
		if (!MAINFRAME->isMaximized())
		{
			popup->SetIsMagnetPopup(true);
			popup->RefreshMagnetOffset(popup->pos());
			popup->MoveMagnet(popup->pos());
		}
	}
}

void AFQBlockManager::ChangeAllMagnet(bool magnetOn)
{
	foreach(AFTTopBaseWidget * popup, m_popupWidgets)
	{
		auto borderPopup = qobject_cast<AFQBorderPopupBaseWidget*>(popup);
		if (borderPopup)
		{
			borderPopup->SetIsMagnetPopup(magnetOn);
			borderPopup->raise();
		}
	}
}

QSize AFQBlockManager::CheckImageSize(QString path)
{
	QSize retval = QSize(0, 0);
	QImageReader reader(path);

	if (reader.format().isEmpty())
		return retval;
	
	if (!reader.canRead())
		return retval;

	retval = reader.size();
	return retval;
}

void AFQBlockManager::CheckDockState()
{
	QMap<ENUM_WINDOW_TYPE, AFQBaseDockWidget*>::iterator docks;
	for (docks = m_dockWidgets.begin(); docks != m_dockWidgets.end(); ++docks) {
		QRect adjustRect;
		if (docks.value()->isVisible())
			_ChangeBlockUse(true, docks.key());

		if (docks.key() == ENUM_WINDOW_TYPE::BroadInfo)
			FinishEditTitle();
	}
}

void AFQBlockManager::CheckPopupState()
{
	QMap<ENUM_WINDOW_TYPE, AFTTopBaseWidget*>::iterator popups;
	for (popups = m_popupWidgets.begin(); popups != m_popupWidgets.end(); ++popups) {
		QRect adjustRect;
		AdjustPositionOutSideFullScreen(popups.value()->geometry(), adjustRect);
		popups.value()->setGeometry(adjustRect);
		popups.value()->show();
		_ChangeBlockUse(true, popups.key());

		if (popups.key() == ENUM_WINDOW_TYPE::BroadInfo)
			FinishEditTitle();
		
	}
}

void AFQBlockManager::qslotCreateYoutubePage(QString chat_id, std::string api_chat_id)
{
	_CreateYoutubeChat(chat_id, api_chat_id);
}

void AFQBlockManager::qslotShowAllOpenedBlocksOnExecute()
{
	disconnect(MAINFRAME, &AFMainFrame::qsignalMainShowEventTriggered,
		this, &AFQBlockManager::qslotShowAllOpenedBlocksOnExecute);

	if (m_dockWidgets.count() == 0 && m_popupWidgets.count() == 0)
		return;

	QTimer::singleShot(100, [this]() {
		CheckPopupState();

		foreach(AFQBaseDockWidget * dock, m_browserDocks)
			dock->show();
		

		foreach(AFQBorderPopupBaseWidget * popup, m_browserWidgetMap)
		{
			popup->show();
			popup->raise();
			popup->activateWindow();
		}

		DYNAMIC_COMPOSIT->CheckDocksState();
		
		//Event Banner
		EventTime eventStart = { 2026, 4, 30, 0, 0, 0 }; // 2026-04-30 00:00:00 ~
		EventTime eventEnd = { 2026, 6, 15, 23, 59, 59 }; // ~ 2026-06-26 23:59:59

		if (MAINFRAME->IsInEventPeriod(eventStart, eventEnd))
		{
			AFQBorderPopupBaseWidget* popup = nullptr;
			MakePopup(ENUM_WINDOW_TYPE::EventBannerImage, popup);
		}
		//Event Banner
		});
}

void AFQBlockManager::_ToPopup(ENUM_WINDOW_TYPE type)
{
	AFQBlockManager::BlockBaseInfo baseInfo;
	bool registeredInBlock = _GetBlockBaseInfo(type, baseInfo);
	if (!registeredInBlock || !baseInfo.transformable)
		return;

	QPoint dockPos = QPoint(0, 0);
	QSize dockSize = QSize(0, 0);
	AFQBaseDockWidget* outDock = nullptr;
	if (GetDock(type, outDock))
	{
		if (outDock && outDock->isFloating())
		{
			dockPos = outDock->pos();
			dockSize = outDock->size();
		}
	}
	_CloseDock(type, false);

	AFQBorderPopupBaseWidget* popup = nullptr;
	MakePopup(type, popup, true);

	if (dockSize.width() != 0 && dockSize.height() != 0)
	{
		popup->move(dockPos);
		popup->resize(dockSize);
	}

	if (DYNAMIC_COMPOSIT)
		DYNAMIC_COMPOSIT->CheckDocksState();
}

void AFQBlockManager::_ToDock(ENUM_WINDOW_TYPE type)
{
	AFQBlockManager::BlockBaseInfo baseInfo;
	bool registeredInBlock = _GetBlockBaseInfo(type, baseInfo);
	if (!registeredInBlock || !baseInfo.transformable)
		return;

	AFQBaseDockWidget* dockWidget = new AFQBaseDockWidget(type, DYNAMIC_COMPOSIT);

	AFQBorderPopupBaseWidget* popup = nullptr;

	if (GetPopup(type, popup))
	{
		QWidget* block;
		if (FindBlock(type, block))
		{
			QList<AFDockTitle*> Titles = popup->findChildren<AFDockTitle*>();
			AFDockTitle* dockTitle = nullptr;

			bool makeDocktitle = false;
			if (Titles.count() == 0)
			{
				makeDocktitle = true;
			}
			else
			{
				dockTitle = Titles[0];
				if (!dockTitle)
					makeDocktitle = true;
			}

			if (makeDocktitle)
			{
				dockTitle = new AFDockTitle(nullptr);
				_BasicSetTitleWidget(type, dockTitle, false);
				PolishStyleSheet(dockTitle);
			}

			dockTitle->SetToggleWindowToDockButton(false);

			dockTitle->setParent(dockWidget);
			block->setParent(dockWidget);

			AFQBlockManager::BlockBaseInfo baseInfo;
			bool registeredInBlock = _GetBlockBaseInfo(type, baseInfo);
			if (registeredInBlock)
			{
				QSize miniSize = QSize(baseInfo.minSize.width(), DOCK_MIN_SIZE_HEIGHT);
				dockWidget->setMinimumSize(miniSize);
				dockWidget->setMaximumSize(baseInfo.maxSize);

				_ClosePopup(type, true);

				dockWidget->setTitleBarWidget(dockTitle);

				if (baseInfo.isCef)
				{
					if (auto cef = reinterpret_cast<QCefWidget*>(block))
					{
						dockWidget->AddCefWidget(cef);

						connect(cef, SIGNAL(cefCreateAfter()),
							dockWidget, SLOT(qslotReceivedCreateAfterCefBrowser()));

						connect(cef, SIGNAL(cefLoadEnd()),
							dockWidget, SLOT(qslotReceivedLoadEndCefBrowser()));

						connect(dockWidget, &AFQBaseDockWidget::qsignalCreateAfterCefBrowser,
							this, &AFQBlockManager::qslotCreateAfterCefBrowser);

						connect(dockWidget, &AFQBaseDockWidget::qsignalLoadEndCefBrowser,
							this, &AFQBlockManager::qslotLoadEndCefBrowser);

					}
					else
						dockWidget->setWidget(block);
				}
				else
				{
					dockWidget->setWidget(block);
				}

				if (type == ENUM_WINDOW_TYPE::SoopChat)
				{
					connect(block, SIGNAL(cefQueryRequest(const QCefQuery&)),
						dockWidget, SLOT(qslotDataReceivedFromBrowserToDock(const QCefQuery&)));

					connect(dockWidget, &AFQBaseDockWidget::qsignalDataFromBrowser,
						this, &AFQBlockManager::qslotCefQueryReceived);
				}

				QMetaEnum BlockTypeEnum = QMetaEnum::fromType<WindowTypes>();
				const char* key = BlockTypeEnum.valueToKey(type);
				QString keystring = QString(key);
				dockWidget->setObjectName(keystring);

				dockWidget->setWindowTitle(dockTitle->GetLabelText());

				dockWidget->SetStyle();
				dockWidget->setAllowedAreas(Qt::DockWidgetArea::AllDockWidgetAreas);

				if(DYNAMIC_COMPOSIT)
					if(type == ENUM_WINDOW_TYPE::SoopChat ||
					   type == ENUM_WINDOW_TYPE::TwitchChat ||
					   type == ENUM_WINDOW_TYPE::YoutubeChat)
						DYNAMIC_COMPOSIT->addDockWidget(Qt::RightDockWidgetArea, dockWidget);
					else
						DYNAMIC_COMPOSIT->addDockWidget(Qt::BottomDockWidgetArea, dockWidget);
				dockWidget->setFloating(false);

				m_dockWidgets.insert(type, dockWidget);

				if (DYNAMIC_COMPOSIT)
					DYNAMIC_COMPOSIT->CheckDocksState();
			}
		}
	}
}

bool AFQBlockManager::_ResetBlockToDock(ENUM_WINDOW_TYPE type, AFQBaseDockWidget*& outDock)
{
	QWidget* outBlock = nullptr;
	if (!FindBlock(type, outBlock))
		return false;
	
	if (!GetDock(type, outDock))
	{
		AFQBorderPopupBaseWidget* sceneSourcePopup = nullptr;
		if (GetPopup(type, sceneSourcePopup))
		{
			_ToDock(type);
			GetDock(type, outDock);
		}
		else
		{
			MakeDock(type, outDock, true);
		}
	}

	return true;
}

bool AFQBlockManager::_CheckDockCanInsert(QSize dockSize)
{
	return false;
}

bool AFQBlockManager::_ClosePopup(ENUM_WINDOW_TYPE key, bool isToDock)
{
	AFQBorderPopupBaseWidget* outPopup = nullptr;
	if (GetPopup(key, outPopup))
	{
		m_popupWidgets.remove(key);
		if (outPopup)
		{
			outPopup->SetToDockValue(isToDock);
			outPopup->close();
			outPopup->deleteLater();
		}

		MAINFRAME->OnSoopEvent(SOOP_FRONTEND_EVENT_STATE_LNBMENU_OFF, &key);

		return true;
	}
	return false;
}

bool AFQBlockManager::_CloseDock(ENUM_WINDOW_TYPE key, bool closeCef)
{
	AFQBaseDockWidget* outDock = nullptr;
	if (GetDock(key, outDock))
	{
		disconnect(outDock, &AFQBaseDockWidget::qsignalCloseDock, this, &AFQBlockManager::qslotHideDock);
		m_dockWidgets.remove(key);
		outDock->SetCloseCef(closeCef);
		outDock->close();
		outDock->deleteLater();
		return true;
	}
	return false;
}

void AFQBlockManager::_DeleteBlock(ENUM_WINDOW_TYPE key)
{
	QWidget* checkChat = nullptr;
	if (FindBlock(key, checkChat))
	{
		if (checkChat)
		{
			checkChat->close();
			checkChat->deleteLater();
			m_blockMap.remove(key);
		}
		else
		{
			m_blockMap.remove(key);
		}
	}
}

bool AFQBlockManager::_CreateNeededBlock(ENUM_WINDOW_TYPE type, QWidget*& outblock)
{
	switch (type)
	{
	case ENUM_WINDOW_TYPE::AdvanceControls:
		_CreateAdvanceControlBlock();
		break;
	case ENUM_WINDOW_TYPE::SceneControl:
		_CreateSceneControl();
		break;
	case ENUM_WINDOW_TYPE::CustomBrowserCollection:
		_CreateCustomBrowserCollection();
		break;
	case ENUM_WINDOW_TYPE::StatPage:
		_CreateStat();
		break;
	case ENUM_WINDOW_TYPE::Mission:
	case ENUM_WINDOW_TYPE::Vote:
	case ENUM_WINDOW_TYPE::Extensions:
	case ENUM_WINDOW_TYPE::SAVVYReaction:
	case ENUM_WINDOW_TYPE::AquaControl:
		_CreateCefPopup(type);
		break;
	case ENUM_WINDOW_TYPE::SoopOverlay:
		_CreateOverlay();
		break;
	case ENUM_WINDOW_TYPE::Breaktime:
		_CreateBreakTime();
		break;
	case ENUM_WINDOW_TYPE::EventBannerImage:
		_CreateEventImage();
		break;
	}

	return FindBlock(type, outblock);
}

void AFQBlockManager::_CreateCustomBrowserBlock()
{
	for (AFQCustomBrowserCollection::CustomBrowserInfo info : m_customBrowserInfoVector)
	{
		if (info.isDock)
		{
			if (!info.isOpen)
				continue;

			OpenCustomBrowserDock(info, -1, false);
		}
		else
		{
			//Custom Dock previous version check
			if (info.width != 0 && info.height != 0)
			{
				OpenCustomBrowserDock(info, -1 , true);
			}
		}
	}
}

void AFQBlockManager::_SaveCustomBrowserBlock()
{
}

bool AFQBlockManager::_CreateCustomBrowserCollection()
{
	if (m_blockMap.contains(ENUM_WINDOW_TYPE::CustomBrowserCollection))
		return false;

	AFQCustomBrowserCollection* BrowserCollection = new AFQCustomBrowserCollection(nullptr);
	BrowserCollection->CustomBrowserCollectionInit(m_customBrowserInfoVector);
	connect(BrowserCollection, &AFQCustomBrowserCollection::qsignalCloseTriggered, this, &AFQBlockManager::qslotClosePopup);
	AddBlock(ENUM_WINDOW_TYPE::CustomBrowserCollection, BrowserCollection);

	return true;
}

bool AFQBlockManager::_CreateAdvanceControlBlock()
{
	if (m_blockMap.contains(ENUM_WINDOW_TYPE::AdvanceControls))
		return false;

	AFAdvanceControlsWidget* AdvanceControlsWidget = new AFAdvanceControlsWidget();

	auto activeConfig = ACTIVECONFIG;
	//
	bool replayBuf = false;
	const char* mode = config_get_string(activeConfig, "Output", "Mode");
	if (astrcmpi(mode, "Advanced") == 0) {
		const char* advRecType = config_get_string(activeConfig, "AdvOut", "RecType");
		if (astrcmpi(advRecType, "FFmpeg") == 0)
			replayBuf = false;
		else
			replayBuf = config_get_bool(activeConfig, "AdvOut", "RecRB");
	}
	else
		replayBuf = config_get_bool(activeConfig, "SimpleOutput", "RecRB");

	AdvanceControlsWidget->EnableReplayBuffer(replayBuf);
	AdvanceControlsWidget->SetReplayBufferStartStopStyle(AFOutputUtil::IsReplayBufferActive());
	AdvanceControlsWidget->setVisible(false);

	AddBlock(ENUM_WINDOW_TYPE::AdvanceControls, AdvanceControlsWidget);

	return true;
}

bool AFQBlockManager::_CreateSceneControl()
{
	if (m_blockMap.contains(ENUM_WINDOW_TYPE::SceneControl))
		return false;

	AFSceneControlWidget* SceneControlDock = new AFSceneControlWidget();

	SceneControlDock->setVisible(false);
	AddBlock(ENUM_WINDOW_TYPE::SceneControl, SceneControlDock);
	return true;
}

bool AFQBlockManager::_CreateStat()
{
	if (m_blockMap.contains(ENUM_WINDOW_TYPE::StatPage))
		return false;

	AFQStatWidget* StatFrame = new AFQStatWidget(nullptr);
	connect(StatFrame, &AFQStatWidget::qsignalCloseTriggered, this, &AFQBlockManager::qslotClosePopup);

	StatFrame->setVisible(false);
	AddBlock(ENUM_WINDOW_TYPE::StatPage, StatFrame);
	return true;
}

bool AFQBlockManager::_CreateOverlay()
{
	if (m_blockMap.contains(ENUM_WINDOW_TYPE::SoopOverlay))
		return false;

	AFQOverlayWidget* widget = new AFQOverlayWidget();
	connect(widget, &AFQOverlayWidget::qsignalCloseTriggered, this, &AFQBlockManager::qslotClosePopup);

	widget->setVisible(false);
	AddBlock(ENUM_WINDOW_TYPE::SoopOverlay, widget);
	return true;
}

bool AFQBlockManager::_CreateBreakTime(bool bSkip)
{
	if (!AFOutputUtil::IsStreamActive()){
		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", QTStr("breaktime.is.stream"), false, true, "", 0, 0, "type1");
		return false;
	}
	
	auto& breaktime = BREAKTIME_MANAGER;
	if (!bSkip)
	{
		if (breaktime.IsNextActive()) {
			const int remainNext = qMax(0, breaktime.GetNextRemainTime());
			const int min = remainNext / 60;
			const int sec = remainNext % 60;

			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,	"", QTStr("breaktime.next.wait").arg(min).arg(sec), false, true, "", 0, 0, "type1");
			return false;
		}
	}
	
	auto config = ACTIVECONFIG;
	//
	
	const char* sceneC = config_get_string(config, "BreakTime", "BreakTime.Scene");
	const QString sceneName = (sceneC && *sceneC) ? QString::fromUtf8(sceneC) : QString();

	config_set_default_int(config, "BreakTime", "BreakTime.Time", 3);
	int GetSelectedMinutes = config_get_int(config, "BreakTime", "BreakTime.Time");
	config_set_default_int(config, "BreakTime", "BreakTime.BGM", 1);
	int GetSelectedBgm = config_get_int(config, "BreakTime", "BreakTime.BGM");
	bool muted = breaktime.GetAudioMuted();
	int GetSliderVolume = breaktime.GetAudioVolume();
	int IsMessageChecked = config_get_int(config, "BreakTime", "BreakTime.MSG.Check");

	int minutes = (GetSelectedMinutes >= 1 && GetSelectedMinutes <= 10) ? GetSelectedMinutes : 3;
	int bgm = (GetSelectedBgm >= 0 && GetSelectedBgm <= 4) ? GetSelectedBgm : 1;
	int volume = (GetSliderVolume >= 0 && GetSliderVolume <= FADER_PRECISION) ? GetSliderVolume : FADER_PRECISION;

	bool msgChecked = (IsMessageChecked != 0);

	QString messageText;
	const char* msgC = config_get_string(config, "BreakTime", "BreakTime.MSG.Text");
	if (msgC && *msgC) {
		messageText = QString::fromUtf8(msgC);
	}
			
	AFBreaktime breaktimeDlg(MAINFRAME, breaktime.IsActive());
	if (breaktime.IsActive() == false) {
		breaktimeDlg.setFixedSize(QSize(560, 794));
	}

	if (breaktime.IsActive() == false)
	{
		breaktimeDlg.SetInitialState(
			minutes,
			bgm,
			msgChecked,
			muted,
			volume,
			sceneName,
			messageText
		);
	}
	else 
	{
		breaktimeDlg.SetInitialState_Playing(
			breaktime.GetRemainTime(),
			bgm,
			msgChecked,
			muted,
			volume,
			sceneName,
			messageText
		);
	}

	_ChangeBlockUse(true, ENUM_WINDOW_TYPE::Breaktime);

	setCenterPositionNotUseParent(&breaktimeDlg, MAINFRAME);

	if (breaktimeDlg.exec() == QDialog::Accepted)
	{
		_CreateBreakTime(true);
	}

	_ChangeBlockUse(false, ENUM_WINDOW_TYPE::Breaktime);

	return true;
}

bool AFQBlockManager::_CreateEventImage()
{
	//bool retval = false;
	//EventTime eventStart = { 2026, 3, 13, 9, 0, 0 };      // 2026-03-13 09:00:00
	//EventTime eventEnd = { 2026, 3, 15, 23, 59, 59 };    // 2026-03-15 23:59:59              

	//if (MAINFRAME->IsInEventPeriod(awardStart, awardEnd))
	//	retval = true;
	

	if (m_blockMap.contains(ENUM_WINDOW_TYPE::EventBannerImage))
	{
		QWidget* bannerWidget = m_blockMap.value(ENUM_WINDOW_TYPE::EventBannerImage);
		bannerWidget->close();
		delete bannerWidget;

		m_blockMap.remove(ENUM_WINDOW_TYPE::EventBannerImage);
	}

	QWidget* widget = new QWidget();
	widget->setObjectName("widget_Contents");

	QVBoxLayout* layout = new QVBoxLayout();

	//layout->setContentsMargins(99, 355, 100, 12);

	QPushButton* button = new QPushButton(widget);
	button->setObjectName("pushButton_EventNavigate");
	button->setFixedSize(458, 423);

	connect(button, &QPushButton::clicked, [=] {
		MAINFRAME->NavigateDefaultBrowser(EVENT_BANNER_NAVIGATE_URL);
		});

	layout->addWidget(button);

	widget->setLayout(layout);
	widget->setVisible(false);

	std::string fileName;

	GetDataFilePath(EVENT_BANNER_IMAGE, fileName);

	QString styleSheet = QString(
		"#widget_Contents {"
		"background-image: url('%1');"
		"background-repeat: no-repeat;"
		"background-position: center;"
		"}"
		"#pushButton_EventNavigate {"
		"background-color: transparent;"
		"border: none;"
		"}").arg(QString::fromUtf8(fileName));

	widget->setStyleSheet(styleSheet);
	AddBlock(ENUM_WINDOW_TYPE::EventBannerImage, widget);

	return true;
}

void AFQBlockManager::qslotBreaktimeShow()
{
	_CreateBreakTime();
}

//Block+Popup (bool) : Popup Style    -> Popup or Dock
//Block(const char*) : Popup Position -> no value no make
void AFQBlockManager::_LoadBlock()
{
	config_t* userConfig = USERCONFIG;
	for (int i = 0; i < ENUM_WINDOW_TYPE::BLOCKITER; i++)
	{
		QMetaEnum BlockTypeEnum = QMetaEnum::fromType<WindowTypes>();
		const char* key = BlockTypeEnum.valueToKey(i);

		std::string check = QString("%1Popup").arg(key).toStdString();

		const char* PopupPosition = config_get_string(userConfig, "BasicWindow", key);
		if ((PopupPosition == nullptr) || (PopupPosition[0] == '\0'))
			continue;

		QByteArray posbyteArray = QByteArray::fromBase64(QByteArray(PopupPosition));

		bool isDockPopup = config_get_bool(userConfig, "BasicWindow", check.c_str());

		ENUM_WINDOW_TYPE val = static_cast<ENUM_WINDOW_TYPE>(i);
		if (isDockPopup)
		{
			AFQBorderPopupBaseWidget* popup = nullptr;

			MakePopup(val, popup, false, false);
			if (popup)
			{
				popup->restoreGeometry(posbyteArray);
				QPoint movePoint = popup->pos();
				//Check Position

				std::string check2 = QString("%1CheckRect").arg(key).toStdString();
				QString strPoint = QString(config_get_string(userConfig, "BasicWindow", check2.c_str()));
				QStringList parts = strPoint.split('.');

				//double check rect
				QRect checkRect;
				if (parts.size() == 4) {
					checkRect.setX(parts[0].toInt());
					checkRect.setY(parts[1].toInt());
					checkRect.setWidth(parts[2].toInt());
					checkRect.setHeight(parts[3].toInt());
				}

				if (movePoint.y() != checkRect.y())
				{
					movePoint.setY(checkRect.y());
					popup->move(movePoint);
				}

				QRect adjustRect;
				AdjustPositionOutSideFullScreen(popup->geometry(), adjustRect);
				popup->setGeometry(adjustRect);
			}
		}
		else
		{
			std::string pos = config_get_string(USERCONFIG, "BasicWindow", key);
			if (!pos.empty())
			{
				bool showDock = i == ENUM_WINDOW_TYPE::SoopChat ? true : false;
				AFQBaseDockWidget* dock = nullptr;
				MakeDock(val, dock, showDock);
			}
		}
	}

	if (!AUTH_CONTEXT.IsSoopRegistered())
	{
		QWidget* broadInfo = nullptr;
		if (FindBlock(ENUM_WINDOW_TYPE::BroadInfo, broadInfo))
		{
			if (broadInfo)
			{
				broadInfo->setParent(nullptr);
				broadInfo->hide();
			}
		}
		_ClosePopup(ENUM_WINDOW_TYPE::BroadInfo);
		_CloseDock(ENUM_WINDOW_TYPE::BroadInfo, true);
	}
}

void AFQBlockManager::_SaveBlock()
{
	config_t* userConfig = USERCONFIG;
	config_set_string(userConfig, "BasicWindow", "DockState", DYNAMIC_COMPOSIT->saveState().toBase64().constData());
	for (int i = 0; i < ENUM_WINDOW_TYPE::BLOCKITER; i++)
	{
		QMetaEnum BlockTypeEnum = QMetaEnum::fromType<WindowTypes>();
		const char* key = BlockTypeEnum.valueToKey(i);

		ENUM_WINDOW_TYPE val = static_cast<ENUM_WINDOW_TYPE>(i);
		if (m_popupWidgets.contains(val))
		{
			if (m_popupWidgets.value(val))
			{
				std::string check = QString("%1Popup").arg(key).toStdString();
				std::string check2 = QString("%1CheckRect").arg(key).toStdString();
				std::string maxCheck = QString("%1Max").arg(key).toStdString();

				config_set_bool(userConfig, "BasicWindow", check.c_str(), true);

				config_set_string(userConfig, "BasicWindow", key, m_popupWidgets.value(val)->saveGeometry().toBase64().constData());

				std::string strPos = std::to_string(m_popupWidgets.value(val)->x()) + "." + std::to_string(m_popupWidgets.value(val)->y())
					+ "." + std::to_string(m_popupWidgets.value(val)->width()) + "." + std::to_string(m_popupWidgets.value(val)->height());
				bool maxCheckBool = false;

				if (m_popupWidgets.value(val)->isMaximized())
				{
					QRect normalRect = m_popupWidgets.value(val)->normalGeometry();
					strPos = std::to_string(normalRect.x()) + "." + std::to_string(normalRect.y())
						+ "." + std::to_string(normalRect.width()) + "." + std::to_string(normalRect.height());
					maxCheckBool = true;
				}

				config_set_string(userConfig, "BasicWindow", check2.c_str(), strPos.c_str());
				config_set_bool(userConfig, "BasicWindow", maxCheck.c_str(), maxCheckBool);
			}
			continue;
		}

		if (m_dockWidgets.contains(val))
		{
			if (m_dockWidgets.value(val))
			{
				std::string check = QString("%1Popup").arg(key).toStdString();

				config_set_bool(userConfig, "BasicWindow", check.c_str(), false);

				disconnect(m_dockWidgets.value(val), &AFQBaseDockWidget::qsignalCloseDock, this, &AFQBlockManager::qslotHideDock);

				if (m_dockWidgets.value(val)->isVisible()) {
					config_set_string(userConfig, "BasicWindow", key,
									  m_dockWidgets.value(val)->saveGeometry().toBase64().constData());
				} else {
					config_set_string(userConfig, "BasicWindow", key, "");
				}
			}
			continue;
		}

		std::string check = QString("%1Popup").arg(key).toStdString();

		config_set_bool(userConfig, "BasicWindow", check.c_str(), false);
		config_set_string(userConfig, "BasicWindow", key, "");
	}

	config_save_safe(userConfig, "tmp", nullptr);
}

template <typename Container>
void ClearWidgetContainer(Container& container)
{
	foreach (auto* widget, container) {
		if (widget) {
			widget->close();
			widget->deleteLater();
		}
	}
	container.clear();
}

void AFQBlockManager::_ClearAllBlocks()
{
	ClearWidgetContainer(m_blockMap);
	ClearWidgetContainer(m_popupWidgets);
	ClearWidgetContainer(m_dockWidgets);
}

void AFQBlockManager::_CloseAllPopups(QList<ENUM_WINDOW_TYPE> exclusions)
{
	QList<AFTTopBaseWidget*> list;
	for (auto it = m_popupWidgets.begin(); it != m_popupWidgets.end();)
	{
		if (exclusions.contains(it.key()))
		{
			++it;
			continue;
		}

		list.append(it.value());
		it = m_popupWidgets.erase(it);
	}

	foreach(AFTTopBaseWidget * widget, list)
	{
		widget->close();
		widget->deleteLater();
	}
}

void AFQBlockManager::_CloseAllDocks(QList<ENUM_WINDOW_TYPE> exclusions)
{
	for (auto it = m_dockWidgets.begin(); it != m_dockWidgets.end();)
	{
		qDebug() << it.key();
		if (exclusions.contains(it.key()))
		{
			++it;
			continue;
		}

		QWidget* outBlock = nullptr;
		if (FindBlock(it.key(), outBlock))
		{
			if (it.key() > ENUM_WINDOW_TYPE::BLOCKITER)
			{
				if (outBlock)
				{
					outBlock->close();
					outBlock->deleteLater();
				}
				m_blockMap.remove(it.key());
			}
			else
			{
				outBlock->setParent(nullptr);
				outBlock->hide();
			}

			_ChangeBlockUse(false, it.key());
		}

		disconnect(it.value(), &AFQBaseDockWidget::qsignalCloseDock, this, &AFQBlockManager::qslotHideDock);

		it.value()->close();
		delete it.value();
		it = m_dockWidgets.erase(it);
	}

}

void AFQBlockManager::_LoadCustomBrowserList()
{
	m_customBrowserInfoVector.clear();
}

bool AFQBlockManager::_CreateSceneSourceBlock()
{
	if (m_blockMap.contains(ENUM_WINDOW_TYPE::SceneSource))
		return false;
	AFSceneSourceWidget* SceneSourceDock = new AFSceneSourceWidget();

	connect(SceneSourceDock, &AFSceneSourceWidget::qsignalSceneDoubleClickedTriggered,
			DYNAMIC_COMPOSIT, &AFMainDynamicComposit::ChangeSceneOnDoubleClick);

	QMetaEnum BlockTypeEnum = QMetaEnum::fromType<ENUM_WINDOW_TYPE>();
	const char* key = BlockTypeEnum.valueToKey(ENUM_WINDOW_TYPE::SceneSource);
	SceneSourceDock->setObjectName(key);

	connect(SceneSourceDock, &AFSceneSourceWidget::qsignalAddScene,
			MAIN_SCENESOURCE, &CMainSceneSource::qslotAddSceneTriggered);

	connect(SceneSourceDock, &AFSceneSourceWidget::qsignalAddSource,
			MAIN_SCENESOURCE, &CMainSceneSource::qslotShowSelectSourcePopup);

	AddBlock(ENUM_WINDOW_TYPE::SceneSource, SceneSourceDock);
	return true;
}

bool AFQBlockManager::_CreateAudioMixer()
{
	if (m_blockMap.contains(ENUM_WINDOW_TYPE::AudioMixer))
		return false;
	AFAudioMixerWidget* AudioMixerDockWidget = new AFAudioMixerWidget();

	AddBlock(ENUM_WINDOW_TYPE::AudioMixer, AudioMixerDockWidget);

	return true;
}

bool AFQBlockManager::_CreateBroadInfo()
{
	if (m_blockMap.contains(ENUM_WINDOW_TYPE::BroadInfo))
		return false;

	AFBroadInfoDockWidget* BroadInfoWidget = new AFBroadInfoDockWidget();
	
	AddBlock(ENUM_WINDOW_TYPE::BroadInfo, BroadInfoWidget);
	
	return true;
}

bool AFQBlockManager::_CreateSoopChat(bool force)
{
	QWidget* checkChat = nullptr;
	FindBlock(ENUM_WINDOW_TYPE::SoopChat, checkChat);

	if (checkChat)
	{
		if (force)
		{
			QCefWidget* refreshWidget = static_cast<QCefWidget*>(checkChat);
			refreshWidget->reloadPage();
		}

		return true;
	}

	std::string strChatUrl;
	if (AUTH_CONTEXT.GetSoopChatUrl(strChatUrl))
	{
		QCefWidget* cefWidget = CEFMANAGER.createWidget(nullptr, strChatUrl);	
		cefWidget->setURL(strChatUrl);

		AddBlock(ENUM_WINDOW_TYPE::SoopChat, cefWidget);
	}
	return true;
}

bool AFQBlockManager::_CreateTwitchChat()
{
	QWidget* checkChat = nullptr;
	FindBlock(ENUM_WINDOW_TYPE::TwitchChat, checkChat);
	if (checkChat)
		return true;

	std::string strChatUrl;
	std::string moderation_tools_url;
	if (AUTH_CONTEXT.GetTwitchChatUrl(moderation_tools_url, strChatUrl))
	{
		if(!CEFMANAGER.GetCef())
			return false;
		//
		QCefWidget* cefWidget = CEFMANAGER.createWidget(nullptr, strChatUrl);
		CEFMANAGER.GetCef()->add_force_popup_url(moderation_tools_url, cefWidget);
		
		std::string script = "localStorage.setItem('twilight.theme', 1);";
		cefWidget->setStartupScript(script);

		AddBlock(ENUM_WINDOW_TYPE::TwitchChat, cefWidget);
		return true;
	}

	return false;
}

//Always Reset (No Reuse)
bool AFQBlockManager::_CreateYoutubeChat(const QString& chat_id, const std::string& api_chat_id)
{
	//QRect chatRect = QRect(0, 0, 0, 0);
	//AFQBorderPopupBaseWidget* outPopup = nullptr;
	//if (_GetPopup(ENUM_WINDOW_TYPE::YoutubeChat, outPopup))
	//{
	//	chatRect = outPopup->geometry();
	//}

	bool deletedChat = _DeleteYoutubePage();

	if (chat_id.isNull() || chat_id.isEmpty())
	{		
		QCefWidget* youtubeBlank = CEFMANAGER.createWidget(nullptr, YOUTUBE_BLANK_CHAT);
		AddBlock(ENUM_WINDOW_TYPE::YoutubeChat, youtubeBlank);
	}
	                   
	std::string strChatUrl;
	std::string stdChatId = chat_id.toStdString();
	if (AUTH_CONTEXT.GetYoutubeChatUrl(stdChatId, api_chat_id, strChatUrl))
	{
		QCefWidget* cefWidget = CEFMANAGER.createWidget(nullptr, strChatUrl);
		AddBlock(ENUM_WINDOW_TYPE::YoutubeChat, cefWidget);
	}

	if (deletedChat)
	{
		AFQBorderPopupBaseWidget* popup = nullptr;
		MakePopup(ENUM_WINDOW_TYPE::YoutubeChat, popup);
	}

	return true;
}

void AFQBlockManager::_DeleteSoopPage()
{
	_ClosePopup(ENUM_WINDOW_TYPE::Breaktime);
	_DeleteBlock(ENUM_WINDOW_TYPE::Breaktime);

	_ClosePopup(ENUM_WINDOW_TYPE::SoopOverlay);
	_DeleteBlock(ENUM_WINDOW_TYPE::SoopOverlay);

	_ClosePopup(ENUM_WINDOW_TYPE::SoopChat);
	_DeleteBlock(ENUM_WINDOW_TYPE::SoopChat);
}

void AFQBlockManager::_DeleteTwitchPage()
{
	_ClosePopup(ENUM_WINDOW_TYPE::TwitchChat);
	_DeleteBlock(ENUM_WINDOW_TYPE::TwitchChat);
}

bool AFQBlockManager::_DeleteYoutubePage()
{
	bool retVal = _ClosePopup(ENUM_WINDOW_TYPE::YoutubeChat);
	_DeleteBlock(ENUM_WINDOW_TYPE::YoutubeChat);

	return retVal;
}


void AFQBlockManager::_ProcessCefQuery(CefQueryMessage& queryMessage)
{
}

void AFQBlockManager::_cefWorkerLoop()
{
	while (m_cefQueryThreadRunning) {
		CefQueryMessage queryMessage;

		{
			std::unique_lock<std::mutex> lock(m_cefQueryMutex);
			m_cefQueryCV.wait(lock, [&]() { 
				return !m_cefQueryQueue.empty() || !m_cefQueryThreadRunning; 
				});

			if (!m_cefQueryThreadRunning) {
				break;
			}

			if (m_cefQueryQueue.empty()) {
				continue;
			}

			queryMessage = m_cefQueryQueue.front();
			m_cefQueryQueue.pop();
		}

		_ProcessCefQuery(queryMessage);
	}
}