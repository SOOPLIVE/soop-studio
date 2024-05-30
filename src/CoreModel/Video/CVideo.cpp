#include "CVideo.h"




#include <util/dstr.hpp>
#include <util/profiler.hpp>

#include "Blocks/SceneControlDock/CProjector.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Log/CLogManager.h"
#include "CoreModel/OBSOutput/COBSOutputContext.h"

#include "ViewModel/MainWindow/CMainWindowAccesser.h"
#include "CoreModel/Statistics/CStatistics.h"


int AFVideoUtil::ResetVideo()
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	auto& logManager = AFLogManager::GetSingletonInstance();

	auto& tmpOutputHandler = AFOBSOutputContext::GetSingletonInstance().GetMainOuputHandler();
	if (tmpOutputHandler && tmpOutputHandler->Active())
		return OBS_VIDEO_CURRENTLY_ACTIVE;

	ProfileScope("OBSBasic::ResetVideo");

	struct obs_video_info ovi;
	int ret = 0;
	_GetConfigFPS(ovi.fps_num, ovi.fps_den);

	const char* colorFormat = config_get_string(confManager.GetBasic(), "Video", "ColorFormat");
	const char* colorSpace = config_get_string(confManager.GetBasic(), "Video", "ColorSpace");
	const char* colorRange = config_get_string(confManager.GetBasic(), "Video", "ColorRange");
	//
	ovi.graphics_module = AFGraphicsContext::GetSingletonInstance().GetRenderModule();
	ovi.base_width = (uint32_t)config_get_uint(confManager.GetBasic(), "Video", "BaseCX");
	ovi.base_height = (uint32_t)config_get_uint(confManager.GetBasic(), "Video", "BaseCY");
	ovi.output_width = (uint32_t)config_get_uint(confManager.GetBasic(), "Video", "OutputCX");
	ovi.output_height = (uint32_t)config_get_uint(confManager.GetBasic(), "Video", "OutputCY");
	ovi.output_format = _GetVideoFormatFromName(colorFormat);
	ovi.colorspace = _GetVideoColorSpaceFromName(colorSpace);
	ovi.range = astrcmpi(colorRange, "Full") == 0 ? VIDEO_RANGE_FULL
												  : VIDEO_RANGE_PARTIAL;
	ovi.adapter = config_get_uint(confManager.GetGlobal(), "Video", "AdapterIdx");
	ovi.gpu_conversion = true;
	ovi.scale_type = _GetScaleType(confManager.GetBasic());
	if (ovi.base_width < 32 || ovi.base_height < 32)
	{
		ovi.base_width = 1920;
		ovi.base_height = 1080;
		config_set_uint(confManager.GetBasic(), "Video", "BaseCX", 1920);
		config_set_uint(confManager.GetBasic(), "Video", "BaseCY", 1080);
	}
	if (ovi.output_width < 32 || ovi.output_height < 32)
	{
		ovi.output_width = ovi.base_width;
		ovi.output_height = ovi.base_height;
		config_set_uint(confManager.GetBasic(), "Video", "OutputCX",
						ovi.base_width);
		config_set_uint(confManager.GetBasic(), "Video", "OutputCY",
						ovi.base_height);
	}
	ret = obs_reset_video(&ovi);
	if(OBS_VIDEO_CURRENTLY_ACTIVE == ret) {
		blog(LOG_WARNING, "Tried to reset when already active");
		return ret;
	}
    
    if (ret == OBS_VIDEO_SUCCESS)
    {
        auto& tmpViewModels = g_ViewModelsDynamic.GetInstance();
        tmpViewModels.m_RenderModel.ResizePreview(ovi.base_width, ovi.base_height);
        AFStateAppContext* tmpStateApp = AFConfigManager::GetSingletonInstance().GetStates();
        if (tmpStateApp->IsPreviewProgramMode())
            tmpViewModels.m_RenderModel.ResizeProgram(ovi.base_width, ovi.base_height);
        
		const float sdr_white_level = (float)config_get_uint(confManager.GetBasic(),
															 "Video", "SdrWhiteLevel");
		const float hdr_nominal_peak_level = (float)config_get_uint(confManager.GetBasic(),
																	"Video", "HdrNominalPeakLevel");
		obs_set_video_levels(sdr_white_level, hdr_nominal_peak_level);

		AFStatistics::InitializeValues();
        AFQProjector::UpdateMultiviewProjectors();
    }

	ret = 2;

	return ret;
}
//
void AFVideoUtil::_GetFPSCommon(uint32_t& num, uint32_t& den) const
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	const char* pVal = config_get_string(confManager.GetBasic(), "Video", "FPSCommon");
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
void AFVideoUtil::_GetFPSInteger(uint32_t& num, uint32_t& den) const
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	num = (uint32_t)config_get_uint(confManager.GetBasic(), "Video", "FPSInt");
	den = 1;
}

void AFVideoUtil::_GetFPSFraction(uint32_t& num, uint32_t& den) const
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	num = (uint32_t)config_get_uint(confManager.GetBasic(), "Video", "FPSNum");
	den = (uint32_t)config_get_uint(confManager.GetBasic(), "Video", "FPSDen");
}

void AFVideoUtil::_GetFPSNanoseconds(uint32_t& num, uint32_t& den) const
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	num = 1000000000;
	den = (uint32_t)config_get_uint(confManager.GetBasic(), "Video", "FPSNS");
}

void AFVideoUtil::_GetConfigFPS(uint32_t& num, uint32_t& den) const
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	uint32_t type = config_get_uint(confManager.GetBasic(), "Video", "FPSType");
	if(1 == type) { // "integer"
		_GetFPSInteger(num, den);
	} else if(2 == type) { // "fraction"
		_GetFPSFraction(num, den);
	}
    /*
     * 	else if (false) //"Nanoseconds", currently not implemented
     *		GetFPSNanoseconds(num, den);
     */
	else {
		_GetFPSCommon(num, den);
	}
}
