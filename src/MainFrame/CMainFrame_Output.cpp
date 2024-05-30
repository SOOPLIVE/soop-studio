#include "CMainFrame.h"
#include "ui_aneta-main-frame.h"

#include "Application/CApplication.h"

#include <QStyleOption>
#include <QWindow>
#include <QMessageBox>
#include <qcheckbox.h>
#include <qfileinfo.h>

#include "CUIValidation.h"
#include "Common/SettingsMiscDef.h"
#include "include/qt-wrapper.h"
#include "libavformat/avformat.h"

#include "CoreModel/Service/CService.h"
#include "UIComponent/CMessageBox.h"
#include "PopupWindows/CRemuxFrame.h"

// public function
void AFMainFrame::OBSStreamStarting(void* data, calldata_t* params)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
    obs_output_t* obj = (obs_output_t*)calldata_ptr(params, "output");

    int sec = (int)obs_output_get_active_delay(obj);
    if(sec == 0)
        return;

    output->delayActive = true;
    QMetaObject::invokeMethod(output->main, "qslotStreamDelayStarting", Q_ARG(void*, output), Q_ARG(int, sec));
}
void AFMainFrame::OBSStreamStopping(void* data, calldata_t* params)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
    obs_output_t* obj = (obs_output_t*)calldata_ptr(params, "output");

    int sec = (int)obs_output_get_active_delay(obj);
    if(sec == 0)
        QMetaObject::invokeMethod(output->main, "qslotStreamStopping", Q_ARG(void*, output));
    else
        QMetaObject::invokeMethod(output->main, "qslotStreamDelayStopping",
                                  Q_ARG(void*, output), Q_ARG(int, sec));
}
void AFMainFrame::OBSStartStreaming(void* data, calldata_t* /* params */)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
    output->streamingActive = true;
    QMetaObject::invokeMethod(output->main, "qslotStreamingStart",
                              Q_ARG(void*, output));
}
void AFMainFrame::OBSStopStreaming(void* data, calldata_t* params)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
    int code = (int)calldata_int(params, "code");
    const char* last_error = calldata_string(params, "last_error");

    QString arg_last_error = QString::fromUtf8(last_error);

    output->streamingActive = false;
    output->delayActive = false;

    QMetaObject::invokeMethod(output->main, "qslotStreamingStop",
                              Q_ARG(void*, output),
                              Q_ARG(int, code),
                              Q_ARG(QString, arg_last_error));
}
void AFMainFrame::OBSStartRecording(void* data, calldata_t* /* params */)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);

    output->recordingActive = true;

    QMetaObject::invokeMethod(output->main, "qslotRecordingStart");
}
void AFMainFrame::OBSStopRecording(void* data, calldata_t* params)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
    int code = (int)calldata_int(params, "code");
    const char* last_error = calldata_string(params, "last_error");

    QString arg_last_error = QString::fromUtf8(last_error);

    output->recordingActive = false;

    QMetaObject::invokeMethod(output->main, "qslotRecordingStop",
                              Q_ARG(int, code),
                              Q_ARG(QString, arg_last_error));
}
void AFMainFrame::OBSRecordStopping(void* data, calldata_t* /* params */)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
    QMetaObject::invokeMethod(output->main, "qslotRecordStopping");
}
void AFMainFrame::OBSRecordFileChanged(void* data, calldata_t* params)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
    const char* next_file = calldata_string(params, "next_file");
    QString arg_last_file = QString::fromUtf8(output->lastRecordingPath.c_str());
    QMetaObject::invokeMethod(output->main, "qslotRecordingFileChanged", Q_ARG(QString, arg_last_file));
    output->lastRecordingPath = next_file;
}
void AFMainFrame::OBSStartReplayBuffer(void* data, calldata_t* /* params */)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);

    output->replayBufferActive = true;

    QMetaObject::invokeMethod(output->main, "qslotReplayBufferStart");
}
void AFMainFrame::OBSStopReplayBuffer(void* data, calldata_t* params)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
    int code = (int)calldata_int(params, "code");

    output->replayBufferActive = false;

    QMetaObject::invokeMethod(output->main, "qslotReplayBufferStop", Q_ARG(int, code));
}
void AFMainFrame::OBSReplayBufferStopping(void* data, calldata_t* /* params */)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
    QMetaObject::invokeMethod(output->main, "qslotReplayBufferStopping");
}
void AFMainFrame::OBSReplayBufferSaved(void* data, calldata_t* /* params */)
{
    AFBasicOutputHandler* output = static_cast<AFBasicOutputHandler*>(data);
    QMetaObject::invokeMethod(output->main, "qslotReplayBufferSaved", Qt::QueuedConnection);
}
//
void AFMainFrame::OBSErrorMessageBox(const char* errorMsg, const char* defaultMsg, const char* errorLabel)
{
    QWidget* main  = AFMainDynamicComposit::Get();
    //
    QString error_reason;
    if(errorMsg)
        error_reason = QT_UTF8(errorMsg);
    else
        error_reason = QTStr(defaultMsg);
    //
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, main, "", errorMsg);
}

void AFMainFrame::qslotRefreshNetworkText() 
{
    int network = App()->GetStatistics()->GetNetworkState();
    QString str = QString::number(network) + QStringLiteral("%");
    ui->label_NetworkValue->setText(str);
}

void AFMainFrame::qslotNetworkState(PCStatState state) {
    SetPCStateIconStyle(ui->label_NetworkIcon, state);
}
// public qslot
// streaming
void AFMainFrame::qslotStartStreaming()
{
    if (IsStreamActive())
        return;

    if(m_disableOutputsRef)
        return;

    auto& authManager = AFAuthManager::GetSingletonInstance();

    AFChannelData* mainChannel = nullptr;

    if (authManager.GetMainChannelData(mainChannel))
    {
        if (mainChannel->bIsStreaming)
        {
            OBSDataAutoRelease settingData = obs_data_create();
            obs_data_set_bool(settingData, "bwtest", false);
            obs_data_set_string(settingData, "key", mainChannel->pAuthData->strKeyRTMP.c_str());
            obs_data_set_string(settingData, "server", mainChannel->pAuthData->strUrlRTMP.c_str());
            obs_data_set_bool(settingData, "use_auth", false); 
            obs_service_t* obsService = obs_service_create("rtmp_common", "default_service", settingData, nullptr);
            mainChannel->pObjOBSService = obsService;
            PrepareStreamingOutput(0, (obs_service_t*)mainChannel->pObjOBSService);
        }
    }

    int cntOfAccount = authManager.GetCntChannel();
    int handlerIdx = 0;

    for (int idx = 0; idx < cntOfAccount; idx++) {
        AFChannelData* tmpChannel = nullptr;
        authManager.GetChannelData(idx, tmpChannel);
        if (!tmpChannel || !tmpChannel->pAuthData)
            continue;
        if (tmpChannel->bIsStreaming) {
            OBSDataAutoRelease settingData = obs_data_create();
            obs_data_set_bool(settingData, "bwtest", false);
            obs_data_set_string(settingData, "key", tmpChannel->pAuthData->strKeyRTMP.c_str());
            obs_data_set_string(settingData, "server", tmpChannel->pAuthData->strUrlRTMP.c_str());
            if (!tmpChannel->pAuthData->strCustomID.empty()) {
                obs_data_set_bool(settingData, "use_auth", true);
                obs_data_set_string(settingData, "username", tmpChannel->pAuthData->strCustomID.c_str());
                if (!tmpChannel->pAuthData->strCustomPassword.empty())
                    obs_data_set_string(settingData, "password", tmpChannel->pAuthData->strCustomPassword.c_str());
            }
            else
                obs_data_set_bool(settingData, "use_auth", false);

            // set service
            obs_service_t* obsService = obs_service_create("rtmp_common", "default_service", settingData, nullptr);
            tmpChannel->pObjOBSService = obsService;

            handlerIdx++;
            PrepareStreamingOutput(handlerIdx, (obs_service_t*)tmpChannel->pObjOBSService); 
        }
    }    

    if(api)
        api->on_event(OBS_FRONTEND_EVENT_STREAMING_STARTING);

    qslotSaveProject();

    _ChangeStreamState(false, false, Str("Basic.Main.Connecting"), 110);

    OUTPUT_HANDLER_LIST::iterator it = m_outputHandlers.begin();
    for (; it != m_outputHandlers.end(); ++it) {
        if ((*it).first) {
            m_streamingOuputRef++;
            if (!StartStreamingOutput((*it).first)) {
                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                    QTStr("Output.Streaming.Failed"),
                    QTStr("Output.StartStreaming.Failed"));
            }
        }
    }
    
    auto& statusLogManager = AFStatusLogManager::GetSingletonInstance();
    statusLogManager.SendLogStartBroad();

    SetOutputHandler();

    bool recordWhenStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "RecordWhenStreaming");
    if(recordWhenStreaming)
        qslotStartRecording();

    bool replayBufferWhileStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
    if(replayBufferWhileStreaming)
        qslotStartReplayBuffer();

    _ToggleBroadTimer(true);

    emit qsignalToggleUseVideo(true);

    os_atomic_set_bool(&m_streaming_active, true);
}

void AFMainFrame::qslotStopStreaming()
{
    qslotSaveProject();

    m_streamingStopping = true;
    m_statusbar.ClearAllSignals();

    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (outputIter->second->StreamingActive()) {
            outputIter->second->StopStreaming(m_streamingStopping);
            obs_service_release(outputIter->first);
            outputIter->first = nullptr;
        }
    }

    OnDeactivate();

    bool recordWhenStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "RecordWhenStreaming");
    bool keepRecordingWhenStreamStops =
        config_get_bool(GetGlobalConfig(), "BasicWindow", "KeepRecordingWhenStreamStops");
    if(!keepRecordingWhenStreamStops)
        qslotStopRecording();

    bool replayBufferWhileStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
    bool keepReplayBufferStreamStops =
        config_get_bool(GetGlobalConfig(), "BasicWindow", "KeepReplayBufferStreamStops");
    if(!keepReplayBufferStreamStops)
        qslotStopReplayBuffer();

    _ToggleBroadTimer(false);
    emit qsignalToggleUseVideo(false);

    os_atomic_set_bool(&m_streaming_active, false);
}

void AFMainFrame::qslotForceStopStreaming()
{
    qslotSaveProject();

    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (outputIter->second->StreamingActive()) {
            outputIter->second->StopStreaming(true);
            obs_service_release(outputIter->first);
            outputIter->first = nullptr;
        }
    }

    OnDeactivate();

    bool recordWhenStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "RecordWhenStreaming");
    bool keepRecordingWhenStreamStops =
        config_get_bool(GetGlobalConfig(), "BasicWindow", "KeepRecordingWhenStreamStops");
    if(!keepRecordingWhenStreamStops)
        qslotStopRecording();

    bool replayBufferWhileStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
    bool keepReplayBufferStreamStops =
        config_get_bool(GetGlobalConfig(), "BasicWindow", "KeepReplayBufferStreamStops");
    if(replayBufferWhileStreaming && !keepReplayBufferStreamStops)
        qslotStopReplayBuffer();

    emit qsignalToggleUseVideo(false);
}

void AFMainFrame::qslotStreamDelayStarting(void* output, int sec)
{
    /*
    if(!startStreamMenu.isNull())
        startStreamMenu->deleteLater();

    startStreamMenu = new QMenu();
    startStreamMenu->addAction(QTStr("Basic.Main.StopStreaming"), this, &OBSBasic::StopStreaming);
    startStreamMenu->addAction(QTStr("Basic.Main.ForceStopStreaming"), this, &OBSBasic::ForceStopStreaming);
    ui->streamButton->setMenu(startStreamMenu);
    ui->statusbar->StreamDelayStarting(sec);
    */
    _ChangeStreamState(true, true, "END LIVE", 92);

    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (output == outputIter->second.get()) {
            m_statusbar.StreamDelayStarting(sec);
            break;
        }
    }

    OnActivate();
}

void AFMainFrame::qslotStreamDelayStopping(void* output, int sec)
{
    /*
    if(!startStreamMenu.isNull())
        startStreamMenu->deleteLater();

    startStreamMenu = new QMenu();
    startStreamMenu->addAction(QTStr("Basic.Main.StartStreaming"), this, &OBSBasic::StartStreaming);
    startStreamMenu->addAction(QTStr("Basic.Main.ForceStopStreaming"), this, &OBSBasic::ForceStopStreaming);
    ui->streamButton->setMenu(startStreamMenu);
    ui->statusbar->StreamDelayStopping(sec);
    */
    if (!IsStreamActive()) {
        _ChangeStreamState(true, false, "LIVE", 77);
    }

    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (output == outputIter->second.get()) {
            m_statusbar.StreamDelayStopping(sec);
            break;
        }
    }

    if(api)
        api->on_event(OBS_FRONTEND_EVENT_STREAMING_STOPPING);
}

void AFMainFrame::qslotStreamingStart(void* output)
{
    /*
    ui->statusbar->StreamStarted(outputHandler->streamOutput);
    */
    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (output == outputIter->second.get()) {
            m_statusbar.StreamStarted(outputIter->second->streamOutput);
            //break;
        }
    }

#ifdef YOUTUBE_ENABLED
    // get a current stream key
    obs_service_t* service_obj = AFServiceManager::GetSingletonInstance().GetService();
    OBSDataAutoRelease settings = obs_service_get_settings(service_obj);
    std::string key = obs_data_get_string(settings, "stream_id");
    if(!key.empty() && !youtubeStreamCheckThread) {
        youtubeStreamCheckThread = CreateQThread([this, key] { YoutubeStreamCheck(key); });
        youtubeStreamCheckThread->setObjectName("YouTubeStreamCheckThread");
        youtubeStreamCheckThread->start();
    }
#endif

    if(api)
        api->on_event(OBS_FRONTEND_EVENT_STREAMING_STARTED);

    _ChangeStreamState(true, true, "END LIVE", 92);
    App()->GetStatistics()->BroadStatus(true);
    App()->GetStatistics()->SetCongestionUpdate(true);

    OnActivate();
    
#ifdef YOUTUBE_ENABLED
    if(YouTubeAppDock::IsYTServiceSelected())
        youtubeAppDock->IngestionStarted();
#endif

    emit qsignalToggleUseVideo(false);
    blog(LOG_INFO, STREAMING_START);
}

void AFMainFrame::qslotStreamStopping(void* output)
{
    if (true == m_streamingStopping) {
        _ChangeStreamState(false, false, Str("Basic.Main.StoppingStreaming"), 120);

        if (api)
            api->on_event(OBS_FRONTEND_EVENT_STREAMING_STOPPING);
    }
}

void AFMainFrame::qslotStreamingStop(void* output, int code, QString last_error)
{
    const char* errorDescription = "";
    DStr errorMessage;
    bool use_last_error = false;
    bool encode_error = false;

    
    if (!output)
        return;

    std::string strChannelID = "";
    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (outputIter->second.get() == output) {
            AFChannelData* tmpChannel = nullptr;
            auto& authManager = AFAuthManager::GetSingletonInstance();
            authManager.GetChannelData(outputIter->first, tmpChannel);
            if (tmpChannel) {
                strChannelID = tmpChannel->pAuthData->strChannelID;
                tmpChannel->bIsStreaming = false;
                LoadAccounts();
                break;
            }                        
        }
    }
   
    switch(code) {
        case OBS_OUTPUT_BAD_PATH:
            errorDescription = Str("Output.ConnectFail.BadPath");
            break;

        case OBS_OUTPUT_CONNECT_FAILED:
            use_last_error = true;
            errorDescription = Str("Output.ConnectFail.ConnectFailed");
            break;

        case OBS_OUTPUT_INVALID_STREAM:
            errorDescription = Str("Output.ConnectFail.InvalidStream");
            break;

        case OBS_OUTPUT_ENCODE_ERROR:
            encode_error = true;
            break;

        case OBS_OUTPUT_HDR_DISABLED:
            errorDescription = Str("Output.ConnectFail.HdrDisabled");
            break;

        default:
        case OBS_OUTPUT_ERROR:
            use_last_error = true;
            errorDescription = Str("Output.ConnectFail.Error");
            break;

        case OBS_OUTPUT_DISCONNECTED:
            // doesn't happen if output is set to reconnect.  note that
            // reconnects are handled in the output, not in the UI
            use_last_error = true;
            errorDescription = Str("Output.ConnectFail.Disconnected");
    }

    std::string strErrorDesc = "";
    strErrorDesc = (!strChannelID.empty() ? strChannelID + " : " : "") + errorDescription;
    
    if(use_last_error && !last_error.isEmpty())
        dstr_printf(errorMessage, "%s\n\n%s", strErrorDesc.c_str(),
                QT_TO_UTF8(last_error));
    else
        dstr_copy(errorMessage, strErrorDesc.c_str());

    bool currentStreamOutputStopped = false;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (output == outputIter->second.get()) {
            m_statusbar.StreamStopped(outputIter->second->streamOutput);
            currentStreamOutputStopped = true;
            break;
        }
    }

    if(currentStreamOutputStopped)
        SetOutputHandler();

    m_streamingOuputRef--;

    if (true == m_streamingStopping) {

        if (0 == m_streamingOuputRef) {
            
            m_streamingStopping = false;
            if (api)
                api->on_event(OBS_FRONTEND_EVENT_STREAMING_STOPPED);

            OnDeactivate();

            _ToggleBroadTimer(false);
            _ChangeStreamState(true, false, "LIVE", 77);
            App()->GetStatistics()->BroadStatus(false);

#ifdef YOUTUBE_ENABLED
            if (YouTubeAppDock::IsYTServiceSelected())
                youtubeAppDock->IngestionStopped();
#endif

            blog(LOG_INFO, STREAMING_STOP);
        }
    }
    else {
        if (0 == m_streamingOuputRef) {

            qslotSaveProject();

            OnDeactivate();

            bool recordWhenStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "RecordWhenStreaming");
            bool keepRecordingWhenStreamStops =
                config_get_bool(GetGlobalConfig(), "BasicWindow", "KeepRecordingWhenStreamStops");
            if (!keepRecordingWhenStreamStops)
                qslotStopRecording();

            bool replayBufferWhileStreaming = config_get_bool(GetGlobalConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
            bool keepReplayBufferStreamStops =
               config_get_bool(GetGlobalConfig(), "BasicWindow", "KeepReplayBufferStreamStops");
            if (!keepReplayBufferStreamStops)
                qslotStopReplayBuffer();            

            os_atomic_set_bool(&m_streaming_active, false);
            // 
            _ToggleBroadTimer(false);
            _ChangeStreamState(true, false, "LIVE", 77);
            App()->GetStatistics()->BroadStatus(false);

            if (api)
                api->on_event(OBS_FRONTEND_EVENT_STREAMING_STOPPED);
            blog(LOG_INFO, STREAMING_STOP);
        }
    }

    if(encode_error) {
        QString msg = last_error.isEmpty()
            ? QTStr("Output.StreamEncodeError.Msg")
            : QTStr("Output.StreamEncodeError.Msg.LastError")
            .arg(last_error);
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   QTStr("Output.StreamEncodeError.Title"),
                                   msg);

    } else if(code != OBS_OUTPUT_SUCCESS && isVisible()) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   QTStr("Output.ConnectFail.Title"),
                                   QT_UTF8(errorMessage));

    }
    /*else if(code != OBS_OUTPUT_SUCCESS && !isVisible()) {
        SysTrayNotify(QT_UTF8(errorDescription), QSystemTrayIcon::Warning);
    }*/

    /*
    if(!startStreamMenu.isNull()) {
        ui->streamButton->setMenu(nullptr);
        startStreamMenu->deleteLater();
        startStreamMenu = nullptr;
    }
    */
    if (0 == m_streamingOuputRef) {
        auto& statusLogManager = AFStatusLogManager::GetSingletonInstance();
        statusLogManager.SendLogStartBroad(false);
        emit qsignalToggleUseVideo(false);
    }
}
void AFMainFrame::qslotStartRecording()
{
    if(IsRecordingActive())
        return;
    if(m_disableOutputsRef)
        return;

    if(!_OutputPathValid()) {
        _OutputPathInvalidMessage();
        ui->pushButton_Record->setChecked(false);
        return;
    }
    if(_LowDiskSpace()) {
        _DiskSpaceMessage();
        ui->pushButton_Record->setChecked(false);
        return;
    }

    if(api)
        api->on_event(OBS_FRONTEND_EVENT_RECORDING_STARTING);

    qslotSaveProject();


    if(!m_outputHandlers[0].second->StartRecording())
        ui->pushButton_Record->setChecked(false);


    emit qsignalToggleUseVideo(true);
}
void AFMainFrame::qslotStopRecording()
{
    qslotSaveProject();

    if(IsRecordingActive())
        m_outputHandlers[0].second->StopRecording(m_recordingStopping);

    OnDeactivate();

    emit qsignalToggleUseVideo(false);
}
void AFMainFrame::qslotRecordingStart()
{
    m_statusbar.RecordingStarted(m_outputHandlers[0].second->fileOutput);

    _ChangeRecordState(true);

    m_recordingStopping = false;
    if(api)
        api->on_event(OBS_FRONTEND_EVENT_RECORDING_STARTED);

    OnActivate();
    _UpdatePause();

    emit qsignalToggleUseVideo(true);
    blog(LOG_INFO, RECORDING_START);
}
void AFMainFrame::qslotRecordStopping()
{
    /*
    ui->recordButton->setText(QTStr("Basic.Main.StoppingRecording"));
    */
    m_recordingStopping = true;

    if(api)
        api->on_event(OBS_FRONTEND_EVENT_RECORDING_STOPPING);
}
void AFMainFrame::qslotRecordingStop(int code, QString last_error)
{
    _ChangeRecordState(false);

    blog(LOG_INFO, RECORDING_STOP);

    if(code == OBS_OUTPUT_UNSUPPORTED && isVisible()) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   QTStr("Output.RecordFail.Title"),
                                   QTStr("Output.RecordFail.Unsupported"));

    } else if(code == OBS_OUTPUT_ENCODE_ERROR && isVisible()) {
        QString msg =
            last_error.isEmpty()
            ? QTStr("Output.RecordError.EncodeErrorMsg")
            : QTStr("Output.RecordError.EncodeErrorMsg.LastError")
            .arg(last_error);
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   QTStr("Output.RecordError.Title"),
                                   msg);

    } else if(code == OBS_OUTPUT_NO_SPACE && isVisible()) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                               QTStr("Output.RecordNoSpace.Title"),
                               QTStr("Output.RecordNoSpace.Msg"));

    } else if(code != OBS_OUTPUT_SUCCESS && isVisible()) {

        const char* errorDescription = nullptr;
        DStr errorMessage;
        bool use_last_error = true;

        errorDescription = Str("Output.RecordError.Msg");

        if(use_last_error && !last_error.isEmpty())
            dstr_printf(errorMessage, "%s<br><br>%s", errorDescription, QT_TO_UTF8(last_error));
        else
            dstr_copy(errorMessage, errorDescription);

        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   QTStr("Output.RecordError.Title"),
                                   QT_UTF8(errorMessage));

    } else if(code == OBS_OUTPUT_UNSUPPORTED && !isVisible()) {
        ShowSystemAlert(QTStr("Output.RecordFail.Unsupported"));
        //SysTrayNotify(QTStr("Output.RecordFail.Unsupported"), QSystemTrayIcon::Warning);
    } else if(code == OBS_OUTPUT_NO_SPACE && !isVisible()) {
        ShowSystemAlert(QTStr("Output.RecordNoSpace.Msg"));
        //SysTrayNotify(QTStr("Output.RecordNoSpace.Msg"), QSystemTrayIcon::Warning);
    } else if(code != OBS_OUTPUT_SUCCESS && !isVisible()) {
        ShowSystemAlert(QTStr("Output.RecordError.Msg"));
        //SysTrayNotify(QTStr("Output.RecordError.Msg"), QSystemTrayIcon::Warning);
    } else if(code == OBS_OUTPUT_SUCCESS) {
        std::string path = m_outputHandlers[0].second->lastRecordingPath;
        QString str = QTStr("Basic.StatusBar.RecordingSavedTo");
        
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   QT_UTF8(""), str.arg(QT_UTF8(path.c_str())), false, true);
        //ShowStatusBarMessage());
    }

    if(api)
        api->on_event(OBS_FRONTEND_EVENT_RECORDING_STOPPED);

    _AutoRemux(m_outputHandlers[0].second->lastRecordingPath.c_str());

    OnDeactivate();
    _UpdatePause(false);

    emit qsignalToggleUseVideo(false);
}
void AFMainFrame::qslotRecordingFileChanged(QString lastRecordingPath)
{
    QString str = QTStr("Basic.StatusBar.RecordingSavedTo");
    
    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                               QT_UTF8(""), str.arg(lastRecordingPath), false, true);
    //ShowStatusBarMessage(str.arg(lastRecordingPath));

    _AutoRemux(lastRecordingPath, true);
}
void AFMainFrame::qslotShowReplayBufferPauseWarning()
{
    auto msgBox = []() {
        QMessageBox msgbox(App()->GetMainView());
        msgbox.setWindowTitle(QTStr("Output.ReplayBuffer." "PauseWarning.Title"));
        msgbox.setText(QTStr("Output.ReplayBuffer." "PauseWarning.Text"));
        msgbox.setIcon(QMessageBox::Icon::Information);
        msgbox.addButton(QMessageBox::Ok);
        //
        QCheckBox* cb = new QCheckBox(QTStr("DoNotShowAgain"));
        msgbox.setCheckBox(cb);

        msgbox.exec();

        if(cb->isChecked()) {
            config_set_bool(GetGlobalConfig(), "General", "WarnedAboutReplayBufferPausing", true);
            config_save_safe(GetGlobalConfig(), "tmp", nullptr);
        }
    };

    bool warned = config_get_bool(GetGlobalConfig(), "General",
                      "WarnedAboutReplayBufferPausing");
    if(!warned) {
        QMetaObject::invokeMethod(App(), "Exec", Qt::QueuedConnection, Q_ARG(VoidFunc, msgBox));
    }
}

void AFMainFrame::qslotStartReplayBuffer()
{
    if(IsReplayBufferActive())
        return;
    if(m_disableOutputsRef)
        return;

    if(!AFUIValidation::NoSourcesConfirmation(this)) {
//        recording_paused->first()->setChecked(false);
        GetMainWindow()->SetReplayBufferStartStopMode(false);
        return;
    }
    if(!_OutputPathValid()) {
        _OutputPathInvalidMessage();
        GetMainWindow()->SetReplayBufferStartStopMode(false);
//        recording_paused->first()->setChecked(false);
        return;
    }
    if(_LowDiskSpace()) {
        _DiskSpaceMessage();
        GetMainWindow()->SetReplayBufferStartStopMode(false);
//        recording_paused->first()->setChecked(false);
        return;
    }

    GetMainWindow()->SetReplayBufferReleased();

    if(api)
        api->on_event(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING);

    qslotSaveProject();

    if(!m_outputHandlers[0].second->StartReplayBuffer()) {
        GetMainWindow()->SetReplayBufferStartStopMode(false);
    } else if(os_atomic_load_bool(&m_recording_paused)) {
        qslotShowReplayBufferPauseWarning();
    }
}
void AFMainFrame::qslotStopReplayBuffer()
{
    if(!m_outputHandlers[0].second->replayBuffer)
        return;

    qslotSaveProject();

    if(IsReplayBufferActive())
        m_outputHandlers[0].second->StopReplayBuffer(m_replayBufferStopping);

    OnDeactivate();
}
void AFMainFrame::qslotReplayBufferStart()
{
    if (!m_outputHandlers[0].second->replayBuffer)
        return;

    GetMainWindow()->SetReplayBufferStartStopMode(true);
    /*
    replayBufferButton->first()->setText(QTStr("Basic.Main.StopReplayBuffer"));
    replayBufferButton->first()->setChecked(true);
    */

    m_replayBufferStopping = false;
    if(api)
        api->on_event(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED);

    OnActivate();
    _UpdateReplayBuffer();

    blog(LOG_INFO, REPLAY_BUFFER_START);
}
void AFMainFrame::qslotReplayBufferStopping()
{
    if (!m_outputHandlers[0].second->replayBuffer)
        return;

    GetMainWindow()->SetReplayBufferStoppingMode();
//    replayBufferButton->first()->setText( QTStr("Basic.Main.StoppingReplayBuffer"));

    m_replayBufferStopping = true;
    if(api)
        api->on_event(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPING);
}
void AFMainFrame::qslotReplayBufferStop(int code)
{
    if (!m_outputHandlers[0].second->replayBuffer)
        return;

    GetMainWindow()->SetReplayBufferStartStopMode(false);
    /*
    replayBufferButton->first()->setText(QTStr("Basic.Main.StartReplayBuffer"));
    replayBufferButton->first()->setChecked(false);
    */

    blog(LOG_INFO, REPLAY_BUFFER_STOP);

    if(code == OBS_OUTPUT_UNSUPPORTED && isVisible()) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, 
                                   QTStr("Output.RecordFail.Title"),
                                   QTStr("Output.RecordFail.Unsupported"));
    } else if(code == OBS_OUTPUT_NO_SPACE && isVisible()) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   QTStr("Output.RecordNoSpace.Title"),
                                   QTStr("Output.RecordNoSpace.Msg"));
    } else if(code != OBS_OUTPUT_SUCCESS && isVisible()) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this, 
                                   QTStr("Output.RecordError.Title"),
                                   QTStr("Output.RecordError.Msg"));
    } else if(code == OBS_OUTPUT_UNSUPPORTED && !isVisible()) {
        ShowSystemAlert(QTStr("Output.RecordFail.Unsupported"));
//        SysTrayNotify(QTStr("Output.RecordFail.Unsupported"), QSystemTrayIcon::Warning);
    } else if(code == OBS_OUTPUT_NO_SPACE && !isVisible()) {
        ShowSystemAlert(QTStr("Output.RecordNoSpace.Msg"));
//        SysTrayNotify(QTStr("Output.RecordNoSpace.Msg"), QSystemTrayIcon::Warning);
    } else if(code != OBS_OUTPUT_SUCCESS && !isVisible()) {
        ShowSystemAlert(QTStr("Output.RecordError.Msg"));
//        SysTrayNotify(QTStr("Output.RecordError.Msg"), QSystemTrayIcon::Warning);
    }

    if(api)
        api->on_event(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED);

    OnDeactivate();
    _UpdateReplayBuffer(false);
}

void AFMainFrame::qslotPauseRecording()
{
    if(//!pause || 
       !m_outputHandlers[0].second || !m_outputHandlers[0].second->fileOutput ||
        os_atomic_load_bool(&m_recording_paused))
        return;

    obs_output_t* output = m_outputHandlers[0].second->fileOutput;
    if(obs_output_pause(output, true))
    {
        /*pause->setAccessibleName(QTStr("Basic.Main.UnpauseRecording"));
        pause->setToolTip(QTStr("Basic.Main.UnpauseRecording"));
        pause->blockSignals(true);
        pause->setChecked(true);
        pause->blockSignals(false);

        ui->statusbar->RecordingPaused();*/

        os_atomic_set_bool(&m_recording_paused, true);

        /*auto replay = replayBufferButton ? replayBufferButton->second() : nullptr;
        if(replay)
            replay->setEnabled(false);*/

        if(api)
            api->on_event(OBS_FRONTEND_EVENT_RECORDING_PAUSED);

        if(os_atomic_load_bool(&m_replaybuf_active))
            qslotShowReplayBufferPauseWarning();
    }
}
void AFMainFrame::qslotUnpauseRecording()
{
    if(//!pause || 
       !m_outputHandlers[0].second || !m_outputHandlers[0].second->fileOutput ||
        !os_atomic_load_bool(&m_recording_paused))
        return;

    obs_output_t* output = m_outputHandlers[0].second->fileOutput;
    if(obs_output_pause(output, false))
    {
        /*pause->setAccessibleName(QTStr("Basic.Main.PauseRecording"));
        pause->setToolTip(QTStr("Basic.Main.PauseRecording"));
        pause->blockSignals(true);
        pause->setChecked(false);
        pause->blockSignals(false);

        ui->statusbar->RecordingUnpaused();*/

        os_atomic_set_bool(&m_recording_paused, false);

        /*auto replay = replayBufferButton ? replayBufferButton->second() : nullptr;
        if(replay)
            replay->setEnabled(true);*/

        if(api)
            api->on_event(OBS_FRONTEND_EVENT_RECORDING_UNPAUSED);
    }
}
//
void AFMainFrame::_AutoRemux(QString input, bool no_show)
{
    auto config = GetBasicConfig();

    bool autoRemux = config_get_bool(config, "Video", "AutoRemux");
    if(!autoRemux)
        return;

    bool isSimpleMode = false;
    const char* mode = config_get_string(config, "Output", "Mode");
    if(!mode) {
        isSimpleMode = true;
    } else {
        isSimpleMode = strcmp(mode, "Simple") == 0;
    }

    if(!isSimpleMode) {
        const char* recType = config_get_string(config, "AdvOut", "RecType");

        bool ffmpegOutput = astrcmpi(recType, "FFmpeg") == 0;
        if(ffmpegOutput)
            return;
    }

    if(input.isEmpty())
        return;

    QFileInfo fi(input);
    QString suffix = fi.suffix();

    /* do not remux if lossless */
    if(suffix.compare("avi", Qt::CaseInsensitive) == 0) {
        return;
    }

    QString path = fi.path();

    QString output = input;
    output.resize(output.size() - suffix.size());

    const obs_encoder_t* videoEncoder = obs_output_get_video_encoder(m_outputHandlers[0].second->fileOutput);
    const obs_encoder_t* audioEncoder = obs_output_get_audio_encoder(m_outputHandlers[0].second->fileOutput, 0);
    const char* vCodecName = obs_encoder_get_codec(videoEncoder);
    const char* aCodecName = obs_encoder_get_codec(audioEncoder);
    const char* format = config_get_string(config, isSimpleMode ? "SimpleOutput" : "AdvOut", "RecFormat2");

    bool audio_is_pcm = strncmp(aCodecName, "pcm", 3) == 0;

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(60, 5, 100)
    /* FFmpeg <= 6.0 cannot remux AV1+PCM into any supported format. */
    if(audio_is_pcm && strcmp(vCodecName, "av1") == 0)
        return;
#endif

    /* Retain original container for fMP4/fMOV */
    if(strncmp(format, "fragmented", 10) == 0) {
        output += "remuxed." + suffix;
    } else if(strcmp(vCodecName, "prores") == 0) {
        output += "mov";
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(60, 5, 100)
    } else if(audio_is_pcm) {
        output += "mov";
#endif
    } else {
        output += "mp4";
    }

    AFQRemux* remux = new AFQRemux(QT_TO_UTF8(path), this, true);
    if(!no_show)
        remux->show();
    remux->AutoRemux(input, output);

}
void AFMainFrame::_UpdatePause(bool activate)
{
    if(!activate ||
       !IsRecordingActive()) {
        return;
    }

    auto config = GetBasicConfig();

    const char* mode = config_get_string(config, "Output", "Mode");
    bool adv = astrcmpi(mode, "Advanced") == 0;
    bool shared = false;
    if(adv) {
        const char* recType = config_get_string(config, "AdvOut", "RecType");
        if(astrcmpi(recType, "FFmpeg") == 0) {
            shared = config_get_bool(config, "AdvOut", "FFOutputToFile");
        } else {
            const char* recordEncoder = config_get_string(config, "AdvOut", "RecEncoder");
            shared = astrcmpi(recordEncoder, "none") == 0;
        }
    } else {
        const char* quality = config_get_string(config, "SimpleOutput", "RecQuality");
        shared = strcmp(quality, "Stream") == 0;
    }

    if(!shared) {
        /*pause.reset(new QPushButton());
        pause->setAccessibleName(QTStr("Basic.Main.PauseRecording"));
        pause->setToolTip(QTStr("Basic.Main.PauseRecording"));
        pause->setCheckable(true);
        pause->setChecked(false);
        pause->setProperty("themeID", QVariant(QStringLiteral("pauseIconSmall")));

        QSizePolicy sp;
        sp.setHeightForWidth(true);
        pause->setSizePolicy(sp);

        connect(pause.data(), &QAbstractButton::clicked, this, &OBSBasic::PauseToggled);
        ui->recordingLayout->addWidget(pause.data());*/
    } else {
//        pause.reset();
    }
}
void AFMainFrame::_UpdateReplayBuffer(bool activate)
{
   if(!activate || !IsReplayBufferActive()) {
        return;
    }
}

void AFMainFrame::_ClearAllStreamSignals()
{
    OUTPUT_HANDLER_LIST::iterator outputIter;
    for (outputIter = m_outputHandlers.begin(); outputIter != m_outputHandlers.end(); ++outputIter) {
        if (outputIter->first != nullptr && 
            outputIter->second != nullptr) 
        {
            m_statusbar.StreamStopped(outputIter->second->streamOutput);
        }
    }

    m_statusbar.ClearAllSignals();
}