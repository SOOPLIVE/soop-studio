#pragma once

#include <mutex>

#include "media-io/audio-resampler.h"

#include <vector>

class SOOPAudioResample
{
public:
	SOOPAudioResample() {}
	virtual ~SOOPAudioResample() {}

	bool Initialize();
	void Finalize();

	bool SetInputFormat(int iSPS, enum audio_format eAF, enum speaker_layout eSL);
	bool SetInputBuffer(const struct audio_data* audio_data);
	bool SetOutputFormat(int iSPS, enum audio_format eAF, enum speaker_layout eSL);
	bool GetOutputBuffer(std::vector<std::vector<uint8_t>>& tcVV);

	void Clear() { _Clear(); }

private:
	void _Thread();
	bool _Work();
	void _Clear();
	//

	std::mutex m_cIM; // Input Mutex
	int m_iISPS = 0; // Input Samples Per Sec
	enum audio_format m_eIAF = {}; // Input Audio Format
	enum speaker_layout m_eISL = {}; // Input Speaker Layout
	int m_iMinIF = 0; // Minimum Input Frames
	int m_iMinIS = 0; // Minimum Input Size
	int m_iMaxIS = 0; // Maximum Input Size
	std::vector<std::vector<uint8_t>> m_tcIVV; // Input Vector Vector [Channle][PCM]

	std::mutex m_cOM; // Output Mutex
	int m_iOSPS = 0; // Output Samples Per Sec
	enum audio_format m_eOAF = {}; // Output Audio Format
	enum speaker_layout m_eOSL = {}; // Output Speaker Layout
	int m_iOBPC = 0; // Output Size Per Cycle
	int m_iLate = 0;
	std::vector<std::vector<uint8_t>> m_tcOVV; // Output Vector Vector [Channle][PCM]

	std::shared_ptr<audio_resampler_t> m_tcSP = nullptr; // audio_resampler_t

	//

	std::chrono::steady_clock::time_point m_cRefTime;
	bool m_bWork = false;
	std::thread m_cThread;
};
