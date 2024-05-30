#pragma once

#include <obs.hpp>
#include <util/util.hpp>

#include "CoreModel/OBSOutput/SBasicOutputHandler.h"
//
class AFSimpleOutput : public AFBasicOutputHandler
{
#pragma region QT Field, CTOR/DTOR
public:
    AFSimpleOutput(AFMainFrame* main);
	~AFSimpleOutput();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    int CalcCRF(int crf);

    void UpdateRecordingSettings_x264_crf(int crf);
    void UpdateRecordingSettings_qsv11(int crf, bool av1);
    void UpdateRecordingSettings_nvenc(int cqp);
    void UpdateRecordingSettings_nvenc_hevc_av1(int cqp);
    void UpdateRecordingSettings_amd_cqp(int cqp);
    void UpdateRecordingSettings_apple(int quality);
#ifdef ENABLE_HEVC
    void UpdateRecordingSettings_apple_hevc(int quality);
#endif
    void UpdateRecordingSettings();
    void UpdateRecordingAudioSettings();
    virtual void Update() override;

    void SetupOutputs() override;
    int GetAudioBitrate() const;

    void LoadRecordingPreset_Lossy(const char* encoder);
    void LoadRecordingPreset_Lossless();
    void LoadRecordingPreset();

    void LoadStreamingPreset_Lossy(const char* encoder);

    void UpdateRecording();
    bool ConfigureRecording(bool useReplayBuffer);

    void SetupVodTrack(obs_service_t* service);

    virtual bool SetupStreaming(obs_service_t* service) override;
    virtual bool StartStreaming(obs_service_t* service) override;
    virtual bool StartRecording() override;
    virtual bool StartReplayBuffer() override;
    virtual void StopStreaming(bool force) override;
    virtual void StopRecording(bool force) override;
    virtual void StopReplayBuffer(bool force) override;
    virtual bool StreamingActive() const override;
    virtual bool RecordingActive() const override;
    virtual bool ReplayBufferActive() const override;
#pragma endregion public func

#pragma region private func
private:
    bool icq_available(obs_encoder_t* encoder);
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
	OBSEncoder audioStreaming;
	OBSEncoder videoStreaming;
	OBSEncoder audioRecording;
	OBSEncoder audioArchive;
	OBSEncoder videoRecording;
	OBSEncoder audioTrack[MAX_AUDIO_MIXES];

	std::string	videoEncoder;
	std::string videoQuality;
	bool usingRecordingPreset = false;
	bool recordingConfigured = false;
	bool ffmpegOutput = false;
	bool lowCPUx264 = false;
#pragma endregion private member var
};

#include "COutput.inl"