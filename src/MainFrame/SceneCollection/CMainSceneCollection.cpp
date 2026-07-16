#include "CMainSceneCollection.h"
#include "ui_aneta-main-frame.h"

#include <QDir>

#include "qt-wrappers.hpp"

#include "Application/CApplication.h"

#include "Common/SettingsMiscDef.h"
#include "Common/StudioDefine.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/Profile/CMainProfile.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"

#include "UIComponent/CNameDialog.h"
#include "UIComponent/CMessageBox.h"

#include "PopupWindows/CImporterDialog.h"

//const std::string SOOPSceneCollectionPath = "/" + LOCAL_FOLDER_NAME + "/basic/scenes/";
QString AFMainSceneCollection::GetSceneCollectionPathSection()
{
	return "/" + QString::fromStdString(LOCAL_FOLDER_NAME) + "/basic/scenes/";
}

// MARK: - Anonymous Namespace
namespace {
	QList<QString> sortedSceneCollections {};

	void updateSortedSceneCollections(const OBSSceneCollectionCache& collections)
	{
		const QLocale locale = QLocale::system();
		QList<QString> newList {};

		for(auto [collectionName, _] : collections) {
			QString entry = QString::fromStdString(collectionName);
			newList.append(entry);
		}

		std::sort(newList.begin(), newList.end(), [&locale](const QString& lhs, const QString& rhs) -> bool {
			int result = QString::localeAwareCompare(locale.toLower(lhs), locale.toLower(rhs));

			return (result < 0);
		});

		sortedSceneCollections.swap(newList);
	}

	void cleanBackupCollision(const OBSSceneCollection& collection)
	{
		std::filesystem::path backupFilePath = collection.collectionFile;
		backupFilePath.replace_extension(".json.bak");

		if(std::filesystem::exists(backupFilePath)) {
			try {
				std::filesystem::remove(backupFilePath);
			} catch(std::filesystem::filesystem_error&) {
				throw std::logic_error("Failed to remove pre-existing scene collection backup file: " + backupFilePath.u8string());
			}
		}
	}
} // namespace

//
AFMainSceneCollection::AFMainSceneCollection(QWidget* parent)
    :QObject(parent)
{}

// for api
bool AFMainSceneCollection::qSlotCreateNewSceneCollection(const QString& name)
{
	try {
		SetupNewSceneCollection(name.toStdString());
		return true;
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	}
}

// action
void AFMainSceneCollection::qActionNewSceneCollectionTriggered()
{
	const OBSPromptCallback sceneCollectionCallback = [this](const OBSPromptResult& result) {
		if(GetSceneCollectionByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request {Str("Basic.Main.AddSceneCollection.Title"), Str("Basic.Main.AddSceneCollection.Text")};
	OBSPromptResult result = MAIN_PROFILE->PromptForName(request, sceneCollectionCallback);
	if(!result.success) {
		return;
	}

	try {
		SetupNewSceneCollection(result.promptValue);
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}
void AFMainSceneCollection::qActionDupSceneCollectionTriggered()
{
	const OBSPromptCallback sceneCollectionCallback = [this](const OBSPromptResult& result) {
		if(GetSceneCollectionByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request {
		Str("Basic.Main.AddSceneCollection.Title"),
		Str("Basic.Main.AddSceneCollection.Text")
	};
	OBSPromptResult result = MAIN_PROFILE->PromptForName(request, sceneCollectionCallback);
	if(!result.success) {
		return;
	}

	try {
		SetupDuplicateSceneCollection(result.promptValue);
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}
void AFMainSceneCollection::qActionRenameSceneCollectionTriggered()
{
	const OBSSceneCollection& currentCollection = GetCurrentSceneCollection();

	const OBSPromptCallback sceneCollectionCallback = [this](const OBSPromptResult& result) {
		if(GetSceneCollectionByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request {
		Str("Basic.Main.RenameSceneCollection.Title"),
		Str("Basic.Main.AddSceneCollection.Text"),
		currentCollection.name
	};
	OBSPromptResult result = MAIN_PROFILE->PromptForName(request, sceneCollectionCallback);
	if(!result.success) {
		return;
	}

	try {
		SetupRenameSceneCollection(result.promptValue);
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}
void AFMainSceneCollection::qActionRemoveSceneCollectionTriggered(bool skipConfirmation)
{
	if(m_collections.size() < 2) {
		return;
	}

	OBSSceneCollection currentCollection;

	try {
		currentCollection = GetCurrentSceneCollection();

		if(!skipConfirmation) {
			const QString confirmationText = QTStr("ConfirmRemove.Text").arg(QString::fromStdString(currentCollection.name));
			int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
													MAINFRAME, Str("ConfirmRemove.Title"), confirmationText);
			if(result != 1)
				return;

		}

		MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

		m_collections.erase(currentCollection.name);
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
	}

	const OBSSceneCollection& newCollection = m_collections.begin()->second;

	ActivateSceneCollection(newCollection);
	RemoveSceneCollection(currentCollection);

	blog(LOG_INFO, "Switched to scene collection '%s' (%s)", newCollection.name.c_str(), newCollection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}
void AFMainSceneCollection::qActionImportSceneCollectionTriggered()
{
	AFQImporterDialog imp(MAINFRAME);
	imp.exec();

	RefreshSceneCollections(true);
}
void AFMainSceneCollection::qActionExportSceneCollectionTriggered()
{
	LOADSAVE_CONTEXT.SaveProjectNow();

	const OBSSceneCollection& currentCollection = GetCurrentSceneCollection();
	const QString home = QDir::homePath();
	const QString destinationFileName = SaveFile(MAINFRAME, QTStr("Basic.MainMenu.SceneCollection.Export"),
							 home + "/" + currentCollection.fileName.c_str(),
							 "JSON Files (*.json)");

	if(!destinationFileName.isEmpty() && !destinationFileName.isNull()) {
		const std::filesystem::path sourceFile = currentCollection.collectionFile;
		const std::filesystem::path destinationFile = std::filesystem::u8path(destinationFileName.toStdString());

		OBSDataAutoRelease collection = obs_data_create_from_json_file(sourceFile.u8string().c_str());
		OBSDataArrayAutoRelease sources = obs_data_get_array(collection, "sources");
		if(!sources) {
			blog(LOG_WARNING, "No sources in exported scene collection");
			return;
		}

		obs_data_erase(collection, "sources");

		using OBSDataVector = std::vector<OBSData>;

		OBSDataVector sourceItems;
		obs_data_array_enum(
			sources,
			[](obs_data_t* data, void* vector) -> void {
			OBSDataVector& sourceItems {*static_cast<OBSDataVector*>(vector)};
			sourceItems.push_back(data);
		},
			&sourceItems);

		std::sort(sourceItems.begin(), sourceItems.end(), [](const OBSData& a, const OBSData& b) {
			return astrcmpi(obs_data_get_string(a, "name"), obs_data_get_string(b, "name")) < 0;
		});

		OBSDataArrayAutoRelease newSources = obs_data_array_create();
		for(auto& item : sourceItems) {
			obs_data_array_push_back(newSources, item);
		}

		obs_data_set_array(collection, "sources", newSources);
		obs_data_save_json_pretty_safe(collection, destinationFile.u8string().c_str(), "tmp", "bak");
	}
}


const OBSSceneCollection& AFMainSceneCollection::GetCurrentSceneCollection() const
{
	std::string currentCollectionName {config_get_string(App()->GetUserConfig(), "Basic", "SceneCollection")};

	if(currentCollectionName.empty()) {
		throw std::invalid_argument("No valid scene collection name in configuration Basic->SceneCollection");
	}

	const auto& foundCollection = m_collections.find(currentCollectionName);

	if(foundCollection != m_collections.end()) {
		return foundCollection->second;
	} else {
		throw std::invalid_argument("Scene collection not found in collection list: " + currentCollectionName);
	}
}

std::optional<OBSSceneCollection> AFMainSceneCollection::GetSceneCollectionByName(const std::string& collectionName) const
{
	auto foundCollection = m_collections.find(collectionName);
	if(foundCollection == m_collections.end()) {
		return {};
	} else {
		return foundCollection->second;
	}
}
std::optional<OBSSceneCollection> AFMainSceneCollection::GetSceneCollectionByFileName(const std::string& fileName) const
{
	for(auto& [iterator, collection] : m_collections) {
		if(collection.fileName == fileName) {
			return collection;
		}
	}

	return {};
}

// MARK: - Scene Collection Management Helper Functions

void AFMainSceneCollection::SetupNewSceneCollection(const std::string& collectionName)
{
	if(collectionName.empty()) {
		throw std::logic_error("Cannot create new scene collection with empty collection name");
	}

	const OBSSceneCollection& newCollection = CreateSceneCollection(collectionName);

	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

	cleanBackupCollision(newCollection);
	ActivateSceneCollection(newCollection);

	blog(LOG_INFO, "Created scene collection '%s' (clean, %s)", newCollection.name.c_str(), newCollection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}
void AFMainSceneCollection::SetupDuplicateSceneCollection(const std::string& collectionName)
{
	const OBSSceneCollection& newCollection = CreateSceneCollection(collectionName);
	const OBSSceneCollection& currentCollection = GetCurrentSceneCollection();

	LOADSAVE_CONTEXT.SaveProjectNow();

	const auto copyOptions = std::filesystem::copy_options::overwrite_existing;

	try {
		std::filesystem::copy(currentCollection.collectionFile, newCollection.collectionFile, copyOptions);
	} catch(const std::filesystem::filesystem_error& error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to copy file for cloned scene collection: " + newCollection.name);
	}

	OBSDataAutoRelease collection = obs_data_create_from_json_file(newCollection.collectionFile.u8string().c_str());

	obs_data_set_string(collection, "name", newCollection.name.c_str());

	OBSDataArrayAutoRelease sources = obs_data_get_array(collection, "sources");

	if(sources) {
		obs_data_erase(collection, "sources");

		obs_data_array_enum(
			sources,
			[](obs_data_t* data, void*) -> void {
			const char* uuid = os_generate_uuid();

			obs_data_set_string(data, "uuid", uuid);

			bfree((void*)uuid);
		},
			nullptr);

		obs_data_set_array(collection, "sources", sources);
	}

	obs_data_save_json_safe(collection, newCollection.collectionFile.u8string().c_str(), "tmp", nullptr);

	cleanBackupCollision(newCollection);
	ActivateSceneCollection(newCollection);

	blog(LOG_INFO, "Created scene collection '%s' (duplicate, %s)", newCollection.name.c_str(), newCollection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}
void AFMainSceneCollection::SetupRenameSceneCollection(const std::string& collectionName)
{
	const OBSSceneCollection& newCollection = CreateSceneCollection(collectionName);
	const OBSSceneCollection currentCollection = GetCurrentSceneCollection();

	LOADSAVE_CONTEXT.SaveProjectNow();

	const auto copyOptions = std::filesystem::copy_options::overwrite_existing;

	try {
		std::filesystem::copy(currentCollection.collectionFile, newCollection.collectionFile, copyOptions);
	} catch(const std::filesystem::filesystem_error& error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to copy file for scene collection: " + currentCollection.name);
	}

	m_collections.erase(currentCollection.name);

	OBSDataAutoRelease collection = obs_data_create_from_json_file(newCollection.collectionFile.u8string().c_str());

	obs_data_set_string(collection, "name", newCollection.name.c_str());

	obs_data_save_json_safe(collection, newCollection.collectionFile.u8string().c_str(), "tmp", nullptr);

	cleanBackupCollision(newCollection);
	ActivateSceneCollection(newCollection);
	RemoveSceneCollection(currentCollection);

	blog(LOG_INFO, "Renamed scene collection '%s' to '%s' (%s)", currentCollection.name.c_str(),
		 newCollection.name.c_str(), newCollection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");

	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_RENAMED);
}

// MARK: - Scene Collection File Management Functions

const OBSSceneCollection& AFMainSceneCollection::CreateSceneCollection(const std::string& collectionName)
{
	if(const auto& foundCollection = GetSceneCollectionByName(collectionName)) {
		throw std::invalid_argument("Scene collection already exists: " + collectionName);
	}

	std::string fileName;
	if(!CONFIG_CONTEXT.GetFileSafeName(collectionName.c_str(), fileName)) {
		throw std::invalid_argument("Failed to create safe directory for new scene collection: " +
						collectionName);
	}

	std::filesystem::path userSceneLocation = CONFIG_CONTEXT.GetUserScenesPath();
	QString currentScenePath = GetSceneCollectionPathSection();
	std::string currentScenePathStd = currentScenePath.toStdString();

	std::string collectionFile;
	collectionFile.reserve(userSceneLocation.u8string().size() + currentScenePathStd.size() + fileName.size());
	collectionFile.append(userSceneLocation.u8string()).append(currentScenePathStd).append(fileName);

	if(!CONFIG_CONTEXT.GetClosestUnusedFileName(collectionFile, "json")) {
		throw std::invalid_argument("Failed to get closest file name for new scene collection: " + fileName);
	}

	const std::filesystem::path collectionFilePath = std::filesystem::u8path(collectionFile);

	auto [iterator, success] = m_collections.try_emplace(
		collectionName,
		OBSSceneCollection {collectionName, collectionFilePath.filename().u8string(), collectionFilePath});

	return iterator->second;
}
void AFMainSceneCollection::RemoveSceneCollection(OBSSceneCollection collection)
{
	try {
		std::filesystem::remove(collection.collectionFile);
	} catch(const std::filesystem::filesystem_error& error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to remove scene collection file: " + collection.fileName);
	}

	blog(LOG_INFO, "Removed scene collection '%s' (%s)", collection.name.c_str(), collection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

void AFMainSceneCollection::DeleteSceneCollection(const QString& name)
{
	const std::string_view currentCollectionName { config_get_string(USERCONFIG, "Basic", "SceneCollection")};

	if(currentCollectionName == name.toStdString()) {
		qActionRemoveSceneCollectionTriggered();
		return;
	}

	OBSSceneCollection currentCollection = GetCurrentSceneCollection();
	RemoveSceneCollection(currentCollection);

	m_collections.erase(name.toStdString());

	RefreshSceneCollections();

	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
}
void AFMainSceneCollection::ChangeSceneCollection()
{
	QAction* action = reinterpret_cast<QAction*>(sender());
	if(!action) {
		return;
	}

	const std::string_view currentCollectionName {
		config_get_string(USERCONFIG, "Basic", "SceneCollection")};
	const QVariant qCollectionName = action->property("collection_name");
	const std::string selectedCollectionName {qCollectionName.toString().toStdString()};

	if(currentCollectionName == selectedCollectionName) {
		action->setChecked(true);
		return;
	}

	const std::optional<OBSSceneCollection> foundCollection = GetSceneCollectionByName(selectedCollectionName);

	if(!foundCollection) {
		const std::string errorMessage {"Selected scene collection not found: "};

		throw std::invalid_argument(errorMessage + currentCollectionName.data());
	}

	const OBSSceneCollection& selectedCollection = foundCollection.value();

	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING);

	ActivateSceneCollection(selectedCollection);

	blog(LOG_INFO, "Switched to scene collection '%s' (%s)", selectedCollection.name.c_str(),
		 selectedCollection.fileName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

void AFMainSceneCollection::RefreshSceneCollectionCache()
{
	OBSSceneCollectionCache foundCollections {};

	std::filesystem::path userScenesLocation = CONFIG_CONTEXT.GetUserScenesPath();

	QString currentScenePath = GetSceneCollectionPathSection();
	const std::filesystem::path collectionsPath = userScenesLocation / std::filesystem::u8path(currentScenePath.mid(1).toStdString());

	if(!std::filesystem::exists(collectionsPath)) {
		blog(LOG_WARNING, "Failed to get scene collections config path");
		return;
	}

	for(const auto& entry : std::filesystem::directory_iterator(collectionsPath)) {
		if(entry.is_directory()) {
			continue;
		}

		if(entry.path().extension().u8string() != ".json") {
			continue;
		}

		OBSDataAutoRelease collectionData =
			obs_data_create_from_json_file_safe(entry.path().u8string().c_str(), "bak");

		std::string candidateName;
		std::string collectionName = obs_data_get_string(collectionData, "name");

		if(collectionName.empty()) {
			candidateName = entry.path().stem().u8string();
		} else {
			candidateName = std::move(collectionName);
		}

		foundCollections.try_emplace(candidateName,
						 OBSSceneCollection {candidateName, entry.path().filename().u8string(),
								entry.path()});
	}

	m_collections.swap(foundCollections);
}

void AFMainSceneCollection::RefreshSceneCollections(bool refreshCache)
{
	std::string_view currentCollectionName {config_get_string(USERCONFIG, "Basic", "SceneCollection")};

	QMenu* sceneCollectionMenu = MAINFRAME_UI->action_SceneCollection->menu();
	QList<QAction*> menuActions = sceneCollectionMenu->actions();

	for(auto& action : menuActions) {
		QVariant variant = action->property("file_name");
		if(variant.typeName() != nullptr) {
			delete action;
		}
	}

	if(refreshCache) {
		RefreshSceneCollectionCache();
	}

	updateSortedSceneCollections(m_collections);

	size_t numAddedCollections = 0;
	for(auto& name : sortedSceneCollections) {
		const std::string collectionName = name.toStdString();
		try {
			const OBSSceneCollection& collection = m_collections.at(collectionName);
			const QString qCollectionName = QString().fromStdString(collectionName);

			QAction* action = new QAction(qCollectionName, this);
			action->setProperty("collection_name", qCollectionName);
			action->setProperty("file_name", QString().fromStdString(collection.fileName));
			connect(action, &QAction::triggered, this, &AFMainSceneCollection::ChangeSceneCollection);
			action->setCheckable(true);
			action->setChecked(collectionName == currentCollectionName);

			sceneCollectionMenu->addAction(action);

			numAddedCollections += 1;
		} catch(const std::out_of_range& error) {
			blog(LOG_ERROR, "No scene collection with name %s found in scene collection cache.\n%s",
				 collectionName.c_str(), error.what());
		}
	}

	MAINFRAME_UI->action_RemoveSceneCollection->setEnabled(numAddedCollections > 1);

	MAINFRAME_UI->action_PasteFilters->setEnabled(false);
	MAINFRAME_UI->action_PasteSourceRef->setEnabled(false);
	MAINFRAME_UI->action_PasteSourceDuplicate->setEnabled(false);
}

// MARK: - Scene Collection Management Helper Functions
void AFMainSceneCollection::ActivateSceneCollection(const OBSSceneCollection& collection, bool remigrate)
{
	const std::string currentCollectionName {config_get_string(USERCONFIG, "Basic", "SceneCollection")};

	if(auto foundCollection = GetSceneCollectionByName(currentCollectionName)) {
		if(collection.name != foundCollection.value().name) {
			LOADSAVE_CONTEXT.SaveProjectNow();
		}
	}

	config_set_string(USERCONFIG, "Basic", "SceneCollection", collection.name.c_str());
	config_set_string(USERCONFIG, "Basic", "SceneCollectionFile", collection.fileName.c_str());

	LOADSAVE_CONTEXT.Load(collection.collectionFile.u8string().c_str(), remigrate);

	RefreshSceneCollections();

	//UpdateTitleBar();

	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED);
	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED);
}