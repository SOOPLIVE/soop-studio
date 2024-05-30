#pragma once

#include <util/dstr.hpp>

inline enum obs_scale_type AFVideoUtil::_GetScaleType(config_t* basicConfig)
{
    const char* scaleTypeStr = config_get_string(basicConfig, "Video", "ScaleType");
    if(0 == astrcmpi(scaleTypeStr, "bilinear")) {
        return OBS_SCALE_BILINEAR;
    } else if(0 == astrcmpi(scaleTypeStr, "lanczos")) {
        return OBS_SCALE_LANCZOS;
    } else if(0 == astrcmpi(scaleTypeStr, "area")) {
        return OBS_SCALE_AREA;
    } else {
        return OBS_SCALE_BICUBIC;
    }
};

inline enum video_format AFVideoUtil::_GetVideoFormatFromName(const char* name)
{
    if(0 == astrcmpi(name, "I420")) {
        return VIDEO_FORMAT_I420;
    } else if(0 == astrcmpi(name, "NV12")) {
        return VIDEO_FORMAT_NV12;
    } else if(0 == astrcmpi(name, "I444")) {
        return VIDEO_FORMAT_I444;
    } else if(0 == astrcmpi(name, "I010")) {
        return VIDEO_FORMAT_I010;
    } else if(0 == astrcmpi(name, "P010")) {
        return VIDEO_FORMAT_P010;
    } else if(0 == astrcmpi(name, "P216")) {
        return VIDEO_FORMAT_P216;
    } else if(0 == astrcmpi(name, "P416")) {
        return VIDEO_FORMAT_P416;
    }
#if 1 // currently unsupported
    else if(0 == astrcmpi(name, "YVYU")) {
        return VIDEO_FORMAT_YVYU;
    } else if(0 == astrcmpi(name, "YUY2")) {
        return VIDEO_FORMAT_YUY2;
    } else if(0 == astrcmpi(name, "UYVY")) {
        return VIDEO_FORMAT_UYVY;
    }
#endif //
    else {
        return VIDEO_FORMAT_BGRA;
    }
};

inline enum video_colorspace AFVideoUtil::_GetVideoColorSpaceFromName(const char* name)
{
    enum video_colorspace colorspace = VIDEO_CS_SRGB;
    if(0 == strcmp(name, "601")) {
        colorspace = VIDEO_CS_601;
    } else if(0 == strcmp(name, "709")) {
        colorspace = VIDEO_CS_709;
    } else if(0 == strcmp(name, "2100PQ")) {
        colorspace = VIDEO_CS_2100_PQ;
    } else if(0 == strcmp(name, "2100HLG")) {
        colorspace = VIDEO_CS_2100_HLG;
    }
    return colorspace;
};
