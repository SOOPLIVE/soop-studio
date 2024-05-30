#include "CDrawLinePreview.h"


#include <cmath>
#include <string>


#include "CMouseStaterPreview.h"
#include "CModelPreview.h"
#include "UIComponent/CBasicPreview.h"


#define SPACER_LABEL_MARGIN 6.0f


#include "Common/MathMiscUtils.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Scene/CScene.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"


#include "platform/platform.hpp"


AFDrawLinePreview::~AFDrawLinePreview()
{
	obs_enter_graphics();

	if (m_texOverflow)
		gs_texture_destroy(m_texOverflow);
	if (m_vbRectFill)
		gs_vertexbuffer_destroy(m_vbRectFill);
    if (m_vbCircleFill)
        gs_vertexbuffer_destroy(m_vbCircleFill);

	obs_leave_graphics();
}

void AFDrawLinePreview::DrawRectOBSGFX(float thickness, vec2 scale)
{
	gs_render_start(true);

	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f + (thickness / scale.x), 0.0f);
	gs_vertex2f(0.0f, 1.0f);
	gs_vertex2f(0.0f + (thickness / scale.x), 1.0f);
	gs_vertex2f(0.0f, 1.0f - (thickness / scale.y));
	gs_vertex2f(1.0f, 1.0f);
	gs_vertex2f(1.0f, 1.0f - (thickness / scale.y));
	gs_vertex2f(1.0f - (thickness / scale.x), 1.0f);
	gs_vertex2f(1.0f, 0.0f);
	gs_vertex2f(1.0f - (thickness / scale.x), 0.0f);
	gs_vertex2f(1.0f, 0.0f + (thickness / scale.y));
	gs_vertex2f(0.0f, 0.0f);
	gs_vertex2f(0.0f, 0.0f + (thickness / scale.y));

	gs_vertbuffer_t* rect = gs_render_save();

	gs_load_vertexbuffer(rect);
	gs_draw(GS_TRISTRIP, 0, 0);
	gs_vertexbuffer_destroy(rect);
}

void AFDrawLinePreview::DrawLineOBSGFX(float x1, float y1, float x2, float y2,
											float thickness, vec2 scale)
{
	float ySide = (y1 == y2) ? (y1 < 0.5f ? 1.0f : -1.0f) : 0.0f;
	float xSide = (x1 == x2) ? (x1 < 0.5f ? 1.0f : -1.0f) : 0.0f;

	gs_render_start(true);

	gs_vertex2f(x1, y1);
	gs_vertex2f(x1 + (xSide * (thickness / scale.x)),
		y1 + (ySide * (thickness / scale.y)));
	gs_vertex2f(x2 + (xSide * (thickness / scale.x)),
		y2 + (ySide * (thickness / scale.y)));
	gs_vertex2f(x2, y2);
	gs_vertex2f(x1, y1);

	gs_vertbuffer_t* line = gs_render_save();

	gs_load_vertexbuffer(line);
	gs_draw(GS_TRISTRIP, 0, 0);
	gs_vertexbuffer_destroy(line);
}

void AFDrawLinePreview::DrawSquareAtPosOBSGFX(float x, float y, float pixelRatio)
{
	struct vec3 pos;
	vec3_set(&pos, x, y, 0.0f);

	struct matrix4 matrix;
	gs_matrix_get(&matrix);
	vec3_transform(&pos, &pos, &matrix);

	gs_matrix_push();
	gs_matrix_identity();
	gs_matrix_translate(&pos);

	gs_matrix_translate3f(-HANDLE_RADIUS * pixelRatio,
		-HANDLE_RADIUS * pixelRatio, 0.0f);
	gs_matrix_scale3f(HANDLE_RADIUS * pixelRatio * 2,
		HANDLE_RADIUS * pixelRatio * 2, 1.0f);
	gs_draw(GS_TRISTRIP, 0, 0);

	gs_matrix_pop();
}

void AFDrawLinePreview::SetUnsafeAccessContext(AFGraphicsContext* pGraphicsContext, 
											   AFSceneContext* pSceneContext)
{
	if (m_pInitedContextGraphics == nullptr)
		m_pInitedContextGraphics = pGraphicsContext;

	if (m_pInitedContextScene == nullptr)
		m_pInitedContextScene = pSceneContext;
}

void AFDrawLinePreview::DrawOverflow()
{
	if (m_bOverflowHidden)
		return;

    GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "DrawOverflow");
    
	if (!m_texOverflow)
    {
        std::string absPath;
        GetDataFilePath("assets/preview/overflow.png", absPath);
        
		m_texOverflow = gs_texture_create_from_file(absPath.data());
	}

	OBSScene scene = nullptr;
	
	if (m_pInitedContextScene != nullptr)
		scene = m_pInitedContextScene->GetCurrOBSScene();

	if (scene)
	{
		float previewScale = 1.0f;

		if (m_pInitedContextGraphics != nullptr)
			previewScale = m_pInitedContextGraphics->GetMainPreviewScale();

		gs_matrix_push();
		gs_matrix_scale3f(previewScale, previewScale, 1.0f);
		obs_scene_enum_items(scene, _DrawSelectedOverflow, this);
		gs_matrix_pop();
	}

	gs_load_vertexbuffer(nullptr);
    
    GS_DEBUG_MARKER_END();
}

void AFDrawLinePreview::DrawSceneEditing(const vec2& startPos, const vec2& mousePos, 
                                         bool selectionBox, float dpiValue/* = 1.f*/)
{
    m_fDpiValueLastDraw = dpiValue;
    
    GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "DrawSceneEditing");
    
	gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_technique_t* tech = gs_effect_get_technique(solid, "Solid");

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);

	float previewScale = 1.0f;
	
	if (m_pInitedContextGraphics != nullptr)
		previewScale = m_pInitedContextGraphics->GetMainPreviewScale();


	OBSScene scene = nullptr;

	if (m_pInitedContextScene != nullptr)
		scene = m_pInitedContextScene->GetCurrOBSScene();

	if (scene)
	{
		gs_matrix_push();
		gs_matrix_scale3f(previewScale, previewScale, 1.0f);
		obs_scene_enum_items(scene, _DrawSelectedItem, this);
		gs_matrix_pop();
	}

	if (selectionBox)
	{
		if (!m_vbRectFill)
		{
			gs_render_start(true);

			gs_vertex2f(0.0f, 0.0f);
			gs_vertex2f(1.0f, 0.0f);
			gs_vertex2f(0.0f, 1.0f);
			gs_vertex2f(1.0f, 1.0f);

			m_vbRectFill = gs_render_save();
		}

		_DrawSelectionBox(startPos.x * previewScale,
						  startPos.y * previewScale,
						  mousePos.x * previewScale,
						  mousePos.y * previewScale, m_vbRectFill, dpiValue);
	}

	gs_load_vertexbuffer(nullptr);

	gs_technique_end_pass(tech);
	gs_technique_end(tech);
    
    GS_DEBUG_MARKER_END();
}

void AFDrawLinePreview::DrawSpacingHelpers(float dpiValue/* = 1.f*/)
{
    m_fDpiValueLastDraw = dpiValue;
    
	if (m_pInitedContextGraphics == nullptr)
		return;


	vec2 s;
	SceneFindBoxData data(s, s);

	OBSScene scene = nullptr;

	if (m_pInitedContextScene != nullptr)
		scene = m_pInitedContextScene->GetCurrOBSScene();

	obs_scene_enum_items(scene, AFModelPreview::FindSelected, &data);

	if (data.sceneItems.size() != 1)
		return;

	OBSSceneItem item = data.sceneItems[0];
	if (!item)
		return;

	//if (obs_sceneitem_locked(item))
	//	return;

	vec2 itemSize = AFModelPreview::GetItemSize(item);
	if (itemSize.x == 0.0f || itemSize.y == 0.0f)
		return;

	//obs_sceneitem_t* parentGroup = obs_sceneitem_get_group(scene, item);
	//if (parentGroup && obs_sceneitem_locked(parentGroup))
	//	return;

	matrix4 boxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);

	obs_transform_info oti;
	obs_sceneitem_get_info(item, &oti);

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	vec3 size;
	vec3_set(&size, ovi.base_width, ovi.base_height, 1.0f);

	// Init box transform side locations
	vec3 left, right, top, bottom;

	vec3_set(&left, 0.0f, 0.5f, 1.0f);
	vec3_set(&right, 1.0f, 0.5f, 1.0f);
	vec3_set(&top, 0.5f, 0.0f, 1.0f);
	vec3_set(&bottom, 0.5f, 1.0f, 1.0f);

	// Decide which side to use with box transform, based on rotation
	// Seems hacky, probably a better way to do it
	float rot = oti.rot;

	//if (parentGroup) {
	//	obs_transform_info groupOti;
	//	obs_sceneitem_get_info(parentGroup, &groupOti);

	//	//Correct the scene item rotation angle
	//	rot = oti.rot + groupOti.rot;

	//	// Correct the scene item box transform
	//	// Based on scale, rotation angle, position of parent's group
	//	matrix4_scale3f(&boxTransform, &boxTransform, groupOti.scale.x,
	//		groupOti.scale.y, 1.0f);
	//	matrix4_rotate_aa4f(&boxTransform, &boxTransform, 0.0f, 0.0f,
	//		1.0f, RAD(groupOti.rot));
	//	matrix4_translate3f(&boxTransform, &boxTransform,
	//		groupOti.pos.x, groupOti.pos.y, 0.0f);
	//}

    // Switch top/bottom or right/left if scale is negative
	if (oti.scale.x < 0.0f && oti.bounds_type == OBS_BOUNDS_NONE) {
		vec3 l = left;
		vec3 r = right;

		vec3_copy(&left, &r);
		vec3_copy(&right, &l);
	}

	if (oti.scale.y < 0.0f && oti.bounds_type == OBS_BOUNDS_NONE) {
		vec3 t = top;
		vec3 b = bottom;

		vec3_copy(&top, &b);
		vec3_copy(&bottom, &t);
	}

	if (rot >= HELPER_ROT_BREAKPOINT) {
		for (float i = HELPER_ROT_BREAKPOINT; i <= 360.0f; i += 90.0f) {
			if (rot < i)
				break;

			vec3 l = left;
			vec3 r = right;
			vec3 t = top;
			vec3 b = bottom;

			vec3_copy(&top, &l);
			vec3_copy(&right, &t);
			vec3_copy(&bottom, &r);
			vec3_copy(&left, &b);
		}
	}
	else if (rot <= -HELPER_ROT_BREAKPOINT) {
		for (float i = -HELPER_ROT_BREAKPOINT; i >= -360.0f;
			i -= 90.0f) {
			if (rot > i)
				break;

			vec3 l = left;
			vec3 r = right;
			vec3 t = top;
			vec3 b = bottom;

			vec3_copy(&top, &r);
			vec3_copy(&right, &b);
			vec3_copy(&bottom, &l);
			vec3_copy(&left, &t);
		}
	}

	// Get sides of box transform
	left = GetTransformedPos(left.x, left.y, boxTransform);
	right = GetTransformedPos(right.x, right.y, boxTransform);
	top = GetTransformedPos(top.x, top.y, boxTransform);
	bottom = GetTransformedPos(bottom.x, bottom.y, boxTransform);

	bottom.y = size.y - bottom.y;
	right.x = size.x - right.x;

	// Init viewport
	vec3 viewport;
	vec3_set(&viewport, 
			 m_pInitedContextGraphics->GetMainPreviewCX(),
			 m_pInitedContextGraphics->GetMainPreviewCY(),
			 1.0f);

	vec3_div(&left, &left, &viewport);
	vec3_div(&right, &right, &viewport);
	vec3_div(&top, &top, &viewport);
	vec3_div(&bottom, &bottom, &viewport);


	float previewScale = m_pInitedContextGraphics->GetMainPreviewScale();

	vec3_mulf(&left, &left, previewScale);
	vec3_mulf(&right, &right, previewScale);
	vec3_mulf(&top, &top, previewScale);
	vec3_mulf(&bottom, &bottom, previewScale);

	// Draw spacer lines and labels
	vec3 start, end;

	float pixelRatio = dpiValue;
	    
	LabelSourceData initLabel = LabelSourceData(16);
	initLabel.labelRatioSize =  pixelRatio;
	initLabel.labelOutline = true;
#ifdef _WIN32
	initLabel.labelRatioOulineSize = 3;
#endif

	for (int i = 0; i < 4; i++)
		if (!m_obsSpacerLabel[i])
			m_obsSpacerLabel[i] = AFSourceUtil::CreateLabelSource(initLabel);
	

	vec3_set(&start, top.x, 0.0f, 1.0f);
	vec3_set(&end, top.x, top.y, 1.0f);
	_RenderSpacingHelper(0, start, end, viewport, pixelRatio);

	vec3_set(&start, bottom.x, 1.0f - bottom.y, 1.0f);
	vec3_set(&end, bottom.x, 1.0f, 1.0f);
	_RenderSpacingHelper(1, start, end, viewport, pixelRatio);

	vec3_set(&start, 0.0f, left.y, 1.0f);
	vec3_set(&end, left.x, left.y, 1.0f);
	_RenderSpacingHelper(2, start, end, viewport, pixelRatio);

	vec3_set(&start, 1.0f - right.x, right.y, 1.0f);
	vec3_set(&end, 1.0f, right.y, 1.0f);
	_RenderSpacingHelper(3, start, end, viewport, pixelRatio);
}

void AFDrawLinePreview::SetSelectColor(QColor color)
{
	m_selColor = color;
}

void AFDrawLinePreview::SetCropColor(QColor color)
{
	m_cropColor = color;
}

void AFDrawLinePreview::SetHoverColor(QColor color)
{
	m_hoverColor = color;
}

bool AFDrawLinePreview::_DrawSelectedOverflow(obs_scene_t* scene, obs_sceneitem_t* item, void* param)
{
	if (obs_sceneitem_locked(item))
		return true;

	if (!AFSceneUtil::SceneItemHasVideo(item))
		return true;

	AFDrawLinePreview* caller = reinterpret_cast<AFDrawLinePreview*>(param);

	AFBasicPreview* prev = reinterpret_cast<AFBasicPreview*>(param);

	if (!caller->GetOverflowSelectionHidden() && !obs_sceneitem_visible(item))
		return true;

	if (obs_sceneitem_is_group(item)) {
		matrix4 mat;
		obs_sceneitem_get_draw_transform(item, &mat);

		gs_matrix_push();
		gs_matrix_mul(&mat);
		obs_sceneitem_group_enum_items(item, _DrawSelectedOverflow,
			param);
		gs_matrix_pop();
	}

	if (!caller->GetOverflowAlwaysVisible() && !obs_sceneitem_selected(item))
		return true;

	matrix4 boxTransform;
	matrix4 invBoxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);
	matrix4_inv(&invBoxTransform, &boxTransform);

	vec3 bounds[] = {
		{{{0.f, 0.f, 0.f}}},
		{{{1.f, 0.f, 0.f}}},
		{{{0.f, 1.f, 0.f}}},
		{{{1.f, 1.f, 0.f}}},
	};

	bool visible = std::all_of(
		std::begin(bounds), std::end(bounds), [&](const vec3& b) {
			vec3 pos;
			vec3_transform(&pos, &b, &boxTransform);
			vec3_transform(&pos, &pos, &invBoxTransform);
			return close_float(pos.x, b.x, DEF_TOL_EPSILON) && close_float(pos.y, b.y, DEF_TOL_EPSILON);
		});

	if (!visible)
		return true;

	GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "DrawSelectedOverflow");

	gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_REPEAT);
	gs_eparam_t* image = gs_effect_get_param_by_name(solid, "image");
	gs_eparam_t* scale = gs_effect_get_param_by_name(solid, "scale");

	vec2 s;
	vec2_set(&s, boxTransform.x.x / 96, boxTransform.y.y / 96);

	gs_effect_set_vec2(scale, &s);
	gs_effect_set_texture(image, caller->m_texOverflow);

	gs_matrix_push();
	gs_matrix_mul(&boxTransform);

	obs_sceneitem_crop crop;
	obs_sceneitem_get_crop(item, &crop);

	while (gs_effect_loop(solid, "Draw")) {
		gs_draw_sprite(caller->m_texOverflow, 0, 1, 1);
	}

	gs_matrix_pop();

	GS_DEBUG_MARKER_END();

	return true;
}

static void DrawLine(float x1, float y1, float x2, float y2, 
                     float thickness, vec2 scale)
{
    float ySide = (y1 == y2) ? (y1 < 0.5f ? 1.0f : -1.0f) : 0.0f;
    float xSide = (x1 == x2) ? (x1 < 0.5f ? 1.0f : -1.0f) : 0.0f;

    gs_render_start(true);

    gs_vertex2f(x1, y1);
    gs_vertex2f(x1 + (xSide * (thickness / scale.x)),
            y1 + (ySide * (thickness / scale.y)));
    gs_vertex2f(x2 + (xSide * (thickness / scale.x)),
            y2 + (ySide * (thickness / scale.y)));
    gs_vertex2f(x2, y2);
    gs_vertex2f(x1, y1);

    gs_vertbuffer_t *line = gs_render_save();

    gs_load_vertexbuffer(line);
    gs_draw(GS_TRISTRIP, 0, 0);
    gs_vertexbuffer_destroy(line);
}

static void DrawStripedLine(float x1, float y1, float x2, float y2,
                            float thickness, vec2 scale)
{
    float ySide = (y1 == y2) ? (y1 < 0.5f ? 1.0f : -1.0f) : 0.0f;
    float xSide = (x1 == x2) ? (x1 < 0.5f ? 1.0f : -1.0f) : 0.0f;

    float dist =
        sqrt(pow((x1 - x2) * scale.x, 2) + pow((y1 - y2) * scale.y, 2));
    float offX = (x2 - x1) / dist;
    float offY = (y2 - y1) / dist;

    for (int i = 0, l = ceil(dist / 15); i < l; i++) {
        gs_render_start(true);

        float xx1 = x1 + i * 15 * offX;
        float yy1 = y1 + i * 15 * offY;

        float dx;
        float dy;

        if (x1 < x2) {
            dx = std::min(xx1 + 7.5f * offX, x2);
        } else {
            dx = std::max(xx1 + 7.5f * offX, x2);
        }

        if (y1 < y2) {
            dy = std::min(yy1 + 7.5f * offY, y2);
        } else {
            dy = std::max(yy1 + 7.5f * offY, y2);
        }

        gs_vertex2f(xx1, yy1);
        gs_vertex2f(xx1 + (xSide * (thickness / scale.x)),
                yy1 + (ySide * (thickness / scale.y)));
        gs_vertex2f(dx, dy);
        gs_vertex2f(dx + (xSide * (thickness / scale.x)),
                dy + (ySide * (thickness / scale.y)));

        gs_vertbuffer_t *line = gs_render_save();

        gs_load_vertexbuffer(line);
        gs_draw(GS_TRISTRIP, 0, 0);
        gs_vertexbuffer_destroy(line);
    }
}

static void DrawRotationHandle(gs_vertbuffer_t *circle, float rot,
                   float pixelRatio, bool invert)
{
    struct vec3 pos;
    vec3_set(&pos, 0.5f, invert ? 1.0f : 0.0f, 0.0f);

    struct matrix4 matrix;
    gs_matrix_get(&matrix);
    vec3_transform(&pos, &pos, &matrix);

    gs_render_start(true);

    gs_vertex2f(0.5f - 0.34f / HANDLE_RADIUS, 0.5f);
    gs_vertex2f(0.5f - 0.34f / HANDLE_RADIUS, -2.0f);
    gs_vertex2f(0.5f + 0.34f / HANDLE_RADIUS, -2.0f);
    gs_vertex2f(0.5f + 0.34f / HANDLE_RADIUS, 0.5f);
    gs_vertex2f(0.5f - 0.34f / HANDLE_RADIUS, 0.5f);

    gs_vertbuffer_t *line = gs_render_save();

    gs_load_vertexbuffer(line);

    gs_matrix_push();
    gs_matrix_identity();
    gs_matrix_translate(&pos);

    gs_matrix_rotaa4f(0.0f, 0.0f, 1.0f, RAD(rot));
    gs_matrix_translate3f(-HANDLE_RADIUS * 1.5 * pixelRatio,
                  -HANDLE_RADIUS * 1.5 * pixelRatio, 0.0f);
    gs_matrix_scale3f(HANDLE_RADIUS * 3 * pixelRatio,
              HANDLE_RADIUS * 3 * pixelRatio, 1.0f);

    gs_draw(GS_TRISTRIP, 0, 0);

    gs_matrix_translate3f(0.0f, -HANDLE_RADIUS * 2 / 3, 0.0f);

    gs_load_vertexbuffer(circle);
    gs_draw(GS_TRISTRIP, 0, 0);

    gs_matrix_pop();
    gs_vertexbuffer_destroy(line);
}
//

bool AFDrawLinePreview::_DrawSelectedItem(obs_scene_t* scene, obs_sceneitem_t* item, void* param)
{
	if (obs_sceneitem_locked(item))
		return true;

	if (!AFSceneUtil::SceneItemHasVideo(item))
		return true;

    AFDrawLinePreview* caller = reinterpret_cast<AFDrawLinePreview*>(param);

	if (obs_sceneitem_is_group(item)) {
		matrix4 mat;
		obs_transform_info groupInfo;
		obs_sceneitem_get_draw_transform(item, &mat);
		obs_sceneitem_get_info(item, &groupInfo);

		caller->m_fGroupRot = groupInfo.rot;

		gs_matrix_push();
		gs_matrix_mul(&mat);
		obs_sceneitem_group_enum_items(item, _DrawSelectedItem, caller);
		gs_matrix_pop();

		caller->m_fGroupRot = 0.0f;
	}

	float pixelRatio = caller->m_fDpiValueLastDraw;
    

    bool hovered = caller->m_pPreviewModel->CheckNowHovered(item);
	bool selected = obs_sceneitem_selected(item);

	if (!selected && !hovered)
		return true;

	matrix4 boxTransform;
	matrix4 invBoxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);
	matrix4_inv(&invBoxTransform, &boxTransform);
    
    vec3 bounds[] = {
                    {{{0.f, 0.f, 0.f}}},
                    {{{1.f, 0.f, 0.f}}},
                    {{{0.f, 1.f, 0.f}}},
                    {{{1.f, 1.f, 0.f}}},
    };

	QColor selColor = caller->m_selColor;
    QColor cropColor = caller->m_cropColor;
    QColor hoverColor = caller->m_hoverColor;
    
	vec4 vecSelColor;
	vec4_set(&vecSelColor, selColor.redF(), selColor.greenF(), selColor.blueF(), 1.0f);
    
    vec4 vecCropColor;
    vec4_set(&vecCropColor, cropColor.redF(), cropColor.greenF(), cropColor.blueF(), 1.0f);
    
    vec4 vecHoverColor;
    vec4_set(&vecHoverColor, hoverColor.redF(), hoverColor.greenF(), hoverColor.blueF(), 1.0f);

    bool visible = std::all_of(
        std::begin(bounds), std::end(bounds), [&](const vec3 &b) {
            vec3 pos;
            vec3_transform(&pos, &b, &boxTransform);
            vec3_transform(&pos, &pos, &invBoxTransform);
            return close_float(pos.x, b.x, DEF_TOL_EPSILON) && close_float(pos.y, b.y, DEF_TOL_EPSILON);
        });

    if (!visible)
        return true;
    
    GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "_DrawSelectedItem");
    
    matrix4 curTransform;
    vec2 boxScale;
    gs_matrix_get(&curTransform);
    obs_sceneitem_get_box_scale(item, &boxScale);
    boxScale.x *= curTransform.x.x;
    boxScale.y *= curTransform.y.y;
    
	obs_transform_info info;
	obs_sceneitem_get_info(item, &info);

	gs_matrix_push();
	gs_matrix_mul(&boxTransform);

	obs_sceneitem_crop crop;
	obs_sceneitem_get_crop(item, &crop);
    
    gs_effect_t* eff = gs_get_effect();
    gs_eparam_t* colParam = gs_effect_get_param_by_name(eff, "color");

    
	gs_effect_set_vec4(colParam, &vecSelColor);

	if (info.bounds_type == OBS_BOUNDS_NONE && AFSceneUtil::IsCropEnabled(&crop)) {
		#define DRAW_SIDE(side, x1, y1, x2, y2)                                 \
			if (hovered && !selected) {                                         \
                gs_effect_set_vec4(colParam, &vecHoverColor);                   \
                DrawLine(x1, y1, x2, y2, HANDLE_RADIUS *pixelRatio / 2,         \
                        boxScale);                                              \
			} else if (crop.side > 0) {                                         \
                gs_effect_set_vec4(colParam, &vecCropColor);                    \
                DrawStripedLine(x1, y1, x2, y2, HANDLE_RADIUS *pixelRatio / 2,  \
                        boxScale);   											\
			} else {                                                            \
				DrawLine(x1, y1, x2, y2, HANDLE_RADIUS *pixelRatio / 2,         \
					 boxScale);													\
			}                                                                   \
			gs_effect_set_vec4(colParam, &vecSelColor);
		
				DRAW_SIDE(left, 0.0f, 0.0f, 0.0f, 1.0f);
				DRAW_SIDE(top, 0.0f, 0.0f, 1.0f, 0.0f);
				DRAW_SIDE(right, 1.0f, 0.0f, 1.0f, 1.0f);
				DRAW_SIDE(bottom, 0.0f, 1.0f, 1.0f, 1.0f);
		#undef DRAW_SIDE
        //
	}
	else {
		if (!selected) {
            gs_effect_set_vec4(colParam, &vecHoverColor);
            AFDrawLinePreview::DrawRectOBSGFX(4.0f * 1 / 2, boxScale);
		}
		else {
			AFDrawLinePreview::DrawRectOBSGFX(4.0f * 1 / 2, boxScale);
		}
	}
    //



	if (caller->m_pInitedContextGraphics != nullptr)
		gs_load_vertexbuffer(caller->m_pInitedContextGraphics->GetBoxVB());

    gs_effect_set_vec4(colParam, &vecSelColor);
    
	if (selected)
    {
		AFDrawLinePreview::DrawSquareAtPosOBSGFX(0.0f, 0.0f, pixelRatio);
		AFDrawLinePreview::DrawSquareAtPosOBSGFX(0.0f, 1.0f, pixelRatio);
		AFDrawLinePreview::DrawSquareAtPosOBSGFX(1.0f, 0.0f, pixelRatio);
		AFDrawLinePreview::DrawSquareAtPosOBSGFX(1.0f, 1.0f, pixelRatio);
		AFDrawLinePreview::DrawSquareAtPosOBSGFX(0.5f, 0.0f, pixelRatio);
		AFDrawLinePreview::DrawSquareAtPosOBSGFX(0.0f, 0.5f, pixelRatio);
		AFDrawLinePreview::DrawSquareAtPosOBSGFX(0.5f, 1.0f, pixelRatio);
		AFDrawLinePreview::DrawSquareAtPosOBSGFX(1.0f, 0.5f, pixelRatio);
        
        if (!caller->m_vbCircleFill)
        {
            gs_render_start(true);

            float angle = 180;
            for (int i = 0, l = 40; i < l; i++) 
            {
                gs_vertex2f(sin(RAD(angle)) / 2 + 0.5f,
                            cos(RAD(angle)) / 2 + 0.5f);
                angle += 360 / l;
                gs_vertex2f(sin(RAD(angle)) / 2 + 0.5f,
                            cos(RAD(angle)) / 2 + 0.5f);
                gs_vertex2f(0.5f, 1.0f);
            }

            caller->m_vbCircleFill = gs_render_save();
        }
        
        bool invert = info.scale.y < 0.0f &&
                        info.bounds_type == OBS_BOUNDS_NONE;
        DrawRotationHandle(caller->m_vbCircleFill, info.rot /*+ prev->groupRot*/,
                           pixelRatio, invert);
       
        //
	}

	gs_matrix_pop();

    GS_DEBUG_MARKER_END();
    
	return true;
}

void AFDrawLinePreview::_DrawLabel(OBSSource source, vec3& pos, vec3& viewport)
{
	if (!source)
		return;

	vec3_mul(&pos, &pos, &viewport);

	gs_matrix_push();
	gs_matrix_identity();
	gs_matrix_translate(&pos);
	obs_source_video_render(source);
	gs_matrix_pop();
}

void AFDrawLinePreview::_DrawSpacingLine(vec3& start, vec3& end, vec3& viewport, float pixelRatio)
{
	matrix4 transform;
	matrix4_identity(&transform);
	transform.x.x = viewport.x;
	transform.y.y = viewport.y;

	gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
	gs_technique_t* tech = gs_effect_get_technique(solid, "Solid");
    
	QColor selColor = m_selColor;
	vec4 color;
	vec4_set(&color, selColor.redF(), selColor.greenF(), selColor.blueF(), 1.0f);

	gs_effect_set_vec4(gs_effect_get_param_by_name(solid, "color"), &color);

	gs_technique_begin(tech);
	gs_technique_begin_pass(tech, 0);

	gs_matrix_push();
	gs_matrix_mul(&transform);

	vec2 scale;
	vec2_set(&scale, viewport.x, viewport.y);

	DrawLineOBSGFX(start.x, start.y, end.x, end.y,
				   pixelRatio * (HANDLE_RADIUS / 2), scale);

	gs_matrix_pop();

	gs_load_vertexbuffer(nullptr);

	gs_technique_end_pass(tech);
	gs_technique_end(tech);
}

void AFDrawLinePreview::_SetLabelText(int sourceIndex, int px)
{
    
	if (px == m_iarrSpacerPx[sourceIndex])
		return;

	std::string text = std::to_string(px) + " px";

	obs_source_t* source = m_obsSpacerLabel[sourceIndex];

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_string(settings, "text", text.c_str());
	obs_source_update(source, settings);

	m_iarrSpacerPx[sourceIndex] = px;
}

bool AFDrawLinePreview::_DrawSelectionBox(float x1, float y1, float x2, float y2,
                                          gs_vertbuffer_t* rectFill, float dpiValue/* = 1.f*/)
{
	float pixelRatio = dpiValue;

	x1 = std::round(x1);
	x2 = std::round(x2);
	y1 = std::round(y1);
	y2 = std::round(y2);

	gs_effect_t* eff = gs_get_effect();
	gs_eparam_t* colParam = gs_effect_get_param_by_name(eff, "color");

	vec4 fillColor;
	vec4_set(&fillColor, 0.7f, 0.7f, 0.7f, 0.5f);

	vec4 borderColor;
	vec4_set(&borderColor, 1.0f, 1.0f, 1.0f, 1.0f);

	vec2 scale;
	vec2_set(&scale, std::abs(x2 - x1), std::abs(y2 - y1));

	gs_matrix_push();
	gs_matrix_identity();

	gs_matrix_translate3f(x1, y1, 0.0f);
	gs_matrix_scale3f(x2 - x1, y2 - y1, 1.0f);

	gs_effect_set_vec4(colParam, &fillColor);
	gs_load_vertexbuffer(rectFill);
	gs_draw(GS_TRISTRIP, 0, 0);

	gs_effect_set_vec4(colParam, &borderColor);
	DrawRectOBSGFX(HANDLE_RADIUS * pixelRatio / 2, scale);

	gs_matrix_pop();


	return true;
}

void AFDrawLinePreview::_RenderSpacingHelper(int sourceIndex, vec3& start, vec3& end,
	vec3& viewport, float pixelRatio)
{
	bool horizontal = (sourceIndex == 2 || sourceIndex == 3);

	// If outside of preview, don't render
	if (!((horizontal && (end.x >= start.x)) ||
		(!horizontal && (end.y >= start.y))))
		return;

	float length = vec3_dist(&start, &end);

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	float px;

	if (horizontal) {
		px = length * ovi.base_width;
	}
	else {
		px = length * ovi.base_height;
	}

	if (px <= 0.0f)
		return;

	obs_source_t* source = m_obsSpacerLabel[sourceIndex];
	vec3 labelSize, labelPos;
	vec3_set(&labelSize, obs_source_get_width(source),
		obs_source_get_height(source), 1.0f);

	vec3_div(&labelSize, &labelSize, &viewport);

	vec3 labelMargin;
	vec3_set(&labelMargin, SPACER_LABEL_MARGIN * pixelRatio,
		SPACER_LABEL_MARGIN * pixelRatio, 1.0f);
	vec3_div(&labelMargin, &labelMargin, &viewport);

	vec3_set(&labelPos, end.x, end.y, end.z);
	if (horizontal) {
		labelPos.x -= (end.x - start.x) / 2;
		labelPos.x -= labelSize.x / 2;
		labelPos.y -= labelMargin.y + (labelSize.y / 2) +
			(HANDLE_RADIUS / viewport.y);
	}
	else {
		labelPos.y -= (end.y - start.y) / 2;
		labelPos.y -= labelSize.y / 2;
		labelPos.x += labelMargin.x;
	}

	_DrawSpacingLine(start, end, viewport, pixelRatio);
	_SetLabelText(sourceIndex, (int)px);
	_DrawLabel(source, labelPos, viewport);
}
