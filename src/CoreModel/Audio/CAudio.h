#pragma once

#include <obs.hpp>
#include <util/util.hpp>

namespace AFAudioUtil {
	void CreateFirstRunSources();
	int	 ResetAudio();
	void ResetAudioDevice(const char* sourceId, const char* deviceId, const char* deviceDesc, int channel);
	void LoadAudioMonitoring();

	void LogFilter(obs_source_t*, obs_source_t* filter, void* v_val);
	void LoadAudioDevice(const char* name, int channel, obs_data_t* parent);
	inline bool HasAudioDevices(const char* source_id);
};

extern char* get_new_source_name(const char* name, const char* format);

