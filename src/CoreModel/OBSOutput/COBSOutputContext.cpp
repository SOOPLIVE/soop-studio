#include "COBSOutputContext.h"

#include "CoreModel/OBSOutput/SBasicOutputHandler.h"

//
AFOBSOutputContext::AFOBSOutputContext()
{
    m_outputHandlers.reserve(5);
}
AFOBSOutputContext::~AFOBSOutputContext()
{
    OUTPUT_HANDLER_LIST::iterator outputIter;
    for(outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if(outputIter->first) {
            obs_service_release(outputIter->first);
            outputIter->first = nullptr;
        }
    }
    m_outputHandlers.clear();
}
//
OUTPUT_HANDLER_LIST& AFOBSOutputContext::GetOutputHandlerLists()
{
	return m_outputHandlers;
}

const AFBasicOutputHandler* AFOBSOutputContext::getMainOutputHandler()
{
    if(m_outputHandlers.empty())
        return nullptr;
    //
    OUTPUT_HANDLER_LIST::iterator itr = m_outputHandlers.begin();
    if(itr == m_outputHandlers.end())
        return nullptr;
    //
    return itr->second.get();
}

bool AFOBSOutputContext::IsStreamingStopping()
{
	return m_streamingStopping;
}

void AFOBSOutputContext::SetStreamingStopping(bool stopping)
{
	m_streamingStopping = stopping;
}

bool AFOBSOutputContext::IsRecordingStopping()
{
	return m_recordingStopping;
}

void AFOBSOutputContext::SetRecordingStopping(bool stopping)
{
	m_recordingStopping = stopping;
}

bool AFOBSOutputContext::IsReplayBufferStopping()
{
	return m_replayBufferStopping;
}

void AFOBSOutputContext::SetReplayBufferStopping(bool stopping)
{
	m_replayBufferStopping = stopping;
}

bool AFOBSOutputContext::IsVirtualCamStopping()
{
	return m_vcamStopping;
}
void AFOBSOutputContext::SetVirtualCamStopping(bool stopping)
{
	m_vcamStopping = stopping;
}
