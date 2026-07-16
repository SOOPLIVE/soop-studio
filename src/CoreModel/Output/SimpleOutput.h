#pragma once

#include <obs.hpp>
#include <util/util.hpp>

#include "CoreModel/OBSOutput/SBasicOutputHandler.h"
//
class AFSimpleOutput : public AFBasicOutputHandler
{
public:
    AFSimpleOutput(AFMainFrame* main);
    ~AFSimpleOutput() {}

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
#endif // ENABLE_HEVC
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

    bool IsVodTrackEnabled(obs_service_t* service);
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

private:
    bool icq_available(obs_encoder_t* encoder);

private:
	OBSEncoder m_audioStreaming;
	OBSEncoder m_videoStreaming;
	OBSEncoder m_audioRecording;
	OBSEncoder m_audioArchive;
	OBSEncoder m_videoRecording;
	OBSEncoder m_audioTrack[MAX_AUDIO_MIXES];

	std::string	m_videoEncoder;
	std::string m_videoQuality;
	bool m_usingRecordingPreset = false;
	bool m_recordingConfigured = false;
	bool m_ffmpegOutput = false;
	bool m_lowCPUx264 = false;
};