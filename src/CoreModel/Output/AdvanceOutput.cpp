
#include "AdvanceOutput.h"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"

#include "CoreModel/Service/CService.h"
#include "CoreModel/Profile/CProfile.h"

#include "Common/SettingsMiscDef.h"
#include "Common/StringMiscUtils.h"

#define ADV_ARCHIVE_NAME "adv_archive_audio"

AFAdvanceOutput::AFAdvanceOutput(AFMainFrame* main)
    :AFBasicOutputHandler(main)
{
    config_t* basicConfig = GetBasicConfig();
    const char* recType = config_get_string(GetBasicConfig(), "AdvOut", "RecType");
    const char* streamEncoder = config_get_string(GetBasicConfig(), "AdvOut", "Encoder");
    const char* streamAudioEncoder = config_get_string(GetBasicConfig(), "AdvOut", "AudioEncoder");
    const char* recordEncoder = config_get_string(GetBasicConfig(), "AdvOut", "RecEncoder");
    const char* recAudioEncoder = config_get_string(GetBasicConfig(), "AdvOut", "RecAudioEncoder");
#ifdef __APPLE__
    translate_macvth264_encoder(streamEncoder);
    translate_macvth264_encoder(recordEncoder);
#endif
    ffmpegOutput = astrcmpi(recType, "FFmpeg") == 0;
    ffmpegRecording =
        ffmpegOutput &&
        config_get_bool(GetBasicConfig(), "AdvOut", "FFOutputToFile");
    useStreamEncoder = astrcmpi(recordEncoder, "none") == 0;
    useStreamAudioEncoder = astrcmpi(recAudioEncoder, "none") == 0;
    OBSData streamEncSettings = GetDataFromJsonFile("streamEncoder.json");
    OBSData recordEncSettings = GetDataFromJsonFile("recordEncoder.json");
    if(ffmpegOutput) {
        fileOutput = obs_output_create("ffmpeg_output", "adv_ffmpeg_output", nullptr, nullptr);
        if(!fileOutput)
            throw "Failed to create recording FFmpeg output "
            "(advanced output)";
    } else {
        bool useReplayBuffer = config_get_bool(GetBasicConfig(), "AdvOut", "RecRB");
        if(useReplayBuffer) {
            OBSDataAutoRelease hotkey;
            const char* str = config_get_string( GetBasicConfig(), "Hotkeys", "ReplayBuffer");
            if(str)
                hotkey = obs_data_create_from_json(str);
            else
                hotkey = nullptr;

            replayBuffer = obs_output_create("replay_buffer",
                                             Str("ReplayBuffer"),
                                             nullptr, hotkey);
            if(!replayBuffer)
                throw "Failed to create replay buffer output "
                "(simple output)";

            signal_handler_t* signal = obs_output_get_signal_handler(replayBuffer);
            startReplayBuffer.Connect(signal, "start", &AFMainFrame::OBSStartReplayBuffer, this);
            stopReplayBuffer.Connect(signal, "stop", &AFMainFrame::OBSStopReplayBuffer, this);
            replayBufferStopping.Connect(signal, "stopping", &AFMainFrame::OBSReplayBufferStopping, this);
            replayBufferSaved.Connect(signal, "saved", &AFMainFrame::OBSReplayBufferSaved, this);
        }
        fileOutput = obs_output_create("ffmpeg_muxer", "adv_file_output", nullptr, nullptr);
        if(!fileOutput)
            throw "Failed to create recording output "
            "(advanced output)";
        if(!useStreamEncoder) {
            videoRecording = obs_video_encoder_create(recordEncoder, "advanced_video_recording",
                                                      recordEncSettings, nullptr);
            if(!videoRecording)
                throw "Failed to create recording video "
                "encoder (advanced output)";
            obs_encoder_release(videoRecording);
        }
    }
    videoStreaming = obs_video_encoder_create(streamEncoder,
                                              "advanced_video_stream",
                                              streamEncSettings, nullptr);
    if(!videoStreaming)
        throw "Failed to create streaming video encoder "
        "(advanced output)";
    obs_encoder_release(videoStreaming);

    const char* rate_control = obs_data_get_string(useStreamEncoder ? streamEncSettings : recordEncSettings, "rate_control");
    if(!rate_control)
        rate_control = "";
    usesBitrate = astrcmpi(rate_control, "CBR") == 0 ||
        astrcmpi(rate_control, "VBR") == 0 ||
        astrcmpi(rate_control, "ABR") == 0;
    for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
        char name[19] = {0,};
        snprintf(name, sizeof(name), "adv_record_audio_%d", i);
        recordTrack[i] = obs_audio_encoder_create(useStreamAudioEncoder ? streamAudioEncoder
                                                  : recAudioEncoder,
                                                  name, nullptr, i, nullptr);
        if(!recordTrack[i]) {
            throw "Failed to create audio encoder "
                "(advanced output)";
        }
        obs_encoder_release(recordTrack[i]);
        snprintf(name, sizeof(name), "adv_stream_audio_%d", i);
        streamTrack[i] = obs_audio_encoder_create(
            streamAudioEncoder, name, nullptr, i, nullptr);
        if(!streamTrack[i]) {
            throw "Failed to create streaming audio encoders "
                "(advanced output)";
        }
        obs_encoder_release(streamTrack[i]);
    }
    std::string id;
    int streamTrackIndex = config_get_int(GetBasicConfig(), "AdvOut", "TrackIndex") - 1;
    streamAudioEnc = obs_audio_encoder_create(streamAudioEncoder,
                                              "adv_stream_audio", nullptr,
                                              streamTrackIndex, nullptr);
    if(!streamAudioEnc)
        throw "Failed to create streaming audio encoder "
        "(advanced output)";
    obs_encoder_release(streamAudioEnc);
    id = "";
    int vodTrack = config_get_int(GetBasicConfig(), "AdvOut", "VodTrackIndex") - 1;
    streamArchiveEnc = obs_audio_encoder_create(streamAudioEncoder,
                                                ADV_ARCHIVE_NAME, nullptr,
                                                vodTrack, nullptr);
    if(!streamArchiveEnc)
        throw "Failed to create archive audio encoder "
        "(advanced output)";
    obs_encoder_release(streamArchiveEnc);

    startRecording.Connect(obs_output_get_signal_handler(fileOutput), "start", &AFMainFrame::OBSStartRecording, this);
    stopRecording.Connect(obs_output_get_signal_handler(fileOutput), "stop", &AFMainFrame::OBSStopRecording, this);
    recordStopping.Connect(obs_output_get_signal_handler(fileOutput), "stopping", &AFMainFrame::OBSRecordStopping, this);
    recordFileChanged.Connect(obs_output_get_signal_handler(fileOutput), "file_changed", &AFMainFrame::OBSRecordFileChanged, this);
}
AFAdvanceOutput::~AFAdvanceOutput()
{
}
//
inline void ApplyEncoderDefaults(OBSData& settings, const obs_encoder_t* encoder)
{
    OBSData dataRet = obs_encoder_get_defaults(encoder);
    obs_data_release(dataRet);

    if(!!settings)
        obs_data_apply(dataRet, settings);
    settings = std::move(dataRet);
}
void AFAdvanceOutput::UpdateStreamSettings()
{
    bool applyServiceSettings = config_get_bool(GetBasicConfig(), "AdvOut", "ApplyServiceSettings");
    bool enforceBitrate = !config_get_bool(GetBasicConfig(), "Stream1", "IgnoreRecommended");
    bool dynBitrate = config_get_bool(GetBasicConfig(), "Output", "DynamicBitrate");
    const char* streamEncoder = config_get_string(GetBasicConfig(), "AdvOut", "Encoder");
    OBSData settings = GetDataFromJsonFile("streamEncoder.json");
    ApplyEncoderDefaults(settings, videoStreaming);
    if(applyServiceSettings) {
        int bitrate = (int)obs_data_get_int(settings, "bitrate");
        int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
        obs_service_apply_encoder_settings(AFServiceManager::GetSingletonInstance().GetService(), settings, nullptr);
        if(!enforceBitrate) {
            blog(LOG_INFO,
                 "User is ignoring service bitrate limits.");
            obs_data_set_int(settings, "bitrate", bitrate);
        }
        int enforced_keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
        if(keyint_sec != 0 && keyint_sec < enforced_keyint_sec)
            obs_data_set_int(settings, "keyint_sec", keyint_sec);
    } else {
        blog(LOG_WARNING, "User is ignoring service settings.");
    }
    if(dynBitrate && astrcmpi(streamEncoder, "jim_nvenc") == 0)
        obs_data_set_bool(settings, "lookahead", false);

    video_t* video = obs_get_video();
    enum video_format format = video_output_get_format(video);
    switch(format) {
        case VIDEO_FORMAT_I420:
        case VIDEO_FORMAT_NV12:
        case VIDEO_FORMAT_I010:
        case VIDEO_FORMAT_P010:
            break;
        default:
            obs_encoder_set_preferred_video_format(videoStreaming, VIDEO_FORMAT_NV12);
    }
    obs_encoder_update(videoStreaming, settings);
}
void AFAdvanceOutput::UpdateRecordingSettings()
{
    OBSData settings = GetDataFromJsonFile("recordEncoder.json");
    obs_encoder_update(videoRecording, settings);
}
void AFAdvanceOutput::UpdateAudioSettings()
{
    bool applyServiceSettings = config_get_bool(GetBasicConfig(), "AdvOut", "ApplyServiceSettings");
    bool enforceBitrate = !config_get_bool(GetBasicConfig(), "Stream1", "IgnoreRecommended");
    int streamTrackIndex = config_get_int(GetBasicConfig(), "AdvOut", "TrackIndex");
    int vodTrackIndex = config_get_int(GetBasicConfig(), "AdvOut", "VodTrackIndex");
    const char* audioEncoder = config_get_string(GetBasicConfig(), "AdvOut", "AudioEncoder");
    const char* recAudioEncoder = config_get_string(GetBasicConfig(), "AdvOut", "RecAudioEncoder");
    bool is_multitrack_output = allowsMultiTrack();
    OBSDataAutoRelease settings[MAX_AUDIO_MIXES];
    for(size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
        std::string cfg_name = "Track";
        cfg_name += std::to_string((int)i + 1);
        cfg_name += "Name";
        const char* name = config_get_string(GetBasicConfig(), "AdvOut", cfg_name.c_str());
        std::string def_name = "Track";
        def_name += std::to_string((int)i + 1);
        obs_encoder_set_name(recordTrack[i], (name && *name) ? name : def_name.c_str());
        obs_encoder_set_name(streamTrack[i], (name && *name) ? name : def_name.c_str());
    }
    for(size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
        int track = (int)(i + 1);
        settings[i] = obs_data_create();
        obs_data_set_int(settings[i], "bitrate", GetAudioBitrate(i, recAudioEncoder));
        obs_encoder_update(recordTrack[i], settings[i]);
        obs_data_set_int(settings[i], "bitrate", GetAudioBitrate(i, audioEncoder));
        if(!is_multitrack_output) {
            if(track == streamTrackIndex ||
                track == vodTrackIndex) {
                if(applyServiceSettings) {
                    int bitrate = (int)obs_data_get_int(settings[i], "bitrate");
                    obs_service_apply_encoder_settings(AFServiceManager::GetSingletonInstance().GetService(), nullptr, settings[i]);
                    if(!enforceBitrate)
                        obs_data_set_int(settings[i], "bitrate", bitrate);
                }
            }
            if(track == streamTrackIndex)
                obs_encoder_update(streamAudioEnc, settings[i]);
            if(track == vodTrackIndex)
                obs_encoder_update(streamArchiveEnc,
                            settings[i]);
        } else {
            obs_encoder_update(streamTrack[i], settings[i]);
        }
    }
}
void AFAdvanceOutput::Update()
{
    UpdateStreamSettings();
    if(!useStreamEncoder && !ffmpegOutput)
        UpdateRecordingSettings();
    UpdateAudioSettings();
}

void AFAdvanceOutput::SetupVodTrack(obs_service_t* service)
{
    int streamTrackIndex = config_get_int(GetBasicConfig(), "AdvOut", "TrackIndex");
    bool vodTrackEnabled = config_get_bool(GetBasicConfig(), "AdvOut", "VodTrackEnabled");
    int vodTrackIndex = config_get_int(GetBasicConfig(), "AdvOut", "VodTrackIndex");
    bool enableForCustomServer = config_get_bool(GetGlobalConfig(), "General", "EnableCustomServerVodTrack");
    const char* id = obs_service_get_id(service);
    if(strcmp(id, "rtmp_custom") == 0) {
        vodTrackEnabled = enableForCustomServer ? vodTrackEnabled : false;
    } else {
        OBSDataAutoRelease settings = obs_service_get_settings(service);
        const char* service = obs_data_get_string(settings, "service");
        if(!ServiceSupportsVodTrack(service))
            vodTrackEnabled = false;
    }
    if(vodTrackEnabled && streamTrackIndex != vodTrackIndex)
        obs_output_set_audio_encoder(streamOutput, streamArchiveEnc, 1);
    else
        clear_archive_encoder(streamOutput, ADV_ARCHIVE_NAME);
}

void AFAdvanceOutput::SetupStreaming()
{
    const char* rescaleRes = config_get_string(GetBasicConfig(), "AdvOut", "RescaleRes");
    int rescaleFilter = config_get_int(GetBasicConfig(), "AdvOut", "RescaleFilter");
    int multiTrackAudioMixes = config_get_int(GetBasicConfig(), "AdvOut", "StreamMultiTrackAudioMixes");
    unsigned int cx = 0;
    unsigned int cy = 0;
    int idx = 0;
    bool is_multitrack_output = allowsMultiTrack();

    if(rescaleFilter != OBS_SCALE_DISABLE &&
       rescaleRes &&
       *rescaleRes) {
        if(sscanf(rescaleRes, "%ux%u", &cx, &cy) != 2) {
            cx = 0;
            cy = 0;
        }
    }

    if(!is_multitrack_output) {
        obs_output_set_audio_encoder(streamOutput, streamAudioEnc, 0);
    } else {
        for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
            if((multiTrackAudioMixes & (1 << i)) != 0) {
                obs_output_set_audio_encoder(streamOutput, streamTrack[i], idx);
                idx++;
            }
        }
    }

    obs_encoder_set_scaled_size(videoStreaming, cx, cy);
    obs_encoder_set_gpu_scale_type(videoStreaming, (obs_scale_type)rescaleFilter);

    const char* id = obs_service_get_id(AFServiceManager::GetSingletonInstance().GetService());
    if(strcmp(id, "rtmp_custom") == 0) {
        OBSDataAutoRelease settings = obs_data_create();
        obs_service_apply_encoder_settings(AFServiceManager::GetSingletonInstance().GetService(), settings, nullptr);
        obs_encoder_update(videoStreaming, settings);
    }
}
void AFAdvanceOutput::SetupRecording()
{
    const char* path = config_get_string(GetBasicConfig(), "AdvOut", "RecFilePath");
    const char* mux = config_get_string(GetBasicConfig(), "AdvOut", "RecMuxerCustom");
    bool rescale = config_get_bool(GetBasicConfig(), "AdvOut", "RecRescale");
    const char* rescaleRes = config_get_string(GetBasicConfig(), "AdvOut", "RecRescaleRes");
    int rescaleFilter = config_get_int(GetBasicConfig(), "AdvOut", "RecRescaleFilter");
    int tracks = 0;
    const char* recFormat = config_get_string(GetBasicConfig(), "AdvOut", "RecFormat2");
    bool is_fragmented = strncmp(recFormat, "fragmented", 10) == 0;
    bool flv = strcmp(recFormat, "flv") == 0;
    if(flv)
        tracks = config_get_int(GetBasicConfig(), "AdvOut", "FLVTrack");
    else
        tracks = config_get_int(GetBasicConfig(), "AdvOut", "RecTracks");

    OBSDataAutoRelease settings = obs_data_create();
    unsigned int cx = 0;
    unsigned int cy = 0;
    int idx = 0;
    /* Hack to allow recordings without any audio tracks selected. It is no
     * longer possible to select such a configuration in settings, but legacy
     * configurations might still have this configured and we don't want to
     * just break them. */
    if(tracks == 0)
        tracks = config_get_int(GetBasicConfig(), "AdvOut", "TrackIndex");
    if(useStreamEncoder) {
        obs_output_set_video_encoder(fileOutput, videoStreaming);
        if(replayBuffer)
            obs_output_set_video_encoder(replayBuffer,
                             videoStreaming);
    } else {
        //if(rescaleFilter != OBS_SCALE_DISABLE && rescaleRes &&
        //    *rescaleRes) {
        if(rescale && rescaleRes && *rescaleRes) {
            if(sscanf(rescaleRes, "%ux%u", &cx, &cy) != 2) {
                cx = 0;
                cy = 0;
            }
        }
        obs_encoder_set_scaled_size(videoRecording, cx, cy);
        obs_encoder_set_gpu_scale_type(videoRecording, (obs_scale_type)rescaleFilter);
        obs_output_set_video_encoder(fileOutput, videoRecording);
        if(replayBuffer)
            obs_output_set_video_encoder(replayBuffer, videoRecording);
    }
    if(!flv) {
        for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
            if((tracks & (1 << i)) != 0) {
                obs_output_set_audio_encoder(
                    fileOutput, recordTrack[i], idx);
                if(replayBuffer)
                    obs_output_set_audio_encoder(
                        replayBuffer, recordTrack[i],
                        idx);
                idx++;
            }
        }
    } else if(flv && tracks != 0) {
        obs_output_set_audio_encoder(fileOutput, recordTrack[tracks - 1], idx);
        if(replayBuffer)
            obs_output_set_audio_encoder(replayBuffer, recordTrack[tracks - 1], idx);
    }
    // Use fragmented MOV/MP4 if user has not already specified custom movflags
    if(is_fragmented && (!mux || strstr(mux, "movflags") == NULL)) {
        std::string mux_frag = "movflags=frag_keyframe+empty_moov+delay_moov";
        if(mux) {
            mux_frag += " ";
            mux_frag += mux;
        }
        obs_data_set_string(settings, "muxer_settings",
                    mux_frag.c_str());
    } else {
        if(is_fragmented)
            blog(LOG_WARNING,
                 "User enabled fragmented recording, "
                 "but custom muxer settings contained movflags.");
        obs_data_set_string(settings, "muxer_settings", mux);
    }
    obs_data_set_string(settings, "path", path);
    obs_output_update(fileOutput, settings);
    if(replayBuffer)
        obs_output_update(replayBuffer, settings);
}
void AFAdvanceOutput::SetupFFmpeg()
{
    const char* url = config_get_string(GetBasicConfig(), "AdvOut", "FFURL");
    int vBitrate = config_get_int(GetBasicConfig(), "AdvOut", "FFVBitrate");
    int gopSize = config_get_int(GetBasicConfig(), "AdvOut", "FFVGOPSize");
    bool rescale = config_get_bool(GetBasicConfig(), "AdvOut", "FFRescale");
    const char* rescaleRes = config_get_string(GetBasicConfig(), "AdvOut", "FFRescaleRes");
    const char* formatName = config_get_string(GetBasicConfig(), "AdvOut", "FFFormat");
    const char* mimeType = config_get_string(GetBasicConfig(), "AdvOut", "FFFormatMimeType");
    const char* muxCustom = config_get_string(GetBasicConfig(), "AdvOut", "FFMCustom");
    const char* vEncoder = config_get_string(GetBasicConfig(), "AdvOut", "FFVEncoder");
    int vEncoderId = config_get_int(GetBasicConfig(), "AdvOut", "FFVEncoderId");
    const char* vEncCustom = config_get_string(GetBasicConfig(), "AdvOut", "FFVCustom");
    int aBitrate = config_get_int(GetBasicConfig(), "AdvOut", "FFABitrate");
    int aMixes = config_get_int(GetBasicConfig(), "AdvOut", "FFAudioMixes");
    const char* aEncoder = config_get_string(GetBasicConfig(), "AdvOut", "FFAEncoder");
    int aEncoderId = config_get_int(GetBasicConfig(), "AdvOut", "FFAEncoderId");
    const char* aEncCustom = config_get_string(GetBasicConfig(), "AdvOut", "FFACustom");
    OBSDataArrayAutoRelease audio_names = obs_data_array_create();
    for(size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
        std::string cfg_name = "Track";
        cfg_name += std::to_string((int)i + 1);
        cfg_name += "Name";
        const char* audioName = config_get_string(GetBasicConfig(), "AdvOut", cfg_name.c_str());
        OBSDataAutoRelease item = obs_data_create();
        obs_data_set_string(item, "name", audioName);
        obs_data_array_push_back(audio_names, item);
    }

    OBSDataAutoRelease settings = obs_data_create();
    obs_data_set_array(settings, "audio_names", audio_names);
    obs_data_set_string(settings, "url", url);
    obs_data_set_string(settings, "format_name", formatName);
    obs_data_set_string(settings, "format_mime_type", mimeType);
    obs_data_set_string(settings, "muxer_settings", muxCustom);
    obs_data_set_int(settings, "gop_size", gopSize);
    obs_data_set_int(settings, "video_bitrate", vBitrate);
    obs_data_set_string(settings, "video_encoder", vEncoder);
    obs_data_set_int(settings, "video_encoder_id", vEncoderId);
    obs_data_set_string(settings, "video_settings", vEncCustom);
    obs_data_set_int(settings, "audio_bitrate", aBitrate);
    obs_data_set_string(settings, "audio_encoder", aEncoder);
    obs_data_set_int(settings, "audio_encoder_id", aEncoderId);
    obs_data_set_string(settings, "audio_settings", aEncCustom);
    if(rescale && rescaleRes && *rescaleRes) {
        int width = 0;
        int height = 0;
        int val = sscanf(rescaleRes, "%dx%d", &width, &height);
        if(val == 2 && width && height) {
            obs_data_set_int(settings, "scale_width", width);
            obs_data_set_int(settings, "scale_height", height);
        }
    }
    obs_output_set_mixers(fileOutput, aMixes);
    obs_output_set_media(fileOutput, obs_get_video(), obs_get_audio());
    obs_output_update(fileOutput, settings);
}
void AFAdvanceOutput::SetupOutputs()
{
    obs_encoder_set_video(videoStreaming, obs_get_video());
    if(videoRecording)
        obs_encoder_set_video(videoRecording, obs_get_video());
    for(size_t i = 0; i < MAX_AUDIO_MIXES; i++) {
        obs_encoder_set_audio(streamTrack[i], obs_get_audio());
        obs_encoder_set_audio(recordTrack[i], obs_get_audio());
    }
    obs_encoder_set_audio(streamAudioEnc, obs_get_audio());
    obs_encoder_set_audio(streamArchiveEnc, obs_get_audio());
    SetupStreaming();
    if(ffmpegOutput)
        SetupFFmpeg();
    else
        SetupRecording();
}
int AFAdvanceOutput::GetAudioBitrate(size_t i, const char* id) const
{
    static const char* names[] = {
       "Track1Bitrate", "Track2Bitrate", "Track3Bitrate",
       "Track4Bitrate", "Track5Bitrate", "Track6Bitrate",
    };
    int bitrate = (int)config_get_uint(GetBasicConfig(), "AdvOut", names[i]);
    return FindClosestAvailableAudioBitrate(id, bitrate);
}

bool AFAdvanceOutput::SetupStreaming(obs_service_t* service)
{
    int multiTrackAudioMixes = config_get_int(GetBasicConfig(), "AdvOut", "StreamMultiTrackAudioMixes");
    int idx = 0;
    bool is_multitrack_output = allowsMultiTrack();
    if(!useStreamEncoder ||
        (!ffmpegOutput && !obs_output_active(fileOutput))) {
        UpdateStreamSettings();
    }

    UpdateAudioSettings();

    if(!Active())
        SetupOutputs();

    AFMainFrame* main = App()->GetMainView();
    AFAuth* auth = main->GetAuth();
    if(auth)
        auth->OnStreamConfig();

    /* --------------------- */

    const char* type = GetStreamOutputType(service);
    if(!type)
        return false;

    /* XXX: this is messy and disgusting and should be refactored */
    if(outputType != type) {
        streamDelayStarting.Disconnect();
        streamStopping.Disconnect();
        startStreaming.Disconnect();
        stopStreaming.Disconnect();

        streamOutput = obs_output_create(type, "adv_stream", nullptr, nullptr);
        if(!streamOutput) {
            blog(LOG_WARNING,
                 "Creation of stream output type '%s' "
                 "failed!",
                 type);
            return false;
        }

        streamDelayStarting.Connect(obs_output_get_signal_handler(streamOutput), "starting", &AFMainFrame::OBSStreamStarting, this);
        streamStopping.Connect(obs_output_get_signal_handler(streamOutput), "stopping", &AFMainFrame::OBSStreamStopping, this);

        startStreaming.Connect(obs_output_get_signal_handler(streamOutput), "start", &AFMainFrame::OBSStartStreaming, this);
        stopStreaming.Connect(obs_output_get_signal_handler(streamOutput), "stop", &AFMainFrame::OBSStopStreaming, this);

        outputType = type;
    }

    obs_output_set_video_encoder(streamOutput, videoStreaming);
    if(!is_multitrack_output) {
        obs_output_set_audio_encoder(streamOutput, streamAudioEnc, 0);
    } else {
        for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
            if((multiTrackAudioMixes & (1 << i)) != 0) {
                obs_output_set_audio_encoder(
                    streamOutput, streamTrack[i], idx);
                idx++;
            }
        }
    }
    return true;
}
bool AFAdvanceOutput::StartStreaming(obs_service_t* service)
{
    obs_output_set_service(streamOutput, service);

    bool reconnect = config_get_bool(GetBasicConfig(), "Output", "Reconnect");
    int retryDelay = config_get_int(GetBasicConfig(), "Output", "RetryDelay");
    int maxRetries = config_get_int(GetBasicConfig(), "Output", "MaxRetries");
    bool useDelay = config_get_bool(GetBasicConfig(), "Output", "DelayEnable");
    int delaySec = config_get_int(GetBasicConfig(), "Output", "DelaySec");
    bool preserveDelay = config_get_bool(GetBasicConfig(), "Output", "DelayPreserve");
    const char* bindIP = config_get_string(GetBasicConfig(), "Output", "BindIP");
    const char* ipFamily = config_get_string(GetBasicConfig(), "Output", "IPFamily");
#ifdef _WIN32
    bool enableNewSocketLoop = config_get_bool(GetBasicConfig(), "Output", "NewSocketLoopEnable");
    bool enableLowLatencyMode = config_get_bool(GetBasicConfig(), "Output", "LowLatencyEnable");
#endif
    bool enableDynBitrate = config_get_bool(GetBasicConfig(), "Output", "DynamicBitrate");

    bool is_rtmp = false;
    obs_service_t* service_obj = AFServiceManager::GetSingletonInstance().GetService();
    const char* protocol = obs_service_get_protocol(service_obj);
    if(protocol) {
        if(strncmp(protocol, RTMP_PROTOCOL, strlen(RTMP_PROTOCOL)) == 0)
            is_rtmp = true;
    }

    OBSDataAutoRelease settings = obs_data_create();
    obs_data_set_string(settings, "bind_ip", bindIP);
    obs_data_set_string(settings, "ip_family", ipFamily);
#ifdef _WIN32
    obs_data_set_bool(settings, "new_socket_loop_enabled", enableNewSocketLoop);
    obs_data_set_bool(settings, "low_latency_mode_enabled", enableLowLatencyMode);
#endif
    obs_data_set_bool(settings, "dyn_bitrate", enableDynBitrate);
    obs_output_update(streamOutput, settings);

    if(!reconnect)
        maxRetries = 0;

    obs_output_set_delay(streamOutput, useDelay ? delaySec : 0,
                         preserveDelay ? OBS_OUTPUT_DELAY_PRESERVE : 0);

    obs_output_set_reconnect_settings(streamOutput, maxRetries, retryDelay);
    if(is_rtmp) {
        SetupVodTrack(service);
    }
    if(obs_output_start(streamOutput)) {
        return true;
    }

    const char* error = obs_output_get_last_error(streamOutput);
    bool hasLastError = error && *error;
    if(hasLastError)
        lastError = error;
    else
        lastError = std::string();

    const char* type = obs_output_get_id(streamOutput);
    blog(LOG_WARNING, "Stream output type '%s' failed to start!%s%s", type,
         hasLastError ? "  Last Error: " : "", hasLastError ? error : "");
    return false;
}
bool AFAdvanceOutput::StartRecording()
{
    const char* path = nullptr;
    const char* recFormat = nullptr;
    const char* filenameFormat = nullptr;
    bool noSpace = false;
    bool overwriteIfExists = false;
    bool splitFile = false;
    const char* splitFileType = nullptr;
    int splitFileTime = -1;
    int splitFileSize = -1;
    if(!useStreamEncoder) {
        if(!ffmpegOutput) {
            UpdateRecordingSettings();
        }
    } else if(!obs_output_active(streamOutput)) {
        UpdateStreamSettings();
    }
    UpdateAudioSettings();
    if(!Active())
        SetupOutputs();
    if(!ffmpegOutput || ffmpegRecording) {
        path = config_get_string(GetBasicConfig(), "AdvOut",
                                 ffmpegRecording ? "FFFilePath"
                                 : "RecFilePath");
        recFormat = config_get_string(GetBasicConfig(), "AdvOut",
                                      ffmpegRecording ? "FFExtension"
                                      : "RecFormat2");
        filenameFormat = config_get_string(GetBasicConfig(), "Output", "FilenameFormatting");
        overwriteIfExists = config_get_bool(GetBasicConfig(), "Output", "OverwriteIfExists");
        noSpace = config_get_bool(GetBasicConfig(), "AdvOut",
                                  ffmpegRecording
                                  ? "FFFileNameWithoutSpace"
                                  : "RecFileNameWithoutSpace");
        splitFile = config_get_bool(GetBasicConfig(), "AdvOut", "RecSplitFile");
        std::string strPath = GetRecordingFilename(path, recFormat, noSpace,
                                                   overwriteIfExists,
                                                   filenameFormat,
                                                   ffmpegRecording);
        OBSDataAutoRelease settings = obs_data_create();
        obs_data_set_string(settings, ffmpegRecording ? "url" : "path",
                            strPath.c_str());
        if(splitFile) {
            splitFileType = config_get_string(
                GetBasicConfig(), "AdvOut", "RecSplitFileType");
            splitFileTime = (astrcmpi(splitFileType, "Time") == 0)
                ? config_get_int(GetBasicConfig(), "AdvOut", "RecSplitFileTime")
                : 0;
            splitFileSize = (astrcmpi(splitFileType, "Size") == 0)
                ? config_get_int(GetBasicConfig(), "AdvOut", "RecSplitFileSize")
                : 0;
            std::string ext = GetFormatExt(recFormat);
            obs_data_set_string(settings, "directory", path);
            obs_data_set_string(settings, "format", filenameFormat);
            obs_data_set_string(settings, "extension", ext.c_str());
            obs_data_set_bool(settings, "allow_spaces", !noSpace);
            obs_data_set_bool(settings, "allow_overwrite", overwriteIfExists);
            obs_data_set_bool(settings, "split_file", true);
            obs_data_set_int(settings, "max_time_sec", splitFileTime * 60);
            obs_data_set_int(settings, "max_size_mb", splitFileSize);
        }
        obs_output_update(fileOutput, settings);
    }

    bool bResult = obs_output_start(fileOutput);
    if(false == bResult) {
        const char* error = obs_output_get_last_error(fileOutput);
        App()->GetMainView()->ShowSystemAlert(QTStr("Output.StartRecordingFailed"));
    }
    return bResult;
}
bool AFAdvanceOutput::StartReplayBuffer()
{
    const char* path = nullptr;
    const char* recFormat = nullptr;
    const char* filenameFormat = nullptr;
    bool noSpace = false;
    bool overwriteIfExists = false;
    const char* rbPrefix = nullptr;
    const char* rbSuffix = nullptr;
    int rbTime;
    int rbSize;
    if(!useStreamEncoder) {
        if(!ffmpegOutput)
            UpdateRecordingSettings();
    } else if(!obs_output_active(streamOutput)) {
        UpdateStreamSettings();
    }
    UpdateAudioSettings();
    if(!Active())
        SetupOutputs();
    if(!ffmpegOutput || ffmpegRecording) {
        path = config_get_string(GetBasicConfig(), "AdvOut",
                                 ffmpegRecording ? "FFFilePath"
                                 : "RecFilePath");
        recFormat = config_get_string(GetBasicConfig(), "AdvOut",
                                      ffmpegRecording ? "FFExtension"
                                      : "RecFormat2");
        filenameFormat = config_get_string(GetBasicConfig(), "Output", "FilenameFormatting");
        overwriteIfExists = config_get_bool(GetBasicConfig(), "Output", "OverwriteIfExists");
        noSpace = config_get_bool(GetBasicConfig(), "AdvOut",
                                  ffmpegRecording
                                  ? "FFFileNameWithoutSpace"
                                  : "RecFileNameWithoutSpace");
        rbPrefix = config_get_string(GetBasicConfig(), "SimpleOutput", "RecRBPrefix");
        rbSuffix = config_get_string(GetBasicConfig(), "SimpleOutput", "RecRBSuffix");
        rbTime = config_get_int(GetBasicConfig(), "AdvOut", "RecRBTime");
        rbSize = config_get_int(GetBasicConfig(), "AdvOut", "RecRBSize");
        std::string f = GetFormatString(filenameFormat, rbPrefix, rbSuffix);
        std::string ext = GetFormatExt(recFormat);
        OBSDataAutoRelease settings = obs_data_create();
        obs_data_set_string(settings, "directory", path);
        obs_data_set_string(settings, "format", f.c_str());
        obs_data_set_string(settings, "extension", ext.c_str());
        obs_data_set_bool(settings, "allow_spaces", !noSpace);
        obs_data_set_int(settings, "max_time_sec", rbTime);
        obs_data_set_int(settings, "max_size_mb", usesBitrate ? 0 : rbSize);
        obs_output_update(replayBuffer, settings);
    }

    bool bResult = obs_output_start(replayBuffer);
    if(false == bResult) {
        const char* error = obs_output_get_last_error(replayBuffer);
        AFMainFrame::OBSErrorMessageBox(error,
                                       "Output.StartFailedGeneric",
                                       "Output.StartReplayFailed");
    }
    return bResult;
}
void AFAdvanceOutput::StopStreaming(bool force)
{
    if(force)
        obs_output_force_stop(streamOutput);
    else
        obs_output_stop(streamOutput);
}
void AFAdvanceOutput::StopRecording(bool force)
{
    if(force)
        obs_output_force_stop(fileOutput);
    else
        obs_output_stop(fileOutput);
}
void AFAdvanceOutput::StopReplayBuffer(bool force)
{
    if(force)
        obs_output_force_stop(replayBuffer);
    else
        obs_output_stop(replayBuffer);
}
bool AFAdvanceOutput::StreamingActive() const
{
    return obs_output_active(streamOutput);
}
bool AFAdvanceOutput::RecordingActive() const
{
    return obs_output_active(fileOutput);
}
bool AFAdvanceOutput::ReplayBufferActive() const
{
    return obs_output_active(replayBuffer);
}
bool AFAdvanceOutput::allowsMultiTrack()
{
    const char* protocol = nullptr;
    obs_service_t* service_obj = AFServiceManager::GetSingletonInstance().GetService();
    protocol = obs_service_get_protocol(service_obj);
    if(!protocol)
        return false;
    //
    return astrcmpi_n(protocol, SRT_PROTOCOL, strlen(SRT_PROTOCOL)) == 0 ||
        astrcmpi_n(protocol, RIST_PROTOCOL, strlen(RIST_PROTOCOL)) == 0;
}