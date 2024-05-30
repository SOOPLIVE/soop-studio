
#include "SimpleOutput.h"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"

#include "CoreModel/Service/CService.h"

#include "Common/SettingsMiscDef.h"
#include "Common/StringMiscUtils.h"


#define CROSS_DIST_CUTOFF 2000.0


AFSimpleOutput::AFSimpleOutput(AFMainFrame* main)
	:AFBasicOutputHandler(main)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();
	//
	const char* encoder = config_get_string(confManager.GetBasic(), "SimpleOutput", "StreamEncoder");
	const char* audio_encoder = config_get_string(confManager.GetBasic(), "SimpleOutput", "StreamAudioEncoder");

	LoadStreamingPreset_Lossy(get_simple_output_encoder(encoder));

	bool success = false;
	if(strcmp(audio_encoder, "opus") == 0)
		success = CreateSimpleOpusEncoder(audioStreaming, GetAudioBitrate(), "simple_opus", 0);
	else
		success = CreateSimpleAACEncoder(audioStreaming, GetAudioBitrate(), "simple_aac", 0);

	if(!success)
		throw "Failed to create audio streaming encoder (simple output)";

	if(strcmp(audio_encoder, "opus") == 0)
		success = CreateSimpleOpusEncoder(audioArchive,
										  GetAudioBitrate(),
										  SIMPLE_ARCHIVE_NAME, 1);
	else
		success = CreateSimpleAACEncoder(audioArchive,
										 GetAudioBitrate(),
										 SIMPLE_ARCHIVE_NAME, 1);
	if(!success)
		throw "Failed to create audio archive encoder (simple output)";

	LoadRecordingPreset();

	if(!ffmpegOutput) {
		bool useReplayBuffer = config_get_bool(confManager.GetBasic(), "SimpleOutput", "RecRB");
		if(useReplayBuffer) {
			OBSDataAutoRelease hotkey;
			const char* str = config_get_string(
				confManager.GetBasic(), "Hotkeys", "ReplayBuffer");
			if(str)
				hotkey = obs_data_create_from_json(str);
			else
				hotkey = nullptr;

			replayBuffer = obs_output_create("replay_buffer",
											 localeTextManager.Str("ReplayBuffer"),
											 nullptr, hotkey);

			if(!replayBuffer)
				throw "Failed to create replay buffer output "
				"(simple output)";

			signal_handler_t* signal =
				obs_output_get_signal_handler(replayBuffer);

			startReplayBuffer.Connect(signal, "start", &AFMainFrame::OBSStartReplayBuffer, this);
			stopReplayBuffer.Connect(signal, "stop", &AFMainFrame::OBSStopReplayBuffer, this);
			replayBufferStopping.Connect(signal, "stopping", &AFMainFrame::OBSReplayBufferStopping, this);
			replayBufferSaved.Connect(signal, "saved", &AFMainFrame::OBSReplayBufferSaved, this);
		}

		fileOutput = obs_output_create("ffmpeg_muxer", "simple_file_output", nullptr, nullptr);
		if(!fileOutput)
			throw "Failed to create recording output "
			"(simple output)";
	}

	startRecording.Connect(obs_output_get_signal_handler(fileOutput), "start", &AFMainFrame::OBSStartRecording, this);
	stopRecording.Connect(obs_output_get_signal_handler(fileOutput), "stop", &AFMainFrame::OBSStopRecording, this);
	recordStopping.Connect(obs_output_get_signal_handler(fileOutput), "stopping", &AFMainFrame::OBSRecordStopping, this);
}
AFSimpleOutput::~AFSimpleOutput()
{
}
//
int AFSimpleOutput::CalcCRF(int crf)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	int cx = config_get_uint(confManager.GetBasic(), "Video", "OutputCX");
	int cy = config_get_uint(confManager.GetBasic(), "Video", "OutputCY");
	double fCX = double(cx);
	double fCY = double(cy);

	if(lowCPUx264)
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
	obs_data_set_string(settings, "preset", lowCPUx264 ? "ultrafast" : "veryfast");
	//
	obs_encoder_update(videoRecording, settings);
}
void AFSimpleOutput::UpdateRecordingSettings_qsv11(int crf, bool av1)
{
	bool icq = icq_available(videoRecording);

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "profile", "high");

	if(icq && !av1) {
		obs_data_set_string(settings, "rate_control", "ICQ");
		obs_data_set_int(settings, "icq_quality", crf);
	} else {
		obs_data_set_string(settings, "rate_control", "CQP");
		obs_data_set_int(settings, "cqp", crf);
	}

	obs_encoder_update(videoRecording, settings);
}
void AFSimpleOutput::UpdateRecordingSettings_nvenc(int cqp)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CQP");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_int(settings, "cqp", cqp);

	obs_encoder_update(videoRecording, settings);
}
void AFSimpleOutput::UpdateRecordingSettings_nvenc_hevc_av1(int cqp)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CQP");
	obs_data_set_string(settings, "profile", "main");
	obs_data_set_int(settings, "cqp", cqp);

	obs_encoder_update(videoRecording, settings);
}
void AFSimpleOutput::UpdateRecordingSettings_amd_cqp(int cqp)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CQP");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_string(settings, "preset", "quality");
	obs_data_set_int(settings, "cqp", cqp);
	obs_encoder_update(videoRecording, settings);
}
void AFSimpleOutput::UpdateRecordingSettings_apple(int quality)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CRF");
	obs_data_set_string(settings, "profile", "high");
	obs_data_set_int(settings, "quality", quality);

	obs_encoder_update(videoRecording, settings);
}
#ifdef ENABLE_HEVC
void AFSimpleOutput::UpdateRecordingSettings_apple_hevc(int quality)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CRF");
	obs_data_set_string(settings, "profile", "main");
	obs_data_set_int(settings, "quality", quality);

	obs_encoder_update(videoRecording, settings);
}
#endif
void AFSimpleOutput::UpdateRecordingSettings()
{
	bool ultra_hq = (videoQuality == "HQ");
	int crf = CalcCRF(ultra_hq ? 16 : 23);

	if(astrcmp_n(videoEncoder.c_str(), "x264", 4) == 0) {
		UpdateRecordingSettings_x264_crf(crf);

	} else if(videoEncoder == SIMPLE_ENCODER_QSV) {
		UpdateRecordingSettings_qsv11(crf, false);

	} else if(videoEncoder == SIMPLE_ENCODER_QSV_AV1) {
		UpdateRecordingSettings_qsv11(crf, true);

	} else if(videoEncoder == SIMPLE_ENCODER_AMD) {
		UpdateRecordingSettings_amd_cqp(crf);

#ifdef ENABLE_HEVC
	} else if(videoEncoder == SIMPLE_ENCODER_AMD_HEVC) {
		UpdateRecordingSettings_amd_cqp(crf);
#endif

	} else if(videoEncoder == SIMPLE_ENCODER_AMD_AV1) {
		UpdateRecordingSettings_amd_cqp(crf);

	} else if(videoEncoder == SIMPLE_ENCODER_NVENC) {
		UpdateRecordingSettings_nvenc(crf);

#ifdef ENABLE_HEVC
	} else if(videoEncoder == SIMPLE_ENCODER_NVENC_HEVC) {
		UpdateRecordingSettings_nvenc_hevc_av1(crf);
#endif
	} else if(videoEncoder == SIMPLE_ENCODER_NVENC_AV1) {
		UpdateRecordingSettings_nvenc_hevc_av1(crf);

	} else if(videoEncoder == SIMPLE_ENCODER_APPLE_H264) {
		/* These are magic numbers. 0 - 100, more is better. */
		UpdateRecordingSettings_apple(ultra_hq ? 70 : 50);
#ifdef ENABLE_HEVC
	} else if(videoEncoder == SIMPLE_ENCODER_APPLE_HEVC) {
		UpdateRecordingSettings_apple_hevc(ultra_hq ? 70 : 50);
#endif
	}
	UpdateRecordingAudioSettings();
}
void AFSimpleOutput::UpdateRecordingAudioSettings()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "bitrate", 192);
	obs_data_set_string(settings, "rate_control", "CBR");

	int tracks = config_get_int(confManager.GetBasic(), "SimpleOutput", "RecTracks");
	const char* recFormat = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecFormat2");
	const char* quality = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecQuality");
	bool flv = strcmp(recFormat, "flv") == 0;

	if(flv || strcmp(quality, "Stream") == 0) {
		obs_encoder_update(audioRecording, settings);
	} else {
		for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
			if((tracks & (1 << i)) != 0) {
				obs_encoder_update(audioTrack[i], settings);
			}
		}
	}
}
void AFSimpleOutput::Update()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	OBSDataAutoRelease videoSettings = obs_data_create();
	OBSDataAutoRelease audioSettings = obs_data_create();

	int videoBitrate = config_get_uint(confManager.GetBasic(), "SimpleOutput", "VBitrate");
	int audioBitrate = GetAudioBitrate();
	bool advanced = config_get_bool(confManager.GetBasic(), "SimpleOutput", "UseAdvanced");
	bool enforceBitrate = !config_get_bool(confManager.GetBasic(), "Stream1", "IgnoreRecommended");
	const char* custom = config_get_string(confManager.GetBasic(), "SimpleOutput", "x264Settings");
	const char* encoder = config_get_string(confManager.GetBasic(), "SimpleOutput", "StreamEncoder");
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
#endif

	} else if(strcmp(encoder, SIMPLE_ENCODER_NVENC) == 0) {
		presetType = "NVENCPreset2";

#ifdef ENABLE_HEVC
	} else if(strcmp(encoder, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
		presetType = "NVENCPreset2";
#endif

	} else if(strcmp(encoder, SIMPLE_ENCODER_AMD_AV1) == 0) {
		presetType = "AMDAV1Preset";

	} else if(strcmp(encoder, SIMPLE_ENCODER_NVENC_AV1) == 0) {
		presetType = "NVENCPreset2";

	} else {
		presetType = "Preset";
	}

	preset = config_get_string(confManager.GetBasic(), "SimpleOutput", presetType);
	obs_data_set_string(videoSettings,
						(strcmp(presetType, "NVENCPreset2") == 0)
						? "preset2"
						: "preset",
						preset);

	obs_data_set_string(videoSettings, "rate_control", "CBR");
	obs_data_set_int(videoSettings, "bitrate", videoBitrate);

	if(advanced)
		obs_data_set_string(videoSettings, "x264opts", custom);

	obs_data_set_string(audioSettings, "rate_control", "CBR");
	obs_data_set_int(audioSettings, "bitrate", audioBitrate);

	obs_service_apply_encoder_settings(AFServiceManager::GetSingletonInstance().GetService(),
									   videoSettings, audioSettings);

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
			obs_encoder_set_preferred_video_format(videoStreaming,
								   VIDEO_FORMAT_NV12);
	}

	obs_encoder_update(videoStreaming, videoSettings);
	obs_encoder_update(audioStreaming, audioSettings);
	obs_encoder_update(audioArchive, audioSettings);
}
inline void AFSimpleOutput::SetupOutputs()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	AFSimpleOutput::Update();
	obs_encoder_set_video(videoStreaming, obs_get_video());
	obs_encoder_set_audio(audioStreaming, obs_get_audio());
	obs_encoder_set_audio(audioArchive, obs_get_audio());
	int tracks = config_get_int(confManager.GetBasic(), "SimpleOutput", "RecTracks");
	const char* recFormat = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecFormat2");
	bool flv = strcmp(recFormat, "flv") == 0;

	if(usingRecordingPreset) {
		if(ffmpegOutput) {
			obs_output_set_media(fileOutput, obs_get_video(),
						 obs_get_audio());
		} else {
			obs_encoder_set_video(videoRecording, obs_get_video());
			if(flv) {
				obs_encoder_set_audio(audioRecording,
							  obs_get_audio());
			} else {
				for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
					if((tracks & (1 << i)) != 0) {
						obs_encoder_set_audio(
							audioTrack[i],
							obs_get_audio());
					}
				}
			}
		}
	} else {
		obs_encoder_set_audio(audioRecording, obs_get_audio());
	}
}
int AFSimpleOutput::GetAudioBitrate() const
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	const char* audio_encoder = config_get_string(confManager.GetBasic(), "SimpleOutput", "StreamAudioEncoder");
	int bitrate = (int)config_get_uint(confManager.GetBasic(), "SimpleOutput", "ABitrate");

	if(strcmp(audio_encoder, "opus") == 0)
		return FindClosestAvailableSimpleOpusBitrate(bitrate);

	return FindClosestAvailableSimpleAACBitrate(bitrate);
}
void AFSimpleOutput::LoadRecordingPreset_Lossy(const char* encoderId)
{
	videoRecording = obs_video_encoder_create(encoderId, "simple_video_recording", nullptr, nullptr);
	if(!videoRecording)
		throw "Failed to create video recording encoder (simple output)";
	obs_encoder_release(videoRecording);
}
void AFSimpleOutput::LoadRecordingPreset_Lossless()
{
	fileOutput = obs_output_create("ffmpeg_output", "simple_ffmpeg_output", nullptr, nullptr);
	if(!fileOutput)
		throw "Failed to create recording FFmpeg output "
		"(simple output)";

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "format_name", "avi");
	obs_data_set_string(settings, "video_encoder", "utvideo");
	obs_data_set_string(settings, "audio_encoder", "pcm_s16le");

	obs_output_update(fileOutput, settings);
}
void AFSimpleOutput::LoadRecordingPreset()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	const char* quality = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecQuality");
	const char* encoder = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecEncoder");
	const char* audio_encoder = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecAudioEncoder");

	videoEncoder = encoder;
	videoQuality = quality;
	ffmpegOutput = false;

	if(strcmp(quality, "Stream") == 0) {
		videoRecording = videoStreaming;
		audioRecording = audioStreaming;
		usingRecordingPreset = false;
		return;

	} else if(strcmp(quality, "Lossless") == 0) {
		LoadRecordingPreset_Lossless();
		usingRecordingPreset = true;
		ffmpegOutput = true;
		return;

	} else {
		lowCPUx264 = false;

		if(strcmp(encoder, SIMPLE_ENCODER_X264_LOWCPU) == 0)
			lowCPUx264 = true;
		LoadRecordingPreset_Lossy(get_simple_output_encoder(encoder));
		usingRecordingPreset = true;

		bool success = false;

		if(strcmp(audio_encoder, "opus") == 0)
			success = CreateSimpleOpusEncoder(audioRecording, 192, "simple_opus_recording", 0);
		else
			success = CreateSimpleAACEncoder(audioRecording, 192, "simple_aac_recording", 0);
		if(!success)
			throw "Failed to create audio recording encoder "
			"(simple output)";

		for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
			char name[23];
			if(strcmp(audio_encoder, "opus") == 0) {
				snprintf(name, sizeof name, "simple_opus_recording%d", i);
				success = CreateSimpleOpusEncoder(audioTrack[i], GetAudioBitrate(), name, i);
			} else {
				snprintf(name, sizeof name, "simple_aac_recording%d", i);
				success = CreateSimpleAACEncoder(audioTrack[i], GetAudioBitrate(), name, i);
			}
			if(!success)
				throw "Failed to create multi-track audio recording encoder "
				"(simple output)";
		}
	}
}
void AFSimpleOutput::LoadStreamingPreset_Lossy(const char* encoderId)
{
	videoStreaming = obs_video_encoder_create(encoderId, "simple_video_stream", nullptr, nullptr);
	if(!videoStreaming)
		throw "Failed to create video streaming encoder (simple output)";
	obs_encoder_release(videoStreaming);
}
void AFSimpleOutput::UpdateRecording()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	const char* recFormat = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecFormat2");
	bool flv = strcmp(recFormat, "flv") == 0;
	int tracks = config_get_int(confManager.GetBasic(), "SimpleOutput", "RecTracks");
	int idx = 0;
	int idx2 = 0;
	const char* quality = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecQuality");

	if(replayBufferActive || recordingActive)
		return;

	if(usingRecordingPreset) {
		if(!ffmpegOutput)
			UpdateRecordingSettings();
	} else if(!obs_output_active(streamOutput)) {
		Update();
	}

	if(!Active())
		SetupOutputs();

	if(!ffmpegOutput) {
		obs_output_set_video_encoder(fileOutput, videoRecording);
		if(flv || strcmp(quality, "Stream") == 0) {
			obs_output_set_audio_encoder(fileOutput, audioRecording,
							 0);
		} else {
			for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
				if((tracks & (1 << i)) != 0) {
					obs_output_set_audio_encoder(
						fileOutput, audioTrack[i],
						idx++);
				}
			}
		}
	}
	if(replayBuffer) {
		obs_output_set_video_encoder(replayBuffer, videoRecording);
		if(flv || strcmp(quality, "Stream") == 0) {
			obs_output_set_audio_encoder(replayBuffer,
							 audioRecording, 0);
		} else {
			for(int i = 0; i < MAX_AUDIO_MIXES; i++) {
				if((tracks & (1 << i)) != 0) {
					obs_output_set_audio_encoder(
						replayBuffer, audioTrack[i],
						idx2++);
				}
			}
		}
	}

	recordingConfigured = true;
}
bool AFSimpleOutput::ConfigureRecording(bool updateReplayBuffer)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	const char* path = config_get_string(confManager.GetBasic(), "SimpleOutput", "FilePath");
	const char* format = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecFormat2");
	const char* mux = config_get_string(confManager.GetBasic(), "SimpleOutput", "MuxerCustom");
	bool noSpace = config_get_bool(confManager.GetBasic(), "SimpleOutput", "FileNameWithoutSpace");
	const char* filenameFormat = config_get_string(confManager.GetBasic(), "Output", "FilenameFormatting");
	bool overwriteIfExists = config_get_bool(confManager.GetBasic(), "Output", "OverwriteIfExists");
	const char* rbPrefix = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecRBPrefix");
	const char* rbSuffix = config_get_string(confManager.GetBasic(), "SimpleOutput", "RecRBSuffix");
	int rbTime = config_get_int(confManager.GetBasic(), "SimpleOutput", "RecRBTime");
	int rbSize = config_get_int(confManager.GetBasic(), "SimpleOutput", "RecRBSize");
	int tracks = config_get_int(confManager.GetBasic(), "SimpleOutput", "RecTracks");

	bool is_fragmented = strncmp(format, "fragmented", 10) == 0;
	bool is_lossless = videoQuality == "Lossless";

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
				 usingRecordingPreset ? rbSize : 0);
	} else {
		f = GetFormatString(filenameFormat, nullptr, nullptr);
		std::string strPath = GetRecordingFilename(path, ffmpegOutput ? "avi" : format, noSpace,
												   overwriteIfExists, f.c_str(), ffmpegOutput);
		obs_data_set_string(settings, ffmpegOutput ? "url" : "path", strPath.c_str());
		if(ffmpegOutput)
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
void AFSimpleOutput::SetupVodTrack(obs_service_t* service)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	bool advanced = config_get_bool(confManager.GetBasic(), "SimpleOutput", "UseAdvanced");
	bool enable = config_get_bool(confManager.GetBasic(), "SimpleOutput", "VodTrackEnabled");
	bool enableForCustomServer = config_get_bool(confManager.GetGlobal(), "General", "EnableCustomServerVodTrack");

	OBSDataAutoRelease settings = obs_service_get_settings(service);
	const char* name = obs_data_get_string(settings, "service");

	const char* id = obs_service_get_id(service);
	if(strcmp(id, "rtmp_custom") == 0)
		enable = enableForCustomServer ? enable : false;
	else
		enable = advanced && enable && ServiceSupportsVodTrack(name);

	if(enable)
		obs_output_set_audio_encoder(streamOutput, audioArchive, 1);
	else
		clear_archive_encoder(streamOutput, SIMPLE_ARCHIVE_NAME);
}
//
bool AFSimpleOutput::SetupStreaming(obs_service_t* service)
{
	if(!Active())
		SetupOutputs();

	AFMainFrame* main  = App()->GetMainView();
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

		streamOutput = obs_output_create(type, "simple_stream", nullptr, nullptr);
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
	obs_output_set_audio_encoder(streamOutput, audioStreaming, 0);
	obs_output_set_service(streamOutput, service);
	return true;
}
bool AFSimpleOutput::StartStreaming(obs_service_t* service)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	bool reconnect = config_get_bool(confManager.GetBasic(), "Output", "Reconnect");
	int retryDelay = config_get_uint(confManager.GetBasic(), "Output", "RetryDelay");
	int maxRetries = config_get_uint(confManager.GetBasic(), "Output", "MaxRetries");
	bool useDelay = config_get_bool(confManager.GetBasic(), "Output", "DelayEnable");
	int delaySec = config_get_int(confManager.GetBasic(), "Output", "DelaySec");
	bool preserveDelay = config_get_bool(confManager.GetBasic(), "Output", "DelayPreserve");
	const char* bindIP = config_get_string(confManager.GetBasic(), "Output", "BindIP");
	const char* ipFamily = config_get_string(confManager.GetBasic(), "Output", "IPFamily");
#ifdef _WIN32
	bool enableNewSocketLoop = config_get_bool(confManager.GetBasic(), "Output", "NewSocketLoopEnable");
	bool enableLowLatencyMode = config_get_bool(confManager.GetBasic(), "Output", "LowLatencyEnable");
#endif
	bool enableDynBitrate = config_get_bool(confManager.GetBasic(), "Output", "DynamicBitrate");

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
