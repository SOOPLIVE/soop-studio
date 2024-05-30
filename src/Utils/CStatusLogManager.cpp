#include "CStatusLogManager.h"

#include <future>

#include <QCoreApplication>
#include <QHostAddress>
#include <QHostInfo>

#include "ViewModel/Auth/COAuthLogin.hpp"

#include "CoreModel/Config/CConfigManager.h"

void AFStatusLogManager::SendLogProgramStart(bool bStart)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    const char* installLang =
    config_get_string(
        confManager.GetGlobal(), "General", "LanguageBase");

    const char* moduleUUID =
        config_get_string(
            confManager.GetGlobal(), "General", "ModuleUUID");

    QString localhostname = QHostInfo::localHostName();
    QString localhostIP;
    QList<QHostAddress> hostList = QHostInfo::fromName(localhostname).addresses();
    foreach(const QHostAddress & address, hostList) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address.isLoopback() == false) {
            localhostIP = address.toString();
        }
    }
    
    
    // witout Channel Data
    json11::Json out = json11::Json::object{
        { "type", bStart ? "on" : "off" },
        { "local_ip", localhostIP.toStdString()},
        { "language", installLang ? installLang : ""},
        { "uuid", moduleUUID }
    };
    
    std::async(std::launch::async, [this, out](){_SendLog(out.dump());});
    //
    

    auto& authManager = AFAuthManager::GetSingletonInstance();
    
    int cntList = authManager.GetCntChannel();
    for (int idx = 0; idx < cntList; idx++) {
        json11::Json::array chList;
        AFChannelData* tmpChannelNode = nullptr;
        authManager.GetChannelData(idx, tmpChannelNode);
        if (tmpChannelNode) {
            chList.push_back(_ChannelObjToJson(tmpChannelNode));
        }
        
        _SendLogChannelData(chList, bStart ? "init" : "exit",
                            localhostIP.toStdString(),
                            installLang ? installLang : "",
                            moduleUUID);
    }
    
    AFChannelData* mainChannelData = nullptr;
    authManager.GetMainChannelData(mainChannelData);
    
    if (mainChannelData != nullptr)
    {
        json11::Json::array chList;
        chList.push_back(_ChannelObjToJson(mainChannelData));
        
        _SendLogChannelData(chList, bStart ? "init" : "exit",
                            localhostIP.toStdString(),
                            installLang ? installLang : "",
                            moduleUUID);
    }
}

void AFStatusLogManager::SendLogStartBroad(bool bStart)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();

    const char* installLang =
        config_get_string(
            confManager.GetGlobal(), "General", "LanguageBase");

    const char* moduleUUID =
        config_get_string(
            confManager.GetGlobal(), "General", "ModuleUUID");

    QString localhostname = QHostInfo::localHostName();
    QString localhostIP;
    QList<QHostAddress> hostList = QHostInfo::fromName(localhostname).addresses();
    foreach(const QHostAddress & address, hostList) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address.isLoopback() == false) {
            localhostIP = address.toString();
        }
    }
    
    
    int cntOnBroad = 0;
    int cntOffBroad = 0;

    auto& authManager = AFAuthManager::GetSingletonInstance();

    int cntList = authManager.GetCntChannel();
    for (int idx = 0; idx < cntList; idx++) {
        json11::Json::array chList;
        AFChannelData* tmpChannelNode = nullptr;
        authManager.GetChannelData(idx, tmpChannelNode);
        if (tmpChannelNode) {
            chList.push_back(_ChannelObjToJson(tmpChannelNode));
            if (tmpChannelNode->bIsStreaming)
                cntOnBroad++;
            else
                cntOffBroad++;
        }
        
        _SendLogChannelData(chList,
                            bStart ? "startbroad" : "stopbroad",
                            localhostIP.toStdString(),
                            installLang ? installLang : "",
                            moduleUUID);
    }
    
    AFChannelData* mainChannelData = nullptr;
    authManager.GetMainChannelData(mainChannelData);
    
    if (mainChannelData != nullptr)
    {
        if (mainChannelData->bIsStreaming)
            cntOnBroad++;
        else
            cntOffBroad++;
        
        json11::Json::array chList;
        chList.push_back(_ChannelObjToJson(mainChannelData));
        
        _SendLogChannelData(chList,
                            bStart ? "startbroad" : "stopbroad",
                            localhostIP.toStdString(),
                            installLang ? installLang : "",
                            moduleUUID);
    }
    
    
    // witout Channel Data
    json11::Json out = json11::Json::object{
                                { "countOn", cntOnBroad },
                                { "countOff", cntOffBroad },
                                { "countTotal", cntOnBroad + cntOffBroad },
                                { "type", bStart ? "cntstartbroad" : "cntstopbroad" },
                                { "local_ip", localhostIP.toStdString()},
                                { "language", installLang ? installLang : ""},
                                { "uuid", moduleUUID }
    };
    
    std::async(std::launch::async, [this, out](){_SendLog(out.dump());});
    //
}

json11::Json AFStatusLogManager::_ChannelObjToJson(AFChannelData* pChannel)
{
    return json11::Json::object{
           { "channelID", pChannel->pAuthData->strChannelID },
           { "channelNick", pChannel->pAuthData->strChannelNick },
           { "serverUrl", pChannel->pAuthData->strUrlRTMP },
           //{ "streamKey", pChannel->pAuthData->strKeyRTMP },
            { "platform", pChannel->pAuthData->strPlatform },
            { "streaming", pChannel->bIsStreaming }
    };
}

void AFStatusLogManager::_SendLogChannelData(json11::Json::array chList, const char* logType,
                                             std::string localIP,
                                             const char* installLang,
                                             const char* uuid)
{
    json11::Json out = json11::Json::object{
        { "channel", chList },
        { "type", logType },
        { "local_ip", localIP},
        { "language", installLang},
        { "uuid", uuid }
    };

    std::async(std::launch::async, [this, out](){_SendLog(out.dump());});
}


bool AFStatusLogManager::_SendLog(const std::string& sendLogMessage)
{
    QString logAPIUrl = "";

    return false;
        
    std::vector<std::string> headers;
    std::string body;
    std::string resOutput;
    std::string resError;
    std::string strURL = logAPIUrl.toStdString();
    return GetRemoteFile(strURL.c_str(), resOutput, resError, nullptr,
                         "application/json", "POST",
                         sendLogMessage.c_str(),
                         headers, nullptr, 10, true, 0);
}
