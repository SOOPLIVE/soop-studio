
#ifndef AFCSTATUSLOGMANAGER_H
#define AFCSTATUSLOGMANAGER_H

#include <json11.hpp>
#include "CoreModel/Auth/CAuthManager.h"

class AFStatusLogManager final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFStatusLogManager& GetSingletonInstance()
    {
        static AFStatusLogManager* instance = nullptr;
        if (instance == nullptr)
            instance = new AFStatusLogManager;

        return *instance;
    };
    ~AFStatusLogManager();
private:
    // singleton constructor
    AFStatusLogManager() = default;
    AFStatusLogManager(const AFStatusLogManager&) = delete;
    AFStatusLogManager(/* rValue */AFStatusLogManager&& other) noexcept = delete;
    AFStatusLogManager& operator=(const AFStatusLogManager&) = delete;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void SendLogProgramStart(bool bStart = true);
    void SendLogStartBroad(bool bStart = true);
#pragma endregion public func

#pragma region private func
private:
    void _SendLogChannelData(json11::Json::array chList, const char* logType,
                             std::string localIP,
                             const char* installLang,
                             const char* uuid);
    
    bool _SendLog(const std::string& sendLogMessage);
    json11::Json _ChannelObjToJson(AFChannelData* pChannel);
#pragma endregion private func
};

#endif // AFCSTATUSLOGMANAGER_H
