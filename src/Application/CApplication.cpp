#include "CApplication.h"

#include <string>
#include <sstream>
#include <iostream>

#include <util/profiler.hpp>

#include <QFile>
#include <QProcess>
#include <qevent.h>

#include "qt-wrapper.h"

#include "platform/platform.hpp"
#include "Utils/CJsonController.h"
#include "Common/EventFilterUtils.h"
#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CMakeDirectory.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Log/CLogManager.h"
#include "CoreModel/Log/COBSProfiler.h"
#include "CoreModel/OBSData/CInhibitSleepContext.h"
#include "CoreModel/Service/CService.h"
#include "CAppStyling.h"
#include "UIComponent/CMessageBox.h"

// GPU hint exports for AMD/NVIDIA laptops
#ifdef _MSC_VER
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif


bool							g_bRestart = false;
bool							g_bRestartSafe = false;
bool							g_opt_minimize_tray = false;
bool							g_opt_always_on_top = false;
QStringList						g_Arguments;
AFApplicationInitializer        g_Initializer;


#ifndef _WIN32
    int                         AFQApplication::sigintFd[2];
#endif



QObject* CreateShortcutFilter()
{
	return new OBSEventFilter([](QObject* obj, QEvent* event)
	{
		auto mouse_event = [](QMouseEvent& event) {
			if(!App()->GetEnableHotkeysInFocus() &&
				event.button() != Qt::LeftButton)
				return true;

			obs_key_combination_t hotkey = {0, OBS_KEY_NONE};
			bool pressed = event.type() == QEvent::MouseButtonPress;

			switch(event.button()) {
				case Qt::NoButton:
				case Qt::LeftButton:
				case Qt::RightButton:
				case Qt::AllButtons:
				case Qt::MouseButtonMask:
					return false;

				case Qt::MiddleButton:
					hotkey.key = OBS_KEY_MOUSE3;
					break;

#define MAP_BUTTON(i, j)                       \
case Qt::ExtraButton##i:               \
	hotkey.key = OBS_KEY_MOUSE##j; \
	break;
					MAP_BUTTON(1, 4);
					MAP_BUTTON(2, 5);
					MAP_BUTTON(3, 6);
					MAP_BUTTON(4, 7);
					MAP_BUTTON(5, 8);
					MAP_BUTTON(6, 9);
					MAP_BUTTON(7, 10);
					MAP_BUTTON(8, 11);
					MAP_BUTTON(9, 12);
					MAP_BUTTON(10, 13);
					MAP_BUTTON(11, 14);
					MAP_BUTTON(12, 15);
					MAP_BUTTON(13, 16);
					MAP_BUTTON(14, 17);
					MAP_BUTTON(15, 18);
					MAP_BUTTON(16, 19);
					MAP_BUTTON(17, 20);
					MAP_BUTTON(18, 21);
					MAP_BUTTON(19, 22);
					MAP_BUTTON(20, 23);
					MAP_BUTTON(21, 24);
					MAP_BUTTON(22, 25);
					MAP_BUTTON(23, 26);
					MAP_BUTTON(24, 27);
#undef MAP_BUTTON
			}

			hotkey.modifiers = TranslateQtKeyboardEventModifiers(event.modifiers());

			obs_hotkey_inject_event(hotkey, pressed);
			return true;
		};

		auto key_event = [&](QKeyEvent* event)
		{
			int key = event->key();
			bool enabledInFocus = App()->GetEnableHotkeysInFocus();

			if(key != Qt::Key_Enter && key != Qt::Key_Escape &&
				key != Qt::Key_Return && !enabledInFocus)
				return true;

			QDialog* dialog = qobject_cast<QDialog*>(obj);

			obs_key_combination_t hotkey = {0, OBS_KEY_NONE};
			bool pressed = event->type() == QEvent::KeyPress;

			switch(key) {
				case Qt::Key_Shift:
				case Qt::Key_Control:
				case Qt::Key_Alt:
				case Qt::Key_Meta:
					break;

#ifdef __APPLE__
				case Qt::Key_CapsLock:
					// kVK_CapsLock == 57
					hotkey.key = obs_key_from_virtual_key(57);
					pressed = true;
					break;
#endif

				case Qt::Key_Enter:
				case Qt::Key_Escape:
				case Qt::Key_Return:
					if(dialog && pressed)
						return false;
					if(!enabledInFocus)
						return true;
					/* Falls through. */
				default:
					hotkey.key = obs_key_from_virtual_key(event->nativeVirtualKey());
			}

			if(event->isAutoRepeat())
				return true;

			hotkey.modifiers = TranslateQtKeyboardEventModifiers(event->modifiers());

			obs_hotkey_inject_event(hotkey, pressed);
			return true;
		};

		switch(event->type()) {
			case QEvent::MouseButtonPress:
			case QEvent::MouseButtonRelease:
				return mouse_event(*static_cast<QMouseEvent*>(event));

				/*case QEvent::MouseButtonDblClick:
				case QEvent::Wheel:*/
			case QEvent::KeyPress:
			case QEvent::KeyRelease:
				return key_event(static_cast<QKeyEvent*>(event));

			default:
				return false;
		}
	});
}

QString AFQTranslator::translate(const char*, const char* sourceText,
	const char*, int) const
{
	const char* out = nullptr;
	QString str(sourceText);
	str.replace(" ", "");
	if (!AFLocaleTextManager::GetSingletonInstance().TranslateString(QT_TO_UTF8(str), &out))
		return QString(sourceText);

	return QT_UTF8(out);
}

#ifdef _WIN32
#include <crtdbg.h>
#endif

AFQApplication::AFQApplication(int& argc, char** argv) : QApplication(argc, argv)
{
	// 	setAttribute(Qt::AA_EnableHighDpiScaling);

    auto& logManager = AFLogManager::GetSingletonInstance();
    
	/* fix float handling */
#if defined(Q_OS_UNIX)
	if (!setlocale(LC_NUMERIC, "C"))
		logManager.OBSBaseLog(LOG_WARNING, "Failed to set LC_NUMERIC to C locale");
#endif

#ifndef _WIN32
	/* Handle SIGINT properly */
	socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd);
	snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
	connect(snInt, &QSocketNotifier::activated, this,
			&AFQApplication::qslotProcessSigInt);
#else
	connect(qApp, &QGuiApplication::commitDataRequest, this,
			&AFQApplication::qslotCommitData);
#endif

	AFInhibitSleepContext::GetSingletonInstance().InitContext();
}

AFQApplication::~AFQApplication()
{
#ifdef _WIN32
// Check Windows
//    bool disableAudioDucking =
//        config_get_bool(globalConfig, "Audio", "DisableAudioDucking");
//    if (disableAudioDucking)
//        DisableAudioDucking(false);
#else
    delete snInt;
    close(sigintFd[0]);
    close(sigintFd[1]);
#endif
    
	AFInhibitSleepContext::GetSingletonInstance().FinContext();


	if(false == m_Statistics.isNull()) {
		delete m_Statistics;
		m_Statistics = nullptr;
	}
	if(false == mainView.isNull()) {
		delete mainView;
		mainView = nullptr;
	}
     
    if (m_bLibobsInitialized)
        obs_shutdown();
    
    auto& logManager = AFLogManager::GetSingletonInstance();
    auto* pTmpMainProfiler = logManager.GetMainOBSProfile();
    if (pTmpMainProfiler != nullptr) pTmpMainProfiler->StopProfiler();
    logManager.DeleteMainOBSProfiler();
}

#ifdef _WIN32
void AFQApplication::qslotCommitData(QSessionManager& manager)
{
	auto mainView = App()->GetMainView();
	if (auto main = mainView->GetMainWindow()) 
	{
		QMetaObject::invokeMethod(main, "close", Qt::QueuedConnection);
		manager.cancel();
	}
}
#endif

void AFQApplication::qslotProcessSigInt()
{
	/* This looks weird, but we can't ifdef a Qt slot function so
 * the SIGINT handler simply does nothing on Windows. */
#ifndef _WIN32
	char tmp;
	recv(sigintFd[1], &tmp, sizeof(tmp), 0);

	AFMainFrame* mainView = App()->GetMainView();
	AFMainDynamicComposit* main = reinterpret_cast<AFMainDynamicComposit*>(mainView->GetMainWindow());
	if (main)
		main->close();
#endif
}

void AFQApplication::AppInit()
{
	ProfileScope("AFQApplication::AppInit");

	QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);

    AFMakeDirectory tmpDirMaker;
    
	if (tmpDirMaker.MakeUserDirs() == false)
		throw "Failed to create required user directories";

	if (AFConfigManager::GetSingletonInstance().InitGlobal() == false)
		throw "Failed to initialize global config";

	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();
	if (localeTextManager.InitLocale() == false)
		throw "Failed to load locale";

	if (!localeTextManager.GetCurrentLocaleStr().empty())
		QLocale::setDefault(QLocale(
			QString::fromStdString(localeTextManager.GetCurrentLocaleStr()).replace('-', '_')));

	AFQAppStyling::GetSingletonInstance().InitStyle(palette());
	//if (!InitTheme())
	//	throw "Failed to load theme";

    bool stateActive = (bool)(applicationState() == Qt::ApplicationActive);
    g_Initializer.AppSetGlobalConfig(&tmpDirMaker, stateActive);
}

bool AFQApplication::MainFrameInit()
{
	ProfileScope("AFQApplication::MainFrameInit");

	qRegisterMetaType<VoidFunc>("VoidFunc");

// __APPLE__ Check APPLE
#if !defined(_WIN32) && !defined(__APPLE__)
	if (QApplication::platformName() == "xcb") {
		obs_set_nix_platform(OBS_NIX_PLATFORM_X11_EGL);
		blog(LOG_INFO, "Using EGL/X11");
	}

#ifdef ENABLE_WAYLAND
	if (QApplication::platformName().contains("wayland")) {
		obs_set_nix_platform(OBS_NIX_PLATFORM_WAYLAND);
		setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
		blog(LOG_INFO, "Platform: Wayland");
	}
#endif

	QPlatformNativeInterface* native =
		QGuiApplication::platformNativeInterface();
	obs_set_nix_platform_display(
		native->nativeResourceForIntegration("display"));
#endif

#ifdef __APPLE__
	setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#endif
//

    if (_LibOBSInitialize() == false)
        return false;
	
    m_bLibobsInitialized = true;
    
	// --> browerHWAccel, hideFromCapture,
#if defined(_WIN32) || defined(__APPLE__)
	config_t* globalConfig = AFConfigManager::GetSingletonInstance().GetGlobal();

	bool browserHWAccel =
		config_get_bool(globalConfig, "General", "BrowserHWAccel");

	/*OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_bool(settings, "BrowserHWAccel", browserHWAccel);
	obs_apply_private_data(settings);*/

	blog(LOG_INFO, "Current Date/Time: %s",
		CurrentDateTimeString().c_str());

	blog(LOG_INFO, "Browser Hardware Acceleration: %s",
		browserHWAccel ? "true" : "false");
#endif
#ifdef _WIN32
	bool hideFromCapture = config_get_bool(globalConfig, "BasicWindow",
		"HideOBSWindowsFromCapture");
	blog(LOG_INFO, "Hide OBS windows from screen capture: %s",
		hideFromCapture ? "true" : "false");
#endif
	
	setQuitOnLastWindowClosed(false);

	mainView = new AFMainFrame();
	connect(mainView, &AFMainFrame::destroyed, this, &AFQApplication::quit);

		
	m_Statistics = new AFStatistics;
	m_Statistics->StatisticsInit();
		
	mainView->AFMainFrameInit(mainView->m_bNoUpdate);

	// HotKey
	//connect(this, &QGuiApplication::applicationStateChanged,
	//	[this](Qt::ApplicationState state) {
	//		ResetHotkeyState(state == Qt::ApplicationActive);
	//	});
	//ResetHotkeyState(applicationState() == Qt::ApplicationActive);
	//

	return true;
}

void AFQApplication::AppRelease()
{
}

void AFQApplication::MainFrameRelease()
{
}

int AFQApplication::RunProgramProcess(AFQApplication* app,
                                       AFArgOption* pArgOption, int argc, char* argv[])
{
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__FreeBSD__)
    // Mounted by termina during chromeOS linux container startup
    // https://chromium.googlesource.com/chromiumos/overlays/board-overlays/+/master/project-termina/chromeos-base/termina-lxd-scripts/files/lxd_setup.sh
    os_dir_t* crosDir = os_opendir("/opt/google/cros-containers");
    if (crosDir) {
        QMessageBox::StandardButtons buttons(QMessageBox::Ok);
        QMessageBox mb(QMessageBox::Critical,
            QTStr("ChromeOS.Title"),
            QTStr("ChromeOS.Text"), buttons,
            nullptr);

        mb.exec();
        return 0;
    }
#endif
//
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    auto& logManager = AFLogManager::GetSingletonInstance();

    logManager.CreateLogFile();


    if (pArgOption && pArgOption->GetUncleanShutdown())
        logManager.OBSBaseLog(LOG_WARNING, "[Safe Mode] Unclean shutdown detected!");
    

    if (pArgOption && pArgOption->GetUncleanShutdown() &&
        pArgOption->GetSafeMode() == false)
    {
        auto QTStr = [](const char* lookupVal) {
            return QString::fromUtf8(
                AFLocaleTextManager::GetSingletonInstance().Str(lookupVal));
            };

		bool result = AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, nullptr,
			QT_UTF8(""),
			QTStr("AutoSafeMode.Text"), QTStr("AutoSafeMode.LaunchSafe"));

        bool tmpStateSafeMode = result == QDialog::Accepted;
        if (tmpStateSafeMode)
            logManager.OBSBaseLog(LOG_INFO,
                "[Safe Mode] User has launched in Safe Mode.");
        else
            logManager.OBSBaseLog(LOG_WARNING,
                "[Safe Mode] User elected to launch normally.");

        pArgOption->SetSafeMode(tmpStateSafeMode);
    }

    qInstallMessageHandler([](QtMsgType type,
        const QMessageLogContext&,
        const QString& message) {
            auto& logManager = AFLogManager::GetSingletonInstance();

            switch (type) {
#ifdef _DEBUG
            case QtDebugMsg:
                logManager.OBSBaseLog(LOG_DEBUG, "%s", QT_TO_UTF8(message));
                break;
            case QtInfoMsg:
                logManager.OBSBaseLog(LOG_INFO, "%s", QT_TO_UTF8(message));
                break;
#else
            case QtDebugMsg:
            case QtInfoMsg:
                break;
#endif
            case QtWarningMsg:
                logManager.OBSBaseLog(LOG_WARNING, "%s", QT_TO_UTF8(message));
                break;
            case QtCriticalMsg:
            case QtFatalMsg:
                logManager.OBSBaseLog(LOG_ERROR, "%s", QT_TO_UTF8(message));
                break;
            }
        });

// __APPLE__ Check APPLE
#ifdef __APPLE__
    MacPermissionStatus audio_permission =
        CheckPermission(kAudioDeviceAccess);
    MacPermissionStatus video_permission =
        CheckPermission(kVideoDeviceAccess);
    MacPermissionStatus accessibility_permission =
        CheckPermission(kAccessibility);
    MacPermissionStatus screen_permission =
        CheckPermission(kScreenCapture);

    int permissionsDialogLastShown =
        config_get_int(confManager.GetGlobal(), "General",
                       "MacOSPermissionsDialogLastShown");
    
//    if (permissionsDialogLastShown <
//        MACOS_PERMISSIONS_DIALOG_VERSION) {
//        OBSPermissions check(nullptr, screen_permission,
//            video_permission, audio_permission,
//            accessibility_permission);
//        check.exec();
//    }
#endif
//

#ifdef _WIN32
    if (IsRunningOnWine())
    {
        auto QTStr = [](const char* lookupVal) {
            return QString::fromUtf8(
                AFLocaleTextManager::GetSingletonInstance().Str(lookupVal));
            };


		bool result = AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, nullptr,
			QT_UTF8(""),
			QTStr("Wine.Text"), QTStr("AlreadyRunning.LaunchAnyway"));

        /*QMessageBox mb(QMessageBox::Question,
                       QTStr("Wine.Title"), QTStr("Wine.Text"));
        mb.setTextFormat(Qt::RichText);
        mb.addButton(QTStr("AlreadyRunning.LaunchAnyway"),
                           QMessageBox::AcceptRole);
        QPushButton* closeButton = mb.addButton(QMessageBox::Close);

        mb.setDefaultButton(closeButton);
        mb.exec();*/

        if (result == QDialog::Rejected)
            return 0;
    }
#endif

    
    if (argc > 1)
    {
        std::stringstream stor;
        stor << argv[1];
        for (int i = 2; i < argc; ++i) {
            stor << " " << argv[i];
        }
        logManager.OBSBaseLog(LOG_INFO, "Command Line Arguments: %s",
            stor.str().c_str());
    }
    //

    if (!app->MainFrameInit())
        return 0;

    auto* pTmpMainProfiler = logManager.GetMainOBSProfile();
    if (pTmpMainProfiler != nullptr) pTmpMainProfiler->StopProfiler();

    return app->exec();
}

#ifndef _WIN32
void AFQApplication::SigIntSignalHandler(int s)
{
    /* Handles SIGINT and writes to a socket. Qt will read
     * from the socket in the main thread event loop and trigger
     * a call to the ProcessSigInt slot, where we can safely run
     * shutdown code without signal safety issues. */
    UNUSED_PARAMETER(s);

    char a = 1;
    send(sigintFd[0], &a, sizeof(a), 0);
}
#endif


inline void AFQApplication::ResetHotkeyState(bool inFocus)
{
	obs_hotkey_enable_background_press(
			(inFocus && m_EnableHotkeysInFocus) ||
			(!inFocus && m_EnableHotkeysOutOfFocus));
}

void AFQApplication::UpdateHotkeyFocusSetting(bool resetState)
{
	m_EnableHotkeysInFocus = true;
	m_EnableHotkeysOutOfFocus = true;

	const char* hotkeyFocusType =
		config_get_string(AFConfigManager::GetSingletonInstance().GetGlobal(), "General", "HotkeyFocusType");

	if (astrcmpi(hotkeyFocusType, "DisableHotkeysInFocus") == 0) {
		m_EnableHotkeysInFocus = false;
	}
	else if (astrcmpi(hotkeyFocusType, "DisableHotkeysOutOfFocus") == 0) {
		m_EnableHotkeysOutOfFocus = false;
	}

	if (resetState)
		ResetHotkeyState(applicationState() == Qt::ApplicationActive);
}

void AFQApplication::DisableHotkeys()
{
	m_EnableHotkeysInFocus = false;
	m_EnableHotkeysOutOfFocus = false;
	ResetHotkeyState(applicationState() == Qt::ApplicationActive);

}

inline bool AFQApplication::HotkeysEnabledInFocus() const
{
	return m_EnableHotkeysInFocus;
}

void AFQApplication::SetQss(QString type)
{
	QString qss;
	QString path = QString("../theme/%1/%1.qss").arg(type);

	QFile file(path);
	if (!file.open(QFile::ReadOnly)) {
		return;
	}
	qss = file.readAll();
	mainView->setStyleSheet(qss);
}

#ifdef __APPLE__
#define INPUT_AUDIO_SOURCE "coreaudio_input_capture"
#define OUTPUT_AUDIO_SOURCE "coreaudio_output_capture"
#elif _WIN32
#define INPUT_AUDIO_SOURCE "wasapi_input_capture"
#define OUTPUT_AUDIO_SOURCE "wasapi_output_capture"
#else
#define INPUT_AUDIO_SOURCE "pulse_input_capture"
#define OUTPUT_AUDIO_SOURCE "pulse_output_capture"
#endif

const char* AFQApplication::InputAudioSource() const
{
	return INPUT_AUDIO_SOURCE;
}

const char* AFQApplication::OutputAudioSource() const
{
	return OUTPUT_AUDIO_SOURCE;
}

bool AFQApplication::GetEnableHotkeysInFocus()
{
	return AFConfigManager::GetSingletonInstance().GetEnableHotkeysInFocus();
}

bool AFQApplication::notify(QObject* receiver, QEvent* e)
{
	QWidget* w;
	QWindow* window;
	int windowType;

	if (!receiver->isWidgetType())
		goto skip;

	if (e->type() != QEvent::Show)
		goto skip;

	w = qobject_cast<QWidget*>(receiver);

	if (!w->isWindow())
		goto skip;

	window = w->windowHandle();
	if (!window)
		goto skip;

	windowType = window->flags() & Qt::WindowType::WindowType_Mask;

	if (windowType == Qt::WindowType::Dialog ||
		windowType == Qt::WindowType::Window ||
		windowType == Qt::WindowType::Tool) {
		AFMainFrame* main = reinterpret_cast<AFMainFrame*>(GetMainView());
		if (main)
			main->SetDisplayAffinity(window);
	}

skip:
	return QApplication::notify(receiver, e);
}

bool AFQApplication::_LibOBSInitialize()
{
    auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto& logManager = AFLogManager::GetSingletonInstance();
    
    
    bool resStartUpOBS = false;
    {
        char path[512];

        if (confManager.GetConfigPath(path,
                                      sizeof(path), "SOOPStudio/plugin_config") <= 0)
            resStartUpOBS = false;

        AFOBSProfiler* mainOBSProfiler = logManager.GetMainOBSProfile();
        
        resStartUpOBS = obs_startup(localeTextManager.GetCurrentLocale(),
                                    path,
                                    mainOBSProfiler->GetGetProfilerNameStore());
    }
    
    
    return resStartUpOBS;
}

bool WindowPositionValid(QRect rect)
{
	for (QScreen* screen : QGuiApplication::screens()) {
		if (screen->availableGeometry().intersects(rect))
			return true;
	}
	return false;
}

config_t* GetBasicConfig() {
	return AFConfigManager::GetSingletonInstance().GetBasic();
}
config_t* GetGlobalConfig() {
	return AFConfigManager::GetSingletonInstance().GetGlobal();
}
obs_service_t* GetService() {
	return AFServiceManager::GetSingletonInstance().GetService();
}
const char* Str(const char* lookupVal) {
	auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();
	return localeTextManager.Str(lookupVal);
}
QString QTStr(const char* lookupVal)
{
	return QString::fromUtf8(AFLocaleTextManager::GetSingletonInstance().Str(lookupVal));
}

//QAccessibleInterface* accessibleFactory(const QString& classname,
//	QObject* object)
//{
//	// slider-ignorewheel.cpp
//
//	//if (classname == QLatin1String("VolumeSlider") && object &&
//	//	object->isWidgetType())
//	//	return new VolumeAccessibleInterface(
//	//		static_cast<QWidget*>(object));
//
//	return nullptr;
//}
static int run_program(int argc, char* argv[])
{
	int ret = -1;

	auto& logManager = AFLogManager::GetSingletonInstance();
	logManager.CreateMainOBSProfiler();


	auto* pTmpMainProfiler = logManager.GetMainOBSProfile();
	if (pTmpMainProfiler != nullptr) pTmpMainProfiler->StartProfiler();


#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)) && defined(_WIN32)
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
		Qt::HighDpiScaleFactorRoundingPolicy::Round);
#endif


	QCoreApplication::addLibraryPath(".");

// __APPLE__ Check APPLE
#if __APPLE__
	InstallNSApplicationSubclass();
	InstallNSThreadLocks();

	if (!isInBundle()) {
		logManager.OBSBaseLog(LOG_ERROR,
			"SOOPStudio cannot be run as a standalone binary on macOS. Run the Application bundle instead.");
		return ret;
	}
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
	/* NOTE: The Breeze Qt style plugin adds frame arround QDockWidget with
	 * QPainter which can not be modifed. To avoid this the base style is
	 * enforce to the Qt default style on Linux: Fusion. */

	setenv("QT_STYLE_OVERRIDE", "Fusion", false);

	/* NOTE: Users blindly set this, but this theme is incompatble with Qt6 and
	 * crashes loading saved geometry. Just turn off this theme and let users complain OBS
	 * looks ugly instead of crashing. */
	const char* platform_theme = getenv("QT_QPA_PLATFORMTHEME");
	if (platform_theme && strcmp(platform_theme, "qt5ct") == 0)
		unsetenv("QT_QPA_PLATFORMTHEME");

#if defined(ENABLE_WAYLAND) && defined(USE_XDG)
	/* NOTE: Qt doesn't use the Wayland platform on GNOME, so we have to
	 * force it using the QT_QPA_PLATFORM env var. It's still possible to
	 * use other QPA platforms using this env var, or the -platform command
	 * line option. Remove after Qt 6.3 is everywhere. */

	const char* desktop = getenv("XDG_CURRENT_DESKTOP");
	const char* session_type = getenv("XDG_SESSION_TYPE");
	if (session_type && desktop && strstr(desktop, "GNOME") != nullptr &&
		strcmp(session_type, "wayland") == 0)
		setenv("QT_QPA_PLATFORM", "wayland", false);
#endif
#endif
//

	/* NOTE: This disables an optimisation in Qt that attempts to determine if
	 * any "siblings" intersect with a widget when determining the approximate
	 * visible/unobscured area. However, by Qt's own admission this is slow
	 * and in the case of OBS it significantly slows down lists with many
	 * elements (e.g. Hotkeys) and it is actually faster to disable it. */
	 //qputenv("QT_NO_SUBTRACTOPAQUESIBLINGS", "1");



	AFQApplication app(argc, argv);
	QIcon icon;
	LoadIconFromABSPath("assets/soopicon.ico", icon);
	app.setWindowIcon(icon);
	try
	{
		//QAccessible::installFactory(accessibleFactory);
		//QFontDatabase::addApplicationFont(":/fonts/OpenSans-Regular.ttf");
		//QFontDatabase::addApplicationFont(":/fonts/OpenSans-Bold.ttf");
		//QFontDatabase::addApplicationFont(":/fonts/OpenSans-Italic.ttf");

		// AFConfigManager::InitGlobal, AFLocaleTextManager::InitLocale
		app.AppInit();  // throw exception
		//

		logManager.DeleteOldestFile(false, "SOOPStudio/profiler_data");

		AFQTranslator translator;
		app.installTranslator(&translator);

		/* --------------------------------------- */
		/* check and warn if already running       */

		bool cancel_launch = false;
		bool already_running = false;


#ifdef _WIN32
		RunOnceMutex rom =
#endif
			CheckIfAlreadyRunning(already_running);


		AFArgOption* pTmpArgOption = AFConfigManager::GetSingletonInstance().GetArgOption();
		
		if (already_running == false)
			ret = AFQApplication::RunProgramProcess(&app, pTmpArgOption, argc, argv);
		else
		{
			if (pTmpArgOption && pTmpArgOption->GetMulti() == false)
			{
				auto QTStr = [](const char* lookupVal) {
					return QString::fromUtf8(
						AFLocaleTextManager::GetSingletonInstance().Str(lookupVal));
					};

				bool result = AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, nullptr,
					QT_UTF8(""),
					QTStr("AlreadyRunning.Text"), QTStr("AlreadyRunning.LaunchAnyway"));

				/*QMessageBox mb(QMessageBox::Question,
							   QTStr("AlreadyRunning.Title"),
							   QTStr("AlreadyRunning.Text"));
				mb.addButton(QTStr("AlreadyRunning.LaunchAnyway"),
							 QMessageBox::YesRole);
				QPushButton* cancelButton = mb.addButton(
					QTStr("Cancel"), QMessageBox::NoRole);
				mb.setDefaultButton(cancelButton);

				mb.exec();*/

				cancel_launch = result == QDialog::Rejected;
			}

			if (cancel_launch)
				return 0;

			logManager.CreateLogFile();

			if (pTmpArgOption && pTmpArgOption->GetMulti())
			{
				logManager.OBSBaseLog(LOG_INFO, "User enabled --multi flag and is now "
					 "running multiple instances of OBS.");
			}
			else
			{
				logManager.OBSBaseLog(LOG_WARNING, "================================");
				logManager.OBSBaseLog(LOG_WARNING, "Warning: OBS is already running!");
				logManager.OBSBaseLog(LOG_WARNING, "================================");
				logManager.OBSBaseLog(LOG_WARNING, "User is now running multiple "
					"instances of OBS!");
				/* Clear unclean_shutdown flag as multiple instances
				 * running from the same config will lead to a
				 * false-positive detection.*/

				pTmpArgOption->SetUncleanShutdown(false);
			}

			ret = AFQApplication::RunProgramProcess(&app, pTmpArgOption, argc, argv);
		}
	}
	catch (const char* error) 
	{
		logManager.OBSBaseLog(LOG_ERROR, "%s", error);
		AFErrorBox(nullptr, "%s", error);
	}


	if (g_bRestart || g_bRestartSafe) 
	{
		g_Arguments = qApp->arguments();

		if (g_bRestartSafe) 
			g_Arguments.append("--safe-mode");
		else 
			g_Arguments.removeAll("--safe-mode");
	}

    
    app.AppRelease();
    
    
	return ret;
}

int main(int argc, char* argv[])
{
	g_Initializer.AppEntrySetting(argc, argv);

	int ret = run_program(argc, argv);

	g_Initializer.AppEntryRelease();

	if (g_bRestart || g_bRestartSafe) 
	{
		auto executable = g_Arguments.takeFirst();
		QProcess::startDetached(executable, g_Arguments);
	}

	return ret;
}
