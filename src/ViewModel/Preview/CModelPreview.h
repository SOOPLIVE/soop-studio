#pragma once

#include <mutex>
#include <vector>

#include <obs.hpp>
#include <graphics/vec2.h>
#include <graphics/matrix4.h>


#include "CMouseStaterPreview.h"


struct SceneFindData
{
	const vec2& pos;
	OBSSceneItem item;
	bool selectBelow;

	obs_sceneitem_t* group = nullptr;

	SceneFindData(const SceneFindData&) = delete;
	SceneFindData(SceneFindData&&) = delete;
	SceneFindData& operator=(const SceneFindData&) = delete;
	SceneFindData& operator=(SceneFindData&&) = delete;

	inline SceneFindData(const vec2& pos_, bool selectBelow_)
		: pos(pos_),
		selectBelow(selectBelow_)
	{
	}
};

struct SceneFindBoxData
{
	const vec2& startPos;
	const vec2& pos;
	std::vector<obs_sceneitem_t*> sceneItems;

	SceneFindBoxData(const SceneFindData&) = delete;
	SceneFindBoxData(SceneFindData&&) = delete;
	SceneFindBoxData& operator=(const SceneFindData&) = delete;
	SceneFindBoxData& operator=(SceneFindData&&) = delete;

	inline SceneFindBoxData(const vec2& startPos_, const vec2& pos_)
		: startPos(startPos_),
		pos(pos_)
	{
	}
};


class AFGraphicsContext;
class AFSceneContext;


class AFModelPreview final
{
#pragma region QT Field, CTOR/DTOR
public:
	AFModelPreview() = default;
	~AFModelPreview() = default;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	// callback for libobs
	static bool		FindSelected(obs_scene_t* scene, obs_sceneitem_t* item, void* param);
    static bool     FindItemsInBox(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
    static bool     NudgeCallBack(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
	//


	static vec2		GetItemSize(obs_sceneitem_t* item);


	void			SetUnsafeAccessContext(AFGraphicsContext* pGraphicsContext,
										   AFSceneContext* pSceneContext);

    void            ClearSelectedItems();
    void            SetSelectedItems();
    void            ClearHoveredItems(bool selectionBox);
    void            EnumSelecedHoveredItems(const bool altDown,
                                            const bool shiftDown,
                                            const bool ctrlDown);
    uint32_t        MakeHoveredItem(const vec2& pos);
    void            MakeLastHoveredItem(const vec2& pos);

    void			Reset();
	bool			IsLocked();
	void			GetStretchHandleData(const vec2& pos, bool ignoreGroup,
                                         AFMouseStaterPreview& mouseState, float dpiValue = 1.f);

	OBSSceneItem	GetItemAtPos(const vec2& pos, bool selectBelow);
	bool			SelectedAtPos(const vec2& pos);
    void            DoSelect(const vec2& pos);
	void			DoCtrlSelect(const vec2& pos);

    void            CropItem(const vec2& pos, AFMouseStaterPreview& mouseState);
	void			StretchItem(const vec2& pos, AFMouseStaterPreview& mouseState,
                                bool shiftDown, bool controlDown);
    void            RotateItem(const vec2& pos, bool shiftDown, bool controlDown);
	void			MoveItems(const vec2& pos, vec2& lastMoveOffset, vec2& startPos, bool controlDown);
    void            BoxItems(OBSScene scene, const vec2 &startPos, const vec2 &pos);
    
    
    bool            CheckNowHovered(obs_sceneitem_t* item);
#pragma endregion public func

#pragma region private func
private:
	// callback for libobs
	static bool		_FindItemAtPos(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
	static bool		_FindHandleAtPos(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);

	static bool		_SelectOne(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
	static bool		_CheckItemSelected(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
	static bool		_MoveItems(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
    static bool     _AddItemBounds(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
    static bool     _GetSourceSnapOffset(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
	//

    
    static bool     _IntersectBox(matrix4 transform, 
                                  float x1, float x2,
                                  float y1, float y2);
    
    void            _SnapItemMovement(vec2 &offset);
    vec3            _GetSnapOffset(const vec3& tl, const vec3& br);
    void            _SnapStretchingToScreen(vec3& tl, vec3& br, uint32_t stretchFlags);
	void			_ClampAspect(vec3& tl, vec3& br, vec2& size, const vec2& baseSize,
								 AFMouseStaterPreview& mouseState);
	vec3			_CalculateStretchPos(const vec3& tl, const vec3& br);
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
	// Fast Access Context
	AFGraphicsContext*		            m_pInitedContextGraphics = nullptr;
	AFSceneContext*			            m_pInitedContextScene = nullptr;
	//
            
                
    obs_sceneitem_crop                  m_obsStartCrop;
    vec2                                m_vec2StartItemPos;
    vec2                                m_vec2CropSize;
            
	OBSSceneItem			            m_obsStretchItem;   // Current Stretch Item
    OBSSceneItem                        m_obsStretchGroup;
            
    float                               m_fRotateAngle;
    vec2                                m_vec2RotatePoint;
    vec2                                m_vec2OffsetPoint;
	vec2					            m_vec2StretchSize;
            
	matrix4					            m_matScreenToItem;
	matrix4					            m_matItemToScreen;
    matrix4                             m_matInvGroupTransform;
    
    
    std::vector<obs_sceneitem_t *>      m_vecHoveredPreviewItems;
    std::vector<obs_sceneitem_t *>      m_vecSelectedItems;
    std::mutex                          m_SelectMutex;
#pragma endregion private member var
};
