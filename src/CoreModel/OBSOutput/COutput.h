#pragma once

#include <obs.hpp>
#include <string>

namespace AFOutputUtil {
	void ResetOutput(bool advOut);

	void OBSStreamStarting(void* data, calldata_t* params);
	void OBSStreamStopping(void* data, calldata_t* params);
	void OBSStartStreaming(void* data, calldata_t* /* params */);
	void OBSStopStreaming(void* data, calldata_t* params);
	void OBSStartRecording(void* data, calldata_t* /* params */);
	void OBSStopRecording(void* data, calldata_t* params);
	void OBSRecordStopping(void* data, calldata_t* /* params */);
	void OBSRecordFileChanged(void* data, calldata_t* params);
	void OBSStartReplayBuffer(void* data, calldata_t* /* params */);
	void OBSStopReplayBuffer(void* data, calldata_t* params);
	void OBSReplayBufferStopping(void* data, calldata_t* /* params */);
	void OBSReplayBufferSaved(void* data, calldata_t* /* params */);

	void OBSStartVirtualCam(void* data, calldata_t* /* params */);
	void OBSStopVirtualCam(void* data, calldata_t* params);
	void OBSDeactivateVirtualCam(void* data, calldata_t* /* params */);

	bool IsActive();
	bool IsStreamActive();
	bool GetStreamingCheck();
	bool GetRecordingCheck();
	bool GetRecordingPauseCheck();
	bool GetReplayBufferCheck();
	bool IsReplayBufferActive();
	bool IsRecordingActive();
	bool IsVirtualCamActive();

	bool IsStartStreamingOutput(obs_service_t* service);
	bool IsStartStreamingOutput(obs_output_t* output);

	bool IsMainStreamOutput(obs_output_t* output);
	bool PauseOutput();
	void SaveReplayBuffer();
	std::string SavedReplayBuffer();
	obs_output_t* GetRecordingFileOutput();
	std::string GetLastRecordingPath();
	const char* GetCurrentOutputPath();

	void StopStreaming();
	void StopForceStreaming();
	bool StartRecording();
	void StopRecording();
	bool StartReplayBuffer();
	bool StopReplayBuffer();
	//bool PauseRecording();
	//bool UnPauseRecording();
	bool StartStreamingOutput(obs_service_t* service);
};

/* mistakes have been made to lead us to this. */
extern const char* get_simple_output_encoder(const char* encoder);
