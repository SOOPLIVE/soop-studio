#include <obs-module.h>

#include "shader-filter.cpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("soop-filters", "en-US")

MODULE_EXPORT const char *obs_module_description(void) {
    return "Soop filters";
}

void register_shader_filter() {
    struct obs_source_info shader_filter = {};
    shader_filter.id = "soop_shader_filter";
    shader_filter.type = OBS_SOURCE_TYPE_FILTER;
    shader_filter.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB;
    shader_filter.create = shader_filter_create;
    shader_filter.destroy = shader_filter_destroy;
    shader_filter.update = shader_filter_update;
    shader_filter.video_tick = shader_filter_tick;
    shader_filter.get_name = shader_filter_get_name;
    shader_filter.get_defaults = shader_filter_defaults;
    shader_filter.get_width = shader_filter_getwidth;
    shader_filter.get_height = shader_filter_getheight;
    shader_filter.video_render = shader_filter_render;
    shader_filter.get_properties = shader_filter_properties;
    obs_register_source(&shader_filter);
};

bool obs_module_load(void) {
    register_shader_filter();
    return true;
}
