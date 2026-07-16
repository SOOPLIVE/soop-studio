#pragma once
#include <sstream>
#include <string>
#include <json11.hpp>

#include <QObject>
#include <QMap> //CoreModel Qt
#include <QByteArray>
#include <QTimer>

#define DEFAULT_MANAGER 10

class AFQBroadInfo;

struct AFItemInfo 
{
    int topListEndTime = 0;

    //
    int addManagerEndTime = 0;

    //
    bool haveBurningItem = false;
    bool usingBurningItem = false;
    int  burningItemKey = 0;
    int  burningItemEndTime = 0;

    //
    int  haveMobileAlarmItem = 0;   // 1 : 1day, 7 : 7day
    bool useMobileAlarmItem = false;
    int  mobileAlarmItemEndTime = 0;
    int  sendAbleAlarm = 0;
    int  sendAbleRemainTime = 0;


    bool IsHavingItems()
    {
        return (false != haveBurningItem || 0 != haveMobileAlarmItem);
    }

    bool IsUsingItems()
    {
        return (0 != topListEndTime ||
                0 != addManagerEndTime ||
                false != usingBurningItem ||
                false != useMobileAlarmItem);
    }

    void clear() 
    {
        topListEndTime = 0;
     
        addManagerEndTime = 0;
      
        haveBurningItem = false;
        usingBurningItem = false;
        burningItemKey = 0;
        burningItemEndTime = 0;
     
        haveMobileAlarmItem = 0;
        useMobileAlarmItem = false;
        mobileAlarmItemEndTime = 0;
        sendAbleAlarm = 0;
        sendAbleRemainTime = 0;
    }
};

class AFBroadInfo
{
    friend class AFQBroadInfo;

public:
    explicit AFBroadInfo() {};
    ~AFBroadInfo() {};

private:
    AFItemInfo itemInfo;
    std::string title;
    std::string endingMsg;
    std::string broadPwd;

    std::string categoryName;
    std::vector<std::string> hashTags;

    std::vector<std::pair<std::string, std::string>> langVector; 
    std::string lang;
    
    int category = -1;
    int maxAllowedManagers = DEFAULT_MANAGER;
    int currentMaxManagers = DEFAULT_MANAGER;

    bool allow1440p = false;
    bool allowAV1 = false;
    bool allowSubTitle = false;
    bool adultOnly = false;
    bool broadPwdChk = false;
    bool broadHidden = false;
    bool broadTuneOut = false;
    bool paidPromotion = false;
    int watermarkPos = 1;

    bool isWait = false;
    
    int waitingTime = 0; // No need to be int
    bool sarsaUser = false;

    int broadNumber = 0;
    int isSubscribeBroad = false;

    std::string prevTitle;

    bool isAIManger = false;
    int aiManagerTime = 0;
};

struct ViewerUserInfo
{
    int currentViewer = 0;
    int pcViewer = 0;
    int mobileViewer = 0;
    int relayViewer = 0;
    int relayPcViewer = 0;
    int relayMobileViewer = 0;
    int accumulateViewer = 0;
    int accumulatePcViewer = 0;
    int accumulateMobileViewer = 0;
    int relayCount = 0;
};

struct UpInfo
{
    int upTotal = 0;
    QString upNickname;
};

struct GiftInfo
{
    QString total;
};

class AFQBroadInfo : public QObject
{
    Q_OBJECT

private slots:
    void _qslotResponseCategorysAPI(const QByteArray& responseData);
    void _qslotResponseUserNationAPI(const QByteArray& responseData);    
    void _qslotResponseStickerItemInfo(const QByteArray& responseData);
    void _qslotSendBroadInfoResult(const QByteArray& responseData); // Broad Info, Broad Setting
    void _qslotSendUpResult(const QByteArray& responseData);
    void _qslotSendViewerResult(const QByteArray& responseData);
    void _qslotSendGiftResult(const QByteArray& responseData);
    void _qslotReceiveStreamerInfo(const QByteArray& responseData);

public slots:
    void qslotResponseBroadInfoAPI(const QByteArray& responseData, int initStudio);

    void qslotPollingBroadNumberTimer();
    void qslotSendBroadInfoTimer();
    void qslotResponseBroadNumberDataAPI(const QByteArray& responseData);
    void qslotResponseOpenAPIDamainDataAPI(const QByteArray& responseData);

signals:
    void qsignalBroadStartAPIError(int result, QString msg);
    void qsignalBroadInfoReceived(int result, QString msg);
    void qsignalSendBroadInfoResult(int result, QString msg);
    void qsignalBroadInfoAPIError(int result, QString msg);
    void qsignalUpResult(int result, QString msg);
    void qsignalViewerResult(int result, QString msg);
    void qsignalStreamInfoResult(int result, QString msg);
    void checkResolution();

    //subscribe
    void qsignalSubscribeCheck();

public:
    explicit AFQBroadInfo(QObject* parent = nullptr);
    ~AFQBroadInfo();

    void ReceiveSoopStreamerInfo();
    void ReceiveSoopStreamerInfoWithTempCookie(QObject* receiver, const char* slot, std::string cookie);
    void RequestStickerItemInfo();
    void RequestCategoryListAPI();
    void RequestUserNationAPI();
    void ReceiveUp();
    void ReceiveViewer();
    void ReceiveGift();
    void SendBroadInfoSetting(bool sendSubscribe = false);

    void BroadNumTimerAfterStart();

    std::vector<std::string> SplitTag(const std::string& str, char delimiter);
    std::string MergeTag(std::vector<std::string> vect);
    void SetHashTags(std::vector<std::string> tags);
    void ClearHashTags();

    bool SetCategory(int categoryNum);
    std::string CategoryNumberString(int category);
    bool ReceiveCategoryString(int categoryNum, std::string& categoryString);
    bool ReceiveCategoryString(int categoryNum, QString& categoryString, bool parentcategory = false);
    bool ReceiveCategoryNum(std::string categoryString, int& categoryNum);
    bool ReceiveCategoryNum(QString categoryString, int& categoryNum);
    void RefreshCategoryList(const QByteArray& responseData);

    void Lang(std::string lanuage) { m_broadInfo->lang = lanuage; };

    void SetTitle(std::string title) { m_broadInfo->title = title; };
    std::string Title() { return m_broadInfo->title; };
    void SetEndingMessage(std::string message) { m_broadInfo->endingMsg = message; };
    std::string EndingMessage() { return m_broadInfo->endingMsg; };
    void SetPassword(std::string password);
    std::string Password() { return m_broadInfo->broadPwd; };

    int CategoryNumber() { return m_broadInfo->category; };
    std::string Category() { return m_broadInfo->categoryName; };

    std::vector<std::string> HashTags() { return m_broadInfo->hashTags; };

    std::vector<std::pair<std::string, std::string>>& LangVector() { return m_broadInfo->langVector; };
    std::string Lang() { return m_broadInfo->lang; }

    bool IsHavingItems() { return m_broadInfo->itemInfo.IsHavingItems(); };
    bool IsUsingItems() { return m_broadInfo->itemInfo.IsUsingItems(); };

    int MaxAllowedManagers() { return m_broadInfo->maxAllowedManagers; }
    int CurrentMaxManagers() { return m_broadInfo->currentMaxManagers; }
    int SetCurrentMaxManagers(int managers) { return m_broadInfo->currentMaxManagers; }

    bool Allow1440P() { return m_broadInfo->allow1440p; };
    bool AllowAV1() { return m_broadInfo->allowAV1; };
    bool AllowSubTitle() { return m_broadInfo->allowSubTitle; }
    void SetAdultOnly(bool adult) { m_broadInfo->adultOnly = adult; };
    bool AdultOnly() { return m_broadInfo->adultOnly; };
    void SetUsePassword(bool use) { m_broadInfo->broadPwdChk = use; };
    bool UsePassword() { return m_broadInfo->broadPwdChk; };
    void SetBroadHidden(bool hidden) { m_broadInfo->broadHidden = hidden; };
    bool BroadHidden() { return m_broadInfo->broadHidden; };
    void SetBroadTuneOut(bool tuneout) { m_broadInfo->broadTuneOut = tuneout; };
    bool BroadTuneOut() { return m_broadInfo->broadTuneOut; };
    void SetPaidPromotion(bool promotion) { m_broadInfo->paidPromotion = promotion; };
    bool PaidPromotion() { return m_broadInfo->paidPromotion; };
    void SetWaterMarkPosition(int waterMark) { m_broadInfo->watermarkPos = waterMark; };
    int WaterMarkPosition() { return m_broadInfo->watermarkPos; };

    int CurrentViewer() { return m_viewerInfo.currentViewer; };
    int PcViewer() { return m_viewerInfo.pcViewer; };
    int MobileViewer() { return m_viewerInfo.mobileViewer; };
    int RelayViewer() { return m_viewerInfo.relayViewer; };
    int RelayPcViewer() { return m_viewerInfo.relayPcViewer; };
    int RelayMobileViewer() { return m_viewerInfo.relayMobileViewer; };
    int AccumulateViewer() { return m_viewerInfo.accumulateViewer; };
    int AccumulatePcViewer() { return m_viewerInfo.accumulatePcViewer; };
    int AccumulateMobileViewer() { return m_viewerInfo.accumulateMobileViewer; };
    int RelayCount() { return m_viewerInfo.relayCount; };
    void ResetAllViewerCount();

    int UpTotal() { return m_upInfo.upTotal; };
    QString UpNickname() { return m_upInfo.upNickname; };
    QString Gift() { return m_giftInfo.total; }
    void SetBroadWaitingTime(int waitTime) { m_broadInfo->waitingTime = waitTime; };
    int  BroadWaitingTime() { return m_broadInfo->waitingTime; };

    bool  SarsaUser() { return m_broadInfo->sarsaUser; };
    
    // Item Info
    AFItemInfo ItemInfo() { return m_broadInfo->itemInfo; }

    std::string PrevTitle() { return m_broadInfo->prevTitle; }
    void  SetPrevTitle(std::string prevTitle) { m_broadInfo->prevTitle = prevTitle; }

    int  BroadNumber() { return m_broadInfo->broadNumber; }
    void SetBroadNumber(int broadNum) { m_broadInfo->broadNumber = broadNum; }

    QString BroadStartTime() { return m_broadStartTime; }
    void SetBroadStartTime(QString broadStartTime) { m_broadStartTime = broadStartTime; }

    QString OpenAPIDomain() { return m_openAPIDomain; }
    void SetOpenAPIDomain(QString openAPIDomain) { m_openAPIDomain = openAPIDomain; }

    QString ExtSessionDomain() { return m_extSessionDomain; }
    void SetExtSessionDomain(QString extSessionDomain) { m_extSessionDomain = extSessionDomain; }

    int SubscribeBroad() { return m_broadInfo->isSubscribeBroad; };
    void SetSubscribeBroad(int subBroad) { m_broadInfo->isSubscribeBroad = subBroad; };

    void SetUserNation(const QString& nation) { m_userNation = nation; }
    QString GetUserNation() const { return m_userNation; }

    bool  IsAIManager() { return m_broadInfo->isAIManger; }
    void SetIsAIManager(bool isAIManager) { m_broadInfo->isAIManger = isAIManager; };

    int  AIManagerTime() { return m_broadInfo->aiManagerTime; }
    void SetAIManagerTime(int afktime);

private:
    bool _ParseBroadInfoJson(json11::Json& jsonBroadInfo);
    int getJsonObjNumber(json11::Json obj);
    bool safe_stoi(const std::string& str, int& out);

private:
    std::unique_ptr<AFBroadInfo> m_broadInfo;
    ViewerUserInfo m_viewerInfo;
    UpInfo m_upInfo;
    GiftInfo m_giftInfo;
    QMap<int, QString> m_categorys;

    QTimer  m_pollingBroadNumTimer;
    QString m_broadStartTime;
    QString m_openAPIDomain;
    QString m_extSessionDomain;
    QString m_userNation = "kr";
};
