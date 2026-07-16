#include "CMainFrame.h"
#include "ui_aneta-main-frame.h"

#include <string>
#include <QScreen>
#include <QDir>

#include "Application/CApplication.h"

#include "Common/SettingsMiscDef.h"

#include "Profile/CMainProfile.h"
#include "SceneCollection/CMainSceneCollection.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Browser/CCefManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Source/CSource.h"
#include "CoreModel/Profile/CProfile.h"

#include "UIComponent/CNameDialog.h"
#include "UIComponent/CMessageBox.h"

#include "PopupWindows/CImporterDialog.h"
#include "PopupWindows/CMissingFilesDialog.h"

#include "ViewModel/Auth/CAuth.h"

#include "AudioSource/CAudioSource.h"


void AFMainFrame::qActionRemigrateSceneCollectionTriggered()
{
}

void AFMainFrame::qslotImportPreset()
{
    if (!AUTH_CONTEXT.IsSoopRegistered())
        return;

    ShowPresetGuide();
}

void AFMainFrame::qslotShowMissingFiles()
{
    obs_missing_files_t *files = obs_missing_files_create();

    auto cb_sources = [](void *data, obs_source_t *source) {
        AFProfileUtil::AddMissingFiles(data, source);
        return true;
    };

    obs_enum_all_sources(cb_sources, files);
    ShowMissingFilesDialog(files);
}

int AFMainFrame::qslotShowImportGuide()
{
    AFQImportGuide* importGuide = new AFQImportGuide(this);
    importGuide->setModal(true);
    importGuide->ImportGuideInit();

    importGuide->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    importGuide->show();
    importGuide->raise();
    importGuide->activateWindow();

    importGuide->setWindowFlag(Qt::WindowStaysOnTopHint, false);
    importGuide->show();

    if (importGuide->exec())
    {
        m_mainProfile->RefreshProfiles();
        m_mainProfile->ChangeProfile();
    }

    if (importGuide->GetPresetType() != BroadPresetType::New)
        m_presetSourcesGeometry = importGuide->GetSourcesGeometry(importGuide->GetPresetType());

    return importGuide->GetStartChoice();
}

int AFMainFrame::qslotShowImportRecentGuide()
{
    AFQImportGuide* importGuide = new AFQImportGuide(this);
    importGuide->setModal(true);
    importGuide->ImportRecentGuideInit();

    importGuide->setWindowFlag(Qt::WindowStaysOnTopHint, true);
    importGuide->setFixedSize(QSize(720, 485));
    importGuide->show();
    importGuide->raise();
    importGuide->activateWindow();

    importGuide->setWindowFlag(Qt::WindowStaysOnTopHint, false);
    importGuide->show();

    if (importGuide->exec())
    {
        m_mainProfile->RefreshProfiles();
        m_mainProfile->ChangeProfile();
        m_mainSceneCollection->RefreshSceneCollections();
    }

    if (importGuide->GetPresetType() != BroadPresetType::New)
        m_presetSourcesGeometry = importGuide->GetSourcesGeometry(importGuide->GetPresetType());

    return importGuide->GetStartChoice();
}

void AFMainFrame::ShowPresetGuide()
{
    AFQImportGuide* importGuide = new AFQImportGuide(this);
    importGuide->setModal(true);
    importGuide->ImportGuideInit(true);
    if (importGuide->exec())
    {
        _AddSceneDefaultName();
        if (importGuide->GetPresetType() != BroadPresetType::New) {
            m_presetSourcesGeometry = importGuide->GetSourcesGeometry(importGuide->GetPresetType());
            _AddBroadPreset();
        }
    }
}
