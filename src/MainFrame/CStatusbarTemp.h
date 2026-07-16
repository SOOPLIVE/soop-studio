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
	AFBasicOutputHandler* GetOutputHandler() { return m_pOutputHandler; }
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

	void qslotBroadStartAPIResponse_Reconnect(const QByteArray& responseData);
	void qslotShowSystemAlert(QString channelID, QString alertText);

#pragma endregion private func

#pragma region private member var
private:
	AFBasicOutputHandler* m_pOutputHandler = nullptr;
	obs_output_t* m_pStreamOutput = nullptr;
	std::vector<OBSSignal> m_streamSigs;
	obs_output_t* m_pRecordOutput = nullptr;
	//
	bool m_active = false;
	bool m_overloadedNotify = true;
	bool m_streamPauseIconToggle = false;
	bool m_disconnected = false;
	bool m_firstCongestionUpdate = false;

	int m_retries = 0;
	int m_totalStreamSeconds = 0;
	int m_totalRecordSeconds = 0;

	int m_reconnectTimeout = 0;

	int m_delaySecTotal = 0;
	int m_delaySecStarting = 0;
	int m_delaySecStopping = 0;

	int m_startSkippedFrameCount = 0;
	int m_startTotalFrameCount = 0;
	int m_lastSkippedFrameCount = 0;

	int m_seconds = 0;
	uint64_t m_lastBytesSent = 0;
	uint64_t m_lastBytesSentTime = 0;
    //

	QPointer<QTimer> m_refreshTimer;
#pragma endregion private member var
};
