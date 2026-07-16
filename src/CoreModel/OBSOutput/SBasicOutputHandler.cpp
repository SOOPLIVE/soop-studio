#include "SBasicOutputHandler.h"

#include <qt-wrappers.hpp>

#include "COBSOutputContext.h"

#include "Common/SettingsMiscDef.h"
#include "Common/StringMiscUtils.h"

#include "Application/CApplication.h"

#include "CoreModel/Service/CService.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Output/SimpleOutput.h"
#include "CoreModel/Output/AdvanceOutput.h"
#include "CoreModel/Encoder/CEncoder.h"
#include "CoreModel/Source/CSource.h"

#include "PopupWindows/SettingPopup/CSettingUtils.h"

#include "MainFrame/CMainFrame.h"

AFBasicOutputHandler::AFBasicOutputHandler(AFMainFrame* main_)
	:main(main_)
{
	// virtual cam
	if(main->VirtualCamEnabled()) {
		virtualCam = obs_output_create("virtualcam_output", "virtualcam_output", nullptr, nullptr);
		//
		signal_handler_t* signal = obs_output_get_signal_handler(virtualCam);
		startVirtualCam.Connect(signal, "start", &AFOutputUtil::OBSStartVirtualCam, this);
		stopVirtualCam.Connect(signal, "stop", &AFOutputUtil::OBSStopVirtualCam, this);
		deactivateVirtualCam.Connect(signal, "deactivate", &AFOutputUtil::OBSDeactivateVirtualCam, this);
	}
}
//
void AFBasicOutputHandler::SetupAutoRemux(const char*& container)
{
	bool autoRemux = config_get_bool(ACTIVECONFIG, "Video", "AutoRemux");
	if(autoRemux && strcmp(container, "mp4") == 0)
		container = "mkv";
}
std::string AFBasicOutputHandler::GetRecordingFilename(const char* path,
													   const char* container, bool noSpace,
													   bool overwrite, const char* format,
													   bool ffmpeg)
{
	if(!ffmpeg)
		SetupAutoRemux(container);

	std::string dst = GetOutputFilename(path, container, noSpace, overwrite, format);
	lastRecordingPath = dst;
	return dst;
}
//
void log_vcam_changed(const VCamConfig& config, bool starting)
{
	const char* action = starting ? "Starting" : "Changing";
	switch(config.type) {
		case VCamOutputType::Invalid:
			break;
		case VCamOutputType::ProgramView:
			blog(LOG_INFO, "%s Virtual Camera output to Program", action);
			break;
		case VCamOutputType::PreviewOutput:
			blog(LOG_INFO, "%s Virtual Camera output to Preview", action);
			break;
		case VCamOutputType::SceneOutput:
			blog(LOG_INFO, "%s Virtual Camera output to Scene : %s", action,
				 config.scene.c_str());
			break;
		case VCamOutputType::SourceOutput:
			blog(LOG_INFO, "%s Virtual Camera output to Source : %s",
				 action, config.source.c_str());
			break;
	}
}
bool AFBasicOutputHandler::StartVirtualCam()
{
	if(!main->VirtualCamEnabled())
		return false;

	VCamConfig& config = main->VirtualCamConfig();
	bool typeIsProgram = (config.type == VCamOutputType::ProgramView);

	if(!vcamView && !typeIsProgram)
		vcamView = obs_view_create();

	UpdateVirtualCamOutputSource();

	if(!vcamVideo) {
		vcamVideo = typeIsProgram ? obs_get_video()
								  : obs_view_add(vcamView);

		if(!vcamVideo)
			return false;
	}

	obs_output_set_media(virtualCam, vcamVideo, obs_get_audio());
	if(!Active())
		SetupOutputs();

	bool success = obs_output_start(virtualCam);
	if(!success) {
		QString errorReason;

		const char* error = obs_output_get_last_error(virtualCam);
		if(error) {
			errorReason = QT_UTF8(error);
		} else {
			errorReason = QTStr("Output.StartFailedGeneric");
		}

		QMessageBox::critical(main,
					  QTStr("Output.StartVirtualCamFailed"),
					  errorReason);

		DestroyVirtualCamView();
	}

	log_vcam_changed(config, true);

	return success;
}
void AFBasicOutputHandler::StopVirtualCam()
{
	if(main->VirtualCamEnabled()) {
		obs_output_stop(virtualCam);
	}
}
bool AFBasicOutputHandler::VirtualCamActive() const
{
	if(main->VirtualCamEnabled()) {
		return obs_output_active(virtualCam);
	}
	return false;
}
void AFBasicOutputHandler::UpdateVirtualCamOutputSource()
{
	if(!main->VirtualCamEnabled() ||
	   !vcamView)
		return;

	VCamConfig& config = main->VirtualCamConfig();
	OBSSourceAutoRelease source;
	switch(config.type) {
		case VCamOutputType::Invalid:
		case VCamOutputType::ProgramView:
			DestroyVirtualCamScene();
			return;
		case VCamOutputType::PreviewOutput: {
			DestroyVirtualCamScene();
			OBSSource s = SCENE_CONTEXT.GetCurrentSceneSource();
			obs_source_get_ref(s);
			source = s.Get();
			break;
		}
		case VCamOutputType::SceneOutput:
			DestroyVirtualCamScene();
			source = obs_get_source_by_name(config.scene.c_str());
			break;
		case VCamOutputType::SourceOutput:
			OBSSourceAutoRelease s =
				obs_get_source_by_name(config.source.c_str());

			if(!vcamSourceScene)
				vcamSourceScene =
				obs_scene_create_private("vcam_source");
			source = obs_source_get_ref(
				obs_scene_get_source(vcamSourceScene));

			if(vcamSourceSceneItem &&
				(obs_sceneitem_get_source(vcamSourceSceneItem) != s)) {
				obs_sceneitem_remove(vcamSourceSceneItem);
				vcamSourceSceneItem = nullptr;
			}

			if(!vcamSourceSceneItem) {
				vcamSourceSceneItem = obs_scene_add(vcamSourceScene, s);

				obs_sceneitem_set_bounds_type(vcamSourceSceneItem,
								  OBS_BOUNDS_SCALE_INNER);
				obs_sceneitem_set_bounds_alignment(vcamSourceSceneItem,
								   OBS_ALIGN_CENTER);

				const struct vec2 size = {
					(float)obs_source_get_width(source),
					(float)obs_source_get_height(source),
				};
				obs_sceneitem_set_bounds(vcamSourceSceneItem, &size);
			}
			break;
	}

	OBSSourceAutoRelease current = obs_view_get_source(vcamView, 0);
	if(source != current)
		obs_view_set_source(vcamView, 0, source);
}
void AFBasicOutputHandler::DestroyVirtualCamView()
{
	VCamConfig& config = main->VirtualCamConfig();
	if(config.type == VCamOutputType::ProgramView) {
		vcamVideo = nullptr;
		return;
	}

	obs_view_remove(vcamView);
	obs_view_set_source(vcamView, 0, nullptr);
	vcamVideo = nullptr;

	obs_view_destroy(vcamView);
	vcamView = nullptr;

	DestroyVirtualCamScene();
}
void AFBasicOutputHandler::DestroyVirtualCamScene()
{
	if(!vcamSourceScene)
		return;

	obs_scene_release(vcamSourceScene);
	vcamSourceScene = nullptr;
	vcamSourceSceneItem = nullptr;
}
//
AFBasicOutputHandler* CreateSimpleOutputHandler(AFMainFrame* main)
{
	return new AFSimpleOutput(main);
}
AFBasicOutputHandler* CreateAdvancedOutputHandler(AFMainFrame* main)
{
	return new AFAdvanceOutput(main);
}