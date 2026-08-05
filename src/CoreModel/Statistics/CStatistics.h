#pragma once

#include <obs.hpp>
#include <QObject>
#include <QPointer>
#include <qtimer.h>

#include <util/platform.h>
#include <util/threading.h>
#include <util/util.hpp>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#endif

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

public:
    AFStatistics();
    ~AFStatistics();

public:
    void StatisticsInit(int interval = 1000); // interval ms
    void BroadStatus(bool broad);
    void Reset();
    static void InitializeValues();
    void SetOutputHandler(AFBasicOutputHandler* handler) { m_pOutputHandler = handler; }
    void SetCongestionUpdate(bool start) { m_firstCongestionUpdate = start; }
    void SetDisconnected(bool disconnected);
    void ClearCongestionArray();
    void SetDiskFullTimer(bool start);

    /*inline QTimer* GetCpuUsageTimer() { return m_cpuUsageTimer.data(); }
    inline QTimer* GetMemoryTimer() { return m_memoryTimer.data(); }
    inline QTimer* GetDiskFullTimer() { return m_diskFullTimer.data(); }*/

    inline int GetNetworkState() const { return m_networkState; }
    inline double GetCPUUsage() const { return m_cpuUsage; }
    inline double GetCPUTotalUsage() const { return m_cpuTotal; }
    inline double GetCurFPS() const { return m_curFPS; }
    inline double GetEncoderFPS() const { return m_encoderFPS; }
    inline std::string GetCPUInfo() const { return m_cpuInfo; }
    inline std::string GetOSInfo() const { return m_osInfo; }
    inline std::string GetGPUInfo() const { return m_gpuInfo; }
    inline std::string GetDisplayInfo() const { return m_displayInfo; }

    inline long double GetMemorySize() const { return m_memorySize; }
    inline long double GetMemoryTotalUsage() const { return m_memoryTotalUsage; }
    inline long double GetVirtualMemorySize() const { return m_virtualMemory; }
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

    void GetNetworkBandWidth(std::string& sentBits, std::string& recvBits);

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

private:
    void _UpdateStreamRecState(obs_output_t* output, bool rec);
    void _ChangeNetworkIconState(PCStatState state);
    void _ChangeCPUIconState(PCStatState state);
    void _ChangeDiskIconState(PCStatState state);
    void _ChangeMemoryIconState(PCStatState state);
    void _ChangeSkippedFrameIconState(PCStatState state);
    void _ChangeLaggedFrameIconState(PCStatState state);
    bool _GetPCInfomation();

private slots:
    void qslotUpdateNetworkState();
    void qslotUpdateCPUUsage();
    void qslotDiskTimerTick();
    void qslotMemoryTimerTick();
    void qslotUpdateFPS();
    void qslotUpdateSkippedFrame();
    void qslotUpdateLaggedFrame();
    void qslotUpdateStreamRecResource();

private:
    AFBasicOutputHandler* m_pOutputHandler = nullptr;

    QPointer<QTimer> m_cpuUsageTimer;
    QPointer<QTimer> m_memoryTimer;
    QPointer<QTimer> m_diskFullTimer;
    //
    os_cpu_usage_info_t* m_pCpuUsageInfo = nullptr;
    
#ifdef _WIN32
    PDH_HQUERY _cpuQuery = nullptr;
    PDH_HCOUNTER _cpuTotal = nullptr;

    PDH_HQUERY network_query = nullptr;
    PDH_HCOUNTER recv_bytes = nullptr;
    PDH_HCOUNTER sent_bytes = nullptr;
#endif // _WIN32

    std::string m_cpuInfo;
    std::string m_osInfo;
    std::string m_gpuInfo;
    std::string m_displayInfo;
    //
    std::string network_info;

    int m_networkState = 0;
    double m_cpuUsage = 0;
    double m_cpuTotal = 0;
           
    double m_curFPS = 0;
    double m_encoderFPS = 0;
    uint64_t m_diskSize = 0;
    uint32_t m_totalEncoded = 0;
    uint32_t m_totalSkipped = 0;
    uint32_t m_totalRendered = 0;
    uint32_t m_totalLagged = 0;
    long double m_skippedFrameRate = 0;
    long double m_laggedFrameRate = 0;
    long double m_memorySize = 0;
    long double m_memoryTotalUsage = 0;
    long double m_virtualMemory = 0;
    long double m_obsAvgFrameTime = 0;

    int m_continuousHighCpuTicks = 0;

    // Stream, Rec
    bool m_disconnected = false;
    bool m_firstCongestionUpdate = false;

    std::vector<float> congestionArray;

    int m_streamFirstTotal = 0;
    int m_streamFirstDropped = 0;
    int m_streamTotalFrame = 0;
    int m_streamDroppedFrame = 0;
    uint64_t m_streamLastBytesSent = 0;
    uint64_t m_streamLastBytesSentTime = 0;
    uint64_t m_recLastBytesSent = 0;
    uint64_t m_recLastBytesSentTime = 0;
    long double m_streamDroppedFrameRate = 0;
    long double m_streamMegabytesSent = 0;
    long double m_streamBitrate = 0;
    long double m_recMegabytesSent = 0;
    long double m_recBitrate = 0;
    float m_lastCongestion = 0.0f;

    PCStatState m_networkIconState = PCStatState::None;
    PCStatState m_cpuIconState = PCStatState::None;
    PCStatState m_diskIconState = PCStatState::None;
    PCStatState m_memoryIconState = PCStatState::None;
    PCStatState m_skippedFrameIconState = PCStatState::None;
    PCStatState m_laggedFrameIconState = PCStatState::None;

    uint64_t m_lastTotalRendered = 0;
    uint64_t m_lastLaggedRendered = 0;
    uint64_t m_lastUpdateTimeRendered = 0;

    uint64_t m_lastTotalStream = 0;
    uint64_t m_lastLaggedStream = 0;
    uint64_t m_lastUpdateTimeStream = 0;
};
