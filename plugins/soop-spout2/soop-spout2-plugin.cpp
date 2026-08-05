#include <obs-module.h>
#include <obs.hpp>

#include "./Spout2/SPOUTSDK/SpoutDirectX/SpoutDX/SpoutDX.h"
#include "./Spout2/SPOUTSDK/SpoutLibrary/SpoutLibrary.h"

constexpr int SENDER_NAME_LEN = 256;
constexpr int MAX_SENDER_COUNT = 30;

struct SOOPSpout2Source {

	obs_source_t* source = nullptr;

	bool transparent = true;

	unsigned int width = 800;
	unsigned int height = 600;
	std::string senderName;

	HANDLE sharedHandle = NULL;
	DWORD format = 0;
	gs_texture_t* texture = nullptr;
};

spoutDX* g_pSpoutReceiver = nullptr;

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("soop-spout2", "en-US")
MODULE_EXPORT const char* obs_module_description(void)
{
	return "soop spout2 plugin";
}

static const char* get_name_soop_spout2(void* type_data)
{
	return "Spout2";
}

static void clearSpout2SourceTextureInfo(SOOPSpout2Source* spoutSource)
{
	if (spoutSource) {
		spoutSource->format = 0;
		spoutSource->sharedHandle = NULL;

		if (spoutSource->texture) {
			obs_enter_graphics();
			gs_texture_destroy(spoutSource->texture);
			obs_leave_graphics();

			spoutSource->texture = nullptr;
		}
	}
}

static void getSpout2SenderList(obs_property_t* prop)
{
	obs_property_list_add_string(prop, obs_module_text("None"), "");

	if (!g_pSpoutReceiver)
		return;

	const int senderCount = g_pSpoutReceiver->GetSenderCount();
	if (senderCount == 0)
		return;

	for (int i = 0; i < senderCount; i++) {
		char szSenderName[SENDER_NAME_LEN] = { 0, };
		g_pSpoutReceiver->GetSender(i, szSenderName, SENDER_NAME_LEN);
		obs_property_list_add_string(prop, szSenderName, szSenderName);
	}
}

static bool makeSpoutSenderTexture(SOOPSpout2Source* spoutSource)
{
	if (!g_pSpoutReceiver)
		return false;

	DWORD  format;
	HANDLE sharedHandle;
	unsigned int width, height;

	std::string senderName = spoutSource->senderName;
	if (senderName.size() >= SENDER_NAME_LEN) {
		return false;
	}

	bool bResult = g_pSpoutReceiver->GetSenderInfo(senderName.c_str(), width, height, sharedHandle, format);

	if (bResult && (sharedHandle != spoutSource->sharedHandle)) 
	{
		obs_enter_graphics();

		gs_texture_destroy(spoutSource->texture);
		spoutSource->texture = nullptr;

		gs_texture_t* texture = gs_texture_open_shared((uint32_t)(uintptr_t)sharedHandle);
		if (texture) {
			spoutSource->texture = texture;
			spoutSource->width = width;
			spoutSource->height = height;
			spoutSource->sharedHandle = sharedHandle;
			spoutSource->format = format;
		}
		else {
			gs_texture_destroy(texture);
		}

		obs_leave_graphics();

		return true;
	}

	if(!bResult){
		clearSpout2SourceTextureInfo(spoutSource);
		return true;
	}

	return false;
}

static bool spout2_sender_modified(obs_properties_t* props,
								   obs_property_t* p,
								   obs_data_t* settings)
{
	struct SOOPSpout2Source* spoutSource = (SOOPSpout2Source*)obs_properties_get_param(props);
	if (!spoutSource)
		return false;

	spoutSource->senderName = obs_data_get_string(settings, "spout2_sender");

	return true;
}

static void* create_soop_spout2(obs_data_t* settings, obs_source_t* source)
{
	SOOPSpout2Source* spoutSource = new SOOPSpout2Source;
	spoutSource->source = source;

	obs_source_update(source, settings);

	return spoutSource;
}

static void destroy_soop_spout2(void* data)
{
	struct SOOPSpout2Source* spoutSource = static_cast<SOOPSpout2Source*>(data);
	if (spoutSource) {
		delete spoutSource;
		spoutSource = nullptr;
	}
}

static obs_properties_t* get_properties_soop_spout2(void* data)
{
	struct SOOPSpout2Source* spoutSource = static_cast<SOOPSpout2Source*>(data);
	if (!spoutSource)
		return nullptr;

	obs_properties_t* props = obs_properties_create();
	obs_properties_set_param(props, spoutSource, NULL);

	obs_property_t* p;

	p = obs_properties_add_list(props, "spout2_sender",
									   "Spout Sender",
									   OBS_COMBO_TYPE_LIST,
									   OBS_COMBO_FORMAT_STRING);

	getSpout2SenderList(p);

	obs_property_set_modified_callback(p, spout2_sender_modified);

	p = obs_properties_add_bool(props, "transparent",
								obs_module_text("TransparencyControl"));
	return props;
};

static void get_default_soop_spout2(obs_data_t* settings)
{
	obs_data_set_default_bool(settings, "transparent", true);
}

static void update_soop_spout2(void* data, obs_data_t* settings)
{
	struct SOOPSpout2Source* spoutSource = static_cast<SOOPSpout2Source*>(data);
	if (!spoutSource)
		return;

	spoutSource->senderName = obs_data_get_string(settings, "spout2_sender");
	spoutSource->transparent = obs_data_get_bool(settings, "transparent");
}

static uint32_t get_width_soop_spout2(void* data)
{
	return static_cast<SOOPSpout2Source*>(data)->width;
}

static uint32_t get_height_soop_spout2(void* data)
{
	return static_cast<SOOPSpout2Source*>(data)->height;
}

static void video_tick_soop_spout2(void* data, float seconds)
{
	struct SOOPSpout2Source* spoutSource = static_cast<SOOPSpout2Source*>(data);
	if (!spoutSource)
		return;

	makeSpoutSenderTexture(spoutSource);
}

static void video_render_soop_spout2(void* data, gs_effect_t* )
{
	struct SOOPSpout2Source* spoutSource = static_cast<SOOPSpout2Source*>(data);
	if (!spoutSource)
		return;

	uint32_t width = spoutSource->width;
	uint32_t height = spoutSource->height;
	gs_texture_t* const texture = spoutSource->texture;

	if (!texture)
		return;

	obs_base_effect base_effect = spoutSource->transparent ? 
								  OBS_EFFECT_DEFAULT : OBS_EFFECT_OPAQUE;
	gs_effect_t* effect = obs_get_base_effect(base_effect);

	gs_eparam_t* image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, texture);

	while (gs_effect_loop(effect, "Draw")) {
		gs_draw_sprite(texture, 0, width, height);
	}
}

void RegisterSOOPSpout2Source()
{
	struct obs_source_info info = {};
	info.id = "soop_spout2";
	info.get_name = get_name_soop_spout2;
	info.type = OBS_SOURCE_TYPE_INPUT;
	info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW;
	info.icon_type = OBS_ICON_TYPE_CUSTOM;
	info.create = create_soop_spout2;
	info.destroy = destroy_soop_spout2;
	info.get_properties = get_properties_soop_spout2;
	info.get_defaults = get_default_soop_spout2;
	info.update = update_soop_spout2;
	info.get_width = get_width_soop_spout2;
	info.get_height = get_height_soop_spout2;
	info.video_tick = video_tick_soop_spout2;
	info.video_render = video_render_soop_spout2;

	obs_register_source(&info);
};

bool obs_module_load(void)
{
	bool init = false;
	if (nullptr == g_pSpoutReceiver) {
		g_pSpoutReceiver = new spoutDX();
		init = g_pSpoutReceiver->OpenDirectX11();
		if (init) {
			g_pSpoutReceiver->SetMaxSenders(MAX_SENDER_COUNT);
			RegisterSOOPSpout2Source();

			return true;
		}
	}

	return false;
}

void obs_module_unload(void)
{
	if (g_pSpoutReceiver)
	{
		g_pSpoutReceiver->CloseDirectX11();
		delete g_pSpoutReceiver;
		g_pSpoutReceiver = nullptr;
	}
}