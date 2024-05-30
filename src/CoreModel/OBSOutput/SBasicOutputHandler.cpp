#include "COBSOutputContext.h"
#include "SBasicOutputHandler.h"

#include "Common/StringMiscUtils.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Output/SimpleOutput.h"
#include "CoreModel/Output/AdvanceOutput.h"


//
void AFBasicOutputHandler::SetupAutoRemux(const char*& container)
{
	auto& confManager = AFConfigManager::GetSingletonInstance();
	//
	bool autoRemux = config_get_bool(confManager.GetBasic(), "Video", "AutoRemux");
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
AFBasicOutputHandler* CreateSimpleOutputHandler(AFMainFrame* main)
{
	return new AFSimpleOutput(main);
}
AFBasicOutputHandler* CreateAdvancedOutputHandler(AFMainFrame* main)
{
	return new AFAdvanceOutput(main);
}