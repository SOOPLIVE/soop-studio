#pragma once

#include <string>
#include <obs.hpp>

// forward class
class AFMainFrame;


struct AFBasicOutputHandler
{
	OBSOutputAutoRelease fileOutput;
	OBSOutputAutoRelease streamOutput;
	OBSOutputAutoRelease replayBuffer;
	OBSOutputAutoRelease virtualCam;
	bool streamingActive = false;
	bool recordingActive = false;
	bool delayActive = false;
	bool replayBufferActive = false;
	bool virtualCamActive = false;

	AFMainFrame* main = nullptr;

	/*std::unique_ptr<MultitrackVideoOutput> multitrackVideo;
	bool multitrackVideoActive = false;*/

	OBSOutputAutoRelease StreamingOutput() const
	{
		/*return (multitrackVideo && multitrackVideoActive)
			? multitrackVideo->StreamingOutput()
			: OBSOutputAutoRelease {obs_output_get_ref(streamOutput)};*/
		return OBSOutputAutoRelease { obs_output_get_ref(streamOutput) };
	}
	// virtual cam
	obs_view_t* vcamView = nullptr;
	video_t* vcamVideo = nullptr;
	obs_scene_t* vcamSourceScene = nullptr;
	obs_sceneitem_t* vcamSourceSceneItem = nullptr;

	std::string outputType;
	std::string lastError;

	std::string lastRecordingPath;

	OBSSignal startRecording;
	OBSSignal stopRecording;
	OBSSignal startReplayBuffer;
	OBSSignal stopReplayBuffer;
	OBSSignal startStreaming;
	OBSSignal stopStreaming;
	OBSSignal startVirtualCam;
	OBSSignal stopVirtualCam;
	OBSSignal deactivateVirtualCam;
	OBSSignal streamDelayStarting;
	OBSSignal streamStopping;
	OBSSignal recordStopping;
	OBSSignal recordFileChanged;
	OBSSignal replayBufferStopping;
	OBSSignal replayBufferSaved;

	AFBasicOutputHandler(AFMainFrame* main_);
	virtual ~AFBasicOutputHandler() {}

	virtual bool SetupStreaming(obs_service_t* service) = 0;
	virtual bool StartStreaming(obs_service_t* service) = 0;
	virtual bool StartRecording() = 0;
	virtual bool StartReplayBuffer() { return false; }
	virtual void StopStreaming(bool force = false) = 0;
	virtual void StopRecording(bool force = false) = 0;
	virtual void StopReplayBuffer(bool force = false) { (void)force; }
	virtual bool StreamingActive() const = 0;
	virtual bool RecordingActive() const = 0;
	virtual bool ReplayBufferActive() const { return false; }

	virtual void Update() = 0;
	virtual void SetupOutputs() = 0;

	virtual bool StartVirtualCam();
	virtual void StopVirtualCam();
	virtual bool VirtualCamActive() const;
	virtual void UpdateVirtualCamOutputSource();
	virtual void DestroyVirtualCamView();
	virtual void DestroyVirtualCamScene();

	inline bool Active() const
	{
		return streamingActive || recordingActive || delayActive ||
			replayBufferActive || virtualCamActive;
	}

protected:
	void SetupAutoRemux(const char*& container);
	std::string GetRecordingFilename(const char* path,
									 const char* container, bool noSpace,
									 bool overwrite, const char* format,
									 bool ffmpeg);
};

AFBasicOutputHandler* CreateSimpleOutputHandler(AFMainFrame* main);
AFBasicOutputHandler* CreateAdvancedOutputHandler(AFMainFrame* main);