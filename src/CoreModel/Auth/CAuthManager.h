#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <QByteArray>
#include <QSslCipher>
#include <QCryptographicHash>


#include <json11.hpp>



#include "SBaseAuth.h"


#define IN
#define OUT




typedef std::unordered_map<std::string, AFBasicAuth*> tAUTH_MAP;  // key : uuid
typedef std::unordered_map<std::string, int> tAUTH_CACHE_MAP;  // key : uuid

class AFAuthManager final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFAuthManager& GetSingletonInstance()
    {
        static AFAuthManager* instance = nullptr;
        if (instance == nullptr)
            instance = new AFAuthManager;
        
        return *instance;
    };
    ~AFAuthManager();
private:
    // singleton constructor
    AFAuthManager() = default;
    AFAuthManager(const AFAuthManager&) = delete;
    AFAuthManager(/* rValue */AFAuthManager&& other) noexcept = delete;
    AFAuthManager& operator=(const AFAuthManager&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
//    void                                InitContext() { void; };
//    void                                FinContext() { void; };
 
    void                                LoadAllAuthed();
    void                                SaveAllAuthed();
    
    void                                ClearCache();
    bool                                CacheAuth(bool main, AFBasicAuth cacheAuth);
    void                                RemoveCachedAuth(const char* uuid);
    void                                FlushAuthCache();

    int                                 IsRegisterChannel(const char* uuid);
    void                                swapChannel(int newIndex, int oldIndex);

    void ClearRegisterChannel() {
        m_vecListRegChannel.clear();
    };

    void                                RegisterChannel(const char* uuid, AFChannelData* pChannel);
    void                                RemoveChannel(int indx);
    void                                RemoveMainChannel();
    
    bool                                CacheMain(AFBasicAuth cacheAuth);
    void                                FlushAuthMain();
    
    std::string                         CreateUrlProfileImg(AFChannelData* pChannelData);
    bool                                GetChannelData(int viewIndx, AFChannelData*& outRefDataPointer);
    bool                                GetChannelData(void* pObjOBSService, AFChannelData*& outRefDataPointer);
    bool                                GetMainChannelData(AFChannelData*& outRefDataPointer);
    
    bool                                FindRTMPKey(const char* channelID, OUT std::string& key);
    bool                                FindRTMPUrl(const char* channelID, OUT std::string& url);
    
    bool                                GetRTMPKeyGlobalSoopMainAuth(OUT std::string& key);
    bool                                GetRTMPUrlGlobalSoopMainAuth( OUT std::string& url);
    
    
    bool                                IsAuthed(const char* uuid);
    bool                                IsCachedAuth(const char* uuid);
    
    
    std::vector<void*>*                 GetContainerDeferrdDelObj();
    
    
    int                                 GetCntChannel() { return m_vecListRegChannel.size(); };
    bool                                IsHaveLoadedAuth() { return m_bLoaded; };

    bool                                IsTwitchRegistered() { return m_bTwitchRegistered; };
    bool                                IsYoutubeRegistered() { return m_bYoutubeRegistered; };
    bool                                IsSoopRegistered() { return m_bSoopRegistered; };
    bool                                IsSoopGlobalRegistered() { return m_bSoopGlobalRegistered; };
    void                                SetTwitchRegistered(bool regi) { SetTwitchPreregistered(regi);  m_bTwitchRegistered = regi; };
    void                                SetYoutubeRegistered(bool regi) { SetYoutubePreregistered(regi);  m_bYoutubeRegistered = regi; };
    void                                SetSoopRegistered(bool regi) { SetSoopPreregistered(regi);  m_bSoopRegistered = regi; };
    void                                SetSoopGlobalRegistered(bool regi) { SetSoopGlobalPreregistered(regi);  m_bSoopGlobalRegistered = regi; };

    bool                                IsTwitchPreregistered() { return m_bTwitchPreregistered; };
    bool                                IsYoutubePreregistered() { return m_bYoutubePreregistered; };
    bool                                IsSoopPreregistered() { return m_bSoopPreregistered; };
    bool                                IsSoopGlobalPreregistered() { return m_bSoopGlobalPreregistered; };
    void                                SetTwitchPreregistered(bool regi) { m_bTwitchPreregistered = regi; };
    void                                SetYoutubePreregistered(bool regi) { m_bYoutubePreregistered = regi; };
    void                                SetSoopPreregistered(bool regi) { m_bSoopPreregistered = regi; };
    void                                SetSoopGlobalPreregistered(bool regi) { m_bSoopGlobalPreregistered = regi; };
    
    void                                SetSoopGlobalClientID(std::string value) { m_strMainAuthClientID = value; };
    std::string                         GetSoopGlobalClientID() { return m_strMainAuthClientID; };
#pragma endregion public func

#pragma region private func
private:
    std::string                         _APIProfileImgSoopGlobal(AFBasicAuth* pAuthedData);
    std::string                         _APIProfileImgSoop(AFBasicAuth* pAuthedData);
    std::string                         _APIProfileImgTwitch(AFBasicAuth* pAuthedData);
    std::string                         _APIProfileImgYoutube(AFBasicAuth* pAuthedData);

    

    std::string                         _GetPathSaveFile();
    std::string                         _ToString(uint64_t value);
    json11::Json                        _AuthObjToJson(AFBasicAuth* pAuth, AFChannelData* pChannel);
    AFBasicAuth                         _JsonObjToAuth(json11::Json& item);
    
    bool                                _CheckInvalidAuth(AFBasicAuth& auth);
#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
    std::string                         m_strMainAuthClientID;
    AFBasicAuth*                        m_CacheMainGlobalSoop = nullptr;
    AFChannelData*                      m_ChannelMainGlobalSoop = nullptr;
    
    
    int                                 m_IndxLastCache = -1;
    std::vector<AFBasicAuth*>           m_vecCacheAuth;
    tAUTH_CACHE_MAP                     m_mapCacheAuth;
    
    std::vector<AFChannelData*>         m_vecListRegChannel;
    
    
    std::vector<void*>                  m_vecDeferredDeleteQtPixmap;
    
    bool                                m_bLoaded = false;
    bool                                m_bTwitchRegistered = false;
    bool                                m_bYoutubeRegistered = false;
    bool                                m_bSoopRegistered = false;
    bool                                m_bSoopGlobalRegistered = false;

    bool                                m_bTwitchPreregistered = false;
    bool                                m_bYoutubePreregistered = false;
    bool                                m_bSoopPreregistered = false;
    bool                                m_bSoopGlobalPreregistered = false;
#pragma endregion private member var
};
