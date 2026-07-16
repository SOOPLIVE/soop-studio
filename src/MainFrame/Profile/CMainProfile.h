#pragma once

#include <obs.hpp>

#include <map>
#include <vector>
#include <memory>
#include <future>
#include <filesystem>

#include <qobject.h>
#include <QThread>

#include <util/util.hpp>
#include <util/profiler.hpp>


struct OBSProfile {
	std::string name;
	std::string directoryName;
	std::filesystem::path path;
	std::filesystem::path profileFile;
};

struct OBSPromptResult {
	bool success;
	std::string promptValue;
	bool optionValue;
};

struct OBSPromptRequest {
	std::string title;
	std::string prompt;
	std::string promptValue;
	bool withOption;
	std::string optionPrompt;
	bool optionValue;
};

using OBSPromptCallback = std::function<bool(const OBSPromptResult& result)>;
using OBSProfileCache = std::map<std::string, OBSProfile>;
//
class AFMainProfile : public QObject
{
	Q_OBJECT

public:
	explicit AFMainProfile(QWidget* parent);
	virtual ~AFMainProfile() {}

	// qt slots
public slots:
	// for api
	bool qslotCreateNewProfile(const QString& name);
	bool qslotCreateDuplicateProfile(const QString& name);
	void qslotDeleteProfile(const QString& name);

	// action
	void qActionNewProfileTriggered();
	void qActionDupProfileTriggered();
	void qActionRenameProfileTriggered();
	void qActionRemoveProfileTriggered(bool skipConfirmation = false);
	void qActionImportProfileTriggered();
	void qActionExportProfileTriggered();

	// public function
public:
	// MARK: - Generic UI Helper Functions
	OBSPromptResult PromptForName(const OBSPromptRequest& request, const OBSPromptCallback& callback);

	static QString GetProfilePathSection();

	// MARK: - OBS Profile Management
	void SetupNewProfile(const std::string& profileName, bool useWizard = false);
	void SetupDuplicateProfile(const std::string& profileName);
	void SetupRenameProfile(const std::string& profileName);

	const OBSProfile& CreateProfile(const std::string& profileName);
	void RemoveProfile(OBSProfile profile);

	void ChangeProfile();

	void RefreshProfileCache();

	void RefreshProfiles(bool refreshCache = false);

	void ActivateProfile(const OBSProfile& profile, bool reset = false);
	void UpdateProfileEncoders();
	std::vector<std::string> GetRestartRequirements(const ConfigFile& config) const;
	void ResetProfileData();
	void CheckForSimpleModeX264Fallback();
	void MigrationNVEncoder();

	//
	inline const OBSProfileCache& GetProfileCache() const noexcept { return profiles; };

	const OBSProfile& GetCurrentProfile() const;

	std::optional<OBSProfile> GetProfileByName(const std::string& profileName) const;
	std::optional<OBSProfile> GetProfileByDirectoryName(const std::string& directoryName) const;

	void WaitDevicePropertiesThread();

	// private function
private:

	// private member value
private:
	OBSProfileCache profiles {};

	QScopedPointer<QThread> m_devicePropertiesThread;
};