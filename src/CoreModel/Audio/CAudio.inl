#pragma once

#include <string>


static void LogFilter(obs_source_t*, obs_source_t* filter, void* v_val)
{
	const char* name = obs_source_get_name(filter);
	const char* id = obs_source_get_id(filter);
	int val = (int)(intptr_t)v_val;
	std::string indent;
	for(int i = 0; i < val; i++) {
		indent += "    ";
	}
	blog(LOG_INFO, "%s- filter: '%s' (%s)", indent.c_str(), name, id);
};

static void LoadAudioDevice(const char* name, int channel, obs_data_t* parent)
{
	OBSDataAutoRelease data = obs_data_get_obj(parent, name);
	if(!data) {
		return;
	}

	OBSSourceAutoRelease source = obs_load_source(data);
	if(!source) {
		return;
	}

	obs_set_output_source(channel, source);

	const char* source_name = obs_source_get_name(source);
	blog(LOG_INFO, "[Loaded global audio device]: '%s'", source_name);
	obs_source_enum_filters(source, LogFilter, (void*)(intptr_t)1);
	obs_monitoring_type monitoring_type =
		obs_source_get_monitoring_type(source);
	if(monitoring_type != OBS_MONITORING_TYPE_NONE) {
		const char* type =
			(monitoring_type == OBS_MONITORING_TYPE_MONITOR_ONLY)
			? "monitor only"
			: "monitor and output";

		blog(LOG_INFO, "    - monitoring: %s", type);
	}
};
static inline bool HasAudioDevices(const char* source_id)
{
	const char* output_id = source_id;
	obs_properties_t* props = obs_get_source_properties(output_id);
	if(!props) {
		return false;
	}

	size_t count = 0;
	obs_property_t* devices = obs_properties_get(props, "device_id");
	if(devices) {
		count = obs_property_list_item_count(devices);
	}
	obs_properties_destroy(props);

	return count != 0;
};