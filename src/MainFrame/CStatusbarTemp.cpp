#include "CStatusbarTemp.h"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"


//
void AFStatusbarTemp::SetOutputHandler(AFBasicOutputHandler* handler) 
{ 
	if (!handler)
		return;

	outputHandler = handler; 
	streamOutput = outputHandler->streamOutput;
}

void AFStatusbarTemp::StreamDelayStarting(int sec)
{
    if(!outputHandler) {
        return;
    }
    streamOutput = outputHandler->streamOutput;

    delaySecTotal = delaySecStarting = sec;

    UpdateDelayMsg();
    Activate();
}
void AFStatusbarTemp::StreamDelayStopping(int sec)
{
	delaySecTotal = delaySecStopping = sec;
	UpdateDelayMsg();
}
void AFStatusbarTemp::StreamStarted(obs_output_t* output)
{
	if (!output)
		return;

	streamSigs.emplace_back(obs_output_get_signal_handler(output), "reconnect", OBSOutputReconnect, this);
	streamSigs.emplace_back(obs_output_get_signal_handler(output), "reconnect_success", OBSOutputReconnectSuccess, this);

	retries = 0;
	lastBytesSent = 0;
	lastBytesSentTime = os_gettime_ns();
	Activate();
}
void AFStatusbarTemp::StreamStopped(obs_output_t* output)
{
	if (!output)
		return;

	signal_handler_disconnect(
		obs_output_get_signal_handler(output),
		"reconnect", OBSOutputReconnect, this);
	signal_handler_disconnect(
		obs_output_get_signal_handler(output),
		"reconnect_success", OBSOutputReconnectSuccess, this);

	if(streamOutput) {
		ReconnectClear();
		streamOutput = nullptr;
		qslotClearMessage();
		Deactivate();
	}
}
void AFStatusbarTemp::RecordingStarted(obs_output_t* output)
{
	recordOutput = output;
	Activate();
}
void AFStatusbarTemp::RecordingStopped()
{
	recordOutput = nullptr;
	Deactivate();
}
void AFStatusbarTemp::RecordingPaused()
{
	/*QString text = statusWidget->ui->recordTime->text() +
		QStringLiteral(" (PAUSED)");
	statusWidget->ui->recordTime->setText(text);

	if(recordOutput) {
		statusWidget->ui->recordIcon->setPixmap(recordingPausePixmap);
		streamPauseIconToggle = true;
	}*/
}
void AFStatusbarTemp::RecordingUnpaused()
{
	/*if(recordOutput) {
		statusWidget->ui->recordIcon->setPixmap(recordingActivePixmap);
	}*/
}

void AFStatusbarTemp::ClearAllSignals() {
	streamSigs.clear();
}

void AFStatusbarTemp::ReconnectClear()
{
	retries = 0;
	reconnectTimeout = 0;
	seconds = -1;
	lastBytesSent = 0;
	lastBytesSentTime = os_gettime_ns();
	delaySecTotal = 0;
	UpdateDelayMsg();
}

void AFStatusbarTemp::qslotShowMessage(const QString& message, int timeout)
{
	/*messageTimer->stop();

	statusWidget->ui->message->setText(message);

	if(timeout)
		messageTimer->start(timeout);*/
}
void AFStatusbarTemp::qslotClearMessage()
{
	//statusWidget->ui->message->setText("");
}
//
void AFStatusbarTemp::Activate()
{
	if(!active) {
		m_refreshTimer = new QTimer(this);
		connect(m_refreshTimer, &QTimer::timeout, this, &AFStatusbarTemp::UpdateStatusBar);

		int skipped = video_output_get_skipped_frames(obs_get_video());
		int total = video_output_get_total_frames(obs_get_video());

		totalStreamSeconds = 0;
		totalRecordSeconds = 0;
		lastSkippedFrameCount = 0;
		startSkippedFrameCount = skipped;
		startTotalFrameCount = total;

		m_refreshTimer->start(1000);
		active = true;

		if(streamOutput) {
			//statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
		}
	}

	if(streamOutput) {
		/*statusWidget->ui->streamIcon->setPixmap(streamingActivePixmap);
		statusWidget->ui->streamTime->setDisabled(false);
		statusWidget->ui->issuesFrame->show();
		statusWidget->ui->kbps->show();*/
		firstCongestionUpdate = true;
	}

	if(recordOutput) {
		/*statusWidget->ui->recordIcon->setPixmap(recordingActivePixmap);
		statusWidget->ui->recordTime->setDisabled(false);*/
	}
}
void AFStatusbarTemp::Deactivate()
{
	if(!streamOutput) {
		/*statusWidget->ui->streamTime->setText(QString("00:00:00"));
		statusWidget->ui->streamTime->setDisabled(true);
		statusWidget->ui->streamIcon->setPixmap(streamingInactivePixmap);
		statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
		statusWidget->ui->delayFrame->hide();
		statusWidget->ui->issuesFrame->hide();
		statusWidget->ui->kbps->hide();*/
		totalStreamSeconds = 0;
		disconnected = false;
		firstCongestionUpdate = false;
		App()->GetStatistics()->SetDisconnected(disconnected);
		App()->GetStatistics()->SetCongestionUpdate(firstCongestionUpdate);
		App()->GetStatistics()->ClearCongestionArray();
	}

	if(!recordOutput) {
		/*statusWidget->ui->recordTime->setText(QString("00:00:00"));
		statusWidget->ui->recordTime->setDisabled(true);
		statusWidget->ui->recordIcon->setPixmap(recordingInactivePixmap);*/
		totalRecordSeconds = 0;
	}

	if(outputHandler && !outputHandler->Active()) {
		delete m_refreshTimer;

		/*statusWidget->ui->delayInfo->setText("");
		statusWidget->ui->droppedFrames->setText(QTStr("DroppedFrames").arg("0", "0.0"));
		statusWidget->ui->kbps->setText("0 kbps");*/

		delaySecTotal = 0;
		delaySecStarting = 0;
		delaySecStopping = 0;
		reconnectTimeout = 0;
		active = false;
		overloadedNotify = true;

		//statusWidget->ui->statusIcon->setPixmap(inactivePixmap);
	}
}

void AFStatusbarTemp::UpdateDelayMsg()
{
	QString msg;

	if(delaySecTotal) {
		if(delaySecStarting && !delaySecStopping) {
			msg = QTStr("Basic.StatusBar.DelayStartingIn");
			msg = msg.arg(QString::number(delaySecStarting));

		} else if(!delaySecStarting && delaySecStopping) {
			msg = QTStr("Basic.StatusBar.DelayStoppingIn");
			msg = msg.arg(QString::number(delaySecStopping));

		} else if(delaySecStarting && delaySecStopping) {
			msg = QTStr("Basic.StatusBar.DelayStartingStoppingIn");
			msg = msg.arg(QString::number(delaySecStopping),
					  QString::number(delaySecStarting));
		} else {
			msg = QTStr("Basic.StatusBar.Delay");
			msg = msg.arg(QString::number(delaySecTotal));
		}
	}
}

void AFStatusbarTemp::UpdateBandwidth() {}
void AFStatusbarTemp::UpdateStreamTime()
{
	totalStreamSeconds++;

	int seconds = totalStreamSeconds % 60;
	int totalMinutes = totalStreamSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	QString text = QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
//	statusWidget->ui->streamTime->setText(text);
	/*if(streamOutput && !statusWidget->ui->streamTime->isEnabled())
		statusWidget->ui->streamTime->setDisabled(false);*/

	if(reconnectTimeout > 0) {
		QString msg = QTStr("Basic.StatusBar.Reconnecting")
			.arg(QString::number(retries),
			 QString::number(reconnectTimeout));
		qslotShowMessage(msg);
		disconnected = true;
		App()->GetStatistics()->SetDisconnected(disconnected);
		App()->GetStatistics()->ClearCongestionArray();
//		statusWidget->ui->statusIcon->setPixmap(disconnectedPixmap);
//		congestionArray.clear();
		reconnectTimeout--;

	} else if(retries > 0) {
		QString msg = QTStr("Basic.StatusBar.AttemptingReconnect");
		qslotShowMessage(msg.arg(QString::number(retries)));
	}

	if(delaySecStopping > 0 || delaySecStarting > 0) {
		if(delaySecStopping > 0)
			--delaySecStopping;
		if(delaySecStarting > 0)
			--delaySecStarting;
		UpdateDelayMsg();
	}
}
volatile bool recording_paused = false;
void AFStatusbarTemp::UpdateRecordTime()
{
	bool paused = os_atomic_load_bool(&recording_paused);
	if(!paused)
	{
		totalRecordSeconds++;

		int seconds = totalRecordSeconds % 60;
		int totalMinutes = totalRecordSeconds / 60;
		int minutes = totalMinutes % 60;
		int hours = totalMinutes / 60;

		QString text = QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);

//		statusWidget->ui->recordTime->setText(text);
		/*if(recordOutput && !statusWidget->ui->recordTime->isEnabled())
			statusWidget->ui->recordTime->setDisabled(false);*/
	} else {
		/*statusWidget->ui->recordIcon->setPixmap(streamPauseIconToggle
												? recordingPauseInactivePixmap
												: recordingPausePixmap);*/

		streamPauseIconToggle = !streamPauseIconToggle;
	}
}
void AFStatusbarTemp::UpdateDroppedFrames() {}

void AFStatusbarTemp::OBSOutputReconnect(void* data, calldata_t* params)
{
	AFStatusbarTemp* statusBar = reinterpret_cast<AFStatusbarTemp*>(data);
	int seconds = (int)calldata_int(params, "timeout_sec");
	QMetaObject::invokeMethod(statusBar, "Reconnect", Q_ARG(int, seconds));

	obs_output_t* output = (obs_output_t*)calldata_ptr(params, "output");
	QString channelID = App()->GetMainView()->GetChannelID(output);
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

	obs_output_t* output = (obs_output_t*)calldata_ptr(params, "output");
	QString channelID = App()->GetMainView()->GetChannelID(output);
	QString alertText = QTStr("Basic.StatusBar.ReconnectSuccessful");

	QMetaObject::invokeMethod(statusBar,
		"qslotShowSystemAlert",
		Q_ARG(QString, channelID),
		Q_ARG(QString, alertText));
}
//
void AFStatusbarTemp::Reconnect(int seconds)
{
	reconnectTimeout = seconds;
	if(streamOutput) {
		delaySecTotal = obs_output_get_active_delay(streamOutput);
		UpdateDelayMsg();

		retries++;
	}
}
void AFStatusbarTemp::ReconnectSuccess()
{
	QString msg = QTStr("Basic.StatusBar.ReconnectSuccessful");
	qslotShowMessage(msg, 4000);

	ReconnectClear();

	if(streamOutput) {
		delaySecTotal = obs_output_get_active_delay(streamOutput);
		UpdateDelayMsg();
		disconnected = false;
		firstCongestionUpdate = true;
		App()->GetStatistics()->SetDisconnected(disconnected);
		App()->GetStatistics()->SetCongestionUpdate(firstCongestionUpdate);
	}
}
void AFStatusbarTemp::UpdateStatusBar() 
{
	if (streamOutput)
		UpdateStreamTime();
	if (recordOutput)
		UpdateRecordTime();
}

void AFStatusbarTemp::UpdateCurrentFPS() {}
void AFStatusbarTemp::UpdateIcons() {}

void AFStatusbarTemp::qslotShowSystemAlert(QString channelID, QString alertText) {
	App()->GetMainView()->ShowSystemAlert(alertText, channelID);
}