#pragma once


#include <memory>
#include <vector>

#include "SBasicOutputHandler.h"


typedef std::unique_ptr<AFBasicOutputHandler>            OUTPUT_HANDLER_PTR;
typedef std::pair<obs_service_t*, OUTPUT_HANDLER_PTR>    PAIR_OUTPUT_HANDLER;
typedef std::vector<PAIR_OUTPUT_HANDLER>                 OUTPUT_HANDLER_LIST;


class AFOBSOutputContext final
{
public:
    AFOBSOutputContext();
    ~AFOBSOutputContext();

public:
    OUTPUT_HANDLER_LIST& GetOutputHandlerLists();
    const AFBasicOutputHandler* getMainOutputHandler();

    volatile bool m_streamingActive = false;
    volatile bool m_recordingActive = false;
    volatile bool m_recordingPaused = false;
    volatile bool m_replaybufActive = false;
    volatile bool m_vcamActive = false;

    bool IsStreamingStopping();
    void SetStreamingStopping(bool stopping);

    bool IsRecordingStopping();
    void SetRecordingStopping(bool stopping);

    bool IsReplayBufferStopping();
    void SetReplayBufferStopping(bool stopping);

    bool IsVirtualCamStopping();
    void SetVirtualCamStopping(bool stopping);

private:
    OUTPUT_HANDLER_LIST m_outputHandlers;

    bool m_streamingStopping = false;
    bool m_recordingStopping = false;
    bool m_replayBufferStopping = false;
    bool m_vcamStopping = false;
};