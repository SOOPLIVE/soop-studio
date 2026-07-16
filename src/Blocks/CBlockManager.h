#pragma once

#include <QObject>
#include <QMetaEnum>
#include <QEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <mutex>
#include <queue>

#include "CoreModel/Browser/CCefManager.h"

#include "UIComponent/CBorderPopupBaseWidget.h"

#include "Blocks/CBaseDockWidget.h"

#include "PopupWindows/CustomBrowser/CCustomBrowserCollection.h"
#include "Blocks/SceneControlDock/CProjector.h"

#include "PopupWindows/CBreaktime.h"

#define ENUM_WINDOW_TYPE AFQBlockManager::WindowTypes

//foward class
class AFDockTitle;
class AFQCustomMenu;

//To Use BlockManager (Basic Popup, Title)
//1) _SetBlockBaseInfo
//2) Add Contents to m_BlockMap
//3) MakePopup
class AFQBlockManager final : public QObject
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT
public:
    //Add Type: _SetBlockBaseInfo need to be add

    explicit AFQBlockManager(QWidget* parent = nullptr);
    ~AFQBlockManager();

    enum ParentTypes
    {
        NoParent = 0,
        MainView,
        MainWindow
    }; Q_ENUM(ParentTypes)
        
    enum CustomTypes
    {
        CustomBrowser = 0,
        Projector,
        ExtraBrowser
    }; Q_ENUM(CustomTypes)

    enum WindowTypes
    {
        None = -1,
        SceneSource = 0,
        AudioMixer,
        Null_3,         // not used ( must exist )
        SoopChat,
        TwitchChat, 
        YoutubeChat,
        Null_7,         // not used ( must exist )
        BroadInfo,
        BLOCKITER,          // (~BlockIter) Blocks which pos needs to be saved
        AdvanceControls,    // ReplyBuffer
        SceneControl,
        CustomBrowserCollection,
        StatPage,
        Mission,
        Vote,
        Extensions,
        SAVVYReaction,
        AquaControl,
        SoopOverlay,
        Breaktime,
        EventBannerImage,  //Single Image Event Banner with 1 Button
        SubTitle,
        ENDOFINDEX
    }; Q_ENUM(WindowTypes)

    struct BlockBaseInfo
    {
        bool            noData           = false;
        bool            transparent      = false;
        bool            transformable    = false;
        bool            showLeft         = true;
        bool            showRight        = false;
        bool            needQuestionMark = false;
        bool            isCef            = false;
        QSize           minSize          = QSize(0,0);
        QSize           maxSize          = QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        const char*     windowTitle      = "";
	    QString         questionMarkToolTip = "";
        Qt::WindowFlags flags           = Qt::Window;
        ParentTypes     parentOnPopup   = NoParent;
    };

    struct ListReactionInfo
    {
        QString             remoteUrl = "";
        QString             localPath = "";
        int                 reactionIndex = 0;
    };
    
    struct ListReactionType
    {
        QString             reactionType = "";
        QString             styleType = "";
    };

    struct CefQueryMessage {
        QWidget* senderWidget;
        QCefQuery query;
    };



public slots:
    void qslotSwitchBlockWindowType(bool toPopup, int type);
    void qslotSwitchCustomBrowser(bool toPopup, QString uuid);
    void qslotClosePopup(int type);
    void qslotCloseSoopChat(bool switchFrame);
    void qslotClosePopupBySender();
    void qslotHideDock(int type);
    void qslotChangeSceneDoubleClick();

    void qslotToggleMenuCustomBrowser(bool show);
    void qslotCloseUuidDock(QString uuid, bool closeCef);
    void qslotCloseCustom(QString key, int type);
    void qslotHideCustom(QString key, int type);
    void qslotLockDock(bool lock);

    void qslotDockTitleRefreshClicked(int blockType);

    // cef query receive
    void qslotCefQueryReceived(int blockType, const QCefQuery& query);
    void qslotSoopChatDataReceived(const QCefQuery& query);
    void qslotCefPopupDataReceived(const QCefQuery& query);
    void qslotExtensionDataReceived(const QCefQuery& query);

    void qslotShowSceneControlDockTriggered();

    void qslotProjectorFullScreenTriggered();
    void qslotProjectorWindowTriggered();

    void qslotCreateYoutubePage(QString chat_id, std::string api_chat_id);

    void qslotShowAllOpenedBlocksOnExecute();

    void qslotCreateAfterCefBrowser(int type);
    void qslotLoadEndCefBrowser(int type);
    void qslotReceiveAquaRemoteControlUrl(const QByteArray& responseData);

    void qslotBreaktimeShow();
signals:
    void qsignalBlockAreaToggled(bool show, int key);
    void qsignalTransitionTriggered();
    void qsignalCloseGuide(int currentMission);
    void qsignalBlockVisible(bool visible, int type, bool enableFavoriteMenu = false);
    void qsignalAddSource(QString sourceID);
    void qsignalBreaktimeTick(int remainingSec, int totalSec);
    void qsignalBreaktimeFinished();

#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    bool InitPopups();
    void FinPopups();
    void CreatePlatformPage(std::string platform = "", bool reset = false);
    void DeletePlatformPage(std::string platform = "");
    bool AddBlock(ENUM_WINDOW_TYPE type, QWidget* contents);
    bool FindBlock(ENUM_WINDOW_TYPE type, QWidget*& outBlock) const;

    void ReloadCustomBrowserList(QVector<CUSTOMBROWSERINFO> info,
        QList<QString> deletedUuid);


    bool MakeDock(ENUM_WINDOW_TYPE type, AFQBaseDockWidget*& outDock, bool showDock);
    bool MakePopup(int type, AFQBorderPopupBaseWidget*& outPopup, bool toPopup = false, 
        bool showPopup = true, bool hasParent = false, QWidget* parent = nullptr);
    bool GetPopup(ENUM_WINDOW_TYPE type, AFQBorderPopupBaseWidget*& outPopup) const;
    bool GetDock(ENUM_WINDOW_TYPE type, AFQBaseDockWidget*& outDock) const;
    void RaiseAllPopup();
    void ResetDockUI(bool visible = true);
    void CloseAllBlocks(QList<ENUM_WINDOW_TYPE> exclusions = QList<ENUM_WINDOW_TYPE>());
    void InitDefaultDock();

    QString MakeProjector(int MonitorNum);

    bool CreateCustomBrowserListMenu(AFQCustomMenu*& menu);
    void OpenCustomBrowserBlock(CUSTOMBROWSERINFO info, int overlapCount = -1, bool firstRun = false);//overlap -1: Open with info
    bool OpenCustomBrowserDock(CUSTOMBROWSERINFO info, int overlapCount = -1, bool firstRun = true);//overlap -1: Open with info
    void ChangeCustomBrowserOpenState(QString uuid, bool isOpen);
    void ChangeAllCustomBrowserOpenState(bool isOpen);
    void ShowAllCustomBrowser();

    void OpenExtraBrowserPopup(QString key, QString title, std::string url, QSize size, bool allowedHide, QWidget* parent = nullptr);
    void ExecuteBrowserScript(QString key, const std::string& script);
    
    void ApplyBroadInfoToUI();

    void AdjustPositionOutSideFullScreen(QRect windowGeometry, QRect& outAdjustGeometry, bool checkMainScreen = false);
    void ReversePositionOutSideFullScreen(QRect parentGeometry, QRect windowGeometry, QRect& outAdjustGeometry);
    static void ApplyMoveInAllArea(QObject* applyWidget);
    static QRect GetMidGeometry(QSize popupSize);

    void CreateSoopPage(bool reload = false, bool reCreate = false);
    void CreateTwitchPage(bool reset = false);

    void ShowVodSplit(QWidget* parent = nullptr, QString prevTitle = "");
    void ShowVodAutoUploadNotice();
    bool ShowBroadcastNotice();

    void SendBroadState(bool broadstart);
    void SendScriptToChatBrowser(QString& script);

    void CheckDockState();
    void CheckPopupState();

    void FinishEditTitle();

    void DoubleCheckPosition(ENUM_WINDOW_TYPE type);
    void RestoreMagnetPopup();
    void ResetMagnet(ENUM_WINDOW_TYPE type);

    void ChangeAllMagnet(bool magnetOn);

    QSize CheckImageSize(QString path);

#pragma endregion public func

#pragma region private func
private:
    void _SetBlockBaseInfo();
    void _SetBlockBaseInfoExtra(QMap<int, const char*>& tooltipmap);
    bool _GetBlockBaseInfo(ENUM_WINDOW_TYPE type, AFQBlockManager::BlockBaseInfo& info);

    bool _MakePopup(ENUM_WINDOW_TYPE type, AFQBorderPopupBaseWidget*& outPopup,
        QSize min = QSize(0, 0), QSize max = QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX),
        bool transparent = false, bool showLeft = true, bool showRight = false,
        QWidget* parent = nullptr,
        Qt::WindowFlags flags = Qt::Window, bool showPopup = true, bool isCef = false);

    bool    _CreateCurrentSceneProjector(AFQProjector*& projector, QString& sceneName, int monitor);

    bool _BasicSetTitleWidget(ENUM_WINDOW_TYPE type, AFDockTitle*& outTitle,
        bool popupmode = true, bool fixed = false);

    void _ChangeBlockUse(bool used, ENUM_WINDOW_TYPE checkType);

    void _ToPopup(QString uuid);
    void _ToDock(QString uuid);
    void _ToPopup(ENUM_WINDOW_TYPE type);
    void _ToDock(ENUM_WINDOW_TYPE type);
    bool _ResetBlockToDock(ENUM_WINDOW_TYPE type, AFQBaseDockWidget*& outDock);

    bool _CheckDockCanInsert(QSize dockSize);

    bool _ClosePopup(ENUM_WINDOW_TYPE key, bool isToDock = false);
    bool _CloseDock(ENUM_WINDOW_TYPE key, bool closeCef);
    void _DeleteBlock(ENUM_WINDOW_TYPE key);

    bool _CreateNeededBlock(ENUM_WINDOW_TYPE type, QWidget*& outblock);

    void _CreateCustomBrowserBlock();
    bool _CreateSceneSourceBlock();
    bool _CreateAudioMixer();
    bool _CreateBroadInfo();

    bool _CreateSoopChat(bool force);
    bool _CreateTwitchChat();
    bool _CreateYoutubeChat(const QString& chat_id, const std::string& api_chat_id); //Always Reset (No Reuse)

    bool _CreateCefPopup(ENUM_WINDOW_TYPE type);
    bool _GetCefPopupURL(ENUM_WINDOW_TYPE type, std::string& url);

    void _DeleteSoopPage();
    void _DeleteTwitchPage();
    bool _DeleteYoutubePage();

    bool _CreateCustomBrowserCollection();
    bool _CreateAdvanceControlBlock();
    bool _CreateSceneControl();
    bool _CreateStat();
    bool _CreateOverlay();
    bool _CreateBreakTime(bool bSkip = false);
    bool _CreateEventImage();

    void _LoadBlock();
    void _SaveBlock();
    void _ClearAllBlocks();
    void _CloseAllPopups(QList<ENUM_WINDOW_TYPE> exclusions = QList<ENUM_WINDOW_TYPE>()); // Popup has close in closeEvent qslotClosePopup
    void _CloseAllDocks(QList<ENUM_WINDOW_TYPE> exclusions = QList<ENUM_WINDOW_TYPE>()); // Dock close without closeEvent

    void _LoadCustomBrowserList();
    void _SaveCustomBrowserBlock();

    bool _AuthToGlobalMain();

    void _AddMissionSource(int type, std::string url);

    void _OpenSavvySoopFolder();
    void _DownloadSavvyReactionVideo(std::string strUrl, std::string strReactionType, std::string strStyleType, int nReactionIndex);
    void _StartDownload();
    void _DownloadFinished(QNetworkReply* reply);
    void _AddReactionListSource(std::string url);
    obs_source_t* _IsExistingReactionListSource();
    bool m_firstAddReaction = true;

    void _ProcessCefQuery(CefQueryMessage& queryMessage);
    void _cefWorkerLoop();

#pragma endregion private func
#pragma region public member var

#pragma endregion public member var

#pragma region private member var
private:
    QMap<ENUM_WINDOW_TYPE, BlockBaseInfo>             m_blockBaseInfoMap;

    QMap<ENUM_WINDOW_TYPE, QWidget*>                  m_blockMap;
    QMap<ENUM_WINDOW_TYPE, AFTTopBaseWidget*>         m_popupWidgets;   //Key: Block Type
    QMap<ENUM_WINDOW_TYPE, AFQBaseDockWidget*>        m_dockWidgets;    //Key: Block Type

    QVector<CUSTOMBROWSERINFO>                        m_customBrowserInfoVector;
    QMap<QString, AFQBorderPopupBaseWidget*>          m_browserWidgetMap; //Key: UUID
    QMap<QString, AFQBaseDockWidget*>                 m_browserDocks; //Key: UUID
    QMap<QString, AFQBorderPopupBaseWidget*>          m_extraBrowserPopups; //Key: poup key

    QMap<QString, AFQBorderPopupBaseWidget*>          m_projectorWidgetMap; //Key: UUID
    bool                                m_isBlockAnimating = false;
    
    QTimer                              m_scriptExecuteTimer;

    QNetworkAccessManager               m_netManager;
    std::list<ListReactionInfo>         m_listReactionData;
    std::list<ListReactionType>         m_listReactionType;
    std::mutex                          m_listMutex;


    std::queue<CefQueryMessage>         m_cefQueryQueue;
    std::mutex                          m_cefQueryMutex;
    std::condition_variable             m_cefQueryCV;
    std::atomic_bool                    m_cefQueryThreadRunning = true;
    std::thread                         m_cefWorkerThread;

    QPointer<AFTTopBaseDialog>         m_breaktimeDialog;
#pragma endregion private member var

};

extern std::string EscapeForJavaScript(const std::string& input);
