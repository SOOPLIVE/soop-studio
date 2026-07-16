#include "CApplication.h"

#include <string>
#include <sstream>
#include <iostream>

#include <QFile>
#include <QProcess>
#include <qevent.h>
#include <QComboBox>
#include <QSpinBox>
#include <qframe.h>
#include <QFontDatabase>

#include "qt-wrappers.hpp"

#include <util/profiler.hpp>

#include "platform/platform.hpp"
#include "Utils/CJsonController.h"
#include "Common/EventFilterUtils.h"
#include "Common/StringMiscUtils.h"
#include "Common/StudioDefine.h"

#include "CoreModel/Config/CArgOption.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Config/CMakeDirectory.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Log/CLogManager.h"
#include "CoreModel/Log/COBSProfiler.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"
#include "CoreModel/OBSData/CInhibitSleepContext.h"
#include "CoreModel/Statistics/CStatistics.h"
#include "CoreModel/Action/CHotkeyContext.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Icon/CIconContext.h"

#include "CAppStyling.h"
#include "ViewModel/CApplicationInitializer.h"

#include "clickable-label.hpp"
#include "focus-list.hpp"

#include "UIComponent/CMessageBox.h"
#include "UIComponent/CBasicToggleButton.h"
#include "UIComponent/CBaseClickWidget.h"
#include "UIComponent/CCustomDoubleSpinbox.h"
#include "UIComponent/CBasicHoverWidget.h"

#include "Blocks/SceneSourceDock/CSceneListItem.h"

#include "MainFrame/CMainFrame.h"


#if defined(USE_VLD)
	#include <vld.h>
#endif // USE_VLD

#ifdef _WIN32
#include <crtdbg.h>
#endif

// GPU hint exports for AMD/NVIDIA laptops
#ifdef _MSC_VER
extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 1;
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif


bool g_bRestart = false;
bool g_bRestartSafe = false;
bool g_opt_minimize_tray = false;
bool g_opt_always_on_top = false;
bool g_bIsDownloadingUpdate = false;
QStringList	g_Arguments;
AFApplicationInitializer        g_Initializer;
//

#define MAX_CRASH_REPORT_SIZE (150 * 1024)


#ifndef _WIN32
    int                         AFQApplication::s_sigintFd[2];
#endif

bool RegisterFontTTF()
{
	std::vector<QString> fontLists;
	fontLists.push_back("NotoSansKR-Black.ttf");
	fontLists.push_back("NotoSansKR-Bold.ttf");
	fontLists.push_back("NotoSansKR-ExtraBold.ttf");
	fontLists.push_back("NotoSansKR-ExtraLight.ttf");
	fontLists.push_back("NotoSansKR-Light.ttf");
	fontLists.push_back("NotoSansKR-Medium.ttf");
	fontLists.push_back("NotoSansKR-Regular.ttf");
	fontLists.push_back("NotoSansKR-SemiBold.ttf");
	fontLists.push_back("NotoSansKR-Thin.ttf");

	for (size_t idx = 0; idx < fontLists.size(); idx++) {
		std::string fontFilePath;
		QString fontPath = QString("assets/font/%1").arg(fontLists[idx]);
		GetDataFilePath(fontPath.toStdString().c_str(), fontFilePath);

		int fontId = QFontDatabase::addApplicationFont(fontFilePath.c_str());
		QFontDatabase::applicationFontFamilies(fontId).at(0);
	}

	return true;
}

QString AFQTranslator::translate(const char*, const char* sourceText, const char*, int) const
{
	const char* out = nullptr;
	QString str(sourceText);
	str.replace(" ", "");
	if (!LOCALE_CONTEXT.TranslateString(QT_TO_UTF8(str), &out))
		return QString(sourceText);

	return QT_UTF8(out);
}

bool WinShutDownEventFilter::nativeEventFilter(const QByteArray&, void* message, qintptr* result)
{
#ifdef Q_OS_WIN
	MSG* msg = reinterpret_cast<MSG*>(message);

	if(msg->message == WM_QUERYENDSESSION)
	{
		bool isShutDown = (msg->lParam & ENDSESSION_LOGOFF ? false : true);
		if(isShutDown)
		{
			blog(LOG_INFO, "=== SHUTDOWN ===================");
			auto mainView = App()->GetMainView();
			if(!mainView)
				return false;

			mainView->ShutDown();
		}
	}
#endif
	return false;
}

AFQApplication::AFQApplication(int& argc, char** argv) :
	QApplication(argc, argv),
	//
	m_inhibitSleepContext(std::make_unique<AFInhibitSleepContext>()),
	//
	m_statistics(std::make_unique<AFStatistics>()),
	//
	m_statesApp(std::make_unique<AFStateAppContext>()),
	m_config(std::make_unique<AFConfigManager>()),
	m_hotkey(std::make_unique<AFHotkeyContext>()),
	m_auth(std::make_unique<AFAuthManager>()),
	m_appStyling(std::make_unique<CAppStyling>()),
	m_loadSave(std::make_unique<AFLoadSaveManager>()),
	m_localeManager(std::make_unique<AFLocaleTextManager>()),
	m_iconManager(std::make_unique<AFIconContext>())
{
	m_startTime = std::chrono::steady_clock::now();
	m_Update_Path = "";

	/* fix float handling */
#if defined(Q_OS_UNIX)
	if (!setlocale(LC_NUMERIC, "C"))
		blog((LOG_WARNING, "Failed to set LC_NUMERIC to C locale");
#endif

#ifndef _WIN32
	/* Handle SIGINT properly */
	socketpair(AF_UNIX, SOCK_STREAM, 0, s_sigintFd);
	m_pSNInt = new QSocketNotifier(s_sigintFd[1], QSocketNotifier::Read, this);
	connect(m_pSNInt, &QSocketNotifier::activated, this, &AFQApplication::qslotProcessSigInt);
#else
	connect(qApp, &QGuiApplication::commitDataRequest, this, &AFQApplication::qslotCommitData);
#endif

	installEventFilter(this);
	installNativeEventFilter(new WinShutDownEventFilter);
}

AFQApplication::~AFQApplication()
{
#ifdef _WIN32
#else
    delete m_pSNInt;
    close(s_sigintFd[0]);
    close(s_sigintFd[1]);
#endif
    
	if(false == m_mainView.isNull()) {
		delete m_mainView;
		m_mainView = nullptr;
	}
     
    if (m_libobsInitialized)
        obs_shutdown();

	auto& profiler = LOGMANAGER.GetProfiler();
	profiler.StopProfiler();
}

#ifdef _WIN32
void AFQApplication::qslotCommitData(QSessionManager& manager)
{
	if (auto main = DYNAMIC_COMPOSIT)
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
	recv(s_sigintFd[1], &tmp, sizeof(tmp), 0);

	if (DYNAMIC_COMPOSIT)
		DYNAMIC_COMPOSIT->close();
#endif
}

void AFQApplication::qslotFocusChanged(QWidget* old, QWidget* now)
{
	if (old)
	{
		qDebug() << "old: " << old->objectName();
	}

	if (now)
	{
		qDebug() << "now" << now->objectName();
	}
}

int AFQApplication::RunProgramProcess(AFQApplication* app, int argc, char* argv[])
{
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__FreeBSD__)
	// Mounted by termina during chromeOS linux container startup
	// https://chromium.googlesource.com/chromiumos/overlays/board-overlays/+/master/project-termina/chromeos-base/termina-lxd-scripts/files/lxd_setup.sh
	os_dir_t* crosDir = os_opendir("/opt/google/cros-containers");
	if(crosDir) {
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
	LOGMANAGER.CreateLogFile();

	auto& argOption = ARGOPTION;
	//if(argOption.GetUncleanShutdown())
	//	blog(LOG_WARNING, "[Safe Mode] Unclean shutdown detected!");

	//if(argOption.GetUncleanShutdown() &&
	//   argOption.GetSafeMode() == false)
	//{
	//	auto QTStr = [](const char* lookupVal) {
	//		return QString::fromUtf8(Str(lookupVal));
	//	};

	//	bool result = AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, nullptr,
	//		QT_UTF8(""),
	//		QTStr("AutoSafeMode.Text"), QTStr("AutoSafeMode.LaunchSafe"));

	//	bool tmpStateSafeMode = result == QDialog::Accepted;
	//	if(tmpStateSafeMode)
	//		blog(LOG_INFO, "[Safe Mode] User has launched in Safe Mode.");
	//	else
	//		blog(LOG_WARNING, "[Safe Mode] User elected to launch normally.");

	//	argOption.SetSafeMode(tmpStateSafeMode);
	//}

	qInstallMessageHandler([](QtMsgType type, const QMessageLogContext&, const QString& message)
	{
		switch(type) {
#ifdef _DEBUG
			case QtDebugMsg:
				blog(LOG_DEBUG, "%s", QT_TO_UTF8(message));
				break;
			case QtInfoMsg:
				blog(LOG_INFO, "%s", QT_TO_UTF8(message));
				break;
#else
			case QtDebugMsg:
			case QtInfoMsg:
				break;
#endif
			case QtWarningMsg:
				blog(LOG_WARNING, "%s", QT_TO_UTF8(message));
				break;
			case QtCriticalMsg:
			case QtFatalMsg:
				blog(LOG_ERROR, "%s", QT_TO_UTF8(message));
				break;
		}
	});

#ifdef __APPLE__
	MacPermissionStatus audio_permission = CheckPermission(kAudioDeviceAccess);
	MacPermissionStatus video_permission = CheckPermission(kVideoDeviceAccess);
	MacPermissionStatus accessibility_permission = CheckPermission(kAccessibility);
	MacPermissionStatus screen_permission = CheckPermission(kScreenCapture);

	int permissionsDialogLastShown = config_get_int(APPCONFIG, "General","MacOSPermissionsDialogLastShown");

#endif
//

#ifdef _WIN32
	if(IsRunningOnWine())
	{
		auto QTStr = [](const char* lookupVal) {
			return QString::fromUtf8(Str(lookupVal));
		};


		bool result = AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
															   nullptr, QT_UTF8(""),
															   QTStr("Wine.Text"), QTStr("AlreadyRunning.LaunchAnyway"));

		/*QMessageBox mb(QMessageBox::Question, QTStr("Wine.Title"), QTStr("Wine.Text"));
		mb.setTextFormat(Qt::RichText);
		mb.addButton(QTStr("AlreadyRunning.LaunchAnyway"), QMessageBox::AcceptRole);
		QPushButton* closeButton = mb.addButton(QMessageBox::Close);

		mb.setDefaultButton(closeButton);
		mb.exec();*/

		if(result == QDialog::Rejected)
			return 0;
	}
#endif

	if(argc > 1)
	{
		std::stringstream stor;
		stor << argv[1];
		for(int i = 2; i < argc; ++i) {
			stor << " " << argv[i];
		}
	}

	if(!app->MainFrameInit())
		return 0;

	auto& profiler = LOGMANAGER.GetProfiler();
	profiler.StopProfiler();

	return app->exec();
}

void AFQApplication::AppInit()
{
	ProfileScope("AFQApplication::AppInit");

	//Focus Change Test
	//connect(this, &QApplication::focusChanged, this, &AFQApplication::qslotFocusChanged);

	QString configPath = GetLocalAppDataPath() + "/SOOPStudio/config.ini";
	if (QFile::exists(configPath)) {
		QSettings settings(configPath, QSettings::IniFormat);
		QString updatePath = settings.value("serverPath", "STUDIO").toString();
	}

	RegisterFontTTF();

	QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);
	    
	if (AFMakeDirectoryUtil::MakeUserDirs() == false)
		throw "Failed to create required user directories";

	if (m_config->InitGlobal() == false)
		throw "Failed to initialize global config";

	if (m_localeManager->InitLocale() == false)
		throw "Failed to load locale";

	m_localeStr = "en_US";
	if (!m_localeManager->GetCurrentLocaleStr().empty()) {
		m_localeStr = QString::fromStdString(m_localeManager->GetCurrentLocaleStr()).replace('-', '_');
		QLocale::setDefault(QLocale(m_localeStr));
	}

	m_appStyling->InitStyle(palette());
	//if (!InitTheme())
	//	throw "Failed to load theme";

	config_t* userConfig = USERCONFIG;
	//
	config_set_default_string(userConfig, "Basic", "Profile", Str("Untitled"));
	config_set_default_string(userConfig, "Basic", "ProfileDir", Str("Untitled"));
	config_set_default_string(userConfig, "Basic", "SceneCollection", Str("Untitled"));
	config_set_default_string(userConfig, "Basic", "SceneCollectionFile", Str("Untitled"));
	config_set_default_bool(userConfig, "Basic", "ConfigOnNewProfile", true);

	if(!config_has_user_value(userConfig, "Basic", "Profile")) {
		config_set_string(userConfig, "Basic", "Profile", Str("Untitled"));
		config_set_string(userConfig, "Basic", "ProfileDir", Str("Untitled"));
	}

	if(!config_has_user_value(userConfig, "Basic", "SceneCollection")) {
		config_set_string(userConfig, "Basic", "SceneCollection", Str("Untitled"));
		config_set_string(userConfig, "Basic", "SceneCollectionFile", Str("Untitled"));
	}

    bool stateActive = (bool)(applicationState() == Qt::ApplicationActive);
	g_Initializer.AppSetGlobalConfig(stateActive);
}

bool AFQApplication::MainFrameInit()
{
	ProfileScope("AFQApplication::MainFrameInit");

	qRegisterMetaType<VoidFunc>("VoidFunc");
	setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

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

	QPlatformNativeInterface* native = QGuiApplication::platformNativeInterface();
	obs_set_nix_platform_display(native->nativeResourceForIntegration("display"));
#endif

#ifdef __APPLE__
	setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#endif

    if (_LibOBSInitialize() == false)
        return false;
	
    m_libobsInitialized = true;
    
#if defined(_WIN32) || defined(__APPLE__)
	bool browserHWAccel = config_get_bool(APPCONFIG, "General", "BrowserHWAccel");

	/*OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_bool(settings, "BrowserHWAccel", browserHWAccel);
	obs_apply_private_data(settings);*/

	blog(LOG_INFO, "Current Date/Time: %s", CurrentDateTimeString().c_str());
	blog(LOG_INFO, "Browser Hardware Acceleration: %s", browserHWAccel ? "true" : "false");
#endif // defined(_WIN32) || defined(__APPLE__)
#ifdef _WIN32
	bool hideFromCapture = config_get_bool(USERCONFIG, "BasicWindow", "HideOBSWindowsFromCapture");
	blog(LOG_INFO, "Hide OBS windows from screen capture: %s", hideFromCapture ? "true" : "false");
#endif // _WIN32

	blog(LOG_INFO, "Qt Version: %s (runtime), %s (compiled)", qVersion(), QT_VERSION_STR);
	blog(LOG_INFO, "Portable mode: %s", ARGOPTION.GetPortableMode() ? "true" : "false");
	
	setQuitOnLastWindowClosed(false); 
	
#ifdef _WIN32
#ifndef _DEBUG
	_InitializeWithArguments();
#endif
	//if (!m_noUpdate)
	//	return true;
#endif

	// get migration user
	if(config_get_bool(APPCONFIG, "General", "MigrationUser")) {
		m_Freecshot_Type = "3";
	}

	bool conversionOk = false;

	m_mainView = new AFMainFrame(nullptr, Qt::WindowFlags(), m_Update_Path);
	connect(m_mainView, &AFMainFrame::destroyed, this, &AFQApplication::quit);


#ifdef __APPLE__
	if (g_bIsDownloadingUpdate)
		return;
#endif

	m_statistics->StatisticsInit();
	{
		const char* sceneCollectionFile = config_get_string(USERCONFIG, "Basic", "SceneCollectionFile");
		if (sceneCollectionFile) {
			std::string name = sceneCollectionFile;
			if (name.find(".json") == std::string::npos) {
				std::string newName;
				if (!CONFIG_CONTEXT.GetFileSafeName(name.c_str(), newName)) {
					blog(LOG_WARNING, "Failed to create safe file name for '%s'", newName.c_str());
				}

				newName += ".json";

				char basePath[512] = { 0 };
				if (GetAppConfigPath(basePath, sizeof(basePath), (LOCAL_FOLDER_NAME + "/basic/scenes/").c_str()) > 0) {
					std::string oldFilePath = std::string(basePath) + name + ".json";
					std::string newNamePath = std::string(basePath) + newName;

					if (oldFilePath != newNamePath) {
						if (os_file_exists(oldFilePath.c_str())) {
							if (os_rename(oldFilePath.c_str(), newNamePath.c_str()) == 0) {
								blog(LOG_INFO, "[Migration] Renamed scene file: %s -> %s",
									oldFilePath.c_str(), newNamePath.c_str());
							}
						}
					}
				}
				config_set_string(USERCONFIG, "Basic", "SceneCollectionFile", newName.c_str());
				config_save_safe(USERCONFIG, "tmp", nullptr);
			}
			else {
				char basePath[512] = { 0 };
				if (GetAppConfigPath(basePath, sizeof(basePath), (LOCAL_FOLDER_NAME + "/basic/scenes/").c_str()) > 0) {
					std::string oldFilePath = std::string(basePath) + name + ".json";
					if (os_file_exists(oldFilePath.c_str())) {

						std::string newNamePath = std::string(basePath) + name;
						if (os_rename(oldFilePath.c_str(), newNamePath.c_str()) == 0) {
							blog(LOG_INFO, "[Migration] Renamed scene file: %s -> %s",
								oldFilePath.c_str(), newNamePath.c_str());
						}
					}
				}
			}
		}
	}

	bool retVal = m_mainView->AFMainFrameInit(m_noUpdate,
											  m_streamerID.toStdString(),
											  m_cookie.toStdString(),
											  m_install_Type.toStdString(),
											  m_Freecshot_Type.toStdString());
	if (retVal)
	{
		QApplication::processEvents();
		m_mainView->show();
		m_mainView->raise(); 
		m_mainView->activateWindow();
		m_mainView->ConnectSignalForScreen();
				
		connect(this, &QGuiApplication::applicationStateChanged,
			[this](Qt::ApplicationState state) {
				ResetHotkeyState(state == Qt::ApplicationActive);
			});
		ResetHotkeyState(applicationState() == Qt::ApplicationActive);
		
#ifdef _WIN32
		//UpdaterKill();
#endif
		SetVersionInfo();
	}
	else
	{
		m_mainView->close();
	}
	return true;
}

void AFQApplication::SetVersionInfo()
{
	QString filePath = QCoreApplication::applicationFilePath();
	QFile file(filePath);

	if (!file.exists()) return;

	// Get version information
	QVersionNumber version = QVersionNumber::fromString(QCoreApplication::applicationVersion());
	if (version.isNull()) return;

	// Extract version components
	int majorVer = version.majorVersion();
	int minorVer = version.minorVersion();
	int buildNum = version.microVersion();
	int revisionNum = 0; // Qt does not provide revision number directly

	// Format version string
	QString versionString = QString("%1.%2.%3.%4").arg(majorVer).arg(minorVer).arg(buildNum).arg(revisionNum);
	config_set_string(APPCONFIG, "General", "Version", versionString.toStdString().c_str());
	config_save(APPCONFIG);
}

//
AFLogManager& AFQApplication::GetLogManager() const
{
	return g_Initializer.GetLogManager();
}
AFArgOption& AFQApplication::GetArgOption() const
{
	return g_Initializer.GetArgOption();
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
    send(s_sigintFd[0], &a, sizeof(a), 0);
}
#endif

config_t* AFQApplication::GetAppConfig() const
{
	return m_config->GetAppConfig();
}
config_t* AFQApplication::GetUserConfig() const
{
	return m_config->GetUserConfig();
}
config_t* AFQApplication::GetActiveConfig() const
{
	return m_config->GetActiveConfig();
}

void AFQApplication::ResetHotkeyState(bool inFocus)
{
	obs_hotkey_enable_background_press(
			(inFocus && m_enableHotkeysInFocus) ||
			(!inFocus && m_enableHotkeysOutOfFocus));
}

void AFQApplication::UpdateHotkeyFocusSetting(bool resetState)
{
	m_enableHotkeysInFocus = true;
	m_enableHotkeysOutOfFocus = true;

	const char* hotkeyFocusType = config_get_string(USERCONFIG, "General", "HotkeyFocusType");
	if (astrcmpi(hotkeyFocusType, "DisableHotkeysInFocus") == 0) {
		m_enableHotkeysInFocus = false;
	}
	else if (astrcmpi(hotkeyFocusType, "DisableHotkeysOutOfFocus") == 0) {
		m_enableHotkeysOutOfFocus = false;
	}

	if (resetState)
		ResetHotkeyState(applicationState() == Qt::ApplicationActive);
}

void AFQApplication::DisableHotkeys()
{
	m_enableHotkeysInFocus = false;
	m_enableHotkeysOutOfFocus = false;
	ResetHotkeyState(applicationState() == Qt::ApplicationActive);

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
	m_mainView->setStyleSheet(qss);
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

bool AFQApplication::CheckClickableWidget(QObject* obj, bool disableMove)
{
	bool retVal = false;

	auto mo = obj->metaObject();
	if (mo->inherits(&QPushButton::staticMetaObject) || 
		mo->inherits(&QComboBox::staticMetaObject) ||
		mo->inherits(&QCheckBox::staticMetaObject) ||
		mo->inherits(&QSlider::staticMetaObject) || 
		mo->inherits(&AFQToggleButton::staticMetaObject) || 
		mo->inherits(&AFQBaseClickWidget::staticMetaObject) || 
		mo->inherits(&ClickableLabel::staticMetaObject) ||
		mo->inherits(&QMenu::staticMetaObject) || 
		mo->inherits(&QTabBar::staticMetaObject) ||
		mo->inherits(&AFQHoverWidget::staticMetaObject) ||
		mo->inherits(&AFQSceneListItem::staticMetaObject) ||
		mo->inherits(&QPlainTextEdit::staticMetaObject) ||
		obj->property("showHandCursor").toBool())
	{
		retVal = true;
	}

	if (disableMove)
	{
		if (mo->inherits(&QTextBrowser::staticMetaObject) ||
			mo->inherits(&QScrollBar::staticMetaObject) ||
			mo->inherits(&QLineEdit::staticMetaObject) ||
			mo->inherits(&QListWidget::staticMetaObject) ||
			mo->inherits(&FocusList::staticMetaObject) ||
			mo->inherits(&QListView::staticMetaObject) ||
			mo->inherits(&QDoubleSpinBox::staticMetaObject) ||
			mo->inherits(&QScrollArea::staticMetaObject) ||
			mo->inherits(&QSpinBox::staticMetaObject))
		{
			retVal = true;
		}
	}

	return retVal;
}

bool AFQApplication::eventFilter(QObject* obj, QEvent* event)
{
	if (!obj->isWidgetType() || obj->property("notShowHandCursor").toBool())
		return QApplication::eventFilter(obj, event);

	//if (true) {
	//	for (auto o = obj; o; o = o->parent())
	//	{
	//		qDebug() << "event = " << event->type();
	//		qDebug() << "obj name = " << o->objectName() << "class name = " << o->metaObject()->className();
	//	}
	//}
	auto mo = obj->metaObject();
	if(CheckClickableWidget(obj))
	{
		//#1795, when click close button to close dialog, when window deacitve, maybe trigger QEvent::Enter after deactive window.
		if (static_cast<QWidget*>(obj)->isVisible() && event->type() == QEvent::Enter) 
		{
			static_cast<QWidget*>(obj)->setCursor(Qt::PointingHandCursor);
		}
		else if (event->type() == QEvent::Leave) 
		{
			if (mo->inherits(&QMenu::staticMetaObject) && static_cast<QWidget*>(obj)->isVisible()) 
			{
				return QApplication::eventFilter(obj, event);
			}
			static_cast<QWidget*>(obj)->unsetCursor();
		}
	}

	return QApplication::eventFilter(obj, event);
}
//
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
		if (MAINFRAME)
			MAINFRAME->SetDisplayAffinity(window);
	}

skip:
	return QApplication::notify(receiver, e);
}

bool AFQApplication::_LibOBSInitialize()
{
	bool resStartUpOBS = false;
	{
		char path[512];

		if(GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/plugin_config").c_str()) <= 0)
			resStartUpOBS = false;

		auto& profiler = LOGMANAGER.GetProfiler();
		resStartUpOBS = obs_startup(m_localeManager->GetCurrentLocale(), path, profiler.GetGetProfilerNameStore());
	}
	return resStartUpOBS;
}
void AFQApplication::_InitializeWithArguments()
{
	const QStringList& args = QCoreApplication::arguments();

	m_noUpdate = args.contains("--no-update");
	m_type = "1";
	m_Freecshot_Type = "0";
	m_Update_Path = "";

	for(int i = 1; i < args.size(); ++i) {
		if(args[i].startsWith("--soop_id=")) {
			QString idValue = args[i].mid(QString("--soop_id=").length());
			if(!idValue.isEmpty()) {
				m_streamerID = idValue;
				blog(LOG_INFO, "Parsed StreamerID: %s", m_streamerID.constData());
			}
		} else if(args[i].startsWith("--soop_cookie=")) {
			QString cookieValue = args[i].mid(QString("--soop_cookie=").length());

			if(!cookieValue.isEmpty()) {
				if(cookieValue.contains("AuthTicket%3D")) {
					QString decoded = QUrl::fromPercentEncoding(cookieValue.toUtf8());
					m_cookie = decoded;
				} else {
					m_cookie = cookieValue;
				}
			}
		} else if(args[i].startsWith("--soop_type=")) {
			QString typeValue = args[i].mid(QString("--soop_type=").length());
			if(!typeValue.isEmpty()) {

				m_type = QUrl::fromPercentEncoding(typeValue.toUtf8());
				blog(LOG_INFO, "Parsed type: %s ", m_type.constData());
			}
		} else if (args[i].startsWith("--soop_installer=")) {
			QString typeValue = args[i].mid(QString("--soop_installer=").length());
			if (!typeValue.isEmpty()) {

				m_install_Type = QUrl::fromPercentEncoding(typeValue.toUtf8());
				blog(LOG_INFO, "Parsed install_Type: %s", m_install_Type.constData());
			}
		} else if(args[i].startsWith("--freecshot=")) {
			QString typeValue = args[i].mid(QString("--freecshot=").length());
			if(!typeValue.isEmpty()) {

				m_Freecshot_Type = QUrl::fromPercentEncoding(typeValue.toUtf8());
				blog(LOG_INFO, "Parsed Freecshot_Type: %s ", m_Freecshot_Type.constData());

				if("3" == m_Freecshot_Type) {
					config_set_bool(APPCONFIG, "General", "MigrationUser", true);
					config_save_safe(APPCONFIG, "tmp", nullptr);
				}
			}
		}
		else if (args[i].startsWith("--soop_update_path=")) {
			QString typeValue = args[i].mid(QString("--soop_update_path=").length());
			if (!typeValue.isEmpty()) {

				m_Update_Path = QUrl::fromPercentEncoding(typeValue.toUtf8());
				blog(LOG_INFO, "Parsed Update_Path: %s ", m_Update_Path.constData());
			}
		}
	}
}
//

bool WindowPositionValid(QRect rect)
{
	for (QScreen* screen : QGuiApplication::screens()) {
		if (screen->availableGeometry().intersects(rect))
			return true;
	}
	return false;
}

const char* Str(const char* lookupVal) {
	return LOCALE_CONTEXT.Str(lookupVal);
}
QString QTStr(const char* lookupVal)
{
	return QString::fromUtf8(LOCALE_CONTEXT.Str(lookupVal));
}

std::string GetChannelId(const char* platform)
{
	std::string channelId;
	bool success = AUTH_CONTEXT.GetChannelID(platform, channelId);

	return channelId;
}

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

			if (pressed &&
				key == Qt::Key_V &&
				event->modifiers() == Qt::ControlModifier)
			{
				QWidget* focusWidget = QApplication::focusWidget();
				bool isInputWidget = false;

				if (focusWidget) {
					isInputWidget =
						qobject_cast<QLineEdit*>(focusWidget) ||
						qobject_cast<QTextEdit*>(focusWidget) ||
						qobject_cast<QPlainTextEdit*>(focusWidget) ||
						qobject_cast<QAbstractSpinBox*>(focusWidget);
				}
#ifdef _WIN32
				if (!isInputWidget) {
					HWND hwnd = GetFocus();
					if (hwnd) {
						char className[256] = { 0 };
						GetClassNameA(hwnd, className, sizeof(className));
						if (strstr(className, "Chrome") != nullptr ||
							strstr(className, "Cef") != nullptr)
							isInputWidget = true;
					}
				}
#endif
				if (isInputWidget)
					return false;

				if (!MAINFRAME)
					return false;
				QMetaObject::invokeMethod(MAINFRAME, "qslotPasteClipboardAsSource",
					Qt::QueuedConnection);
				return true;
			}

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

//namespace AFConfigUtil {
#define CONFIG_PATH BASE_PATH "/config"
QString GetLocalAppDataPath() {
	QString localAppDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	int lastSlashIndex = localAppDataPath.lastIndexOf("/");
	if(lastSlashIndex != -1) {
		localAppDataPath = localAppDataPath.left(lastSlashIndex);
	}
	return localAppDataPath;
}
int GetAppConfigPath(char* path, size_t size, const char* name)
{
#if ALLOW_PORTABLE_MODE
	if(ARGOPTION.GetPortableMode())
	{
		if(name && *name)
			return snprintf(path, size, CONFIG_PATH "/%s", name);
		else
			return snprintf(path, size, CONFIG_PATH);
	} else
	{
		return os_get_config_path(path, size, name);
	}
#else
	return os_get_config_path(path, size, name);
#endif
}
char* GetAppConfigPathPtr(const char* name)
{
#if ALLOW_PORTABLE_MODE
	if(ARGOPTION.GetPortableMode())
	{
		char path[512];

		if(snprintf(path, sizeof(path), CONFIG_PATH "/%s", name) > 0)
			return bstrdup(path);
		else
			return NULL;
	} else
	{
		return os_get_config_path_ptr(name);
	}
#else
	return os_get_config_path_ptr(name);
#endif
}
int GetProgramDataPath(char* path, size_t size, const char* name)
{
	return os_get_program_data_path(path, size, name);
}
char* GetProgramDataPathPtr(const char* name)
{
	return os_get_program_data_path_ptr(name);
}
int GetProfilePath(char* path, size_t size, const char* file)
{
	char profiles_path[512];
	const char* profile = config_get_string(USERCONFIG, "Basic", "ProfileDir");
	int ret;

	if(!profile)
		return -1;
	if(!path)
		return -1;
	if(!file)
		file = "";

	ret = GetAppConfigPath(profiles_path, 512, (LOCAL_FOLDER_NAME + "/basic/profiles").c_str());
	if(ret <= 0)
		return ret;

	if(!*file)
		return snprintf(path, size, "%s/%s", profiles_path, profile);

	return snprintf(path, size, "%s/%s/%s", profiles_path, profile, file);
}

//};

//QAccessibleInterface* accessibleFactory(const QString& classname,
//	QObject* object)
//{
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

	auto& profiler = LOGMANAGER.GetProfiler();
	profiler.StartProfiler();


#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)) && defined(_WIN32)
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif


	QCoreApplication::addLibraryPath(".");

#if __APPLE__
	InstallNSApplicationSubclass();
	InstallNSThreadLocks();

	if (!isInBundle()) {
		blog(LOG_ERROR, "SOOPStudio cannot be run as a standalone binary on macOS. Run the Application bundle instead.");
		return ret;
	}
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
	/* NOTE: Users blindly set this, but this theme is incompatble with Qt6 and
	 * crashes loading saved geometry. Just turn off this theme and let users complain OBS
	 * looks ugly instead of crashing. */
	const char* platform_theme = getenv("QT_QPA_PLATFORMTHEME");
	if (platform_theme && strcmp(platform_theme, "qt5ct") == 0)
		unsetenv("QT_QPA_PLATFORMTHEME");
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
	LoadIconFromABSPath("assets/freecshotplus_icon.ico", icon);
	app.setWindowIcon(icon);
	try
	{
		app.AppInit();  // throw exception
		//
		LOGMANAGER.DeleteOldestFile(false, "/profiler_data");

		AFQTranslator translator;
		app.installTranslator(&translator);

		/* --------------------------------------- */
		/* check and warn if already running       */

		bool already_running = false;


#ifndef _DEBUG
#ifdef _WIN32
		RunOnceMutex rom =
#endif
			CheckIfAlreadyRunning(already_running);
#endif // _DEBUG

		if (already_running == false)
			ret = AFQApplication::RunProgramProcess(&app, argc, argv);
		else
		{
			auto& argOption = ARGOPTION;
			if (argOption.GetMulti() == false)
			{
				auto QTStr = [](const char* lookupVal) {
					return QString::fromUtf8(LOCALE_CONTEXT.Str(lookupVal));
					};

				AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
					"", QTStr("AlreadyRunning.Text"), false, true);

				/*QMessageBox mb(QMessageBox::Question,
							   QTStr("AlreadyRunning.Title"),
							   QTStr("AlreadyRunning.Text"));
				mb.addButton(QTStr("AlreadyRunning.LaunchAnyway"),
							 QMessageBox::YesRole);
				QPushButton* cancelButton = mb.addButton(
					QTStr("Cancel"), QMessageBox::NoRole);
				mb.setDefaultButton(cancelButton);

				mb.exec();*/

#ifdef _WIN32  
				QString qStr = QString::fromUtf8(APPNAME);
				LPCWSTR appName = reinterpret_cast<LPCWSTR>(qStr.utf16());

				HWND hwnd = FindWindow(NULL, appName);
				if (hwnd)
				{
					if (IsWindowVisible(hwnd)) {
						ShowWindow(hwnd, SW_SHOW);
					}
					SetForegroundWindow(hwnd);
				}
#endif
				return 0;
			}

			if (argOption.GetMulti())
			{
				blog(LOG_INFO, "User enabled --multi flag and is now running multiple instances of OBS.");
			}
			else
			{
				blog(LOG_WARNING, "================================");
				blog(LOG_WARNING, "Warning: OBS is already running!");
				blog(LOG_WARNING, "================================");
				blog(LOG_WARNING, "User is now running multiple instances of OBS!");
				/* Clear unclean_shutdown flag as multiple instances
				 * running from the same config will lead to a
				 * false-positive detection.*/

				argOption.SetUncleanShutdown(false);
			}

			ret = AFQApplication::RunProgramProcess(&app, argc, argv);
		}
	}
	catch (const char* error) 
	{
		blog(LOG_ERROR, "%s", error);
		OBSErrorBox(nullptr, "%s", error);
	}


	if (g_bRestart || g_bRestartSafe) 
	{
		QStringList args = qApp->arguments();

		g_Arguments.append(args.first());

		bool skipNext = false;

		for (int i = 1; i < args.size(); ++i) {
			if (skipNext) {
				skipNext = false;
				continue;
			}

			if (args[i].startsWith("--soop_id=") || args[i].startsWith("--soop_cookie=")) {
				continue;
			}

			if (args[i] == "--id" || args[i] == "--cookie") {
				skipNext = true;
				continue;
			}

			g_Arguments.append(args[i]);
		}

		if (g_bRestartSafe) 
			g_Arguments.append("--safe-mode");
		else 
			g_Arguments.removeAll("--safe-mode");
	}
    
    
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
