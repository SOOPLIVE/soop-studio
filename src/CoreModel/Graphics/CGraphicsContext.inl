#pragma once

#define OUTLINE_COLOR 0xFFD0D0D0
#define LINE_LENGTH 0.1f

// Rec. ITU-R BT.1848-1 / EBU R 95
#define ACTION_SAFE_PERCENT 0.035f       // 3.5%
#define GRAPHICS_SAFE_PERCENT 0.05f      // 5.0%
#define FOURBYTHREE_SAFE_PERCENT 0.1625f // 16.25%


inline void AFGraphicsContext::InitSafeAreas(gs_vertbuffer_t** actionSafeMargin,
                                            gs_vertbuffer_t** graphicsSafeMargin,
                                            gs_vertbuffer_t** fourByThreeSafeMargin,
                                            gs_vertbuffer_t** leftLine,
                                            gs_vertbuffer_t** topLine,
                                            gs_vertbuffer_t** rightLine)
{
	obs_enter_graphics();

	// All essential action should be placed inside this area
	gs_render_start(true);
	gs_vertex2f(ACTION_SAFE_PERCENT, ACTION_SAFE_PERCENT);
	gs_vertex2f(ACTION_SAFE_PERCENT, 1 - ACTION_SAFE_PERCENT);
	gs_vertex2f(1 - ACTION_SAFE_PERCENT, 1 - ACTION_SAFE_PERCENT);
	gs_vertex2f(1 - ACTION_SAFE_PERCENT, ACTION_SAFE_PERCENT);
	gs_vertex2f(ACTION_SAFE_PERCENT, ACTION_SAFE_PERCENT);
	*actionSafeMargin = gs_render_save();

	// All graphics should be placed inside this area
	gs_render_start(true);
	gs_vertex2f(GRAPHICS_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
	gs_vertex2f(GRAPHICS_SAFE_PERCENT, 1 - GRAPHICS_SAFE_PERCENT);
	gs_vertex2f(1 - GRAPHICS_SAFE_PERCENT, 1 - GRAPHICS_SAFE_PERCENT);
	gs_vertex2f(1 - GRAPHICS_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
	gs_vertex2f(GRAPHICS_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
	*graphicsSafeMargin = gs_render_save();

	// 4:3 safe area for widescreen
	gs_render_start(true);
	gs_vertex2f(FOURBYTHREE_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
	gs_vertex2f(1 - FOURBYTHREE_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
	gs_vertex2f(1 - FOURBYTHREE_SAFE_PERCENT, 1 - GRAPHICS_SAFE_PERCENT);
	gs_vertex2f(FOURBYTHREE_SAFE_PERCENT, 1 - GRAPHICS_SAFE_PERCENT);
	gs_vertex2f(FOURBYTHREE_SAFE_PERCENT, GRAPHICS_SAFE_PERCENT);
	*fourByThreeSafeMargin = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.0f, 0.5f);
	gs_vertex2f(LINE_LENGTH, 0.5f);
	*leftLine = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(0.5f, 0.0f);
	gs_vertex2f(0.5f, LINE_LENGTH);
	*topLine = gs_render_save();

	gs_render_start(true);
	gs_vertex2f(1.0f, 0.5f);
	gs_vertex2f(1 - LINE_LENGTH, 0.5f);
	*rightLine = gs_render_save();

	obs_leave_graphics();
};

#include <graphics/vec4.h>
#include <graphics/matrix4.h>

inline void AFGraphicsContext::RenderSafeAreas(gs_vertbuffer_t *vb, int cx, int cy)
{
    if (!vb)
        return;

    matrix4 transform;
    matrix4_identity(&transform);
    transform.x.x = cx;
    transform.y.y = cy;

    gs_load_vertexbuffer(vb);

    gs_matrix_push();
    gs_matrix_mul(&transform);

    gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_eparam_t *color = gs_effect_get_param_by_name(solid, "color");

    gs_effect_set_color(color, OUTLINE_COLOR);
    while (gs_effect_loop(solid, "Solid"))
        gs_draw(GS_LINESTRIP, 0, 0);

    gs_matrix_pop();
}
