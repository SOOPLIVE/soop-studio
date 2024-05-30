#pragma once

#include <numeric>

#include "util/dstr.h"
#include "UIComponent/CAudioEncoders.h"

#define SIMPLE_ARCHIVE_NAME	"simple_archive_audio"

static bool CreateSimpleAACEncoder(OBSEncoder& res, int bitrate,
								   const char* name, size_t idx)
{
	const char* id_ = GetSimpleAACEncoderForBitrate(bitrate);
	if(!id_) {
		res = nullptr;
		return false;
	}

	res = obs_audio_encoder_create(id_, name, nullptr, idx, nullptr);
	if(res) {
		obs_encoder_release(res);
		return true;
	}
	return false;
}
static bool CreateSimpleOpusEncoder(OBSEncoder& res, int bitrate,
									const char* name, size_t idx)
{
	const char* id_ = GetSimpleOpusEncoderForBitrate(bitrate);
	if(!id_) {
		res = nullptr;
		return false;
	}

	res = obs_audio_encoder_create(id_, name, nullptr, idx, nullptr);
	if(res) {
		obs_encoder_release(res);
		return true;
	}
	return false;
}
static bool return_first_id(void* data, const char* id)
{
	const char** output = (const char**)data;

	*output = id;
	return false;
}
static inline bool can_use_output(const char* prot, const char* output,
								  const char* prot_test1,
								  const char* prot_test2 = nullptr)
{
	return (strcmp(prot, prot_test1) == 0 ||
			(prot_test2 && strcmp(prot, prot_test2) == 0)) &&
		(obs_get_output_flags(output) & OBS_OUTPUT_SERVICE) != 0;
}
static const char* GetStreamOutputType(const obs_service_t* service)
{
	const char* protocol = obs_service_get_protocol(service);
	const char* output = nullptr;

	if(!protocol) {
		blog(LOG_WARNING, "The service '%s' has no protocol set", obs_service_get_id(service));
		return nullptr;
	}

	if(!obs_is_output_protocol_registered(protocol)) {
		blog(LOG_WARNING, "The protocol '%s' is not registered", protocol);
		return nullptr;
	}

	/* Check if the service has a preferred output type */
	output = obs_service_get_preferred_output_type(service);
	if(output) {
		if((obs_get_output_flags(output) & OBS_OUTPUT_SERVICE) != 0)
			return output;

		blog(LOG_WARNING, "The output '%s' is not registered, fallback to another one", output);
	}

	/* Otherwise, prefer first-party output types */
	if(can_use_output(protocol, "rtmp_output", "RTMP", "RTMPS")) {
		return "rtmp_output";
	} else if(can_use_output(protocol, "ffmpeg_hls_muxer", "HLS")) {
		return "ffmpeg_hls_muxer";
	} else if(can_use_output(protocol, "ffmpeg_mpegts_muxer", "SRT",
							 "RIST")) {
		return "ffmpeg_mpegts_muxer";
	}

	/* If third-party protocol, use the first enumerated type */
	obs_enum_output_types_with_protocol(protocol, &output, return_first_id);
	if(output)
		return output;

	blog(LOG_WARNING,
		 "No output compatible with the service '%s' is registered",
		 obs_service_get_id(service));

	return nullptr;
}
static std::tuple<int, int> AspectRatio(int cx, int cy)
{
	int common = std::gcd(cx, cy);
	int newCX = cx / common;
	int newCY = cy / common;

	if(newCX == 8 && newCY == 5) {
		newCX = 16;
		newCY = 10;
	}

	return std::make_tuple(newCX, newCY);
}
static bool ReturnFirstId(void* data, const char* id)
{
	const char** output = (const char**)data;

	*output = id;
	return false;
}
static bool EncoderAvailable(const char* encoder)
{
	const char* val = nullptr;
	int i = 0;

	while(obs_enum_encoder_types(i++, &val))
		if(strcmp(val, encoder) == 0)
			return true;

	return false;
}
static bool ServiceSupportsCodec(const char** codecs, const char* codec)
{
	if(!codecs)
		return true;

	while(*codecs) {
		if(strcmp(*codecs, codec) == 0)
			return true;
		codecs++;
	}

	return false;
}
static bool ServiceSupportsEncoder(const char** codecs, const char* encoder)
{
	if(!EncoderAvailable(encoder))
		return false;

	const char* codec = obs_get_encoder_codec(encoder);
	return ServiceSupportsCodec(codecs, codec);
}
inline bool ServiceSupportsVodTrack(const char* service)
{
	static const char* vodTrackServices[] = {"Twitch"};

	for(const char* vodTrackService : vodTrackServices) {
		if(astrcmpi(vodTrackService, service) == 0)
			return true;
	}

	return false;
}
inline void clear_archive_encoder(obs_output_t* output, const char* expected_name)
{
	obs_encoder_t* last = obs_output_get_audio_encoder(output, 1);
	bool clear = false;

	/* ensures that we don't remove twitch's soundtrack encoder */
	if(last) {
		const char* name = obs_encoder_get_name(last);
		clear = name && strcmp(name, expected_name) == 0;
		obs_encoder_release(last);
	}

	if(clear)
		obs_output_set_audio_encoder(output, nullptr, 1);
}

#ifdef __APPLE__
static void translate_macvth264_encoder(const char *&encoder)
{
    if (strcmp(encoder, "vt_h264_hw") == 0) {
        encoder = "com.apple.videotoolbox.videoencoder.h264.gva";
    } else if (strcmp(encoder, "vt_h264_sw") == 0) {
        encoder = "com.apple.videotoolbox.videoencoder.h264";
    }
}
#endif
