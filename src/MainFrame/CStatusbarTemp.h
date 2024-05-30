#pragma once

#include <obs.h>
#include <qstring.h>
#include <qobject.h>

#include <vector>
#include <QPointer>
#include <QTimer>

#include "CoreModel/OBSOutput/SBasicOutputHandler.h"

class AFStatusbarTemp : public QObject
{
	Q_OBJECT
#pragma region QT Field
public:
    AFStatusbarTemp() {};
    ~AFStatusbarTemp() {};

#pragma endregion QT Field

#pragma region public func
public:
	void SetOutputHandler(AFBasicOutputHandler* handler);
	AFBasicOutputHandler* GetOutputHandler() { return outputHandler; }
	void StreamDelayStarting(int sec);
	void StreamDelayStopping(int sec);
	void StreamStarted(obs_output_t* output);
	void StreamStopped(obs_output_t* output);
	void RecordingStarted(obs_output_t* output);
	void RecordingStopped();
	void RecordingPaused();
	void RecordingUnpaused();

	void ClearAllSignals();
	void ReconnectClear();

public slots:
	void qslotShowMessage(const QString& message, int timeout = 0);
	void qslotClearMessage();
#pragma endregion public func

#pragma region protected func
protected:

#pragma endregion protected func

#pragma region private func
private:
	void Activate();
	void Deactivate();

	void UpdateDelayMsg();
	void UpdateBandwidth();
	void UpdateStreamTime();
	void UpdateRecordTime();
	void UpdateDroppedFrames();

	static void OBSOutputReconnect(void* data, calldata_t* params);
	static void OBSOutputReconnectSuccess(void* data, calldata_t* params);

private slots:
	void Reconnect(int seconds);
	void ReconnectSuccess();
	void UpdateStatusBar();
	void UpdateCurrentFPS();
	void UpdateIcons();

	void qslotShowSystemAlert(QString channelID, QString alertText);

#pragma endregion private func

#pragma region private member var
private:
	AFBasicOutputHandler* outputHandler = nullptr;
	obs_output_t* streamOutput = nullptr;
	std::vector<OBSSignal> streamSigs;
	obs_output_t* recordOutput = nullptr;
	//
	bool active = false;
	bool overloadedNotify = true;
	bool streamPauseIconToggle = false;
	bool disconnected = false;
	bool firstCongestionUpdate = false;

	int retries = 0;
	int totalStreamSeconds = 0;
	int totalRecordSeconds = 0;

	int reconnectTimeout = 0;

	int delaySecTotal = 0;
	int delaySecStarting = 0;
	int delaySecStopping = 0;

	int startSkippedFrameCount = 0;
	int startTotalFrameCount = 0;
	int lastSkippedFrameCount = 0;

	int seconds = 0;
	uint64_t lastBytesSent = 0;
	uint64_t lastBytesSentTime = 0;

	QPointer<QTimer> m_refreshTimer;
#pragma endregion private member var
};
