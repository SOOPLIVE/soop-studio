
#include "CProfile.h"
#include <util/dstr.hpp>
#include <util/profiler.hpp>

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Log/CLogManager.h"

#include "Common/SettingsMiscDef.h"

AFProfileUtil::AFProfileUtil()
{
}
AFProfileUtil::~AFProfileUtil()
{
}

//bool AFProfileUtil::CreateProfile(const std::string& newName, bool create_new,
//								  bool showWizardChecked, bool rename,
//								  ConfigFile& config, std::string& newDir)
//{
//	auto& confManager = AFConfigManager::GetSingletonInstance();
//	//
//	std::string newPath;
//	if(!_FindSafeProfileDirName(newName, newDir))
//		return false;
//
//	if(create_new) {
//		config_set_bool(confManager.GetGlobal(), "Basic", "ConfigOnNewProfile", showWizardChecked);
//	}
//
//	std::string curDir = config_get_string(confManager.GetGlobal(), "Basic", "ProfileDir");
//
//	char baseDir[512] = {0,};
//	int ret = confManager.GetConfigPath(baseDir, sizeof(baseDir), "SOOPStudio/basic/profiles/");
//	if(ret <= 0) {
//		blog(LOG_WARNING, "Failed to get profiles config path");
//		return false;
//	}
//
//	newPath = baseDir;
//	newPath += newDir;
//
//	if(os_mkdir(newPath.c_str()) < 0) {
//		blog(LOG_WARNING, "Failed to create profile directory '%s'", newDir.c_str());
//		return false;
//	}
//
//	if(!create_new)
//		CopyProfile(curDir.c_str(), newPath.c_str());
//
//	newPath += "/basic.ini";
//
//	if(config.Open(newPath.c_str(), CONFIG_OPEN_ALWAYS) != 0) {
//		blog(LOG_ERROR, "Failed to open new config file '%s'", newDir.c_str());
//		return false;
//	}
//    
////    if (api && !rename)
////        api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGING);
//    //
//    
//    
//    
//    
//	return true;
//}
//void AFProfileUtil::DeleteProfile(const char* profile_name, const char* profile_dir)
//{
//	auto& confManager = AFConfigManager::GetSingletonInstance();
//	//
//	char profilePath[512] = {0,};
//	char basePath[512] = {0,};
//
//	int ret = confManager.GetConfigPath(basePath, sizeof(basePath), "SOOPStudio/basic/profiles");
//	if(ret <= 0) {
//		blog(LOG_WARNING, "Failed to get profiles config path");
//		return;
//	}
//
//	ret = snprintf(profilePath, sizeof(profilePath), "%s/%s/*", basePath,
//				   profile_dir);
//	if(ret <= 0) {
//		blog(LOG_WARNING, "Failed to get path for profile dir '%s'", profile_dir);
//		return;
//	}
//
//	os_glob_t* glob = NULL;
//	if(os_glob(profilePath, 0, &glob) != 0) {
//		blog(LOG_WARNING, "Failed to glob profile dir '%s'", profile_dir);
//		return;
//	}
//
//	for(size_t i = 0; i < glob->gl_pathc; i++) {
//		const char* filePath = glob->gl_pathv[i].path;
//
//		if(glob->gl_pathv[i].directory)
//			continue;
//
//		os_unlink(filePath);
//	}
//
//	os_globfree(glob);
//
//	ret = snprintf(profilePath, sizeof(profilePath), "%s/%s", basePath, profile_dir);
//	if(ret <= 0) {
//		blog(LOG_WARNING, "Failed to get path for profile dir '%s'", profile_dir);
//		return;
//	}
//
//	os_rmdir(profilePath);
//
//	blog(LOG_INFO, "------------------------------------------------");
//	blog(LOG_INFO, "Removed profile '%s' (%s)", profile_name, profile_dir);
//	blog(LOG_INFO, "------------------------------------------------");
//}
//void AFProfileUtil::CheckForSimpleModeX264Fallback()
//{
//	auto& confManager = AFConfigManager::GetSingletonInstance();
//	config_t* basicConfig = confManager.GetBasic();
//	//
//	const char* curStreamEncoder = config_get_string(basicConfig, "SimpleOutput", "StreamEncoder");
//	const char* curRecEncoder = config_get_string(basicConfig, "SimpleOutput", "RecEncoder");
//	bool qsv_supported = false;
//	bool qsv_av1_supported = false;
//	bool amd_supported = false;
//	bool nve_supported = false;
//#ifdef ENABLE_HEVC
//	bool amd_hevc_supported = false;
//	bool nve_hevc_supported = false;
//	bool apple_hevc_supported = false;
//#endif
//	bool amd_av1_supported = false;
//	bool apple_supported = false;
//	bool changed = false;
//	size_t idx = 0;
//	const char* id;
//
//	while(obs_enum_encoder_types(idx++, &id))
//	{
//		if(strcmp(id, "h264_texture_amf") == 0)
//			amd_supported = true;
//		else if(strcmp(id, "obs_qsv11") == 0)
//			qsv_supported = true;
//		else if(strcmp(id, "obs_qsv11_av1") == 0)
//			qsv_av1_supported = true;
//		else if(strcmp(id, "ffmpeg_nvenc") == 0)
//			nve_supported = true;
//#ifdef ENABLE_HEVC
//		else if(strcmp(id, "h265_texture_amf") == 0)
//			amd_hevc_supported = true;
//		else if(strcmp(id, "ffmpeg_hevc_nvenc") == 0)
//			nve_hevc_supported = true;
//#endif
//		else if(strcmp(id, "av1_texture_amf") == 0)
//			amd_av1_supported = true;
//		else if(strcmp(id, "com.apple.videotoolbox.videoencoder.ave.avc") == 0)
//			apple_supported = true;
//#ifdef ENABLE_HEVC
//		else if(strcmp(id, "com.apple.videotoolbox.videoencoder.ave.hevc") == 0)
//			apple_hevc_supported = true;
//#endif
//	}
//
//	auto CheckEncoder = [&](const char*& name)
//	{
//		if(strcmp(name, SIMPLE_ENCODER_QSV) == 0) {
//			if(!qsv_supported) {
//				changed = true;
//				name = SIMPLE_ENCODER_X264;
//				return false;
//			}
//		} else if(strcmp(name, SIMPLE_ENCODER_QSV_AV1) == 0) {
//			if(!qsv_av1_supported) {
//				changed = true;
//				name = SIMPLE_ENCODER_X264;
//				return false;
//			}
//		} else if(strcmp(name, SIMPLE_ENCODER_NVENC) == 0) {
//			if(!nve_supported) {
//				changed = true;
//				name = SIMPLE_ENCODER_X264;
//				return false;
//			}
//		} else if(strcmp(name, SIMPLE_ENCODER_NVENC_AV1) == 0) {
//			if(!nve_supported) {
//				changed = true;
//				name = SIMPLE_ENCODER_X264;
//				return false;
//			}
//#ifdef ENABLE_HEVC
//		} else if(strcmp(name, SIMPLE_ENCODER_AMD_HEVC) == 0) {
//			if(!amd_hevc_supported) {
//				changed = true;
//				name = SIMPLE_ENCODER_X264;
//				return false;
//			}
//		} else if(strcmp(name, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
//			if(!nve_hevc_supported) {
//				changed = true;
//				name = SIMPLE_ENCODER_X264;
//				return false;
//			}
//#endif
//		} else if(strcmp(name, SIMPLE_ENCODER_AMD) == 0) {
//			if(!amd_supported) {
//				changed = true;
//				name = SIMPLE_ENCODER_X264;
//				return false;
//			}
//		} else if(strcmp(name, SIMPLE_ENCODER_AMD_AV1) == 0) {
//			if(!amd_av1_supported) {
//				changed = true;
//				name = SIMPLE_ENCODER_X264;
//				return false;
//			}
//		} else if(strcmp(name, SIMPLE_ENCODER_APPLE_H264) == 0) {
//			if(!apple_supported) {
//				changed = true;
//				name = SIMPLE_ENCODER_X264;
//				return false;
//			}
//#ifdef ENABLE_HEVC
//		} else if(strcmp(name, SIMPLE_ENCODER_APPLE_HEVC) == 0) {
//			if(!apple_hevc_supported) {
//				changed = true;
//				name = SIMPLE_ENCODER_X264;
//				return false;
//			}
//#endif
//		}
//
//		return true;
//	};
//
//	if(!CheckEncoder(curStreamEncoder))
//		config_set_string(basicConfig, "SimpleOutput", "StreamEncoder", curStreamEncoder);
//	if(!CheckEncoder(curRecEncoder))
//		config_set_string(basicConfig, "SimpleOutput", "RecEncoder", curRecEncoder);
//	if(changed)
//		config_save_safe(basicConfig, "tmp", nullptr);
//}
////
//bool AFProfileUtil::_FindSafeProfileDirName(const std::string& profileName, std::string& dirName)
//{
//	auto& confManager = AFConfigManager::GetSingletonInstance();
//	//
//	char path[512] = {0,};
//	int ret = -1;
//	if(ProfileExists(profileName.c_str())) {
//		blog(LOG_WARNING, "Profile '%s' exists", profileName.c_str());
//		return false;
//	}
//
//	if(!confManager.GetFileSafeName(profileName.c_str(), dirName)) {
//		blog(LOG_WARNING, "Failed to create safe file name for '%s'",
//			 profileName.c_str());
//		return false;
//	}
//
//	ret = confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/basic/profiles/");
//	if(ret <= 0) {
//		blog(LOG_WARNING, "Failed to get profiles config path");
//		return false;
//	}
//
//	dirName.insert(0, path);
//
//	if(!confManager.GetClosestUnusedFileName(dirName, nullptr)) {
//		blog(LOG_WARNING, "Failed to get closest file name for %s",
//			 dirName.c_str());
//		return false;
//	}
//
//	dirName.erase(0, ret);
//	return true;
//}
