#include <float.h>
#include <graphics/graphics.h>
#include <graphics/image-file.h>
#include <limits.h>
#include <obs-module.h>
#include <util/base.h>
#include <util/dstr.h>
#include <util/platform.h>

#include <sstream>
#include <string>

#define SETTING_PRESET_TYPE_DEFAULT "preset_default"
#define SETTING_PRESET_TYPE_TWOWAY "preset_2"
#define SETTING_PRESET_TYPE_THREEWAY "preset_3"
#define SETTING_PRESET_TYPE_FOURWAY "preset_4"
#define SETTING_PRESET_TYPE_FIVEWAY "preset_5"
#define SETTING_PRESET_TYPE_SIXWAY "preset_6"
#define SETTING_PRESET_TYPE_HORIZONTALFLIP "preset_horizontal_flip"
#define SETTING_PRESET_TYPE_COLOREDSPLIT "preset_3_rgb"
#define SETTING_PRESET_TYPE_GRAYSPLIT "preset_3x2_color_gray"
#define SETTING_PRESET_TYPE_USER "preset_user"

#define PRESET "preset"
#define REPEAT_X "repeat_x"
#define REPEAT_Y "repeat_y"
#define CROP_LEFT_RATIO "crop_left_ratio"
#define CROP_RIGHT_RATIO "crop_right_ratio"
#define CROP_TOP_RATIO "crop_top_ratio"
#define CROP_BOTTOM_RATIO "crop_bottom_ratio"
#define FLIP_ODD_INDEX_HORIZONTALLY "flip_odd_index_horizontally"
#define COLORMAP "colormap"
#define COLORMAP_NONE "colormap_none"
#define COLORMAP_RGB "colormap_rgb"
#define COLORMAP_GRAY "colormap_color_gray"


static const char *effect_template_begin =
    "\
uniform float4x4 ViewProj;\
uniform texture2d image;\
\
uniform float elapsed_time;\
uniform float2 uv_offset;\
uniform float2 uv_scale;\
uniform float2 uv_pixel_interval;\
\
sampler_state textureSampler{\
	Filter = Linear;\
	AddressU = Border;\
	AddressV = Border;\
	BorderColor = 00000000;\
};\
\
struct VertData {\
	float4 pos : POSITION;\
	float2 uv : TEXCOORD0;\
};\
\
VertData mainTransform(VertData v_in)\
{\
	VertData vert_out;\
	vert_out.pos = mul(float4(v_in.pos.xyz, 1.0), ViewProj);\
	vert_out.uv = v_in.uv * uv_scale + uv_offset;\
	return vert_out;\
}\
\
";

static std::string build_shader(int repeat_x = 1, int repeat_y = 1,
                                double umin = 0.0, double umax = 1.0,
                                double vmin = 0.0, double vmax = 1.0,
                                int colormap = 0,
                                bool flip_odd_index_horizontally = false) {
    std::ostringstream shader_code{};
    std::ostringstream color_code{};
    double width = umax - umin;
    double height = vmax - vmin;
    if (width < 0.0) {
        width = 0.0;
    }
    if (height < 0.0) {
        height = 0.0;
    }

    color_code << "\n"
               << "float4 getColor(float4 color, int index)\n"
               << "{\n";

    if (colormap == 0) {
        ;
    } else if (colormap == 1) {
        color_code << "\n"
                   << "if (index % 3 == 0) {\n"
                   << "	color.g /= 2;\n"
                   << "	color.b /= 2;\n"
                   << "}\n"
                   << "else if (index % 3 == 1) {\n"
                   << "	color.r /= 2;\n"
                   << "	color.b /= 2;\n"
                   << "}\n"
                   << "else if (index % 3 == 2) {\n"
                   << "	color.r /= 2;\n"
                   << "	color.g /= 2;\n"
                   << "}\n";
    } else if (colormap == 2) {
        color_code << "\n"
                   << "if (index % 2 == 1) {\n"
                   << "	float gray = 0.2126 * color.r + 0.7152 * color.g + "
                      "0.0722 * color.b;\n"
                   << "	color.r = color.g = color.b = gray;\n"
                   << "}\n";
    }
    color_code << "return color;\n"
               << "}\n";

    shader_code << "\n"
                << "float4 mainImage(VertData v_in) : TARGET\n"
                << "{\n"
                << "    float2 uv_range = v_in.uv * float2(" << width << ", "
                << height << ");\n"
                << "    float2 uv_index = v_in.uv * float2(" << repeat_x << ", "
                << repeat_y << ");\n"
                << "    uv_index = floor(uv_index);\n"
                << "    int index = int(uv_index.x) + int(uv_index.y) * "
                << repeat_x << ";\n"
                << "	if(" << (flip_odd_index_horizontally ? "true" : "false")  << " && index % 2 != 0) {\n"
                << "		uv_range = float2(" << width << "- uv_range.x, uv_range.y);\n "  // flip
                << "}\n"
                << "    float2 uv_repeat = uv_range * float2(" << repeat_x
                << ", " << repeat_y << ");\n"
                << "    float2 uv_fractional = uv_repeat;\n"
                << "    uv_fractional.x = fmod(uv_fractional.x, " << width
                << ") + " << umin << ";\n"
                << "    uv_fractional.y = fmod(uv_fractional.y, " << height
                << ") + " << vmin << ";\n"
                << "\n"
                << "	float4 color = image.Sample(textureSampler, "
                   "uv_fractional);\n"
                << "	color = getColor(color, index);\n"
                << "    return color;\n"
                << "}\n";

    std::string str_shader_code = color_code.str() + shader_code.str();

    return str_shader_code;
}

static const char *effect_template_end =
    "\
technique Draw\
{\
	pass\
	{\
		vertex_shader = mainTransform(v_in);\
		pixel_shader = mainImage(v_in);\
	}\
}";

struct effect_param_data {
    struct dstr name;
    enum gs_shader_param_type type;
    gs_eparam_t *param;

    gs_image_file_t *image;

    union {
        long long i;
        double f;
    } value;
};

struct shader_filter_data {
    obs_source_t *context;
    gs_effect_t *effect;

    gs_eparam_t *param_uv_offset;
    gs_eparam_t *param_uv_scale;
    gs_eparam_t *param_uv_pixel_interval;
    gs_eparam_t *param_elapsed_time;

    double crop_left_ratio;
    double crop_right_ratio;
    double crop_top_ratio;
    double crop_bottom_ratio;

    int repeat_x = 1;
    int repeat_y = 1;
    int colormap = 0;

    int total_width;
    int total_height;

    bool flip_odd_index_horizontally = false;

    struct vec2 uv_offset;
    struct vec2 uv_scale;
    struct vec2 uv_pixel_interval;
    float elapsed_time;

    DARRAY(struct effect_param_data) stored_param_list;
};

static void shader_filter_reload_effect(struct shader_filter_data *filter) {
    // First, clean up the old effect and all references to it.
    size_t param_count = filter->stored_param_list.num;
    for (size_t param_index = 0; param_index < param_count; param_index++) {
        struct effect_param_data *param =
            (filter->stored_param_list.array + param_index);
        if (param->image != NULL) {
            obs_enter_graphics();
            gs_image_file_free(param->image);
            obs_leave_graphics();

            bfree(param->image);
            param->image = NULL;
        }
    }

    da_free(filter->stored_param_list);

    filter->param_elapsed_time = NULL;
    filter->param_uv_offset = NULL;
    filter->param_uv_pixel_interval = NULL;
    filter->param_uv_scale = NULL;

    if (filter->effect != NULL) {
        obs_enter_graphics();
        gs_effect_destroy(filter->effect);
        filter->effect = NULL;
        obs_leave_graphics();
    }

    // Build shader code
    const char *shader_text = NULL;
    // TODO: Ensure 0~1 ranges
    double umin = filter->crop_left_ratio;
    double umax = 1.0 - filter->crop_right_ratio;
    double vmin = filter->crop_top_ratio;
    double vmax = 1.0 - filter->crop_bottom_ratio;
    std::string str_shader =
        build_shader(filter->repeat_x, filter->repeat_y, umin, umax, vmin, vmax,
                     filter->colormap,
		     filter->flip_odd_index_horizontally);
    shader_text = bstrdup(str_shader.c_str());

    size_t effect_header_length = strlen(effect_template_begin);
    size_t effect_body_length = strlen(shader_text);
    size_t effect_footer_length = strlen(effect_template_end);
    size_t effect_buffer_total_size =
        effect_header_length + effect_body_length + effect_footer_length;

    struct dstr effect_text = {0};
    dstr_cat(&effect_text, effect_template_begin);
    dstr_cat(&effect_text, shader_text);
    dstr_cat(&effect_text, effect_template_end);

    // Create the effect.
    char *errors = NULL;

    obs_enter_graphics();
    filter->effect = gs_effect_create(effect_text.array, NULL, &errors);
    obs_leave_graphics();

    dstr_free(&effect_text);

    if (filter->effect == NULL) {
        blog(
            LOG_WARNING,
            "[soop-shaderfilter] Unable to create effect. Errors returned from "
            "parser:\n%s",
            (errors == NULL || strlen(errors) == 0 ? "(None)" : errors));
    }

    // Store references to the new effect's parameters.
    da_init(filter->stored_param_list);
    size_t effect_count = gs_effect_get_num_params(filter->effect);
    for (size_t effect_index = 0; effect_index < effect_count; effect_index++) {
        gs_eparam_t *param =
            gs_effect_get_param_by_idx(filter->effect, effect_index);
        struct gs_effect_param_info info;
        gs_effect_get_param_info(param, &info);

        if (strcmp(info.name, "uv_offset") == 0) {
            filter->param_uv_offset = param;
        } else if (strcmp(info.name, "uv_scale") == 0) {
            filter->param_uv_scale = param;
        } else if (strcmp(info.name, "uv_pixel_interval") == 0) {
            filter->param_uv_pixel_interval = param;
        } else if (strcmp(info.name, "elapsed_time") == 0) {
            filter->param_elapsed_time = param;
        } else if (strcmp(info.name, "ViewProj") == 0 ||
                   strcmp(info.name, "image") == 0) {
            // Nothing.
        } else {
            struct effect_param_data *cached_data =
                (struct effect_param_data *)da_push_back_new(
                    filter->stored_param_list);
            dstr_copy(&cached_data->name, info.name);
            cached_data->type = info.type;
            cached_data->param = param;
        }
    }
}

static const char *shader_filter_get_name(void *unused) {
    UNUSED_PARAMETER(unused);
    return obs_module_text("soop_shader_filter");
}

static void *shader_filter_create(obs_data_t *settings, obs_source_t *source) {
    UNUSED_PARAMETER(source);

    struct shader_filter_data *filter =
        (struct shader_filter_data *)bzalloc(sizeof(struct shader_filter_data));
    filter->context = source;

    obs_source_update(source, settings);

    return filter;
}

static void shader_filter_destroy(void *data) {
    struct shader_filter_data *filter = (struct shader_filter_data *)data;

    bfree(filter);
}

static const char *shader_filter_texture_file_filter =
    "Textures (*.bmp *.tga *.png *.jpeg *.jpg *.gif);;";

static void reset_properties(obs_data_t *settings) {
    obs_data_set_int(settings, REPEAT_X, 1);
    obs_data_set_int(settings, REPEAT_Y, 1);
    obs_data_set_int(settings, CROP_LEFT_RATIO, 0);
    obs_data_set_int(settings, CROP_RIGHT_RATIO, 0);
    obs_data_set_int(settings, CROP_TOP_RATIO, 0);
    obs_data_set_int(settings, CROP_BOTTOM_RATIO, 0);
    obs_data_set_bool(settings, FLIP_ODD_INDEX_HORIZONTALLY, false);
    obs_data_set_string(settings, COLORMAP, COLORMAP_NONE);
    obs_data_set_default_string(settings, PRESET, SETTING_PRESET_TYPE_DEFAULT);
}

static void shader_filter_defaults(obs_data_t *settings)
{
    obs_data_set_default_int(settings, REPEAT_X, 1);
    obs_data_set_default_int(settings, REPEAT_Y, 1);
    obs_data_set_default_int(settings, CROP_LEFT_RATIO, 0);
    obs_data_set_default_int(settings, CROP_RIGHT_RATIO, 0);
    obs_data_set_default_int(settings, CROP_TOP_RATIO, 0);
    obs_data_set_default_int(settings, CROP_BOTTOM_RATIO, 0);
    obs_data_set_default_bool(settings, FLIP_ODD_INDEX_HORIZONTALLY, false);
    obs_data_set_default_string(settings, COLORMAP, COLORMAP_NONE);
    obs_data_set_default_string(settings, PRESET, SETTING_PRESET_TYPE_DEFAULT);
}

static bool preset_selected_callback(obs_properties_t *props,
                                     obs_property_t *property,
                                     obs_data_t *settings) {
    const char *preset_name = obs_data_get_string(settings, PRESET);

    if (strcmp(preset_name, SETTING_PRESET_TYPE_DEFAULT) == 0) {
        reset_properties(settings);
        return true;
    } else if (strcmp(preset_name, SETTING_PRESET_TYPE_TWOWAY) == 0) {
        obs_data_set_int(settings, REPEAT_X, 2);
        obs_data_set_int(settings, REPEAT_Y, 1);
        obs_data_set_int(settings, CROP_LEFT_RATIO, 25);
        obs_data_set_int(settings, CROP_RIGHT_RATIO, 25);
        obs_data_set_int(settings, CROP_TOP_RATIO, 0);
        obs_data_set_int(settings, CROP_BOTTOM_RATIO, 0);
        obs_data_set_bool(settings, FLIP_ODD_INDEX_HORIZONTALLY, false);
        obs_data_set_string(settings, COLORMAP, COLORMAP_NONE);
    } else if (strcmp(preset_name, SETTING_PRESET_TYPE_THREEWAY) == 0) {
        obs_data_set_int(settings, REPEAT_X, 3);
        obs_data_set_int(settings, REPEAT_Y, 1);
        obs_data_set_int(settings, CROP_LEFT_RATIO, 33);
        obs_data_set_int(settings, CROP_RIGHT_RATIO, 33);
        obs_data_set_int(settings, CROP_TOP_RATIO, 0);
        obs_data_set_int(settings, CROP_BOTTOM_RATIO, 0);
        obs_data_set_bool(settings, FLIP_ODD_INDEX_HORIZONTALLY, false);
        obs_data_set_string(settings, COLORMAP, COLORMAP_NONE);
    } else if (strcmp(preset_name, SETTING_PRESET_TYPE_FOURWAY) == 0) {
        obs_data_set_int(settings, REPEAT_X, 4);
        obs_data_set_int(settings, REPEAT_Y, 1);
        obs_data_set_int(settings, CROP_LEFT_RATIO, 37);
        obs_data_set_int(settings, CROP_RIGHT_RATIO, 37);
        obs_data_set_int(settings, CROP_TOP_RATIO, 0);
        obs_data_set_int(settings, CROP_BOTTOM_RATIO, 0);
        obs_data_set_bool(settings, FLIP_ODD_INDEX_HORIZONTALLY, false);
        obs_data_set_string(settings, COLORMAP, COLORMAP_NONE);
    } else if (strcmp(preset_name, SETTING_PRESET_TYPE_FIVEWAY) == 0) {
        obs_data_set_int(settings, REPEAT_X, 5);
        obs_data_set_int(settings, REPEAT_Y, 1);
        obs_data_set_int(settings, CROP_LEFT_RATIO, 40);
        obs_data_set_int(settings, CROP_RIGHT_RATIO, 40);
        obs_data_set_int(settings, CROP_TOP_RATIO, 0);
        obs_data_set_int(settings, CROP_BOTTOM_RATIO, 0);
        obs_data_set_bool(settings, FLIP_ODD_INDEX_HORIZONTALLY, false);
        obs_data_set_string(settings, COLORMAP, COLORMAP_NONE);
    } else if (strcmp(preset_name, SETTING_PRESET_TYPE_SIXWAY) == 0) {
        obs_data_set_int(settings, REPEAT_X, 3);
        obs_data_set_int(settings, REPEAT_Y, 2);
        obs_data_set_int(settings, CROP_LEFT_RATIO, 33);
        obs_data_set_int(settings, CROP_RIGHT_RATIO, 33);
        obs_data_set_int(settings, CROP_TOP_RATIO, 25);
        obs_data_set_int(settings, CROP_BOTTOM_RATIO, 25);
        obs_data_set_bool(settings, FLIP_ODD_INDEX_HORIZONTALLY, false);
        obs_data_set_string(settings, COLORMAP, COLORMAP_NONE);
    } else if (strcmp(preset_name, SETTING_PRESET_TYPE_HORIZONTALFLIP) == 0) {
        obs_data_set_int(settings, REPEAT_X, 2);
        obs_data_set_int(settings, REPEAT_Y, 1);
        obs_data_set_int(settings, CROP_LEFT_RATIO, 25);
        obs_data_set_int(settings, CROP_RIGHT_RATIO, 25);
        obs_data_set_int(settings, CROP_TOP_RATIO, 0);
        obs_data_set_int(settings, CROP_BOTTOM_RATIO, 0);
        obs_data_set_bool(settings, FLIP_ODD_INDEX_HORIZONTALLY, true);
        obs_data_set_string(settings, COLORMAP, COLORMAP_NONE);
    } else if (strcmp(preset_name, SETTING_PRESET_TYPE_COLOREDSPLIT) == 0) {
        obs_data_set_int(settings, REPEAT_X, 3);
        obs_data_set_int(settings, REPEAT_Y, 1);
        obs_data_set_int(settings, CROP_LEFT_RATIO, 33);
        obs_data_set_int(settings, CROP_RIGHT_RATIO, 33);
        obs_data_set_int(settings, CROP_TOP_RATIO, 0);
        obs_data_set_int(settings, CROP_BOTTOM_RATIO, 0);
        obs_data_set_bool(settings, FLIP_ODD_INDEX_HORIZONTALLY, false);
        obs_data_set_string(settings, COLORMAP, COLORMAP_RGB);
    } else if (strcmp(preset_name, SETTING_PRESET_TYPE_GRAYSPLIT) == 0) {
        reset_properties(settings);
        obs_data_set_int(settings, CROP_LEFT_RATIO, 33);
        obs_data_set_int(settings, CROP_RIGHT_RATIO, 33);
        obs_data_set_int(settings, REPEAT_X, 3);
        obs_data_set_int(settings, REPEAT_Y, 2);
        obs_data_set_string(settings, COLORMAP, COLORMAP_GRAY);
        return true;
    }
    return false;
}

static bool settings_updated_callback(obs_properties_t *props,
                                      obs_property_t *property,
                                      obs_data_t *settings) {
    const char *preset_name = obs_data_get_string(settings, PRESET);
    obs_data_set_string(settings, PRESET, SETTING_PRESET_TYPE_USER);

    return true;
}

static obs_properties_t *shader_filter_properties(void *data) {
    // TODO: locale화 obs_module_text("ShaderFilter.CropLeft")
    struct shader_filter_data *filter = (struct shader_filter_data *)data;

    obs_properties_t *props = obs_properties_create();

    obs_properties_set_param(props, filter, NULL);

    obs_property_t *preset =
        obs_properties_add_list(props, PRESET, "STR.Preset",
                                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

    obs_property_set_modified_callback(preset, preset_selected_callback);
    obs_property_list_add_string(preset, "STR.PresetDefault", SETTING_PRESET_TYPE_DEFAULT);
    obs_property_list_add_string(preset, "STR.Preset2", SETTING_PRESET_TYPE_TWOWAY);
    obs_property_list_add_string(preset, "STR.Preset3", SETTING_PRESET_TYPE_THREEWAY);
    obs_property_list_add_string(preset, "STR.Preset4", SETTING_PRESET_TYPE_FOURWAY);
    obs_property_list_add_string(preset, "STR.Preset5", SETTING_PRESET_TYPE_FIVEWAY);
    obs_property_list_add_string(preset, "STR.Preset6", SETTING_PRESET_TYPE_SIXWAY);
    obs_property_list_add_string(preset, "STR.PresetFlip", SETTING_PRESET_TYPE_HORIZONTALFLIP);
    obs_property_list_add_string(preset, "STR.Preset3RGB", SETTING_PRESET_TYPE_COLOREDSPLIT);
    obs_property_list_add_string(preset, "STR.Preset3x2ColorGray", SETTING_PRESET_TYPE_GRAYSPLIT);
    obs_property_list_add_string(preset, "STR.PresetUser", SETTING_PRESET_TYPE_USER);

    obs_properties_add_int_slider(props, CROP_LEFT_RATIO, "STR.CropLeft%", 0,
                                  100, 1);
    obs_properties_add_int_slider(props, CROP_RIGHT_RATIO, "STR.CropRight%",
                                  0, 100, 1);
    obs_properties_add_int_slider(props, CROP_TOP_RATIO, "STR.CropTop%", 0,
                                  100, 1);
    obs_properties_add_int_slider(props, CROP_BOTTOM_RATIO, "STR.CropBottom%",
                                  0, 100, 1);
    obs_properties_add_int_slider(props, REPEAT_X, "STR.repeat_x", 1, 5, 1);
    obs_properties_add_int_slider(props, REPEAT_Y, "STR.repeat_y", 1, 5, 1);

    obs_property_t *colormap =
        obs_properties_add_list(props, COLORMAP, "STR.ColorMap",
                                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(colormap, "STR.ColorMapNone", COLORMAP_NONE);
    obs_property_list_add_string(colormap, "STR.ColorMapRGB", COLORMAP_RGB);
    obs_property_list_add_string(colormap, "STR.ColormapColorGray", COLORMAP_GRAY);

    // flip
    obs_properties_add_bool(props, FLIP_ODD_INDEX_HORIZONTALLY, "FlipOddIndexHorizontally");
    const char *properties[] = {CROP_LEFT_RATIO, CROP_RIGHT_RATIO,
                                CROP_TOP_RATIO,  CROP_BOTTOM_RATIO,
                                REPEAT_X,        REPEAT_Y,
                                COLORMAP,        FLIP_ODD_INDEX_HORIZONTALLY};

    for (size_t i = 0; i < sizeof(properties) / sizeof(properties[0]); i++) {
        obs_property_t *prop = obs_properties_get(props, properties[i]);
        obs_property_set_modified_callback(prop, settings_updated_callback);
    }

    return props;
}

static void shader_filter_update(void *data, obs_data_t *settings) {
    struct shader_filter_data *filter = (struct shader_filter_data *)data;

    // Get expansions. Will be used in the video_tick() callback.
    filter->crop_left_ratio =
        obs_data_get_double(settings, CROP_LEFT_RATIO) / 100.;
    filter->crop_right_ratio =
        obs_data_get_double(settings, CROP_RIGHT_RATIO) / 100.;
    filter->crop_top_ratio =
        obs_data_get_double(settings, CROP_TOP_RATIO) / 100.;
    filter->crop_bottom_ratio =
        obs_data_get_double(settings, CROP_BOTTOM_RATIO) / 100.;
    filter->repeat_x = (int)obs_data_get_int(settings, REPEAT_X);
    filter->repeat_y = (int)obs_data_get_int(settings, REPEAT_Y);
    std::string str_colormap = obs_data_get_string(settings, COLORMAP);
    filter->flip_odd_index_horizontally = obs_data_get_bool(settings, FLIP_ODD_INDEX_HORIZONTALLY);

    if (str_colormap == COLORMAP_RGB) {
        filter->colormap = 1;
    } else if (str_colormap == COLORMAP_GRAY) {
        filter->colormap = 2;
    } else {
        filter->colormap = 0;
    }

    obs_source_t *target = obs_filter_get_target(filter->context);
    int base_width = obs_source_get_base_width(target);
    int base_height = obs_source_get_base_height(target);
    filter->total_width = base_width;
    filter->total_height = base_height;
    filter->uv_scale.x = 1.0;
    filter->uv_scale.y = 1.0;
    filter->uv_offset.x = 0.0;
    filter->uv_offset.y = 0.0;

    shader_filter_reload_effect(filter);

    size_t param_count = filter->stored_param_list.num;
    for (size_t param_index = 0; param_index < param_count; param_index++) {
        struct effect_param_data *param =
            (filter->stored_param_list.array + param_index);
        const char *param_name = param->name.array;

        switch (param->type) {
            case GS_SHADER_PARAM_BOOL:
                param->value.i = obs_data_get_bool(settings, param_name);
                break;
            case GS_SHADER_PARAM_FLOAT:
                param->value.f = obs_data_get_double(settings, param_name);
                break;
            case GS_SHADER_PARAM_INT:
                param->value.i = obs_data_get_int(settings, param_name);
                break;
            case GS_SHADER_PARAM_VEC4:  // Assumed to be a color.

                // Hack to ensure we have a default...
                obs_data_set_default_int(settings, param_name, 0xff000000);

                param->value.i = obs_data_get_int(settings, param_name);
                break;
            case GS_SHADER_PARAM_TEXTURE:
                if (param->image == NULL) {
                    param->image =
                        (gs_image_file_t *)bzalloc(sizeof(gs_image_file_t));
                } else {
                    obs_enter_graphics();
                    gs_image_file_free(param->image);
                    obs_leave_graphics();
                }

                gs_image_file_init(param->image,
                                   obs_data_get_string(settings, param_name));

                obs_enter_graphics();
                gs_image_file_init_texture(param->image);
                obs_leave_graphics();
                break;
        }
    }
}

static void shader_filter_tick(void *data, float seconds) {
    struct shader_filter_data *filter = (struct shader_filter_data *)data;

    obs_source_t *target = obs_filter_get_target(filter->context);

    int base_width = obs_source_get_base_width(target);
    int base_height = obs_source_get_base_height(target);

    filter->uv_pixel_interval.x = 1.0f / base_width;
    filter->uv_pixel_interval.y = 1.0f / base_height;

    filter->elapsed_time += seconds;
}

static void shader_filter_render(void *data, gs_effect_t *effect) {
    UNUSED_PARAMETER(effect);

    struct shader_filter_data *filter = (struct shader_filter_data *)data;

    if (filter->effect != NULL) {
        if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
                                             OBS_NO_DIRECT_RENDERING)) {
            return;
        }

	filter->total_width = obs_source_get_base_width(obs_filter_get_target(filter->context));
        filter->total_height = obs_source_get_base_height(obs_filter_get_target(filter->context));

        if (filter->param_uv_scale != NULL) {
            gs_effect_set_vec2(filter->param_uv_scale, &filter->uv_scale);
        }
        if (filter->param_uv_offset != NULL) {
            gs_effect_set_vec2(filter->param_uv_offset, &filter->uv_offset);
        }
        if (filter->param_uv_pixel_interval != NULL) {
            gs_effect_set_vec2(filter->param_uv_pixel_interval,
                               &filter->uv_pixel_interval);
        }
        if (filter->param_elapsed_time != NULL) {
            gs_effect_set_float(filter->param_elapsed_time,
                                filter->elapsed_time);
        }

        size_t param_count = filter->stored_param_list.num;
        for (size_t param_index = 0; param_index < param_count; param_index++) {
            struct effect_param_data *param =
                (filter->stored_param_list.array + param_index);
            struct vec4 color;

            switch (param->type) {
                case GS_SHADER_PARAM_BOOL:
                    gs_effect_set_bool(param->param, param->value.i);
                    break;
                case GS_SHADER_PARAM_FLOAT:
                    gs_effect_set_float(param->param, (float)param->value.f);
                    break;
                case GS_SHADER_PARAM_INT:
                    gs_effect_set_int(param->param, (int)param->value.i);
                    break;
                case GS_SHADER_PARAM_VEC4:
                    vec4_from_rgba(&color, (unsigned int)param->value.i);
                    gs_effect_set_vec4(param->param, &color);
                    break;
                case GS_SHADER_PARAM_TEXTURE:
                    gs_effect_set_texture(
                        param->param,
                        (param->image ? param->image->texture : NULL));
                    break;
            }
        }

        obs_source_process_filter_end(filter->context, filter->effect,
                                      filter->total_width,
                                      filter->total_height);
    }
}

static uint32_t shader_filter_getwidth(void *data) {
    struct shader_filter_data *filter = (struct shader_filter_data *)data;

    return filter->total_width;
}

static uint32_t shader_filter_getheight(void *data) {
    struct shader_filter_data *filter = (struct shader_filter_data *)data;

    return filter->total_height;
}
