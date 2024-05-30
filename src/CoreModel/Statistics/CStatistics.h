#pragma once

#include <obs.hpp>
#include <QObject>
#include <QPointer>
#include <qtimer.h>

#include <util/platform.h>
#include <util/threading.h>
#include <util/util.hpp>

#include "CoreModel/OBSOutput/SBasicOutputHandler.h"


static constexpr int congestionUpdateSeconds = 4;
static constexpr float excellentThreshold = 0.0f;
static constexpr float goodThreshold = 0.3333f;
static constexpr float mediocreThreshold = 0.6667f;
static constexpr float badThreshold = 1.0f;

enum class PCStatState {
    None,
    Normal,
    Warning,
    Error,
};

class AFStatistics : public QObject
{
	Q_OBJECT
#pragma region QT Field, CTOR/DTOR
public:
	AFStatistics();
    ~AFStatistics();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void StatisticsInit(int interval = 3000); // interval ms
    void BroadStatus(bool broad);
    void Reset();
    static void InitializeValues();
    void SetOutputHandler(AFBasicOutputHandler* handler) { m_outputHandler = handler; }
    void SetCongestionUpdate(bool start) { m_firstCongestionUpdate = start; }
    void SetDisconnected(bool disconnected);
    void ClearCongestionArray();

    /*inline QTimer* GetCpuUsageTimer() { return m_CpuUsageTimer.data(); }
    inline QTimer* GetMemoryTimer() { return m_MemoryTimer.data(); }
    inline QTimer* GetDiskFullTimer() { return m_DiskFullTimer.data(); }*/

    inline int GetNetworkState() const { return m_networkState; }
    inline double GetCPUUsage() const { return m_cpuUsage; }
    inline double GetCurFPS() const { return m_curFPS; }
    inline long double GetMemorySize() const { return m_memorySize; }
    inline long double GetOBSAvgFrameTime() const { return m_obsAvgFrameTime; }
    inline long double GetSkippedFrameRate() const { return m_skippedFrameRate; }
    inline long double GetLaggedFrameRate() const { return m_laggedFrameRate; }
    inline uint64_t GetDiskSize() const { return m_diskSize; }
    inline uint32_t GetTotalEncoded() const { return m_totalEncoded; }
    inline uint32_t GetTotalSkipped() const { return m_totalSkipped; }
    inline uint32_t GetTotalRendered() const { return m_totalRendered; }
    inline uint32_t GetTotalLagged() const { return m_totalLagged; }

    inline PCStatState GetNetworkIconState() const { return m_networkIconState; }
    inline PCStatState GetCPUIconState() const { return m_cpuIconState; }
    inline PCStatState GetDiskIconState() const { return m_diskIconState; }
    inline PCStatState GetMemoryIconState() const { return m_memoryIconState; }
    inline PCStatState GetSkippedFrameIconState() const { return m_skippedFrameIconState; }
    inline PCStatState GetLaggedFrameIconState() const { return m_laggedFrameIconState; }


    // Stream, Rec
    inline int GetStreamTotalFrame() const { return m_streamTotalFrame; }
    inline int GetStreamDroppedFrame() const { return m_streamDroppedFrame; }
    inline long double GetStreamDroppedFrameRate() const { return m_streamDroppedFrameRate; }
    inline long double GetStreamMegabytesSent() const { return m_streamMegabytesSent; }
    inline long double GetStreamBitrate() const { return m_streamBitrate; }
    inline long double GetRecMegabytesSent() const { return m_recMegabytesSent; }
    inline long double GetRecBitrate() const { return m_recBitrate; }

signals:
    void qsignalCheckDiskSpaceRemaining(uint64_t num_bytes);
    void qsignalNetworkState(PCStatState state);
    void qsignalCPUState(PCStatState state);
    void qsignalDiskState(PCStatState state);
    void qsignalMemoryState(PCStatState state);
    void qsignalFPSState(PCStatState state);
    void qsignalRenderTimeState(PCStatState state);
    void qsignalSkippedFrameState(PCStatState state);
    void qsignalLaggedFrameState(PCStatState state);
    void qsignalStreamFrameDropState(PCStatState state);

    void qsignalNetworkError();
    void qsignalCPUError();
    void qsignalMemoryError();
#pragma endregion public func

#pragma region private func
private:
    const char* _GetCurrentOutputPath();
    void        _UpdateStreamRecState(obs_output_t* output, bool rec);
    void        _ChangeNetworkIconState(PCStatState state);
    void        _ChangeCPUIconState(PCStatState state);
    void        _ChangeDiskIconState(PCStatState state);
    void        _ChangeMemoryIconState(PCStatState state);
    void        _ChangeSkippedFrameIconState(PCStatState state);
    void        _ChangeLaggedFrameIconState(PCStatState state);
#pragma endregion private func

#pragma region private slot func
private slots:
    void qslotUpdateNetworkState();
    void qslotUpdateCPUUsage();
    void qslotDiskTimerTick();
    void qslotMemoryTimerTick();
    void qslotUpdateFPS();
    void qslotUpdateSkippedFrame();
    void qslotUpdateLaggedFrame();
    void qslotUpdateStreamRecResource();
#pragma endregion private slot func

#pragma region public member var
public:

#pragma endregion public member var

#pragma region private member var
private:
    AFBasicOutputHandler* m_outputHandler = nullptr;

    QPointer<QTimer> m_CpuUsageTimer;
    QPointer<QTimer> m_MemoryTimer;
    QPointer<QTimer> m_DiskFullTimer;

    os_cpu_usage_info_t* m_cpuUsageInfo = nullptr;


    int         m_networkState = 0;
    double      m_cpuUsage = 0;
    double      m_curFPS = 0;
    uint64_t    m_diskSize = 0;
    uint32_t    m_totalEncoded = 0;
    uint32_t    m_totalSkipped = 0;
    uint32_t    m_totalRendered = 0;
    uint32_t    m_totalLagged = 0;
    long double m_skippedFrameRate = 0;
    long double m_laggedFrameRate = 0;
    long double m_memorySize = 0;
    long double m_obsAvgFrameTime = 0;

    // Stream, Rec
    bool        m_disconnected = false;
    bool        m_firstCongestionUpdate = false;

    std::vector<float> congestionArray;

    int         m_streamFirstTotal = 0;
    int         m_streamFirstDropped = 0;
    int         m_streamTotalFrame = 0;
    int         m_streamDroppedFrame = 0;
    uint64_t    m_streamLastBytesSent = 0;
    uint64_t    m_streamLastBytesSentTime = 0;
    uint64_t    m_recLastBytesSent = 0;
    uint64_t    m_recLastBytesSentTime = 0;
    long double m_streamDroppedFrameRate = 0;
    long double m_streamMegabytesSent = 0;
    long double m_streamBitrate = 0;
    long double m_recMegabytesSent = 0;
    long double m_recBitrate = 0;
    float       m_lastCongestion = 0.0f;

    PCStatState m_networkIconState = PCStatState::None;
    PCStatState m_cpuIconState = PCStatState::None;
    PCStatState m_diskIconState = PCStatState::None;
    PCStatState m_memoryIconState = PCStatState::None;
    PCStatState m_skippedFrameIconState = PCStatState::None;
    PCStatState m_laggedFrameIconState = PCStatState::None;
#pragma endregion private member var
};
