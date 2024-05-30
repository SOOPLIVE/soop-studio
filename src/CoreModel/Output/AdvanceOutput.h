#pragma once

#include <obs.hpp>
#include <util/util.hpp>

#include "CoreModel/OBSOutput/SBasicOutputHandler.h"
//
class AFAdvanceOutput : public AFBasicOutputHandler
{
#pragma region QT Field, CTOR/DTOR
public:
    AFAdvanceOutput(AFMainFrame* main);
    ~AFAdvanceOutput();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
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
#pragma endregion public func

#pragma region private func
private:
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
    OBSEncoder streamAudioEnc;
    OBSEncoder streamArchiveEnc;
    OBSEncoder streamTrack[MAX_AUDIO_MIXES];
    OBSEncoder recordTrack[MAX_AUDIO_MIXES];
    OBSEncoder videoStreaming;
    OBSEncoder videoRecording;

    bool ffmpegOutput = false;
    bool ffmpegRecording = false;
    bool useStreamEncoder = false;
    bool useStreamAudioEncoder = false;
    bool usesBitrate = false;
#pragma endregion private member var
};

#include "COutput.inl"