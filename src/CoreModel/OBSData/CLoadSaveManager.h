#pragma once

#include <vector>
#include <functional>
#include <atomic>

#include <obs.hpp>


class AFMainFrame;

class AFLoadSaveManager final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFLoadSaveManager& GetSingletonInstance()
    {
        static AFLoadSaveManager* instance = nullptr;
        if (instance == nullptr)
            instance = new AFLoadSaveManager;
        return *instance;
    };
    ~AFLoadSaveManager() {};
private:
    // singleton constructor
    AFLoadSaveManager() = default;
    AFLoadSaveManager(const AFLoadSaveManager&) = delete;
    AFLoadSaveManager(/* rValue */AFLoadSaveManager&& other) noexcept = delete;
    AFLoadSaveManager& operator=(const AFLoadSaveManager&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    static void                         EnumProfiles(std::function<bool(const char *, const char *)> &&cb);
    static bool                         GetProfileDir(const char* findName, const char*& profileDir);
    static bool                         ProfileExists(const char* findName);
    
    static void                         EnumSceneCollections(std::function<bool(const char *, const char *)> &&cb);
    static bool                         SceneCollectionExists(const char* findName);
    
    
    
    static inline void                  AddMissingFiles(void *data, obs_source_t *source);

    inline                              void IncreaseCheckSaveCnt() {
                                            long tmpValue = m_DisableSaving;
                                            ++tmpValue;
                                            m_DisableSaving.store(tmpValue);
                                        };
    inline void                         DecreaseCheckSaveCnt() {
                                            long tmpValue = m_DisableSaving;
                                            --tmpValue;
                                            m_DisableSaving.store(tmpValue);
                                        };
    
    void                                SetUIObjMainFrame(AFMainFrame* pMain) { m_pMainUI = pMain; };
    
    bool                                Load(const char* file);
    
    
    //obs_data_array_t*                   SaveProjectors();
    inline bool                         CheckDisableSaving() { return m_DisableSaving.load(); };
    inline bool                         CheckCanSaveProject();
    void                                ForceSaveProjectNow();
    void                                SaveProjectNow();
    void                                SaveProjectDeferred();
#pragma endregion public func

#pragma region private func
private:
    void                                _LoadTransitions(obs_data_array_t *transitions,
                                                         obs_load_source_cb cb, void *private_data);
    void                                _LoadSceneListOrder(obs_data_array_t *array);

    void                                _LoadData(obs_data_t* datat, const char* file);
    
    
    void                                _SaveAudioDevice(const char *name, int channel, obs_data_t *parent,
                                                         std::vector<OBSSource> &audioSources);
    obs_data_t*                         _GenerateSaveData(obs_data_array_t* sceneOrder,
                                                          obs_data_array_t* quickTransitionData,
                                                          int transitionDuration,
                                                          obs_data_array_t* transitions,
                                                          OBSScene& scene, OBSSource& curProgramScene,
                                                          obs_data_array_t* savedProjectorList);
    obs_data_array_t*                   _SaveSceneListOrder();
    obs_data_array_t*                   _SaveTransitions();
    obs_data_array_t*                   _SaveQuickTransitions();
    void                                _Save(const char* file);
#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
    AFMainFrame*                        m_pMainUI = nullptr;
    
    std::atomic<long>                   m_DisableSaving = 1;
    bool                                m_bProjectChanged = false;
#pragma endregion private member var
};

inline bool AFLoadSaveManager::CheckCanSaveProject()
{
    if (m_DisableSaving.load())
        return false;
    
    m_bProjectChanged = true;
    
    return true;
};

inline void AFLoadSaveManager::AddMissingFiles(void *data, obs_source_t *source)
{
    obs_missing_files_t *f = (obs_missing_files_t *)data;
    obs_missing_files_t *sf = obs_source_get_missing_files(source);

    obs_missing_files_append(f, sf);
    obs_missing_files_destroy(sf);
};
