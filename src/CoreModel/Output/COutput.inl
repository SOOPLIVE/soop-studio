#pragma once

#include "Common/SettingsMiscDef.h"
#include "CoreModel/Encoder/CEncoder.inl"

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
#endif
	} else if(strcmp(encoder, SIMPLE_ENCODER_AMD_AV1) == 0) {
		return "av1_texture_amf";
	} else if(strcmp(encoder, SIMPLE_ENCODER_NVENC) == 0) {
		return EncoderAvailable("jim_nvenc") ? "jim_nvenc" : "ffmpeg_nvenc";
#ifdef ENABLE_HEVC
	} else if(strcmp(encoder, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
		return EncoderAvailable("jim_hevc_nvenc") ? "jim_hevc_nvenc" : "ffmpeg_hevc_nvenc";
#endif
	} else if(strcmp(encoder, SIMPLE_ENCODER_NVENC_AV1) == 0) {
		return "jim_av1_nvenc";
	} else if(strcmp(encoder, SIMPLE_ENCODER_APPLE_H264) == 0) {
		return "com.apple.videotoolbox.videoencoder.ave.avc";
#ifdef ENABLE_HEVC
	} else if(strcmp(encoder, SIMPLE_ENCODER_APPLE_HEVC) == 0) {
		return "com.apple.videotoolbox.videoencoder.ave.hevc";
#endif
	}

	return "obs_x264";
}