#pragma once

#include <string>

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
    AuthType        eType;
    std::string     strChannelID;
    std::string     strChannelNick;
    std::string     strAccessToken;
    std::string     strRefreshToken;
    std::string     strCustomID;
    std::string     strCustomPassword;
    uint64_t        uiExpireTime;
    std::string     strUrlRTMP;
    std::string     strKeyRTMP;
    std::string     strPlatform;
    std::string     strUuid;
    bool            bCheckedRTMPKey;
    int             iViewChannelIndx;


    
    AFBasicAuth()
        :eType(AuthType::None),
        strChannelID(""),
        strChannelNick(""),
        strAccessToken(""),
        strRefreshToken(""),
        strCustomID(""),
        strCustomPassword(""),
        uiExpireTime(0),
        strUrlRTMP(""),
        strKeyRTMP(""),
        strPlatform(""),
        strUuid(""),
        bCheckedRTMPKey(false),
        iViewChannelIndx(-1) {};
};

struct AFChannelData
{
public:
    AFBasicAuth*    pAuthData;
    std::string     strImgUrlUserThumb;
    void*           pObjQtPixmap;
    bool            bIsStreaming;
    void*           pObjOBSService;    

    
    AFChannelData()
    :pAuthData(nullptr),
    strImgUrlUserThumb(""),
    pObjQtPixmap(nullptr),
    bIsStreaming(false),
    pObjOBSService(nullptr){};
};

