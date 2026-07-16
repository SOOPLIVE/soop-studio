#include "COutput.h"

#include <QMetaObject>

#include <util/threading.h>
#include "Application/CApplication.h"

#include "Common/SettingsMiscDef.h"

#include "SBasicOutputHandler.h"

#include "COBSOutputContext.h"

#include "CoreModel/Encoder/CEncoder.h"

#define MAX_OUTPUT_STREAM 5

namespace AFOutputUtil
{
    void ResetOutput(bool advOut)
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        if(MAX_OUTPUT_STREAM != outputHandlers.size()) {
            for(int i = 0; i < MAX_OUTPUT_STREAM; ++i) {
                OBSService service = nullptr;
                outputHandlers.emplace_back(service,
                    advOut ? CreateAdvancedOutputHandler(MAINFRAME) :
                             CreateSimpleOutputHandler(MAINFRAME));
            }
        } else {
            OUTPUT_HANDLER_LIST::iterator outputIter;
            for(outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
                if(!outputIter->second->Active()) {
                    outputIter->second.reset(advOut ?
                                CreateAdvancedOutputHandler(MAINFRAME) :
                                CreateSimpleOutputHandler(MAINFRAME));
                } else {
                    outputIter->second->Update();
                }
            }
        }
    }

    void OBSStreamStarting(void* data, calldata_t* params)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        obs_output_t* obj = (obs_output_t*)calldata_ptr(params, "output");

        int sec = (int)obs_output_get_active_delay(obj);
        if(sec == 0)
            return;

        output->delayActive = true;
        QMetaObject::invokeMethod(MAINFRAME, "qslotStreamDelayStarting", Q_ARG(void*, output), Q_ARG(int, sec));
    }


    void OBSStreamStopping(void* data, calldata_t* params)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        obs_output_t* obj = (obs_output_t*)calldata_ptr(params, "output");

        int sec = (int)obs_output_get_active_delay(obj);
        if(sec == 0)
            QMetaObject::invokeMethod(MAINFRAME, "qslotStreamStopping", Q_ARG(void*, output));
        else
            QMetaObject::invokeMethod(MAINFRAME, "qslotStreamDelayStopping", Q_ARG(void*, output), Q_ARG(int, sec));
    }

    void OBSStartStreaming(void* data, calldata_t* /* params */)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        output->streamingActive = true;

        //os_atomic_set_bool(&m_streamingActive, true);
        QMetaObject::invokeMethod(MAINFRAME, "qslotStreamingStart", Q_ARG(void*, output));
    }
    void OBSStopStreaming(void* data, calldata_t* params)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        int code = (int)calldata_int(params, "code");
        const char* last_error = calldata_string(params, "last_error");

        QString arg_last_error = QString::fromUtf8(last_error);

        output->streamingActive = false;
        output->delayActive = false;

        //os_atomic_set_bool(&streaming_active, false);
        QMetaObject::invokeMethod(MAINFRAME, "qslotStreamingStop",
                                  Q_ARG(void*, output),
                                  Q_ARG(int, code),
                                  Q_ARG(QString, arg_last_error));
    }

    void OBSStartRecording(void* data, calldata_t* /* params */)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);

        output->recordingActive = true;

        //os_atomic_set_bool(&recording_active, true);
        QMetaObject::invokeMethod(MAINFRAME, "qslotRecordingStart");
    }

    void OBSStopRecording(void* data, calldata_t* params)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        int code = (int)calldata_int(params, "code");
        const char* last_error = calldata_string(params, "last_error");

        QString arg_last_error = QString::fromUtf8(last_error);

        output->recordingActive = false;

        //os_atomic_set_bool(&recording_active, false);
        //os_atomic_set_bool(&recording_paused, false);
        QMetaObject::invokeMethod(MAINFRAME, "qslotRecordingStop",
                                  Q_ARG(int, code),
                                  Q_ARG(QString, arg_last_error));
    }

    void OBSRecordStopping(void* data, calldata_t* /* params */)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        QMetaObject::invokeMethod(MAINFRAME, "qslotRecordStopping");
    }

    void OBSRecordFileChanged(void* data, calldata_t* params)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        const char* next_file = calldata_string(params, "next_file");
        QString arg_last_file = QString::fromUtf8(output->lastRecordingPath.c_str());
        QMetaObject::invokeMethod(MAINFRAME, "qslotRecordingFileChanged", Q_ARG(QString, arg_last_file));
        output->lastRecordingPath = next_file;
    }

    void OBSStartReplayBuffer(void* data, calldata_t* /* params */)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);

        output->replayBufferActive = true;

        //os_atomic_set_bool(&replaybuf_active, true);
        QMetaObject::invokeMethod(MAINFRAME, "qslotReplayBufferStart");
    }
    void OBSStopReplayBuffer(void* data, calldata_t* params)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        int code = (int)calldata_int(params, "code");

        output->replayBufferActive = false;

        //os_atomic_set_bool(&replaybuf_active, false);
        QMetaObject::invokeMethod(MAINFRAME, "qslotReplayBufferStop", Q_ARG(int, code));
    }

    void OBSReplayBufferStopping(void* data, calldata_t* /* params */)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        QMetaObject::invokeMethod(MAINFRAME, "qslotReplayBufferStopping");
    }

    void OBSReplayBufferSaved(void* data, calldata_t* /* params */)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        QMetaObject::invokeMethod(MAINFRAME, "qslotReplayBufferSaved", Qt::QueuedConnection);
    }

    void OBSStartVirtualCam(void* data, calldata_t* /* params */)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        if(!output)
            return;

        output->virtualCamActive = true;
        //os_atomic_set_bool(&virtualcam_active, true);
        QMetaObject::invokeMethod(MAINFRAME, "qslotVirtualCamStart");
    }
    void OBSStopVirtualCam(void* data, calldata_t* params)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        if(!output ||
           !params)
            return;

        int code = (int)calldata_int(params, "code");

        output->virtualCamActive = false;
        //os_atomic_set_bool(&virtualcam_active, false);
        QMetaObject::invokeMethod(MAINFRAME, "qslotVirtualCamStop", Q_ARG(int, code));
    }
    void OBSDeactivateVirtualCam(void* data, calldata_t* /* params */)
    {
        AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
        if(output)
            output->DestroyVirtualCamView();
    }

    bool IsActive()
    {
        bool isActive = false;
        OUTPUT_HANDLER_LIST::iterator outputIter;
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        for(outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
            if(outputIter->second->Active()) {
                isActive = true;
                break;
            }
        }
        return isActive;
    }

    bool IsStreamActive()
    {
        bool isActive = false;
        OUTPUT_HANDLER_LIST::iterator outputIter;
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        for(outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
            if(outputIter->first && outputIter->second->StreamingActive()) {
                isActive = true;
                break;
            }
        }
        return isActive;
    }

    bool GetStreamingCheck()
    {
        return os_atomic_load_bool(&OUTPUT_CONTEXT.m_streamingActive);
    };

    bool GetRecordingCheck()
    {
        return os_atomic_load_bool(&OUTPUT_CONTEXT.m_recordingActive);
    }

    bool GetRecordingPauseCheck()
    {
        return os_atomic_load_bool(&OUTPUT_CONTEXT.m_recordingPaused);
    }

    bool GetReplayBufferCheck()
    {
        return os_atomic_load_bool(&OUTPUT_CONTEXT.m_replaybufActive);
    }

    bool IsReplayBufferActive()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        if(outputHandlers[0].second && outputHandlers[0].second->ReplayBufferActive())
            return true;
        return false;
    }

    bool IsRecordingActive()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        if(outputHandlers[0].second && outputHandlers[0].second->RecordingActive())
            return true;
        return false;
    }

    bool IsVirtualCamActive()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        if(outputHandlers.empty())
            return false;
        //
        OUTPUT_HANDLER_PTR& outputHandler = outputHandlers[0].second;
        if(!outputHandler ||
           !outputHandler->virtualCam)
            return false;
        //
        return outputHandler->VirtualCamActive();
    }

    bool IsMainStreamOutput(obs_output_t* output)
    {
        if(!output)
            return false;
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        if(outputHandlers[0].second.get()->streamOutput == output)
            return true;

        return false;
    }

    bool IsStartStreamingOutput(obs_service_t* service)
    {
        if(!service)
            return false;
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        for(auto outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
            if(service == outputIter->first) {
                if(outputIter->second->StreamingActive())
                    return true;
            }
        }
        return false;
    }

    bool IsStartStreamingOutput(obs_output_t* output)
    {
        if(!output)
            return false;
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        for(auto outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
            if(outputIter->second.get()->streamOutput == output) {
                if(outputIter->second->StreamingActive())
                    return true;
            }
        }
        return false;
    }

    bool PauseOutput()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();

        if(!outputHandlers[0].second || !outputHandlers[0].second->fileOutput)
            return false;

        obs_output_t* output_ = outputHandlers[0].second->fileOutput;

        return obs_output_paused(output_);
    }

    void SaveReplayBuffer()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();

        calldata_t cd = {0};
        proc_handler_t* ph = obs_output_get_proc_handler(outputHandlers[0].second->replayBuffer);
        proc_handler_call(ph, "save", &cd);
        calldata_free(&cd);
    }


    std::string SavedReplayBuffer()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();

        calldata_t cd = {0};
        proc_handler_t* ph = obs_output_get_proc_handler(outputHandlers[0].second->replayBuffer);
        proc_handler_call(ph, "get_last_replay", &cd);
        std::string path = calldata_string(&cd, "path");

        calldata_free(&cd);

        return path;
    }

    obs_output_t* GetRecordingFileOutput()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        return outputHandlers[0].second->fileOutput;
    }

    std::string GetLastRecordingPath()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        return outputHandlers[0].second->lastRecordingPath;
    }

    const char* GetCurrentOutputPath()
    {
        auto activeConfig = ACTIVECONFIG;
        //
        const char* path = nullptr;
        const char* mode = config_get_string(activeConfig, "Output", "Mode");
        if(strcmp(mode, "Advanced") == 0) {
            const char* advanced_mode = config_get_string(activeConfig, "AdvOut", "RecType");
            if(strcmp(advanced_mode, "FFmpeg") == 0) {
                path = config_get_string(activeConfig, "AdvOut", "FFFilePath");
            } else {
                path = config_get_string(activeConfig, "AdvOut", "RecFilePath");
            }
        } else {
            path = config_get_string(activeConfig, "SimpleOutput", "FilePath");
        }
        return path;
    }

    void StopStreaming()
    {
        OUTPUT_CONTEXT.SetStreamingStopping(true);

        OUTPUT_HANDLER_LIST::iterator outputIter;
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        for(outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
            if(outputIter->second->StreamingActive()) {
                outputIter->second->StopStreaming(OUTPUT_CONTEXT.IsStreamingStopping());
                if(outputIter->first) {
                    obs_service_release(outputIter->first);
                    outputIter->first = nullptr;
                }
            }
        }
    }

    void StopForceStreaming()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        for(auto outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
            if(outputIter->second->StreamingActive()) {
                outputIter->second->StopStreaming(true);
                if(outputIter->first) {
                    obs_service_release(outputIter->first);
                    outputIter->first = nullptr;
                }
            }
        }
    }

    bool StartRecording()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        return outputHandlers[0].second->StartRecording();
    }

    void StopRecording()
    {
        if(IsRecordingActive()) {
            OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
            outputHandlers[0].second->StopRecording(OUTPUT_CONTEXT.IsRecordingStopping());
        }
    }

    bool StartReplayBuffer()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        return outputHandlers[0].second->StartReplayBuffer();
    }

    bool StopReplayBuffer()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        if(!outputHandlers[0].second->replayBuffer)
            return false;

        if(!IsReplayBufferActive())
            return false;

        outputHandlers[0].second->StopReplayBuffer(OUTPUT_CONTEXT.IsReplayBufferStopping());

        return true;
    }

    bool PauseRecording()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        if (!outputHandlers[0].second || !outputHandlers[0].second->fileOutput ||
            GetRecordingPauseCheck())
            return false;
    
        obs_output_t* output = outputHandlers[0].second->fileOutput;
        if (!obs_output_pause(output, true))
            return false;
    
        os_atomic_set_bool(&OUTPUT_CONTEXT.m_recordingPaused, true);
    
        return true;
    }
    
    bool UnPauseRecording()
    {
        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        if (!outputHandlers[0].second || !outputHandlers[0].second->fileOutput ||
            GetRecordingPauseCheck())
            return false;
    
        obs_output_t* output = outputHandlers[0].second->fileOutput;
        if (!obs_output_pause(output, false))
            return false;
    
        os_atomic_set_bool(&OUTPUT_CONTEXT.m_recordingPaused, false);
    
        return true;
    }

    bool StartStreamingOutput(obs_service_t* service)
    {
        if(!service)
            return false;

        OUTPUT_HANDLER_LIST& outputHandlers = OUTPUT_CONTEXT.GetOutputHandlerLists();
        for(auto outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
            if(service == outputIter->first) {
                if(!outputIter->second->StartStreaming(outputIter->first)) {
                    return false;
                }
                return true;
            }
        }
        return false;
    }
};

/* mistakes have been made to lead us to this. */
inline const char* get_simple_output_encoder(const char* encoder)
{
    if(strcmp(encoder, SIMPLE_ENCODER_X264) == 0) {
        return "obs_x264";
    } else if(strcmp(encoder, SIMPLE_ENCODER_X264_LOWCPU) == 0) {
        return "obs_x264";
    } else if(strcmp(encoder, SIMPLE_ENCODER_QSV) == 0) {
        return "obs_qsv11_v2";
    } else if(strcmp(encoder, SIMPLE_ENCODER_QSV_AV1) == 0) {
        return "obs_qsv11_av1";
    } else if(strcmp(encoder, SIMPLE_ENCODER_AMD) == 0) {
        return "h264_texture_amf";
#ifdef ENABLE_HEVC
    } else if(strcmp(encoder, SIMPLE_ENCODER_AMD_HEVC) == 0) {
        return "h265_texture_amf";
#endif // ENABLE_HEVC
    } else if(strcmp(encoder, SIMPLE_ENCODER_AMD_AV1) == 0) {
        return "av1_texture_amf";
    } else if(strcmp(encoder, SIMPLE_ENCODER_NVENC) == 0) {
        return AFEncoderUtil::EncoderAvailable("obs_nvenc_h264_tex") ? "obs_nvenc_h264_tex" : "ffmpeg_nvenc";
#ifdef ENABLE_HEVC
    } else if(strcmp(encoder, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
        return AFEncoderUtil::EncoderAvailable("obs_nvenc_hevc_tex") ? "obs_nvenc_hevc_tex" : "ffmpeg_hevc_nvenc";
#endif// ENABLE_HEVC
    } else if(strcmp(encoder, SIMPLE_ENCODER_NVENC_AV1) == 0) {
        return "obs_nvenc_av1_tex";
#ifdef __APPLE__
    } else if(strcmp(encoder, SIMPLE_ENCODER_APPLE_H264) == 0) {
        return "com.apple.videotoolbox.videoencoder.ave.avc";
#ifdef ENABLE_HEVC
    } else if(strcmp(encoder, SIMPLE_ENCODER_APPLE_HEVC) == 0) {
        return "com.apple.videotoolbox.videoencoder.ave.hevc";
#endif // ENABLE_HEVC
#endif // __APPLE__
    }

    return "obs_x264";
}