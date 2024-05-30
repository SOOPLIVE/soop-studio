#include "CMainFrame.h"
#include "ui_aneta-main-frame.h"

#include "UIComponent/CItemWidgetHelper.h"

#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Scene/CSceneContext.h"

#include "DynamicCompose/CMainDynamicComposit.h"

#include "Blocks/SceneSourceDock/CSourceListView.h"

void AFMainFrame::ClearSceneData(bool init)
{
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    auto& loadSaver = AFLoadSaveManager::GetSingletonInstance();
    
    loadSaver.IncreaseCheckSaveCnt();

    setCursor(Qt::WaitCursor);

    // Need Clear Scene, Source Ref 
    ClearSceneBottomButtons();

    SceneItemVector& sceneItems = sceneContext.GetSceneItemVector();
    auto iterScene = sceneItems.begin();
    for (; iterScene != sceneItems.end(); ++iterScene) {
        AFQSceneListItem* item = (*iterScene);
        if (!item)
            continue;
        item->close();
        delete item;
    }
    sceneItems.clear();
    sceneContext.SetCurrOBSScene(nullptr);

    AFQSourceListView* sourceListView = sceneContext.GetSourceListViewPtr();
    if (sourceListView)
        sourceListView->Clear();

    std::vector<OBSSource>& transitions = sceneContext.GetRefTransitions();
    transitions.clear();
    sceneContext.SetCurTransition(nullptr);

    for (int i = 0; i < MAX_CHANNELS; i++)
        obs_set_output_source(i, nullptr);
    
    sceneContext.ClearContext();

    m_clipboard.clear();

    
    if (api)
        api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP);

    m_undo_s.Clear();

    /* using QEvent::DeferredDelete explicitly is the only way to ensure
     * that deleteLater events are processed at this point */
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    do {
        QApplication::sendPostedEvents(nullptr);
    } while (obs_wait_for_destroy_queue());

    /* Pump Qt events one final time to give remaining signals time to be
     * processed (since this happens after the destroy thread finishes and
     * the audio/video threads have processed their tasks). */
    QApplication::sendPostedEvents(nullptr);

    unsetCursor();

    /* If scene data wasn't actually cleared, e.g. faulty plugin holding a
     * reference, they will still be in the hash table, enumerate them and
     * store the names for logging purposes. */
    auto cb2 = [](void *param, obs_source_t *source) {
        auto orphans = static_cast<std::vector<std::string> *>(param);
        orphans->push_back(obs_source_get_name(source));
        return true;
    };

    std::vector<std::string> orphan_sources;
    obs_enum_sources(cb2, &orphan_sources);

    if (!orphan_sources.empty()) {
        loadSaver.DecreaseCheckSaveCnt();
        /* Avoid logging list twice in case it gets called after
         * setting the flag the first time. */
        if (!m_bClearingFailed) {
            /* This ugly mess exists to join a vector of strings
             * with a user-defined delimiter. */
            std::string orphan_names = std::accumulate(
                orphan_sources.begin(), orphan_sources.end(),
                std::string(""), [](std::string a, std::string b) {
                    return std::move(a) + "\n- " + b;
                });

            blog(LOG_ERROR,
                 "Not all sources were cleared when clearing scene data:\n%s\n",
                 orphan_names.c_str());
        }

        /* We do not decrement disableSaving here to avoid OBS
         * overwriting user data with garbage. */
        m_bClearingFailed = true;
    } else {
        loadSaver.DecreaseCheckSaveCnt();

        blog(LOG_INFO, "All scene data cleared");
        blog(LOG_INFO,
             "------------------------------------------------");
    }
}

