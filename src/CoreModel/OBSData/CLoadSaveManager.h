#pragma once

#include <vector>
#include <functional>
#include <atomic>
#include <filesystem>

#include <obs.hpp>


class AFLoadSaveManager final
{
public:
    AFLoadSaveManager() = default;
    ~AFLoadSaveManager() = default;

public:
    inline              void IncreaseCheckSaveCnt() {
                            long tmpValue = m_disableSaving;
                            ++tmpValue;
                            m_disableSaving.store(tmpValue);
                        };
    inline void         DecreaseCheckSaveCnt() {
                            long tmpValue = m_disableSaving;
                            --tmpValue;
                            m_disableSaving.store(tmpValue);
                        };

    bool                InitLoadSave();
    bool                Load(const char* file, bool remigrate = false);


    //obs_data_array_t*   SaveProjectors();
    inline bool         CheckDisableSaving() { return m_disableSaving.load(); };
    bool                CheckCanSaveProject();
    void                ForceSaveProjectNow();
    void                SaveProjectNow();
    void                SaveProjectDeferred();

    void                MoveProfileToBackup(std::string remainID);
    void                MoveSceneCollectionToBackup(std::string remainID);

private:
    void                _LoadTransitions(obs_data_array_t *transitions,
                                         obs_load_source_cb cb, void *private_data);
    void                _LoadSceneListOrder(obs_data_array_t *array);

    void                _LoadData(obs_data_t* datat, const char* file);


    void                _SaveAudioDevice(const char *name, int channel, obs_data_t *parent,
                                         std::vector<OBSSource> &audioSources);
    obs_data_t*         _GenerateSaveData(obs_data_array_t* sceneOrder,
                                          obs_data_array_t* quickTransitionData,
                                          int transitionDuration,
                                          obs_data_array_t* transitions,
                                          OBSScene& scene, OBSSource& curProgramScene,
                                          obs_data_array_t* savedProjectorList);
    obs_data_array_t*   _SaveSceneListOrder();
    obs_data_array_t*   _SaveTransitions();
    obs_data_array_t*   _SaveQuickTransitions();
    void                _Save(const char* file);
    void                _CheckBackupDir(std::string remainID);

    void sceneCollectionBackup(const char* file, obs_data_t* data);

private:
    std::atomic<long>   m_disableSaving = 1;
    bool                m_projectChanged = false;

};