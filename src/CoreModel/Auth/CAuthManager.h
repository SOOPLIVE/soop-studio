#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <QByteArray>
#include <QSslCipher>
#include <QCryptographicHash>
#include <QtConcurrent/QtConcurrent>

#include "SBaseAuth.h"
#include "SBroadInfo.h"


#define IN
#define OUT

#define PLATFORM_SOOP           "SOOP"
#define PLATFORM_TWITCH         "Twitch"
#define PLATFORM_YOUTUBE        "Youtube"
#define PLATFORM_CUSTOM_RTMP    "Custom RTMP"

typedef std::unordered_map<std::string, AFBasicAuth*> tAUTH_MAP;  // key : uuid
typedef std::unordered_map<std::string, int> tAUTH_CACHE_MAP;  // key : uuid

class AFAuthManager final
{
public:
    AFAuthManager() = default;
    ~AFAuthManager() = default;

public:
    void                                LoadAllAuthed();
    void                                SaveAllAuthed();

    void                                ClearCache();
    bool                                CacheAuth(bool main, AFBasicAuth cacheAuth);
    bool                                CacheAuth(bool main, AFBasicAuth* cacheAuth);
    void                                GetUuidFromCache(std::string platform, std::string& outUuid);
    void                                RemoveCachedAuth(const char* uuid);
    void                                FlushAuthCache();

    int                                 IsRegisterChannel(const char* uuid);
    void                                swapChannel(int newIndex, int oldIndex);
 
    void 								ClearRegisterChannel() { m_listRegChannel.clear(); };

    void                                RegisterChannel(const char* uuid, AFChannelData* pChannel);
    void                                RemoveAllChannel(bool removeMain = true);
    void                                RemoveChannel(int indx);
    void                                RemoveMainChannel();
    void                                RemoveChannel(std::string platform);
    void                                RemoveChannel(AFChannelData* data);
    
    bool                                CacheMain(AFBasicAuth cacheAuth);

    /// SOOP
    bool                                SetSoopCookie(std::string cookie);
    std::string                         SoopCookie();
    bool                                RefreshCookie();
    bool                                VodSaveAvailableFromAPI();
    bool                                VodRequestVodSave(std::string title, std::string hashtag);
    bool                                RequestBroadInfoAPI(bool initStudio = false);
    bool                                RequestStickerItemInfo();
    void                                SendSoopBroadInfoSetting(bool sendSubscribe = false);
    std::string                         MergeHashTag();

    void                                SendCheckBroadStart(QObject* receiver, const char* slot);
    void                                SendCheckBroading(QObject* receiver, const char* slot);

    std::chrono::steady_clock::time_point    GetMinsimCheckStartTime() { return m_minsim_check_start_time; }
    void                                     SetMinsimCheckStartTime(std::chrono::steady_clock::time_point time) { m_minsim_check_start_time = time; }
    void                                     InitMinsimCheckStartTime() { m_minsim_check_start_time = std::chrono::steady_clock::time_point{}; }

    std::string                         GetMinsimCheckThemeInfo() { return m_minsim_check_theme_log_type; }
    void                                SetMinsimCheckThemeInfo(std::string str_info) { m_minsim_check_theme_log_type = str_info; }

    /// SOOP

    std::string                         CreateUrlProfileImg(AFChannelData* pChannelData);
    bool                                GetChannelID(const char* platform, std::string& channelID);
    bool                                GetChannelData(int viewIndx, AFChannelData*& outRefDataPointer);
    bool                                GetChannelData(void* pObjOBSService, AFChannelData*& outRefDataPointer);
    bool                                GetChannelData(std::string platform, AFChannelData*& outRefDataPointer);
    bool                                GetMainChannelData(AFChannelData*& outRefDataPointer);
        
    bool                                IsAuthed(const char* uuid);
    bool                                IsCachedAuth(const char* uuid);
    
    
    std::vector<void*>*                 GetContainerDeferrdDelObj();
    
    
    int                                 GetCntChannel() { return m_listRegChannel.size(); };
    bool                                IsHaveLoadedAuth() { return m_loaded; };

    bool                                IsTwitchRegistered() { return m_twitchRegistered; };
    bool                                IsYoutubeRegistered() { return m_youtubeRegistered; };
    bool                                IsSoopRegistered() { return m_soopRegistered; };
    bool                                IsSoopStreaming();
    void                                SetTwitchRegistered(bool regi) { m_twitchRegistered = regi; };
    void                                SetYoutubeRegistered(bool regi) { m_youtubeRegistered = regi; };
    void                                SetSoopRegistered(bool regi) { m_soopRegistered = regi; };
    
    //KR BroadInfo
    void                                InitSoopBroadInfo();
    void                                DeleteSoopBroadInfo();
    bool                                LoadSoopBroadInfo();    
    bool                                LoadSoopBroadInfo(const char* json_data);
    void                                LoadSoopStreamerInfo();
    void                                SaveSoopBroadInfo();
    AFQBroadInfo*                       GetSoopBroadInfo() { return m_pSoopBroadInfo; }

    //KR BroadInfo

    bool                                GetSoopChatUrl(std::string& chaturl);
    bool                                GetTwitchChatUrl(std::string& moderation_tools_url, std::string& chaturl);
    bool                                GetYoutubeChatUrl(std::string chat_id, std::string api_chat_id, std::string& chatUrl);

    std::string                         CategoryNumberString(int category);
    std::string                         FullCategoryName(int category);
    bool                                ReceiveCategoryString(int categoryNum, std::string& categoryString);
    bool                                ReceiveCategoryNum(std::string categoryString, int& categoryNumString);

    void                                TwitchTryLoadSecondaryUIPanes();
    QMap<std::string, std::string>      GetLiveChannels();
    void                                OffLiveExceptSoop();

    int64_t                             GetServerTime();
    void                                GetAIManagerToken(QObject* receiver, const char* slot);

private: 
    std::string                         _APIProfileImgSoop(AFBasicAuth* pAuthedData);
    std::string                         _APIProfileImgTwitch(AFBasicAuth* pAuthedData);
    std::string                         _APIProfileImgYoutube(AFBasicAuth* pAuthedData);

    std::string                         _GetPathSaveFile();
    std::string                         _ToString(uint64_t value);
    
    bool                                _CheckInvalidAuth(AFBasicAuth& auth);

    void                                _InitPrivateKey(OUT QByteArray& HashedKey,
                                                        OUT QByteArray& initVector);
    //
private:
    AFBasicAuth*                        m_pCacheMainSoop = nullptr;
    AFChannelData*                      m_pChannelMainSoop = nullptr;
    AFQBroadInfo*                       m_pSoopBroadInfo = nullptr;
    
    int                                 m_indxLastCache = -1;
    std::vector<AFBasicAuth*>           m_cacheAuths;
    tAUTH_CACHE_MAP                     m_cacheAuthMap;
    
    std::vector<AFChannelData*>         m_listRegChannel;
    
    std::vector<void*>                  m_deferredDeleteQtPixmap;

    bool                                m_loaded = false;
    bool                                m_twitchRegistered = false;
    bool                                m_youtubeRegistered = false;
    bool                                m_soopRegistered = false;

    std::chrono::steady_clock::time_point       m_minsim_check_start_time{};
    std::string                                 m_minsim_check_theme_log_type = "default";
};
