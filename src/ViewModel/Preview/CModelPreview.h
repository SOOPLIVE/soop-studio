#pragma once

#include <mutex>
#include <vector>

#include <obs.hpp>
#include <graphics/vec2.h>
#include <graphics/matrix4.h>

#include "CMouseStaterPreview.h"

class AFGraphicsContext;

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

struct PendingBrowserSizeUpdate {
	obs_source_t* source;
	int width;
	int height;
};

class CModelPreview final
{
public:
	CModelPreview();// = default;
	~CModelPreview();// = default;

public:
	// callback for libobs
	static bool FindSelected(obs_scene_t* scene, obs_sceneitem_t* item, void* param);
    static bool FindItemsInBox(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
    static bool NudgeCallBack(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
	//

	static vec2 GetItemSize(obs_sceneitem_t* item);

    void ClearSelectedItems();
    void SetSelectedItems();
    void ClearHoveredItems(bool selectionBox);
    void EnumSelecedHoveredItems(const bool altDown,
                                            const bool shiftDown,
                                            const bool ctrlDown);
    uint32_t MakeHoveredItem(const vec2& pos);
    void MakeLastHoveredItem(const vec2& pos);

    void Reset();
	bool IsLocked();
	void GetStretchHandleData(const vec2& pos, bool ignoreGroup,
                                         CMouseStatePreview& mouseState, float dpiValue = 1.f);

	OBSSceneItem GetItemAtPos(const vec2& pos, bool selectBelow);
	bool SelectedAtPos(const vec2& pos);
    void DoSelect(const vec2& pos);
	void DoCtrlSelect(const vec2& pos);

    void CropItem(const vec2& pos, CMouseStatePreview& mouseState);
	void StretchItem(const vec2& pos, CMouseStatePreview& mouseState,
                                bool shiftDown, bool controlDown);
    void RotateItem(const vec2& pos, bool shiftDown, bool controlDown);
	void MoveItems(const vec2& pos, vec2& lastMoveOffset, vec2& startPos, bool controlDown);
    void BoxItems(OBSScene scene, const vec2 &startPos, const vec2 &pos);
     
    bool CheckNowHovered(obs_sceneitem_t* item);

private:
	// callback for libobs
	static bool FindItemAtPos(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
	static bool FindHandleAtPos(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);

	static bool SelectOne(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
	static bool CheckItemSelected(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
	static bool MoveItems(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
    static bool AddItemBounds(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
    static bool GetSourceSnapOffset(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param);
	//
  
    static bool IntersectBox(matrix4 transform, 
                                float x1, float x2,
                                float y1, float y2);
    
	static vec2 AdjustScreenInItems(obs_sceneitem_t* item, vec2 pos);

    void SnapItemMovement(vec2 &offset);
    vec3 GetSnapOffset(const vec3& tl, const vec3& br);
    void SnapStretchingToScreen(vec3& tl, vec3& br, uint32_t stretchFlags);
	void ClampAspect(vec3& tl, vec3& br, vec2& size, const vec2& baseSize,
							CMouseStatePreview& mouseState);
	vec3 CalculateStretchPos(const vec3& tl, const vec3& br);

	void StartBrowserResizeThread();
	void ProcessPendingBrowserSize();

private:       
    obs_sceneitem_crop startCrop;
    vec2 startItemPos;
    vec2 cropSize;
            
	OBSSceneItem stretchItem;
    OBSSceneItem stretchGroup;
            
    float rotateAngle;
    vec2 rotatePoint;
    vec2 offsetPoint;
	vec2 stretchItemSize;
	matrix4 screenToItem;
	matrix4 itemToScreen;
    matrix4 invGroupTransform;
    
    std::vector<obs_sceneitem_t *> hoveredPreviewItems;
    std::vector<obs_sceneitem_t *> selectedItems;
    std::mutex selectMutex;

	// Update Browser Source Size
	std::mutex queueBrowserSizeMutex;
	std::shared_ptr<PendingBrowserSizeUpdate> pendingBrowserSizeUpdate;
	std::thread browserUpdateThread;
	std::atomic<bool> workBrowserThread = false;

};
