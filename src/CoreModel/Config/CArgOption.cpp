#include "CArgOption.h"


#include <obs.hpp>

void AFArgOption::LoadArgProgram(int argc, char* argv[])
{
	obs_set_cmdline_args(argc, argv);

	for (int i = 1; i < argc; i++)
	{
		if (_ArgIs(argv[i], "--multi", "-m"))
		{
			m_bMulti = true;
			m_bDisableShutdownCheck = true;

#if ALLOW_PORTABLE_MODE
		}
		else if (_ArgIs(argv[i], "--portable", "-p"))
		{
			portable_mode = true;

#endif
		}
		else if (_ArgIs(argv[i], "--verbose", nullptr))
		{
			m_bLogVerbose = true;
		}
		else if (_ArgIs(argv[i], "--safe-mode", nullptr))
		{
			m_bSafeMode = true;
		}
		else if (_ArgIs(argv[i], "--only-bundled-plugins", nullptr))
		{
			m_bDisable3pPlugins = true;
		}
		else if (_ArgIs(argv[i], "--disable-shutdown-check", nullptr))
		{
			/* This exists mostly to bypass the dialog during development. */
			m_bDisableShutdownCheck = true;
		}
		else if (_ArgIs(argv[i], "--always-on-top", nullptr))
		{
			m_optAlwaysOnTop = true;
		}
		else if (_ArgIs(argv[i], "--unfiltered_log", nullptr))
		{
			m_bUnfilteredLog = true;
		}
		else if (_ArgIs(argv[i], "--startstreaming", nullptr))
		{
			m_optStartStreaming = true;
		}
		else if (_ArgIs(argv[i], "--startrecording", nullptr))
		{
			m_optStartRecording = true;
		}
		else if (_ArgIs(argv[i], "--startreplaybuffer", nullptr))
		{
			m_optStartReplaybuffer = true;
		}
		else if (_ArgIs(argv[i], "--startvirtualcam", nullptr))
		{
			m_optStartVirtualCam = true;
		}
		else if (_ArgIs(argv[i], "--collection", nullptr))
		{
			if (++i < argc)
				m_optStartingCollection = argv[i];
		}
		else if (_ArgIs(argv[i], "--profile", nullptr))
		{
			if (++i < argc)
				m_optStartingProfile = argv[i];
		}
		else if (_ArgIs(argv[i], "--scene", nullptr))
		{
			if (++i < argc)
				m_optStartingScene = argv[i];
		}
		else if (_ArgIs(argv[i], "--minimize-to-tray", nullptr))
		{
			m_optMinimizeTray = true;
		}
		else if (_ArgIs(argv[i], "--studio-mode", nullptr))
		{
			m_optStudioMode = true;
		}
		else if (_ArgIs(argv[i], "--allow-opengl", nullptr))
		{
			m_optAllowOpenGL = true;
		}
		//else if (_ArgIs(argv[i], "--disable-updater", nullptr))
		//{
		//	opt_disable_updater = true;
		//}
		else if (_ArgIs(argv[i], "--disable-missing-files-check", nullptr))
		{
			m_optDisableMissingFilesCheck = true;
		}
		else if (_ArgIs(argv[i], "--steam", nullptr))
		{
			m_bSteam = true;
		}
//		else if (_ArgIs(argv[i], "--help", "-h")) {
//			std::string help =
//				"--help, -h: Get list of available commands.\n\n"
//				"--startstreaming: Automatically start streaming.\n"
//				"--startrecording: Automatically start recording.\n"
//				"--startreplaybuffer: Start replay buffer.\n"
//				"--startvirtualcam: Start virtual camera (if available).\n\n"
//				"--collection <string>: Use specific scene collection."
//				"\n"
//				"--profile <string>: Use specific profile.\n"
//				"--scene <string>: Start with specific scene.\n\n"
//				"--studio-mode: Enable studio mode.\n"
//				"--minimize-to-tray: Minimize to system tray.\n"
//#if ALLOW_PORTABLE_MODE
//				"--portable, -p: Use portable mode.\n"
//#endif
//				"--multi, -m: Don't warn when launching multiple instances.\n\n"
//				"--safe-mode: Run in Safe Mode (disables third-party plugins, scripting, and WebSockets).\n"
//				"--only-bundled-plugins: Only load included (first-party) plugins\n"
//				"--disable-shutdown-check: Disable unclean shutdown detection.\n"
//				"--verbose: Make log more verbose.\n"
//				"--always-on-top: Start in 'always on top' mode.\n\n"
//				"--unfiltered_log: Make log unfiltered.\n\n"
//				"--disable-updater: Disable built-in updater (Windows/Mac only)\n\n"
//				"--disable-missing-files-check: Disable the missing files dialog which can appear on startup.\n\n";
//
//#ifdef _WIN32
//			MessageBoxA(NULL, help.c_str(), "Help",
//				MB_OK | MB_ICONASTERISK);
//#else
//			std::cout << help
//				<< "--version, -V: Get current version.\n";
//#endif
//			exit(0);
//
//		}
//		else if (_ArgIs(argv[i], "--version", "-V")) {
//			std::cout << "OBS Studio - "
//				<< App()->GetVersionString(false) << "\n";
//			exit(0);
//		}
	}
}
