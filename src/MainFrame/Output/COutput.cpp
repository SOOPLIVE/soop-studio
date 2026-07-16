#include "COutput.h"
#include "ui_aneta-main-frame.h"

#include <QFileInfo>

#include "qt-wrappers.hpp"

#include "libavformat/avformat.h"

#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/OBSOutput/COBSOutputContext.h"
#include "CoreModel/Action/CHotkeyContext.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Statistics/CStatistics.h"

#include "UIComponent/CMessageBox.h"

#include "PopupWindows/CRemuxFrame.h"

CMainOutput::CMainOutput(QObject* parent) :
    QObject(parent)
{

}

void CMainOutput::ResetOutputs()
{
    const char* mode = config_get_string(ACTIVECONFIG, "Output", "Mode");
    bool advOut = astrcmpi(mode, "Advanced") == 0;

    AFOutputUtil::ResetOutput(advOut);

    OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
    OUTPUT_HANDLER_LIST::iterator outputIter = outputHandlers.begin();
    m_statusbar.SetOutputHandler(outputIter->second.get());
    STATISTICS.SetOutputHandler(outputIter->second.get());

    // Register Hotkey
    bool useReplayBuffer = false;
    if (advOut)
        useReplayBuffer = config_get_bool(ACTIVECONFIG, "AdvOut", "RecRB");
    else
        useReplayBuffer = config_get_bool(ACTIVECONFIG, "SimpleOutput", "RecRB");

    if (useReplayBuffer)
        HOTKEY_CONTEXT.RegisterHotkeyReplayBufferSave(outputIter->second->replayBuffer);
    else
        HOTKEY_CONTEXT.UnRegisterHotkeyReplayBufferSave();

}

void CMainOutput::SetStreamingOutput()
{
    int handlerIdx = 0;
    AFChannelData* mainChannel = nullptr;
    if (AUTH_CONTEXT.GetMainChannelData(mainChannel)) {
        if (mainChannel != nullptr
            && !mainChannel->isStreaming)
        {
            if (AFOutputUtil::IsStreamActive()) 
                StopStreamingOutput((obs_service_t*)mainChannel->pObjOBSService);
        }
    }

    int cntOfAccount = AUTH_CONTEXT.GetCntChannel();
    for (int idx = 0; idx < cntOfAccount; idx++)
    {
        AFChannelData* tmpChannel = nullptr;
        AUTH_CONTEXT.GetChannelData(idx, tmpChannel);
        if (!tmpChannel)
            continue;

        if (!tmpChannel->isStreaming) {
            if (AFOutputUtil::IsStartStreamingOutput((obs_service_t*)tmpChannel->pObjOBSService))
                StopStreamingOutput((obs_service_t*)tmpChannel->pObjOBSService);
        }
    }
}

bool CMainOutput::PrepareStreamingOutput(int index, obs_service_t* service)
{
    if (!service)
        return false;

    if (AFOutputUtil::IsStartStreamingOutput(service))
        return  false;

    OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
    if (outputHandlers[index].first && outputHandlers[index].second->StreamingActive())
        return false;

    outputHandlers[index].first = service;
    if (!outputHandlers[index].second->SetupStreaming(outputHandlers[index].first)) {
        outputHandlers[index].first = nullptr;
        return false;
    }
    return true;
}

bool SetMainChannelServiceData(AFChannelData* channel, std::string url)
{
    if (!channel->isStreaming)
        return false;

    OBSDataAutoRelease settingData = obs_data_create();
    obs_data_set_bool(settingData, "bwtest", false);
    obs_data_set_string(settingData, "key", channel->pAuthData->keyRTMP.c_str());

    if(url != "")
        obs_data_set_string(settingData, "server", url.c_str());
    else
        obs_data_set_string(settingData, "server", channel->pAuthData->urlRTMP.c_str());

    obs_data_set_bool(settingData, "use_auth", false);
    obs_service_t* obsService = obs_service_create("rtmp_common", "default_service", settingData, nullptr);
    channel->pObjOBSService = obsService;

    return true;
}

bool SetCustomChannelServiceData(AFChannelData* channel)
{
    if (!channel->isStreaming)
        return false;
    if (channel->pAuthData->keyRTMP.empty() || channel->pAuthData->urlRTMP.empty())
        return false;

    OBSDataAutoRelease settingData = obs_data_create();
    obs_data_set_bool(settingData, "bwtest", false);
    obs_data_set_string(settingData, "key", channel->pAuthData->keyRTMP.c_str());
    obs_data_set_string(settingData, "server", channel->pAuthData->urlRTMP.c_str());
    if (!channel->pAuthData->customID.empty()) {
        obs_data_set_bool(settingData, "use_auth", true);
        obs_data_set_string(settingData, "username", channel->pAuthData->customID.c_str());
        if (!channel->pAuthData->customPassword.empty())
            obs_data_set_string(settingData, "password", channel->pAuthData->customPassword.c_str());
    }
    else
        obs_data_set_bool(settingData, "use_auth", false);

    // set service
    obs_service_t* obsService = obs_service_create("rtmp_common", "default_service", settingData, nullptr);
    channel->pObjOBSService = obsService;

    return true;
}

// SIMULCAST_BROAD //RTMP CHECK
bool CMainOutput::StartStreamingOutputs(bool isMain)
{
    auto& authManager = AUTH_CONTEXT;
    //
    bool retVal = true;
    do {
        if (isMain) {
            bool mainstreaming = false;
            AFChannelData* mainChannel = nullptr;
            std::string rtmpUrl = "";

            if (authManager.GetMainChannelData(mainChannel))
            {
                std::string platformName = PLATFORM_SOOP;

                mainstreaming = SetMainChannelServiceData(mainChannel, rtmpUrl);
                if (mainstreaming) {
                    if (!PrepareStreamingOutput(0, (obs_service_t*)mainChannel->pObjOBSService)) {
                        std::string mainChannelName = mainChannel->pAuthData->platform;
                        QString msg = QTStr("Output.PrepareStreaming.Failed").arg(mainChannelName.c_str());
                        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                            QTStr("Output.Streaming.Failed"), msg);
                        retVal &= false;
                        break;
                    }
                }
            }

            if (!mainstreaming) {
                for (int idx = 0; idx < authManager.GetCntChannel(); idx++) {
                    AFChannelData* tmpChannel = nullptr;
                    authManager.GetChannelData(idx, tmpChannel);
                    if (!tmpChannel || !tmpChannel->pAuthData)
                        continue;

                    if (tmpChannel->pAuthData->platform == PLATFORM_SOOP) {
                        bool isStreaming = SetCustomChannelServiceData(tmpChannel);
                        if (isStreaming) {
                            if (!PrepareStreamingOutput(1, (obs_service_t*)tmpChannel->pObjOBSService)) {
                                std::string tmpChannelName = tmpChannel->pAuthData->platform;
                                QString msg = QTStr("Output.StartStreaming.Failed").arg(tmpChannelName.c_str());
                                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                    QTStr("Output.Streaming.Failed"), msg);

                                retVal &= false;
                                break;
                            }
                        }
                    }                    
                }
            }
            if (false == retVal)
                return retVal;

            MAINFRAME->ChangeStreamStateUI(false, false, Str("Basic.Main.Connecting"), 110);

            // Start Streaming (main)
            OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
            OUTPUT_HANDLER_LIST::iterator it = mainstreaming ? outputHandlers.begin() : (outputHandlers.begin()+1);
            if ((*it).first) {
                m_streamingOutputRef++;
                if (!AFOutputUtil::StartStreamingOutput((*it).first)) {
                    std::string ChannelName;
                    if (mainstreaming)
                        ChannelName = mainChannel->pAuthData->platform;
                    else {
                        AFChannelData* tmpChannel = nullptr;
                        authManager.GetChannelData((*it).first, tmpChannel);
                        ChannelName = tmpChannel->pAuthData->platform;
                    }
                    QString msg = QTStr("Output.StartStreaming.Failed").arg(ChannelName.c_str());
                    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                        QTStr("Output.Streaming.Failed"), msg);
                    retVal &= false;                    
                }
            }
            if (false == retVal)
                return retVal;
            
            MAINFRAME->SetMainStreaming(mainstreaming);
            SetOutputHandler();
        }
        // sub
        else {
            int handlerIdx = 1;
            for (int idx = 0; idx < authManager.GetCntChannel(); idx++) {
                AFChannelData* tmpChannel = nullptr;
                authManager.GetChannelData(idx, tmpChannel);
                if (!tmpChannel || !tmpChannel->pAuthData ||
                    tmpChannel->pAuthData->platform == PLATFORM_SOOP)
                    continue;

                bool isStreaming = SetCustomChannelServiceData(tmpChannel);
                if (isStreaming) {
                    /*if (tmpChannel->pAuthData->platform == PLATFORM_SOOP) {
                        if (MAINFRAME->IsMainStreaming()) {
                            if (!PrepareStreamingOutput(1, (obs_service_t*)tmpChannel->pObjOBSService)) {
                                std::string tmpChannelName = tmpChannel->pAuthData->platform;
                                QString msg = QTStr("Output.StartStreaming.Failed").arg(tmpChannelName.c_str());
                                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                    QTStr("Output.Streaming.Failed"), msg);
                            }
                        }
                        else
                            continue;
                    } else {*/
                        handlerIdx++;
                        if (!PrepareStreamingOutput(handlerIdx, (obs_service_t*)tmpChannel->pObjOBSService)) {
                            std::string tmpChannelName = tmpChannel->pAuthData->platform;
                            QString msg = QTStr("Output.StartStreaming.Failed").arg(tmpChannelName.c_str());
                            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                QTStr("Output.Streaming.Failed"), msg);
                        }
                    //}                    
                }
            }

            // Start Streaming (other)
            const size_t start = MAINFRAME->IsMainStreaming() ? 1u : 2u;
            OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
            if (outputHandlers.size() <= start)
                return false;

			for (size_t i = start; i != outputHandlers.size(); ++i)
			{
				auto& handler = outputHandlers[i];
				if (!handler.first)
					continue;

				AFChannelData* tmpChannel = nullptr;
				authManager.GetChannelData(handler.first, tmpChannel);
				if (!tmpChannel || !tmpChannel->pAuthData)
					continue;

				m_streamingOutputRef++;
				if (!AFOutputUtil::StartStreamingOutput(handler.first))
				{
					AFChannelData* tmpChannel = nullptr;
					authManager.GetChannelData(handler.first, tmpChannel);
					if (tmpChannel && tmpChannel->pAuthData)
					{
						std::string tmpChannelName = tmpChannel->pAuthData->platform;
						QString msg = QTStr("Output.StartStreaming.Failed").arg(tmpChannelName.c_str());
						AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                                   QTStr("Output.Streaming.Failed"), msg);
					}
				}
			}
        }
    } while (false);    
    
    return retVal;
}

bool CMainOutput::StopStreamingOutput(obs_service_t* service)
{
    bool currentStreamOutputStopped = false;

    OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
    for (auto outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
        if (service == outputIter->first) {
            if (m_statusbar.GetOutputHandler() == outputIter->second.get())
                currentStreamOutputStopped = true;

            outputIter->second->StopStreaming(true);
            obs_service_release(outputIter->first);
            outputIter->first = nullptr;

            if (currentStreamOutputStopped)
                SetOutputHandler();
            return true;
        }
    }
    return false;
}

void CMainOutput::SetOutputHandler()
{
    OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
    for (auto outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter)
    {
        if (outputIter->first != nullptr) {
            STATISTICS.SetOutputHandler(outputIter->second.get());
            m_statusbar.SetOutputHandler(outputIter->second.get());
            return;
        }
    }
}

bool CMainOutput::IsStreamingOnlySoop()
{
    int cntList = AUTH_CONTEXT.GetCntChannel();
    for (int idx = 0; idx < cntList; idx++) {
        AFChannelData* tmpChannel = nullptr;
        AUTH_CONTEXT.GetChannelData(idx, tmpChannel);
        if (tmpChannel && tmpChannel->pAuthData &&
            (tmpChannel->pAuthData->platform == PLATFORM_SOOP))
            continue;

        if (tmpChannel && tmpChannel->pAuthData && tmpChannel->isStreaming)
            return false;
    }

    return true;
}

void CMainOutput::ClearAllStreamSignals()
{
    OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
        if (outputIter->first != nullptr &&
            outputIter->second != nullptr)
        {
            m_statusbar.StreamStopped(outputIter->second->streamOutput);
        }
    }
    m_statusbar.ClearAllSignals();
}

void CMainOutput::AutoRemux(QString input, bool noShow)
{
    auto activeConfig = ACTIVECONFIG;
    //
    bool autoRemux = config_get_bool(activeConfig, "Video", "AutoRemux");
    if (!autoRemux)
        return;

    bool isSimpleMode = false;
    const char* mode = config_get_string(activeConfig, "Output", "Mode");
    if (!mode) {
        isSimpleMode = true;
    }
    else {
        isSimpleMode = strcmp(mode, "Simple") == 0;
    }

    if (!isSimpleMode) {
        const char* recType = config_get_string(activeConfig, "AdvOut", "RecType");

        bool ffmpegOutput = astrcmpi(recType, "FFmpeg") == 0;
        if (ffmpegOutput)
            return;
    }

    if (input.isEmpty())
        return;

    QFileInfo fi(input);
    QString suffix = fi.suffix();

    /* do not remux if lossless */
    if (suffix.compare("avi", Qt::CaseInsensitive) == 0) {
        return;
    }

    QString path = fi.path();

    QString output = input;
    output.resize(output.size() - suffix.size());

    OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
    const obs_encoder_t* videoEncoder = obs_output_get_video_encoder(outputHandlers[0].second->fileOutput);
    const obs_encoder_t* audioEncoder = obs_output_get_audio_encoder(outputHandlers[0].second->fileOutput, 0);
    const char* vCodecName = obs_encoder_get_codec(videoEncoder);
    const char* aCodecName = obs_encoder_get_codec(audioEncoder);
    const char* format = config_get_string(activeConfig, isSimpleMode ? "SimpleOutput" : "AdvOut", "RecFormat2");

    bool audio_is_pcm = strncmp(aCodecName, "pcm", 3) == 0;

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(60, 5, 100)
    /* FFmpeg <= 6.0 cannot remux AV1+PCM into any supported format. */
    if (audio_is_pcm && strcmp(vCodecName, "av1") == 0)
        return;
#endif

    /* Retain original container for fMP4/fMOV */
    if (strncmp(format, "fragmented", 10) == 0) {
        output += "remuxed." + suffix;
    }
    else if (strcmp(vCodecName, "prores") == 0) {
        output += "mov";
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(60, 5, 100)
    }
    else if (audio_is_pcm) {
        output += "mov";
#endif
    }
    else {
        output += "mp4";
    }

    AFQRemux* remux = new AFQRemux(QT_TO_UTF8(path), MAINFRAME, true);
    if (!noShow)
        remux->show();
    remux->AutoRemux(input, output);
}

void CMainOutput::UpdatePause(bool activate)
{
    if (!activate ||
        !AFOutputUtil::IsRecordingActive()) {
        // pause.reset();
        return;
    }

    auto activeConfig = ACTIVECONFIG;
    //
    const char* mode = config_get_string(activeConfig, "Output", "Mode");
    bool adv = astrcmpi(mode, "Advanced") == 0;
    bool shared = false;
    if (adv) {
        const char* recType = config_get_string(activeConfig, "AdvOut", "RecType");
        if (astrcmpi(recType, "FFmpeg") == 0) {
            shared = config_get_bool(activeConfig, "AdvOut", "FFOutputToFile");
        }
        else {
            const char* recordEncoder = config_get_string(activeConfig, "AdvOut", "RecEncoder");
            shared = astrcmpi(recordEncoder, "none") == 0;
        }
    }
    else {
        const char* quality = config_get_string(activeConfig, "SimpleOutput", "RecQuality");
        shared = strcmp(quality, "Stream") == 0;
    }

    if (!shared) {
    }
    else {
    }
}

void CMainOutput::UpdateReplayBuffer(bool activate)
{
    if (!activate || !AFOutputUtil::IsReplayBufferActive()) {
        return;
    }
}
