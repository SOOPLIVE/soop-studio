#pragma once

#include <string>
#include <optional>


// Forward



class AFArgOption final
{
#pragma region QT Field, CTOR/DTOR
public:
	AFArgOption() = default;
	~AFArgOption() = default;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	void			LoadArgProgram(int argc, char* argv[]);
	

	bool			GetMulti() { return m_multi; };
	void			SetMulti(bool value) { m_multi = value; };
	bool			GetUncleanShutdown() { return m_uncleanShutdown; };
	void			SetUncleanShutdown(bool value) { m_uncleanShutdown = value; };
	bool			GetDisableShutdownCheck() { return m_disableShutdownCheck; };
	void			SetDisableShutdownCheck(bool value) { m_disableShutdownCheck = value; };
	bool			GetPortableMode() { return m_portableMode; };
	void			SetPortableMode(bool value) { m_portableMode = value; };
	bool			GetLogVerbose() { return m_logVerbose; };
	void			SetLogVerbose(bool value) { m_logVerbose = value; };
	bool			GetSafeMode() { return m_safeMode; };
	void			SetSafeMode(bool value) { m_safeMode = value; };
	bool			GetDisable3pPlugins() { return m_disable3pPlugins; };
	void			SetDisable3pPlugins(bool value) { m_disable3pPlugins = value; };
	bool			GetAlwaysOnTop() { return m_optAlwaysOnTop; };
	void			SetAlwaysOnTop(bool value) { m_optAlwaysOnTop = value; };
	bool			GetUnfilteredLog() { return m_unfilteredLog; };
	void			SetUnfilteredLog(bool value) { m_unfilteredLog = value; };
	bool			GetStartStreaming() { return m_optStartStreaming; };
	void			SetStartStreaming(bool value) { m_optStartStreaming = value; };
	bool			GetStartRecording() { return m_optStartRecording; };
	void			SetStartRecording(bool value) { m_optStartRecording = value; };
	bool			GetStartReplaybuffer() { return m_optStartReplaybuffer; };
	void			SetStartReplaybuffer(bool value) { m_optStartReplaybuffer = value; };
	bool			GetStartVirtualCam() { return m_optStartVirtualCam; };
	void			SetStartVirtualCam(bool value) { m_optStartVirtualCam = value; };
	std::string&	startingCollection(std::optional<std::string> newValue = std::nullopt) {
		if(newValue.has_value())
			m_optStartingCollection = newValue.value();
		return m_optStartingCollection;
	}
	std::string&	startingProfile(std::optional<std::string> newValue = std::nullopt) {
		if(newValue.has_value())
			m_optStartingProfile = newValue.value();
		return m_optStartingProfile;
	}
	std::string&	startingScene(std::optional<std::string> newValue = std::nullopt) {
		if(newValue.has_value())
			m_optStartingScene = newValue.value();
		return m_optStartingScene;
	}
	bool			GetMinimizeTray() { return m_optMinimizeTray; };
	void			SetMinimizeTray(bool value) { m_optMinimizeTray = value; };
	bool			GetStudioMode() { return m_optStudioMode; };
	void			SetStudioMode(bool value) { m_optStudioMode = value; };
	bool			GetAllowOpenGL() { return m_optAllowOpenGL; };
	void			SetAllowOpenGL(bool value) { m_optAllowOpenGL = value; };
	//bool			GetDisableUpdater() { return m_optDisableUpdater; };
	//void			SetDisableUpdater(bool value) { m_optDisableUpdater = value; };
	bool			GetDisableMissingFilesCheck() { return m_optDisableMissingFilesCheck; };
	void			SetDisableMissingFilesCheck(bool value) { m_optDisableMissingFilesCheck = value; };
	bool			GetSteam() { return m_steam; };
	void			SetSteam(bool value) { m_steam = value; };
#pragma endregion public func

#pragma region private func
private:
	inline bool		_ArgIs(const char* arg, const char* long_form, const char* short_form) {
		return (long_form && strcmp(arg, long_form) == 0) ||
			(short_form && strcmp(arg, short_form) == 0);
	}
#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
	bool			m_multi = false;
	bool			m_uncleanShutdown = false;
	bool			m_disableShutdownCheck = false;
	bool			m_portableMode = false;
    bool			m_logVerbose = false;
	bool			m_safeMode = false;
	bool			m_disable3pPlugins = false;
	bool			m_optAlwaysOnTop = false;
	bool			m_unfilteredLog = false;
	bool			m_optStartStreaming = false;
	bool			m_optStartRecording = false;
	bool			m_optStartReplaybuffer = false;
	bool			m_optStartVirtualCam = false;
	std::string		m_optStartingCollection;
	std::string		m_optStartingProfile;
	std::string		m_optStartingScene;
	bool			m_optMinimizeTray = false;
	bool			m_optStudioMode = false;
	bool			m_optAllowOpenGL = false;
	//bool			m_optDisableUpdater = false;
	bool			m_optDisableMissingFilesCheck = false;
	bool			m_steam = false;
#pragma endregion private member var
};
