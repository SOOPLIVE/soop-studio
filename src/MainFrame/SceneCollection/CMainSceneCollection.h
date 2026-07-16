#pragma once

#include <obs.hpp>

#include <map>
#include <vector>
#include <memory>
#include <future>
#include <filesystem>

#include <qobject.h>

#include <util/util.hpp>


struct OBSSceneCollection {
	std::string name;
	std::string fileName;
	std::filesystem::path collectionFile;
};

using OBSSceneCollectionCache = std::map<std::string, OBSSceneCollection>;

// MARK: - OBS Scene Collection Management
class AFMainSceneCollection : public QObject
{
	Q_OBJECT

public:
	explicit AFMainSceneCollection(QWidget* parent);
	virtual ~AFMainSceneCollection() {}

	static QString GetSceneCollectionPathSection();
	// qt slots
public slots:
	bool qSlotCreateNewSceneCollection(const QString& name);

	// action
	void qActionNewSceneCollectionTriggered();
	void qActionDupSceneCollectionTriggered();
	void qActionRenameSceneCollectionTriggered();
	void qActionRemoveSceneCollectionTriggered(bool skipConfirmation = false);
	void qActionImportSceneCollectionTriggered();
	void qActionExportSceneCollectionTriggered();

	// public function
public:
	inline const OBSSceneCollectionCache& GetSceneCollectionCache() const noexcept { return m_collections; }
	const OBSSceneCollection& GetCurrentSceneCollection() const;

	std::optional<OBSSceneCollection> GetSceneCollectionByName(const std::string& collectionName) const;
	std::optional<OBSSceneCollection> GetSceneCollectionByFileName(const std::string& fileName) const;

	// MARK: - Main Scene Collection Management Functions
	void SetupNewSceneCollection(const std::string& collectionName);
	void SetupDuplicateSceneCollection(const std::string& collectionName);
	void SetupRenameSceneCollection(const std::string& collectionName);

	// MARK: - Scene Collection Management Helper Functions
	const OBSSceneCollection& CreateSceneCollection(const std::string& collectionName);
	void RemoveSceneCollection(OBSSceneCollection collection);

	// MARK: - Scene Collection File Management Functions
	bool CreateDuplicateSceneCollection(const QString& name);
	void DeleteSceneCollection(const QString& name);
	void ChangeSceneCollection();

	// MARK: - Scene Collection Cache Functions
	void RefreshSceneCollectionCache();

	void RefreshSceneCollections(bool refreshCache = false);

	// MARK: - Scene Collection Management Helper Functions
	void ActivateSceneCollection(const OBSSceneCollection& collection, bool remigrate = false);

	// private function
private:

private:
	OBSSceneCollectionCache m_collections {};

};