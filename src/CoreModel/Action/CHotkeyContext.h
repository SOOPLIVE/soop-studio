#pragma once

#include <obs.hpp>

class AFHotkeyContext final
{
public:
    AFHotkeyContext() {}
    ~AFHotkeyContext() {};

public:
    void InitHotkeys();
    void CreateHotkeys();
    void ClearHotkeys();
         
    void RegisterHotkeyReplayBufferSave(obs_output_t* output);
    void UnRegisterHotkeyReplayBufferSave();
    //
    obs_hotkey_id RegisterHotkey(const char* name, const char* description, obs_hotkey_func func);
    void UnRegisterHotkey(obs_hotkey_id id);

private:
    static OBSData loadHotkeyData(const char* name);
    static void loadHotkey(obs_hotkey_id id, const char* name);
    static void loadHotkeyPair(obs_hotkey_pair_id id, const char* name0,
                               const char* name1, const char* oldName = NULL);

private:
    obs_hotkey_pair_id m_streamingHotkeys = 0, m_recordingHotkeys = 0, m_pauseHotkeys = 0;
    obs_hotkey_pair_id m_replayBufHotkeys = 0, m_virCamHotkeys = 0, m_togglePreviewHotkeys = 0;
    obs_hotkey_pair_id m_contextBarHotkeys = 0, m_togglePreviewProgramHotkeys = 0;

    obs_hotkey_id m_forceStreamingStopHotkey = 0, m_splitFileHotkey = 0, m_addChapterHotkey = 0;
    obs_hotkey_id m_transitionHotkey = 0, m_statsHotkey = 0;
    obs_hotkey_id m_screenshotHotkey = 0, m_sourceScreenshotHotkey = 0;

    obs_hotkey_id m_replayBufferSave = 0;
};