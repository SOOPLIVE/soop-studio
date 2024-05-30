#include "CAuthManager.h"


#include <iostream>
#include <sstream>
#include <list>
#include <tuple>

#include <util/platform.h>


#include "CoreModel/Config/CConfigManager.h"

#include "ViewModel/Auth/COAuthLogin.hpp"
#include "ViewModel/Auth/Soop/auth-soop-global.h"
#include "ViewModel/Auth/Soop/auth-soop.hpp"
#include "ViewModel/Auth/Twitch/auth-twitch.h"
#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"


void AFAuthManager::LoadAllAuthed()
{
    std::string savedFileName = _GetPathSaveFile();

    FILE* f = os_fopen(savedFileName.c_str(), "rb");
    if (!f)
        return;

    char savedData[65536] = { 0, };
    size_t fileSize = fread(savedData, 1, sizeof(savedData), f);
    fclose(f);
    f = NULL;

    if (fileSize == 0)
        return;

    QByteArray savedBytes(savedData, fileSize);

   
    std::string errorParse;
    json11::Json json = json11::Json::parse(savedBytes.toStdString(),
        errorParse);


    m_strMainAuthClientID = json["globalSoopMainClientId"].string_value();
    
    auto items = json["items"].array_items();

    if (items.size() == 0)
        return;


    std::list<std::tuple<AFBasicAuth*, AFChannelData*>> listNotOrderd;

    for (auto& item : items)
    {
        AFBasicAuth* pTemp = new AFBasicAuth(_JsonObjToAuth(item));


        AFChannelData* pChannel = new AFChannelData();
        pChannel->pAuthData = pTemp;
        pChannel->bIsStreaming = item["channelData"]["nowStreaming"].bool_value();
        pChannel->strImgUrlUserThumb = item["channelData"]["profileImgUrl"].string_value();

        std::string platform = pTemp->strPlatform;
        if (pTemp->strPlatform == "SOOP Global") 
        {
            CacheAuth(true, *pTemp);
            m_bSoopGlobalRegistered = true;
            m_CacheMainGlobalSoop = pTemp;
            m_ChannelMainGlobalSoop = pChannel;
        }
        else 
        {
            CacheAuth(false, *pTemp);
            int indexChannelList = item["channelData"]["IndxViewList"].int_value();
            pTemp->iViewChannelIndx = indexChannelList;

            if (pTemp->strPlatform == "afreecaTV")
                m_bSoopRegistered = true;
            else if (pTemp->strPlatform == "Twitch")
                m_bTwitchRegistered = true;
            else if (pTemp->strPlatform == "Youtube")
                m_bYoutubeRegistered = true;
            

            if (indexChannelList == m_vecListRegChannel.size())
                m_vecListRegChannel.insert(m_vecListRegChannel.begin() + indexChannelList, pChannel);
            else if (indexChannelList == 0)
                m_vecListRegChannel.emplace_back(pChannel);
            else
                listNotOrderd.emplace_back(pTemp, pChannel);

        }
    }

    int cntNotOrdered = listNotOrderd.size();
    int cntItt = 0;
    auto itterListNotOrderd = listNotOrderd.begin();
    while (listNotOrderd.size() > 0)
    {
        auto itterSecound = listNotOrderd.begin();
        while (itterSecound != listNotOrderd.end())
        {
            AFBasicAuth* rawAuth = nullptr;
            AFChannelData* channel = nullptr;
            std::tie(rawAuth, channel) = *itterSecound;

            if (rawAuth->iViewChannelIndx == m_vecListRegChannel.size())
            {
                m_vecListRegChannel.insert(m_vecListRegChannel.begin() +
                    rawAuth->iViewChannelIndx,
                    channel);
                itterSecound = listNotOrderd.erase(itterSecound);
            }
            else if (listNotOrderd.size() == 1)
            {
                rawAuth->iViewChannelIndx = m_vecListRegChannel.size();
                m_vecListRegChannel.emplace_back(channel);
                itterSecound = listNotOrderd.erase(itterSecound);
            }
            else
                itterSecound++;
        }

        cntItt++;
        if (cntItt > cntNotOrdered * 2)
            break;
    }

    int leftSize = m_vecListRegChannel.size();

    while (listNotOrderd.size() > 0)
    {
        auto itterSecound = listNotOrderd.begin();
        while (itterSecound != listNotOrderd.end())
        {
            AFBasicAuth* rawAuth = nullptr;
            AFChannelData* channel = nullptr;
            std::tie(rawAuth, channel) = *itterSecound;

            rawAuth->iViewChannelIndx = leftSize;

            m_vecListRegChannel.emplace_back(channel);
            itterSecound = listNotOrderd.erase(itterSecound);

            leftSize++;
        }
    }

    if (m_CacheMainGlobalSoop)
        m_bLoaded = true;

    if (m_vecListRegChannel.size() > 0)
        m_bLoaded = true;
}

void AFAuthManager::SaveAllAuthed()
{
    json11::Json::array authList;

    if (m_CacheMainGlobalSoop)
    {
        authList.push_back(_AuthObjToJson(m_CacheMainGlobalSoop, m_ChannelMainGlobalSoop));
    }

    for(auto& node : m_vecListRegChannel)
    {
        authList.push_back(_AuthObjToJson(node->pAuthData, node));
    }
    
    json11::Json out = json11::Json::object {
        { "items", authList },
        { "globalSoopMainClientId", m_strMainAuthClientID }
    };

    std::string outJsonString = out.dump();
    //blog(LOG_INFO, outJsonString.c_str());
    QByteArray outBytes(outJsonString.c_str());
    

    std::string saveFileName = _GetPathSaveFile();

    bool success = os_quick_write_utf8_file(saveFileName.c_str(),
                                            outJsonString.c_str(),
                                            outJsonString.size(),
                                            false);
}

void AFAuthManager::ClearCache()
{
    m_mapCacheAuth.clear();
    m_vecCacheAuth.clear();
    m_IndxLastCache = -1;

    SetSoopGlobalPreregistered(IsSoopGlobalRegistered());
    SetSoopPreregistered(IsSoopRegistered());
    SetYoutubePreregistered(IsYoutubeRegistered());
    SetTwitchPreregistered(IsTwitchRegistered());
}

bool AFAuthManager::CacheAuth(bool main, AFBasicAuth cacheAuth)
{
    AFBasicAuth* tmp = new AFBasicAuth(cacheAuth);
    if (cacheAuth.strUuid.empty() || cacheAuth.strUuid == "")
        return false;
    
    if (main)
    {
        m_CacheMainGlobalSoop = tmp;
    }
    else
    {
        m_IndxLastCache++;
        m_mapCacheAuth.insert({tmp->strUuid, m_IndxLastCache});
        m_vecCacheAuth.emplace_back(tmp);
    }
    
    return true;
}

void AFAuthManager::RemoveCachedAuth(const char* uuid)
{
    auto itterfound = m_mapCacheAuth.find(uuid);
    if (itterfound != m_mapCacheAuth.end())
    {
        m_vecCacheAuth.erase(m_vecCacheAuth.begin() + itterfound->second);
        m_IndxLastCache--;

        int foundIndx = itterfound->second;

        m_mapCacheAuth.erase(itterfound);

        for (auto& node : m_mapCacheAuth) {
            if (node.second > foundIndx)
                node.second = node.second - 1;
        }
    }

}

void AFAuthManager::FlushAuthCache()
{    
    ClearCache();
}

void AFAuthManager::swapChannel(int newIndex, int oldIndex)
{
    if (newIndex == oldIndex)
        return;
    if (m_vecListRegChannel.size() <= newIndex || m_vecListRegChannel.size() <= oldIndex)
        return;
    
    if (oldIndex > newIndex)
        std::rotate(m_vecListRegChannel.rend() - oldIndex - 1, m_vecListRegChannel.rend() - oldIndex, m_vecListRegChannel.rend() - newIndex);
    else
        std::rotate(m_vecListRegChannel.begin() + oldIndex, m_vecListRegChannel.begin() + oldIndex + 1, m_vecListRegChannel.begin() + newIndex + 1);
}

int  AFAuthManager::IsRegisterChannel(const char* uuid)
{
    int nowIndx = -1;
    for (int i = 0; i < m_vecListRegChannel.size(); ++i)
    {
        if (!m_vecListRegChannel[i]->pAuthData->strUuid.empty() &&
            strcmp(uuid, m_vecListRegChannel[i]->pAuthData->strUuid.c_str()) == 0) {
            nowIndx = i;
            break;
        }
    }

    return nowIndx;
}

void AFAuthManager::RegisterChannel(const char* uuid, AFChannelData* pChannel)
{
    if (pChannel == nullptr)
        return;
    
    auto itterfound = m_mapCacheAuth.find(uuid);
    if (itterfound == m_mapCacheAuth.end())
    {
        if (m_CacheMainGlobalSoop)
        {
            if (m_CacheMainGlobalSoop->strPlatform == "SOOP Global")
            {
                pChannel->pAuthData = m_CacheMainGlobalSoop;

                m_bSoopGlobalRegistered = true;
                m_ChannelMainGlobalSoop = pChannel;
            }
        }

        return;
    }

    AFBasicAuth* pData = m_vecCacheAuth[itterfound->second];
    pData->strUuid = uuid;
    pData->iViewChannelIndx = m_vecListRegChannel.size();
    pChannel->pAuthData = pData;

    if (pData->strPlatform == "afreecaTV")
        m_bSoopRegistered = true;
    else if (pData->strPlatform == "Twitch")
        m_bTwitchRegistered = true;
    else if (pData->strPlatform == "Youtube")
        m_bYoutubeRegistered = true;

    m_vecListRegChannel.emplace_back(pChannel);
}

void AFAuthManager::RemoveChannel(int indx)
{
    if (m_vecListRegChannel.size() < indx)
        return;
    
    AFChannelData* channelData = m_vecListRegChannel[indx];
    AFBasicAuth* rawAuth = channelData->pAuthData;
    
    std::string& strUuid = channelData->pAuthData->strUuid;
    auto itterfound = m_mapCacheAuth.find(strUuid);
    if (itterfound != m_mapCacheAuth.end())
    {
        m_vecCacheAuth.erase(m_vecCacheAuth.begin() + itterfound->second);
        m_IndxLastCache--;
    }
    
    m_mapCacheAuth.erase(strUuid);
    m_vecListRegChannel.erase(m_vecListRegChannel.begin() + indx);
    
    int nowIndx = 0;
    for (auto& node : m_vecListRegChannel)
    {
        AFBasicAuth* tmpRawAuth = node->pAuthData;
        tmpRawAuth->iViewChannelIndx = nowIndx;
        nowIndx++;
    }
    
    if (channelData->pObjQtPixmap != nullptr)
        m_vecDeferredDeleteQtPixmap.push_back(channelData->pObjQtPixmap);
    
    delete rawAuth;
    delete channelData;
}

void AFAuthManager::RemoveMainChannel()
{
    if (m_CacheMainGlobalSoop)
    {
        m_CacheMainGlobalSoop = nullptr;
    }

    if (m_ChannelMainGlobalSoop)
    {
        if (m_ChannelMainGlobalSoop->pObjQtPixmap != nullptr)
            m_vecDeferredDeleteQtPixmap.push_back(m_ChannelMainGlobalSoop->pObjQtPixmap);
        
        delete m_ChannelMainGlobalSoop;
        m_ChannelMainGlobalSoop = nullptr;
    }

    m_bSoopGlobalRegistered = false;
}

bool AFAuthManager::CacheMain(AFBasicAuth cacheAuth)
{
    return false;
}

void AFAuthManager::FlushAuthMain()
{

}

std::string AFAuthManager::CreateUrlProfileImg(AFChannelData* pChannelData)
{
    std::string resUrl;
    resUrl.clear();
    
    
    if (pChannelData == nullptr)
        return resUrl;
    
    AFBasicAuth* authData = pChannelData->pAuthData;
    
    if (authData == nullptr)
        return resUrl;

    if (authData->strPlatform == "SOOP Global")
        resUrl = _APIProfileImgSoopGlobal(authData);
    else if (authData->strPlatform ==  "afreecaTV")
    {
        if (pChannelData->strImgUrlUserThumb != "")
            resUrl = pChannelData->strImgUrlUserThumb;
        else
            resUrl = _APIProfileImgSoop(authData);
    }
    else if (authData->strPlatform ==  "Twitch")
        resUrl = _APIProfileImgTwitch(authData);
    else  if (authData->strPlatform == "Youtube")
        resUrl = _APIProfileImgYoutube(authData);

    
    return resUrl;
}

bool AFAuthManager::GetChannelData(int viewIndx, AFChannelData*& outRefDataPointer)
{
    if (m_vecListRegChannel.empty() ||
        viewIndx > m_vecListRegChannel.size())
        return false;
    
    outRefDataPointer =  m_vecListRegChannel[viewIndx];
    
    return true;
}

bool AFAuthManager::GetChannelData(void* pObjOBSService, AFChannelData*& outRefDataPointer)
{
    bool result = false;
    for (int i = 0; i < m_vecListRegChannel.size(); ++i)
    {
        if (m_vecListRegChannel[i]->pObjOBSService == pObjOBSService) {
            outRefDataPointer = m_vecListRegChannel[i];
            result = true;
            break;
        }
    }

    return result;
}

bool AFAuthManager::GetMainChannelData(AFChannelData*& outRefDataPointer)
{
    bool result = false;
    if (IsSoopGlobalRegistered())
    {
        result = true;
        outRefDataPointer = m_ChannelMainGlobalSoop;
    }

    return result;
}

bool AFAuthManager::FindRTMPKey(const char* channelID, OUT std::string& key)
{
    return false;
}

bool AFAuthManager::FindRTMPUrl(const char* channelID, OUT std::string& url)
{
    return false;
}

bool AFAuthManager::GetRTMPKeyGlobalSoopMainAuth(std::string& key)
{
    return false;
}

bool AFAuthManager::GetRTMPUrlGlobalSoopMainAuth(std::string& url)
{
    return false;
}

bool AFAuthManager::IsAuthed(const char* uuid)
{
    for (auto& node : m_vecListRegChannel) {
        if (0 == strcmp(uuid, node->pAuthData->strUuid.c_str()))
            return true;
    }

    return false;
}

bool AFAuthManager::IsCachedAuth(const char* uuid)
{
    return (m_mapCacheAuth.count(uuid));
}

std::vector<void*>*  AFAuthManager::GetContainerDeferrdDelObj()
{
    return &m_vecDeferredDeleteQtPixmap;
}

std::string AFAuthManager::_APIProfileImgSoopGlobal(AFBasicAuth* pAuthedData)
{
    std::string resUrl;
    resUrl.clear();
    
    
    AFAuth::Def rawAuthDef = { "SOOP Global",
                                AFAuth::Type::OAuth_StreamKey,
                                true, true };
    
    SoopGlobalAuth tmpSoopGlobalAuth(rawAuthDef, nullptr);
    AFOAuth* rawAuth = reinterpret_cast<AFOAuth*>(&tmpSoopGlobalAuth);
    rawAuth->ConnectAuthedAFBase(pAuthedData, true);
    
    resUrl = tmpSoopGlobalAuth.GetUrlProfileImg();
    
    
    return resUrl;
}

std::string AFAuthManager::_APIProfileImgSoop(AFBasicAuth* pAuthedData)
{
    std::string resUrl;
    resUrl.clear();
    
    
    AFAuth::Def rawAuthDef = { "afreecaTV",
                                AFAuth::Type::OAuth_StreamKey,
                                true, true };
    
    SoopAuth tmpSoopAuth(rawAuthDef, nullptr);
    AFOAuth* rawAuth = reinterpret_cast<AFOAuth*>(&tmpSoopAuth);
    rawAuth->ConnectAuthedAFBase(pAuthedData);
    
    resUrl = tmpSoopAuth.GetUrlProfileImg();
    
    
    return resUrl;
}

std::string AFAuthManager::_APIProfileImgTwitch(AFBasicAuth* pAuthedData)
{
    std::string resUrl;
    resUrl.clear();
    
    
    AFAuth::Def rawAuthDef = { "Twitch",
                                AFAuth::Type::OAuth_StreamKey,
                                true, true };
    
    TwitchAuth tmpTwitchAuth(rawAuthDef, nullptr);
    AFOAuth* rawAuth = reinterpret_cast<AFOAuth*>(&tmpTwitchAuth);
    rawAuth->ConnectAuthedAFBase(pAuthedData);
    
    resUrl = tmpTwitchAuth.GetUrlProfileImg();
    
    
    return resUrl;
}

std::string AFAuthManager::_APIProfileImgYoutube(AFBasicAuth* pAuthedData)
{
    std::string resUrl;
    resUrl.clear();
    
    
    AFAuth::Def rawAuthDef = { "YouTube - RTMP",
                                AFAuth::Type::OAuth_LinkedAccount,
                                true, true };
    
    YoutubeApiWrappers tmpApiYoutube(rawAuthDef);
    AFOAuth* rawAuth = reinterpret_cast<AFOAuth*>(&tmpApiYoutube);
    rawAuth->ConnectAuthedAFBase(pAuthedData);
    
    resUrl = tmpApiYoutube.GetUrlProfileImg();
 
    
    return resUrl;
}


std::string AFAuthManager::_GetPathSaveFile()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    char savePath[512];
    int ret = confManager.GetConfigPath(savePath, 512, "SOOPStudio/channelData");
    if (ret <= 0)
        return std::string();
    
    if(os_file_exists(savePath) == false)
        os_mkdir(savePath);
    
    std::string saveFile = savePath;
    saveFile += "/data.dat";
    
    return saveFile;
}

std::string AFAuthManager::_ToString(uint64_t value)
{
    std::ostringstream oss; oss << value; return oss.str();
}
    
json11::Json AFAuthManager::_AuthObjToJson(AFBasicAuth* pAuth, AFChannelData* pChannel)
{
    return json11::Json::object {
       { "type", static_cast<int>(pAuth->eType) },
       { "channelID", pAuth->strChannelID },
       { "channelNick", pAuth->strChannelNick },
       { "accessToken", pAuth->strAccessToken },
       { "refreshToken", pAuth->strRefreshToken },
       { "expireTime", _ToString(pAuth->uiExpireTime) },
       { "serverUrl", pAuth->strUrlRTMP },
       { "streamKey", pAuth->strKeyRTMP },
       { "customID", pAuth->strCustomID },
       { "customPassword", pAuth->strCustomPassword },
        {"platform", pAuth->strPlatform},
        {"uuid", pAuth->strUuid},
        { "channelData", json11::Json::object {
            { "IndxViewList", pAuth->iViewChannelIndx },
            { "nowStreaming", pChannel->bIsStreaming },
            { "profileImgUrl", pChannel->strImgUrlUserThumb }
        }
       }};
}

AFBasicAuth AFAuthManager::_JsonObjToAuth(json11::Json& item)
{
    AFBasicAuth res;
    
    res.eType = static_cast<AuthType>(item["type"].int_value());
    res.strChannelID = item["channelID"].string_value();
    res.strChannelNick = item["channelNick"].string_value();
    res.strAccessToken = item["accessToken"].string_value();
    res.strRefreshToken = item["refreshToken"].string_value();
    res.uiExpireTime = std::stoull(item["expireTime"].string_value());
    res.strUrlRTMP = item["serverUrl"].string_value();
    res.strKeyRTMP = item["streamKey"].string_value();
    res.strCustomID = item["customID"].string_value();
    res.strCustomPassword = item["customPassword"].string_value();
    res.strPlatform = item["platform"].string_value();
    res.strUuid = item["uuid"].string_value();
    
    return res;
}
    
bool AFAuthManager::_CheckInvalidAuth(AFBasicAuth& auth)
{
    return false;
}
