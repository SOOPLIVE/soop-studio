#pragma once

#include <atomic>
#include <vector>

#include "obs.hpp"

#include "Blocks/SceneSourceDock/CSceneListItem.h"

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

using SceneItemVector = std::vector<AFQSceneListItem*>;

class AFSceneContext final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFSceneContext& GetSingletonInstance()
    {
        static AFSceneContext* instance = nullptr;
        if (instance == nullptr)
            instance = new AFSceneContext;
        return *instance;
    };
    ~AFSceneContext();
private:
    // singleton constructor
    AFSceneContext() = default;    
    AFSceneContext(const AFSceneContext&) = delete;
    AFSceneContext(/* rValue */AFSceneContext&& other) noexcept = delete;
    AFSceneContext& operator=(const AFSceneContext&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    // callback for libobs
    static void                         AddSource(void* _data, obs_scene* scene);
    //

    void                                InitContext();
    bool                                FinContext();
    void                                LoadScene();
    void                                ClearContext();
    // !!
    
    OBSScene                            GetCurrOBSScene() { return m_obsCurrScene.load(); };
    void                                SetCurrOBSScene(obs_scene_t* scene);
    OBSWeakSource                       GetLastOBSScene() { return m_obsLastScene; };
    void                                SetLastOBSScene(obs_source_t* obsSource);
    OBSWeakSource                       GetSwapOBSScene() { return m_obsSwapScene; };
    void                                SetSwapOBSScene(obs_source_t* obsSource);
    void                                SetSwapOBSScene(OBSWeakSource obsSource);
    OBSWeakSource                       GetProgramOBSScene() { return m_obsProgramScene; };
    void                                SetProgramOBSScene(obs_source_t* obsSource);
    OBSWeakSource                       GetLastProgramOBSScene() { return m_obsLastProgramScene; };
    void                                SetLastProgramOBSScene(obs_source_t* obsSource);
    void                                SetLastProgramOBSScene(OBSWeakSource obsSource);
    //

    OBSSource                           GetCurrOBSSceneSource() { return OBSSource(obs_scene_get_source(GetCurrOBSScene())); }


    //
    std::vector<OBSSource>              GetTransitions() { return m_obsTransitions; }
    std::vector<OBSSource>&             GetRefTransitions() { return m_obsTransitions; }
    OBSSource                           GetCurTransition() { return m_curTransition; }
    void                                SetCurTransition(OBSSource source);
    int                                 GetCurDuraition() { return m_transDuration; }
    void                                SetCurDuration(int duration);
    void                                AddTransition(OBSSource source);
    void                                RemoveTransition(OBSSource source);

    // [Transition]
    void                                InitDefaultTransition();

    void                                InitTransition(obs_source_t* transition);
    obs_source_t*                       FindTransition(const char* name);

    obs_source_t*                       GetFadeTransition() { return m_fadeTransition; };
    obs_source_t*                       GetCutTransition() { return m_cutTransition; };
    // !!

    // [Scene ListView UI]
    AFQSourceListView*                  GetSourceListViewPtr();
    void                                SetSourceListViewPtr(AFQSourceListView* listview);

    SceneItemVector&                    GetSceneItemVector();
    size_t                              GetSceneItemSize();
    AFQSceneListItem*                   GetCurSelectedSceneItem();
    void                                SetCurSelectedSceneItem(AFQSceneListItem* sceneItem);
    void                                AddSceneItem(AFQSceneListItem* sceneItem);
    void                                SwapSceneItem(int from, int dest);

#pragma endregion public func

#pragma region private func
private:
    void                                _Clear();

#pragma endregion private func

#pragma region public member var
public:
    OBSWeakSourceAutoRelease            m_obsCopyFiltersSource;

    OBSWeakSource                       m_copyFilter;

    obs_transform_info                  m_copiedTransformInfo;
    obs_sceneitem_crop                  m_copiedCropInfo;

    // !!
#pragma endregion public member var
#pragma region private member var
private:
    // [Scene ListView UI]
    AFQSourceListView*                  m_sourceListViewPtr = nullptr;
    SceneItemVector                     m_vecSceneItem;
    std::atomic<AFQSceneListItem*> 	    m_clickedSceneItem = nullptr;

    std::atomic<obs_scene_t*>           m_obsCurrScene = nullptr;
    // !!

    OBSWeakSource                       m_obsLastScene = nullptr;
    OBSWeakSource                       m_obsSwapScene = nullptr;
    OBSWeakSource                       m_obsProgramScene = nullptr;
    OBSWeakSource                       m_obsLastProgramScene = nullptr;
    // !!

    std::vector<OBSSource>              m_obsTransitions;
    OBSSource                           m_curTransition;
    int                                 m_transDuration = 300;
    obs_source_t*                       m_fadeTransition = nullptr;
    obs_source_t*                       m_cutTransition = nullptr;
    // !!

    
    OBSSource                           m_prevFTBSource = nullptr;

#pragma endregion private member var
};