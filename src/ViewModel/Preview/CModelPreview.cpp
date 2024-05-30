#include "CModelPreview.h"


#include <cmath>
#include <vector>
#include <algorithm>




#include "CMouseStaterPreview.h"
#include "CDrawLinePreview.h"
#include "UIComponent/CBasicPreview.h"





#include "Common/MathMiscUtils.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Scene/CScene.h"
#include "CoreModel/Scene/CSceneContext.h"




struct HandleFindData
{
	const vec2& pos;
	const float radius;
	matrix4 parent_xform;

	OBSSceneItem item;
	ItemHandle handle = ItemHandle::None;
	float angle = 0.0f;
	vec2 rotatePoint;
	vec2 offsetPoint;

	float angleOffset = 0.0f;

	HandleFindData(const HandleFindData&) = delete;
	HandleFindData(HandleFindData&&) = delete;
	HandleFindData& operator=(const HandleFindData&) = delete;
	HandleFindData& operator=(HandleFindData&&) = delete;

	inline HandleFindData(const vec2& pos_, float scale)
		: pos(pos_),
		radius(HANDLE_SEL_RADIUS / scale)
	{
		matrix4_identity(&parent_xform);
	}

	inline HandleFindData(const HandleFindData& hfd,
		obs_sceneitem_t* parent)
		: pos(hfd.pos),
		radius(hfd.radius),
		item(hfd.item),
		handle(hfd.handle),
		angle(hfd.angle),
		rotatePoint(hfd.rotatePoint),
		offsetPoint(hfd.offsetPoint)
	{
		obs_sceneitem_get_draw_transform(parent, &parent_xform);
	}
};

struct SelectedItemBounds 
{
    bool first = true;
    vec3 tl, br;
};

struct OffsetData
{
    float clampDist;
    vec3 tl, br, offset;
};


bool AFModelPreview::FindSelected(obs_scene_t* scene, obs_sceneitem_t* item, void* param)
{
	SceneFindBoxData* data = reinterpret_cast<SceneFindBoxData*>(param);

	if (obs_sceneitem_selected(item))
		data->sceneItems.push_back(item);

	return true;
}

bool AFModelPreview::FindItemsInBox(obs_scene_t* /*scene*/, obs_sceneitem_t* item, void* param)
{
    SceneFindBoxData *data = reinterpret_cast<SceneFindBoxData *>(param);
    matrix4 transform;
    matrix4 invTransform;
    vec3 transformedPos;
    vec3 pos3;
    vec3 pos3_;

    vec2 pos_min, pos_max;
    vec2_min(&pos_min, &data->startPos, &data->pos);
    vec2_max(&pos_max, &data->startPos, &data->pos);

    const float x1 = pos_min.x;
    const float x2 = pos_max.x;
    const float y1 = pos_min.y;
    const float y2 = pos_max.y;

    if (!AFSceneUtil::SceneItemHasVideo(item))
        return true;
    if (obs_sceneitem_locked(item))
        return true;
    if (!obs_sceneitem_visible(item))
        return true;

    vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

    obs_sceneitem_get_box_transform(item, &transform);

    matrix4_inv(&invTransform, &transform);
    vec3_transform(&transformedPos, &pos3, &invTransform);
    vec3_transform(&pos3_, &transformedPos, &transform);

    if (close_float(pos3.x, pos3_.x, DEF_TOL_EPSILON) &&
        close_float(pos3.y, pos3_.y, DEF_TOL_EPSILON) &&
        transformedPos.x >= 0.0f && transformedPos.x <= 1.0f &&
        transformedPos.y >= 0.0f && transformedPos.y <= 1.0f)
    {
        data->sceneItems.push_back(item);
        return true;
    }

    if (transform.t.x > x1 && transform.t.x < x2 && transform.t.y > y1 &&
        transform.t.y < y2)
    {
        data->sceneItems.push_back(item);
        return true;
    }

    if (transform.t.x + transform.x.x > x1 &&
        transform.t.x + transform.x.x < x2 &&
        transform.t.y + transform.x.y > y1 &&
        transform.t.y + transform.x.y < y2)
    {
        data->sceneItems.push_back(item);
        return true;
    }

    if (transform.t.x + transform.y.x > x1 &&
        transform.t.x + transform.y.x < x2 &&
        transform.t.y + transform.y.y > y1 &&
        transform.t.y + transform.y.y < y2)
    {
        data->sceneItems.push_back(item);
        return true;
    }

    if (transform.t.x + transform.x.x + transform.y.x > x1 &&
        transform.t.x + transform.x.x + transform.y.x < x2 &&
        transform.t.y + transform.x.y + transform.y.y > y1 &&
        transform.t.y + transform.x.y + transform.y.y < y2)
    {
        data->sceneItems.push_back(item);
        return true;
    }

    if (transform.t.x + 0.5 * (transform.x.x + transform.y.x) > x1 &&
        transform.t.x + 0.5 * (transform.x.x + transform.y.x) < x2 &&
        transform.t.y + 0.5 * (transform.x.y + transform.y.y) > y1 &&
        transform.t.y + 0.5 * (transform.x.y + transform.y.y) < y2)
    {
        data->sceneItems.push_back(item);
        return true;
    }

    if (_IntersectBox(transform, x1, x2, y1, y2))
    {
        data->sceneItems.push_back(item);
        return true;
    }

    return true;
}

bool AFModelPreview::NudgeCallBack(obs_scene_t* /*scene*/, obs_sceneitem_t* item, void* param)
{
    if (obs_sceneitem_locked(item))
        return true;

    struct vec2 &offset = *reinterpret_cast<struct vec2 *>(param);
    struct vec2 pos;

    if (!obs_sceneitem_selected(item)) {
        if (obs_sceneitem_is_group(item)) {
            struct vec3 offset3;
            vec3_set(&offset3, offset.x, offset.y, 0.0f);

            struct matrix4 matrix;
            obs_sceneitem_get_draw_transform(item, &matrix);
            vec4_set(&matrix.t, 0.0f, 0.0f, 0.0f, 1.0f);
            matrix4_inv(&matrix, &matrix);
            vec3_transform(&offset3, &offset3, &matrix);

            struct vec2 new_offset;
            vec2_set(&new_offset, offset3.x, offset3.y);
            obs_sceneitem_group_enum_items(item, NudgeCallBack,
                               &new_offset);
        }

        return true;
    }

    obs_sceneitem_get_pos(item, &pos);
    vec2_add(&pos, &pos, &offset);
    obs_sceneitem_set_pos(item, &pos);
    return true;
}

vec2 AFModelPreview::GetItemSize(obs_sceneitem_t* item)
{
	obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(item);
	vec2 size;


	if (boundsType != OBS_BOUNDS_NONE)
		obs_sceneitem_get_bounds(item, &size);
	else
	{
		obs_source_t* source = obs_sceneitem_get_source(item);
		obs_sceneitem_crop crop;
		vec2 scale;

		obs_sceneitem_get_scale(item, &scale);
		obs_sceneitem_get_crop(item, &crop);


		size.x = float(obs_source_get_width(source) - crop.left -
			crop.right) * scale.x;
		size.y = float(obs_source_get_height(source) - crop.top -
			crop.bottom) * scale.y;
	}


	return size;
}

void AFModelPreview::ClearSelectedItems()
{
    std::lock_guard<std::mutex> lock(m_SelectMutex);
    m_vecSelectedItems.clear();
}

void AFModelPreview::SetSelectedItems()
{
    if (m_pInitedContextScene == nullptr)
        return;
    
    
    vec2 s;
    SceneFindBoxData data(s, s);

    obs_scene_enum_items(m_pInitedContextScene->GetCurrOBSScene(), 
                         FindSelected, &data);

    std::lock_guard<std::mutex> lock(m_SelectMutex);
    m_vecSelectedItems = data.sceneItems;
}

void AFModelPreview::ClearHoveredItems(bool selectionBox)
{
    std::lock_guard<std::mutex> lock(m_SelectMutex);
    m_vecHoveredPreviewItems.clear();
}

void AFModelPreview::EnumSelecedHoveredItems(const bool altDown,
                                             const bool shiftDown,
                                             const bool ctrlDown)
{
    std::lock_guard<std::mutex> lock(m_SelectMutex);
    if (altDown || ctrlDown || shiftDown)
    {
        for (size_t i = 0; i < m_vecSelectedItems.size(); i++)
            obs_sceneitem_select(m_vecSelectedItems[i], true);
        
    }

    for (size_t i = 0; i < m_vecHoveredPreviewItems.size(); i++)
    {
        bool select = true;
        obs_sceneitem_t* item = m_vecHoveredPreviewItems[i];

        if (altDown)
            select = false;
        else if (ctrlDown)
            select = !obs_sceneitem_selected(item);
        
        obs_sceneitem_select(m_vecHoveredPreviewItems[i], select);
    }
}

uint32_t AFModelPreview::MakeHoveredItem(const vec2& pos)
{
    OBSSceneItem item = GetItemAtPos(pos, true);

    std::lock_guard<std::mutex> lock(m_SelectMutex);
    m_vecHoveredPreviewItems.clear();
    m_vecHoveredPreviewItems.push_back(item);
    
    uint32_t n = (uint32_t)m_vecHoveredPreviewItems.size();
    
    return n;
}

void AFModelPreview::MakeLastHoveredItem(const vec2& pos)
{
    OBSSceneItem item = GetItemAtPos(pos, true);

    std::lock_guard<std::mutex> lock(m_SelectMutex);
    m_vecHoveredPreviewItems.clear();
    m_vecHoveredPreviewItems.push_back(item);
    m_vecSelectedItems.clear();
}
//

void AFModelPreview::SetUnsafeAccessContext(AFGraphicsContext* pGraphicsContext,
											AFSceneContext* pSceneContext)
{
	if (m_pInitedContextGraphics == nullptr)
		m_pInitedContextGraphics = pGraphicsContext;

	if (m_pInitedContextScene == nullptr)
		m_pInitedContextScene = pSceneContext;
}

void AFModelPreview::Reset()
{
    if (m_obsStretchGroup)
        obs_sceneitem_defer_group_resize_end(m_obsStretchGroup);

    m_obsStretchItem = nullptr;
    m_obsStretchGroup = nullptr;
}

bool AFModelPreview::IsLocked()
{
	return obs_sceneitem_locked(m_obsStretchItem);
}

void AFModelPreview::GetStretchHandleData(const vec2& pos, bool ignoreGroup,
                                          AFMouseStaterPreview& mouseState, float dpiValue/* = 1.f*/)
{
	OBSScene scene = nullptr;

	if (m_pInitedContextScene != nullptr)
		scene = m_pInitedContextScene->GetCurrOBSScene();

	if (!scene)
		return;

	float scale = 1.f;
	
	if ( m_pInitedContextGraphics != nullptr )
		scale = m_pInitedContextGraphics->GetMainPreviewScale() / dpiValue;
	vec2 scaled_pos = pos;
	vec2_divf(&scaled_pos, &scaled_pos, scale);
	HandleFindData data(scaled_pos, scale);
	obs_scene_enum_items(scene, _FindHandleAtPos, &data);

	m_obsStretchItem = std::move(data.item);
	mouseState.SetCurrStateHandle(data.handle);
    
    m_fRotateAngle = data.angle;
    m_vec2RotatePoint = data.rotatePoint;
    m_vec2OffsetPoint = data.offsetPoint;

	if (mouseState.GetCurrStateHandle() != ItemHandle::None)
	{
		matrix4 boxTransform;
		vec3 itemUL;
		float itemRot;

		m_vec2StretchSize = GetItemSize(m_obsStretchItem);

		obs_sceneitem_get_box_transform(m_obsStretchItem, &boxTransform);
		itemRot = obs_sceneitem_get_rot(m_obsStretchItem);
		vec3_from_vec4(&itemUL, &boxTransform.t);

		/* build the item space conversion matrices */
		matrix4_identity(&m_matItemToScreen);
		matrix4_rotate_aa4f(&m_matItemToScreen, &m_matItemToScreen, 0.0f, 0.0f,
							1.0f, RAD(itemRot));
		matrix4_translate3f(&m_matItemToScreen, &m_matItemToScreen, itemUL.x,
							itemUL.y, 0.0f);

		matrix4_identity(&m_matScreenToItem);
		matrix4_translate3f(&m_matScreenToItem, &m_matScreenToItem, -itemUL.x,
							-itemUL.y, 0.0f);
		matrix4_rotate_aa4f(&m_matScreenToItem, &m_matScreenToItem, 0.0f, 0.0f,
							1.0f, RAD(-itemRot));
        
        obs_sceneitem_get_crop(m_obsStretchItem, &m_obsStartCrop);
        obs_sceneitem_get_pos(m_obsStretchItem, &m_vec2StartItemPos);
        
        obs_source_t *source = obs_sceneitem_get_source(m_obsStretchItem);
        m_vec2CropSize.x = float(obs_source_get_width(source) -
                           m_obsStartCrop.left - m_obsStartCrop.right);
        m_vec2CropSize.y = float(obs_source_get_height(source) -
                                 m_obsStartCrop.top - m_obsStartCrop.bottom);
        
        m_obsStretchGroup = obs_sceneitem_get_group(scene, m_obsStretchItem);
        if (m_obsStretchGroup && !ignoreGroup)
        {
            obs_sceneitem_get_draw_transform(m_obsStretchGroup,
                                             &m_matInvGroupTransform);
            matrix4_inv(&m_matInvGroupTransform, &m_matInvGroupTransform);
            obs_sceneitem_defer_group_resize_begin(m_obsStretchGroup);
        }
        else
            m_obsStretchGroup = nullptr;
	}
}

OBSSceneItem AFModelPreview::GetItemAtPos(const vec2& pos, bool selectBelow)
{
	OBSScene scene = nullptr;

	if (m_pInitedContextScene != nullptr)
		scene = m_pInitedContextScene->GetCurrOBSScene();

	if (!scene)
		return OBSSceneItem();

	SceneFindData data(pos, selectBelow);
	obs_scene_enum_items(scene, _FindItemAtPos, &data);
	return data.item;
}

bool AFModelPreview::SelectedAtPos(const vec2& pos)
{
	OBSScene scene = nullptr;

	if (m_pInitedContextScene != nullptr)
		scene = m_pInitedContextScene->GetCurrOBSScene();

	if (!scene)
		return false;

	SceneFindData data(pos, false);
	obs_scene_enum_items(scene, _CheckItemSelected, &data);
	return !!data.item;
}

void AFModelPreview::DoSelect(const vec2& pos)
{
	OBSScene scene = nullptr;

	if (m_pInitedContextScene != nullptr)
		scene = m_pInitedContextScene->GetCurrOBSScene();

	OBSSceneItem item = GetItemAtPos(pos, true);

	obs_scene_enum_items(scene, _SelectOne, (obs_sceneitem_t*)item);
}

void AFModelPreview::DoCtrlSelect(const vec2 &pos)
{
    OBSSceneItem item = GetItemAtPos(pos, false);
    if (!item)
        return;

    bool selected = obs_sceneitem_selected(item);
    obs_sceneitem_select(item, !selected);
}

void AFModelPreview::CropItem(const vec2& pos, AFMouseStaterPreview& mouseState)
{
    obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(m_obsStretchItem);
    ItemHandle stretchHandle = mouseState.GetCurrStateHandle();
    uint32_t stretchFlags = (uint32_t)stretchHandle;
    uint32_t align = obs_sceneitem_get_alignment(m_obsStretchItem);
    vec3 tl, br, pos3;

    vec3_zero(&tl);
    vec3_set(&br, m_vec2StretchSize.x, m_vec2StretchSize.y, 0.0f);

    vec3_set(&pos3, pos.x, pos.y, 0.0f);
    vec3_transform(&pos3, &pos3, &m_matScreenToItem);

    obs_sceneitem_crop crop = m_obsStartCrop;
    vec2 scale, rawscale;

    obs_sceneitem_get_scale(m_obsStretchItem, &rawscale);
    vec2_set(&scale,
             boundsType == OBS_BOUNDS_NONE ? rawscale.x : fabsf(rawscale.x),
             boundsType == OBS_BOUNDS_NONE ? rawscale.y
                           : fabsf(rawscale.y));

    vec2 max_tl;
    vec2 max_br;

    vec2_set(&max_tl, float(-crop.left) * scale.x,
             float(-crop.top) * scale.y);
    vec2_set(&max_br, m_vec2StretchSize.x + crop.right * scale.x,
             m_vec2StretchSize.y + crop.bottom * scale.y);

    typedef std::function<float(float, float)> minmax_func_t;

    
    auto maxfunc = [](float x, float y) {
        return x > y ? x : y;
    };

    auto minfunc = [](float x, float y) {
        return x < y ? x : y;
    };

    
    minmax_func_t min_x = scale.x < 0.0f && boundsType == OBS_BOUNDS_NONE
                      ? maxfunc
                      : minfunc;
    minmax_func_t min_y = scale.y < 0.0f && boundsType == OBS_BOUNDS_NONE
                      ? maxfunc
                      : minfunc;
    minmax_func_t max_x = scale.x < 0.0f && boundsType == OBS_BOUNDS_NONE
                      ? minfunc
                      : maxfunc;
    minmax_func_t max_y = scale.y < 0.0f && boundsType == OBS_BOUNDS_NONE
                      ? minfunc
                      : maxfunc;

    pos3.x = min_x(pos3.x, max_br.x);
    pos3.x = max_x(pos3.x, max_tl.x);
    pos3.y = min_y(pos3.y, max_br.y);
    pos3.y = max_y(pos3.y, max_tl.y);

    if (stretchFlags & ITEM_LEFT) {
        float maxX = m_vec2StretchSize.x - (2.0 * scale.x);
        pos3.x = tl.x = min_x(pos3.x, maxX);

    } else if (stretchFlags & ITEM_RIGHT) {
        float minX = (2.0 * scale.x);
        pos3.x = br.x = max_x(pos3.x, minX);
    }

    if (stretchFlags & ITEM_TOP) {
        float maxY = m_vec2StretchSize.y - (2.0 * scale.y);
        pos3.y = tl.y = min_y(pos3.y, maxY);

    } else if (stretchFlags & ITEM_BOTTOM) {
        float minY = (2.0 * scale.y);
        pos3.y = br.y = max_y(pos3.y, minY);
    }

#define ALIGN_X (ITEM_LEFT | ITEM_RIGHT)
#define ALIGN_Y (ITEM_TOP | ITEM_BOTTOM)
    vec3 newPos;
    vec3_zero(&newPos);

    uint32_t align_x = (align & ALIGN_X);
    uint32_t align_y = (align & ALIGN_Y);
    if (align_x == (stretchFlags & ALIGN_X) && align_x != 0)
        newPos.x = pos3.x;
    else if (align & ITEM_RIGHT)
        newPos.x = m_vec2StretchSize.x;
    else if (!(align & ITEM_LEFT))
        newPos.x = m_vec2StretchSize.x * 0.5f;

    if (align_y == (stretchFlags & ALIGN_Y) && align_y != 0)
        newPos.y = pos3.y;
    else if (align & ITEM_BOTTOM)
        newPos.y = m_vec2StretchSize.y;
    else if (!(align & ITEM_TOP))
        newPos.y = m_vec2StretchSize.y * 0.5f;
#undef ALIGN_X
#undef ALIGN_Y

    crop = m_obsStartCrop;

    if (stretchFlags & ITEM_LEFT)
        crop.left += int(std::round(tl.x / scale.x));
    else if (stretchFlags & ITEM_RIGHT)
        crop.right +=
            int(std::round((m_vec2StretchSize.x - br.x) / scale.x));

    if (stretchFlags & ITEM_TOP)
        crop.top += int(std::round(tl.y / scale.y));
    else if (stretchFlags & ITEM_BOTTOM)
        crop.bottom +=
            int(std::round((m_vec2StretchSize.y - br.y) / scale.y));

    vec3_transform(&newPos, &newPos, &m_matItemToScreen);
    newPos.x = std::round(newPos.x);
    newPos.y = std::round(newPos.y);

#if 0
    vec3 curPos;
    vec3_zero(&curPos);
    obs_sceneitem_get_pos(m_obsStretchItem, (vec2*)&curPos);
    blog(LOG_DEBUG, "curPos {%d, %d} - newPos {%d, %d}",
            int(curPos.x), int(curPos.y),
            int(newPos.x), int(newPos.y));
    blog(LOG_DEBUG, "crop {%d, %d, %d, %d}",
            crop.left, crop.top,
            crop.right, crop.bottom);
#endif

    obs_sceneitem_defer_update_begin(m_obsStretchItem);
    obs_sceneitem_set_crop(m_obsStretchItem, &crop);
    if (boundsType == OBS_BOUNDS_NONE)
        obs_sceneitem_set_pos(m_obsStretchItem, (vec2 *)&newPos);
    obs_sceneitem_defer_update_end(m_obsStretchItem);
}

void AFModelPreview::StretchItem(const vec2& pos, AFMouseStaterPreview& mouseState,
                                 bool shiftDown, bool controlDown)
{
	obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(m_obsStretchItem);
	ItemHandle stretchHandle = mouseState.GetCurrStateHandle();
	uint32_t stretchFlags = (uint32_t)stretchHandle;
	vec3 tl, br, pos3;

	vec3_zero(&tl);
	vec3_set(&br, m_vec2StretchSize.x, m_vec2StretchSize.y, 0.0f);

	vec3_set(&pos3, pos.x, pos.y, 0.0f);
	vec3_transform(&pos3, &pos3, &m_matScreenToItem);

	if (stretchFlags & ITEM_LEFT)
		tl.x = pos3.x;
	else if (stretchFlags & ITEM_RIGHT)
		br.x = pos3.x;

	if (stretchFlags & ITEM_TOP)
		tl.y = pos3.y;
	else if (stretchFlags & ITEM_BOTTOM)
		br.y = pos3.y;

	if (controlDown == false)
        _SnapStretchingToScreen(tl, br, stretchFlags);

	obs_source_t* source = obs_sceneitem_get_source(m_obsStretchItem);

	uint32_t source_cx = obs_source_get_width(source);
	uint32_t source_cy = obs_source_get_height(source);

	/* if the source's internal size has been set to 0 for whatever reason
	 * while resizing, do not update transform, otherwise source will be
	 * stuck invisible until a complete transform reset */
	if (!source_cx || !source_cy)
		return;

	vec2 baseSize;
	vec2_set(&baseSize, float(source_cx), float(source_cy));

	vec2 size;
	vec2_set(&size, br.x - tl.x, br.y - tl.y);

	if (boundsType != OBS_BOUNDS_NONE) {
		if (shiftDown)
			_ClampAspect(tl, br, size, baseSize, mouseState);

		if (tl.x > br.x)
			std::swap(tl.x, br.x);
		if (tl.y > br.y)
			std::swap(tl.y, br.y);

		vec2_abs(&size, &size);

		obs_sceneitem_set_bounds(m_obsStretchItem, &size);
	}
	else {
		obs_sceneitem_crop crop;
		obs_sceneitem_get_crop(m_obsStretchItem, &crop);

		baseSize.x -= float(crop.left + crop.right);
		baseSize.y -= float(crop.top + crop.bottom);

		if (!shiftDown)
			_ClampAspect(tl, br, size, baseSize, mouseState);

		vec2_div(&size, &size, &baseSize);
		obs_sceneitem_set_scale(m_obsStretchItem, &size);
	}

	pos3 = _CalculateStretchPos(tl, br);
	vec3_transform(&pos3, &pos3, &m_matItemToScreen);

	vec2 newPos;
	vec2_set(&newPos, std::round(pos3.x), std::round(pos3.y));
	obs_sceneitem_set_pos(m_obsStretchItem, &newPos);
}

void AFModelPreview::RotateItem(const vec2& pos, bool shiftDown, bool controlDown)
{
    OBSScene scene = nullptr;

    if (m_pInitedContextScene != nullptr)
        scene = m_pInitedContextScene->GetCurrOBSScene();

    vec2 pos2;
    vec2_copy(&pos2, &pos);

    float angle =
        std::atan2(pos2.y - m_vec2RotatePoint.y,
                   pos2.x - m_vec2RotatePoint.x) + RAD(90);

#define ROT_SNAP(rot, thresh)                      \
    if (abs(angle - RAD(rot)) < RAD(thresh)) { \
        angle = RAD(rot);                  \
    }

    if (shiftDown)
    {
        for (int i = 0; i <= 360 / 15; i++)
            ROT_SNAP(i * 15 - 90, 7.5);
    }
    else if (!controlDown)
    {
        ROT_SNAP(m_fRotateAngle, 5)

        ROT_SNAP(-90, 5)
        ROT_SNAP(-45, 5)
        ROT_SNAP(0, 5)
        ROT_SNAP(45, 5)
        ROT_SNAP(90, 5)
        ROT_SNAP(135, 5)
        ROT_SNAP(180, 5)
        ROT_SNAP(225, 5)
        ROT_SNAP(270, 5)
        ROT_SNAP(315, 5)
    }
#undef ROT_SNAP

    vec2 pos3;
    vec2_copy(&pos3, &m_vec2OffsetPoint);
    RotatePos(&pos3, angle);
    pos3.x += m_vec2RotatePoint.x;
    pos3.y += m_vec2RotatePoint.y;

    obs_sceneitem_set_rot(m_obsStretchItem, DEG(angle));
    obs_sceneitem_set_pos(m_obsStretchItem, &pos3);
}

void AFModelPreview::MoveItems(const vec2& pos, vec2& lastMoveOffset, vec2& startPos, bool controlDown)
{
	OBSScene scene = nullptr;

	if (m_pInitedContextScene != nullptr)
		scene = m_pInitedContextScene->GetCurrOBSScene();

	vec2 offset, moveOffset;
	vec2_sub(&offset, &pos, &startPos);
	vec2_sub(&moveOffset, &offset, &lastMoveOffset);

    
    if (controlDown == false)
        _SnapItemMovement(moveOffset);
    
    
	vec2_add(&lastMoveOffset, &lastMoveOffset, &moveOffset);

	obs_scene_enum_items(scene, _MoveItems, &moveOffset);
}

void AFModelPreview::BoxItems(OBSScene scene, const vec2 &startPos, const vec2 &pos)
{
    SceneFindBoxData data(startPos, pos);
    obs_scene_enum_items(scene, FindItemsInBox, &data);


    std::lock_guard<std::mutex> lock(m_SelectMutex);
    m_vecHoveredPreviewItems = data.sceneItems;
}

bool AFModelPreview::CheckNowHovered(obs_sceneitem_t* item)
{
    bool hovered = false;
    
    if (item == nullptr) return hovered;
    
    std::lock_guard<std::mutex> lock(m_SelectMutex);
    for (size_t i = 0; i < m_vecHoveredPreviewItems.size(); i++)
        if (m_vecHoveredPreviewItems[i] == item)
        {
            hovered = true;
            break;
        }

    
    return hovered;
}

bool AFModelPreview::_FindItemAtPos(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param)
{
	SceneFindData* data = reinterpret_cast<SceneFindData*>(param);
	matrix4 transform;
	matrix4 invTransform;
	vec3 transformedPos;
	vec3 pos3;
	vec3 pos3_;

	if (!AFSceneUtil::SceneItemHasVideo(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

	obs_sceneitem_get_box_transform(item, &transform);

	matrix4_inv(&invTransform, &transform);
	vec3_transform(&transformedPos, &pos3, &invTransform);
	vec3_transform(&pos3_, &transformedPos, &transform);

	if (close_float(pos3.x, pos3_.x, DEF_TOL_EPSILON) && 
        close_float(pos3.y, pos3_.y, DEF_TOL_EPSILON) &&
		transformedPos.x >= 0.0f && transformedPos.x <= 1.0f &&
		transformedPos.y >= 0.0f && transformedPos.y <= 1.0f) 
    {
		if (data->selectBelow && obs_sceneitem_selected(item)) 
        {
			if (data->item)
				return false;
			else
				data->selectBelow = false;
		}

		data->item = item;
	}

	return true;
}

bool AFModelPreview::_FindHandleAtPos(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
	HandleFindData& data = *reinterpret_cast<HandleFindData*>(param);

	if (!obs_sceneitem_selected(item))
	{
		if (obs_sceneitem_is_group(item))
		{
			HandleFindData newData(data, item);
			newData.angleOffset = obs_sceneitem_get_rot(item);

			obs_sceneitem_group_enum_items(item, _FindHandleAtPos, &newData);

			data.item = newData.item;
			data.handle = newData.handle;
			data.angle = newData.angle;
			data.rotatePoint = newData.rotatePoint;
			data.offsetPoint = newData.offsetPoint;
		}


		return true;
	}


	matrix4 transform;
	vec3 pos3;
	float closestHandle = data.radius;

	vec3_set(&pos3, data.pos.x, data.pos.y, 0.0f);

	obs_sceneitem_get_box_transform(item, &transform);


	//-- make handle from data.pos --
	auto TestHandle = [&](float x, float y, ItemHandle handle) {
		vec3 handlePos = GetTransformedPos(x, y, transform);
		vec3_transform(&handlePos, &handlePos, &data.parent_xform);

		float dist = vec3_dist(&handlePos, &pos3);
		if (dist < data.radius) {
			if (dist < closestHandle) {
				closestHandle = dist;
				data.handle = handle;
				data.item = item;
			}
		}
		};

	TestHandle(0.0f, 0.0f, ItemHandle::TopLeft);
	TestHandle(0.5f, 0.0f, ItemHandle::TopCenter);
	TestHandle(1.0f, 0.0f, ItemHandle::TopRight);
	TestHandle(0.0f, 0.5f, ItemHandle::CenterLeft);
	TestHandle(1.0f, 0.5f, ItemHandle::CenterRight);
	TestHandle(0.0f, 1.0f, ItemHandle::BottomLeft);
	TestHandle(0.5f, 1.0f, ItemHandle::BottomCenter);
	TestHandle(1.0f, 1.0f, ItemHandle::BottomRight);
	//


	vec2 scale;
	obs_sceneitem_get_scale(item, &scale);
	obs_bounds_type boundsType = obs_sceneitem_get_bounds_type(item);
	vec2 rotHandleOffset;
	vec2_set(&rotHandleOffset, 0.0f,
		HANDLE_RADIUS * data.radius * 1.5 - data.radius);
	bool invertx = scale.x < 0.0f && boundsType == OBS_BOUNDS_NONE;
	float angle = atan2(invertx ? transform.x.y * -1.0f : transform.x.y,
		invertx ? transform.x.x * -1.0f : transform.x.x);
	RotatePos(&rotHandleOffset, angle);
	RotatePos(&rotHandleOffset, RAD(data.angleOffset));

	bool inverty = scale.y < 0.0f && boundsType == OBS_BOUNDS_NONE;
	vec3 handlePos =
		GetTransformedPos(0.5f, inverty ? 1.0f : 0.0f, transform);
	vec3_transform(&handlePos, &handlePos, &data.parent_xform);
	handlePos.x -= rotHandleOffset.x;
	handlePos.y -= rotHandleOffset.y;

	float dist = vec3_dist(&handlePos, &pos3);
	if (dist < data.radius) {
		if (dist < closestHandle) {
			closestHandle = dist;
			data.item = item;
			data.angle = obs_sceneitem_get_rot(item);
			data.handle = ItemHandle::Rot;

			vec2_set(&data.rotatePoint,
				transform.t.x + transform.x.x / 2 +
				transform.y.x / 2,
				transform.t.y + transform.x.y / 2 +
				transform.y.y / 2);

			obs_sceneitem_get_pos(item, &data.offsetPoint);
			data.offsetPoint.x -= data.rotatePoint.x;
			data.offsetPoint.y -= data.rotatePoint.y;

			RotatePos(&data.offsetPoint,
				-RAD(obs_sceneitem_get_rot(item)));
		}
	}

	return true;
}

bool AFModelPreview::_SelectOne(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
	obs_sceneitem_t* selectedItem = reinterpret_cast<obs_sceneitem_t*>(param);
	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, _SelectOne, param);

	obs_sceneitem_select(item, (selectedItem == item));

	return true;
}

bool AFModelPreview::_CheckItemSelected(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
	SceneFindData* data = reinterpret_cast<SceneFindData*>(param);
	matrix4 transform;
	vec3 transformedPos;
	vec3 pos3;

	if (!AFSceneUtil::SceneItemHasVideo(item))
		return true;


	if (obs_sceneitem_is_group(item))
	{
		data->group = item;
		obs_sceneitem_group_enum_items(item, _CheckItemSelected, param);
		data->group = nullptr;


		if (data->item)
			return false;
	}

	vec3_set(&pos3, data->pos.x, data->pos.y, 0.0f);

	obs_sceneitem_get_box_transform(item, &transform);

	if (data->group)
	{
		matrix4 parent_transform;
		obs_sceneitem_get_draw_transform(data->group,
                                         &parent_transform);
		matrix4_mul(&transform, &transform, &parent_transform);
	}

	matrix4_inv(&transform, &transform);
	vec3_transform(&transformedPos, &pos3, &transform);

	if (transformedPos.x >= 0.0f && transformedPos.x <= 1.0f &&
		transformedPos.y >= 0.0f && transformedPos.y <= 1.0f)
	{
		if (obs_sceneitem_selected(item))
		{
			data->item = item;
			return false;
		}
	}


	return true;
}

bool AFModelPreview::_MoveItems(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
	if (obs_sceneitem_locked(item))
		return true;

	bool selected = obs_sceneitem_selected(item);
	vec2* offset = reinterpret_cast<vec2*>(param);

	if (obs_sceneitem_is_group(item) && !selected) {
		matrix4 transform;
		vec3 new_offset;
		vec3_set(&new_offset, offset->x, offset->y, 0.0f);

		obs_sceneitem_get_draw_transform(item, &transform);
		vec4_set(&transform.t, 0.0f, 0.0f, 0.0f, 1.0f);
		matrix4_inv(&transform, &transform);
		vec3_transform(&new_offset, &new_offset, &transform);
		obs_sceneitem_group_enum_items(item, _MoveItems, &new_offset);
	}

	if (selected) {
		vec2 pos;
		obs_sceneitem_get_pos(item, &pos);
		vec2_add(&pos, &pos, offset);
		obs_sceneitem_set_pos(item, &pos);
	}

	return true;
}

bool AFModelPreview::_AddItemBounds(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param)
{
    SelectedItemBounds *data =
        reinterpret_cast<SelectedItemBounds *>(param);
    vec3 t[4];

    auto add_bounds = [data, &t]() {
        for (const vec3 &v : t)
        {
            if (data->first)
            {
                vec3_copy(&data->tl, &v);
                vec3_copy(&data->br, &v);
                data->first = false;
            }
            else
            {
                vec3_min(&data->tl, &data->tl, &v);
                vec3_max(&data->br, &data->br, &v);
            }
        }
    };

    if (obs_sceneitem_is_group(item))
    {
        SelectedItemBounds sib;
        obs_sceneitem_group_enum_items(item, _AddItemBounds, &sib);

        if (!sib.first)
        {
            matrix4 xform;
            obs_sceneitem_get_draw_transform(item, &xform);

            vec3_set(&t[0], sib.tl.x, sib.tl.y, 0.0f);
            vec3_set(&t[1], sib.tl.x, sib.br.y, 0.0f);
            vec3_set(&t[2], sib.br.x, sib.tl.y, 0.0f);
            vec3_set(&t[3], sib.br.x, sib.br.y, 0.0f);
            vec3_transform(&t[0], &t[0], &xform);
            vec3_transform(&t[1], &t[1], &xform);
            vec3_transform(&t[2], &t[2], &xform);
            vec3_transform(&t[3], &t[3], &xform);
            add_bounds();
        }
    }
    if (!obs_sceneitem_selected(item))
        return true;

    matrix4 boxTransform;
    obs_sceneitem_get_box_transform(item, &boxTransform);

    t[0] = GetTransformedPos(0.0f, 0.0f, boxTransform);
    t[1] = GetTransformedPos(1.0f, 0.0f, boxTransform);
    t[2] = GetTransformedPos(0.0f, 1.0f, boxTransform);
    t[3] = GetTransformedPos(1.0f, 1.0f, boxTransform);
    add_bounds();

    return true;
}

bool AFModelPreview::_GetSourceSnapOffset(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param)
{
    OffsetData *data = reinterpret_cast<OffsetData *>(param);

    if (obs_sceneitem_selected(item))
        return true;

    matrix4 boxTransform;
    obs_sceneitem_get_box_transform(item, &boxTransform);

    vec3 t[4] = {GetTransformedPos(0.0f, 0.0f, boxTransform),
             GetTransformedPos(1.0f, 0.0f, boxTransform),
             GetTransformedPos(0.0f, 1.0f, boxTransform),
             GetTransformedPos(1.0f, 1.0f, boxTransform)};

    bool first = true;
    vec3 tl, br;
    vec3_zero(&tl);
    vec3_zero(&br);
    for (const vec3 &v : t) {
        if (first) {
            vec3_copy(&tl, &v);
            vec3_copy(&br, &v);
            first = false;
        } else {
            vec3_min(&tl, &tl, &v);
            vec3_max(&br, &br, &v);
        }
    }

    // Snap to other source edges
#define EDGE_SNAP(l, r, x, y)                                               \
    do {                                                                \
        double dist = fabsf(l.x - data->r.x);                       \
        if (dist < data->clampDist &&                               \
            fabsf(data->offset.x) < EPSILON && data->tl.y < br.y && \
            data->br.y > tl.y &&                                    \
            (fabsf(data->offset.x) > dist ||                        \
             data->offset.x < EPSILON))                             \
            data->offset.x = l.x - data->r.x;                   \
    } while (false)

    EDGE_SNAP(tl, br, x, y);
    EDGE_SNAP(tl, br, y, x);
    EDGE_SNAP(br, tl, x, y);
    EDGE_SNAP(br, tl, y, x);
#undef EDGE_SNAP

    return true;
}

bool AFModelPreview::_IntersectBox(matrix4 transform,
                                  float x1, float x2,
                                  float y1, float y2)
{
    auto CounterClockwise = [](float x1, float x2, 
                               float x3, float y1,
                               float y2, float y3) -> bool {
        return (y3 - y1) * (x2 - x1) > (y2 - y1) * (x3 - x1);
    };
    
    auto IntersectLine = [CounterClockwise](float x1, float x2, float x3, float x4,
                            float y1, float y2, float y3, float y4) -> bool {
        bool a = CounterClockwise(x1, x2, x3, y1, y2, y3);
        bool b = CounterClockwise(x1, x2, x4, y1, y2, y4);
        bool c = CounterClockwise(x3, x4, x1, y3, y4, y1);
        bool d = CounterClockwise(x3, x4, x2, y3, y4, y2);

        return (a != b) && (c != d);
    };
    
    
    float x3, x4, y3, y4;

    x3 = transform.t.x;
    y3 = transform.t.y;
    x4 = x3 + transform.x.x;
    y4 = y3 + transform.x.y;

    if (IntersectLine(x1, x1, x3, x4, y1, y2, y3, y4) ||
        IntersectLine(x1, x2, x3, x4, y1, y1, y3, y4) ||
        IntersectLine(x2, x2, x3, x4, y1, y2, y3, y4) ||
        IntersectLine(x1, x2, x3, x4, y2, y2, y3, y4))
        return true;

    x4 = x3 + transform.y.x;
    y4 = y3 + transform.y.y;

    if (IntersectLine(x1, x1, x3, x4, y1, y2, y3, y4) ||
        IntersectLine(x1, x2, x3, x4, y1, y1, y3, y4) ||
        IntersectLine(x2, x2, x3, x4, y1, y2, y3, y4) ||
        IntersectLine(x1, x2, x3, x4, y2, y2, y3, y4))
        return true;

    x3 = transform.t.x + transform.x.x;
    y3 = transform.t.y + transform.x.y;
    x4 = x3 + transform.y.x;
    y4 = y3 + transform.y.y;

    if (IntersectLine(x1, x1, x3, x4, y1, y2, y3, y4) ||
        IntersectLine(x1, x2, x3, x4, y1, y1, y3, y4) ||
        IntersectLine(x2, x2, x3, x4, y1, y2, y3, y4) ||
        IntersectLine(x1, x2, x3, x4, y2, y2, y3, y4))
        return true;

    x3 = transform.t.x + transform.y.x;
    y3 = transform.t.y + transform.y.y;
    x4 = x3 + transform.x.x;
    y4 = y3 + transform.x.y;

    if (IntersectLine(x1, x1, x3, x4, y1, y2, y3, y4) ||
        IntersectLine(x1, x2, x3, x4, y1, y1, y3, y4) ||
        IntersectLine(x2, x2, x3, x4, y1, y2, y3, y4) ||
        IntersectLine(x1, x2, x3, x4, y2, y2, y3, y4))
        return true;

    return false;
}

void AFModelPreview::_SnapItemMovement(vec2& offset)
{
    if (m_pInitedContextGraphics == nullptr ||
        m_pInitedContextScene == nullptr)
        return;
    
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    OBSScene scene = m_pInitedContextScene->GetCurrOBSScene();
    
    SelectedItemBounds data;
    obs_scene_enum_items(scene, _AddItemBounds, &data);

    data.tl.x += offset.x;
    data.tl.y += offset.y;
    data.br.x += offset.x;
    data.br.y += offset.y;

    vec3 snapOffset = _GetSnapOffset(data.tl, data.br);

    const bool snap = config_get_bool(confManager.GetGlobal(), "BasicWindow",
                                      "SnappingEnabled");
    const bool sourcesSnap = config_get_bool(confManager.GetGlobal(),
                                             "BasicWindow", "SourceSnapping");
    if (snap == false)
        return;
    if (sourcesSnap == false)
    {
        offset.x += snapOffset.x;
        offset.y += snapOffset.y;
        return;
    }

    const float clampDist = config_get_double(confManager.GetGlobal(),
                                              "BasicWindow",
                                              "SnapDistance") /
                                                m_pInitedContextGraphics->GetMainPreviewScale();

    OffsetData offsetData;
    offsetData.clampDist = clampDist;
    offsetData.tl = data.tl;
    offsetData.br = data.br;
    vec3_copy(&offsetData.offset, &snapOffset);

    obs_scene_enum_items(scene, _GetSourceSnapOffset, &offsetData);

    if (fabsf(offsetData.offset.x) > EPSILON ||
        fabsf(offsetData.offset.y) > EPSILON) {
        offset.x += offsetData.offset.x;
        offset.y += offsetData.offset.y;
    } else {
        offset.x += snapOffset.x;
        offset.y += snapOffset.y;
    }
}

vec3 AFModelPreview::_GetSnapOffset(const vec3& tl, const vec3& br)
{
    vec3 clampOffset;
    vec3_zero(&clampOffset);

    if (m_pInitedContextGraphics == nullptr)
        return clampOffset;
    
    auto& confManager = AFConfigManager::GetSingletonInstance();
 
    vec2 screenSize;
    vec2_zero(&screenSize);

    obs_video_info ovi;
    if (obs_get_video_info(&ovi))
    {
        screenSize.x = float(ovi.base_width);
        screenSize.y = float(ovi.base_height);
    }
    
    const bool snap = config_get_bool(confManager.GetGlobal(), "BasicWindow",
                                      "SnappingEnabled");
    if (snap == false)
        return clampOffset;

    const bool screenSnap = config_get_bool(confManager.GetGlobal(),
                                            "BasicWindow", "ScreenSnapping");
    const bool centerSnap = config_get_bool(confManager.GetGlobal(),
                                            "BasicWindow", "CenterSnapping");

    const float clampDist = config_get_double(confManager.GetGlobal(),
                                              "BasicWindow",
                                              "SnapDistance") /
                                                m_pInitedContextGraphics->GetMainPreviewScale();
    const float centerX = br.x - (br.x - tl.x) / 2.0f;
    const float centerY = br.y - (br.y - tl.y) / 2.0f;

    // Left screen edge.
    if (screenSnap && fabsf(tl.x) < clampDist)
        clampOffset.x = -tl.x;
    // Right screen edge.
    if (screenSnap && fabsf(clampOffset.x) < EPSILON &&
        fabsf(screenSize.x - br.x) < clampDist)
        clampOffset.x = screenSize.x - br.x;
    // Horizontal center.
    if (centerSnap && fabsf(screenSize.x - (br.x - tl.x)) > clampDist &&
        fabsf(screenSize.x / 2.0f - centerX) < clampDist)
        clampOffset.x = screenSize.x / 2.0f - centerX;

    // Top screen edge.
    if (screenSnap && fabsf(tl.y) < clampDist)
        clampOffset.y = -tl.y;
    // Bottom screen edge.
    if (screenSnap && fabsf(clampOffset.y) < EPSILON &&
        fabsf(screenSize.y - br.y) < clampDist)
        clampOffset.y = screenSize.y - br.y;
    // Vertical center.
    if (centerSnap && fabsf(screenSize.y - (br.y - tl.y)) > clampDist &&
        fabsf(screenSize.y / 2.0f - centerY) < clampDist)
        clampOffset.y = screenSize.y / 2.0f - centerY;

    return clampOffset;
}

void AFModelPreview::_SnapStretchingToScreen(vec3& tl, vec3& br, uint32_t stretchFlags)
{
    vec3 newTL = GetTransformedPos(tl.x, tl.y, m_matItemToScreen);
    vec3 newTR = GetTransformedPos(br.x, tl.y, m_matItemToScreen);
    vec3 newBL = GetTransformedPos(tl.x, br.y, m_matItemToScreen);
    vec3 newBR = GetTransformedPos(br.x, br.y, m_matItemToScreen);
    vec3 boundingTL;
    vec3 boundingBR;

    vec3_copy(&boundingTL, &newTL);
    vec3_min(&boundingTL, &boundingTL, &newTR);
    vec3_min(&boundingTL, &boundingTL, &newBL);
    vec3_min(&boundingTL, &boundingTL, &newBR);

    vec3_copy(&boundingBR, &newTL);
    vec3_max(&boundingBR, &boundingBR, &newTR);
    vec3_max(&boundingBR, &boundingBR, &newBL);
    vec3_max(&boundingBR, &boundingBR, &newBR);

    vec3 offset = _GetSnapOffset(boundingTL, boundingBR);
    vec3_add(&offset, &offset, &newTL);
    vec3_transform(&offset, &offset, &m_matScreenToItem);
    vec3_sub(&offset, &offset, &tl);

    if (stretchFlags & ITEM_LEFT)
        tl.x += offset.x;
    else if (stretchFlags & ITEM_RIGHT)
        br.x += offset.x;

    if (stretchFlags & ITEM_TOP)
        tl.y += offset.y;
    else if (stretchFlags & ITEM_BOTTOM)
        br.y += offset.y;
}

void AFModelPreview::_ClampAspect(vec3& tl, vec3& br, vec2& size, const vec2& baseSize,
								  AFMouseStaterPreview& mouseState)
{
	float baseAspect = baseSize.x / baseSize.y;
	float aspect = size.x / size.y;
	ItemHandle stretchHandle = mouseState.GetCurrStateHandle();
	uint32_t stretchFlags = (uint32_t)stretchHandle;

	if (stretchHandle == ItemHandle::TopLeft ||
		stretchHandle == ItemHandle::TopRight ||
		stretchHandle == ItemHandle::BottomLeft ||
		stretchHandle == ItemHandle::BottomRight) {
		if (aspect < baseAspect) {
			if ((size.y >= 0.0f && size.x >= 0.0f) ||
				(size.y <= 0.0f && size.x <= 0.0f))
				size.x = size.y * baseAspect;
			else
				size.x = size.y * baseAspect * -1.0f;
		}
		else {
			if ((size.y >= 0.0f && size.x >= 0.0f) ||
				(size.y <= 0.0f && size.x <= 0.0f))
				size.y = size.x / baseAspect;
			else
				size.y = size.x / baseAspect * -1.0f;
		}

	}
	else if (stretchHandle == ItemHandle::TopCenter ||
		stretchHandle == ItemHandle::BottomCenter) {
		if ((size.y >= 0.0f && size.x >= 0.0f) ||
			(size.y <= 0.0f && size.x <= 0.0f))
			size.x = size.y * baseAspect;
		else
			size.x = size.y * baseAspect * -1.0f;

	}
	else if (stretchHandle == ItemHandle::CenterLeft ||
		stretchHandle == ItemHandle::CenterRight) {
		if ((size.y >= 0.0f && size.x >= 0.0f) ||
			(size.y <= 0.0f && size.x <= 0.0f))
			size.y = size.x / baseAspect;
		else
			size.y = size.x / baseAspect * -1.0f;
	}

	size.x = std::round(size.x);
	size.y = std::round(size.y);

	if (stretchFlags & ITEM_LEFT)
		tl.x = br.x - size.x;
	else if (stretchFlags & ITEM_RIGHT)
		br.x = tl.x + size.x;

	if (stretchFlags & ITEM_TOP)
		tl.y = br.y - size.y;
	else if (stretchFlags & ITEM_BOTTOM)
		br.y = tl.y + size.y;
}

vec3 AFModelPreview::_CalculateStretchPos(const vec3& tl, const vec3& br)
{
	uint32_t alignment = obs_sceneitem_get_alignment(m_obsStretchItem);
	vec3 pos;

	vec3_zero(&pos);

	if (alignment & OBS_ALIGN_LEFT)
		pos.x = tl.x;
	else if (alignment & OBS_ALIGN_RIGHT)
		pos.x = br.x;
	else
		pos.x = (br.x - tl.x) * 0.5f + tl.x;

	if (alignment & OBS_ALIGN_TOP)
		pos.y = tl.y;
	else if (alignment & OBS_ALIGN_BOTTOM)
		pos.y = br.y;
	else
		pos.y = (br.y - tl.y) * 0.5f + tl.y;

	return pos;
}
