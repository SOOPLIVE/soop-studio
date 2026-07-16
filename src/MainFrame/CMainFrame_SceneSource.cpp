#include "CMainFrame.h"
#include "ui_aneta-main-frame.h"

#include <fstream>
#include <sstream>
#include <QFileInfo>
#include <QClipboard>
#include <QMimeData>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Config/CStateAppContext.h"

#include "DynamicCompose/CMainDynamicComposit.h"

#include "UIComponent/CMessageBox.h"
#include "UIComponent/CBasicPreview.h"
#include "UIComponent/CItemWidgetHelper.h"
#include "UIComponent/CNameDialog.h"
#include "UIComponent/CCustomMenu.h"
#include "UIComponent/CSceneBottomButton.h"

#include "Blocks/SceneSourceDock/CSourceListView.h"
#include "Blocks/SceneControlDock/CProjector.h"

#include "PopupWindows/SourceDialog/CSelectSourceDialog.h"
#include "PopupWindows/SourceDialog/SOOPBrowserSource/CVideoBalloonProperty.h"

#include "SceneSource/CMainSceneSource.h"

#pragma region _SOOP_BREAKTIME
#include "Utils/BreaktimeManager.h"  
#pragma endregion

inline void clearLayout(QLayout* layout) {
    if (!layout) return;

    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        else if (QLayout* childLayout = item->layout()) {
            clearLayout(childLayout);
        }
        delete item;
    }
}

void AFMainFrame::qslotAddSceneFromCallback(OBSSource scene)
{
    QWidget* outBlock = nullptr;
    if (!m_blockManager->FindBlock(ENUM_WINDOW_TYPE::SceneSource, outBlock))
        return;

    AFSceneSourceWidget* scenesourceBlock = reinterpret_cast<AFSceneSourceWidget*>(outBlock);
    if (scenesourceBlock)
        scenesourceBlock->AddScene(scene);

	RefreshSceneUI();

	//SaveProject();

	//if (!disableSaving) {
	//    obs_source_t* source = obs_scene_get_source(scene);
	//    blog(LOG_INFO, "User added scene '%s'",
	//        obs_source_get_name(source));

	//    OBSProjector::UpdateMultiviewProjectors();
	//}

	//    OnEvent(OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED);

}

void AFMainFrame::qslotRemoveSceneFromCallback(OBSSource scene)
{
    QWidget* outBlock = nullptr;
    if (!m_blockManager->FindBlock(ENUM_WINDOW_TYPE::SceneSource, outBlock))
        return;

    AFSceneSourceWidget* scenesourceBlock = reinterpret_cast<AFSceneSourceWidget*>(outBlock);
    if (scenesourceBlock)
        scenesourceBlock->RemoveScene(scene);

    RefreshSceneUI();
}

void AFMainFrame::qslotTransitionScene()
{
    m_scene->TransitionToScene(AFSceneUtil::CnvtToOBSSource(m_scene->GetCurrentScene()));
}

void AFMainFrame::qslotTransitionSceneTriggered()
{
	OBSSource tr = m_scene->GetCurTransition();
	int duration = m_scene->GetCurDuraition();

	CreateSceneTransitionPopup(tr, duration);
}

void AFMainFrame::qslotPasteClipboardAsSource()
{
    if (m_pMainSceneSource->SizeCopiedSources() &&
        !BREAKTIME_MANAGER.IsActive()) {
        m_pMainSceneSource->qslotActionPasteSource();
        return;
    }

    const QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    if (!mimeData)
        return;

    if (mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (image.isNull())
            return;
        
        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/"
            + QDateTime::currentDateTime().toString("yyyyMMddhhmmsszzz") + ".bmp";
        if (!image.save(tempPath, "BMP")) {
            blog(LOG_ERROR, "[Clipboard] Failed to save image: %s",
                tempPath.toUtf8().constData());
            return;
        }

        obs_transform_info* pInfo = nullptr;
        obs_transform_info itemInfo;
        vec2_set(&itemInfo.pos, 0, 0);
        vec2_set(&itemInfo.scale, 0.0f, 0.0f);
        vec2_set(&itemInfo.bounds, 0.0f, 0.0f);

        itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
        itemInfo.rot = 0.0f;
        itemInfo.bounds_type = OBS_BOUNDS_NONE;
        itemInfo.bounds_alignment = OBS_ALIGN_CENTER;

        pInfo = &itemInfo;

        OBSSource newSource;
        QString fileName = QFileInfo(tempPath).fileName();

        if (!AFSourceUtil::AddNewSource(MAINFRAME, "image_source", fileName.toStdString().c_str(), true, newSource, pInfo))
            return;

        OBSDataAutoRelease settings = obs_source_get_settings(newSource);
        obs_data_set_string(settings, "file", tempPath.toStdString().c_str());
        obs_source_update(newSource, settings);
    } else if (mimeData->hasText()) {
        QString text = mimeData->text().trimmed();
        if (text.isEmpty())
            return;

        bool isUrl = (text.startsWith("http://", Qt::CaseInsensitive) ||
            text.startsWith("https://", Qt::CaseInsensitive));

        if (isUrl) {
            obs_transform_info* pInfo = nullptr;
            obs_transform_info itemInfo;
            vec2_set(&itemInfo.pos, 0, 0);
            vec2_set(&itemInfo.scale, 1.0f, 1.0f);
            vec2_set(&itemInfo.bounds, 800, 600);

            itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
            itemInfo.rot = 0.0f;
            itemInfo.bounds_type = OBS_BOUNDS_STRETCH;
            itemInfo.bounds_alignment = OBS_ALIGN_CENTER;

            pInfo = &itemInfo;

            OBSSource newSource;
            QString displayText = AFSourceUtil::GetPlaceHodlerText("browser_source");

            if (!AFSourceUtil::AddNewSource(MAINFRAME, "browser_source", displayText.toStdString().c_str(), true, newSource, pInfo))
                return;

            OBSDataAutoRelease settings = obs_source_get_settings(newSource);
            obs_data_set_string(settings, "url", text.toStdString().c_str());
            obs_source_update(newSource, settings);
        }
    }
}

static bool FindVideoBalloonSceneItem(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
    obs_source_t* source = obs_sceneitem_get_source(item);
    if (!source)
        return true;

    std::string id = obs_source_get_id(source);
    if (0 == id.compare("soop_videoballoon_source")) {
        OBSSceneItem& foundItem = *reinterpret_cast<OBSSceneItem*>(param);
        foundItem = item;
        return false;
    }
    return true;
};

void AFMainFrame::qslotRefreshVideoBalloonSource()
{
    if (m_refreshVideoBallonTimer)
        m_refreshVideoBallonTimer->stop();

    OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
    if (!scene)
        return;

    OBSSceneItem item;
    obs_scene_enum_items(scene, FindVideoBalloonSceneItem, &item);

    if (!item)
        return;

    obs_source_t* source = obs_sceneitem_get_source(item);
    if (!source)
        return;

    std::unique_ptr<obs_properties_t, decltype(&obs_properties_destroy)> props(
        obs_source_properties(source),
        obs_properties_destroy);

    if (!props)
        return;

    obs_property_t* property = obs_properties_get(props.get(), "refreshnocache");
    if (property) {
        obs_property_button_clicked(property, source);
    }
}

void AFMainFrame::RefreshSceneUI()
{
    if (!m_pMainSceneSource)
        return;

    SceneItemVector& sceneItems = m_scene->GetSceneItemVector();
    if (sceneItems.empty())
        return;

    clearLayout(ui->sceneButtonLayout);

    m_pMainSceneSource->ClearSceneBottomButtons();

    size_t sceneCount = sceneItems.size();

#pragma region _SOOP_BREAKTIME
    if(m_breakTimeManager->IsActive())
        sceneCount = 0; 
#pragma endregion

    for (size_t i = 0; i < sceneCount; i++) {
        OBSScene scene = sceneItems.at(i)->GetScene();
        int index = sceneItems.at(i)->GetSceneIndex();
        const char* name = sceneItems.at(i)->GetSceneName();

        obs_source_t* source = obs_scene_get_source(scene);
        obs_data_t* scene_data = obs_source_get_settings(source);

        bool favorite = obs_data_get_int(scene_data, "favorite_scene");
        if (favorite)
        {
            AFQSceneBottomButton* sceneButton = new AFQSceneBottomButton(this, scene, index, name);
            sceneButton->setObjectName("AFQSceneBottomButton");

            bool selected = (m_scene->GetCurrentScene() == scene);
            sceneButton->SetSelectedState(selected);

            connect(sceneButton, &AFQSceneBottomButton::qsignalSceneButtonClicked,
                    m_pMainSceneSource, &CMainSceneSource::qslotSceneButtonClicked);
            connect(sceneButton, &AFQSceneBottomButton::qsignalSceneButtonDoubleClicked,
                    m_pMainSceneSource, &CMainSceneSource::qslotSceneButtonDoubleClicked);

            ui->sceneButtonLayout->addWidget(sceneButton);

            m_pMainSceneSource->AddSceneBottomButton(sceneButton);
        }
        obs_data_release(scene_data);

        sceneItems.at(i)->SetFavoriteSceneButton(favorite);
    }

    AFQProjector::UpdateMultiviewProjectors();

    OnEvent(OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED);
}

void AFMainFrame::CreateFiltersWindow(obs_source_t* source)
{
    bool closed = true;
    if (m_sourceFilters)
        closed = m_sourceFilters->close();

    if (!closed)
        return;

    m_sourceFilters = new AFQBasicFilters(this, source);
    m_sourceFilters->setAttribute(Qt::WA_DeleteOnClose, true);
    AFQBlockManager::ApplyMoveInAllArea(m_sourceFilters);

    m_sourceFilters->show();

    //Layout Minium Window Size
    m_sourceFilters->move(m_sourceFilters->x(), m_sourceFilters->y() - 1);
}

void AFMainFrame::CreateEditTransformPopup(obs_sceneitem_t* item)
{
    if (m_transformPopup)
        m_transformPopup->close();

    m_transformPopup = new AFQBasicTransform(item, this);
    m_blockManager->ApplyMoveInAllArea(m_transformPopup);

    m_transformPopup->show();
    m_transformPopup->setAttribute(Qt::WA_DeleteOnClose, true);
}

void AFMainFrame::CreateSplitEffectPopup(obs_source_t* source)
{
    if (m_splitEffectDialog) {
        m_splitEffectDialog->close();
    }

    m_splitEffectDialog = new AFQSplitEffectDialog(this, source);
    m_splitEffectDialog->setAttribute(Qt::WA_DeleteOnClose, true);

    connect(m_splitEffectDialog, &AFQSplitEffectDialog::qsignalSplitFilterActivated,
            this, &AFMainFrame::qslotSplitFilterActivated);

    //setTopRightPositionNotUseParent(m_splitEffectDialog, this);

    QRect dockPopupPosition = QRect(this->width() + this->x(), this->y(), m_splitEffectDialog->width(), m_splitEffectDialog->height());
    QRect adjustPosition;
    m_blockManager->AdjustPositionOutSideFullScreen(dockPopupPosition, adjustPosition);
    m_splitEffectDialog->setGeometry(adjustPosition);
    m_splitEffectDialog->show();
}

void AFMainFrame::CreateSoopCefDetailProperties(obs_source_t* source, QWidget* parent)
{
    QWidget* pParent = (nullptr == parent) ? this : parent;

    if (!m_soopCefDetailProperties)
        m_soopCefDetailProperties = new AFQCefPopupDialog(pParent, source);

    if (parent)
    {
        m_soopCefDetailProperties->move(parent->x() + width(), parent->y());
        m_soopCefDetailProperties->show();
    }
    else
    {
        QPoint propertyPoint = QPoint(
            this->x() + ((m_soopCefDetailProperties->width() - m_soopCefDetailProperties->width()) / 2),
            this->y());
        QRect adjustRect;

        QRect detailRect = QRect(propertyPoint.x(), propertyPoint.y(),
            m_soopCefDetailProperties->width(), m_soopCefDetailProperties->height());

        m_blockManager->AdjustPositionOutSideFullScreen(detailRect, adjustRect);
        m_soopCefDetailProperties->setGeometry(adjustRect);
    }
}

void AFMainFrame::SetCurrentScene(OBSSource scene, bool force)
{
    if (force) {
        OBSScene obsScene = obs_scene_from_source(scene);
        m_scene->SetCurrentScene(obsScene);
    }
    else {
        OBSScene curScene = m_scene->GetCurrentScene();
        if (scene == obs_scene_get_source(curScene))
            return;
    }

    if (STATEAPP.IsPreviewProgramMode() == false) {
        m_scene->TransitionToScene(scene, force);
    }
    else {
        OBSSource actualLastScene = OBSGetStrongRef(m_scene->GetLastScene());
        if (actualLastScene != scene) {
            if (scene)
                obs_source_inc_showing(scene);
            if (actualLastScene)
                obs_source_dec_showing(actualLastScene);

            m_scene->SetLastScene(scene);
        }
    }

    if (m_dynamicCompositMainWindow)
        m_dynamicCompositMainWindow->SetCurrentScene(scene);

    if(m_pMainSceneSource)
        m_pMainSceneSource->SetSceneBottomButtonStyleSheet(scene);
}

void AFMainFrame::ClearSceneData(bool init)
{    
    LOADSAVE_CONTEXT.IncreaseCheckSaveCnt();

    setCursor(Qt::WaitCursor);

    // Need Clear Scene, Source Ref 
    if(m_pMainSceneSource)
        m_pMainSceneSource->ClearSceneBottomButtons();

    m_scene->ClearSceneListButtonList();
    m_scene->ClearSceneTransitionList();
    m_scene->ClearVolControlList();

    for (int i = 0; i < MAX_CHANNELS; i++)
        obs_set_output_source(i, nullptr);

    /* Reset VCam to default to clear its private scene and any references
     * it holds. It will be reconfigured during loading. */
    if(VirtualCamEnabled()) {
        SetVirtualCamOutputType(VCamOutputType::ProgramView);
    }

//    safeModeModuleData = nullptr;
    
    m_scene->ClearContext();

    if(m_pMainSceneSource)
        m_pMainSceneSource->ClearClipboard();
 
    OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP);

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
        LOADSAVE_CONTEXT.DecreaseCheckSaveCnt();
        /* Avoid logging list twice in case it gets called after
         * setting the flag the first time. */
        if (!m_clearingFailed) {
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
        m_clearingFailed = true;
    } else {
        LOADSAVE_CONTEXT.DecreaseCheckSaveCnt();

        blog(LOG_INFO, "All scene data cleared");
        blog(LOG_INFO,
             "------------------------------------------------");
    }
}


void AFMainFrame::RefreshVideoBalloonSource()
{
    if (m_refreshVideoBallonTimer) {
    	m_refreshVideoBallonTimer->start(1000);
    }
}

void AFMainFrame::_AddSceneDefaultName()
{
    QString format{ Str("Basic.Main.DefaultSceneName.Text") };
    int i = 2;
    QString placeHolderText = format.arg(i);
    OBSSourceAutoRelease source = nullptr;
    while ((source = obs_get_source_by_name(QT_TO_UTF8(placeHolderText)))) {
        placeHolderText = format.arg(++i);
    }
    obs_get_source_by_name(placeHolderText.toStdString().c_str());

    obs_source_t* scene_source = AFSceneUtil::CreateOBSScene(placeHolderText.toStdString().c_str());
    SetCurrentScene(scene_source);
}