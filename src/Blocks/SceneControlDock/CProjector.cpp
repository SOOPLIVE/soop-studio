#include "CProjector.h"


#include <QAction>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QScreen>
#include "qt-wrapper.h"


#include <platform.hpp>


#include "Common/MathMiscUtils.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Graphics/CGraphicsMiscUtils.inl"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/OBSData/CInhibitSleepContext.h"
#include "CoreModel/Scene/CScene.h"
#include "CoreModel/Scene/CSceneContext.h"


#include "ViewModel/Preview/CMultiview.h"
#include "UIComponent/CCustomMenu.h"


#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"


static QList<AFQProjector*> multiviewProjectors;

static bool updatingMultiview = false, mouseSwitching, transitionOnDoubleClick;

AFQProjector::AFQProjector(QWidget *widget, obs_source_t *source_, int monitor,
                           ProjectorType type_)
    : AFQTDisplay(widget, Qt::Window),
      weakSource(OBSGetWeakRef(source_))
{
    OBSSource source = GetSource();
    if (source)
        destroyedSignal.Connect(obs_source_get_signal_handler(source),
                                "destroy", _OBSSourceDestroyed, this);

    // Mark the window as a projector so SetDisplayAffinity
    // can skip it
    windowHandle()->setProperty("isOBSProjectorWindow", true);

#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
    // Prevents resizing of projector windows
    setAttribute(Qt::WA_PaintOnScreen, false);
#endif

    type = type_;


    if (monitor != -1)
        _SetMonitor(monitor);

    if (source)
        _UpdateProjectorTitle(QT_UTF8(obs_source_get_name(source)));
    else
        _UpdateProjectorTitle(QString());

    QAction *action = new QAction(this);
    action->setShortcut(Qt::Key_Escape);
    addAction(action);
    connect(action, &QAction::triggered, this,
            &AFQProjector::qslotEscapeTriggered);

    setAttribute(Qt::WA_DeleteOnClose, true);

    //disable application quit when last window closed
    setAttribute(Qt::WA_QuitOnClose, false);

    installEventFilter(CreateShortcutFilter());

    connect(qobject_cast<QApplication*>(QCoreApplication::instance()), &QGuiApplication::screenRemoved, this,
            &AFQProjector::qslotScreenRemoved);

    if (type == ProjectorType::Multiview) {
        multiview = new AFMultiview();

        _UpdateMultiview();

        multiviewProjectors.push_back(this);
    }

    AFInhibitSleepContext::GetSingletonInstance().IncrementSleepInhibition();

    if (source)
        obs_source_inc_showing(source);

    ready = true;

    show();

    // We need it here to allow keyboard input in X11 to listen to Escape
    activateWindow();
}

AFQProjector::~AFQProjector()
{
    bool isMultiview = type == ProjectorType::Multiview;
    
    DisconnectRenderCallback();

    OBSSource source = GetSource();
    if (source)
        obs_source_dec_showing(source);

    if (isMultiview)
    {
        delete multiview;
        multiviewProjectors.removeAll(this);
    }

    AFInhibitSleepContext::GetSingletonInstance().DecrementSleepInhibition();

    screen = nullptr;
}

void AFQProjector::qslotEscapeTriggered()
{
    this->window()->close();
}

void AFQProjector::qslotOpenFullScreenProjector()
{
    if (!isFullScreen())
        prevGeometry = this->window()->geometry();

    int monitor = sender()->property("monitor").toInt();
    _SetMonitor(monitor);

    OBSSource source = GetSource();
    _UpdateProjectorTitle(QT_UTF8(obs_source_get_name(source)));
}

void AFQProjector::qslotResizeToContent()
{
    OBSSource source = GetSource();
    uint32_t targetCX;
    uint32_t targetCY;
    int x, y, newX, newY;
    float scale;

    if (source)
    {
        targetCX = std::max(obs_source_get_width(source), 1u);
        targetCY = std::max(obs_source_get_height(source), 1u);
    } 
    else
    {
        struct obs_video_info ovi;
        obs_get_video_info(&ovi);
        targetCX = ovi.base_width;
        targetCY = ovi.base_height;
    }

    QSize size = this->size();
    GetScaleAndCenterPos(targetCX, targetCY, size.width(), size.height(), x,
                         y, scale);

    QSize winSize = this->window()->size();
    float winNewX = winSize.width() - (x * 2);
    float winNewY = winSize.height() - (y * 2);

    this->window()->resize(winNewX, winNewY);
}

void AFQProjector::qslotOpenWindowedProjector()
{
    showFullScreen();
    showNormal();
    setCursor(Qt::ArrowCursor);

    if (!prevGeometry.isNull())
        this->window()->setGeometry(prevGeometry);
    else
        resize(480, 270);

    savedMonitor = -1;

    OBSSource source = GetSource();
    _UpdateProjectorTitle(QT_UTF8(obs_source_get_name(source)));
    screen = nullptr;
}

void AFQProjector::qslotAlwaysOnTopToggled(bool isAlwaysOnTop)
{
    SetIsAlwaysOnTop(isAlwaysOnTop, true);
    config_set_bool(AFConfigManager::GetSingletonInstance().GetGlobal(),
                    "BasicWindow", "ProjectorAlwaysOnTop", isAlwaysOnTop);
}

void AFQProjector::qslotScreenRemoved(QScreen *screen_)
{
    if (GetMonitor() < 0 || !screen)
        return;

    if (screen == screen_)
        qslotEscapeTriggered();
}

void AFQProjector::UpdateMultiviewProjectors()
{
    obs_enter_graphics();
    updatingMultiview = true;
    obs_leave_graphics();

    for (auto &projector : multiviewProjectors)
        projector->_UpdateMultiview();

    obs_enter_graphics();
    updatingMultiview = false;
    obs_leave_graphics();
}

void AFQProjector::RenameProjector(QString oldName, QString newName)
{
    if (oldName == newName)
        return;

    _UpdateProjectorTitle(newName);
}

void AFQProjector::SetHideCursor()
{
    if (savedMonitor == -1)
        return;

    bool hideCursor = config_get_bool(AFConfigManager::GetSingletonInstance().GetGlobal(),
                                      "BasicWindow", "HideProjectorCursor");

    if (hideCursor && type != ProjectorType::Multiview)
        setCursor(Qt::BlankCursor);
    else
        setCursor(Qt::ArrowCursor);
}

void AFQProjector::SetIsAlwaysOnTop(bool isAlwaysOnTop, bool isOverridden)
{
    this->isAlwaysOnTop = isAlwaysOnTop;
    this->isAlwaysOnTopOverridden = isOverridden;

    SetAlwaysOnTop(this, isAlwaysOnTop);
}

void AFQProjector::RenderModeOnlySources()
{
    if (multiview != nullptr)
        multiview->SetOnlyRenderSources(true);
}

void AFQProjector::RenderModeDefault()
{
    if (multiview != nullptr)
        multiview->SetOnlyRenderSources(false);
}

void AFQProjector::DisconnectRenderCallback()
{
    bool isMultiview = type == ProjectorType::Multiview;
    obs_display_remove_draw_callback(GetDisplay(),
                                     isMultiview ? _OBSRenderMultiview : _OBSRender,
                                     this);
}

template <typename Receiver, typename... Args>
void AFQProjector::_AddProjectorMenuMonitors(QMenu *parent, Receiver *target,
                                     void (Receiver::*slot)(Args...)) {
    auto projectors = _GetProjectorMenuMonitorsFormatted();
    for (int i = 0; i < projectors.size(); i++) {
        QString str = projectors[i];
        QAction *action = parent->addAction(str, target, slot);
        action->setProperty("monitor", i);
    }
}

QList<QString> AFQProjector::_GetProjectorMenuMonitorsFormatted() {
    QList<QString> projectorsFormatted;
    QList<QScreen *> screens = QGuiApplication::screens();
    for (int i = 0; i < screens.size(); i++) {
        QScreen *screen = screens[i];
        QRect screenGeometry = screen->geometry();
        qreal ratio = screen->devicePixelRatio();
        QString name = "";
#if defined(_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
        QTextStream fullname(&name);
        fullname << GetMonitorName(screen->name());
        fullname << " (";
        fullname << (i + 1);
        fullname << ")";
#elif defined(__APPLE__) || defined(_WIN32)
        name = screen->name();
#else
        name = screen->model().simplified();

        if (name.length() > 1 && name.endsWith("-"))
            name.chop(1);
#endif
        name = name.simplified();

        if (name.length() == 0) {
            name = QString("%1 %2")
                       .arg(QTStr("Display"))
                       .arg(QString::number(i + 1));
        }
        QString str =
            QString("%1: %2x%3 @ %4,%5")
                .arg(name, QString::number(screenGeometry.width() * ratio),
                     QString::number(screenGeometry.height() * ratio),
                     QString::number(screenGeometry.x()),
                     QString::number(screenGeometry.y()));
        projectorsFormatted.push_back(str);
    }
    return projectorsFormatted;
}

void AFQProjector::mousePressEvent(QMouseEvent *event)
{
    AFQTDisplay::mousePressEvent(event);

    
    auto& textManager = AFLocaleTextManager::GetSingletonInstance();
    
    if (event->button() == Qt::RightButton) {
        AFQCustomMenu *projectorMenu = new AFQCustomMenu(QT_UTF8(textManager.Str("Fullscreen")), nullptr, true);

        AFQProjector::_AddProjectorMenuMonitors(
            projectorMenu, this, &AFQProjector::qslotOpenFullScreenProjector);

        AFQCustomMenu popup(this);
        popup.addMenu(projectorMenu);

        if (GetMonitor() > -1) {
            popup.addAction(QT_UTF8(textManager.Str("Windowed")), this,
                            &AFQProjector::qslotOpenWindowedProjector);

        } else if (!this->isMaximized()) {
            popup.addAction(QT_UTF8(textManager.Str("ResizeProjectorWindowToContent")),
                            this, &AFQProjector::qslotResizeToContent);
        }

        QAction *alwaysOnTopButton = new QAction(
            QT_UTF8(textManager.Str("Basic.MainMenu.View.AlwaysOnTop")), this);
        alwaysOnTopButton->setCheckable(true);
        alwaysOnTopButton->setChecked(isAlwaysOnTop);

        connect(alwaysOnTopButton, &QAction::toggled, this,
            &AFQProjector::qslotAlwaysOnTopToggled);

        popup.addAction(alwaysOnTopButton);

        popup.addAction(QT_UTF8(textManager.Str("Close")), this,
                        &AFQProjector::qslotEscapeTriggered);
        popup.exec(QCursor::pos());
    } else if (event->button() == Qt::LeftButton) {
        // Only MultiView projectors handle left click
        if (this->type != ProjectorType::Multiview)
            return;

        if (!mouseSwitching)
            return;

        QPoint pos = event->pos();
        OBSSource src =
            multiview->GetSourceByPosition(pos.x(), pos.y(), this);
        if (!src)
            return;

        auto& sceneContext =  AFSceneContext::GetSingletonInstance();
        OBSSource tmpCurrSceneSrc = AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrOBSScene());
        if (tmpCurrSceneSrc != src)
            App()->GetMainView()->GetMainWindow()->SetCurrentScene(src, false);
    }
}

void AFQProjector::mouseDoubleClickEvent(QMouseEvent *event)
{
    AFQTDisplay::mouseDoubleClickEvent(event);

    if (!mouseSwitching)
        return;

    if (!transitionOnDoubleClick)
        return;

    // Only MultiView projectors handle double click
    if (this->type != ProjectorType::Multiview)
        return;

    AFStateAppContext* tmpStateApp = AFConfigManager::GetSingletonInstance().GetStates();
    if (tmpStateApp->IsPreviewProgramMode() == false)
        return;

    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();
        OBSSource src =
            multiview->GetSourceByPosition(pos.x(), pos.y());
        if (!src)
            return;

        // Tracsition, Scene Context
//        if (main->GetProgramSource() != src)
//            main->TransitionToScene(src);
        //
    }
}

void AFQProjector::moveEvent(QMoveEvent *event)
{
    AFQTDisplay::moveEvent(event);
    multiview->SetDpi(devicePixelRatioF());
}

void AFQProjector::resizeEvent(QResizeEvent *event)
{
    AFQTDisplay::resizeEvent(event);
    multiview->SetDpi(devicePixelRatioF());
}

void AFQProjector::closeEvent(QCloseEvent *event)
{
    qslotEscapeTriggered();
    event->accept();
}

void AFQProjector::showEvent(QShowEvent * event) {
    QWidget::showEvent(event);

    isAlwaysOnTop =
        config_get_bool(AFConfigManager::GetSingletonInstance().GetGlobal(),
                        "BasicWindow", "ProjectorAlwaysOnTop");
    emit qslotAlwaysOnTopToggled(isAlwaysOnTop);

    // ConnectRenderCallback
    bool isMultiview = type == ProjectorType::Multiview;
    obs_display_add_draw_callback(GetDisplay(),
                                    isMultiview ? _OBSRenderMultiview :
                                    _OBSRender, this);
    obs_display_set_background_color(GetDisplay(), 0x2D2724);
}

void AFQProjector::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);

    DisconnectRenderCallback();
}

void AFQProjector::_OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy)
{
    AFQProjector *window = (AFQProjector *)data;

    if (updatingMultiview || !window->ready)
        return;

    if (window->multiview->GetDpi() != window->devicePixelRatioF())
        window->multiview->SetDpi(window->devicePixelRatioF());
    
    window->multiview->Render(cx, cy);
}

void AFQProjector::_OBSRender(void *data, uint32_t cx, uint32_t cy)
{
    AFQProjector *window = reinterpret_cast<AFQProjector *>(data);

    if (!window->ready)
        return;

    OBSSource source = window->GetSource();

    uint32_t targetCX;
    uint32_t targetCY;
    int x, y;
    int newCX, newCY;
    float scale;

    if (source) {
        targetCX = std::max(obs_source_get_width(source), 1u);
        targetCY = std::max(obs_source_get_height(source), 1u);
    } else {
        struct obs_video_info ovi;
        obs_get_video_info(&ovi);
        targetCX = ovi.base_width;
        targetCY = ovi.base_height;
    }

    GetScaleAndCenterPos(targetCX, targetCY, cx, cy, x, y, scale);

    newCX = int(scale * float(targetCX));
    newCY = int(scale * float(targetCY));

    StartGraphicsViewRegion(x, y, newCX, newCY, 0.0f, float(targetCX), 0.0f,
                            float(targetCY));

    
    AFStateAppContext* tmpStateApp = AFConfigManager::GetSingletonInstance().GetStates();
    
    if (window->type == ProjectorType::Preview &&
        tmpStateApp->IsPreviewProgramMode())
    {
        auto& sceneContext =  AFSceneContext::GetSingletonInstance();
        OBSSource tmpCurrSceneSrc = AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrOBSScene());

        if (source != tmpCurrSceneSrc) 
        {
            obs_source_dec_showing(source);
            obs_source_inc_showing(tmpCurrSceneSrc);
            source = tmpCurrSceneSrc;
            window->weakSource = OBSGetWeakRef(source);
        }
    } 
    else if (window->type == ProjectorType::Preview &&
             tmpStateApp->IsPreviewProgramMode() == false)
    {
        window->weakSource = nullptr;
    }

    if (source)
        obs_source_video_render(source);
    else
        obs_render_main_texture();

    EndGraphicsViewRegion();
}

void AFQProjector::_OBSSourceDestroyed(void *data, calldata_t *)
{
    AFQProjector *window = reinterpret_cast<AFQProjector *>(data);
    QMetaObject::invokeMethod(window, "qslotEscapeTriggered");
}

void AFQProjector::_UpdateMultiview()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    config_t* tmpGlobalConfig = confManager.GetGlobal();
    
    bool drawLabel = config_get_bool(GetGlobalConfig(), "BasicWindow",
        "MultiviewDrawNames");

    mouseSwitching = config_get_bool(tmpGlobalConfig, "BasicWindow",
                                     "MultiviewMouseSwitch");

    transitionOnDoubleClick = config_get_bool(tmpGlobalConfig, "BasicWindow", 
                                              "TransitionOnDoubleClick");

    multiview->Update(drawLabel);
}

void AFQProjector::_UpdateProjectorTitle(QString name)
{
    auto& textManager = AFLocaleTextManager::GetSingletonInstance();
    
    bool window = (GetMonitor() == -1);

    QString title = nullptr;
    switch (type) {
    case ProjectorType::Scene:
        if (!window)
            title = QT_UTF8(textManager.Str("SceneProjector")) + " - " + name;
        else
            title = QT_UTF8(textManager.Str("SceneWindow")) + " - " + name;
        break;
    case ProjectorType::Source:
        if (!window)
            title = QT_UTF8(textManager.Str("SourceProjector")) + " - " + name;
        else
            title = QT_UTF8(textManager.Str("SourceWindow")) + " - " + name;
        break;
    case ProjectorType::Preview:
        if (!window)
            title = QT_UTF8(textManager.Str("PreviewProjector"));
        else
            title = QT_UTF8(textManager.Str("PreviewWindow"));
        break;
    case ProjectorType::StudioProgram:
        if (!window)
            title = QT_UTF8(textManager.Str("StudioProgramProjector"));
        else
            title = QT_UTF8(textManager.Str("StudioProgramWindow"));
        break;
    case ProjectorType::Multiview:
        if (!window)
            title = QT_UTF8(textManager.Str("MultiviewProjector"));
        else
            title = QT_UTF8(textManager.Str("MultiviewWindowed"));
        break;
    default:
        title = name;
        break;
    }

    setWindowTitle(title);
}

void AFQProjector::_SetMonitor(int monitor)
{
    savedMonitor = monitor;
    screen = QGuiApplication::screens()[monitor];
    this->window()->setGeometry(screen->geometry());
    showFullScreen();
    SetHideCursor();
}
