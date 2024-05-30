#include "CMainFrame.h"
#include "ui_aneta-main-frame.h"


#include <string>


#include <QScreen>
#include <QDir>

#include "Application/CApplication.h"

#include "include/qt-wrapper.h"
#include "Common/SettingsMiscDef.h"


#include "CoreModel/Browser/CCefManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/Auth/CAuthManager.h"


#include "UIComponent/CNameDialog.h"
#include "UIComponent/CMessageBox.h"


#include "PopupWindows/CImporterDialog.h"
#include "PopupWindows/CMissingFilesDialog.h"


#include "ViewModel/Auth/CAuth.h"




void AFMainFrame::qSlotChangeProfile()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& cefBrowserContext = AFCefManager::GetSingletonInstance();
    
    
    QAction *action = reinterpret_cast<QAction *>(sender());
    ConfigFile config;
    std::string path;

    if (!action)
        return;

    path = QT_TO_UTF8(action->property("file_name").value<QString>());
    if (path.empty())
        return;

    const char *oldName =
        config_get_string(confManager.GetGlobal(), "Basic", "Profile");
    if (action->text().compare(QT_UTF8(oldName)) == 0) {
        action->setChecked(true);
        return;
    }

    size_t path_len = path.size();
    path += "/basic.ini";

    if (config.Open(path.c_str(), CONFIG_OPEN_ALWAYS) != 0) {
        blog(LOG_ERROR, "ChangeProfile: Failed to load file '%s'",
             path.c_str());
        return;
    }

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGING);

    path.resize(path_len);

    const char *newName = config_get_string(config, "General", "Name");
    const char *newDir = strrchr(path.c_str(), '/') + 1;

    QString settingsRequiringRestart;
    bool needsRestart =
        _ProfileNeedsRestart(config, settingsRequiringRestart);

    config_set_string(confManager.GetGlobal(), "Basic", "Profile", newName);
    config_set_string(confManager.GetGlobal(), "Basic", "ProfileDir", newDir);

    
    AFAuth::Save();
    ResetAuth();
    
    cefBrowserContext.DestroyPanelCookieManager();
#ifdef YOUTUBE_ENABLED
    if (youtubeAppDock)
        DeleteYouTubeAppDock();
#endif

    QList<QScreen*> screens = QGuiApplication::screens();
    QScreen* primaryScreen = QGuiApplication::primaryScreen();

    
    uint32_t cx = primaryScreen->size().width();
    uint32_t cy = primaryScreen->size().height();
    uint32_t cntScreen = (uint32_t)screens.count();
    confManager.SwapOtherToBasic(config,cntScreen, cx, cy, devicePixelRatioF());
    
    
    _ResetProfileData();
    RefreshProfiles();
    config_save_safe(confManager.GetGlobal(), "tmp", nullptr);
    GetMainWindow()->UpdateVolumeControlsDecayRate();

    AFAuth::Load();
#ifdef YOUTUBE_ENABLED
    if (YouTubeAppDock::IsYTServiceSelected() && !youtubeAppDock)
        NewYouTubeAppDock();
#endif

    _CheckForSimpleModeX264Fallback();

    blog(LOG_INFO, "Switched to profile '%s' (%s)", newName, newDir);
    blog(LOG_INFO, "------------------------------------------------");

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);

    if (needsRestart) {
    }
}

void AFMainFrame::qSlotNewProfile()
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    
    AddProfile(true,
               localeManager.Str("AddProfile.Title"),
               localeManager.Str("AddProfile.Text"));
}

void AFMainFrame::qSlotDupProfile()
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();

    AddProfile(false,
        localeManager.Str("AddProfile.Title"),
        localeManager.Str("AddProfile.Text"));
}

void AFMainFrame::qSlotDeleteProfile(bool skipConfirmation)
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& loaderSaver = AFLoadSaveManager::GetSingletonInstance();
    auto& cefBrowserContext = AFCefManager::GetSingletonInstance();
    auto& authManager = AFAuthManager::GetSingletonInstance();

    std::string newName;
    std::string newPath;
    ConfigFile config;

    std::string oldDir =
        config_get_string(confManager.GetGlobal(), "Basic", "ProfileDir");
    std::string oldName =
        config_get_string(confManager.GetGlobal(), "Basic", "Profile");

    auto cb = [&](const char* name, const char* filePath) {
        if (strcmp(oldName.c_str(), name) != 0) {
            newName = name;
            newPath = filePath;
            return false;
        }

        return true;
    };

    loaderSaver.EnumProfiles(cb);

    /* this should never be true due to menu item being grayed out */
    if (newPath.empty())
        return;

    if (!skipConfirmation) {
        QString confirmText = QT_UTF8(localeManager.Str("ConfirmRemove.Text")).arg(QT_UTF8(oldName.c_str()));

        int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            this, "", confirmText);
        if (result == QDialog::Rejected)
            return;
    }

    size_t newPath_len = newPath.size();
    newPath += "/basic.ini";

    if (config.Open(newPath.c_str(), CONFIG_OPEN_ALWAYS) != 0) {
        blog(LOG_ERROR, "ChangeProfile: Failed to load file '%s'",
            newPath.c_str());
        return;
    }

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGING);

    newPath.resize(newPath_len);

    const char* newDir = strrchr(newPath.c_str(), '/') + 1;

    config_set_string(confManager.GetGlobal(), "Basic", "Profile",
        newName.c_str());
    config_set_string(confManager.GetGlobal(), "Basic", "ProfileDir", newDir);

    QString settingsRequiringRestart;
    bool needsRestart =
        _ProfileNeedsRestart(config, settingsRequiringRestart);

    AFAuth::Save();
    m_auth.reset();
    cefBrowserContext.DeleteCookies();
    cefBrowserContext.DestroyPanelCookieManager();

    QList<QScreen*> screens = QGuiApplication::screens();
    QScreen* primaryScreen = QGuiApplication::primaryScreen();

    uint32_t cx = primaryScreen->size().width();
    uint32_t cy = primaryScreen->size().height();
    uint32_t cntScreen = (uint32_t)screens.count();

    confManager.SwapOtherToBasic(config, cntScreen, cx, cy, devicePixelRatioF());

    _ResetProfileData();
    DeleteProfile(oldName.c_str(), oldDir.c_str());
    RefreshProfiles();
    config_save_safe(confManager.GetGlobal(), "tmp", nullptr);

    blog(LOG_INFO, "Switched to profile '%s' (%s)", newName.c_str(),
        newDir);
    blog(LOG_INFO, "------------------------------------------------");

    GetMainWindow()->UpdateVolumeControlsDecayRate();

    AFAuth::Load();

    if (api) {
        api->on_event(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
        api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
    }

    if (needsRestart) {
        int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            this, "", QTStr("LoadProfileNeedsRestart"));
        if (result == QDialog::Accepted) {
            g_bRestart = true;
            close();
        }
    }
}

void AFMainFrame::qSlotRenameProfile()
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();

    std::string curDir =
        config_get_string(confManager.GetGlobal(), "Basic", "ProfileDir");
    std::string curName =
        config_get_string(confManager.GetGlobal(), "Basic", "Profile");

    /* Duplicate and delete in case there are any issues in the process */
    bool success = AddProfile(false, localeManager.Str("RenameProfile.Title"),
        localeManager.Str("AddProfile.Text"), curName.c_str(),
        true);
    if (success) {
        DeleteProfile(curName.c_str(), curDir.c_str());
        RefreshProfiles();
    }

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_PROFILE_RENAMED);
}

void AFMainFrame::qSlotExportProfile()
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
    char path[512];

    QString home = QDir::homePath();

    QString currentProfile = QString::fromUtf8(config_get_string(
                                 confManager.GetGlobal(), "Basic", "ProfileDir"));

    int ret = confManager.GetConfigPath(path, 512, "SOOPStudio/basic/profiles/");
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get profile config path");
        return;
    }

    QString dir = SelectDirectory(this,
                                  QT_UTF8(localeManager.Str(
                                    "Basic.MainMenu.Profile.Export")),
                                  home);

    if (!dir.isEmpty() && !dir.isNull()) {
        QString outputDir = dir + "/" + currentProfile;
        QString inputPath = QString::fromUtf8(path);
        QDir folder(outputDir);

        if (!folder.exists()) {
            folder.mkpath(outputDir);
        } else {
            if (QFile::exists(outputDir + "/basic.ini"))
                QFile::remove(outputDir + "/basic.ini");

            if (QFile::exists(outputDir + "/service.json"))
                QFile::remove(outputDir + "/service.json");

            if (QFile::exists(outputDir + "/streamEncoder.json"))
                QFile::remove(outputDir +
                          "/streamEncoder.json");

            if (QFile::exists(outputDir + "/recordEncoder.json"))
                QFile::remove(outputDir +
                          "/recordEncoder.json");
        }

        QFile::copy(inputPath + currentProfile + "/basic.ini",
                outputDir + "/basic.ini");
        QFile::copy(inputPath + currentProfile + "/service.json",
                outputDir + "/service.json");
        QFile::copy(inputPath + currentProfile + "/streamEncoder.json",
                outputDir + "/streamEncoder.json");
        QFile::copy(inputPath + currentProfile + "/recordEncoder.json",
                outputDir + "/recordEncoder.json");
    }
}

void AFMainFrame::qSlotImportProfile()
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
    char path[512];

    QString home = QDir::homePath();

    int ret = confManager.GetConfigPath(path, 512, "SOOPStudio/basic/profiles/");
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get profile config path");
        return;
    }

    QString dir = SelectDirectory(this,
                                  QT_UTF8(
                                    localeManager.Str("Basic.MainMenu.Profile.Import")),
                                  home);

    if (!dir.isEmpty() && !dir.isNull())
    {
        QString inputPath = QString::fromUtf8(path);
        QFileInfo finfo(dir);
        QString directory = finfo.fileName();
        QString profileDir = inputPath + directory;

        if (AFLoadSaveManager::ProfileExists(directory.toStdString().c_str()))
        {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                "",
                QTStr("Basic.MainMenu.Profile.Exists"));
        }
        else if (os_mkdir(profileDir.toStdString().c_str()) < 0)
        {
            blog(LOG_WARNING,
                 "Failed to create profile directory '%s'",
                 directory.toStdString().c_str());
        } 
        else
        {
            QFile::copy(dir + "/basic.ini",
                    profileDir + "/basic.ini");
            QFile::copy(dir + "/service.json",
                    profileDir + "/service.json");
            QFile::copy(dir + "/streamEncoder.json",
                    profileDir + "/streamEncoder.json");
            QFile::copy(dir + "/recordEncoder.json",
                    profileDir + "/recordEncoder.json");
            RefreshProfiles();
        }
    }
}

void AFMainFrame::qSlotNewSceneCollection()
{
    AddSceneCollection(true);
}

void AFMainFrame::qSlotDupSceneCollection()
{
    AddSceneCollection(false);
}

void AFMainFrame::qSlotRenameSceneCollection()
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& loaderSaver = AFLoadSaveManager::GetSingletonInstance();

    std::string name;
    std::string file;
    std::string oname;
    bool showWizard = false;

    std::string oldFile = config_get_string(confManager.GetGlobal(), "Basic",
        "SceneCollectionFile");
    const char* oldName = config_get_string(confManager.GetGlobal(), "Basic",
        "SceneCollection");
    oname = std::string(oldName);

    if (!_GetSceneCollectionName(this, name, file, oldName))
        return;

    config_set_string(confManager.GetGlobal(), "Basic", "SceneCollection",
        name.c_str());
    config_set_string(confManager.GetGlobal(), "Basic", "SceneCollectionFile",
        file.c_str());

    loaderSaver.SaveProjectNow();

    char path[512];
    int ret = confManager.GetConfigPath(path, 512, "SOOPStudio/basic/scenes/");
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get scene collection config path");
        return;
    }

    oldFile.insert(0, path);
    oldFile += ".json";
    os_unlink(oldFile.c_str());
    oldFile += ".bak";
    os_unlink(oldFile.c_str());

    blog(LOG_INFO, "------------------------------------------------");
    blog(LOG_INFO, "Renamed scene collection to '%s' (%s.json)",
        name.c_str(), file.c_str());
    blog(LOG_INFO, "------------------------------------------------");

    RefreshSceneCollections();

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_RENAMED);
}

void AFMainFrame::qSlotDeleteSceneCollection()
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& loaderSaver = AFLoadSaveManager::GetSingletonInstance();

    std::string newName;
    std::string newPath;

    std::string oldFile = config_get_string(confManager.GetGlobal(), "Basic",
        "SceneCollectionFile");
    std::string oldName = config_get_string(confManager.GetGlobal(), "Basic",
        "SceneCollection");

    auto cb = [&](const char* name, const char* filePath) {
        if (strcmp(oldName.c_str(), name) != 0) {
            newName = name;
            newPath = filePath;
            return false;
        }

        return true;
    };

    loaderSaver.EnumSceneCollections(cb);

    /* this should never be true due to menu item being grayed out */
    if (newPath.empty())
        return;

    QString text =
        QString(localeManager.Str("ConfirmRemove.Text")).arg(QT_UTF8(oldName.c_str()));

    int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this, localeManager.Str("ConfirmRemove.Title"), text);

    if (result != 1)
        return;

    char path[512];
    int ret = confManager.GetConfigPath(path, 512, "SOOPStudio/basic/scenes/");
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get scene collection config path");
        return;
    }

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

    oldFile.insert(0, path);
    /* os_rename() overwrites if necessary, only the .bak file will remain. */
    os_rename((oldFile + ".json").c_str(), (oldFile + ".json.bak").c_str());

    loaderSaver.Load(newPath.c_str());
    RefreshSceneCollections();

    const char* newFile = config_get_string(confManager.GetGlobal(), "Basic",
        "SceneCollectionFile");

    blog(LOG_INFO,
        "Removed scene collection '%s' (%s.json), "
        "switched to '%s' (%s.json)",
        oldName.c_str(), oldFile.c_str(), newName.c_str(), newFile);
    blog(LOG_INFO, "------------------------------------------------");

    if (api) {
        api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
        api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
    }
}

void AFMainFrame::qSlotImportSceneCollection()
{
    AFQImporterDialog imp(this);
    imp.exec();
    
    RefreshSceneCollections();
}

void AFMainFrame::qSlotExportSceneCollection()
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    auto& loaderSaver = AFLoadSaveManager::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
    
    loaderSaver.SaveProjectNow();
    
    char path[512];

    QString home = QDir::homePath();

    QString currentFile = QT_UTF8(config_get_string(confManager.GetGlobal(),
                                                    "Basic", "SceneCollectionFile"));

    int ret = confManager.GetConfigPath(path, 512, "SOOPStudio/basic/scenes/");
    if (ret <= 0)
    {
        blog(LOG_WARNING, "Failed to get scene collection config path");
        return;
    }

    QString exportFile =
        SaveFile(this, QT_UTF8(localeManager.Str("Basic.MainMenu.SceneCollection.Export")),
                 home + "/" + currentFile, "JSON Files (*.json)");

    std::string file = QT_TO_UTF8(exportFile);

    if (!exportFile.isEmpty() && !exportFile.isNull()) {
        QString inputFile = path + currentFile + ".json";

        OBSDataAutoRelease collection =
            obs_data_create_from_json_file(QT_TO_UTF8(inputFile));

        OBSDataArrayAutoRelease sources =
            obs_data_get_array(collection, "sources");
        if (!sources) {
            blog(LOG_WARNING,
                 "No sources in exported scene collection");
            return;
        }
        obs_data_erase(collection, "sources");

        // We're just using std::sort on a vector to make life easier.
        std::vector<OBSData> sourceItems;
        obs_data_array_enum(
            sources,
            [](obs_data_t *data, void *pVec) -> void {
                auto &sourceItems =
                    *static_cast<std::vector<OBSData> *>(pVec);
                sourceItems.push_back(data);
            },
            &sourceItems);

        std::sort(sourceItems.begin(), sourceItems.end(),
              [](const OBSData &a, const OBSData &b) {
                  return astrcmpi(obs_data_get_string(a,
                                      "name"),
                          obs_data_get_string(
                              b, "name")) < 0;
              });

        OBSDataArrayAutoRelease newSources = obs_data_array_create();
        for (auto &item : sourceItems)
            obs_data_array_push_back(newSources, item);

        obs_data_set_array(collection, "sources", newSources);
        obs_data_save_json_pretty_safe(
            collection, QT_TO_UTF8(exportFile), "tmp", "bak");
    }
}

void AFMainFrame::qSlotShowMissingFiles()
{
    obs_missing_files_t *files = obs_missing_files_create();

    auto cb_sources = [](void *data, obs_source_t *source) {
        AFLoadSaveManager::AddMissingFiles(data, source);
        return true;
    };

    obs_enum_all_sources(cb_sources, files);
    ShowMissingFilesDialog(files);
}

void AFMainFrame::RefreshProfiles()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
    QMenu* profilesMenu = ui->action_Profile->menu();
    QList<QAction *> menuActions = profilesMenu->actions();
    int count = 0;

    for (int i = 0; i < menuActions.count(); i++)
    {
        QVariant v = menuActions[i]->property("file_name");
        if (v.typeName() != nullptr)
            delete menuActions[i];
    }

    const char *curName =
        config_get_string(confManager.GetGlobal(), "Basic", "Profile");

    auto addProfile = [&](const char *name, const char *path) {
        std::string file = strrchr(path, '/') + 1;

        QAction* action = new QAction(QT_UTF8(name), this);
        action->setProperty("file_name", QT_UTF8(path));
        connect(action, &QAction::triggered, this,
                &AFMainFrame::qSlotChangeProfile);
        action->setCheckable(true);
        action->setChecked(strcmp(name, curName) == 0);

        profilesMenu->addAction(action);
        
        count++;
        return true;
    };

    AFLoadSaveManager::EnumProfiles(addProfile);

    ui->action_RemoveProfile->setEnabled(count > 1);
}

bool AFMainFrame::CreateProfile(const std::string &newName, bool create_new,
                                bool showWizardChecked, bool rename)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& cefBrowserContext = AFCefManager::GetSingletonInstance();
    
    
    std::string newDir;
    std::string newPath;
    ConfigFile config;

    if (!_FindSafeProfileDirName(newName, newDir))
        return false;

    if (create_new)
        config_set_bool(confManager.GetGlobal(), "Basic",
                        "ConfigOnNewProfile", showWizardChecked);
    

    std::string curDir =
        config_get_string(confManager.GetGlobal(), "Basic", "ProfileDir");

    char baseDir[512];
    int ret = confManager.GetConfigPath(baseDir, sizeof(baseDir),
                                        "SOOPStudio/basic/profiles/");
    if (ret <= 0)
    {
        blog(LOG_WARNING, "Failed to get profiles config path");
        return false;
    }

    newPath = baseDir;
    newPath += newDir;

    if (os_mkdir(newPath.c_str()) < 0) 
    {
        blog(LOG_WARNING, "Failed to create profile directory '%s'",
             newDir.c_str());
        return false;
    }

    if (!create_new)
        _CopyProfile(curDir.c_str(), newPath.c_str());

    newPath += "/basic.ini";

    if (config.Open(newPath.c_str(), CONFIG_OPEN_ALWAYS) != 0) {
        blog(LOG_ERROR, "Failed to open new config file '%s'",
             newDir.c_str());
        return false;
    }

    if (api && !rename)
        api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGING);

    config_set_string(confManager.GetGlobal(), "Basic", "Profile",
                      newName.c_str());
    config_set_string(confManager.GetGlobal(), "Basic", "ProfileDir",
                      newDir.c_str());


    AFAuth::Save();
    
    if (create_new)
    {
        ResetAuth();
        cefBrowserContext.DestroyPanelCookieManager();
#ifdef YOUTUBE_ENABLED
        if (youtubeAppDock)
            DeleteYouTubeAppDock();
#endif
    }
    else if (!rename)
        cefBrowserContext.DuplicateCurrentCookieProfile(config);
    
    
    config_set_string(config, "General", "Name", newName.c_str());
    
    QList<QScreen*> screens = QGuiApplication::screens();
    QScreen* primaryScreen = QGuiApplication::primaryScreen();

    
    uint32_t cx = primaryScreen->size().width();
    uint32_t cy = primaryScreen->size().height();
    uint32_t cntScreen = (uint32_t)screens.count();
    confManager.SafeSwapOtherToBasic(config,cntScreen, cx, cy, devicePixelRatioF());
    
    RefreshProfiles();

    if (create_new)
        _ResetProfileData();

    blog(LOG_INFO, "Created profile '%s' (%s, %s)", newName.c_str(),
         create_new ? "clean" : "duplicate", newDir.c_str());
    blog(LOG_INFO, "------------------------------------------------");

    config_save_safe(confManager.GetGlobal(), "tmp", nullptr);
    
    GetMainWindow()->UpdateVolumeControlsDecayRate();

    
    AFAuth::Load();

    // Run auto configuration setup wizard when a new profile is made to assist
    // setting up blank settings
    if (create_new && showWizardChecked)
    {

    }

    if (api && !rename) {
        api->on_event(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
        api->on_event(OBS_FRONTEND_EVENT_PROFILE_CHANGED);
    }
    
     
    return true;
}

bool AFMainFrame::AddProfile(bool create_new, const char *title, const char *text,
                             const char *init_text, bool rename)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
    std::string name;

    bool showWizardChecked = config_get_bool(confManager.GetGlobal(), "Basic",
                                             "ConfigOnNewProfile");

    if (!_AskForProfileName(this, name, title, text, create_new,
                            showWizardChecked, init_text))
        return false;

    return CreateProfile(name, create_new, showWizardChecked, rename);
}

void AFMainFrame::DeleteProfile(const char* profileName, const char* profileDir)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();

    char profilePath[512];
    char basePath[512];

    int ret = confManager.GetConfigPath(basePath, sizeof(basePath),
        "SOOPStudio/basic/profiles");
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get profiles config path");
        return;
    }

    ret = snprintf(profilePath, sizeof(profilePath), "%s/%s/*", basePath,
        profileDir);
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get path for profile dir '%s'",
            profileDir);
        return;
    }

    os_glob_t* glob;
    if (os_glob(profilePath, 0, &glob) != 0) {
        blog(LOG_WARNING, "Failed to glob profile dir '%s'",
            profileDir);
        return;
    }

    for (size_t i = 0; i < glob->gl_pathc; i++) {
        const char* filePath = glob->gl_pathv[i].path;

        if (glob->gl_pathv[i].directory)
            continue;

        os_unlink(filePath);
    }

    os_globfree(glob);

    ret = snprintf(profilePath, sizeof(profilePath), "%s/%s", basePath,
        profileDir);
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get path for profile dir '%s'",
            profileDir);
        return;
    }

    os_rmdir(profilePath);

    blog(LOG_INFO, "------------------------------------------------");
    blog(LOG_INFO, "Removed profile '%s' (%s)", profileName, profileDir);
    blog(LOG_INFO, "------------------------------------------------");
}

void AFMainFrame::WaitDevicePropertiesThread()
{
    if (m_pDevicePropertiesThread && m_pDevicePropertiesThread->isRunning())
    {
        m_pDevicePropertiesThread->wait();
        m_pDevicePropertiesThread.reset();
    }

    QApplication::sendPostedEvents(nullptr);
}

bool AFMainFrame::AddSceneCollection(bool create_new, const QString& qname)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    std::string name;
    std::string file;

    if (qname.isEmpty())
    {
        if (_GetSceneCollectionName(this, name, file) == false)
            return false;
    } 
    else
    {
        name = QT_TO_UTF8(qname);
        
        if (AFLoadSaveManager::SceneCollectionExists(name.c_str()))
            return false;

        if (confManager.GetUnusedSceneCollectionFile(name, file) == false)
            return false;
    }

    auto new_collection = [this, create_new](const std::string &file,
                         const std::string &name) {
        auto& confManager = AFConfigManager::GetSingletonInstance();
        auto& loaderSaver = AFLoadSaveManager::GetSingletonInstance();
        
        loaderSaver.SaveProjectNow();

        config_set_string(confManager.GetGlobal(), "Basic",
                          "SceneCollection", name.c_str());
        config_set_string(confManager.GetGlobal(), "Basic",
                          "SceneCollectionFile", file.c_str());

        if (create_new)
            GetMainWindow()->CreateDefaultScene(false);
        else
            obs_reset_source_uuids();
        

        loaderSaver.SaveProjectNow();
        RefreshSceneCollections();
    };

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

    new_collection(file, name);

    blog(LOG_INFO, "Added scene collection '%s' (%s, %s.json)",
         name.c_str(), create_new ? "clean" : "duplicate", file.c_str());
    blog(LOG_INFO, "------------------------------------------------");

    if (api) {
        api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
        api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
    }

    return true;
}

void AFMainFrame::RefreshSceneCollections()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
    QMenu* sceneCollectionMenu = ui->action_SceneCollection->menu();
    QList<QAction *> menuActions = sceneCollectionMenu->actions();
    int count = 0;

    for (int i = 0; i < menuActions.count(); i++) {
        QVariant v = menuActions[i]->property("file_name");
        if (v.typeName() != nullptr)
            delete menuActions[i];
    }

    const char *cur_name = config_get_string(confManager.GetGlobal(),
                                             "Basic","SceneCollection");

    auto addCollection = [&](const char *name, const char *path) {
        std::string file = strrchr(path, '/') + 1;
        file.erase(file.size() - 5, 5);

        QAction *action = new QAction(QT_UTF8(name), this);
        action->setProperty("file_name", QT_UTF8(path));
        connect(action, &QAction::triggered, this,
                &AFMainFrame::_ChangeSceneCollection);
        action->setCheckable(true);

        action->setChecked(strcmp(name, cur_name) == 0);

        sceneCollectionMenu->addAction(action);
        count++;
        return true;
    };

    AFLoadSaveManager::EnumSceneCollections(addCollection);

    /* force saving of first scene collection on first run, otherwise
     * no scene collections will show up */
    if (!count)
    {
        AFLoadSaveManager::GetSingletonInstance().
            ForceSaveProjectNow();
        AFLoadSaveManager::EnumSceneCollections(addCollection);
    }

    ui->action_RemoveSceneCollection->setEnabled(count > 1);

    ui->action_PasteFilters->setEnabled(false);
    ui->action_PasteSourceRef->setEnabled(false);
    ui->action_PasteSourceDuplicate->setEnabled(false);
    //
}

void AFMainFrame::ShowMissingFilesDialog(obs_missing_files_t *files)
{
    if (obs_missing_files_count(files) > 0)
    {
        /* When loading the missing files dialog on launch, the
        * window hasn't fully initialized by this point on macOS,
        * so put this at the end of the current task queue. Fixes
        * a bug where the window is behind OBS on startup. */
        QTimer::singleShot(0, [this, files] {
            m_MissDialog = new AFQMissingFilesDialog(files, this);
            m_MissDialog->setAttribute(Qt::WA_DeleteOnClose, true);
            m_MissDialog->show();
            m_MissDialog->raise();
        });
    }
    else
    {
        obs_missing_files_destroy(files);

        auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
        auto& loaderSaver = AFLoadSaveManager::GetSingletonInstance();
        
        /* Only raise dialog if triggered manually */
        if (loaderSaver.CheckDisableSaving() == false)
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                       QT_UTF8(localeManager.Str("MissingFiles.NoMissing.Title")),
                                       QT_UTF8(localeManager.Str("MissingFiles.NoMissing.Text")));
    }
}

bool AFMainFrame::_CopyProfile(const char* fromPartial, const char* to)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
    os_glob_t *glob;
    char path[514];
    char dir[512];
    int ret;

    ret = confManager.GetConfigPath(dir, sizeof(dir), 
                                    "SOOPStudio/basic/profiles/");
    if (ret <= 0) {
        blog(LOG_WARNING, "Failed to get profiles config path");
        return false;
    }

    snprintf(path, sizeof(path), "%s%s/*", dir, fromPartial);

    if (os_glob(path, 0, &glob) != 0) {
        blog(LOG_WARNING, "Failed to glob profile '%s'", fromPartial);
        return false;
    }

    for (size_t i = 0; i < glob->gl_pathc; i++) {
        const char *filePath = glob->gl_pathv[i].path;
        if (glob->gl_pathv[i].directory)
            continue;

        ret = snprintf(path, sizeof(path), "%s/%s", to,
                   strrchr(filePath, '/') + 1);
        if (ret > 0) {
            if (os_copyfile(filePath, path) != 0) {
                blog(LOG_WARNING,
                     "CopyProfile: Failed to "
                     "copy file %s to %s",
                     filePath, path);
            }
        }
    }

    os_globfree(glob);

    return true;
}

void AFMainFrame::_CheckForSimpleModeX264Fallback()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
    
    const char *curStreamEncoder =
        config_get_string(confManager.GetBasic(), "SimpleOutput", "StreamEncoder");
    const char *curRecEncoder =
        config_get_string(confManager.GetBasic(), "SimpleOutput", "RecEncoder");
    
    bool qsv_supported = false;
    bool qsv_av1_supported = false;
    bool amd_supported = false;
    bool nve_supported = false;
#ifdef ENABLE_HEVC
    bool amd_hevc_supported = false;
    bool nve_hevc_supported = false;
    bool apple_hevc_supported = false;
#endif
    bool amd_av1_supported = false;
    bool apple_supported = false;
    bool changed = false;
    size_t idx = 0;
    const char *id;

    while (obs_enum_encoder_types(idx++, &id)) {
        if (strcmp(id, "h264_texture_amf") == 0)
            amd_supported = true;
        else if (strcmp(id, "obs_qsv11") == 0)
            qsv_supported = true;
        else if (strcmp(id, "obs_qsv11_av1") == 0)
            qsv_av1_supported = true;
        else if (strcmp(id, "ffmpeg_nvenc") == 0)
            nve_supported = true;
#ifdef ENABLE_HEVC
        else if (strcmp(id, "h265_texture_amf") == 0)
            amd_hevc_supported = true;
        else if (strcmp(id, "ffmpeg_hevc_nvenc") == 0)
            nve_hevc_supported = true;
#endif
        else if (strcmp(id, "av1_texture_amf") == 0)
            amd_av1_supported = true;
        else if (strcmp(id,
                "com.apple.videotoolbox.videoencoder.ave.avc") ==
             0)
            apple_supported = true;
#ifdef ENABLE_HEVC
        else if (strcmp(id,
                "com.apple.videotoolbox.videoencoder.ave.hevc") ==
             0)
            apple_hevc_supported = true;
#endif
    }

    auto CheckEncoder = [&](const char *&name) {
        if (strcmp(name, SIMPLE_ENCODER_QSV) == 0) {
            if (!qsv_supported) {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
        } else if (strcmp(name, SIMPLE_ENCODER_QSV_AV1) == 0) {
            if (!qsv_av1_supported) {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
        } else if (strcmp(name, SIMPLE_ENCODER_NVENC) == 0) {
            if (!nve_supported) {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
        } else if (strcmp(name, SIMPLE_ENCODER_NVENC_AV1) == 0) {
            if (!nve_supported) {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
#ifdef ENABLE_HEVC
        } else if (strcmp(name, SIMPLE_ENCODER_AMD_HEVC) == 0) {
            if (!amd_hevc_supported) {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
        } else if (strcmp(name, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
            if (!nve_hevc_supported) {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
#endif
        } else if (strcmp(name, SIMPLE_ENCODER_AMD) == 0) {
            if (!amd_supported) {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
        } else if (strcmp(name, SIMPLE_ENCODER_AMD_AV1) == 0) {
            if (!amd_av1_supported) {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
        } else if (strcmp(name, SIMPLE_ENCODER_APPLE_H264) == 0) {
            if (!apple_supported) {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
#ifdef ENABLE_HEVC
        } else if (strcmp(name, SIMPLE_ENCODER_APPLE_HEVC) == 0) {
            if (!apple_hevc_supported) {
                changed = true;
                name = SIMPLE_ENCODER_X264;
                return false;
            }
#endif
        }

        return true;
    };

    if (!CheckEncoder(curStreamEncoder))
        config_set_string(confManager.GetBasic(), 
                          "SimpleOutput", "StreamEncoder",
                          curStreamEncoder);
    if (!CheckEncoder(curRecEncoder))
        config_set_string(confManager.GetBasic(), 
                          "SimpleOutput", "RecEncoder",
                          curRecEncoder);
    if (changed)
        config_save_safe(confManager.GetBasic(), "tmp", nullptr);
}

bool AFMainFrame::_ProfileNeedsRestart(config_t* newConfig, QString& settings)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    

    const char *oldSpeakers =
        config_get_string(confManager.GetBasic(), "Audio", "ChannelSetup");
    uint oldSampleRate =
        config_get_uint(confManager.GetBasic(), "Audio", "SampleRate");

    const char *newSpeakers =
        config_get_string(newConfig, "Audio", "ChannelSetup");
    uint newSampleRate = config_get_uint(newConfig, "Audio", "SampleRate");

    auto appendSetting = [&settings](const char *name) {
        auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
        
        settings += QStringLiteral("\n") +
                    QT_UTF8(localeManager.Str(name));
    };

    bool result = false;
    if (oldSpeakers != NULL && newSpeakers != NULL) {
        result = strcmp(oldSpeakers, newSpeakers) != 0;
        appendSetting("Basic.Settings.Audio.Channels");
    }
    if (oldSampleRate != 0 && newSampleRate != 0) {
        result |= oldSampleRate != newSampleRate;
        appendSetting("Basic.Settings.Audio.SampleRate");
    }

    return result;
}

void AFMainFrame::_ResetProfileData()
{
    AFVideoUtil* videoUtil = GetMainWindow()->GetVideoUtil();
    videoUtil->ResetVideo();
   
    ResetOutputs();

    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    /* load audio monitoring */
    if (obs_audio_monitoring_available())
    {
        const char *device_name = config_get_string(
                                    confManager.GetBasic(), "Audio", "MonitoringDeviceName");
        const char *device_id = config_get_string(confManager.GetBasic(), "Audio",
                                                  "MonitoringDeviceId");

        obs_set_audio_monitoring_device(device_name, device_id);

        blog(LOG_INFO, "Audio monitoring device:\n\tname: %s\n\tid: %s",
             device_name, device_id);
    }
}

bool AFMainFrame::_FindSafeProfileDirName(const std::string &profileName,
                                          std::string &dirName)
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
    char path[512];
    int ret;

    if (AFLoadSaveManager::ProfileExists(profileName.c_str()))
    {
        blog(LOG_WARNING, "Profile '%s' exists", profileName.c_str());
        return false;
    }

    if (confManager.GetFileSafeName(profileName.c_str(), dirName) == false)
    {
        blog(LOG_WARNING, "Failed to create safe file name for '%s'",
             profileName.c_str());
        return false;
    }

    ret = confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/basic/profiles/");
    if (ret <= 0)
    {
        blog(LOG_WARNING, "Failed to get profiles config path");
        return false;
    }

    dirName.insert(0, path);

    if (confManager.GetClosestUnusedFileName(dirName, nullptr) == false)
    {
        blog(LOG_WARNING, "Failed to get closest file name for %s",
             dirName.c_str());
        return false;
    }

    dirName.erase(0, ret);
    return true;
}

bool AFMainFrame::_AskForProfileName(QWidget* parent, std::string& name,
                                     const char* title, const char* text,
                                     const bool showWizard, bool& wizardChecked,
                                     const char* oldName)
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    
    for (;;) {
        bool success = false;

        if (showWizard) {
            success = AFQNameDialog::AskForName(
                parent, title,
                text, name, "");
        } else {
            success = AFQNameDialog::AskForName(
                parent, title, text, name, QT_UTF8(oldName));
        }

        if (!success) {
            return false;
        }

        if (name.empty()) {
            
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                       QT_UTF8(localeManager.Str("NoNameEntered.Title")),
                                       QT_UTF8(localeManager.Str("NoNameEntered.Text")));
            
            continue;
        }
        if (AFLoadSaveManager::ProfileExists(name.c_str())) {
            
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                       QT_UTF8(localeManager.Str("NameExists.Title")),
                                       QT_UTF8(localeManager.Str("NameExists.Text")));
            
            continue;
        }
        break;
    }
    
    
    return true;
}

bool AFMainFrame::_GetSceneCollectionName(QWidget* parent, std::string& name,
                                          std::string& file, const char* oldName)
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    
    bool rename = oldName != nullptr;
    const char *title;
    const char *text;

    if (rename)
    {
        title = localeManager.Str("Basic.Main.RenameSceneCollection.Title");
        text = localeManager.Str("Basic.Main.AddSceneCollection.Text");
    }
    else
    {
        title = localeManager.Str("Basic.Main.AddSceneCollection.Title");
        text = localeManager.Str("Basic.Main.AddSceneCollection.Text");
    }

    for (;;)
    {
        bool success = AFQNameDialog::AskForName(parent, title, text, name,
                                                 QT_UTF8(oldName));
        if (!success)
            return false;
               
        if (name.empty())
        {          
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                       QT_UTF8(localeManager.Str("NoNameEntered.Title")),
                                       QT_UTF8(localeManager.Str("NoNameEntered.Text")));

            continue;
        }
        
        if (AFLoadSaveManager::SceneCollectionExists(name.c_str()))
        {         
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                       QT_UTF8(localeManager.Str("NameExists.Title")),
                                       QT_UTF8(localeManager.Str("NameExists.Text")));
            
            continue;
        }
        break;
    }

    if (confManager.GetUnusedSceneCollectionFile(name, file) == false)
        return false;
    

    
    return true;

}

void AFMainFrame::_ChangeSceneCollection()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& loaderSaver = AFLoadSaveManager::GetSingletonInstance();
    
    
    QAction *action = reinterpret_cast<QAction *>(sender());
    std::string fileName;

    if (!action)
        return;

    fileName = QT_TO_UTF8(action->property("file_name").value<QString>());
    if (fileName.empty())
        return;

    const char *oldName = config_get_string(confManager.GetGlobal(),
                                            "Basic", "SceneCollection");

    if (action->text().compare(QT_UTF8(oldName)) == 0) {
        action->setChecked(true);
        return;
    }

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

    loaderSaver.SaveProjectNow();

    loaderSaver.Load(fileName.c_str());
    RefreshSceneCollections();

    const char *newName = config_get_string(confManager.GetGlobal(),
                                            "Basic", "SceneCollection");
    const char *newFile = config_get_string(confManager.GetGlobal(),
                                            "Basic", "SceneCollectionFile");

    blog(LOG_INFO, "Switched to scene collection '%s' (%s.json)", newName,
         newFile);
    blog(LOG_INFO, "------------------------------------------------");

    if (api)
        api->on_event(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
}
