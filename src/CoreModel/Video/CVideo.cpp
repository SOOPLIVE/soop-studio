#include "CVideo.h"

#include <util/dstr.hpp>
#include <util/profiler.hpp>

#include "Application/CApplication.h"

#include "Blocks/SceneControlDock/CProjector.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Log/CLogManager.h"
#include "CoreModel/OBSOutput/COBSOutputContext.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Statistics/CStatistics.h"

#include "ViewModel/MainWindow/CMainWindowAccesser.h"
#include "ViewModel/MainWindow/CMainWindowRenderModel.h"


inline enum obs_scale_type GetScaleType()
{
	const char* scaleTypeStr = config_get_string(ACTIVECONFIG, "Video", "ScaleType");
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

inline enum video_format _GetVideoFormatFromName(const char* name)
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

inline enum video_colorspace _GetVideoColorSpaceFromName(const char* name)
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
//
void GetFPSCommon(uint32_t& num, uint32_t& den)
{
	auto config = ACTIVECONFIG;
	//
	const char* pVal = config_get_string(ACTIVECONFIG, "Video", "FPSCommon");
	if(0 == strcmp(pVal, "10")) {
		num = 10;
		den = 1;
	} else if(0 == strcmp(pVal, "20")) {
		num = 20;
		den = 1;
	} else if(0 == strcmp(pVal, "24 NTSC")) {
		num = 24000;
		den = 1001;
	} else if(0 == strcmp(pVal, "25 PAL")) {
		num = 25;
		den = 1;
	} else if(0 == strcmp(pVal, "29.97")) {
		num = 30000;
		den = 1001;
	} else if(0 == strcmp(pVal, "48")) {
		num = 48;
		den = 1;
	} else if(0 == strcmp(pVal, "50 PAL")) {
		num = 50;
		den = 1;
	} else if(0 == strcmp(pVal, "59.94")) {
		num = 60000;
		den = 1001;
	} else if(0 == strcmp(pVal, "60")) {
		num = 60;
		den = 1;
	} else {
		num = 30;
		den = 1;
	}
}
void GetFPSInteger(uint32_t& num, uint32_t& den)
{
	num = (uint32_t)config_get_uint(ACTIVECONFIG, "Video", "FPSInt");
	den = 1;
}

void GetFPSFraction(uint32_t& num, uint32_t& den)
{
	num = (uint32_t)config_get_uint(ACTIVECONFIG, "Video", "FPSNum");
	den = (uint32_t)config_get_uint(ACTIVECONFIG, "Video", "FPSDen");
}

void GetFPSNanoseconds(uint32_t& num, uint32_t& den)
{
	num = 1000000000;
	den = (uint32_t)config_get_uint(ACTIVECONFIG, "Video", "FPSNS");
}

void GetConfigFPS(uint32_t& num, uint32_t& den)
{
	uint32_t type = config_get_uint(ACTIVECONFIG, "Video", "FPSType");
	if(1 == type) { // "integer"
		GetFPSInteger(num, den);
	} else if(2 == type) { // "fraction"
		GetFPSFraction(num, den);
	}
    /*
     * 	else if (false) //"Nanoseconds", currently not implemented
     *		GetFPSNanoseconds(num, den);
     */
	else {
		GetFPSCommon(num, den);
	}
}

namespace AFVideoUtil {
    int ResetVideo()
    {
        //auto& tmpOutputHandler = OUTPUT_CONTEXT.GetMainOuputHandler();
        //if (tmpOutputHandler && tmpOutputHandler->Active())
        //	return OBS_VIDEO_CURRENTLY_ACTIVE;

        ProfileScope("OBSBasic::ResetVideo");

        struct obs_video_info ovi;
        int ret = 0;
        GetConfigFPS(ovi.fps_num, ovi.fps_den);

        config_t* activeConfig = ACTIVECONFIG;
		//
        const char* colorFormat = config_get_string(activeConfig, "Video", "ColorFormat");
        const char* colorSpace = config_get_string(activeConfig, "Video", "ColorSpace");
        const char* colorRange = config_get_string(activeConfig, "Video", "ColorRange");
        //
        ovi.graphics_module = GRAPHIC_CONTEXT.GetRenderModule();
        ovi.base_width = (uint32_t)config_get_uint(activeConfig, "Video", "BaseCX");
        ovi.base_height = (uint32_t)config_get_uint(activeConfig, "Video", "BaseCY");
        ovi.output_width = (uint32_t)config_get_uint(activeConfig, "Video", "OutputCX");
        ovi.output_height = (uint32_t)config_get_uint(activeConfig, "Video", "OutputCY");
        ovi.output_format = _GetVideoFormatFromName(colorFormat);
        ovi.colorspace = _GetVideoColorSpaceFromName(colorSpace);
        ovi.range = astrcmpi(colorRange, "Full") == 0 ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL;
        ovi.adapter = config_get_uint(APPCONFIG, "Video", "AdapterIdx");
        ovi.gpu_conversion = true;
        ovi.scale_type = GetScaleType();

        if(ovi.base_width < 32 || ovi.base_height < 32)
        {
            ovi.base_width = 1920;
            ovi.base_height = 1080;
            config_set_uint(activeConfig, "Video", "BaseCX", 1920);
            config_set_uint(activeConfig, "Video", "BaseCY", 1080);
        }

        if(ovi.output_width < 32 || ovi.output_height < 32)
        {
            ovi.output_width = ovi.base_width;
            ovi.output_height = ovi.base_height;
            config_set_uint(activeConfig, "Video", "OutputCX", ovi.base_width);
            config_set_uint(activeConfig, "Video", "OutputCY", ovi.base_height);
        }

		if(ovi.output_width != ovi.base_width || ovi.output_height != ovi.base_height)
		{
			ovi.base_width = ovi.output_width;
			ovi.base_height = ovi.output_height;
			config_set_uint(activeConfig, "Video", "BaseCX", ovi.base_width);
			config_set_uint(activeConfig, "Video", "BaseCY", ovi.base_height);
		}

        ret = obs_reset_video(&ovi);
        if(OBS_VIDEO_CURRENTLY_ACTIVE == ret) {
            blog(LOG_WARNING, "Tried to reset when already active");
            return ret;
        }

        if(ret == OBS_VIDEO_SUCCESS)
        {
			auto& tmpViewModels = g_viewModelsDynamic.GetInstance();
			tmpViewModels.m_renderModel.ResizePreview(ovi.base_width, ovi.base_height);
            if(STATEAPP.IsPreviewProgramMode())
				tmpViewModels.m_renderModel.ResizeProgram(ovi.base_width, ovi.base_height);

            const float sdr_white_level = (float)config_get_uint(activeConfig, "Video", "SdrWhiteLevel");
            const float hdr_nominal_peak_level = (float)config_get_uint(activeConfig, "Video", "HdrNominalPeakLevel");
            obs_set_video_levels(sdr_white_level, hdr_nominal_peak_level);

            AFStatistics::InitializeValues();
            AFQProjector::UpdateMultiviewProjectors();
			AFSceneContext::UpdateVideoSize(ovi.base_width, ovi.base_height);

			/*bool canMigrate = usingAbsoluteCoordinates ||
				(migrationBaseResolution && (migrationBaseResolution->first != ovi.base_width ||
											 migrationBaseResolution->second != ovi.base_height));*/
        }

        return ret;
    }
};