#pragma once

#include <obs.hpp>

class AFLocaleTextManager;

class AFHotkeyContext final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFHotkeyContext& GetSingletonInstance()
    {
        static AFHotkeyContext* instance = nullptr;
        if (instance == nullptr)
            instance = new AFHotkeyContext;
        return *instance;
    };
    ~AFHotkeyContext() {}; 
private:
    // singleton constructor
    AFHotkeyContext() = default;
    AFHotkeyContext(const AFHotkeyContext&) = delete;
    AFHotkeyContext(/* rValue */AFHotkeyContext&& other) noexcept = delete;
    AFHotkeyContext& operator=(const AFHotkeyContext&) = delete;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	static obs_key				    MakeKey(int virtualKeyCode);
	static void                     InjectEvent(obs_key_combination_t key, bool pressed);


    void                            InitContext();
    bool                            FinContext();
    void                            ClearContext();

    void                            RegisterHotkeyReplayBufferSave(obs_output_t* output);
    void                            UnRegisterHotkeyReplayBufferSave();

#pragma endregion public func

#pragma region private func
private:
    void                            _InitHotkeyTranslations(AFLocaleTextManager* pManager);

    static OBSData                  _LoadHotkeyData(const char* name);
    static void                     _LoadHotkey(obs_hotkey_id id, const char* name);
    static void                     _LoadHotkeyPair(obs_hotkey_pair_id id, const char* name0,
                                                    const char* name1,
                                                    const char* oldName = NULL);

#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
    //obs_hotkey_active_func          m_cbStartStreaming = nullptr, m_cbStopStreaming = nullptr;
    //obs_hotkey_func                 m_cbForceStopStreaming = nullptr;
    //obs_hotkey_active_func          m_cbStartRecording = nullptr, m_cbStopRecording = nullptr;
    //obs_hotkey_active_func          m_cbPauseRecording = nullptr, m_cbUnPauseRecording = nullptr;
    //obs_hotkey_active_func          m_cbStartReplayBuffer = nullptr, m_cbStopReplayBuffer = nullptr;
    //obs_hotkey_active_func          m_cbEnablePreview = nullptr, m_cbDisablePreview = nullptr;
    //obs_hotkey_active_func          m_cbEnablePreviewProgram = nullptr, m_cbDisablePreviewProgram = nullptr;
    //obs_hotkey_active_func          m_cbShowContextBar = nullptr, m_cbHideContextBar = nullptr;
    //obs_hotkey_func                 m_cbTransition = nullptr, m_cbResetStats = nullptr;
    //obs_hotkey_func                 m_cbScreenshot = nullptr, m_cbScreenshotSource = nullptr;

    //void*                           m_pStartStreamingData = nullptr; void* m_pStopStreamingData = nullptr;
    //void*                           m_pForceStopStreamingData = nullptr;
    //void*                           m_pStartRecordingData = nullptr; void* m_pStopRecordingData = nullptr;
    //void*                           m_pPauseRecordingData = nullptr; void* m_pUnPauseRecordingData = nullptr;
    //void*                           m_pStartReplayBufferData = nullptr; void* m_pStopReplayBufferData = nullptr;
    //void*                           m_pEnablePreviewData = nullptr; void* m_pDisablePreviewData = nullptr;
    //void*                           m_pEnablePreviewProgramData = nullptr; void* m_pDisablePreviewProgramData = nullptr;
    //void*                           m_pShowContextBarData = nullptr; void* m_pHideContextBarData = nullptr;
    //void*                           m_pTransitionData = nullptr; void* m_pResetStatsData = nullptr;
    //void*                           m_pScreenshotData = nullptr; void* m_pScreenshotSourceData = nullptr;


    obs_hotkey_pair_id              m_StreamingHotkeys = 0, m_RecordingHotkeys = 0, m_PauseHotkeys = 0;
    obs_hotkey_pair_id              m_ReplayBufHotkeys = 0, m_VcamHotkeys = 0, m_TogglePreviewHotkeys = 0;
    obs_hotkey_pair_id              m_ContextBarHotkeys = 0, m_togglePreviewProgramHotkeys = 0;

    obs_hotkey_id                   m_ForceStreamingStopHotkey = 0, m_SplitFileHotkey = 0;
    obs_hotkey_id                   m_TransitionHotkey = 0, m_StatsHotkey = 0;
    obs_hotkey_id                   m_ScreenshotHotkey = 0, m_SourceScreenshotHotkey = 0;

    obs_hotkey_id                   m_ReplayBufferSave = 0;

#pragma endregion private member var
};
