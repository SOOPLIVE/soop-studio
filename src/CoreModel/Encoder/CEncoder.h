#pragma once

#include <obs.hpp>
#include <util/util.hpp>

#include <tuple>

#define SIMPLE_ARCHIVE_NAME	"simple_archive_audio"
//
namespace AFEncoderUtil
{
    bool CreateSimpleAACEncoder(OBSEncoder& res, int bitrate, const char* name, size_t idx);
    bool CreateSimpleOpusEncoder(OBSEncoder& res, int bitrate, const char* name, size_t idx);
    bool return_first_id(void* data, const char* id);
    bool can_use_output(const char* prot, const char* output,
                        const char* prot_test1,
                        const char* prot_test2 = nullptr);
    const char* GetStreamOutputType(const obs_service_t* service);
    std::tuple<int, int> AspectRatio(int cx, int cy);
    bool ReturnFirstId(void* data, const char* id);
    bool EncoderAvailable(const char* encoder);
    bool ServiceSupportsCodec(const char** codecs, const char* codec);
    bool ServiceSupportsEncoder(const char** codecs, const char* encoder);
    bool ServiceSupportsVodTrack(const char* service);
    void clear_archive_encoder(obs_output_t* output, const char* expected_name);

    bool isAV1Codec(const char* encoder);

    const char* getCurrentEncoder();

#ifdef __APPLE__
    void translate_macvth264_encoder(const char*& encoder);
#endif // __APPLE__
};