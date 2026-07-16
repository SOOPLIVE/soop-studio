#pragma once

#include <atomic>
#include <vector>

#include "obs.hpp"

#include "Blocks/SceneSourceDock/CSceneListItem.h"
#include "Blocks/AudioMixerDock/CVolumeControl.h"

class AFQSourceListView;

// Forward
struct AddSourceData {
    obs_source_t* source = nullptr;
    bool visible = false;
    obs_transform_info* transform = nullptr;
    obs_sceneitem_crop* crop = nullptr;
    obs_blending_method* blend_method = nullptr;
    obs_blending_type* blend_mode = nullptr;
};

using SceneItemVector  = std::vector<AFQSceneListItem*>;
using VolControlVector = std::vector<AFQVolControl*>;

class AFSceneContext final
{
public:
    AFSceneContext() = default;
    ~AFSceneContext() { currentScene = nullptr; }

public:
    // callback for libobs
    static void AddSource(void* _data, obs_scene* scene);
    //

    void InitContext();
    bool FinContext();
    void InitSourceSignalCallback();
    void ClearSourceSignalCallback();
         
    void ClearSceneListButtonList();
    void ClearSceneTransitionList();
    void ClearVolControlList();

    // !! locate obs - window-basic-main.hpp 423 line - [34ef67e]
    void ClearContext();
    // !!

    OBSSource GetProgramSource();
    OBSScene GetCurrentScene();
    OBSWeakSource GetProgramScene();
    OBSWeakSource GetLastScene();
    OBSWeakSource GetSwapScene();

    inline OBSSource GetCurrentSceneSource()
    {
        OBSScene curScene = GetCurrentScene();
        return OBSSource(obs_scene_get_source(curScene));
    }

    void SetProgramScene(obs_source_t* obsSource);
    void SetCurrentScene(obs_scene_t* scene);
    void SetLastScene(obs_source_t* obsSource);
    void SetSwapScene(OBSWeakSource obsSource);

    bool IsMustInSizePreview(obs_source_t* source);

    std::vector<OBSSource> GetTransitions() { return m_obsTransitions; }
    std::vector<OBSSource>& GetRefTransitions() { return m_obsTransitions; }
    OBSSource GetCurTransition() { return m_curTransition; }
    void SetCurTransition(OBSSource source);
    int GetCurDuraition() { return m_transDuration; }
    void SetCurDuration(int duration);
    void AddTransition(OBSSource source);
    void RemoveTransition(OBSSource source);

    // [Transition]
    // !! window-basic-main-transition.cpp 54 line - [34ef67e]
    void InitDefaultTransition();

    void TransitionToScene(OBSSource source, bool force = false,
                           bool quickTransition = false,
                           int quickDuration = 0, bool black = false,
                           bool manual = false);

    int GetOverrideTransitionDuration(OBSSource source);
    void OverrideTransition(OBSSource transition);

    void SetTransition(OBSSource transition);
         
    void InitTransition(obs_source_t* transition);
    obs_source_t* FindTransition(const char* name);
                  
    obs_source_t* GetFadeTransition() { return m_pFadeTransition; };
    obs_source_t* GetCutTransition() { return m_pCutTransition; };
    // !!

    // [Scene ListView UI]
    AFQSourceListView* GetSourceListViewPtr();
    void SetSourceListViewPtr(AFQSourceListView* listview);

    SceneItemVector& GetSceneItemVector();
    size_t GetSceneItemSize();
    AFQSceneListItem* GetCurSelectedSceneItem();
    OBSSceneItem GetCurrentOBSSceneItem(int idx = -1);
    void SetCurSelectedSceneItem(AFQSceneListItem* sceneItem);
    void AddSceneItem(AFQSceneListItem* sceneItem);
    void SwapSceneItem(int from, int dest);

    int GetFavoriteSceneCount();
    const int GetFavoriteSceneMaxCount();

    VolControlVector& GetVolControlVector();

    void SetMixerCopyFilter(obs_source_t* source);
    OBSSourceAutoRelease GetMixerCopyFilter();

    obs_transform_info& GetTransformInfo() { return m_copiedTransformInfo; }
    obs_sceneitem_crop& GetCropInfo() { return m_copiedCropInfo; }

    static void UpdateVideoSize(int width, int height);

private:
    void _Clear();

public:
    // !! locate obs - window-basic-main.hpp 246 line - [34ef67e]
    OBSWeakSourceAutoRelease m_obsCopyFiltersSource;

    // !! locate obs - window-basic-main.hpp 1015 line - [34ef67e]
    OBSWeakSource m_copyFilter;

private:
    // [Scene ListView UI]
    AFQSourceListView* m_pSourceListViewPtr = nullptr;
    SceneItemVector m_vecSceneItem;
    std::atomic<AFQSceneListItem*> m_clickedSceneItem = nullptr;

    // !! locate obs - window-basic-main.hpp 361 line - [34ef67e]
    std::atomic<obs_scene_t*> currentScene = nullptr;
    // !!

    // !! locate obs - window-basic-main.hpp 509 line - [34ef67e]
    OBSWeakSource lastScene = nullptr;
    OBSWeakSource swapScene = nullptr;
    OBSWeakSource programScene = nullptr;
    OBSWeakSource lastProgramScene = nullptr;
    // !!

    // !! window-basic-main-transition.cpp 56 line - [34ef67e]
    std::vector<OBSSource> m_obsTransitions;
    OBSSource m_curTransition;
    int m_transDuration = 300;
    // !! window-basic-main.hpp 470 line - [34ef67e]
    obs_source_t* m_pFadeTransition = nullptr;
    obs_source_t* m_pCutTransition = nullptr;
    // !!

    OBSSource m_prevFTBSource = nullptr;

    VolControlVector m_vecVolControl;
    OBSWeakSourceAutoRelease m_mixerCopyFiltersSource = nullptr;

    std::vector<OBSSignal> m_signalHandlers;

    obs_transform_info m_copiedTransformInfo = {0,};
    obs_sceneitem_crop m_copiedCropInfo = {0,};
};
//
namespace AFSceneUtil
{
    OBSSource CnvtToOBSSource(OBSScene scene);
    OBSScene CnvtToOBSScene(OBSSource source);

    bool SceneItemHasVideo(obs_sceneitem_t* item);
    bool IsCropEnabled(const obs_sceneitem_crop* crop);

    obs_source_t* CreateOBSScene(const char* name);
}