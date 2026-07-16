#include "CStatusbarTemp.h"

#include "Application/CApplication.h"

#include "CoreModel/Statistics/CStatistics.h"
#include "CoreModel/Auth/CAuthManager.h"

#include "MainFrame/CMainFrame.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "UIComponent/CMessageBox.h"
//
void AFStatusbarTemp::SetOutputHandler(AFBasicOutputHandler* handler) 
{ 
	if (!handler)
		return;

	m_pOutputHandler = handler; 
	m_pStreamOutput = m_pOutputHandler->streamOutput;
}

void AFStatusbarTemp::StreamDelayStarting(int sec)
{
    if(!m_pOutputHandler) {
        return;
    }
    m_pStreamOutput = m_pOutputHandler->streamOutput;

    m_delaySecTotal = m_delaySecStarting = sec;

    UpdateDelayMsg();
    Activate();
}
void AFStatusbarTemp::StreamDelayStopping(int sec)
{
	m_delaySecTotal = m_delaySecStopping = sec;
	UpdateDelayMsg();
}
void AFStatusbarTemp::StreamStarted(obs_output_t* output)
{
	if (!output)
		return;

	m_streamSigs.emplace_back(obs_output_get_signal_handler(output), "reconnect", OBSOutputReconnect, this);
	m_streamSigs.emplace_back(obs_output_get_signal_handler(output), "reconnect_success", OBSOutputReconnectSuccess, this);

	m_retries = 0;
	m_lastBytesSent = 0;
	m_lastBytesSentTime = os_gettime_ns();
	Activate();
}
void AFStatusbarTemp::StreamStopped(obs_output_t* output)
{
	if (!output)
		return;

	signal_handler_disconnect(obs_output_get_signal_handler(output), "reconnect", OBSOutputReconnect, this);
	signal_handler_disconnect(obs_output_get_signal_handler(output), "reconnect_success", OBSOutputReconnectSuccess, this);

	if(m_pStreamOutput) {
		ReconnectClear();
		m_pStreamOutput = nullptr;
		Deactivate();
	}
}
void AFStatusbarTemp::RecordingStarted(obs_output_t* output)
{
	m_pRecordOutput = output;
	Activate();
}
void AFStatusbarTemp::RecordingStopped()
{
	m_pRecordOutput = nullptr;
	Deactivate();
}
void AFStatusbarTemp::RecordingPaused()
{
	/*QString text = statusWidget->ui->recordTime->text() +
		QStringLiteral(" (PAUSED)");
	statusWidget->ui->recordTime->setText(text);

	if(m_pRecordOutput) {
		statusWidget->ui->recordIcon->setPixmap(recordingPausePixmap);
		m_streamPauseIconToggle = true;
	}*/
}
void AFStatusbarTemp::RecordingUnpaused()
{
	/*if(m_pRecordOutput) {
		statusWidget->ui->recordIcon->setPixmap(recordingActivePixmap);
	}*/
}

void AFStatusbarTemp::ClearAllSignals() {
	m_streamSigs.clear();
}

void AFStatusbarTemp::ReconnectClear()
{
	m_retries = 0;
	m_reconnectTimeout = 0;
	m_seconds = -1;
	m_lastBytesSent = 0;
	m_lastBytesSentTime = os_gettime_ns();
	m_delaySecTotal = 0;
	UpdateDelayMsg();
}

void AFStatusbarTemp::Activate()
{
	if(!m_active) {
		m_refreshTimer = new QTimer(this);
		connect(m_refreshTimer, &QTimer::timeout, this, &AFStatusbarTemp::UpdateStatusBar);

		int skipped = video_output_get_skipped_frames(obs_get_video());
		int total = video_output_get_total_frames(obs_get_video());

		m_totalStreamSeconds = 0;
		m_totalRecordSeconds = 0;
		m_lastSkippedFrameCount = 0;
		m_startSkippedFrameCount = skipped;
		m_startTotalFrameCount = total;

		m_refreshTimer->start(1000);
		m_active = true;

		if(m_pStreamOutput) {
			//statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
		}
	}

	if(m_pStreamOutput) {
		/*statusWidget->ui->streamIcon->setPixmap(streamingActivePixmap);
		statusWidget->ui->streamTime->setDisabled(false);
		statusWidget->ui->issuesFrame->show();
		statusWidget->ui->kbps->show();*/
		m_firstCongestionUpdate = true;
	}

	if(m_pRecordOutput) {
		/*statusWidget->ui->recordIcon->setPixmap(recordingActivePixmap);
		statusWidget->ui->recordTime->setDisabled(false);*/
	}
}
void AFStatusbarTemp::Deactivate()
{
	if(!m_pStreamOutput) {
		/*statusWidget->ui->streamTime->setText(QString("00:00:00"));
		statusWidget->ui->streamTime->setDisabled(true);
		statusWidget->ui->streamIcon->setPixmap(streamingInactivePixmap);
		statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
		statusWidget->ui->delayFrame->hide();
		statusWidget->ui->issuesFrame->hide();
		statusWidget->ui->kbps->hide();*/
		m_totalStreamSeconds = 0;
		m_disconnected = false;
		m_firstCongestionUpdate = false;

		auto& statistics = STATISTICS;
		statistics.SetDisconnected(m_disconnected);
		statistics.SetCongestionUpdate(m_firstCongestionUpdate);
		statistics.ClearCongestionArray();
	}

	if(!m_pRecordOutput) {
		/*statusWidget->ui->recordTime->setText(QString("00:00:00"));
		statusWidget->ui->recordTime->setDisabled(true);
		statusWidget->ui->recordIcon->setPixmap(recordingInactivePixmap);*/
		m_totalRecordSeconds = 0;
	}

	if(m_pOutputHandler && !m_pOutputHandler->Active()) {
		delete m_refreshTimer;

		/*statusWidget->ui->delayInfo->setText("");
		statusWidget->ui->droppedFrames->setText(QTStr("DroppedFrames").arg("0", "0.0"));
		statusWidget->ui->kbps->setText("0 kbps");*/

		m_delaySecTotal = 0;
		m_delaySecStarting = 0;
		m_delaySecStopping = 0;
		m_reconnectTimeout = 0;
		m_active = false;
		m_overloadedNotify = true;

		//statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
	}
}

void AFStatusbarTemp::UpdateDelayMsg()
{
	QString msg;

	if(m_delaySecTotal) {
		if(m_delaySecStarting && !m_delaySecStopping) {
			msg = QTStr("Basic.StatusBar.DelayStartingIn");
			msg = msg.arg(QString::number(m_delaySecStarting));

		} else if(!m_delaySecStarting && m_delaySecStopping) {
			msg = QTStr("Basic.StatusBar.DelayStoppingIn");
			msg = msg.arg(QString::number(m_delaySecStopping));

		} else if(m_delaySecStarting && m_delaySecStopping) {
			msg = QTStr("Basic.StatusBar.DelayStartingStoppingIn");
			msg = msg.arg(QString::number(m_delaySecStopping),
					  QString::number(m_delaySecStarting));
		} else {
			msg = QTStr("Basic.StatusBar.Delay");
			msg = msg.arg(QString::number(m_delaySecTotal));
		}
	}
}

void AFStatusbarTemp::UpdateBandwidth() {}
void AFStatusbarTemp::UpdateStreamTime()
{
	m_totalStreamSeconds++;

	int seconds = m_totalStreamSeconds % 60;
	int totalMinutes = m_totalStreamSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	QString text = QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
//	statusWidget->ui->streamTime->setText(text);
	/*if(m_pStreamOutput && !statusWidget->ui->streamTime->isEnabled())
		statusWidget->ui->streamTime->setDisabled(false);*/

	if(m_reconnectTimeout > 0) {
		QString msg = QTStr("Basic.StatusBar.Reconnecting")
			.arg(QString::number(m_retries),
			 QString::number(m_reconnectTimeout));
		m_disconnected = true;
		STATISTICS.SetDisconnected(m_disconnected);
		STATISTICS.ClearCongestionArray();
//		statusWidget->ui->statusIcon->setPixmap(disconnectedPixmap);
//		congestionArray.clear();
		m_reconnectTimeout--;

	} else if(m_retries > 0) {
		QString msg = QTStr("Basic.StatusBar.AttemptingReconnect");
	}

	if(m_delaySecStopping > 0 || m_delaySecStarting > 0) {
		if(m_delaySecStopping > 0)
			--m_delaySecStopping;
		if(m_delaySecStarting > 0)
			--m_delaySecStarting;
		UpdateDelayMsg();
	}
}
volatile bool recording_paused = false;
void AFStatusbarTemp::UpdateRecordTime()
{
	bool paused = os_atomic_load_bool(&recording_paused);
	if(!paused)
	{
		m_totalRecordSeconds++;

		int seconds = m_totalRecordSeconds % 60;
		int totalMinutes = m_totalRecordSeconds / 60;
		int minutes = totalMinutes % 60;
		int hours = totalMinutes / 60;

		QString text = QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);

//		statusWidget->ui->recordTime->setText(text);
		/*if(m_pRecordOutput && !statusWidget->ui->recordTime->isEnabled())
			statusWidget->ui->recordTime->setDisabled(false);*/
	} else {
		/*statusWidget->ui->recordIcon->setPixmap(m_streamPauseIconToggle
												? recordingPauseInactivePixmap
												: recordingPausePixmap);*/

		m_streamPauseIconToggle = !m_streamPauseIconToggle;
	}
}
void AFStatusbarTemp::UpdateDroppedFrames() {}

void AFStatusbarTemp::OBSOutputReconnect(void* data, calldata_t* params)
{
	AFStatusbarTemp* statusBar = reinterpret_cast<AFStatusbarTemp*>(data);
	int seconds = (int)calldata_int(params, "timeout_sec");
	QMetaObject::invokeMethod(statusBar, "Reconnect", Q_ARG(int, seconds));

	obs_output_t* output = (obs_output_t*)calldata_ptr(params, "output");
	QString channelID = MAINFRAME->GetChannelID(output);
	QString alertText = QTStr("Basic.SystemTray.Message.Reconnecting");

	QMetaObject::invokeMethod(statusBar, 
							  "qslotShowSystemAlert", 
							   Q_ARG(QString, channelID), 
							   Q_ARG(QString, alertText));
}

void AFStatusbarTemp::OBSOutputReconnectSuccess(void* data, calldata_t* params)
{
	AFStatusbarTemp* statusBar = reinterpret_cast<AFStatusbarTemp*>(data);
	QMetaObject::invokeMethod(statusBar, "ReconnectSuccess");
}
//
void AFStatusbarTemp::Reconnect(int seconds)
{
	m_reconnectTimeout = seconds;
	if(m_pStreamOutput) {
		m_delaySecTotal = obs_output_get_active_delay(m_pStreamOutput);
		UpdateDelayMsg();

		m_retries++;
	}
}
void AFStatusbarTemp::ReconnectSuccess()
{
	ReconnectClear();

	if(m_pStreamOutput) {
		m_delaySecTotal = obs_output_get_active_delay(m_pStreamOutput);
		UpdateDelayMsg();
		m_disconnected = false;
		m_firstCongestionUpdate = true;
		STATISTICS.SetDisconnected(m_disconnected);
		STATISTICS.SetCongestionUpdate(m_firstCongestionUpdate);

		if (AUTH_CONTEXT.IsSoopRegistered()) {
			AUTH_CONTEXT.SendCheckBroadStart(this, "qslotBroadStartAPIResponse_Reconnect");
		}
		else
		{
			QString channelID = MAINFRAME->GetChannelID(m_pStreamOutput);
			QString alertText = QTStr("Basic.StatusBar.ReconnectSuccessful");
			qslotShowSystemAlert(channelID, alertText);
		}
	}
}
void AFStatusbarTemp::UpdateStatusBar() 
{
	if (m_pStreamOutput)
		UpdateStreamTime();
	if (m_pRecordOutput)
		UpdateRecordTime();
}

void AFStatusbarTemp::UpdateCurrentFPS() {}
void AFStatusbarTemp::UpdateIcons() {}

void AFStatusbarTemp::qslotBroadStartAPIResponse_Reconnect(const QByteArray& responseData) {
	bool result = false;

	BroadStartAPI_s info = {};

	std::string jsonString = responseData.toStdString();
	std::string err = "";
}

void AFStatusbarTemp::qslotShowSystemAlert(QString channelID, QString alertText) {
	MAINFRAME->ShowSystemAlert(alertText, channelID);
}
