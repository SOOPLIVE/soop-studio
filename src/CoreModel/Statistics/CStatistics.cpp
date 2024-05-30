#include "CStatistics.h"

#include "Application/CApplication.h"

static uint32_t first_encoded = 0xFFFFFFFF;
static uint32_t first_skipped = 0xFFFFFFFF;
static uint32_t first_rendered = 0xFFFFFFFF;
static uint32_t first_lagged = 0xFFFFFFFF;
//
AFStatistics::AFStatistics()
{
}
AFStatistics::~AFStatistics()
{
    if(m_CpuUsageTimer) {
        delete m_CpuUsageTimer;
        m_CpuUsageTimer = nullptr;
    }
    if(m_MemoryTimer) {
        delete m_MemoryTimer;
        m_MemoryTimer = nullptr;
    }
    if(m_DiskFullTimer) {
        delete m_DiskFullTimer;
        m_DiskFullTimer = nullptr;
    }

    os_cpu_usage_info_destroy(m_cpuUsageInfo);
}
//
void AFStatistics::StatisticsInit(int interval)
{
    m_cpuUsageInfo = os_cpu_usage_info_start();
    m_CpuUsageTimer = new QTimer(this);
    m_CpuUsageTimer->setInterval(interval);
    connect(m_CpuUsageTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateCPUUsage);
    m_CpuUsageTimer->start();
    //
    m_MemoryTimer = new QTimer(this);
    m_MemoryTimer->setInterval(interval);
    connect(m_MemoryTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotMemoryTimerTick);
    m_MemoryTimer->start();
    //
    m_DiskFullTimer = new QTimer(this);
    m_DiskFullTimer->setInterval(interval);
    connect(m_DiskFullTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotDiskTimerTick);
    //connect(m_DiskFullTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateNetworkState);
    connect(m_DiskFullTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateFPS);
    connect(m_DiskFullTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateSkippedFrame);
    connect(m_DiskFullTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateLaggedFrame);
    connect(m_DiskFullTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateStreamRecResource);
    m_DiskFullTimer->start();
    //
}
void AFStatistics::BroadStatus(bool broad)
{
    if(broad)
        connect(m_DiskFullTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateNetworkState);
    else
    {
        disconnect(m_DiskFullTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateNetworkState);
        _ChangeNetworkIconState(PCStatState::None);
    }
}
//

void AFStatistics::SetDisconnected(bool disconnected)
{
    m_disconnected = disconnected;
}

void AFStatistics::ClearCongestionArray()
{
    congestionArray.clear();
}

void AFStatistics::Reset()
{
    first_encoded = 0xFFFFFFFF;
    first_skipped = 0xFFFFFFFF;
    first_rendered = 0xFFFFFFFF;
    first_lagged = 0xFFFFFFFF;

    if (!m_outputHandler)
        return;

    OBSOutputAutoRelease streamOutput = obs_output_get_ref(m_outputHandler->streamOutput.Get());
    m_streamFirstTotal = streamOutput ? obs_output_get_total_frames(streamOutput) : 0;
    m_streamFirstDropped = streamOutput ? obs_output_get_frames_dropped(streamOutput) : 0;
    
    // update value
    qslotUpdateStreamRecResource();
    qslotDiskTimerTick();
    qslotUpdateNetworkState();
    qslotUpdateFPS();
    qslotUpdateSkippedFrame();
    qslotUpdateLaggedFrame();
    qslotUpdateCPUUsage();
    qslotMemoryTimerTick();
}

void AFStatistics::InitializeValues()
{
    video_t* video = obs_get_video();
    first_encoded = video_output_get_total_frames(video);
    first_skipped = video_output_get_skipped_frames(video);
    first_rendered = obs_get_total_frames();
    first_lagged = obs_get_lagged_frames();
}

const char* AFStatistics::_GetCurrentOutputPath()
{
    const char* path = nullptr;
    const char* mode = config_get_string(GetBasicConfig(), "Output", "Mode");
    if(strcmp(mode, "Advanced") == 0) {
        const char* advanced_mode = config_get_string(GetBasicConfig(), "AdvOut", "RecType");
        if(strcmp(advanced_mode, "FFmpeg") == 0) {
            path = config_get_string(GetBasicConfig(), "AdvOut", "FFFilePath");
        } else {
            path = config_get_string(GetBasicConfig(), "AdvOut", "RecFilePath");
        }
    } else {
        path = config_get_string(GetBasicConfig(), "SimpleOutput", "FilePath");
    }
    return path;
}
//

void AFStatistics::_UpdateStreamRecState(obs_output_t* output, bool rec)
{
    uint64_t totalBytes = output ? obs_output_get_total_bytes(output) : 0;
    uint64_t curTime = os_gettime_ns();
    uint64_t bytesSent = totalBytes;
    uint64_t bitsBetween;
    long double timePassed;

    if (!rec)
    {
        if (bytesSent < m_streamLastBytesSent)
            bytesSent = 0;
        if (bytesSent == 0)
            m_streamLastBytesSent = 0;
    
        bitsBetween = (bytesSent - m_streamLastBytesSent) * 8;
        timePassed =
            (long double)(curTime - m_streamLastBytesSentTime) / 1000000000.0l;
    }
    else
    {
        if (bytesSent < m_recLastBytesSent)
            bytesSent = 0;
        if (bytesSent == 0)
            m_recLastBytesSent = 0;

        bitsBetween = (bytesSent - m_recLastBytesSent) * 8;
        timePassed =
            (long double)(curTime - m_recLastBytesSentTime) / 1000000000.0l;
    }

    long double kbps = (long double)bitsBetween / timePassed / 1000.0l;

    if (timePassed < 0.01l)
        kbps = 0.0l;

    long double num = (long double)totalBytes / (1024.0l * 1024.0l);

    if (!rec) {
        m_streamMegabytesSent = num;
        m_streamBitrate = kbps;
    }
    else {
        m_recMegabytesSent = num;
        m_recBitrate = kbps;
    }

    if (!rec) {
        int total = output ? obs_output_get_total_frames(output) : 0;
        int dropped = output ? obs_output_get_frames_dropped(output)
            : 0;

        if (total < m_streamFirstTotal || dropped < m_streamFirstDropped) {
            m_streamFirstTotal = 0;
            m_streamFirstDropped = 0;
        }

        total -= m_streamFirstTotal;
        dropped -= m_streamFirstDropped;
        num = total ? (long double)dropped / (long double)total * 100.0l
            : 0.0l;

        m_streamTotalFrame = total;
        m_streamDroppedFrame = dropped;
        m_streamDroppedFrameRate = num;

        bool active = output ? obs_output_active(output) : false;
        if (active) {
            if (num > 5.0l)
                emit qsignalStreamFrameDropState(PCStatState::Error);
            //else if (num > 1.0l)
            //    emit qsignalStreamFrameDropState(PCStatState::Warning);
            else
                emit qsignalStreamFrameDropState(PCStatState::Normal);
        }
        else
            emit qsignalStreamFrameDropState(PCStatState::None);
    }

    if (!rec) {
        m_streamLastBytesSent = bytesSent;
        m_streamLastBytesSentTime = curTime;
    }
    else {
        m_recLastBytesSent = bytesSent;
        m_recLastBytesSentTime = curTime;
    }
}

void AFStatistics::_ChangeNetworkIconState(PCStatState state)
{
    m_networkIconState = state;
    emit qsignalNetworkState(state);
}

void AFStatistics::_ChangeCPUIconState(PCStatState state) 
{
    m_cpuIconState = state;
    emit qsignalCPUState(state);
}

void AFStatistics::_ChangeDiskIconState(PCStatState state) 
{
    m_diskIconState = state;
    emit qsignalDiskState(state);
}

void AFStatistics::_ChangeMemoryIconState(PCStatState state) 
{
    m_memoryIconState = state;
    emit qsignalMemoryState(state);
}

void AFStatistics::_ChangeSkippedFrameIconState(PCStatState state) 
{
    m_skippedFrameIconState = state;
    emit qsignalSkippedFrameState(state);
}

void AFStatistics::_ChangeLaggedFrameIconState(PCStatState state) 
{
    m_laggedFrameIconState = state;
    emit qsignalLaggedFrameState(state);
}

void AFStatistics::qslotUpdateNetworkState()
{
    float congestion = obs_output_get_congestion(m_outputHandler->streamOutput.Get());
    float avgCongestion = (congestion + m_lastCongestion) * 0.5f;
    if (avgCongestion < congestion)
        avgCongestion = congestion;
    if (avgCongestion > 1.0f)
        avgCongestion = 1.0f;

    m_lastCongestion = congestion;

    if (m_disconnected) {
        _ChangeNetworkIconState(PCStatState::Error);
        return;
    }

    bool update = m_firstCongestionUpdate;
    float congestionOverTime = avgCongestion;

    if (congestionArray.size() >= congestionUpdateSeconds) {
        congestionOverTime = accumulate(congestionArray.begin(),
            congestionArray.end(), 0.0f) /
            (float)congestionArray.size();
        m_networkState = congestionOverTime;
        congestionArray.clear();
        update = true;
    }
    else {
        congestionArray.emplace_back(avgCongestion);
    }

    if (update) {
        if (congestionOverTime <= excellentThreshold + EPSILON)
            _ChangeNetworkIconState(PCStatState::Normal);
        else if (congestionOverTime <= goodThreshold)
            _ChangeNetworkIconState(PCStatState::Error);
        else if (congestionOverTime <= mediocreThreshold)
            _ChangeNetworkIconState(PCStatState::Error);
        else if (congestionOverTime <= badThreshold)
            _ChangeNetworkIconState(PCStatState::Error);

        m_firstCongestionUpdate = false;
    }

    //emit qsignalNetworkError();
}

void AFStatistics::qslotUpdateCPUUsage()
{
    m_cpuUsage = os_cpu_usage_info_query(m_cpuUsageInfo);
    
    if (m_cpuUsage >= 40) {
        _ChangeCPUIconState(PCStatState::Error);
        emit qsignalCPUError();
    }
    //else if(m_cpuUsage >= 20)
    //    emit qsignalCPUState(PCStatState::Warning);
    else
        _ChangeCPUIconState(PCStatState::Normal);
}

#define GBYTE (1024ULL * 1024ULL * 1024ULL)
void AFStatistics::qslotDiskTimerTick()
{
    const char* path = _GetCurrentOutputPath();
    if(!path)
        return;

    m_diskSize = os_get_free_disk_space(path);
    //
    emit qsignalCheckDiskSpaceRemaining(m_diskSize);

    if (m_diskSize < GBYTE) {
        _ChangeDiskIconState(PCStatState::Error);
    }
    //else if (m_diskSize < (5 * GBYTE))
    //    emit qsignalDiskState(PCStatState::Warning);
    else
        _ChangeDiskIconState(PCStatState::Normal);
}

void AFStatistics::qslotMemoryTimerTick()
{
    m_memorySize = (long double)os_get_proc_resident_size() / (1024.0l * 1024.0l);

    long double totalMemory = (long double)os_get_sys_total_size() / (1024.0l * 1024.0l);
    //long double freeMemory = (long double)os_get_sys_free_size() / (1024.0l * 1024.0l);

    long double usePercent = m_memorySize / totalMemory;
    usePercent *= 100.0l;
    
    if (usePercent >= 40) {
        _ChangeMemoryIconState(PCStatState::Error);
        emit qsignalMemoryError();
    }
    //else if (usePercent >= 20)
    //    emit qsignalMemoryState(PCStatState::Warning);
    else
        _ChangeMemoryIconState(PCStatState::Normal);
}

void AFStatistics::qslotUpdateFPS()
{
    struct obs_video_info ovi = {};
    obs_get_video_info(&ovi);

    m_curFPS = obs_get_active_fps();
    double obsFPS = (double)ovi.fps_num / (double)ovi.fps_den;

    if (m_curFPS < (obsFPS * 0.8))
        emit qsignalFPSState(PCStatState::Error);
    //else if (m_curFPS < (obsFPS * 0.95))
    //    emit qsignalFPSState(PCStatState::Warning);
    else
        emit qsignalFPSState(PCStatState::Normal);
    
    m_obsAvgFrameTime = (long double)obs_get_average_frame_time_ns() / 1000000.0l;
    long double fpsFrameTime = (long double)ovi.fps_den * 1000.0l / (long double)ovi.fps_num;

    if (m_obsAvgFrameTime > fpsFrameTime)
        emit qsignalRenderTimeState(PCStatState::Error);
    //else if (m_obsAvgFrameTime > fpsFrameTime * 0.75l)
    //    emit qsignalRenderTimeState(PCStatState::Warning);
    else
        emit qsignalRenderTimeState(PCStatState::Normal);
}

void AFStatistics::qslotUpdateSkippedFrame()
{
    video_t* video = obs_get_video();
    m_totalEncoded = video_output_get_total_frames(video);
    m_totalSkipped = video_output_get_skipped_frames(video);

    if (m_totalEncoded < first_encoded || m_totalSkipped < first_skipped) {
        first_encoded = m_totalEncoded;
        first_skipped = m_totalSkipped;
    }
    m_totalEncoded -= first_encoded;
    m_totalSkipped -= first_skipped;

    m_skippedFrameRate = m_totalEncoded
                        ? (long double)m_totalSkipped / (long double)m_totalEncoded
                        : 0.0l;
    m_skippedFrameRate *= 100.0l;

    if (m_skippedFrameRate > 5.0l)
        _ChangeSkippedFrameIconState(PCStatState::Error);
    //else if (m_skippedFrameRate > 1.0l)
    //    emit qsignalSkippedFrameState(PCStatState::Warning);
    else
        _ChangeSkippedFrameIconState(PCStatState::Normal);
}

void AFStatistics::qslotUpdateLaggedFrame()
{
    m_totalRendered = obs_get_total_frames();
    m_totalLagged = obs_get_lagged_frames();

    if (m_totalRendered < first_rendered || m_totalLagged < first_lagged) {
        first_rendered = m_totalRendered;
        first_lagged = m_totalLagged;
    }
    m_totalRendered -= first_rendered;
    m_totalLagged -= first_lagged;

    m_laggedFrameRate = m_totalRendered
                        ? (long double)m_totalLagged / (long double)m_totalRendered
                        : 0.0l;
    m_laggedFrameRate *= 100.0l;

    if (m_laggedFrameRate > 5.0l)
        _ChangeLaggedFrameIconState(PCStatState::Error);
    //else if (m_laggedFrameRate > 1.0l)
    //    emit qsignalLaggedFrameState(PCStatState::Warning);
    else
        _ChangeLaggedFrameIconState(PCStatState::Normal);
}

void AFStatistics::qslotUpdateStreamRecResource()
{
    if (!m_outputHandler)
        return;

    OBSOutputAutoRelease streamOutput = obs_output_get_ref(m_outputHandler->streamOutput.Get());
    OBSOutputAutoRelease recOutput = obs_output_get_ref(m_outputHandler->fileOutput.Get());
    
    if (!streamOutput && !recOutput)
    {
        m_streamTotalFrame = 0;
        m_streamDroppedFrame = 0;
        m_streamDroppedFrameRate = 0;
        m_streamMegabytesSent = 0;
        m_streamBitrate = 0;
        m_recMegabytesSent = 0;
        m_recBitrate = 0;
        emit qsignalStreamFrameDropState(PCStatState::None);
        return;
    }

    _UpdateStreamRecState(streamOutput, false);
    _UpdateStreamRecState(recOutput, true);
}