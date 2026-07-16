#pragma once

#include "MainFrame/CMainFrame.h"

#include "../CStatusbarTemp.h"

class CMainOutput : public QObject
{
	Q_OBJECT

public:
	CMainOutput(QObject* parent);
	~CMainOutput() {}

	inline AFStatusbarTemp& GetStatusBarTemp() { return m_statusbar; }

public:
	void ResetOutputs();
	void SetStreamingOutput();
	bool PrepareStreamingOutput(int index, obs_service_t* service);
	bool StartStreamingOutputs(bool isMain = true);
	bool StopStreamingOutput(obs_service_t* service);
	void SetOutputHandler();
	bool IsStreamingOnlySoop();

	inline void EnableOutputs(bool enable)
	{
		if (enable) {
			if (--m_disableOutputsRef < 0)
				m_disableOutputsRef = 0;
		}
		else {
			m_disableOutputsRef++;
		}
	};

	void ClearAllStreamSignals();

	void AutoRemux(QString input, bool noShow = false);
	void UpdatePause(bool activate = true);
	void UpdateReplayBuffer(bool activate = true);

	int  GetStreamingOutputRef() { return m_streamingOutputRef; }
	void SetStreamingOutputRef(int ref) { m_streamingOutputRef = ref; }

private:
	AFStatusbarTemp m_statusbar;

	int m_disableOutputsRef = 0;
	int m_streamingOutputRef = 0;
};