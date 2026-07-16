#include "CAuthManager.h"

#include <iostream>
#include <sstream>
#include <list>
#include <tuple>
#include <regex>

#include <util/platform.h>
#include <nlohmann/json.hpp>

#include "MainFrame/CMainFrame.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "ViewModel/Auth/COAuthLogin.hpp"
#include "ViewModel/Auth/Soop/auth-soop.hpp"
#include "ViewModel/Auth/Twitch/auth-twitch.h"
#include "ViewModel/Auth/YouTube/auth-youtube.hpp"
#include "ViewModel/Auth/YouTube/youtube-api-wrappers.hpp"

#define KEY ""

void AFAuthManager::LoadAllAuthed()
{
    std::string savedFileName = _GetPathSaveFile();

    FILE* f = os_fopen(savedFileName.c_str(), "rb");
    if (!f)
        return;

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);   

    char* savedData = new char[fileSize];

    size_t bytesRead = fread(savedData, sizeof(char), fileSize, f);
    fclose(f);
    f = NULL;

    if (bytesRead == 0)
    {
        delete[] savedData;
        fclose(f);
        return;
    }
}

void AFAuthManager::SaveAllAuthed()
{
}

void AFAuthManager::ClearCache()
{
    m_cacheAuthMap.clear();
    m_cacheAuths.clear();
    m_indxLastCache = -1;
}

bool AFAuthManager::CacheAuth(bool main, AFBasicAuth cacheAuth)
{
    AFBasicAuth* tmp = new AFBasicAuth(cacheAuth);
    if (cacheAuth.uuid.empty() || cacheAuth.uuid == "")
        return false;
    
    if (main)
    {
        m_pCacheMainSoop = tmp;
    }
    else
    {
        m_indxLastCache++;
        m_cacheAuthMap.insert({tmp->uuid, m_indxLastCache});
        m_cacheAuths.emplace_back(tmp);
    }
    
    return true;
}

bool AFAuthManager::CacheAuth(bool main, AFBasicAuth* cacheAuth)
{
    if (cacheAuth->uuid.empty() || cacheAuth->uuid == "")
        return false;

    if (main)
    {
        m_pCacheMainSoop = cacheAuth;
    }
    else
    {
        m_indxLastCache++;
        m_cacheAuthMap.insert({ cacheAuth->uuid, m_indxLastCache });
        m_cacheAuths.emplace_back(cacheAuth);
    }

    return true;
}

void AFAuthManager::GetUuidFromCache(std::string platform, std::string& outUuid)
{
    for (auto& cacheAuth : m_cacheAuths)
    {
        if (cacheAuth->platform == platform) {
            outUuid = cacheAuth->uuid;
            break;
        }
    }
}

void AFAuthManager::RemoveCachedAuth(const char* uuid)
{
    auto itterfound = m_cacheAuthMap.find(uuid);
    if (itterfound != m_cacheAuthMap.end())
    {
        m_cacheAuths.erase(m_cacheAuths.begin() + itterfound->second);
        m_indxLastCache--;

        int foundIndx = itterfound->second;

        m_cacheAuthMap.erase(itterfound);

        for (auto& node : m_cacheAuthMap) {
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
    if (m_listRegChannel.size() <= newIndex || m_listRegChannel.size() <= oldIndex)
        return;
    
    if (oldIndex > newIndex)
        std::rotate(m_listRegChannel.rend() - oldIndex - 1, m_listRegChannel.rend() - oldIndex, m_listRegChannel.rend() - newIndex);
    else
        std::rotate(m_listRegChannel.begin() + oldIndex, m_listRegChannel.begin() + oldIndex + 1, m_listRegChannel.begin() + newIndex + 1);
}

int  AFAuthManager::IsRegisterChannel(const char* uuid)
{
    int nowIndx = -1;
    for (int i = 0; i < m_listRegChannel.size(); ++i)
    {
        if (!m_listRegChannel[i]->pAuthData->uuid.empty() &&
            strcmp(uuid, m_listRegChannel[i]->pAuthData->uuid.c_str()) == 0) {
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
    
    auto itterfound = m_cacheAuthMap.find(uuid);

    if (itterfound == m_cacheAuthMap.end())
    {
        if (m_pCacheMainSoop) 
        {
            bool isMain = false;
            if (m_pCacheMainSoop->platform == PLATFORM_SOOP) {
                isMain = true;
                m_soopRegistered = true;
            }

            if (isMain) {
                pChannel->pAuthData = m_pCacheMainSoop;
                m_pChannelMainSoop = pChannel;
            }
        }

        return;
    }


    AFBasicAuth* pData = m_cacheAuths[itterfound->second];
    pData->uuid = uuid;
    pData->viewChannelIndx = m_listRegChannel.size();
    pChannel->pAuthData = pData;

    if (pData->platform == PLATFORM_TWITCH)
        m_twitchRegistered = true;
    else if (pData->platform == PLATFORM_YOUTUBE)
        m_youtubeRegistered = true;

    m_listRegChannel.emplace_back(pChannel);
}

void AFAuthManager::RemoveAllChannel(bool removeMain)
{
    if(removeMain)
        RemoveMainChannel();

    m_listRegChannel.clear();

    m_twitchRegistered = false;
    m_youtubeRegistered = false;
    m_soopRegistered = false;

}

void AFAuthManager::RemoveChannel(int indx)
{
    if (m_listRegChannel.size() < indx)
        return;
    
    AFChannelData* channelData = m_listRegChannel[indx];
    AFBasicAuth* rawAuth = channelData->pAuthData;
    
    std::string& strUuid = channelData->pAuthData->uuid;
    auto itterfound = m_cacheAuthMap.find(strUuid);
    if (itterfound != m_cacheAuthMap.end())
    {
        m_cacheAuths.erase(m_cacheAuths.begin() + itterfound->second);
        m_indxLastCache--;
    }
    
    m_cacheAuthMap.erase(strUuid);
    m_listRegChannel.erase(m_listRegChannel.begin() + indx);
    
    int nowIndx = 0;
    for (auto& node : m_listRegChannel)
    {
        AFBasicAuth* tmpRawAuth = node->pAuthData;
        tmpRawAuth->viewChannelIndx = nowIndx;
        nowIndx++;
    }
    
    if (channelData->pObjQtPixmap != nullptr)
        m_deferredDeleteQtPixmap.push_back(channelData->pObjQtPixmap);
    
    delete rawAuth;
    delete channelData;
}

void AFAuthManager::RemoveMainChannel()
{
    if (m_pChannelMainSoop)
    {
        if (m_pChannelMainSoop->pObjQtPixmap != nullptr)
            m_deferredDeleteQtPixmap.push_back(m_pChannelMainSoop->pObjQtPixmap);
        
        delete m_pChannelMainSoop;
        m_pChannelMainSoop = nullptr;
    }

    m_soopRegistered = false;
}

void AFAuthManager::RemoveChannel(std::string platform)
{
    if (platform == PLATFORM_SOOP)
        return;

    for (auto it = m_listRegChannel.begin(); it != m_listRegChannel.end(); ++it) {
        if ((*it)->pAuthData->platform == platform) {

            std::string uuid = (*it)->pAuthData->uuid;

            delete* it;
            m_listRegChannel.erase(it);

            RemoveCachedAuth(uuid.c_str());

            if (platform == PLATFORM_TWITCH)
                m_twitchRegistered = false;
            else if (platform == PLATFORM_YOUTUBE)
                m_youtubeRegistered = false;
            break;
        }
    }
}

void AFAuthManager::RemoveChannel(AFChannelData* data)
{
    if (!data)
        return;

    std::string dataUuid = data->pAuthData->uuid;
    std::string platform = data->pAuthData->platform;

    if (platform == PLATFORM_SOOP)
        return;

    for (auto it = m_listRegChannel.begin(); it != m_listRegChannel.end(); ++it) {
        if ((*it)->pAuthData->uuid == dataUuid) {
            m_listRegChannel.erase(it);
            break;
        }
    }

    if (platform == PLATFORM_TWITCH)
        m_twitchRegistered = false;
    else if (platform == PLATFORM_YOUTUBE)
        m_youtubeRegistered = false;

    if (data->pObjQtPixmap)
        delete data->pObjQtPixmap;
    if (data->pAuthData)
        delete data->pAuthData;
    delete data;
}

bool AFAuthManager::CacheMain(AFBasicAuth cacheAuth)
{
    return false;
}

bool AFAuthManager::SetSoopCookie(std::string cookie)
{
    if (cookie.empty())
        return false;

    return false;
}

std::string AFAuthManager::SoopCookie()
{
    std::string retVal = "";

    AFChannelData* data = nullptr;
    if (GetChannelData(PLATFORM_SOOP, data))
        retVal = data->pAuthData->cookie;

    return retVal;
}

bool AFAuthManager::RefreshCookie()
{
    return true;
}

bool AFAuthManager::VodSaveAvailableFromAPI()
{
    AFAuth::Def rawAuthDef = { PLATFORM_SOOP, AFAuth::Type::OAuth_StreamKey, true, true };
    SoopAuth tmpSoopAuth(rawAuthDef, nullptr);
    std::string vodCheck = tmpSoopAuth.VodSaveAvailable(SoopCookie());

    qDebug() << QString::fromStdString(vodCheck);

    std::string err;
    err.clear();

    return false;
}

bool AFAuthManager::VodRequestVodSave(std::string title, std::string hashtag)
{
    AFAuth::Def rawAuthDef = { PLATFORM_SOOP, AFAuth::Type::OAuth_StreamKey, true, true };
    SoopAuth tmpSoopAuth(rawAuthDef, nullptr);
    std::string vodRequest = tmpSoopAuth.VodSaveRequest(SoopCookie(), title, hashtag);

    std::string err;
    err.clear();

    return false;
}

bool AFAuthManager::RequestBroadInfoAPI(bool initStudio)
{
    if (!m_pSoopBroadInfo) {
        blog(LOG_ERROR, "broadinfo ptr is null");
        return false;
    }

    if (SoopCookie() == "")
        return false;

    int initStudio_ = (initStudio ? 1 : 0);
    QList<int> additionalData = { initStudio_ };
    SOOP_API_HANDLER->getAPIfromId(GET_BROADINFO, {},
        m_pSoopBroadInfo, "qslotResponseBroadInfoAPI", additionalData);

    return true;
}

bool AFAuthManager::RequestStickerItemInfo()
{
    if (SoopCookie() == "")
        return false;

    m_pSoopBroadInfo->RequestStickerItemInfo();
    return true;
}

void AFAuthManager::SendSoopBroadInfoSetting(bool sendSubscribe)
{
    std::string cookie = SoopCookie();
    if (cookie == "")
        return;

    m_pSoopBroadInfo->SendBroadInfoSetting(sendSubscribe);
}

std::string AFAuthManager::MergeHashTag()
{
    return m_pSoopBroadInfo->MergeTag(m_pSoopBroadInfo->HashTags());
}

void AFAuthManager::SendCheckBroadStart(QObject * receiver, const char* slot)
{
    QList<QVariant> values = {  };

    int subTier = m_pSoopBroadInfo->SubscribeBroad();
    values.append(QString::number(subTier));
    QString catNum = QString::fromStdString(m_pSoopBroadInfo->CategoryNumberString(m_pSoopBroadInfo->CategoryNumber()));
    values.append(catNum);

    SOOP_API_HANDLER->postAPIfromId(POST_BROAD_START, values, receiver, slot);
}

void AFAuthManager::SendCheckBroading(QObject* receiver, const char* slot)
{
    SOOP_API_HANDLER->getAPIfromId(GET_KR_BROADING_STATUS, QList<QVariant>{}, receiver, slot);
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

    if (authData->platform == PLATFORM_SOOP)
    {
        if (pChannelData->imgUrlUserThumb != "")
            resUrl = pChannelData->imgUrlUserThumb;
        else
        {
            LoadSoopStreamerInfo();
            resUrl = "";
        }
    }
    else if (authData->platform == PLATFORM_TWITCH)
        resUrl = _APIProfileImgTwitch(authData);
    else if (authData->platform == PLATFORM_YOUTUBE)
        resUrl = _APIProfileImgYoutube(authData);

    
    return resUrl;
}

bool AFAuthManager::GetChannelID(const char* platform, std::string& channelID)
{
    AFChannelData* channel = nullptr;
    GetChannelData(platform, channel);
    if(!channel)
        return false;

    channelID = channel->pAuthData->channelID;
    return true;
}

bool AFAuthManager::GetChannelData(int viewIndx, AFChannelData*& outRefDataPointer)
{
    if (m_listRegChannel.empty() ||
        viewIndx > m_listRegChannel.size())
        return false;
    
    outRefDataPointer =  m_listRegChannel[viewIndx];
    
    return true;
}

bool AFAuthManager::GetChannelData(void* pObjOBSService, AFChannelData*& outRefDataPointer)
{
    bool result = false;
    for (int i = 0; i < m_listRegChannel.size(); ++i)
    {
        if (m_listRegChannel[i]->pObjOBSService == pObjOBSService) {
            outRefDataPointer = m_listRegChannel[i];
            result = true;
            break;
        }
    }

    return result;
}

bool AFAuthManager::GetChannelData(std::string platform, AFChannelData*& outRefDataPointer)
{
    bool result = false;

    if (m_pChannelMainSoop && m_pChannelMainSoop->pAuthData->platform == platform)
    {
        outRefDataPointer = m_pChannelMainSoop;
        return true;
    }

    for (int i = 0; i < m_listRegChannel.size(); ++i)
    {
        if (m_listRegChannel[i]->pAuthData->platform == platform) {
            outRefDataPointer = m_listRegChannel[i];
            result = true;
            break;
        }
    }

    return result;
}

bool AFAuthManager::GetMainChannelData(AFChannelData*& outRefDataPointer)
{
    if (IsSoopRegistered() && m_pChannelMainSoop)
    {
        outRefDataPointer = m_pChannelMainSoop;
        return true;
    }
    return false;
}

bool AFAuthManager::IsAuthed(const char* uuid)
{
    for (auto& node : m_listRegChannel) {
        if (0 == strcmp(uuid, node->pAuthData->uuid.c_str()))
            return true;
    }

    return false;
}

bool AFAuthManager::IsCachedAuth(const char* uuid)
{
    return (m_cacheAuthMap.count(uuid));
}

std::vector<void*>*  AFAuthManager::GetContainerDeferrdDelObj()
{
    return &m_deferredDeleteQtPixmap;
}

void AFAuthManager::InitSoopBroadInfo()
{
    m_pSoopBroadInfo = new AFQBroadInfo();
}


void AFAuthManager::DeleteSoopBroadInfo()
{
    if (m_pSoopBroadInfo)
    {
        delete m_pSoopBroadInfo;
        m_pSoopBroadInfo = nullptr;
    }
}

bool AFAuthManager::LoadSoopBroadInfo()
{
    m_pSoopBroadInfo->ClearHashTags();

    return RequestBroadInfoAPI();
}
bool AFAuthManager::LoadSoopBroadInfo(const char* json_data)
{
    if(nullptr == json_data ||
       0 == strlen(json_data))
        return false;

    bool success = false;
    do {

        success = true;
    } while(false);
    return success;
}

void AFAuthManager::LoadSoopStreamerInfo()
{
    m_pSoopBroadInfo->ReceiveSoopStreamerInfo();
}

bool AFAuthManager::IsSoopStreaming()
{
    if (IsSoopRegistered()) {
        AFChannelData* channel = nullptr;
        GetChannelData(PLATFORM_SOOP, channel);
        if (channel) {
            return channel->isStreaming;
        }
    }    
    return false;
}

void AFAuthManager::SaveSoopBroadInfo()
{
}

bool AFAuthManager::GetSoopChatUrl(std::string& chaturl)
{
    AFChannelData* channel = nullptr;
    GetChannelData(PLATFORM_SOOP, channel);
    if (!channel)
        return false;

    QString qchat = QString::fromStdString(SOOP_CHAT_URL)
        .arg(QString::fromStdString(channel->pAuthData->channelID));

    chaturl = qchat.toStdString();

    return true;
}

bool AFAuthManager::GetTwitchChatUrl(std::string& moderation_tools_url, std::string& chaturl)
{
    AFChannelData* channel = nullptr;
    GetChannelData(PLATFORM_TWITCH, channel);
    if (!channel)
        return false;

    chaturl = TWITCH_POPUP_URL + channel->pAuthData->channelID + "/chat";

    moderation_tools_url = "https://www.twitch.tv/";
    moderation_tools_url += channel->pAuthData->channelID;
    moderation_tools_url += "/dashboard/settings/moderation?no-reload=true";
    return true;
}

bool AFAuthManager::GetYoutubeChatUrl(std::string chat_id, std::string api_chat_id, std::string& chatUrl)
{
    AFChannelData* channel = nullptr;
    GetChannelData(PLATFORM_YOUTUBE, channel);
    if (!channel)
        return false;

    chatUrl = YOUTUBE_CHAT_POPOUT_URL + chat_id;

    return true;
}

std::string AFAuthManager::CategoryNumberString(int category)
{
    return m_pSoopBroadInfo->CategoryNumberString(category);
}

std::string AFAuthManager::FullCategoryName(int category)
{
    std::string categoryNum = CategoryNumberString(category);
    std::string fullCategoryName;
    ReceiveCategoryString(category, fullCategoryName);

    if (!categoryNum.empty() && categoryNum.size() >= 8 && categoryNum.substr(categoryNum.size() - 4) != "0000")
    {
        categoryNum.replace(categoryNum.size() - 4, 4, "0000");

        std::string categoryParentName;
        if(ReceiveCategoryString(stoi(categoryNum), categoryParentName)) {
            fullCategoryName = categoryParentName + " > " + fullCategoryName;
        }
    }

    QString displayText = QString::fromStdString(fullCategoryName);
    if (displayText.isEmpty()) {
        displayText = "";
    }
    return fullCategoryName;
}

bool AFAuthManager::ReceiveCategoryString(int categoryNum, std::string& categoryString)
{
    return false; 
}

bool AFAuthManager::ReceiveCategoryNum(std::string categoryString, int& categoryNumString)
{
    return false;
}

void AFAuthManager::TwitchTryLoadSecondaryUIPanes()
{
    AFAuth::Def rawAuthDef = { PLATFORM_TWITCH, AFAuth::Type::OAuth_StreamKey, true, true };
    TwitchAuth tmpTwitchAuth(rawAuthDef, nullptr);
    tmpTwitchAuth.TryLoadSecondaryUIPanes();
}

QMap<std::string, std::string> AFAuthManager::GetLiveChannels()
{
    QMap<std::string, std::string> retVal;
    if (m_pChannelMainSoop->isStreaming)
        retVal.insert(m_pChannelMainSoop->pAuthData->platform, m_pChannelMainSoop->pAuthData->channelID);

    foreach(AFChannelData* data, m_listRegChannel)
        if(data->isStreaming)
            retVal.insert(data->pAuthData->platform, data->pAuthData->channelID);

    return retVal;
}

void AFAuthManager::OffLiveExceptSoop()
{
    if (m_pChannelMainSoop->pAuthData->platform != PLATFORM_SOOP)
        m_pChannelMainSoop->isStreaming = false;

    foreach(AFChannelData * data, m_listRegChannel)
    {
        if (data->pAuthData->platform != PLATFORM_SOOP)
            data->isStreaming = false;
    }
}

std::string AFAuthManager::_APIProfileImgSoop(AFBasicAuth* pAuthedData)
{
    std::string resUrl;
    resUrl.clear();
    
    
    AFAuth::Def rawAuthDef = { PLATFORM_SOOP, AFAuth::Type::OAuth_StreamKey, true, true };    
    SoopAuth tmpSoopAuth(rawAuthDef, nullptr);
    AFOAuth* rawAuth = reinterpret_cast<AFOAuth*>(&tmpSoopAuth);

    std::string access_token;
    int64_t expires_in;
    std::string refresh_token;
    if (tmpSoopAuth.RefreshAccessToken(pAuthedData->refreshToken, access_token, expires_in, refresh_token))
    {
        AFChannelData* tmpdata;
        if (GetChannelData(PLATFORM_SOOP, tmpdata))
        {
            tmpdata->pAuthData->accessToken = access_token;
            tmpdata->pAuthData->expireTime = expires_in;
            tmpdata->pAuthData->refreshToken = refresh_token;
        }
    }

    rawAuth->ConnectAuthedAFBase(pAuthedData, true);
    resUrl = tmpSoopAuth.GetUrlProfileImg();
    
    return resUrl;
}

std::string AFAuthManager::_APIProfileImgTwitch(AFBasicAuth* pAuthedData)
{
    std::string resUrl;
    resUrl.clear();
    
    
    AFAuth::Def rawAuthDef = { PLATFORM_TWITCH, AFAuth::Type::OAuth_StreamKey, true, true };    
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
    
    
    AFAuth::Def rawAuthDef = { "YouTube - RTMP", AFAuth::Type::OAuth_LinkedAccount, true, true };
    YoutubeApiWrappers tmpApiYoutube(rawAuthDef);
    AFOAuth* rawAuth = reinterpret_cast<AFOAuth*>(&tmpApiYoutube);
    rawAuth->ConnectAuthedAFBase(pAuthedData);
    
    resUrl = tmpApiYoutube.GetUrlProfileImg();
 
    
    return resUrl;
}


std::string AFAuthManager::_GetPathSaveFile()
{
    char savePath[512];
    int ret = GetAppConfigPath(savePath, 512, (LOCAL_FOLDER_NAME + "/channelData").c_str());
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
    
bool AFAuthManager::_CheckInvalidAuth(AFBasicAuth& auth)
{
    return false;
}

size_t AFAuthManager_WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string AFAuthManager_GetHtml(const char* url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, AFAuthManager_WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            blog(LOG_ERROR, "GetHtml curl error: %d", res);
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

int64_t AFAuthManager::GetServerTime()
{
    std::string jsonData = AFAuthManager_GetHtml(GETSERVTIME_URL);

    struct tm receive_tm = {};
    time_t server_tm = mktime(&receive_tm);
    time_t local_tm = time(NULL);

    return (server_tm ? server_tm : local_tm);
}

void AFAuthManager::GetAIManagerToken(QObject* receiver, const char* slot)
{
    SOOP_API_HANDLER->getAPIfromId(GET_AI_MANAGER_TOKEN, QList<QVariant>{}, receiver, slot);
}
