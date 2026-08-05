#include "CStatistics.h"

#include "Application/CApplication.h"

#include "CoreModel/OBSOutput/COutput.h"

#ifdef _WIN32
#include <PdhMsg.h>
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Pdh.lib")
#include <dxgi.h>
#include <D3D10_1.h>
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#endif


#ifdef _WIN32
// dxgi ordinal
typedef HRESULT(WINAPI* CREATEDXGIFACTORY1PROC)(REFIID riid, void** ppFactory);
#endif

#define DEF_CPUINFO_REGKEY      ("Hardware\\Description\\System\\CentralProcessor")
#define DEF_CPUINFO_REGKEY_0	("Hardware\\Description\\System\\CentralProcessor\\0")
#define DEF_CPUINFO_NAME_QUERY  ("ProcessorNameString")
#define DEF_OSINFO_REGKEY       ("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\")
//
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
    if(m_cpuUsageTimer) {
        delete m_cpuUsageTimer;
        m_cpuUsageTimer = nullptr;
    }
    if(m_memoryTimer) {
        delete m_memoryTimer;
        m_memoryTimer = nullptr;
    }
    if(m_diskFullTimer) {
        delete m_diskFullTimer;
        m_diskFullTimer = nullptr;
    }

    os_cpu_usage_info_destroy(m_pCpuUsageInfo);

#ifdef _WIN32
    if (_cpuTotal) {
        PdhRemoveCounter(_cpuTotal);
        _cpuTotal = NULL;
    }
    if (_cpuQuery) {
        PdhCloseQuery(_cpuQuery);
        _cpuQuery = NULL;
    }
    //
    if(recv_bytes) {
        PdhRemoveCounter(recv_bytes);
        recv_bytes = nullptr;
    }
    if(sent_bytes) {
        PdhRemoveCounter(sent_bytes);
        sent_bytes = nullptr;
    }
    if(network_query) {
        PdhCloseQuery(network_query);
        network_query = nullptr;
    }
#endif // _WIN32
}
//
void AFStatistics::StatisticsInit(int interval)
{
    m_pCpuUsageInfo = os_cpu_usage_info_start();
    m_cpuUsageTimer = new QTimer(this);
    m_cpuUsageTimer->setInterval(interval);
    connect(m_cpuUsageTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateCPUUsage);
    
    connect(m_cpuUsageTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateFPS);
    connect(m_cpuUsageTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateSkippedFrame);
    connect(m_cpuUsageTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateLaggedFrame);
    connect(m_cpuUsageTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateStreamRecResource);

    m_cpuUsageTimer->start();
    //
    m_memoryTimer = new QTimer(this);
    m_memoryTimer->setInterval(interval);
    connect(m_memoryTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotMemoryTimerTick);
    m_memoryTimer->start();
    //
    m_diskFullTimer = new QTimer(this);
    m_diskFullTimer->setInterval(interval);
    connect(m_diskFullTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotDiskTimerTick);
    //
    // Total CPU
#ifdef _WIN32
    PdhOpenQuery(NULL, NULL, &_cpuQuery);
    PdhAddCounter(_cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &_cpuTotal);
    PdhCollectQueryData(_cpuQuery);
#endif // _WIN32
    bool result = _GetPCInfomation();
}
void AFStatistics::BroadStatus(bool broad)
{
    if(broad)
        connect(m_cpuUsageTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateNetworkState);
    else
    {
        disconnect(m_cpuUsageTimer.data(), &QTimer::timeout, this, &AFStatistics::qslotUpdateNetworkState);
        _ChangeNetworkIconState(PCStatState::None);
    }
}
//

void AFStatistics::SetDiskFullTimer(bool start)
{
    if (m_diskFullTimer) {
        if (start) {
            if (m_diskFullTimer->isActive())
                m_diskFullTimer->stop();
            m_diskFullTimer->start();
        }
        else {
            m_diskFullTimer->stop();
        }
    }
}

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

    if (!m_pOutputHandler)
        return;

    OBSOutputAutoRelease streamOutput = obs_output_get_ref(m_pOutputHandler->streamOutput.Get());
    m_streamFirstTotal = streamOutput ? obs_output_get_total_frames(streamOutput) : 0;
    m_streamFirstDropped = streamOutput ? obs_output_get_frames_dropped(streamOutput) : 0;
    
    // update value
    qslotUpdateStreamRecResource();
    qslotDiskTimerTick();
    //qslotUpdateNetworkState();
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

    if (!rec) {
        uint64_t currentFrames = m_streamTotalFrame - m_lastTotalStream;
        uint64_t currentDropped = m_streamDroppedFrame - m_lastLaggedStream;
        uint64_t framesPresented = currentFrames - currentDropped;

        uint64_t actualTimePassedNs = curTime - m_lastUpdateTimeStream;
        double actualTimePassed = actualTimePassedNs / 1000000000.0;

        m_encoderFPS = actualTimePassed > 0 ? (double)framesPresented / actualTimePassed : 0;
        //blog(LOG_INFO, "Encoder FPS: %.2f", m_encoderFPS);

        m_lastTotalStream = m_streamTotalFrame;
        m_lastLaggedStream = m_streamDroppedFrame;
        m_lastUpdateTimeStream = curTime;
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

#define PCI_NETWORK_INTERFACE      510
#define PCI_BYTES_RECEIVED_PER_SEC 264
#define PCI_BYTES_SENT_PER_SEC     506

inline bool GetNetworkInterface(std::wstring& downloadPath, std::wstring& uploadPath)
{
    wchar_t szObjName[PDH_MAX_COUNTER_NAME] = {0};
    wchar_t szRecvName[PDH_MAX_COUNTER_NAME] = {0};
    wchar_t szSentName[PDH_MAX_COUNTER_NAME] = {0};

    DWORD dwSize = PDH_MAX_COUNTER_NAME;
    if(PdhLookupPerfNameByIndexW(nullptr, PCI_NETWORK_INTERFACE, szObjName, &dwSize) != ERROR_SUCCESS) return false;
    dwSize = PDH_MAX_COUNTER_NAME;
    if(PdhLookupPerfNameByIndexW(nullptr, PCI_BYTES_RECEIVED_PER_SEC, szRecvName, &dwSize) != ERROR_SUCCESS) return false;
    dwSize = PDH_MAX_COUNTER_NAME;
    if(PdhLookupPerfNameByIndexW(nullptr, PCI_BYTES_SENT_PER_SEC, szSentName, &dwSize) != ERROR_SUCCESS) return false;

    DWORD dwCounterListSize = 0;
    DWORD dwInstanceListSize = 0;

    PDH_STATUS status = PdhEnumObjectItemsW(
        nullptr, nullptr, szObjName,
        nullptr, &dwCounterListSize,
        nullptr, &dwInstanceListSize,
        PERF_DETAIL_WIZARD, 0
    );

    if(status != ERROR_SUCCESS && status != PDH_MORE_DATA)
        return false;

    if(dwInstanceListSize == 0)
        return false;

    std::vector<wchar_t> mszCounterList(dwCounterListSize, 0);
    std::vector<wchar_t> mszInstanceList(dwInstanceListSize, 0);

    status = PdhEnumObjectItemsW(
        nullptr, nullptr, szObjName,
        mszCounterList.data(), &dwCounterListSize,
        mszInstanceList.data(), &dwInstanceListSize,
        PERF_DETAIL_WIZARD, 0
    );
    
    if(status != ERROR_SUCCESS)
        return false;

    const std::vector<std::wstring> vIgnoreKeywords = {
        L"loopback", L"virtual", L"pseudo", L"miniport",
        L"wintun", L"vethernet", L"tg3", L"isatap"
    };

    const wchar_t* pInstance = mszInstanceList.data();
    while(pInstance && *pInstance != L'\0') {
        std::wstring instanceName(pInstance);

        std::wstring lowerName = instanceName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        bool bIsVirtual = false;
        for(const auto& keyword : vIgnoreKeywords) {
            if(lowerName.find(keyword) != std::wstring::npos) {
                bIsVirtual = true;
                break;
            }
        }

        if(!bIsVirtual) {
            PDH_COUNTER_PATH_ELEMENTS_W pathElems = {0};
            pathElems.szMachineName = nullptr;
            pathElems.szObjectName = szObjName;
            pathElems.szInstanceName = const_cast<LPWSTR>(instanceName.c_str());
            pathElems.szParentInstance = nullptr;
            pathElems.dwInstanceIndex = 0;

            wchar_t szFullPath[0x2000] = {0};
            DWORD dwPathSize = 0x2000;

            pathElems.szCounterName = szRecvName;
            dwPathSize = 0x2000;
            if(PdhMakeCounterPathW(&pathElems, szFullPath, &dwPathSize, 0) == ERROR_SUCCESS) {
                downloadPath = szFullPath;
            }

            pathElems.szCounterName = szSentName;
            dwPathSize = 0x2000;
            if(PdhMakeCounterPathW(&pathElems, szFullPath, &dwPathSize, 0) == ERROR_SUCCESS) {
                uploadPath = szFullPath;
                return true;
            }
        }
        pInstance += instanceName.size() + 1;
    }

    return false;
}

bool AFStatistics::_GetPCInfomation()
{
    bool result = false;

#ifdef _WIN32
    HKEY hRegKey = NULL;
    HMODULE hD3D10_1 = NULL;
    HMODULE hDXGI = NULL;

    do {
        // CPU
        char szCPUName[100] = { 0, };
        LSTATUS status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, DEF_CPUINFO_REGKEY_0,
            0, KEY_QUERY_VALUE, &hRegKey);
        if (ERROR_SUCCESS != status) break;

        DWORD c_size = 100;
        status = RegQueryValueExA(hRegKey, DEF_CPUINFO_NAME_QUERY,
            NULL, NULL, (LPBYTE)szCPUName, &c_size);
        if (ERROR_SUCCESS != status) break;

        m_cpuInfo.assign(szCPUName);
        RegCloseKey(hRegKey);
        hRegKey = NULL;

        // OS
        {
            char szProductName[100] = { 0, };
            char szCSDVersion[100] = { 0, };
            char szReleaseID[100] = { 0, };
            char szCurrentBuild[100] = { 0, };

            if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE, DEF_OSINFO_REGKEY,
                0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hRegKey))
            {
                c_size = 100;
                if (ERROR_SUCCESS == RegQueryValueExA(hRegKey, "ProductName",
                    NULL, NULL, (LPBYTE)szProductName, &c_size))
                {
                    c_size = 100;
                    if (ERROR_SUCCESS != RegQueryValueExA(hRegKey, "CSDVersion",
                        NULL, NULL, (LPBYTE)szCSDVersion, &c_size))
                    {
                        c_size = 100;
                        RegQueryValueExA(hRegKey, "ReleaseId",
                            NULL, NULL, (LPBYTE)szReleaseID, &c_size);
                    }
                    c_size = 100;
                    RegQueryValueExA(hRegKey, "CurrentBuild",
                        NULL, NULL, (LPBYTE)szCurrentBuild, &c_size);
                }

                QString szOSInfo = QString("%1 %2 %3")
                    .arg(szProductName)
                    .arg(strlen(szCSDVersion) > 0 ? szCSDVersion : szReleaseID)
                    .arg(szCurrentBuild);
                m_osInfo = szOSInfo.toUtf8().constData();

                RegCloseKey(hRegKey);
                hRegKey = NULL;
            }
        }

        std::map<std::wstring, std::wstring> displayToGpuMap;

        do {
            hD3D10_1 = LoadLibraryA("d3d10_1.dll");
            if (NULL == hD3D10_1) break;

            hDXGI = LoadLibraryA("dxgi.dll");
            if (NULL == hDXGI) break;

            CREATEDXGIFACTORY1PROC pfnCreateDXGIFactory =
                (CREATEDXGIFACTORY1PROC)GetProcAddress(hDXGI, "CreateDXGIFactory1");
            if (NULL == pfnCreateDXGIFactory) break;

            PFN_D3D10_CREATE_DEVICE_AND_SWAP_CHAIN1 pfnD3D10CreateDeviceAndSwapChain =
                (PFN_D3D10_CREATE_DEVICE_AND_SWAP_CHAIN1)
                GetProcAddress(hD3D10_1, "D3D10CreateDeviceAndSwapChain1");
            if (NULL == pfnD3D10CreateDeviceAndSwapChain) break;

            IDXGIFactory1* pFactory = NULL;
            HRESULT hResult = pfnCreateDXGIFactory(__uuidof(IDXGIFactory1), (void**)&pFactory);
            if (FAILED(hResult)) break;

            QStringList gpuList;
            UINT adapterIdx = 0;
            IDXGIAdapter1* pAdapter = NULL;

            while (pFactory->EnumAdapters1(adapterIdx, &pAdapter) != DXGI_ERROR_NOT_FOUND)
            {
                DXGI_ADAPTER_DESC1 adapterDesc = {};
                if (SUCCEEDED(pAdapter->GetDesc1(&adapterDesc)))
                {
                    if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))
                    {
                        UINT outputIdx = 0;
                        IDXGIOutput* pOutput = NULL;
                        while (pAdapter->EnumOutputs(outputIdx, &pOutput) != DXGI_ERROR_NOT_FOUND)
                        {
                            DXGI_OUTPUT_DESC outputDesc = {};
                            if (SUCCEEDED(pOutput->GetDesc(&outputDesc)))
                            {
                                displayToGpuMap[outputDesc.DeviceName] = adapterDesc.Description;
                            }
                            pOutput->Release();
                            pOutput = NULL;
                            outputIdx++;
                        }

                        LARGE_INTEGER lVersion = {};
                        if (SUCCEEDED(pAdapter->CheckInterfaceSupport(__uuidof(ID3D10Device), &lVersion)))
                        {
                            QString versionString = QString("%1.%2.%3.%4")
                                .arg(HIWORD(lVersion.HighPart))
                                .arg(LOWORD(lVersion.HighPart))
                                .arg(HIWORD(lVersion.LowPart))
                                .arg(LOWORD(lVersion.LowPart));

                            gpuList.append(QString("%1(%2)")
                                .arg(QString::fromWCharArray(adapterDesc.Description))
                                .arg(versionString));
                        }
                        else
                        {
                            gpuList.append(QString::fromWCharArray(adapterDesc.Description));
                        }
                    }
                }

                pAdapter->Release();
                pAdapter = NULL;
                adapterIdx++;
            }

            pFactory->Release();

            m_gpuInfo = gpuList.join(", ").toUtf8().constData();
        } while (false);

        std::string displayInfo;
        char buf[512] = { 0, };

        for (int deviceNum = 0; ; deviceNum++)
        {
            DISPLAY_DEVICE device = {};
            device.cb = sizeof(device);
            if (!EnumDisplayDevices(0, deviceNum, &device, 0)) break;

            if (!(device.StateFlags & DISPLAY_DEVICE_ACTIVE)) continue;

            std::wstring gpuName = L"Unknown GPU";
            auto it = displayToGpuMap.find(device.DeviceName);
            if (it != displayToGpuMap.end())
                gpuName = it->second;

            for (int monitorNum = 0; ; monitorNum++)
            {
                DISPLAY_DEVICE monitor = {};
                monitor.cb = sizeof(monitor);
                if (!EnumDisplayDevices(device.DeviceName, monitorNum, &monitor, 0)) break;

                DEVMODE devmode = {};
                if (!::EnumDisplaySettings(device.DeviceName, ENUM_CURRENT_SETTINGS, &devmode))
                    continue;

                UINT scalePercent = 100;
                POINT pt = { devmode.dmPosition.x, devmode.dmPosition.y };
                HMONITOR hMonitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
                if (hMonitor)
                {
                    UINT dpiX = 0, dpiY = 0;
                    if (SUCCEEDED(::GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
                        scalePercent = (dpiX * 100) / 96;
                }

                snprintf(buf, sizeof(buf),
                    "%S(%d x %d, %d bits, %dhz, %d%%)[%S]",
                    monitor.DeviceString,
                    devmode.dmPelsWidth, devmode.dmPelsHeight,
                    devmode.dmBitsPerPel, devmode.dmDisplayFrequency,
                    scalePercent,
                    gpuName.c_str());

                if (!displayInfo.empty()) displayInfo += ", ";
                displayInfo += buf;
            }
        }

        m_displayInfo = displayInfo;

        // network : get network interface(recv, sent)
        std::wstring downloadPath, uploadPath;
        if(GetNetworkInterface(downloadPath, uploadPath)) {
            blog(LOG_INFO, "download path: %S", downloadPath.c_str());
            blog(LOG_INFO, "upload path: %S", uploadPath.c_str());

            if(PdhOpenQuery(NULL, 0, &network_query) == ERROR_SUCCESS) {
                status = PdhAddCounterW(network_query, downloadPath.c_str(), 0, &recv_bytes);
                if(ERROR_SUCCESS != status) break;

                status = PdhAddCounterW(network_query, uploadPath.c_str(), 0, &sent_bytes);
                if(ERROR_SUCCESS != status) break;

                PdhCollectQueryData(network_query);
            }
        }

        result = true;

    } while (false);

    if (NULL != hRegKey) {
        RegCloseKey(hRegKey);
        hRegKey = NULL;
    }
    if (hD3D10_1) {
        FreeLibrary(hD3D10_1);
        hD3D10_1 = NULL;
    }
    if (hDXGI) {
        FreeLibrary(hDXGI);
        hDXGI = NULL;
    }
#endif // _WIN32

    return result;
}

void AFStatistics::qslotUpdateNetworkState()
{
    float congestion = obs_output_get_congestion(m_pOutputHandler->streamOutput.Get());
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
    m_cpuUsage = os_cpu_usage_info_query(m_pCpuUsageInfo);
    bool isCpuHigh = false;
#ifdef _WIN32
    PDH_FMT_COUNTERVALUE counterVal = { 0, };
    PdhCollectQueryData(_cpuQuery);
    PdhGetFormattedCounterValue(_cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
    m_cpuTotal = counterVal.doubleValue;
    if (m_cpuTotal >= 85.0) {
        isCpuHigh = true;
    }
#else
    if (m_cpuUsage >= 80.0) {
        isCpuHigh = true;
    }
#endif
    if (isCpuHigh) {
        m_continuousHighCpuTicks++;
    }
    else {
        m_continuousHighCpuTicks = 0; 
    }

    if (m_continuousHighCpuTicks >= 5) {
        _ChangeCPUIconState(PCStatState::Error);
        emit qsignalCPUError();
    }
    else {
        _ChangeCPUIconState(PCStatState::Normal);
    }
}

#define GBYTE (1024ULL * 1024ULL * 1024ULL)
void AFStatistics::qslotDiskTimerTick()
{
    const char* path = AFOutputUtil::GetCurrentOutputPath();
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
    m_virtualMemory = os_get_proc_virtual_size() / (1024.0l * 1024.0l);
    long double freeMemory = (long double)os_get_sys_free_size() / (1024.0l * 1024.0l);
    long double totalMemory = (long double)os_get_sys_total_size() / (1024.0l * 1024.0l);
    
    m_memoryTotalUsage = totalMemory - freeMemory;
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

    uint64_t curTime = os_gettime_ns();

    uint64_t currentFrames = m_totalRendered - m_lastTotalRendered;
    uint64_t currentDropped = m_totalLagged - m_lastLaggedRendered;
    uint64_t framesPresented = currentFrames - currentDropped;

    uint64_t actualTimePassedNs = curTime - m_lastUpdateTimeRendered;
    double actualTimePassed = actualTimePassedNs / 1000000000.0; 

    double renderFPS = actualTimePassed > 0 ? (double)framesPresented / actualTimePassed : 0;
    //blog(LOG_INFO, "Render FPS: %.2f", renderFPS);

    m_lastTotalRendered = m_totalRendered;
    m_lastLaggedRendered = m_totalLagged;
    m_lastUpdateTimeRendered = curTime;

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
    if (!m_pOutputHandler)
        return;

    OBSOutputAutoRelease streamOutput = obs_output_get_ref(m_pOutputHandler->streamOutput.Get());
    OBSOutputAutoRelease recOutput = obs_output_get_ref(m_pOutputHandler->fileOutput.Get());
    
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
//
void AFStatistics::GetNetworkBandWidth(std::string& sentBits, std::string& recvBits)
{
    if(!network_query || !sent_bytes || !recv_bytes) {
        sentBits = recvBits = "unknown bps";
        return;
    }

#ifdef _WIN32
    if(PdhCollectQueryData(network_query) != ERROR_SUCCESS)
        return;

    const int megaBits = ((1024 * 1024) / 8);
    const int kiloBits = (1024 / 8);
    //
    PDH_FMT_COUNTERVALUE fmtValueUpload;
    PDH_FMT_COUNTERVALUE fmtValueDownload;
    PdhGetFormattedCounterValue(sent_bytes, PDH_FMT_DOUBLE, NULL, &fmtValueUpload);
    PdhGetFormattedCounterValue(recv_bytes, PDH_FMT_DOUBLE, NULL, &fmtValueDownload);
    //
    std::string upUnit = (fmtValueUpload.doubleValue > megaBits ? " Mbps" : " Kbps");
    std::string downUnit = (fmtValueDownload.doubleValue > megaBits ? " Mbps" : " Kbps");

    double upBits = (fmtValueUpload.doubleValue > megaBits ?
                     fmtValueUpload.doubleValue / megaBits :
                     fmtValueUpload.doubleValue / kiloBits);
    double downBits = (fmtValueDownload.doubleValue > megaBits ?
                       fmtValueDownload.doubleValue / megaBits :
                       fmtValueDownload.doubleValue / kiloBits);

    sentBits = std::to_string(upBits) + upUnit;
    recvBits = std::to_string(downBits) + downUnit;
#endif // _WIN32
}