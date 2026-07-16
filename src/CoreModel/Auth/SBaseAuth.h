#pragma once

#include <string>

// GetRemoteFile
#include "Common/CommonUtils.h"

enum class AuthType
{
    None = 0,
    No_OAuth_RTMP,
    OAuth_StreamKey,
    OAuth_LinkedAccount,
};

struct AFBasicAuth
{
public:
    AuthType        type;
    std::string     channelID;
    std::string     channelNick;
    std::string     accessToken;
    std::string     refreshToken;
    std::string     customID;
    std::string     customPassword;
    std::string     cookie; //SOOP 
    uint64_t        expireTime;
    std::string     urlRTMP;
    std::string     keyRTMP;
    std::string     platform;
    std::string     uuid;
    bool            checkedRTMPKey;
    int             viewChannelIndx;
    bool            loginRetain;
    bool            saveId;


    
    AFBasicAuth()
        :type(AuthType::None),
        channelID(""),
        channelNick(""),
        accessToken(""),
        refreshToken(""),
        customID(""),
        customPassword(""),
        cookie(""),
        expireTime(0),
        urlRTMP(""),
        keyRTMP(""),
        platform(""),
        uuid(""),
        checkedRTMPKey(false),
        viewChannelIndx(-1),
        loginRetain(false),
        saveId(false) {};
};

struct AFChannelData
{
public:
    AFBasicAuth*    pAuthData;
    std::string     imgUrlUserThumb;
    void*           pObjQtPixmap;
    bool            isStreaming;
    void*           pObjOBSService;

    
    AFChannelData()
    :pAuthData(nullptr),
    imgUrlUserThumb(""),
    pObjQtPixmap(nullptr),
    isStreaming(false),
    pObjOBSService(nullptr){};
};

