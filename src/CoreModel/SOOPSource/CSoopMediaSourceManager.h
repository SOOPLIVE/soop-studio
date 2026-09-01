#pragma once

#include <vector>
#include <unordered_map>

#include <util/util.hpp>
#include <browser-panel.hpp>

#include "obs.hpp"

#include <QImage>
#include <QTimer>
#include <QPainter>

enum SOOP_VOD_TYPE {
    NONE = 0,
    ANIME = 1,
    EMPTY = 2,
    SPORT = 3,
    DRAMA = 4,
    MOVIE = 5,
    COUNT = 6,
};

struct VodContentInfo_s {
    int contentIdx;
    int categoryNum;
    int is_adult;
    QString contentTitle;
    QString contentInfo;
    QString contentImage;
    QString channelId;
};

struct VodInfo_s {
    int contentIdx;
    QString contentTitle;
    QString seasonTitle;
    QString vodTitle;
    QString duration;
    QString vodImage;
    QString hls;

    int  vodIdx;
    bool recentlyPlay;
    int  allowed_category;
    bool is_adult;
};

struct DirectBroadInfo_s {
    int idx;
    int category;
    QString title;
    QString contents;
    QString start_time;
    int status;
    QString broad_date;
    QString hash_tag;
};

struct TvLiveInfo_s {
    int cpNo;
    QString cpTitle;
};

class SOOPMediaSourceManager final : public QObject
{
    Q_OBJECT

public:
    SOOPMediaSourceManager() = default;
    ~SOOPMediaSourceManager() = default;

public:
    void InitContext();

    void SetSoopMediaSource(OBSSource source, bool removeSource = false);
    OBSSource GetSoopMediaSource();

    void SetSoopMediaSourceToolBar(QWidget* toolbar);
    void SetSoopMediaSourceProps(QDialog* props);

    void ForceStopSoopSource();
    void StopTvLiveBlind();

    // Request API
    void RequestTvLiveOneTimeUrl(int cpNo);
    void RequestTvLiveOneTimeUrlWithGeoBlock(int cpNo);
    void RequestTvLiveBlindCheck();

    void RequestVODContents(int contentType);
    void RequestVODContentList(int contentType, int contentIdx, int recentlyListLoad = 0);
    void RequestDirectBroadList();
    void RequestDirectBroadOneTimeUrl(int idx);
    void RequestDirectBroadOneTimeUrl_2(int idx);

    void RequestDirectBroadOneTimeUrlWithGeoBlock(int idx);
    // Tv Live
    int  GetTvLiveCPNo() { return m_currentCPNo; }
    bool MakeTvSourceBlindImage(const QString& BasePath,
        const QString& resultPath, const char* program, const char* blindTime);

    // VOD    
    std::vector<VodContentInfo_s>& GetSoopVodContents(SOOP_VOD_TYPE type);
    std::vector<QString>& GetSoopVodContentSeason(SOOP_VOD_TYPE type);
    std::vector<VodInfo_s>& GetSoopVodLists(SOOP_VOD_TYPE type);
    std::vector<VodInfo_s>& GetSoopVodPlayLists(SOOP_VOD_TYPE type);

    bool GetSoopVodAdultContentCheck(int category);
    bool GetSoopVodAdultContentCheck(SOOP_VOD_TYPE type, int category);

    VodInfo_s GetCurVodInfo(SOOP_VOD_TYPE type);
    VodInfo_s GetCurVodInfo(const char* vod_source_id);
    void SetCurVodInfo(SOOP_VOD_TYPE type, VodInfo_s info);
    void SetRecentlyVodInfo(SOOP_VOD_TYPE type, VodInfo_s info);
    VodInfo_s GetRecentlyVodInfo(SOOP_VOD_TYPE type);

    SOOP_VOD_TYPE GetSoopVodSourceType(QString id);
    const char* GetSoopVodSourceId(SOOP_VOD_TYPE type);

    void SetRepeatSeries(SOOP_VOD_TYPE type, bool repeat);
    void SetRepeatVodList(SOOP_VOD_TYPE type, bool repeat);

    bool GetRepeatSeries(SOOP_VOD_TYPE type);
    bool GetRepeatVodList(SOOP_VOD_TYPE type);

    void PlayVodMedia(SOOP_VOD_TYPE type, VodInfo_s info, bool selectedItem = false);
    void PlayNextVOD(SOOP_VOD_TYPE type);
    void PlayPrevVOD(SOOP_VOD_TYPE type);

    VodInfo_s FindFirstInSeason(SOOP_VOD_TYPE type, QString season);
    VodInfo_s FindLastInSeason(SOOP_VOD_TYPE type, QString season);

    // Direct Broad
    void SetDirectBroadIdx(int idx) { m_currentIdx = idx; }
    int GetDirectBroadIdx() { return m_currentIdx; }
    std::list<int>& GetDirectBroadArrowedCategorys() { return m_directBroadAllowedCategory; }

    std::vector<QString>& GetDirectBroadDateList() { return m_dates; }
    std::vector<DirectBroadInfo_s>& GetDirectBroadList() { return m_directBroadInfos; }

    std::vector<TvLiveInfo_s>& GetTvLiveLists() { return m_tvLists; };

private:
    void _InitTvLiveLists();
    void _InitVodConfigFile();
    void _ApplyVodPlayAfterGeoCheck();

signals:
    void qsignalRefreshVODContents();
    void qsignalRefreshVODSeasons(QString content, int contentNo, int recentlyListLoad);

    void qsignalRefreshDirectBroadList();
    void qsignalResponseDirectBroadOneTimeUrl(int requestIdx, QString url);

    void qsignalResponseTvLiveOneTimeUrl(int cpNo);

private slots:
    // TV Broad Source
    void _qslotTvSourceBlindTimer();
    void _qslotTvLiveStartTimer();
    void _qslotTvSourceOneTimeUrlAPIData(const QByteArray& responseData, int requestcpNo, int requestType);
    void _qslotTvLiveGeoBlockCheckAPIResponse(const QByteArray& responseData, int cpNo, int categoryNo);

    // Anime / Sport VOD Source
    void _qslotInitAnimationCategoryInfoAPIData(const QByteArray& responseData);
    void _qslotPlayNextVodTimer();
    void _qslotHandleContentListAPIData(const QByteArray& responseData, int contentType);
    void _qslotHandleVodListAPIData(const QByteArray& responseData, int contentType, int contentNo, int recentlyListLoad);

    // Direct Broad
    void _qslotPlayDirectBroadTimer();
    void _qslotHandleDirectBroadDataAPI(const QByteArray& responseData);
    void _qslotDirectBroadOneTimeUrlDataAPI(const QByteArray& responseData, int requestIdx);
    void _qslotDirectBroadOneTimeUrlDataAPI_2(const QByteArray& responseData, int requestIdx);
    void _qslotDirectBroadGeoBlockForDirectAPIData(const QByteArray& responseData, int requestIdx);

    // recv OBS Media Callback ( signal/slot connection is required when receiving data through UI. )
    void qslotRecvOBSMediaStarted();
    void qslotRecvOBSMediaStoped();
    void qslotRecvOBSMediaEnded();
    void qslotRecvOBSMediaPlay();
    void qslotRecvOBSMediaPause();
    void qslotRecvOBSMediaGetFirstFrame(int frame_width, int frame_height);

    void _qslotVodGeoBlockCheckAPIResponse(const QByteArray& responseData, int allowedCategory);

    void qslotVlcRestartRequested(QString sourceUuid, int abnormalCount, int videoAgeMs, int audioAgeMs, int avDriftMs);

protected:
    static void OBSMediaStarted(void* data, calldata_t* calldata);
    static void OBSMediaStopped(void* data, calldata_t* calldata);
    static void OBSMediaEnded(void* data, calldata_t* calldata);
    static void OBSMediaPlay(void* data, calldata_t* calldata);
    static void OBSMediaPause(void* data, calldata_t* calldata);
    static void FSMediaFileLoaded(void* data, calldata_t* calldata);

    static void OBSVlcRestartRequested(void* data, calldata_t* calldata);

public:
    static bool IsEqualVodInfo(const VodInfo_s& a, const VodInfo_s& b);

private:
    // seperate vod source obj
    OBSWeakSourceAutoRelease m_weakSource;
    std::vector<OBSSignal> m_signals;

    QWidget* m_pSoopSourceToolbar = nullptr;
    QDialog* m_pSoopSourceProps = nullptr;

    // TV Broad Source
    int m_currentCPNo = 0;

    bool m_refreshblind = false;
    QString m_blindProgram = "";
    QString m_blindDuration = "";
    QString m_tvLiveUrl = "";

    QTimer m_tvLiveBlindTimer;
    QTimer m_tvLiveStartTimer;

    std::vector<TvLiveInfo_s> m_tvLists;

    // Anime / Sport VOD Source
    ConfigFile	m_configSoopVOD;

    VodInfo_s m_curVodInfo[SOOP_VOD_TYPE::COUNT];
    std::vector<VodContentInfo_s> m_contents[SOOP_VOD_TYPE::COUNT];
    std::vector<QString> m_seasons[SOOP_VOD_TYPE::COUNT];
    std::vector<VodInfo_s> m_vodLists[SOOP_VOD_TYPE::COUNT];
    std::vector<VodInfo_s> m_vodPlayLists[SOOP_VOD_TYPE::COUNT];

    bool m_isVodSeasonRepeat[SOOP_VOD_TYPE::COUNT] = {false,};
    bool m_isVodListRepeat[SOOP_VOD_TYPE::COUNT] = {false,};

    QTimer m_timerNextVod;

    // Direct Broad
    int m_currentIdx = 0;
    QString m_currentURL = "";
    std::list<int>  m_directBroadAllowedCategory;
    std::list<int>  m_directBroadGeoBlockedCategory;

    QTimer m_timerDirectBroad;

    std::vector<QString> m_dates;
    std::vector<DirectBroadInfo_s> m_directBroadInfos;

    SOOP_VOD_TYPE m_pendingVodType = SOOP_VOD_TYPE::NONE;
    VodInfo_s m_pendingVodInfo = {};
    bool m_pendingSelectedItem = false;

    bool m_tvLiveRestartPending = false;
    uint64_t m_tvLiveRefreshSequence = 0;
};
