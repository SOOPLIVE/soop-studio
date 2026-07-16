#include "CMainProfile.h"

#include "ui_aneta-main-frame.h"

#include <QDir>

#include "qt-wrappers.hpp"

#include "Application/CApplication.h"

#include "Common/SettingsMiscDef.h"
#include "Common/StudioDefine.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/Output/COutput.h"
#include "MainFrame/AudioSource/CAudioSource.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Service/CService.h"
#include "CoreModel/Action/CHotkeyContext.h"
#include "CoreModel/Video/CVideo.h"
#include "CoreModel/Encoder/CEncoder.h"

#include "UIComponent/CNameDialog.h"
#include "UIComponent/CMessageBox.h"

#include "Utils/OverlayManager.h"

// MARK: Constant Expressions
//const QString SOOPProfilePath = "/" + QString::fromStdString(LOCAL_FOLDER_NAME) + "/basic/profiles/";

QString AFMainProfile::GetProfilePathSection()
{
	return "/" + QString::fromStdString(LOCAL_FOLDER_NAME) + "/basic/profiles/";
}

constexpr std::string_view SOOPProfileSettingsFile = "basic.ini";

// MARK: - Anonymous Namespace
namespace {
	QList<QString> sortedProfiles {};

	void updateSortedProfiles(const OBSProfileCache& profiles)
	{
		const QLocale locale = QLocale::system();
		QList<QString> newList {};

		for(auto [profileName, _] : profiles) {
			QString entry = QString::fromStdString(profileName);
			newList.append(entry);
		}

		std::sort(newList.begin(), newList.end(), [&locale](const QString& lhs, const QString& rhs) -> bool {
			int result = QString::localeAwareCompare(locale.toLower(lhs), locale.toLower(rhs));

			return (result < 0);
		});

		sortedProfiles.swap(newList);
	}
} // namespace


AFMainProfile::AFMainProfile(QWidget* parent)
	:QObject(parent)
{
};

// for api
bool AFMainProfile::qslotCreateNewProfile(const QString& name)
{
	try {
		SetupNewProfile(name.toStdString());
		return true;
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	}
}
bool AFMainProfile::qslotCreateDuplicateProfile(const QString& name)
{
	try {
		SetupDuplicateProfile(name.toStdString());
		return true;
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
		return false;
	}
}
void AFMainProfile::qslotDeleteProfile(const QString& name)
{
	const std::string_view currentProfileName { config_get_string(USERCONFIG, "Basic", "Profile") };
	if(currentProfileName == name.toStdString()) {
		qActionRemoveProfileTriggered();
		return;
	}

	auto foundProfile = GetProfileByName(name.toStdString());
	if(!foundProfile) {
		blog(LOG_ERROR, "Invalid profile name: %s", QT_TO_UTF8(name));
		return;
	}

	RemoveProfile(foundProfile.value());
	profiles.erase(name.toStdString());

	RefreshProfiles();

	config_save_safe(USERCONFIG, "tmp", nullptr);

	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);
}

// action
void AFMainProfile::qActionNewProfileTriggered()
{
	bool useProfileWizard = config_get_bool(USERCONFIG, "Basic", "ConfigOnNewProfile");
	const OBSPromptCallback profilePromptCallback = [this](const OBSPromptResult& result) {
		if(GetProfileByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request { Str("AddProfile.Title"), Str("AddProfile.Text"), "", true, Str("AddProfile.WizardCheckbox"), useProfileWizard };
	OBSPromptResult result = PromptForName(request, profilePromptCallback);
	if(!result.success) {
		return;
	}

	try {
		SetupNewProfile(result.promptValue, result.optionValue);
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}
void AFMainProfile::qActionDupProfileTriggered()
{
	const OBSPromptCallback profilePromptCallback = [this](const OBSPromptResult& result) {
		if(GetProfileByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request {Str("AddProfile.Title"), Str("AddProfile.Text")};
	OBSPromptResult result = PromptForName(request, profilePromptCallback);
	if(!result.success) {
		return;
	}

	try {
		SetupDuplicateProfile(result.promptValue);
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}
void AFMainProfile::qActionRenameProfileTriggered()
{
	const std::string currentProfileName = config_get_string(USERCONFIG, "Basic", "Profile");
	const OBSPromptCallback profilePromptCallback = [this](const OBSPromptResult& result) {
		if(GetProfileByName(result.promptValue)) {
			return false;
		}

		return true;
	};

	const OBSPromptRequest request {Str("RenameProfile.Title"), Str("AddProfile.Text"), currentProfileName};
	OBSPromptResult result = PromptForName(request, profilePromptCallback);
	if(!result.success) {
		return;
	}

	try {
		SetupRenameProfile(result.promptValue);
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
	}
}
void AFMainProfile::qActionRemoveProfileTriggered(bool skipConfirmation)
{
	if(profiles.size() < 2) {
		return;
	}

	OBSProfile currentProfile;

	try {
		currentProfile = GetCurrentProfile();

		if(!skipConfirmation) {
			const QString confirmationText = QTStr("ConfirmRemove.Text").arg(QString::fromStdString(currentProfile.name));
			int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
													MAINFRAME, Str("ConfirmRemove.Title"), confirmationText);
			if(result != 1)
				return;
		}

		MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_PROFILE_CHANGING);

		profiles.erase(currentProfile.name);
	} catch(const std::invalid_argument& error) {
		blog(LOG_ERROR, "%s", error.what());
	} catch(const std::logic_error& error) {
		blog(LOG_ERROR, "%s", error.what());
	}

	const OBSProfile& newProfile = profiles.begin()->second;

	ActivateProfile(newProfile, true);
	RemoveProfile(currentProfile);

#ifdef YOUTUBE_ENABLED
	if(YouTubeAppDock::IsYTServiceSelected() && !youtubeAppDock)
		NewYouTubeAppDock();
#endif

	blog(LOG_INFO, "Switched to profile '%s' (%s)", newProfile.name.c_str(), newProfile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

void AFMainProfile::qActionImportProfileTriggered()
{
	const QString home = QDir::homePath();
	const QString sourceDirectory = SelectDirectory(MAINFRAME, QTStr("Basic.MainMenu.Profile.Import"), home);

	if(!sourceDirectory.isEmpty() && !sourceDirectory.isNull()) {
		const std::filesystem::path sourcePath = std::filesystem::u8path(sourceDirectory.toStdString());
		const std::string directoryName = sourcePath.filename().u8string();

		if(auto profile = GetProfileByDirectoryName(directoryName)) {
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
				QTStr("Basic.MainMenu.Profile.Import"),
				QTStr("Basic.MainMenu.Profile.Exists"));
			return;
		}

		std::filesystem::path profilePath = CONFIG_CONTEXT.GetUserProfilePath();
		std::string destinationPathString;		
		QString currentProfilePath = GetProfilePathSection();
		destinationPathString.reserve(profilePath.u8string().size() + currentProfilePath.toStdString().size() + directoryName.size());
		destinationPathString.append(profilePath.u8string()).append(currentProfilePath.toStdString()).append(directoryName);

		const std::filesystem::path destinationPath = std::filesystem::u8path(destinationPathString);

		try {
			std::filesystem::create_directory(destinationPath);
		} catch(const std::filesystem::filesystem_error& error) {
			blog(LOG_WARNING, "Failed to create profile directory '%s':\n%s", directoryName.c_str(),
				 error.what());
			return;
		}

		const std::array<std::pair<std::string, bool>, 4> profileFiles {{
			{"basic.ini", true},
			{"service.json", false},
			{"streamEncoder.json", false},
			{"recordEncoder.json", false},
		}};

		for(auto& [file, isMandatory] : profileFiles) {
			const std::filesystem::path sourceFile = sourcePath / std::filesystem::u8path(file);

			if(!std::filesystem::exists(sourceFile)) {
				if(isMandatory) {
					blog(LOG_ERROR, "Failed to import profile from directory '%s' - necessary file '%s' not found",
						 directoryName.c_str(), file.c_str());
					return;
				}
				continue;
			}

			const std::filesystem::path destinationFile = destinationPath / std::filesystem::u8path(file);

			try {
				std::filesystem::copy(sourceFile, destinationFile);
			} catch(const std::filesystem::filesystem_error& error) {
				blog(LOG_WARNING, "Failed to copy import file '%s' for profile '%s':\n%s", file.c_str(),
					 directoryName.c_str(), error.what());
				return;
			}
		}

		RefreshProfiles(true);
	}
}
void AFMainProfile::qActionExportProfileTriggered()
{
	const OBSProfile& currentProfile = GetCurrentProfile();
	const QString home = QDir::homePath();
	const QString destinationDirectory = SelectDirectory(MAINFRAME, QTStr("Basic.MainMenu.Profile.Export"), home);
	const std::array<std::string, 4> profileFiles {
		"basic.ini",
		"streamEncoder.json",
		"recordEncoder.json"
	};

	if(!destinationDirectory.isEmpty() && !destinationDirectory.isNull()) {
		const std::filesystem::path sourcePath = currentProfile.path;
		const std::filesystem::path destinationPath =
			std::filesystem::u8path(destinationDirectory.toStdString()) /
			std::filesystem::u8path(currentProfile.directoryName);

		if(!std::filesystem::exists(destinationPath)) {
			std::filesystem::create_directory(destinationPath);
		}

		std::filesystem::copy_options copyOptions = std::filesystem::copy_options::overwrite_existing;

		for(auto& file : profileFiles) {
			const std::filesystem::path sourceFile = sourcePath / std::filesystem::u8path(file);

			if(!std::filesystem::exists(sourceFile)) {
				continue;
			}

			const std::filesystem::path destinationFile = destinationPath / std::filesystem::u8path(file);

			try {
				std::filesystem::copy(sourceFile, destinationFile, copyOptions);
			} catch(const std::filesystem::filesystem_error& error) {
				blog(LOG_WARNING, "Failed to copy export file '%s' for profile '%s'\n%s", file.c_str(),
					 currentProfile.name.c_str(), error.what());
				return;
			}
		}
	}
}

// MARK: - Generic UI Helper Functions
OBSPromptResult AFMainProfile::PromptForName(const OBSPromptRequest& request, const OBSPromptCallback& callback)
{
	OBSPromptResult result;

	for(;;) {
		result.success = false;

		if(request.withOption && !request.optionPrompt.empty()) {
			result.optionValue = request.optionValue;

			result.success = AFQNameDialog::AskForNameWithOption(
				MAINFRAME, request.title.c_str(), request.prompt.c_str(), result.promptValue,
				request.optionPrompt.c_str(), result.optionValue,
				(request.promptValue.empty() ? nullptr : request.promptValue.c_str()));

		} else {
			result.success = AFQNameDialog::AskForName(
				MAINFRAME, request.title.c_str(), request.prompt.c_str(), result.promptValue,
				(request.promptValue.empty() ? nullptr : request.promptValue.c_str()));
		}

		if(!result.success) {
			break;
		}

		if(result.promptValue.empty()) {
			QMessageBox::warning(MAINFRAME, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
			continue;
		}

		if(!callback(result)) {
			QMessageBox::warning(MAINFRAME, QTStr("NameExists.Title"), QTStr("NameExists.Text"));
			continue;
		}

		break;
	}

	return result;
}

void AFMainProfile::SetupNewProfile(const std::string& profileName, bool useWizard)
{
	const OBSProfile& newProfile = CreateProfile(profileName);

	config_set_bool(USERCONFIG, "Basic", "ConfigOnNewProfile", useWizard);

	ActivateProfile(newProfile, true);

	blog(LOG_INFO, "Created profile '%s' (clean, %s)", newProfile.name.c_str(), newProfile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");

	/*if(useWizard) {
		AutoConfig wizard(this);
		wizard.setModal(true);
		wizard.show();
		wizard.exec();
	}*/
}
void AFMainProfile::SetupDuplicateProfile(const std::string& profileName)
{
	const OBSProfile& newProfile = CreateProfile(profileName);
	const OBSProfile& currentProfile = GetCurrentProfile();

	const auto copyOptions = std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing;

	try {
		std::filesystem::copy(currentProfile.path, newProfile.path, copyOptions);
	} catch(const std::filesystem::filesystem_error& error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to copy files for cloned profile: " + newProfile.name);
	}

	ActivateProfile(newProfile);

	blog(LOG_INFO, "Created profile '%s' (duplicate, %s)",
		 newProfile.name.c_str(), newProfile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}
void AFMainProfile::SetupRenameProfile(const std::string& profileName)
{
	const OBSProfile& newProfile = CreateProfile(profileName);
	const OBSProfile currentProfile = GetCurrentProfile();

	const auto copyOptions = std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing;

	try {
		std::filesystem::copy(currentProfile.path, newProfile.path, copyOptions);
	} catch(const std::filesystem::filesystem_error& error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to copy files for profile: " + currentProfile.name);
	}

	profiles.erase(currentProfile.name);

	ActivateProfile(newProfile);
	RemoveProfile(currentProfile);

	blog(LOG_INFO, "Renamed profile '%s' to '%s' (%s)",
		 currentProfile.name.c_str(), newProfile.name.c_str(), newProfile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");

	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_PROFILE_RENAMED);
}

const OBSProfile& AFMainProfile::CreateProfile(const std::string& profileName)
{
	std::string newProfileName;
	QString orgText = {profileName.c_str()};
	QString text = {orgText};
	std::optional<OBSProfile> found;
	int i = 2;
	while((found = GetProfileByName(QT_TO_UTF8(text)))) {
		text = QString("%1_%2").arg(orgText).arg(i++);
	}
	newProfileName = text.toStdString();

	std::string directoryName;
	if(!CONFIG_CONTEXT.GetFileSafeName(newProfileName.c_str(), directoryName)) {
		throw std::invalid_argument("Failed to create safe directory for new profile: " + newProfileName);
	}

	std::filesystem::path userProfilesLocation = USERPROFILE_PATH;
	//
	std::string profileDirectory;
	QString currentProfilePath = GetProfilePathSection();
	profileDirectory.reserve(userProfilesLocation.u8string().size() + currentProfilePath.toStdString().size() + directoryName.size());
	profileDirectory.append(userProfilesLocation.u8string()).append(currentProfilePath.toStdString()).append(directoryName);

	if(!CONFIG_CONTEXT.GetClosestUnusedFileName(profileDirectory, nullptr)) {
		throw std::invalid_argument("Failed to get closest directory name for new profile: " + newProfileName);
	}

	const std::filesystem::path profileDirectoryPath = std::filesystem::u8path(profileDirectory);

	try {
		std::filesystem::create_directory(profileDirectoryPath);
	} catch(const std::filesystem::filesystem_error error) {
		throw std::logic_error("Failed to create directory for new profile: " + profileDirectory);
	}

	const std::filesystem::path profileFile = profileDirectoryPath / std::filesystem::u8path(SOOPProfileSettingsFile);

	auto [iterator, success] =
		profiles.try_emplace(newProfileName, OBSProfile {newProfileName, profileDirectoryPath.filename().u8string(),
								 profileDirectoryPath, profileFile});

	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED);

	return iterator->second;
}
void AFMainProfile::RemoveProfile(OBSProfile profile)
{
	try {
		std::filesystem::remove_all(profile.path);
	} catch(const std::filesystem::filesystem_error& error) {
		blog(LOG_DEBUG, "%s", error.what());
		throw std::logic_error("Failed to remove profile directory: " + profile.directoryName);
	}

	blog(LOG_INFO, "------------------------------------------------");
	blog(LOG_INFO, "Removed profile '%s' (%s)", profile.name.c_str(), profile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}
// MARK: - Profile UI Handling Functions
void AFMainProfile::ChangeProfile()
{
	QAction* action = reinterpret_cast<QAction*>(sender());
	if(!action)
		return;

	const std::string_view currentProfileName {config_get_string(USERCONFIG, "Basic", "Profile")};
	const QVariant qProfileName = action->property("profile_name");
	const std::string selectedProfileName {qProfileName.toString().toStdString()};

	if(currentProfileName == selectedProfileName) {
		action->setChecked(true);
		return;
	}

	const std::optional<OBSProfile> foundProfile = GetProfileByName(selectedProfileName);

	if(!foundProfile) {
		const std::string errorMessage {"Selected profile not found: "};

		throw std::invalid_argument(errorMessage + selectedProfileName.data());
	}

	const OBSProfile& selectedProfile = foundProfile.value();

	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_PROFILE_CHANGING);

	ActivateProfile(selectedProfile, true);

	blog(LOG_INFO, "Switched to profile '%s' (%s)", selectedProfile.name.c_str(),
		 selectedProfile.directoryName.c_str());
	blog(LOG_INFO, "------------------------------------------------");
}

/// Refreshes profile cache data with profile state found on local file system.
void AFMainProfile::RefreshProfileCache()
{
	std::map<std::string, OBSProfile> foundProfiles {};

	QString currentProfilePath = GetProfilePathSection();
	const std::filesystem::path profilesPath = USERPROFILE_PATH / std::filesystem::u8path(currentProfilePath.mid(1).toStdString());
	const std::filesystem::path profileSettingsFile = std::filesystem::u8path(std::string(SOOPProfileSettingsFile));

	if(!std::filesystem::exists(profilesPath)) {
		blog(LOG_WARNING, "Failed to get profiles config path");
		return;
	}

	for(const auto& entry : std::filesystem::directory_iterator(profilesPath)) {
		if(!entry.is_directory()) {
			continue;
		}

		const auto profileCandidate = entry.path() / profileSettingsFile;

		ConfigFile config;
		if(config.Open(profileCandidate.u8string().c_str(), CONFIG_OPEN_EXISTING) != CONFIG_SUCCESS) {
			continue;
		}

		std::string candidateName;
		std::string configName = config_get_string(config, "General", "Name");
		std::string filename = entry.path().filename().u8string();

		candidateName = (configName.empty() ? filename : configName);
		//
		if(foundProfiles.find(candidateName) != foundProfiles.end()) {
			candidateName = filename;
		}
		if(0 != configName.compare(candidateName))
			config_set_string(config, "General", "Name", candidateName.c_str());

		foundProfiles.try_emplace(candidateName,
								  OBSProfile {candidateName, filename, entry.path(), profileCandidate});
	}

	profiles.swap(foundProfiles);
}

void AFMainProfile::RefreshProfiles(bool refreshCache)
{
	std::string_view currentProfileName {config_get_string(USERCONFIG, "Basic", "Profile")};

	QMenu* profilesMenu = MAINFRAME_UI->action_Profile->menu();
	QList<QAction*> menuActions = profilesMenu->actions();

	for(auto& action : menuActions) {
		QVariant variant = action->property("file_name");

		if(variant.typeName() != nullptr) {
			delete action;
		}
	}

	if(refreshCache) {
		RefreshProfileCache();
	}
	updateSortedProfiles(profiles);

	size_t numAddedProfiles = 0;
	for(auto& name : sortedProfiles) {
		const std::string profileName = name.toStdString();
		try {
			const OBSProfile& profile = profiles.at(profileName);
			const QString qProfileName = QString().fromStdString(profileName);

			QAction* action = new QAction(qProfileName, this);
			action->setProperty("profile_name", qProfileName);
			action->setProperty("file_name", QString().fromStdString(profile.directoryName));
			connect(action, &QAction::triggered, this, &AFMainProfile::ChangeProfile);
			action->setCheckable(true);
			action->setChecked(profileName == currentProfileName);

			profilesMenu->addAction(action);

			numAddedProfiles += 1;
		} catch(const std::out_of_range& error) {
			blog(LOG_ERROR, "No profile with name %s found in profile cache.\n%s", profileName.c_str(),
				 error.what());
		}
	}

	MAINFRAME_UI->action_RemoveProfile->setEnabled(numAddedProfiles > 1);
}

void AFMainProfile::ActivateProfile(const OBSProfile& profile, bool reset)
{
	ConfigFile config;
	if(config.Open(profile.profileFile.u8string().c_str(), CONFIG_OPEN_ALWAYS) != CONFIG_SUCCESS) {
		throw std::logic_error("failed to open configuration file of new profile: " +
					   profile.profileFile.string());
	}

	config_set_string(config, "General", "Name", profile.name.c_str());
	config.SaveSafe("tmp");

	std::vector<std::string> restartRequirements;

	config_t* activeConfig = ACTIVECONFIG;
	if(activeConfig) {
		//Auth::Save();

		if(reset) {
			//auth.reset();
			CEFMANAGER.DestroyPanelCookieManager();
#ifdef YOUTUBE_ENABLED
			if(youtubeAppDock) {
				DeleteYouTubeAppDock();
			}
#endif
		}
		restartRequirements = GetRestartRequirements(config);

		OVERLAY_MANAGER.Finalize();
		config_save_safe(activeConfig, "tmp", nullptr);
	}

	CONFIG_CONTEXT.SwapOtherToBasic(config);

	config_t* userConfig = USERCONFIG;
	//
	config_set_string(userConfig, "Basic", "Profile", profile.name.c_str());
	config_set_string(userConfig, "Basic", "ProfileDir", profile.directoryName.c_str());
	config_save_safe(userConfig, "tmp", nullptr);

	CONFIG_CONTEXT.InitBasicConfigDefaults();

	if(reset) {
		UpdateProfileEncoders();
		ResetProfileData();
	}

	RefreshProfiles();
	config_save_safe(userConfig, "tmp", nullptr);

	//UpdateTitleBar();
	MAIN_AUDIOSOURCE->UpdateVolumeControlsDecayRate();

#ifdef YOUTUBE_ENABLED
	if(YouTubeAppDock::IsYTServiceSelected() && !youtubeAppDock)
		NewYouTubeAppDock();
#endif

	MAINFRAME->OnEvent(OBS_FRONTEND_EVENT_PROFILE_CHANGED);

	if(!restartRequirements.empty()) {
		std::string requirements = std::accumulate(
			std::next(restartRequirements.begin()), restartRequirements.end(), restartRequirements[0],
			[](std::string a, std::string b) { return std::move(a) + "\n" + b; });

		int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
			MAINFRAME, "", QTStr("LoadProfileNeedsRestart").arg(requirements.c_str()));
		if(result == QDialog::Accepted) {
			g_bRestart = true;
			MAINFRAME->close();
		}
	}
}
void AFMainProfile::UpdateProfileEncoders()
{
	CONFIG_CONTEXT.InitBasicConfigDefaults2();
	CheckForSimpleModeX264Fallback();
	MigrationNVEncoder();
}
std::vector<std::string> AFMainProfile::GetRestartRequirements(const ConfigFile& config) const
{
	std::vector<std::string> result;

	config_t* activeConfig = ACTIVECONFIG;
	//
	const char* oldSpeakers = config_get_string(activeConfig, "Audio", "ChannelSetup");
	const char* newSpeakers = config_get_string(config, "Audio", "ChannelSetup");

	uint64_t oldSampleRate = config_get_uint(activeConfig, "Audio", "SampleRate");
	uint64_t newSampleRate = config_get_uint(config, "Audio", "SampleRate");

	if(oldSpeakers != NULL && newSpeakers != NULL) {
		if(std::string_view {oldSpeakers} != std::string_view {newSpeakers}) {
			result.emplace_back(Str("Basic.Settings.Audio.Channels"));
		}
	}

	if(oldSampleRate != 0 && newSampleRate != 0) {
		if(oldSampleRate != newSampleRate) {
			result.emplace_back(Str("Basic.Settings.Audio.SampleRate"));
		}
	}

	return result;
}

void AFMainProfile::ResetProfileData()
{
	AFVideoUtil::ResetVideo();
	SERVICE_MANAGER.InitService();
	MAIN_OUTPUT->ResetOutputs();
	HOTKEY_CONTEXT.ClearHotkeys();
	HOTKEY_CONTEXT.CreateHotkeys();
	OVERLAY_MANAGER.Initialize();

	/* load audio monitoring */
	if(obs_audio_monitoring_available()) {
		config_t* activeConfig = ACTIVECONFIG;
		const char* device_name = config_get_string(activeConfig, "Audio", "MonitoringDeviceName");
		const char* device_id = config_get_string(activeConfig, "Audio", "MonitoringDeviceId");

		obs_set_audio_monitoring_device(device_name, device_id);

		blog(LOG_INFO, "Audio monitoring device:\n\tname: %s\n\tid: %s", device_name, device_id);
	}
}
void AFMainProfile::CheckForSimpleModeX264Fallback()
{
	config_t* activeConfig = ACTIVECONFIG;
	//
	const char* curStreamEncoder = config_get_string(activeConfig, "SimpleOutput", "StreamEncoder");
	const char* curRecEncoder = config_get_string(activeConfig, "SimpleOutput", "RecEncoder");
	bool qsv_supported = false;
	bool qsv_av1_supported = false;
	bool amd_supported = false;
	bool nve_supported = false;
#ifdef ENABLE_HEVC
	bool amd_hevc_supported = false;
	bool nve_hevc_supported = false;
	bool apple_hevc_supported = false;
#endif // ENABLE_HEVC
	bool amd_av1_supported = false;
	bool apple_supported = false;
	bool changed = false;
	size_t idx = 0;
	const char* id;

	while(obs_enum_encoder_types(idx++, &id)) {
		if(strcmp(id, "h264_texture_amf") == 0)
			amd_supported = true;
		else if(strcmp(id, "obs_qsv11") == 0)
			qsv_supported = true;
		else if(strcmp(id, "obs_qsv11_av1") == 0)
			qsv_av1_supported = true;
		else if(strcmp(id, "ffmpeg_nvenc") == 0)
			nve_supported = true;
#ifdef ENABLE_HEVC
		else if(strcmp(id, "h265_texture_amf") == 0)
			amd_hevc_supported = true;
		else if(strcmp(id, "ffmpeg_hevc_nvenc") == 0)
			nve_hevc_supported = true;
#endif // ENABLE_HEVC
		else if(strcmp(id, "av1_texture_amf") == 0)
			amd_av1_supported = true;
		else if(strcmp(id, "com.apple.videotoolbox.videoencoder.ave.avc") == 0)
			apple_supported = true;
#ifdef ENABLE_HEVC
		else if(strcmp(id, "com.apple.videotoolbox.videoencoder.ave.hevc") == 0)
			apple_hevc_supported = true;
#endif // ENABLE_HEVC
	}

	auto CheckEncoder = [&](const char*& name) {
		if(strcmp(name, SIMPLE_ENCODER_QSV) == 0) {
			if(!qsv_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if(strcmp(name, SIMPLE_ENCODER_QSV_AV1) == 0) {
			if(!qsv_av1_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if(strcmp(name, SIMPLE_ENCODER_NVENC) == 0) {
			if(!nve_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if(strcmp(name, SIMPLE_ENCODER_NVENC_AV1) == 0) {
			if(!nve_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#ifdef ENABLE_HEVC
		} else if(strcmp(name, SIMPLE_ENCODER_AMD_HEVC) == 0) {
			if(!amd_hevc_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if(strcmp(name, SIMPLE_ENCODER_NVENC_HEVC) == 0) {
			if(!nve_hevc_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#endif // ENABLE_HEVC
		} else if(strcmp(name, SIMPLE_ENCODER_AMD) == 0) {
			if(!amd_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
		} else if(strcmp(name, SIMPLE_ENCODER_AMD_AV1) == 0) {
			if(!amd_av1_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#ifdef __APPLE__
		} else if(strcmp(name, SIMPLE_ENCODER_APPLE_H264) == 0) {
			if(!apple_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#ifdef ENABLE_HEVC
		} else if(strcmp(name, SIMPLE_ENCODER_APPLE_HEVC) == 0) {
			if(!apple_hevc_supported) {
				changed = true;
				name = SIMPLE_ENCODER_X264;
				return false;
			}
#endif // ENABLE_HEVC
#endif // __APPLE__
		}

		return true;
	};

	if(!CheckEncoder(curStreamEncoder))
		config_set_string(activeConfig, "SimpleOutput", "StreamEncoder", curStreamEncoder);
	if(!CheckEncoder(curRecEncoder))
		config_set_string(activeConfig, "SimpleOutput", "RecEncoder", curRecEncoder);
	if(changed) {
		config_save_safe(activeConfig, "tmp", nullptr);
	}
}

// migration nvenc encoder: jim_nvenc -> new nvenc encoder
void AFMainProfile::MigrationNVEncoder()
{
	auto config = ACTIVECONFIG;
	//
	const char* cur_encoder = config_get_string(config, "AdvOut", "Encoder");
	if(!cur_encoder ||
	   0 != strcmp(cur_encoder, "jim_nvenc"))
		return;

	const char* new_encoder = AFEncoderUtil::EncoderAvailable("obs_nvenc_h264_tex") ? "obs_nvenc_h264_tex" : "ffmpeg_nvenc";
	config_set_string(config, "AdvOut", "Encoder", new_encoder);
	config_save_safe(config, "tmp", nullptr);
}

//
const OBSProfile& AFMainProfile::GetCurrentProfile() const
{
	std::string currentProfileName {config_get_string(USERCONFIG, "Basic", "Profile")};
	if(currentProfileName.empty()) {
		currentProfileName = Str("Untitled");
		config_set_string(USERCONFIG, "Basic", "Profile", Str("Untitled"));
		//throw std::invalid_argument("No valid profile name in configuration Basic->Profile");
	}

	const auto& foundProfile = profiles.find(currentProfileName);
	if(foundProfile != profiles.end()) {
		return foundProfile->second;
	} else {
		if(profiles.empty())
			throw std::invalid_argument("Profile not found in profile list: " + currentProfileName);
		else
			return (profiles.begin())->second;
	}
}

std::optional<OBSProfile> AFMainProfile::GetProfileByName(const std::string& profileName) const
{
	auto foundProfile = profiles.find(profileName);
	if(foundProfile == profiles.end()) {
		return {};
	} else {
		return foundProfile->second;
	}
}
std::optional<OBSProfile> AFMainProfile::GetProfileByDirectoryName(const std::string& directoryName) const
{
	for(auto& [iterator, profile] : profiles) {
		if(profile.directoryName == directoryName) {
			return profile;
		}
	}

	return {};
}

void AFMainProfile::WaitDevicePropertiesThread()
{
	if(m_devicePropertiesThread && m_devicePropertiesThread->isRunning())
	{
		m_devicePropertiesThread->wait();
		m_devicePropertiesThread.reset();
	}
}