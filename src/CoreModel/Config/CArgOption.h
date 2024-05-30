#pragma once

#include <string>


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
	

	bool			GetMulti() { return m_bMulti; };
	void			SetMulti(bool value) { m_bMulti = value; };
	bool			GetUncleanShutdown() { return m_bUncleanShutdown; };
	void			SetUncleanShutdown(bool value) { m_bUncleanShutdown = value; };
	bool			GetDisableShutdownCheck() { return m_bDisableShutdownCheck; };
	void			SetDisableShutdownCheck(bool value) { m_bDisableShutdownCheck = value; };
	bool			GetPortableMode() { return m_bPortableMode; };
	void			SetPortableMode(bool value) { m_bPortableMode = value; };
	bool			GetLogVerbose() { return m_bLogVerbose; };
	void			SetLogVerbose(bool value) { m_bLogVerbose = value; };
	bool			GetSafeMode() { return m_bSafeMode; };
	void			SetSafeMode(bool value) { m_bSafeMode = value; };
	bool			GetDisable3pPlugins() { return m_bDisable3pPlugins; };
	void			SetDisable3pPlugins(bool value) { m_bDisable3pPlugins = value; };
	bool			GetAlwaysOnTop() { return m_optAlwaysOnTop; };
	void			SetAlwaysOnTop(bool value) { m_optAlwaysOnTop = value; };
	bool			GetUnfilteredLog() { return m_bUnfilteredLog; };
	void			SetUnfilteredLog(bool value) { m_bUnfilteredLog = value; };
	bool			GetStartStreaming() { return m_optStartStreaming; };
	void			SetStartStreaming(bool value) { m_optStartStreaming = value; };
	bool			GetStartRecording() { return m_optStartRecording; };
	void			SetStartRecording(bool value) { m_optStartRecording = value; };
	bool			GetStartReplaybuffer() { return m_optStartReplaybuffer; };
	void			SetStartReplaybuffer(bool value) { m_optStartReplaybuffer = value; };
	bool			GetStartVirtualCam() { return m_optStartVirtualCam; };
	void			SetStartVirtualCam(bool value) { m_optStartVirtualCam = value; };
	std::string		GetStartingCollection() { return m_optStartingCollection; };
	void			SetStartingCollection(const char* value) { m_optStartingCollection = value; };
	std::string		GetStartingProfile() { return m_optStartingProfile; };
	void			SetStartingProfile(const char* value) { m_optStartingProfile = value; };
	std::string		GetStartingScene() { return m_optStartingScene; };
	void			SetStartingScene(const char* value) { m_optStartingScene = value; };
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
	bool			GetSteam() { return m_bSteam; };
	void			SetSteam(bool value) { m_bSteam = value; };
#pragma endregion public func

#pragma region private func
private:
	inline bool		_ArgIs(const char* arg, const char* long_form,
						  const char* short_form);
#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
	bool			m_bMulti = false;
	bool			m_bUncleanShutdown = false;
	bool			m_bDisableShutdownCheck = false;
	bool			m_bPortableMode = false;
	bool			m_bLogVerbose = false;
	bool			m_bSafeMode = false;
	bool			m_bDisable3pPlugins = false;
	bool			m_optAlwaysOnTop = false;
	bool			m_bUnfilteredLog = false;
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
	bool			m_bSteam;
#pragma endregion private member var
};

inline bool AFArgOption::_ArgIs(const char* arg, const char* long_form,
							   const char* short_form)
{
	return (long_form && strcmp(arg, long_form) == 0) ||
		   (short_form && strcmp(arg, short_form) == 0);
}