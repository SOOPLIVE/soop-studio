
#include "SimpleOutput.h"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"

#include "CoreModel/Service/CService.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Encoder/CEncoder.h"

#include "Common/SettingsMiscDef.h"
#include "Common/StringMiscUtils.h"

#include "UIComponent/CAudioEncoders.h"


#define CROSS_DIST_CUTOFF 2000.0

AFSimpleOutput::AFSimpleOutput(AFMainFrame* main)
	:AFBasicOutputHandler(main)
{
	auto config = ACTIVECONFIG;
	//
	const char* encoder = config_get_string(config, "SimpleOutput", "StreamEncoder");
	const char* audio_encoder = config_get_string(config, "SimpleOutput", "StreamAudioEncoder");

	LoadStreamingPreset_Lossy(get_simple_output_encoder(encoder));

	bool success = false;
	if(strcmp(audio_encoder, "opus") == 0)
		success = AFEncoderUtil::CreateSimpleOpusEncoder(m_audioStreaming, GetAudioBitrate(), "simple_opus", 0);
	else
		success = AFEncoderUtil::CreateSimpleAACEncoder(m_audioStreaming, GetAudioBitrate(), "simple_aac", 0);

	if(!success)
		throw "Failed to create audio streaming encoder (simple output)";

	if(strcmp(audio_encoder, "opus") == 0)
		success = AFEncoderUtil::CreateSimpleOpusEncoder(m_audioArchive, GetAudioBitrate(), SIMPLE_ARCHIVE_NAME, 1);
	else
		success = AFEncoderUtil::CreateSimpleAACEncoder(m_audioArchive, GetAudioBitrate(), SIMPLE_ARCHIVE_NAME, 1);

	if(!success)
		throw "Failed to create audio archive encoder (simple output)";

	LoadRecordingPreset();

	if(!m_ffmpegOutput) {
		bool useReplayBuffer = config_get_bool(config, "SimpleOutput", "RecRB");
		const char* recFormat = config_get_string(config, "SimpleOutput", "RecFormat2");
		if(useReplayBuffer) {
			OBSDataAutoRelease hotkey;
			const char* str = config_get_string(config, "Hotkeys", "ReplayBuffer");
			if(str)
				hotkey = obs_data_create_from_json(str);
			else
				hotkey = nullptr;

			replayBuffer = obs_output_create("replay_buffer", Str("ReplayBuffer"), nullptr, hotkey);
			if(!replayBuffer)
				throw "Failed to create replay buffer output (simple output)";

			signal_handler_t* signal = obs_output_get_signal_handler(replayBuffer);
			startReplayBuffer.Connect(signal, "start", &AFOutputUtil::OBSStartReplayBuffer, this);
			stopReplayBuffer.Connect(signal, "stop", &AFOutputUtil::OBSStopReplayBuffer, this);
			replayBufferStopping.Connect(signal, "stopping", &AFOutputUtil::OBSReplayBufferStopping, this);
			replayBufferSaved.Connect(signal, "saved", &AFOutputUtil::OBSReplayBufferSaved, this);
		}

		bool use_native = strcmp(recFormat, "hybrid_mp4") == 0;
		fileOutput = obs_output_create(use_native ? "mp4_output" : "ffmpeg_muxer", "simple_file_output",
									   nullptr, nullptr);
		if(!fileOutput)
			throw "Failed to create recording output (simple output)";
	}

	startRecording.Connect(obs_output_get_signal_handler(fileOutput), "start", &AFOutputUtil::OBSStartRecording, this);
	stopRecording.Connect(obs_output_get_signal_handler(fileOutput), "stop", &AFOutputUtil::OBSStopRecording, this);
	recordStopping.Connect(obs_output_get_signal_handler(fileOutput), "stopping", &AFOutputUtil::OBSRecordStopping, this);
}
//
int AFSimpleOutput::CalcCRF(int crf)
{
	auto config = ACTIVECONFIG;
	//
	int cx = config_get_uint(config, "Video", "OutputCX");
	int cy = config_get_uint(config, "Video", "OutputCY");
	double fCX = double(cx);
	double fCY = double(cy);

	if(m_lowCPUx264)
		crf -= 2;

	double crossDist = std::sqrt(fCX * fCX + fCY * fCY);
	double crfResReduction = std::fmin(CROSS_DIST_CUTOFF, crossDist) / CROSS_DIST_CUTOFF;
	crfResReduction = (1.0 - crfResReduction) * 10.0;

	return crf - int(crfResReduction);
}
void AFSimpleOutput::UpdateRecordingSettings_x264_crf(int crf)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "crf", crf);
	obs_data_set_bool(settings, "use_bufsize", true);
	obs_data_set_string(settings, "rate_control", "CRF");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_string(settings, "preset", m_lowCPUx264 ? "ultrafast" : "veryfast");
	//
	obs_encoder_update(m_videoRecording, settings);
}
void AFSimpleOutput::UpdateRecordingSettings_qsv11(int crf, bool av1)
{
	bool icq = icq_available(m_videoRecording);

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "profile", "high");

	if(icq && !av1) {
		obs_data_set_string(settings, "rate_control", "ICQ");
		obs_data_set_int(settings, "icq_quality", crf);
	} else {
		obs_data_set_string(settings, "rate_control", "CQP");
		obs_data_set_int(settings, "cqp", crf);
	}

	obs_encoder_update(m_videoRecording, settings);
}
void AFSimpleOutput::UpdateRecordingSettings_nvenc(int cqp)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CQP");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_int(settings, "cqp", cqp);

	obs_encoder_update(m_videoRecording, settings);
}
void AFSimpleOutput::UpdateRecordingSettings_nvenc_hevc_av1(int cqp)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CQP");
	obs_data_set_string(settings, "profile", "main");
	obs_data_set_int(settings, "cqp", cqp);

	obs_encoder_update(m_videoRecording, settings);
}
void AFSimpleOutput::UpdateRecordingSettings_amd_cqp(int cqp)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CQP");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_string(settings, "preset", "quality");
	obs_data_set_int(settings, "cqp", cqp);
	obs_encoder_update(m_videoRecording, settings);
}
#ifdef __APPLE__
void AFSimpleOutput::UpdateRecordingSettings_apple(int quality)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CRF");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_int(settings, "quality", quality);

	obs_encoder_update(m_videoRecording, settings);
}

#ifdef ENABLE_HEVC
void AFSimpleOutput::UpdateRecordingSettings_apple_hevc(int quality)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CRF");
	obs_data_set_string(settings, "profile", "main");
	obs_data_set_int(settings, "quality", quality);

	obs_encoder_update(m_videoRecording, settings);
}
#endif // ENABLE_HEVC
#endif // __APPLE__

void AFSimpleOutput::UpdateRecordingSettings()
{
	bool ultra_hq = (m_videoQuality == "HQ");
	int crf = CalcCRF(ultra_hq ? 16 : 23);

	if(astrcmp_n(m_videoEncoder.c_str(), "x264", 4) == 0) {
		UpdateRecordingSettings_x264_crf(crf);

	} else if(m_videoEncoder == SIMPLE_ENCODER_QSV) {
		UpdateRecordingSettings_qsv11(crf, false);

	} else if(m_videoEncoder == SIMPLE_ENCODER_QSV_AV1) {
		UpdateRecordingSettings_qsv11(crf, true);

	} else if(m_videoEncoder == SIMPLE_ENCODER_AMD) {
		UpdateRecordingSettings_amd_cqp(crf);

#ifdef ENABLE_HEVC
	} else if(m_videoEncoder == SIMPLE_ENCODER_AMD_HEVC) {
		UpdateRecordingSettings_amd_cqp(crf);
#endif // ENABLE_HEVC

	} else if(m_videoEncoder == SIMPLE_ENCODER_AMD_AV1) {
		UpdateRecordingSettings_amd_cqp(crf);

	} else if(m_videoEncoder == SIMPLE_ENCODER_NVENC) {
		UpdateRecordingSettings_nvenc(crf);

#ifdef ENABLE_HEVC
	} else if(m_videoEncoder == SIMPLE_ENCODER_NVENC_HEVC) {
		UpdateRecordingSettings_nvenc_hevc_av1(crf);
#endif // ENABLE_HEVC
	} else if(m_videoEncoder == SIMPLE_ENCODER_NVENC_AV1) {
		UpdateRecordingSettings_nvenc_hevc_av1(crf);

#ifdef __APPLE__
	}
	else if(m_videoEncoder == SIMPLE_ENCODER_APPLE_H264) {
		/* These are magic numbers. 0 - 100, more is better. */
		UpdateRecordingSettings_apple(ultra_hq ? 70 : 50);
#ifdef ENABLE_HEVC
	} else if(m_videoEncoder == SIMPLE_ENCODER_APPLE_HEVC) {
		UpdateRecordingSettings_apple_hevc(ultra_hq ? 70 : 50);
#endif // ENABLE_HEVC
#endif // __APPLE__
	}
	UpdateRecordingAudioSettings();
}
void AFSimpleOutput::UpdateRecordingAudioSettings()
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "bitrate", 192);
	obs_data_set_string(settings, "rate_control", "CBR");

	auto config = ACTIVECONFIG;
	//
	int tracks = config_get_int(config, "SimpleOutput", "RecTracks");
	const char* recFormat = config_get_string(config, "SimpleOutput", "RecFormat2");
	const char* quality = config_get_string(config, "SimpleOutput", "RecQuality");
	bool flv = strcmp(recFormat, "flv") == 0;

	if(flv || strcmp(quality, "Stream") == 0) {
		obs_encoder_update(m_audioRecording, settings);
	} else {
		for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
			if((tracks & (1 << i)) != 0) {
				obs_encoder_update(m_audioTrack[i], settings);
			}
		}
	}
}
void AFSimpleOutput::Update()
{
	auto config = ACTIVECONFIG;
	//
	OBSDataAutoRelease videoSettings = obs_data_create();
	OBSDataAutoRelease audioSettings = obs_data_create();

	int videoBitrate = config_get_uint(config, "SimpleOutput", "VBitrate");
	int audioBitrate = GetAudioBitrate();
	bool advanced = config_get_bool(config, "SimpleOutput", "UseAdvanced");
	bool enforceBitrate = !config_get_bool(config, "Stream1", "IgnoreRecommended");
	const char* custom = config_get_string(config, "SimpleOutput", "x264Settings");
	const char* encoder = config_get_string(config, "SimpleOutput", "StreamEncoder");
	const char* encoder_id = obs_encoder_get_id(m_videoStreaming);
	const char* presetType;
	const char* preset;

	if(strcmp(encoder, SIMPLE_ENCODER_QSV) == 0) {
		presetType = "QSVPreset";

	} else if(strcmp(encoder, SIMPLE_ENCODER_QSV_AV1) == 0) {
		presetType = "QSVPreset";

	} else if(strcmp(encoder, SIMPLE_ENCODER_AMD) == 0) {
		presetType = "AMDPreset";

#ifdef ENABLE_HEVC
	} else if(strcmp(encoder, SIMPLE_ENCODER_AMD_HEVC) == 0) {
		presetType = "AMDPreset";
#endif // ENABLE_HEVC

	} else if(strcmp(encoder, SIMPLE_ENCODER_NVENC) == 0) {
		presetType = "NVENCPreset2";

#ifdef ENABLE_HEVC
	} else if(strcmp(encoder, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
		presetType = "NVENCPreset2";
#endif // ENABLE_HEVC

	} else if(strcmp(encoder, SIMPLE_ENCODER_AMD_AV1) == 0) {
		presetType = "AMDAV1Preset";

	} else if(strcmp(encoder, SIMPLE_ENCODER_NVENC_AV1) == 0) {
		presetType = "NVENCPreset2";

	} else {
		presetType = "Preset";
	}

	preset = config_get_string(config, "SimpleOutput", presetType);

	/* Only use preset2 for legacy/FFmpeg NVENC Encoder. */
	if(strncmp(encoder_id, "ffmpeg_", 7) == 0 && strcmp(presetType, "NVENCPreset2") == 0) {
		obs_data_set_string(videoSettings, "preset2", preset);
	} else {
		obs_data_set_string(videoSettings, "preset", preset);
	}

	obs_data_set_string(videoSettings, "rate_control", "CBR");
	obs_data_set_int(videoSettings, "bitrate", videoBitrate);

	if(advanced)
		obs_data_set_string(videoSettings, "x264opts", custom);

	obs_data_set_string(audioSettings, "rate_control", "CBR");
	obs_data_set_int(audioSettings, "bitrate", audioBitrate);

	obs_service_apply_encoder_settings(SERVICE_MANAGER.GetService(), videoSettings, audioSettings);

	if(!enforceBitrate) {
		blog(LOG_INFO, "User is ignoring service bitrate limits.");
		obs_data_set_int(videoSettings, "bitrate", videoBitrate);
		obs_data_set_int(audioSettings, "bitrate", audioBitrate);
	}

	video_t* video = obs_get_video();
	enum video_format format = video_output_get_format(video);

	switch(format) {
		case VIDEO_FORMAT_I420:
		case VIDEO_FORMAT_NV12:
		case VIDEO_FORMAT_I010:
		case VIDEO_FORMAT_P010:
			break;
		default:
			obs_encoder_set_preferred_video_format(m_videoStreaming, VIDEO_FORMAT_NV12);
	}

	obs_encoder_update(m_videoStreaming, videoSettings);
	obs_encoder_update(m_audioStreaming, audioSettings);
	obs_encoder_update(m_audioArchive, audioSettings);
}
inline void AFSimpleOutput::SetupOutputs()
{
	auto config = ACTIVECONFIG;
	//
	AFSimpleOutput::Update();
	obs_encoder_set_video(m_videoStreaming, obs_get_video());
	obs_encoder_set_audio(m_audioStreaming, obs_get_audio());
	obs_encoder_set_audio(m_audioArchive, obs_get_audio());
	int tracks = config_get_int(config, "SimpleOutput", "RecTracks");
	const char* recFormat = config_get_string(config, "SimpleOutput", "RecFormat2");
	bool flv = strcmp(recFormat, "flv") == 0;

	if(m_usingRecordingPreset) {
		if(m_ffmpegOutput) {
			obs_output_set_media(fileOutput, obs_get_video(), obs_get_audio());
		} else {
			obs_encoder_set_video(m_videoRecording, obs_get_video());
			if(flv) {
				obs_encoder_set_audio(m_audioRecording, obs_get_audio());
			} else {
				for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
					if((tracks & (1 << i)) != 0) {
						obs_encoder_set_audio(m_audioTrack[i], obs_get_audio());
					}
				}
			}
		}
	} else {
		obs_encoder_set_audio(m_audioRecording, obs_get_audio());
	}
}
int AFSimpleOutput::GetAudioBitrate() const
{
	auto config = ACTIVECONFIG;
	//
	const char* audio_encoder = config_get_string(config, "SimpleOutput", "StreamAudioEncoder");
	int bitrate = (int)config_get_uint(config, "SimpleOutput", "ABitrate");

	if(strcmp(audio_encoder, "opus") == 0)
		return FindClosestAvailableSimpleOpusBitrate(bitrate);

	return FindClosestAvailableSimpleAACBitrate(bitrate);
}
void AFSimpleOutput::LoadRecordingPreset_Lossy(const char* encoderId)
{
	m_videoRecording = obs_video_encoder_create(encoderId, "simple_video_recording", nullptr, nullptr);
	if(!m_videoRecording)
		throw "Failed to create video recording encoder (simple output)";

	obs_encoder_release(m_videoRecording);
}
void AFSimpleOutput::LoadRecordingPreset_Lossless()
{
	fileOutput = obs_output_create("ffmpeg_output", "simple_ffmpeg_output", nullptr, nullptr);
	if(!fileOutput)
		throw "Failed to create recording FFmpeg output (simple output)";

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "format_name", "avi");
	obs_data_set_string(settings, "video_encoder", "utvideo");
	obs_data_set_string(settings, "audio_encoder", "pcm_s16le");

	obs_output_update(fileOutput, settings);
}
void AFSimpleOutput::LoadRecordingPreset()
{
	auto config = ACTIVECONFIG;
	//
	const char* quality = config_get_string(config, "SimpleOutput", "RecQuality");
	const char* encoder = config_get_string(config, "SimpleOutput", "RecEncoder");
	const char* audio_encoder = config_get_string(config, "SimpleOutput", "RecAudioEncoder");

	m_videoEncoder = encoder;
	m_videoQuality = quality;
	m_ffmpegOutput = false;

	if(strcmp(quality, "Stream") == 0) {
		m_videoRecording = m_videoStreaming;
		m_audioRecording = m_audioStreaming;
		m_usingRecordingPreset = false;
		return;

	} else if(strcmp(quality, "Lossless") == 0) {
		LoadRecordingPreset_Lossless();
		m_usingRecordingPreset = true;
		m_ffmpegOutput = true;
		return;

	} else {
		m_lowCPUx264 = false;

		if(strcmp(encoder, SIMPLE_ENCODER_X264_LOWCPU) == 0)
			m_lowCPUx264 = true;
		LoadRecordingPreset_Lossy(get_simple_output_encoder(encoder));
		m_usingRecordingPreset = true;

		bool success = false;

		if(strcmp(audio_encoder, "opus") == 0)
			success = AFEncoderUtil::CreateSimpleOpusEncoder(m_audioRecording, 192, "simple_opus_recording", 0);
		else
			success = AFEncoderUtil::CreateSimpleAACEncoder(m_audioRecording, 192, "simple_aac_recording", 0);

		if(!success)
			throw "Failed to create audio recording encoder (simple output)";

		for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
			char name[23];
			if(strcmp(audio_encoder, "opus") == 0) {
				snprintf(name, sizeof name, "simple_opus_recording%d", i);
				success = AFEncoderUtil::CreateSimpleOpusEncoder(m_audioTrack[i], GetAudioBitrate(), name, i);
			} else {
				snprintf(name, sizeof name, "simple_aac_recording%d", i);
				success = AFEncoderUtil::CreateSimpleAACEncoder(m_audioTrack[i], GetAudioBitrate(), name, i);
			}

			if(!success)
				throw "Failed to create multi-track audio recording encoder (simple output)";
		}
	}
}
void AFSimpleOutput::LoadStreamingPreset_Lossy(const char* encoderId)
{
	m_videoStreaming = obs_video_encoder_create(encoderId, "simple_video_stream", nullptr, nullptr);
	if(!m_videoStreaming)
		throw "Failed to create video streaming encoder (simple output)";
	obs_encoder_release(m_videoStreaming);
}
void AFSimpleOutput::UpdateRecording()
{
	auto config = ACTIVECONFIG;
	//
	const char* recFormat = config_get_string(config, "SimpleOutput", "RecFormat2");
	bool flv = strcmp(recFormat, "flv") == 0;
	int tracks = config_get_int(config, "SimpleOutput", "RecTracks");
	int idx = 0;
	int idx2 = 0;
	const char* quality = config_get_string(config, "SimpleOutput", "RecQuality");

	if(replayBufferActive || recordingActive)
		return;

	if(m_usingRecordingPreset) {
		if(!m_ffmpegOutput)
			UpdateRecordingSettings();
	} else if(!obs_output_active(streamOutput)) {
		Update();
	}

	if(!Active())
		SetupOutputs();

	if(!m_ffmpegOutput) {
		obs_output_set_video_encoder(fileOutput, m_videoRecording);
		if(flv || strcmp(quality, "Stream") == 0) {
			obs_output_set_audio_encoder(fileOutput, m_audioRecording, 0);
		} else {
			for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
				if((tracks & (1 << i)) != 0) {
					obs_output_set_audio_encoder(fileOutput, m_audioTrack[i], idx++);
				}
			}
		}
	}
	if(replayBuffer) {
		obs_output_set_video_encoder(replayBuffer, m_videoRecording);
		if(flv || strcmp(quality, "Stream") == 0) {
			obs_output_set_audio_encoder(replayBuffer, m_audioRecording, 0);
		} else {
			for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
				if((tracks & (1 << i)) != 0) {
					obs_output_set_audio_encoder(replayBuffer, m_audioTrack[i], idx2++);
				}
			}
		}
	}

	m_recordingConfigured = true;
}
bool AFSimpleOutput::ConfigureRecording(bool updateReplayBuffer)
{
	auto config = ACTIVECONFIG;
	//
	const char* path = config_get_string(config, "SimpleOutput", "FilePath");
	const char* format = config_get_string(config, "SimpleOutput", "RecFormat2");
	const char* mux = config_get_string(config, "SimpleOutput", "MuxerCustom");
	bool noSpace = config_get_bool(config, "SimpleOutput", "FileNameWithoutSpace");
	const char* filenameFormat = config_get_string(config, "Output", "FilenameFormatting");
	bool overwriteIfExists = config_get_bool(config, "Output", "OverwriteIfExists");
	const char* rbPrefix = config_get_string(config, "SimpleOutput", "RecRBPrefix");
	const char* rbSuffix = config_get_string(config, "SimpleOutput", "RecRBSuffix");
	int rbTime = config_get_int(config, "SimpleOutput", "RecRBTime");
	int rbSize = config_get_int(config, "SimpleOutput", "RecRBSize");
	int tracks = config_get_int(config, "SimpleOutput", "RecTracks");

	bool is_fragmented = strncmp(format, "fragmented", 10) == 0;
	bool is_lossless = m_videoQuality == "Lossless";

	std::string f;

	OBSDataAutoRelease settings = obs_data_create();
	if(updateReplayBuffer) {
		f = GetFormatString(filenameFormat, rbPrefix, rbSuffix);
		std::string ext = GetFormatExt(format);
		obs_data_set_string(settings, "directory", path);
		obs_data_set_string(settings, "format", f.c_str());
		obs_data_set_string(settings, "extension", ext.c_str());
		obs_data_set_bool(settings, "allow_spaces", !noSpace);
		obs_data_set_int(settings, "max_time_sec", rbTime);
		obs_data_set_int(settings, "max_size_mb",
				 m_usingRecordingPreset ? rbSize : 0);
	} else {
		f = GetFormatString(filenameFormat, nullptr, nullptr);
		std::string strPath = GetRecordingFilename(path, m_ffmpegOutput ? "avi" : format, noSpace,
												   overwriteIfExists, f.c_str(), m_ffmpegOutput);
		obs_data_set_string(settings, m_ffmpegOutput ? "url" : "path", strPath.c_str());
		if(m_ffmpegOutput)
			obs_output_set_mixers(fileOutput, tracks);
	}

	// Use fragmented MOV/MP4 if user has not already specified custom movflags
	if(is_fragmented && !is_lossless &&
		(!mux || strstr(mux, "movflags") == NULL)) {
		std::string mux_frag = "movflags=frag_keyframe+empty_moov+delay_moov";
		if(mux) {
			mux_frag += " ";
			mux_frag += mux;
		}
		obs_data_set_string(settings, "muxer_settings", mux_frag.c_str());
	} else {
		if(is_fragmented && !is_lossless)
			blog(LOG_WARNING, "User enabled fragmented recording, "
				 "but custom muxer settings contained movflags.");
		obs_data_set_string(settings, "muxer_settings", mux);
	}

	if(updateReplayBuffer)
		obs_output_update(replayBuffer, settings);
	else
		obs_output_update(fileOutput, settings);

	return true;
}

bool AFSimpleOutput::IsVodTrackEnabled(obs_service_t* service)
{
	auto config = ACTIVECONFIG;
	//
	bool advanced = config_get_bool(config, "SimpleOutput", "UseAdvanced");
	bool enable = config_get_bool(config, "SimpleOutput", "VodTrackEnabled");
	bool enableForCustomServer = config_get_bool(config, "General", "EnableCustomServerVodTrack");

	OBSDataAutoRelease settings = obs_service_get_settings(service);
	const char* name = obs_data_get_string(settings, "service");

	const char* id = obs_service_get_id(service);
	if(strcmp(id, "rtmp_custom") == 0)
		return enableForCustomServer ? enable : false;
	else
		return advanced && enable && AFEncoderUtil::ServiceSupportsVodTrack(name);
}
void AFSimpleOutput::SetupVodTrack(obs_service_t* service)
{
	if(IsVodTrackEnabled(service))
		obs_output_set_audio_encoder(streamOutput, m_audioArchive, 1);
	else
		AFEncoderUtil::clear_archive_encoder(streamOutput, SIMPLE_ARCHIVE_NAME);
}
//
bool AFSimpleOutput::SetupStreaming(obs_service_t* service)
{
	if(!Active())
		SetupOutputs();

	/* --------------------- */

	const char* type = AFEncoderUtil::GetStreamOutputType(service);
	if(!type)
		return false;

	/* XXX: this is messy and disgusting and should be refactored */
	if(outputType != type) {
		streamDelayStarting.Disconnect();
		streamStopping.Disconnect();
		startStreaming.Disconnect();
		stopStreaming.Disconnect();

		streamOutput = obs_output_create(type, "simple_stream", nullptr, nullptr);
		if(!streamOutput) {
			blog(LOG_WARNING, "Creation of stream output type '%s' failed!", type);
			return false;
		}

		streamDelayStarting.Connect(obs_output_get_signal_handler(streamOutput), "starting", &AFOutputUtil::OBSStreamStarting, this);
		streamStopping.Connect(obs_output_get_signal_handler(streamOutput), "stopping", &AFOutputUtil::OBSStreamStopping, this);

		startStreaming.Connect(obs_output_get_signal_handler(streamOutput), "start", &AFOutputUtil::OBSStartStreaming, this);
		stopStreaming.Connect(obs_output_get_signal_handler(streamOutput), "stop", &AFOutputUtil::OBSStopStreaming, this);

		outputType = type;
	}

	obs_output_set_video_encoder(streamOutput, m_videoStreaming);
	obs_output_set_audio_encoder(streamOutput, m_audioStreaming, 0);
	obs_output_set_service(streamOutput, service);
	return true;
}

bool AFSimpleOutput::StartStreaming(obs_service_t* service)
{
	auto config = ACTIVECONFIG;
	//
	bool reconnect = config_get_bool(config, "Output", "Reconnect");
	int retryDelay = config_get_uint(config, "Output", "RetryDelay");
	int maxRetries = config_get_uint(config, "Output", "MaxRetries");
	bool useDelay = config_get_bool(config, "Output", "DelayEnable");
	int delaySec = config_get_int(config, "Output", "DelaySec");
	bool preserveDelay = config_get_bool(config, "Output", "DelayPreserve");
	const char* bindIP = config_get_string(config, "Output", "BindIP");
	const char* ipFamily = config_get_string(config, "Output", "IPFamily");
#ifdef _WIN32
	bool enableNewSocketLoop = config_get_bool(config, "Output", "NewSocketLoopEnable");
	bool enableLowLatencyMode = config_get_bool(config, "Output", "LowLatencyEnable");
#else
	bool enableNewSocketLoop = false;
#endif // _WIN32
	bool enableDynBitrate = config_get_bool(config, "Output", "DynamicBitrate");

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "bind_ip", bindIP);
	obs_data_set_string(settings, "ip_family", ipFamily);
#ifdef _WIN32
	obs_data_set_bool(settings, "new_socket_loop_enabled", enableNewSocketLoop);
	obs_data_set_bool(settings, "low_latency_mode_enabled", enableLowLatencyMode);
#endif // _WIN32
	obs_data_set_bool(settings, "dyn_bitrate", enableDynBitrate);

	auto streamOutput = StreamingOutput(); // shadowing is sort of bad, but also convenient

	obs_output_update(streamOutput, settings);

	if(!reconnect)
		maxRetries = 0;

	obs_output_set_delay(streamOutput, useDelay ? delaySec : 0, preserveDelay ? OBS_OUTPUT_DELAY_PRESERVE : 0);

	obs_output_set_reconnect_settings(streamOutput, maxRetries, retryDelay);

	SetupVodTrack(service);

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
bool AFSimpleOutput::StartRecording()
{
	UpdateRecording();
	if(!ConfigureRecording(false))
		return false;

	bool bResult = obs_output_start(fileOutput);
	if(false == bResult) {
		const char* error = obs_output_get_last_error(fileOutput);
		AFMainFrame::OBSErrorMessageBox(error,
										"Output.StartFailedGeneric",
										"Output.StartRecordingFailed");
	}
	return bResult;
}
bool AFSimpleOutput::StartReplayBuffer()
{
	UpdateRecording();
	if(!ConfigureRecording(true))
		return false;
	
	bool bResult = obs_output_start(replayBuffer);
	if(false == bResult) {
		const char* error = obs_output_get_last_error(replayBuffer);
		AFMainFrame::OBSErrorMessageBox(error,
										"Output.StartFailedGeneric",
										"Output.StartReplayFailed");
	}
	return bResult;
}
void AFSimpleOutput::StopStreaming(bool force)
{
	if(force)
		obs_output_force_stop(streamOutput);
	else
		obs_output_stop(streamOutput);
}
void AFSimpleOutput::StopRecording(bool force)
{
	if(force)
		obs_output_force_stop(fileOutput);
	else
		obs_output_stop(fileOutput);
}
void AFSimpleOutput::StopReplayBuffer(bool force)
{
	if(force)
		obs_output_force_stop(replayBuffer);
	else
		obs_output_stop(replayBuffer);
}
bool AFSimpleOutput::StreamingActive() const
{
	return obs_output_active(streamOutput);
}
bool AFSimpleOutput::RecordingActive() const
{
	return obs_output_active(fileOutput);
}

bool AFSimpleOutput::ReplayBufferActive() const
{
	return obs_output_active(replayBuffer);
}
//
bool AFSimpleOutput::icq_available(obs_encoder_t* encoder)
{
	obs_properties_t* props = obs_encoder_properties(encoder);
	obs_property_t* p = obs_properties_get(props, "rate_control");
	bool icq_found = false;

	size_t num = obs_property_list_item_count(p);
	for(size_t i = 0; i < num; i++) {
		const char* val = obs_property_list_item_string(p, i);
		if(strcmp(val, "ICQ") == 0) {
			icq_found = true;
			break;
		}
	}

	obs_properties_destroy(props);
	return icq_found;
}