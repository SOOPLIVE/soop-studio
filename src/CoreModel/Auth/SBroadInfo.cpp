#include "SBroadInfo.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "Common/StudioDefine.h"

#include "Application/CApplication.h"
#include "Utils/SOOPAPIHandler.h"
#include "ViewModel/Auth/Soop/auth-soop.hpp"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "MainFrame/CMainFrame.h"

AFQBroadInfo::AFQBroadInfo(QObject* parent)
    : QObject(parent),
    m_broadInfo(std::make_unique<AFBroadInfo>())
{
    RequestUserNationAPI();

    m_broadInfo->aiManagerTime = config_get_int(USERCONFIG, "General", "AIManagerTime");

    connect(&m_pollingBroadNumTimer, &QTimer::timeout, this, &AFQBroadInfo::qslotPollingBroadNumberTimer);
}

AFQBroadInfo::~AFQBroadInfo()
{
}

void AFQBroadInfo::_qslotResponseCategorysAPI(const QByteArray& responseData)
{
    RefreshCategoryList(responseData);
}

void AFQBroadInfo::_qslotResponseUserNationAPI(const QByteArray& responseData)
{
    QString jsonString = QString::fromUtf8(responseData);
    std::string err;
}

void AFQBroadInfo::_qslotResponseStickerItemInfo(const QByteArray& responseData)
{
    m_broadInfo->itemInfo.clear();

    std::string jsonString = responseData.toStdString();
    std::string err;
}

void AFQBroadInfo::_qslotSendBroadInfoResult(const QByteArray& responseData)
{
    std::string jsonString = responseData.toStdString();
    std::string err;
    err.clear();
}

void AFQBroadInfo::_qslotSendUpResult(const QByteArray& responseData)
{
    std::string jsonString = responseData.toStdString();
    std::string err;
    err.clear();
}

void AFQBroadInfo::_qslotSendViewerResult(const QByteArray& responseData)
{
    std::string jsonString = responseData.toStdString();
    std::string err;
    err.clear();
}

void AFQBroadInfo::_qslotSendGiftResult(const QByteArray& responseData)
{
    std::string jsonString = responseData.toStdString();
    std::string err;
    err.clear();
}

void AFQBroadInfo::qslotResponseBroadInfoAPI(const QByteArray& responseData, int initStudio)
{
    int errType = 0;
    do {
        m_broadInfo->itemInfo.clear();

        std::string jsonString = responseData.toStdString();
        std::string err;

    } while(false);
    //
    if(m_broadInfo->allow1440p ||
       m_broadInfo->allowAV1) {
        emit checkResolution();
    }

    if (errType != 0) {
        emit qsignalBroadStartAPIError(errType, "");
    }
}

void AFQBroadInfo::qslotPollingBroadNumberTimer()
{
    SOOP_API_HANDLER->getAPIfromId(GET_USER_INFO, { }, this, "qslotResponseBroadNumberDataAPI");

    SOOP_API_HANDLER->getAPIfromId(GET_REMAIN_BURNNINGTEN_ITEM, { }, MAINFRAME, "qslotDummyAPI");
}

void AFQBroadInfo::qslotSendBroadInfoTimer()
{
    SetTitle(PrevTitle());
    SendBroadInfoSetting();
}

void AFQBroadInfo::qslotResponseBroadNumberDataAPI(const QByteArray& responseData)
{
    bool errorCheck = false;
    std::string err;
    int broadNum = 0;

    if (errorCheck)
    {
        m_pollingBroadNumTimer.start(1000);
        return;
    }

    if(m_pollingBroadNumTimer.isActive())
        m_pollingBroadNumTimer.stop();

    std::string channelId;
    AUTH_CONTEXT.GetChannelID(PLATFORM_SOOP, channelId);
    SetBroadNumber(broadNum);

    char path[512] = { 0, };
    GetAppConfigPath(path, sizeof(path), "SOOPStudio");
    QString fileName = QString("%1\\broad_info").arg(path);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << channelId.c_str() << "|" << broadNum;
        file.close();
    }
    QList<QVariant> values = {  };
    SOOP_API_HANDLER->getAPIfromId(GET_OPENAPI_DAMAIN, values, this, "qslotResponseOpenAPIDamainDataAPI");


    MAINFRAME->RefreshVideoBalloonSource();
}

void AFQBroadInfo::qslotResponseOpenAPIDamainDataAPI(const QByteArray& responseData)
{
    std::string err;
}


void AFQBroadInfo::_qslotReceiveStreamerInfo(const QByteArray& responseData)
{
    std::string jsonString = responseData.toStdString();
    std::string err;
    err.clear();

    QString msg = "";
    int code = 1;
}


void AFQBroadInfo::ReceiveSoopStreamerInfo()
{
    QList<QVariant> values = {  };
    SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_STREAMER_INFO, values, this, "_qslotReceiveStreamerInfo");
}

void AFQBroadInfo::ReceiveSoopStreamerInfoWithTempCookie(QObject* receiver, const char* slot, std::string cookie)
{
    QList<QVariant> values = {  };
    SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_STREAMER_INFO, values, receiver, slot, QList<int>(), cookie);
}

void AFQBroadInfo::RequestStickerItemInfo()
{
    QList<QVariant> values = {  };
    SOOP_API_HANDLER->getAPIfromId(GET_ITEMINFO, values, this, "_qslotResponseStickerItemInfo");
}

void AFQBroadInfo::RequestCategoryListAPI()
{
    QList<QVariant> values = {  };
    SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_CATEGORY_LIST, values, this, "_qslotResponseCategorysAPI");
}

void AFQBroadInfo::RequestUserNationAPI()
{
    QList<QVariant> values = {  };
    SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_BROAD_GEO_BLOCK, values, this, "_qslotResponseUserNationAPI");
}

void AFQBroadInfo::ReceiveUp()
{
    QList<QVariant> values = {  };
    SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_UP_INFO, values, this, "_qslotSendUpResult");
}

void AFQBroadInfo::ReceiveViewer()
{
    QList<QVariant> values = {  };
    SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_USER_INFO, values, this, "_qslotSendViewerResult");
}

void AFQBroadInfo::ReceiveGift()
{
    if (m_broadInfo->broadNumber == 0)
        m_giftInfo.total = "0";
    else
    {
        QList<QVariant> values = { m_broadInfo->broadNumber };
        SOOP_API_HANDLER->getAPIfromId(SOOP_API_KEY::GET_GIFT_INFO, values, this, "_qslotSendGiftResult");
    }
}

void AFQBroadInfo::SendBroadInfoSetting(bool sendSubscribe)
{
    QList<QVariant> values = {  };

    SOOP_API_KEY key = SOOP_API_KEY::POST_BROADINFO_SETTING;

    if(sendSubscribe)
        key = SOOP_API_KEY::POST_BROADINFO_SETTING_WITH_SUB;

    SOOP_API_HANDLER->postAPIfromId(key, values, this, "_qslotSendBroadInfoResult");
}


void AFQBroadInfo::BroadNumTimerAfterStart()
{
    if (m_pollingBroadNumTimer.isActive())
        m_pollingBroadNumTimer.stop();

    m_pollingBroadNumTimer.start(1000);
}

std::vector<std::string> AFQBroadInfo::SplitTag(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

std::string AFQBroadInfo::MergeTag(std::vector<std::string> vect)
{
    std::ostringstream oss;

    for (size_t i = 0; i < vect.size(); ++i) {
        oss << vect[i];
        if (i < vect.size() - 1) {
            oss << ",";
        }
    }
    return oss.str();
}


void AFQBroadInfo::SetHashTags(std::vector<std::string> tags)
{
    ClearHashTags();
    m_broadInfo->hashTags = tags;
}

//
void AFQBroadInfo::ClearHashTags()
{
    m_broadInfo->hashTags.clear();
}

bool AFQBroadInfo::SetCategory(int categoryNum)
{
    QString categoryName;
    bool retVal = ReceiveCategoryString(categoryNum, categoryName);
    if (retVal)
    {
        m_broadInfo->category = categoryNum;
        m_broadInfo->categoryName = categoryName.toStdString();
    }

    return retVal;    
}

std::string AFQBroadInfo::CategoryNumberString(int category)
{
    char buffer[9];
    std::sprintf(buffer, "%08d", category);
    std::string strCategoryNum(buffer);

    return strCategoryNum;
}

bool AFQBroadInfo::ReceiveCategoryString(int categoryNum, std::string& categoryString)
{
    if (m_categorys.contains(categoryNum))
    {
        for (const auto& pair : m_categorys) {
            categoryString = m_categorys.value(categoryNum).toStdString();
        }
        return true;
    }
    return false;
}

bool AFQBroadInfo::ReceiveCategoryString(int categoryNum, QString& categoryString, bool parentcategory /*= false*/)
{
    int cateNum = categoryNum;
    if (parentcategory) {
        if (categoryNum % 10000 != 0) {
            cateNum = (categoryNum / 10000) * 10000;
        }
    }

    if (m_categorys.contains(cateNum))
    {
        for (const auto& pair : m_categorys) {
            categoryString = m_categorys.value(cateNum);
        }
        return true;
    }
    return false;
}

bool AFQBroadInfo::ReceiveCategoryNum(std::string categoryString, int& categoryNum)
{
    categoryNum = m_categorys.key(QString::fromStdString(categoryString), -1);
    if (categoryNum != -1)
        return true;
    return false;
}

bool AFQBroadInfo::ReceiveCategoryNum(QString categoryString, int& categoryNum)
{
    categoryNum = m_categorys.key(categoryString, -1);
    if (categoryNum != -1)
        return true;
    return false;
}

void AFQBroadInfo::RefreshCategoryList(const QByteArray& responseData)
{
    m_categorys.clear();
    std::string err;
}

void AFQBroadInfo::SetPassword(std::string password)
{
    m_broadInfo->broadPwd = password;
}

void AFQBroadInfo::ResetAllViewerCount()
{
    m_viewerInfo.currentViewer = 0;
    m_viewerInfo.pcViewer = 0;
    m_viewerInfo.mobileViewer = 0;
    m_viewerInfo.relayViewer = 0;
    m_viewerInfo.relayPcViewer = 0;
    m_viewerInfo.relayMobileViewer = 0;
    m_viewerInfo.accumulateViewer = 0;
    m_viewerInfo.accumulatePcViewer = 0;
    m_viewerInfo.accumulateMobileViewer = 0;
    m_viewerInfo.relayCount = 0;
}

void AFQBroadInfo::SetAIManagerTime(int afktime)
{
    m_broadInfo->aiManagerTime = afktime;
    config_set_int(USERCONFIG, "General", "AIManagerTime", afktime);
    config_save_safe(USERCONFIG, "tmp", nullptr);
}

bool AFQBroadInfo::_ParseBroadInfoJson(json11::Json& jsonBroadInfo)
{
    if (!jsonBroadInfo.is_object()) {
        return false;
    }

    emit qsignalBroadInfoReceived(1, "");

    return true;
}

int AFQBroadInfo::getJsonObjNumber(json11::Json obj) {
    int num = 0;
    if(obj.is_string()) {
        int tmp = 0;
        if(safe_stoi(obj.string_value(), tmp))
            num = std::stoi(obj.string_value());
    } else if(obj.is_number())
        num = obj.int_value();
    return num;
};

bool AFQBroadInfo::safe_stoi(const std::string& str, int& out)
{
    try {
        size_t idx;
        out = std::stoi(str, &idx);

        return idx == str.length();
    }
    catch (const std::invalid_argument& e) {
        return false;
    }
    catch (const std::out_of_range& e) {
        return false;
    }
}