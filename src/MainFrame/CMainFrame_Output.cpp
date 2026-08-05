#include "CMainFrame.h"
#include "ui_aneta-main-frame.h"

#include <QStyleOption>
#include <QWindow>
#include <QMessageBox>
#include <qcheckbox.h>
#include <qfileinfo.h>

#include "qt-wrappers.hpp"
#include "util/dstr.hpp"

#include "Application/CApplication.h"

#include "ui-validation.hpp"
#include "libavformat/avformat.h"

#include "Common/SettingsMiscDef.h"

#include "CoreModel/Statistics/CStatistics.h"
#include "CoreModel/Service/CService.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/OBSOutput/COBSOutputContext.h"
#include "CoreModel/Video/CVideo.h"
#include "CoreModel/Auth/CAuthManager.h"

#include "UIComponent/CMessageBox.h"

#include "CoreModel/Profile/CProfile.h"

#include "PopupWindows/CRemuxFrame.h"
#include "PopupWindows/SettingPopup/CSettingUtils.h"

#include "Output/COutput.h"


void AFMainFrame::setResolution()
{
    AFQBroadInfo* pSoopBroadInfo = AUTH_CONTEXT.GetSoopBroadInfo();

    if (pSoopBroadInfo && !pSoopBroadInfo->Allow1440P())
    {
        OBSData advEncorderData;
        advEncorderData = AFProfileUtil::GetDataFromJsonFile("streamEncoder.json");

        auto config = ACTIVECONFIG;
        //
        int outCx = config_get_uint(config, "Video", "OutputCX");
        int outCy = config_get_uint(config, "Video", "OutputCY");
        int baseCx = config_get_uint(config, "Video", "BaseCX");
        int baseCy = config_get_uint(config, "Video", "BaseCY");
        int vBitrate = config_get_uint(config, "SimpleOutput", "VBitrate");
        int vFFBitrate = obs_data_get_int(advEncorderData, "bitrate");

        bool validResolution = (outCx > 1920 || outCy > 1080) ? false : true;
        if (!validResolution &&
            ((outCx == 720 && outCy == 1280) || (outCx == 1600 && outCy == 1200)))
        {
            validResolution = true;
        }

        bool validBitrate = vBitrate > 8000 ? false : true;
        bool validFFBitrate = vFFBitrate > 8000 ? false : true;

        if (!validResolution) {
            baseCx = outCx = 1920;
            baseCy = outCy = 1080;

            config_set_uint(config, "Video", "OutputCX", outCx);
            config_set_uint(config, "Video", "OutputCY", outCy);

            config_set_uint(config, "Video", "BaseCX", baseCx);
            config_set_uint(config, "Video", "BaseCY", baseCy);
        }
        if (!validBitrate) {
            vBitrate = 8000;
            config_set_uint(config, "SimpleOutput", "VBitrate", vBitrate);
        }

        if (!validFFBitrate) {
            vFFBitrate = 8000;
            obs_data_set_int(advEncorderData, "bitrate", vFFBitrate);
            AFProfileUtil::SetDataToJsonFile("streamEncoder.json", advEncorderData);
        }

        config_save_safe(config, "tmp", nullptr);
        AFVideoUtil::ResetVideo();
    }
}

void AFMainFrame::qslotRefreshMainResourceText()
{
    struct obs_video_info ovi = {};
    obs_get_video_info(&ovi);
    double obsFPS = (double)ovi.fps_num / (double)ovi.fps_den;

    double fps = STATISTICS.GetCurFPS();
    QString str = QString("%1 / %2 FPS").arg(QString::number(fps, 'f', 2)).arg(QString::number(obsFPS, 'f', 2));
    ui->label_ResourceValue->setText(str);
}

void AFMainFrame::qslotResourceState(PCStatState state) 
{
    SetPCStateIconStyle(ui->label_ResourceIcon, state);
}

void AFMainFrame::qslotStartStreaming()
{
    if (AFOutputUtil::IsStreamActive())
        return;
 
    auto& auth = AUTH_CONTEXT;
    auto userConfig = USERCONFIG;

    int nMinsimCheckCnt = config_get_int(userConfig, "SARSA", "UseMinsimCheckCnt");
    if (nMinsimCheckCnt > 0) {
        auto startTime = std::chrono::steady_clock::now();
        auth.SetMinsimCheckStartTime(startTime);
    }

    bool streamingStart = m_pMainOutput->StartStreamingOutputs();
    if (!streamingStart)
    {
        ToggleBroadTimerUI(false);
        ChangeStreamStateUI(true, false, "LIVE", 77);
        return;
    }

    OnEvent(OBS_FRONTEND_EVENT_STREAMING_STARTING);

    qslotSaveProject();

    emit StreamingStarting(false);

    bool recordWhenStreaming = config_get_bool(USERCONFIG, "BasicWindow", "RecordWhenStreaming");

    if(recordWhenStreaming)
        qslotStartRecording();

    bool replayBufferWhileStreaming = config_get_bool(userConfig, "BasicWindow", "ReplayBufferWhileStreaming");
    if(replayBufferWhileStreaming)
        qslotStartReplayBuffer();

    ToggleBroadTimerUI(true);

    BroadStatusCheckTimerStart();

    emit qsignalToggleUseVideo(true);

    os_atomic_set_bool(&OUTPUT_CONTEXT.m_streamingActive, true);
}

void AFMainFrame::qslotStopStreaming()
{
    qslotSaveProject();

    m_pMainOutput->GetStatusBarTemp().ClearAllSignals();

    AFOutputUtil::StopStreaming();

    OnDeactivate();

    auto userConfig = USERCONFIG;
    //
    bool recordWhenStreaming = config_get_bool(userConfig, "BasicWindow", "RecordWhenStreaming");
    bool keepRecordingWhenStreamStops = config_get_bool(userConfig, "BasicWindow", "KeepRecordingWhenStreamStops");
    if(!keepRecordingWhenStreamStops)
        qslotStopRecording();

    bool replayBufferWhileStreaming = config_get_bool(userConfig, "BasicWindow", "ReplayBufferWhileStreaming");
    bool keepReplayBufferStreamStops = config_get_bool(userConfig, "BasicWindow", "KeepReplayBufferStreamStops");
    if(!keepReplayBufferStreamStops)
        qslotStopReplayBuffer();

    ToggleBroadTimerUI(false);
    emit qsignalToggleUseVideo(false);
    MAINFRAME_UI->pushButton_Broad->setProperty("IsLive", false);

    os_atomic_set_bool(&OUTPUT_CONTEXT.m_streamingActive, false);
}

void AFMainFrame::qslotForceStopStreaming()
{
    qslotSaveProject();

    AFOutputUtil::StopForceStreaming();

    OnDeactivate();

    auto userConfig = USERCONFIG;
    //
    bool recordWhenStreaming = config_get_bool(userConfig, "BasicWindow", "RecordWhenStreaming");
    bool keepRecordingWhenStreamStops = config_get_bool(userConfig, "BasicWindow", "KeepRecordingWhenStreamStops");
    if(!keepRecordingWhenStreamStops)
        qslotStopRecording();

    bool replayBufferWhileStreaming = config_get_bool(userConfig, "BasicWindow", "ReplayBufferWhileStreaming");
    bool keepReplayBufferStreamStops = config_get_bool(userConfig, "BasicWindow", "KeepReplayBufferStreamStops");
    if(replayBufferWhileStreaming && !keepReplayBufferStreamStops)
        qslotStopReplayBuffer();

    emit qsignalToggleUseVideo(false);
}

void AFMainFrame::qslotStreamDelayStarting(void* output, int sec)
{
    emit StreamingStarted(true);
    /*
    if(!startStreamMenu.isNull())
        startStreamMenu->deleteLater();

    startStreamMenu = new QMenu();
    startStreamMenu->addAction(QTStr("Basic.Main.StopStreaming"), this, &OBSBasic::StopStreaming);
    startStreamMenu->addAction(QTStr("Basic.Main.ForceStopStreaming"), this, &OBSBasic::ForceStopStreaming);
    ui->streamButton->setMenu(startStreamMenu);
    ui->statusbar->StreamDelayStarting(sec);
    */
    int buttonSize = 81;
    QString locale = QString::fromStdString(LOCALE_CONTEXT.GetCurrentLocaleStr());
    if (locale != "ko-KR")
        buttonSize = 92;

    ChangeStreamStateUI(true, true, QTStr("Basic.Main.StopBroad"), buttonSize);

    OUTPUT_HANDLER_LIST::iterator outputIter;
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    for (outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
        if (output == outputIter->second.get()) {
            m_pMainOutput->GetStatusBarTemp().StreamDelayStarting(sec);
            break;
        }
    }

    OnActivate();
}

void AFMainFrame::qslotStreamDelayStopping(void* output, int sec)
{
    emit StreamingStopped(true);
    /*
    if(!startStreamMenu.isNull())
        startStreamMenu->deleteLater();

    startStreamMenu = new QMenu();
    startStreamMenu->addAction(QTStr("Basic.Main.StartStreaming"), this, &OBSBasic::StartStreaming);
    startStreamMenu->addAction(QTStr("Basic.Main.ForceStopStreaming"), this, &OBSBasic::ForceStopStreaming);
    ui->streamButton->setMenu(startStreamMenu);
    ui->statusbar->StreamDelayStopping(sec);
    */
    if (!AFOutputUtil::IsStreamActive()) {
        ChangeStreamStateUI(true, false, "LIVE", 77);
    }

    OUTPUT_HANDLER_LIST::iterator outputIter;
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    for (outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
        if (output == outputIter->second.get()) {
            m_pMainOutput->GetStatusBarTemp().StreamDelayStopping(sec);
            break;
        }
    }

    OnEvent(OBS_FRONTEND_EVENT_STREAMING_STOPPING);
}

void AFMainFrame::qslotStreamingStart(void* output)
{
    emit StreamingStarted();

    auto& auth = AUTH_CONTEXT;
    //
    /*
    ui->statusbar->StreamStarted(outputHandler->streamOutput);
    */
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    OUTPUT_HANDLER_LIST::iterator outputIter;
    if (m_isMainStreaming)
        outputIter = outputHandlers.begin();
    else
        outputIter = outputHandlers.begin() + 1;
    if (outputIter->second.get() == output) {
        m_pMainOutput->GetStatusBarTemp().StreamStarted(outputIter->second->streamOutput);

        OnEvent(OBS_FRONTEND_EVENT_STREAMING_STARTED);

        int buttonSize = 81;
        QString locale = QString::fromStdString(LOCALE_CONTEXT.GetCurrentLocaleStr());
        if (locale != "ko-KR")
            buttonSize = 92;

        ChangeStreamStateUI(true, true, QTStr("Basic.Main.StopBroad"), buttonSize);
        STATISTICS.BroadStatus(true);
        STATISTICS.SetCongestionUpdate(true);

        OnActivate();

        if (auth.IsSoopStreaming())
        {
            QDateTime now = QDateTime::currentDateTime();
            QString formattedTime = now.toString("yyyy-MM-dd HH:mm:ss");

            AFQBroadInfo* pSoopBroadInfo = auth.GetSoopBroadInfo();
            if (pSoopBroadInfo) {
                pSoopBroadInfo->SetBroadStartTime(formattedTime);
                pSoopBroadInfo->BroadNumTimerAfterStart();
            }
        }

#ifdef YOUTUBE_ENABLED
        if (YouTubeAppDock::IsYTServiceSelected())
            youtubeAppDock->IngestionStarted();
#endif

        emit qsignalToggleUseVideo(false);
        blog(LOG_INFO, STREAMING_START);

#ifdef YOUTUBE_ENABLED
        // get a current stream key
        obs_service_t* service_obj = SERVICE_MANAGER.GetService();
        OBSDataAutoRelease settings = obs_service_get_settings(service_obj);
        std::string key = obs_data_get_string(settings, "stream_id");
        if (!key.empty() && !youtubeStreamCheckThread) {
            youtubeStreamCheckThread = CreateQThread([this, key] { YoutubeStreamCheck(key); });
            youtubeStreamCheckThread->setObjectName("YouTubeStreamCheckThread");
            youtubeStreamCheckThread->start();
        }
#endif
        m_pMainOutput->StartStreamingOutputs(false);
    }
}

void AFMainFrame::qslotStreamStopping(void* output)
{
    emit StreamingStopping();

    if (true == m_outputContext->IsStreamingStopping()) {
        ChangeStreamStateUI(false, false, Str("Basic.Main.StoppingStreaming"), 120);

        OnEvent(OBS_FRONTEND_EVENT_STREAMING_STOPPING);
    }
}

void AFMainFrame::qslotStreamingStop(void* output, int code, QString last_error)
{
    if(!output)
        return;

    const char* errorDescription = "";
    DStr errorMessage;
    bool use_last_error = false;
    bool encode_error = false;

    bool isMainStreamStop = false;
    int index = 0;
    std::string strChannelID = "";

    auto& auth = AUTH_CONTEXT;
    //
    OUTPUT_HANDLER_LIST::iterator outputIter;
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    for (outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
        if (outputIter->second.get() == output) {
            if (m_isMainStreaming && index == 0)
                isMainStreamStop = true;
            else if (!m_isMainStreaming && index == 1)
                isMainStreamStop = true;

            AFChannelData* tmpChannel = nullptr;
            auth.GetChannelData(outputIter->first, tmpChannel);
            if (tmpChannel) {
                strChannelID = tmpChannel->pAuthData->channelID;
                if(tmpChannel->pAuthData->platform != PLATFORM_SOOP)
                    tmpChannel->isStreaming = false;
                LoadAccounts();
                if (outputIter->first) {
                    obs_service_release(outputIter->first);
                    outputIter->first = nullptr;
                }
                break;
            }                        
        }
        index++;
    }    
    if (isMainStreamStop) {
        BroadStatusCheckTimerStop();
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
            code = OBS_OUTPUT_SUCCESS; //qslotBroadStartAPIResponse_CheckStream
            auth.SendCheckBroading(this, "qslotBroadStartAPIResponse_CheckStream");
    }

    std::string strErrorDesc = "";
    strErrorDesc = (!strChannelID.empty() ? strChannelID + " : " : "") + errorDescription;
    
    if(use_last_error && !last_error.isEmpty())
        dstr_printf(errorMessage, "%s\n\n%s", strErrorDesc.c_str(), QT_TO_UTF8(last_error));
    else
        dstr_copy(errorMessage, strErrorDesc.c_str());

    bool currentStreamOutputStopped = false;
    for (outputIter = outputHandlers.begin(); outputIter != outputHandlers.end(); ++outputIter) {
        if (output == outputIter->second.get()) {
            m_pMainOutput->GetStatusBarTemp().StreamStopped(outputIter->second->streamOutput);
            currentStreamOutputStopped = true;
            break;
        }
    }

    if(currentStreamOutputStopped)
        m_pMainOutput->SetOutputHandler();

    int streamingOutputRef = m_pMainOutput->GetStreamingOutputRef();
    streamingOutputRef--;
    m_pMainOutput->SetStreamingOutputRef(streamingOutputRef);

    if (true == m_outputContext->IsStreamingStopping()) {

        emit StreamingStopped();

        if (0 >= m_pMainOutput->GetStreamingOutputRef()) {
            m_pMainOutput->SetStreamingOutputRef(0);
            m_outputContext->SetStreamingStopping(false);

            OnEvent(OBS_FRONTEND_EVENT_STREAMING_STOPPED);

            OnDeactivate();

            ToggleBroadTimerUI(false);
            ChangeStreamStateUI(true, false, "LIVE", 77);
            STATISTICS.BroadStatus(false);

#ifdef YOUTUBE_ENABLED
            if (YouTubeAppDock::IsYTServiceSelected())
                youtubeAppDock->IngestionStopped();
#endif

            blog(LOG_INFO, STREAMING_STOP);
        }

        if (OBS_OUTPUT_SUCCESS == code)
        {
            //
        }
    }
    else {
        if (0 >= m_pMainOutput->GetStreamingOutputRef()) {

            m_pMainOutput->SetStreamingOutputRef(0);
            qslotSaveProject();

            OnDeactivate();

            auto userConfig = USERCONFIG;
            //
            bool recordWhenStreaming = config_get_bool(userConfig, "BasicWindow", "RecordWhenStreaming");
            bool keepRecordingWhenStreamStops = config_get_bool(userConfig, "BasicWindow", "KeepRecordingWhenStreamStops");
            if (!keepRecordingWhenStreamStops)
                qslotStopRecording();

            bool replayBufferWhileStreaming = config_get_bool(userConfig, "BasicWindow", "ReplayBufferWhileStreaming");
            bool keepReplayBufferStreamStops = config_get_bool(userConfig, "BasicWindow", "KeepReplayBufferStreamStops");
            if (!keepReplayBufferStreamStops)
                qslotStopReplayBuffer();            

            os_atomic_set_bool(&OUTPUT_CONTEXT.m_streamingActive, false);
            //
            ToggleBroadTimerUI(false);
            ChangeStreamStateUI(true, false, "LIVE", 77);
            STATISTICS.BroadStatus(false);

            OnEvent(OBS_FRONTEND_EVENT_STREAMING_STOPPED);
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

    if (0 == m_pMainOutput->GetStreamingOutputRef()) {

        auto soopBroadInfo = auth.GetSoopBroadInfo();
        if (soopBroadInfo) {
            soopBroadInfo->SetBroadNumber(0);
            soopBroadInfo->SetBroadStartTime("");
        }

        //
        int nMinsimCheckCnt = config_get_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt");
        if (nMinsimCheckCnt > 0) {
            auto endTime = std::chrono::steady_clock::now();
            auto startTime = auth.GetMinsimCheckStartTime();
            if (startTime != std::chrono::steady_clock::time_point{}) {
                auth.InitMinsimCheckStartTime();
            }

        }

        // 
        if (m_blockManager)
        {
            QWidget* broadInfoDock = nullptr;
            m_blockManager->FindBlock(ENUM_WINDOW_TYPE::BroadInfo, broadInfoDock);
            if (broadInfoDock) {
                QMetaObject::invokeMethod(broadInfoDock, "qslotStopStreamingInfoTimer");
            }
        }

        _DeleteBroadInfoData();

        emit qsignalToggleUseVideo(false);
    }

    
    if (isMainStreamStop) {
        AFOutputUtil::StopForceStreaming();
    }    
}

void AFMainFrame::qslotStartRecording()
{
    if(AFOutputUtil::IsRecordingActive())
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

    OnEvent(OBS_FRONTEND_EVENT_RECORDING_STARTING);

    qslotSaveProject();


    if(!AFOutputUtil::StartRecording())
        ui->pushButton_Record->setChecked(false);

    STATISTICS.SetDiskFullTimer(true);

    emit qsignalToggleUseVideo(true);
}

void AFMainFrame::qslotStopRecording()
{
    qslotSaveProject();

    AFOutputUtil::StopRecording();

    OnDeactivate();

    STATISTICS.SetDiskFullTimer(false);

    emit qsignalToggleUseVideo(false);

    os_atomic_set_bool(&OUTPUT_CONTEXT.m_recordingActive, false);
}

void AFMainFrame::qslotRecordingStart()
{
    obs_output_t* output = AFOutputUtil::GetRecordingFileOutput();
    m_pMainOutput->GetStatusBarTemp().RecordingStarted(output);

    emit RecordingStarted(m_signalFlags.isRecordingPausable);

    m_outputContext->SetRecordingStopping(false);

    OnEvent(OBS_FRONTEND_EVENT_RECORDING_STARTED);

    OnActivate();
    m_pMainOutput->UpdatePause();

    emit qsignalToggleUseVideo(true);
    blog(LOG_INFO, RECORDING_START);

    int buttonSize = 73;
    QString locale = QString::fromStdString(LOCALE_CONTEXT.GetCurrentLocaleStr());
    if (locale == "th-TH" || locale == "en-US")
        buttonSize = 110;

    ChangeRecordStateUI(true, true, QTStr("Basic.Main.StopRecord"), buttonSize);

    os_atomic_set_bool(&OUTPUT_CONTEXT.m_recordingActive, true);
}

void AFMainFrame::qslotRecordStopping()
{
    /*
    ui->recordButton->setText(QTStr("Basic.Main.StoppingRecording"));
    */
    emit RecordingStopping();

    m_outputContext->SetRecordingStopping(true);

    os_atomic_set_bool(&OUTPUT_CONTEXT.m_recordingActive, false);

    OnEvent(OBS_FRONTEND_EVENT_RECORDING_STOPPING);
}

void AFMainFrame::qslotRecordingStop(int code, QString last_error)
{
    emit RecordingStopped();

    ChangeRecordStateUI(true, false, "REC", 48);
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
        std::string path = AFOutputUtil::GetLastRecordingPath();
        QString str = QTStr("Basic.StatusBar.RecordingSavedTo");
        ShowSystemAlert(str.arg(QT_UTF8(path.c_str())),"", AFQSystemAlert::AlertIcon::Success);
        //ShowStatusBarMessage());
    }

    OnEvent(OBS_FRONTEND_EVENT_RECORDING_STOPPED);

    m_pMainOutput->AutoRemux(AFOutputUtil::GetLastRecordingPath().c_str());

    OnDeactivate();
    m_pMainOutput->UpdatePause(false);
    STATISTICS.SetDiskFullTimer(false);

    emit qsignalToggleUseVideo(false);

}
void AFMainFrame::qslotRecordingFileChanged(QString lastRecordingPath)
{
    QString str = QTStr("Basic.StatusBar.RecordingSavedTo");
    ShowSystemAlert(str.arg(lastRecordingPath), "", AFQSystemAlert::AlertIcon::Success);
   
    m_pMainOutput->AutoRemux(lastRecordingPath, true);
}
//void AFMainFrame::qslotShowReplayBufferPauseWarning()
//{
//    auto userConfig = USERCONFIG;
//    auto msgBox = []() {
//        QMessageBox msgbox(MAINFRAME);
//        msgbox.setWindowTitle(QTStr("Output.ReplayBuffer." "PauseWarning.Title"));
//        msgbox.setText(QTStr("Output.ReplayBuffer." "PauseWarning.Text"));
//        msgbox.setIcon(QMessageBox::Icon::Information);
//        msgbox.addButton(QMessageBox::Ok);
//        //
//        QCheckBox* cb = new QCheckBox(QTStr("DoNotShowAgain"));
//        msgbox.setCheckBox(cb);
//
//        msgbox.exec();
//
//        if(cb->isChecked()) {
//            config_set_bool(userConfig, "General", "WarnedAboutReplayBufferPausing", true);
//            config_save_safe(userConfig, "tmp", nullptr);
//        }
//    };
//
//    bool warned = config_get_bool(userConfig, "General", "WarnedAboutReplayBufferPausing");
//    if(!warned) {
//        QMetaObject::invokeMethod(App(), "Exec", Qt::QueuedConnection, Q_ARG(VoidFunc, msgBox));
//    }
//}

void AFMainFrame::qslotStartReplayBuffer()
{

    if(AFOutputUtil::IsReplayBufferActive())
        return;

    if(!UIValidation::NoSourcesConfirmation(this)) {
//        recording_paused->first()->setChecked(false);

        SetReplayBufferStartStopMode(false);
        return;
    }
    if(!_OutputPathValid()) {
        _OutputPathInvalidMessage();
        SetReplayBufferStartStopMode(false);
//        recording_paused->first()->setChecked(false);
        return;
    }
    if(_LowDiskSpace()) {
        _DiskSpaceMessage();
        SetReplayBufferStartStopMode(false);
//        recording_paused->first()->setChecked(false);
        return;
    }

    //SetReplayBufferReleased();

    OnEvent(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING);

    qslotSaveProject();

    if(!AFOutputUtil::StartReplayBuffer()) {
        SetReplayBufferStartStopMode(false);
    } 

    //else if(os_atomic_load_bool(&OUTPUT_CONTEXT.m_recordingPaused)) {
    //    qslotShowReplayBufferPauseWarning();
    //}

}
void AFMainFrame::qslotStopReplayBuffer()
{
    if (!AFOutputUtil::StopReplayBuffer())
        return;

    qslotSaveProject();

    OnDeactivate();
}

void AFMainFrame::qslotReplayBufferStart()
{
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if (!outputHandlers[0].second->replayBuffer)
        return;

    SetReplayBufferStartStopMode(true);
    /*
    replayBufferButton->first()->setText(QTStr("Basic.Main.StopReplayBuffer"));
    replayBufferButton->first()->setChecked(true);
    */

    m_outputContext->SetReplayBufferStopping(false);

    OnEvent(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED);

    OnActivate();
    m_pMainOutput->UpdateReplayBuffer();

    blog(LOG_INFO, REPLAY_BUFFER_START);
}

void AFMainFrame::qslotReplayBufferStopping()
{
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if (!outputHandlers[0].second->replayBuffer)
        return;

    SetReplayBufferStoppingMode();
//    replayBufferButton->first()->setText( QTStr("Basic.Main.StoppingReplayBuffer"));

    m_outputContext->SetReplayBufferStopping(true);

    OnEvent(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPING);
}

void AFMainFrame::qslotReplayBufferStop(int code)
{
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if (!outputHandlers[0].second->replayBuffer)
        return;

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

    OnEvent(OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED);

    OnDeactivate();
    m_pMainOutput->UpdateReplayBuffer(false);

    SetReplayBufferStartStopMode(false);
}

void AFMainFrame::qslotStartVirtualCam()
{
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if(outputHandlers.empty())
        return;
    //
    OUTPUT_HANDLER_PTR& outputHandler = outputHandlers[0].second;
    if(!outputHandler ||
       !outputHandler->virtualCam)
        return;

    if(outputHandler->VirtualCamActive())
        return;

    qslotSaveProject();

    if(!outputHandler->StartVirtualCam()) {
        //vcamButton->first()->setChecked(false);
    }
}
void AFMainFrame::qslotStopVirtualCam()
{
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if(outputHandlers.empty())
        return;
    //
    OUTPUT_HANDLER_PTR& outputHandler = outputHandlers[0].second;
    if(!outputHandler ||
       !outputHandler->virtualCam)
        return;

    qslotSaveProject();

    if(AFOutputUtil::IsVirtualCamActive())
        outputHandler->StopVirtualCam();

    OnDeactivate();
}
void AFMainFrame::qslotVirtualCamStart()
{
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if(outputHandlers.empty())
        return;
    //
    OUTPUT_HANDLER_PTR& outputHandler = outputHandlers[0].second;
    if(!outputHandler ||
       !outputHandler->virtualCam)
        return;

    //vcamButton->first()->setText(QTStr("Basic.Main.StopVirtualCam"));
    //if(sysTrayVirtualCam)
    //    sysTrayVirtualCam->setText(QTStr("Basic.Main.StopVirtualCam"));
    //vcamButton->first()->setChecked(true);

    OnEvent(OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED);

    OnActivate();

    blog(LOG_INFO, VIRTUALCAM_START);
}
void AFMainFrame::qslotVirtualCamStop(int code)
{
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if(outputHandlers.empty())
        return;
    //
    OUTPUT_HANDLER_PTR& outputHandler = outputHandlers[0].second;
    if(!outputHandler ||
       !outputHandler->virtualCam)
        return;

    /*vcamButton->first()->setText(QTStr("Basic.Main.StartVirtualCam"));
    if(sysTrayVirtualCam)
        sysTrayVirtualCam->setText(QTStr("Basic.Main.StartVirtualCam"));
    vcamButton->first()->setChecked(false);*/

    OnEvent(OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED);

    blog(LOG_INFO, VIRTUALCAM_STOP);

    OnDeactivate();

    if(!m_restartingVCam)
        return;

    /* Restarting needs to be delayed to make sure that the virtual camera
     * implementation is stopped and avoid race condition. */
    QTimer::singleShot(100, this, &AFMainFrame::RestartingVirtualCam);
}

//void AFMainFrame::qslotPauseRecording()
//{
//    if(AFOutputUtil::PauseRecording())
//    {
//        /*pause->setAccessibleName(QTStr("Basic.Main.UnpauseRecording"));
//        pause->setToolTip(QTStr("Basic.Main.UnpauseRecording"));
//        pause->blockSignals(true);
//        pause->setChecked(true);
//        pause->blockSignals(false);
//
//        emit RecordingPaused();
// 
//        ui->statusbar->RecordingPaused();*/
//
//        /*auto replay = replayBufferButton ? replayBufferButton->second() : nullptr;
//        if(replay)
//            replay->setEnabled(false);*/
//
//        OnEvent(OBS_FRONTEND_EVENT_RECORDING_PAUSED);
//
//        if(os_atomic_load_bool(&m_outputContext->m_replaybufActive))
//            qslotShowReplayBufferPauseWarning();
//    }
//}
//void AFMainFrame::qslotUnpauseRecording()
//{
//    if(AFOutputUtil::UnPauseRecording())
//    {
//        /*pause->setAccessibleName(QTStr("Basic.Main.PauseRecording"));
//        pause->setToolTip(QTStr("Basic.Main.PauseRecording"));
//        pause->blockSignals(true);
//        pause->setChecked(false);
//        pause->blockSignals(false);
//
//        emit RecordingUnpaused();
// 
//        ui->statusbar->RecordingUnpaused();*/
//
//        /*auto replay = replayBufferButton ? replayBufferButton->second() : nullptr;
//        if(replay)
//            replay->setEnabled(true);*/
//
//        OnEvent(OBS_FRONTEND_EVENT_RECORDING_UNPAUSED);
//    }
//}

static inline void SetEncoderName(obs_encoder_t* encoder, const char* name,
    const char* defaultName)
{
    obs_encoder_set_name(encoder, (name && *name) ? name : defaultName);
}

//
void AFMainFrame::StopReplayBuffer()
{
    qslotStopReplayBuffer();
}
//
extern void log_vcam_changed(const VCamConfig& config, bool starting);
obs_output_t* AFMainFrame::GetVirtualCamOutput()
{
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if(outputHandlers.empty())
        return nullptr;
    //
    OUTPUT_HANDLER_PTR& outputHandler = outputHandlers[0].second;
    if(!outputHandler ||
       !outputHandler->virtualCam)
        return nullptr;
    //
    OBSOutput output = outputHandler->virtualCam.Get();
    return obs_output_get_ref(output);
}
void AFMainFrame::SetVirtualCamOutputType(const VCamOutputType type)
{
    m_vcamConfig.type = type;
    //
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if(outputHandlers.empty())
        return;
    //
    OUTPUT_HANDLER_PTR& outputHandler = outputHandlers[0].second;
    if(!outputHandler)
        return;

    outputHandler->UpdateVirtualCamOutputSource();
}
void AFMainFrame::UpdateVirtualCamConfig(const VCamConfig& config)
{
    m_vcamConfig = config;
    //
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if(outputHandlers.empty())
        return;
    //
    OUTPUT_HANDLER_PTR& outputHandler = outputHandlers[0].second;
    if(!outputHandler)
        return;

    outputHandler->UpdateVirtualCamOutputSource();
    log_vcam_changed(config, false);
}
void AFMainFrame::RestartVirtualCam(const VCamConfig& config)
{
    m_restartingVCam = true;

    qslotStopVirtualCam();

    m_vcamConfig = config;
}
void AFMainFrame::RestartingVirtualCam()
{
    OUTPUT_HANDLER_LIST& outputHandlers = m_outputContext->GetOutputHandlerLists();
    if(outputHandlers.empty())
        return;
    //
    OUTPUT_HANDLER_PTR& outputHandler = outputHandlers[0].second;
    if(!outputHandler)
        return;
    //
    if(!m_restartingVCam)
        return;

    outputHandler->UpdateVirtualCamOutputSource();
    qslotStartVirtualCam();
    m_restartingVCam = false;
}