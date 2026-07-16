
#pragma once

#include <QTimer>
#include <ctime>

#include "qt-wrappers.hpp"

#define FADER_PRECISION 4096

class BreaktimeManager final : public QObject {
    Q_OBJECT

public:
    explicit BreaktimeManager() = default;
    virtual ~BreaktimeManager() { Finalize(); }

    void Initialize();
    void Finalize();

signals:
    void signalTick(int remainingSec, int totalSec);
    void signalFinished();
    void signalStopApisDone(bool ok);

public slots:
    void qslotResponseBreaktimeStartAPI(const QByteArray& responseData);
    void qslotResponseBreaktimeStopAPI(const QByteArray& responseData);
    void qslotResponseBreaktimeAdViewCntAPI(const QByteArray& responseData);

private slots:
    void _qslotTimer();
    void _qslotNextTimer();

#if 0
    void Test1()
    {
        AFQBorderPopupBaseWidget* popup = nullptr;
        MAIN_BLOCKMANAGER->MakePopup(ENUM_WINDOW_TYPE::Breaktime, popup);
    }
    void Test2()
    {
        //SetSceneText("Test");
    }
#endif // _DEBUG

public:
    void SetTimer(int seconds) { timer_end_time = seconds; }
    int GetRemainTime() { return timer_end_time - timer_cur_time; }
    int GetNextRemainTime() { return next_timer_end_time - next_timer_cur_time; }

    void SetScene(QString name) { target_scene_name = name; }
    void SetSceneText(QString text);
    QString GetScene() { return target_scene_name; }

    void SetAudio(int index) { media_index = index; }
    void PlayAudio() { _PlayMediaSource(); }
    void StopAudio() { _StopMediaSource(); }
    void SetAudioMuted(bool muted);
    bool GetAudioMuted() { return media_muted; }
    void SetAudioVolume(int vol);
    int GetAudioVolume() { return media_vol; }
    
    void Start();
    void Stop(bool forced = false);
    bool IsActive() { return timer.isActive(); }
    bool IsNextActive() { return next_timer.isActive(); }
    void BreaktimeStartAPI();

private:
    void _PlayBrowserSource();
    void _StopBrowserSource();
    
    void _PlayMediaSource(bool preview = true);
    void _StopMediaSource();

    static void _OBSVolumeMuted(void* data, calldata_t* calldata);
    void _VolumeMuted(bool muted) { media_muted = muted; }

    static void _OBSVolumeChanged(void* param, float db);
    void _VolumeChanged() { media_vol = (int)(obs_fader_get_deflection(media_fader) * (float)FADER_PRECISION); } // CVolumeControl 참조

    void _SetSceneSourceDockWidgetBreaktime(bool enable);

    void _BreaktimeStopAPI(int ad_viewcnt);
    void _BreaktimeAdViewCntAPI();

    void _BroadInfoDockTitle(bool bOn);

private:
    QString target_scene_name;
    QString target_scene_text;
    QString last_scene_name;

    int media_index = 0;
    bool media_muted = false;
    int media_vol = FADER_PRECISION;
    OBSFader media_fader;

    int timer_cur_time = 0; // sec
    int timer_min_time = 60; // sec
    int timer_end_time = 0; // sec
    QTimer timer;

    int m_timer_cur_time = 0; // sec

    int next_timer_cur_time = 0; // sec
    int next_timer_end_time = 600; // sec
    QTimer next_timer;

    std::time_t m_unique_key = 0;
    int m_nBroadNumber = 0;
    bool m_skipNextTimer = false;
};