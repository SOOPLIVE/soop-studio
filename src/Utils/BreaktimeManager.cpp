
#include "BreaktimeManager.h"

#include "CoreModel/Auth/CAuthManager.h"

#include "MainFrame/SceneSource/CMainSceneSource.h"
#include "Blocks/SceneSourceDock/CSceneSourceDockWidget.h"
#include "Blocks/CDockTitle.h"

inline bool IsInvalidAPI() {
    return (!AUTH_CONTEXT.GetSoopBroadInfo() || AUTH_CONTEXT.SoopCookie().empty() ? true : false);
}


void BreaktimeManager::Initialize()
{
    auto activeConfig = ACTIVECONFIG;
    //
#if 0
    config_set_string(activeConfig, "Hotkeys", "OBSBasic.BreakTime.Test1", "{\"bindings\":[{\"shift\":true,\"key\":\"OBS_KEY_1\"}]}");
    auto hotkey1 = HOTKEY_CONTEXT.RegisterHotkey("OBSBasic.BreakTime.Test1", "TEST1", [](void*, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
        if(pressed) {
            BreakTimeManager.Test1();
        }
    });

    config_set_string(activeConfig, "Hotkeys", "OBSBasic.BreakTime.Test2", "{\"bindings\":[{\"shift\":true,\"key\":\"OBS_KEY_2\"}]}");
    auto hotkey2 = HOTKEY_CONTEXT.RegisterHotkey("OBSBasic.BreakTime.Test2", "TEST2", [](void*, obs_hotkey_id, obs_hotkey_t*, bool pressed) {
        if(pressed) {
            BREAKTIME_MANAGER.Test2();
        }
    });

    config_save_safe(USERCONFIG, "tmp", nullptr);
#endif // _DEBUG

    config_set_default_bool(activeConfig, "BreakTime", "BreakTime.Muted", false);
    media_muted = config_get_bool(activeConfig, "BreakTime", "BreakTime.Muted");

    config_set_default_int(activeConfig, "BreakTime", "BreakTime.Volume", FADER_PRECISION);
    media_vol = config_get_int(activeConfig, "BreakTime", "BreakTime.Volume");
}
void BreaktimeManager::Finalize()
{
    auto activeConfig = ACTIVECONFIG;
    //
    config_set_int(activeConfig, "BreakTime", "BreakTime.Volume", (int64_t)media_vol);
    config_set_bool(activeConfig, "BreakTime", "BreakTime.Muted", media_muted);

    config_save_safe(activeConfig, "tmp", nullptr);
}

void BreaktimeManager::qslotResponseBreaktimeStartAPI(const QByteArray& responseData)
{
    const QString raw = QString::fromUtf8(responseData).trimmed();

    if (raw.isEmpty() || raw == "{}"
        || raw.compare("OK", Qt::CaseInsensitive) == 0
        || raw.compare("SUCCESS", Qt::CaseInsensitive) == 0
        || raw.compare("null", Qt::CaseInsensitive) == 0) {

        Start();
        return;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(responseData, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        const QString head = raw.left(200);
        blog(LOG_ERROR, "BreaktimeStartAPI parse error: %s | head=%s",
            err.errorString().toUtf8().constData(), head.toUtf8().constData());

        Stop(true);
        return;
    }

    const QJsonObject obj = doc.object();
    if (obj.isEmpty()) {
        Stop(true);
        return;
    }

    const int result = obj.value("result").toInt();
    const QString msg = obj.value("msg").toString();

    QString errorMsg = QString("BreaktimeStartAPI failed:%1\n%2").arg(result).arg(msg);     
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", errorMsg, false, true, "", 0, 0, "type1");
    
    Stop(true);
}

void BreaktimeManager::qslotResponseBreaktimeStopAPI(const QByteArray& responseData)
{
    emit signalStopApisDone(true);

    const QString raw = QString::fromUtf8(responseData).trimmed();

    if (raw.isEmpty() || raw == "{}"
        || raw.compare("OK", Qt::CaseInsensitive) == 0
        || raw.compare("SUCCESS", Qt::CaseInsensitive) == 0
        || raw.compare("null", Qt::CaseInsensitive) == 0) {
        return;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(responseData, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        const QString head = raw.left(200);
        blog(LOG_ERROR, "BreaktimeStopAPI parse error: %s | head=%s",
            err.errorString().toUtf8().constData(), head.toUtf8().constData());

        Stop(true);
        return;
    }

    const QJsonObject obj = doc.object();
    if (obj.isEmpty()) {
        blog(LOG_INFO, "BreaktimeStopAPI success ({})");
        return;
    }

    const int result = obj.value("result").toInt();
    const QString msg = obj.value("msg").toString(); 
}

void BreaktimeManager::qslotResponseBreaktimeAdViewCntAPI(const QByteArray& responseData)
{
    auto trimViewCount = [](int c) {
        if (c <= 9)      return c;
        if (c <= 99)     return (c / 10) * 10;
        if (c <= 999)    return (c / 100) * 100;
        return (c / 1000) * 1000;
        };

    int ad_viewcnt = 0;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(responseData, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        BreaktimeManager::_BreaktimeStopAPI(0);
        return;
    }

    const QJsonObject obj = doc.object();
    const QString result = obj.value("result").toString();

    if (result.compare(QStringLiteral("SUCCESS"), Qt::CaseInsensitive) == 0) {
        if (obj.contains("count") && obj.value("count").isDouble()) {
            const int raw = obj.value("count").toInt();
            ad_viewcnt = trimViewCount(raw);
        }
    }

    _BreaktimeStopAPI(ad_viewcnt);
}

void BreaktimeManager::_qslotTimer()
{
    if (timer_cur_time < timer_end_time)
    {    
        ++timer_cur_time;
        m_timer_cur_time = timer_cur_time;
    }        
    
    emit signalTick(GetRemainTime(), timer_end_time);

    blog(LOG_DEBUG, "%s (%d) : timer_cur_time : %d, timer_end_time : %d", __FILE__, __LINE__, timer_cur_time, timer_end_time);

    if (timer_cur_time == timer_end_time)
    {
        _BroadInfoDockTitle(false);

        emit signalFinished();

        _BreaktimeAdViewCntAPI();

        timer.stop();
        disconnect(&timer, &QTimer::timeout, this, &BreaktimeManager::_qslotTimer);

        //

        MAINFRAME->RefreshSceneUI();
        _SetSceneSourceDockWidgetBreaktime(false);
        MAIN_SCENESOURCE->UpdateEditMenu();
        MAINFRAME->SetSceneCollectionEnabled(true);

        //

        _StopMediaSource();

        if (target_scene_name.length() == 0)
            _StopBrowserSource();
        else
        {
            auto source = obs_get_source_by_name(last_scene_name.toUtf8().constData());
            MAINFRAME->SetCurrentScene(source);
            obs_source_release(source);
        }

        //

        if (!m_skipNextTimer) {
            next_timer_cur_time = 0;
            connect(&next_timer, &QTimer::timeout, this, &BreaktimeManager::_qslotNextTimer);
            next_timer.start(1000);
        }
        else {
            m_skipNextTimer = false;
        }
    }
}

void BreaktimeManager::_qslotNextTimer()
{
    if (next_timer_cur_time < next_timer_end_time)
        ++next_timer_cur_time;

    blog(LOG_DEBUG, "%s (%d) : next_timer_cur_time : %d, next_timer_end_time : %d", __FILE__, __LINE__, next_timer_cur_time, next_timer_end_time);
    
    if (next_timer_cur_time == next_timer_end_time)
    {
        next_timer.stop();        
        disconnect(&next_timer, &QTimer::timeout, this, &BreaktimeManager::_qslotNextTimer);
    }
}

void BreaktimeManager::SetSceneText(QString text)
{
    target_scene_text = text;
    
    if (timer.isActive())
        _PlayBrowserSource();
}

void BreaktimeManager::SetAudioMuted(bool muted)
{
    media_muted = muted;
    OBSSourceAutoRelease source = obs_get_output_source(7);
    obs_source_set_muted(source, muted);
}

void BreaktimeManager::SetAudioVolume(int vol)
{
    media_vol = vol;
    obs_fader_set_deflection(media_fader, float(vol) / (float)FADER_PRECISION);
}

void BreaktimeManager::Start()
{
    if (!timer.isActive() && !next_timer.isActive())
    {
        if (target_scene_name.length() == 0)
            _PlayBrowserSource();
        else
        {
            OBSSourceAutoRelease current_scene_source = obs_frontend_get_current_scene();
            last_scene_name = obs_source_get_name(current_scene_source);

            auto target_scene_source = obs_get_source_by_name(target_scene_name.toUtf8().constData());
            MAINFRAME->SetCurrentScene(target_scene_source);
            obs_source_release(target_scene_source);
        }

        _StopMediaSource();
        _PlayMediaSource(false);

        //

        m_skipNextTimer = false;

        timer_cur_time = 0;
        m_timer_cur_time = 0;
        connect(&timer, &QTimer::timeout, this, &BreaktimeManager::_qslotTimer);
        timer.start(1000);

        //

        MAINFRAME->SetSceneCollectionEnabled(false);
        MAIN_SCENESOURCE->UpdateEditMenu();
        _SetSceneSourceDockWidgetBreaktime(true);
        MAINFRAME->RefreshSceneUI();
    }
}

void BreaktimeManager::Stop(bool forced)
{    
    m_timer_cur_time = timer_cur_time;

    if (forced)
    {
        m_skipNextTimer = true;

        if (next_timer.isActive()) {
            next_timer.stop();
            disconnect(&next_timer, &QTimer::timeout, this, &BreaktimeManager::_qslotNextTimer);
        }

        next_timer_cur_time = next_timer_end_time;
        timer_cur_time = timer_end_time;
    }
    else
    {
        if (timer_cur_time >= timer_min_time)
            timer_cur_time = timer_end_time;
    }    
}

void BreaktimeManager::BreaktimeStartAPI()
{
    if (IsInvalidAPI())
        return;

    m_unique_key = static_cast<qint64>(std::time(nullptr));
    m_nBroadNumber = AUTH_CONTEXT.GetSoopBroadInfo()->BroadNumber();

    QList<QVariant> postVals = { };

    SOOP_API_HANDLER->postAPIfromId(BREAKTIME_START, postVals, this, "qslotResponseBreaktimeStartAPI");
}

void BreaktimeManager::_PlayBrowserSource()
{
    OBSDataAutoRelease settings = obs_data_create();

    uint32_t width = (uint32_t)1920;
    uint32_t height = (uint32_t)1080;
    struct obs_video_info ovi;
    if (obs_get_video_info(&ovi))
    {
        width = ovi.base_width;
        height = ovi.base_height;
    }

    obs_data_set_int(settings, "width", (long long)width);
    obs_data_set_int(settings, "height", (long long)height);

    QDir dir(QCoreApplication::applicationDirPath());
    dir.cdUp(); // ../
    dir.cdUp(); // ../../
    dir.cd("./data/obs-studio/assets/breaktime"); // ../../data/obs-studio/assets/breaktime
    QString url = "file:///" + dir.absolutePath();

    if (width >= height)
        url += "/breaktime.html";
    else
        url += "/breaktime_vertical.html";

    url += "?seconds=" + QString::number(timer_end_time - (timer.isActive() ? timer_cur_time : 0));
    url += "&theme=light";
    url += "&notice_msg=" + target_scene_text;
    url += "&add_msg=" + QTStr("breaktime.message.wait");
    url += "&timer_label=" + QTStr("breaktime.broadtitle.title");
    url += "&min_text=" + QTStr("Minutes");
    url += "&sec_text=" + QTStr("Seconds");
    obs_data_set_string(settings, "url", url.toUtf8().constData());

    obs_data_set_bool(settings, "shutdown", true);
#if 0
    obs_data_set_bool(settings, "reroute_audio", false);
    OBSSourceAutoRelease source = obs_source_create("browser_source", "GlobalBrowser", settings, nullptr);
#else
    OBSSourceAutoRelease source = obs_source_create_private("browser_source", "GlobalBrowser", settings);
 
#endif // 1
    obs_source_inc_showing(source);
    if (timer.isActive())
        os_sleep_ms(1000);
    obs_set_output_source(8, source);
    obs_source_dec_showing(source);
}

void BreaktimeManager::_StopBrowserSource()
{
    obs_set_output_source(8, nullptr);
}

void BreaktimeManager::_PlayMediaSource(bool preview)
{
    if (media_index != 0)
    {
        OBSDataAutoRelease settings = obs_data_create();
        std::string local_file;
        if (media_index == 1)
            local_file = "../../data/obs-studio/assets/breaktime/breaktime_light.mp3";
        else if (media_index == 2)
            local_file = "../../data/obs-studio/assets/breaktime/breaktime_middle.mp3";
        else if (media_index == 3)
            local_file = "../../data/obs-studio/assets/breaktime/breaktime_synthe.mp3";
        else
            local_file = "../../data/obs-studio/assets/breaktime/breaktime_calm.mp3";

        obs_data_set_string(settings, "local_file", local_file.c_str());
        obs_data_set_bool(settings, "looping", true);

        OBSSourceAutoRelease source = preview ?
            obs_source_create_private("ffmpeg_source", QTStr("breaktime.bgm").toUtf8().constData(), settings) :
            obs_source_create("ffmpeg_source", QTStr("breaktime.bgm").toUtf8().constData(), settings, nullptr);

        obs_source_set_muted(source, media_muted);
        signal_handler_connect(obs_source_get_signal_handler(source), "mute", _OBSVolumeMuted, this);
        
        media_fader = obs_fader_create(OBS_FADER_LOG);
        obs_fader_attach_source(media_fader, source);
        obs_fader_add_callback(media_fader, _OBSVolumeChanged, this);
        obs_fader_set_deflection(media_fader, float(media_vol) / (float)FADER_PRECISION);
        
        obs_source_set_monitoring_type(source, preview ?
                                       obs_monitoring_type::OBS_MONITORING_TYPE_MONITOR_ONLY :
                                       obs_monitoring_type::OBS_MONITORING_TYPE_MONITOR_AND_OUTPUT);

        soop_set_output_source(7, source, preview ? false : true);
    }
    else
        soop_set_output_source(7, nullptr, true);
}

void BreaktimeManager::_StopMediaSource()
{
    OBSSourceAutoRelease source = obs_get_output_source(7);
    soop_set_output_source(7, nullptr, false);

    obs_fader_remove_callback(media_fader, _OBSVolumeChanged, this);
    media_fader = nullptr;

    signal_handler_disconnect(obs_source_get_signal_handler(source), "mute", _OBSVolumeMuted, this);
}

void BreaktimeManager::_OBSVolumeMuted(void* data, calldata_t* calldata)
{
    BreaktimeManager* bm = static_cast<BreaktimeManager*>(data);
    bool muted = calldata_bool(calldata, "muted");
    bm->_VolumeMuted(muted); //QMetaObject::invokeMethod(volControl, "VolumeMuted", Q_ARG(bool, muted));
}

void BreaktimeManager::_OBSVolumeChanged(void* data, float db)
{
    Q_UNUSED(db);
    BreaktimeManager* bm = static_cast<BreaktimeManager*>(data);
    bm->_VolumeChanged(); //QMetaObject::invokeMethod(bm, "VolumeChanged");
}

void BreaktimeManager::_SetSceneSourceDockWidgetBreaktime(bool enable)
{
    QWidget* outBlock = nullptr;
    if (!MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::SceneSource, outBlock))
        return;

    AFSceneSourceWidget* scenesourceBlock = reinterpret_cast<AFSceneSourceWidget*>(outBlock);
    if (scenesourceBlock)
        scenesourceBlock->SetBreaktime(enable, target_scene_name.length() == 0 ? QTStr("breaktime.scene.default") : target_scene_name);
}

void BreaktimeManager::_BreaktimeStopAPI(int ad_viewcnt)
{
    if(IsInvalidAPI()) {
        emit signalStopApisDone(false);
        return;
    }

    if (m_unique_key == 0) 
        m_unique_key = static_cast<qint64>(std::time(nullptr));
       
    QList<QVariant> postVals = { };

    SOOP_API_HANDLER->postAPIfromId( BREAKTIME_STOP, postVals, this, "qslotResponseBreaktimeStopAPI" );
}

void BreaktimeManager::_BreaktimeAdViewCntAPI()
{
    if(IsInvalidAPI()) {
        emit signalStopApisDone(false);
        return;
    }

    const int nInterval = (m_timer_cur_time / 60) + 1;

    std::string user_id;
    AUTH_CONTEXT.GetChannelID(PLATFORM_SOOP, user_id);

    SOOP_API_DATA info = SOOP_API_HANDLER->getAPIInfofromId(BREAKTIME_AD_COUNT);
    const QString url = QString::fromStdString(info.url)
        .arg(QString::fromStdString(user_id))
        .arg(nInterval);

   SOOP_API_HANDLER->getAPI( info.apiName.c_str(), url.toStdString().c_str(), this, "qslotResponseBreaktimeAdViewCntAPI", QList<int>(), "", true);
}

void BreaktimeManager::_BroadInfoDockTitle(bool bOn)
{
    AFQBaseDockWidget* broadInfoDock = nullptr;
    if (MAIN_BLOCKMANAGER->GetDock(ENUM_WINDOW_TYPE::BroadInfo, broadInfoDock))
    {
        if (broadInfoDock)
        {
            QList<AFDockTitle*> Titles = broadInfoDock->findChildren<AFDockTitle*>();
            AFDockTitle* dockTitle = nullptr;

            dockTitle = Titles[0];

            if (bOn)
                dockTitle->TitleChangePage(0);
            else
                dockTitle->TitleChangePage(1);

            return;
        }
    }
}
