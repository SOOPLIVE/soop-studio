#pragma once

#include <obs.hpp>
#include <util/util.hpp>

#include "CoreModel/OBSOutput/SBasicOutputHandler.h"
//
class AFAdvanceOutput : public AFBasicOutputHandler
{
public:
    AFAdvanceOutput(AFMainFrame* main);
    ~AFAdvanceOutput();

public:
    void UpdateStreamSettings();
    void UpdateRecordingSettings();
    void UpdateAudioSettings();
    virtual void Update() override;

    void SetupVodTrack(obs_service_t* service);

    void SetupStreaming();
    void SetupRecording();
    void SetupFFmpeg();
    void SetupOutputs() override;
    int GetAudioBitrate(size_t i, const char* id) const;

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
    bool allowsMultiTrack();

private:
    OBSEncoder m_streamAudioEnc;
    OBSEncoder m_streamArchiveEnc;
    OBSEncoder m_streamTrack[MAX_AUDIO_MIXES] = {nullptr,};
    OBSEncoder m_recordTrack[MAX_AUDIO_MIXES] = {nullptr,};
    OBSEncoder m_videoStreaming;
    OBSEncoder m_videoRecording;

    bool m_ffmpegOutput = false;
    bool m_ffmpegRecording = false;
    bool m_useStreamEncoder = false;
    bool m_useStreamAudioEncoder = false;
    bool m_usesBitrate = false;
};