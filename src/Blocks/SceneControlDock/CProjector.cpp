#include "CProjector.h"


#include <QAction>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QScreen>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "display-helpers.hpp"

#include "Common/MathMiscUtils.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/OBSData/CInhibitSleepContext.h"
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
      m_weakSource(OBSGetWeakRef(source_))
{
    setAttribute(Qt::WA_DontCreateNativeAncestors);

    OBSSource source = GetSource();
    if (source)
        m_destroyedSignal.Connect(obs_source_get_signal_handler(source),
                                "destroy", _OBSSourceDestroyed, this);

    // Mark the window as a projector so SetDisplayAffinity
    // can skip it
    windowHandle()->setProperty("isOBSProjectorWindow", true);

#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
    // Prevents resizing of projector windows
    setAttribute(Qt::WA_PaintOnScreen, false);
#endif

    m_type = type_;


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

    if (m_type == ProjectorType::Multiview) {
        m_pMultiview = new AFMultiview();

        _UpdateMultiview();

        multiviewProjectors.push_back(this);
    }

    INHIBITSLEEP_CONTEXT.IncrementSleepInhibition();
    if (source)
        obs_source_inc_showing(source);

    m_ready = true;

    //need to show(activatewindow) before inserting to block to show properly, but it blinks
    setWindowFlag(Qt::FramelessWindowHint);
    setGeometry(0, 0, 1, 1);
    //make it frameless and 1px to hide the blink

    show();

    // We need it here to allow keyboard input in X11 to listen to Escape
    activateWindow();
}

AFQProjector::~AFQProjector()
{
    bool isMultiview = m_type == ProjectorType::Multiview;
    
    DisconnectRenderCallback();

    OBSSource source = GetSource();
    if (source)
        obs_source_dec_showing(source);

    if (isMultiview)
    {
        delete m_pMultiview;
        multiviewProjectors.removeAll(this);
    }

    INHIBITSLEEP_CONTEXT.DecrementSleepInhibition();

    m_pScreen = nullptr;
}

void AFQProjector::qslotEscapeTriggered()
{
    this->window()->close();
    this->window()->deleteLater();
}

void AFQProjector::qslotOpenFullScreenProjector()
{
    if (!isFullScreen())
        m_prevGeometry = this->window()->geometry();

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
    GetScaleAndCenterPos(targetCX, targetCY, size.width(), size.height(), x, y, scale);

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

    if (!m_prevGeometry.isNull())
        this->window()->setGeometry(m_prevGeometry);
    else
        this->window()->resize(480, 270);

    m_savedMonitor = -1;
    emit qsignalWindowProjector();

    OBSSource source = GetSource();
    _UpdateProjectorTitle(QT_UTF8(obs_source_get_name(source)));
    m_pScreen = nullptr;
}

void AFQProjector::qslotAlwaysOnTopToggled(bool isAlwaysOnTop)
{
    SetIsAlwaysOnTop(isAlwaysOnTop, true);
    config_set_bool(USERCONFIG, "BasicWindow", "ProjectorAlwaysOnTop", isAlwaysOnTop);
}

void AFQProjector::qslotScreenRemoved(QScreen *screen_)
{
    if (GetMonitor() < 0 || !m_pScreen)
        return;

    if (m_pScreen == screen_)
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
    if (m_savedMonitor == -1)
        return;

    bool hideCursor = config_get_bool(USERCONFIG, "BasicWindow", "HideProjectorCursor");
    if (hideCursor && m_type != ProjectorType::Multiview)
        setCursor(Qt::BlankCursor);
    else
        setCursor(Qt::ArrowCursor);
}

void AFQProjector::SetIsAlwaysOnTop(bool isAlwaysOnTop, bool isOverridden)
{
    this->m_isAlwaysOnTop = isAlwaysOnTop;
    this->m_isAlwaysOnTopOverridden = isOverridden;

    SetAlwaysOnTop(this, isAlwaysOnTop);
}

void AFQProjector::DisconnectRenderCallback()
{
    bool isMultiview = m_type == ProjectorType::Multiview;
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

    if (event->button() == Qt::RightButton) {
        AFQCustomMenu *projectorMenu = new AFQCustomMenu(QTStr("Fullscreen"), nullptr, true);

        // TODO: check
        AFQProjector::_AddProjectorMenuMonitors(
            projectorMenu, this, &AFQProjector::qslotOpenFullScreenProjector);

        AFQCustomMenu popup(this);
        popup.addMenu(projectorMenu);

        if (GetMonitor() > -1) {
            popup.addAction(QTStr("Windowed"), this,
                            &AFQProjector::qslotOpenWindowedProjector);

        } else if (!this->isMaximized()) {
            popup.addAction(QTStr("ResizeProjectorWindowToContent"),
                            this, &AFQProjector::qslotResizeToContent);
        }

        QAction *alwaysOnTopButton = new QAction(
            QTStr("Basic.MainMenu.View.AlwaysOnTop"), this);
        alwaysOnTopButton->setCheckable(true);
        alwaysOnTopButton->setChecked(m_isAlwaysOnTop);

        connect(alwaysOnTopButton, &QAction::toggled, this,
            &AFQProjector::qslotAlwaysOnTopToggled);

        popup.addAction(alwaysOnTopButton);

        popup.addAction(QTStr("Close"), this,
                        &AFQProjector::qslotEscapeTriggered);
        popup.exec(QCursor::pos());
    } else if (event->button() == Qt::LeftButton) {
        // Only MultiView projectors handle left click
        if (this->m_type != ProjectorType::Multiview)
            return;

        if (!mouseSwitching)
            return;

        QPoint pos = event->pos();
        OBSSource src = m_pMultiview->GetSourceByPosition(pos.x(), pos.y(), this);
        if (!src)
            return;

        OBSSource tmpCurrSceneSrc = AFSceneUtil::CnvtToOBSSource(SCENE_CONTEXT.GetCurrentScene());
        if (tmpCurrSceneSrc != src)
            DYNAMIC_COMPOSIT->SetCurrentScene(src, false);
        //
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
    if (this->m_type != ProjectorType::Multiview)
        return;

    if (STATEAPP.IsPreviewProgramMode() == false)
        return;

    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();
        OBSSource src =
            m_pMultiview->GetSourceByPosition(pos.x(), pos.y());
        if (!src)
            return;
    }
}

void AFQProjector::moveEvent(QMoveEvent *event)
{
    AFQTDisplay::moveEvent(event);
    if(m_pMultiview)
        m_pMultiview->SetDpi(devicePixelRatioF());
}

void AFQProjector::resizeEvent(QResizeEvent *event)
{
    AFQTDisplay::resizeEvent(event);
    if (m_pMultiview)
        m_pMultiview->SetDpi(devicePixelRatioF());
}

void AFQProjector::closeEvent(QCloseEvent *event)
{
    qslotEscapeTriggered();
    event->accept();
}

void AFQProjector::showEvent(QShowEvent * event) {
    QWidget::showEvent(event);

    m_isAlwaysOnTop = config_get_bool(USERCONFIG, "BasicWindow", "ProjectorAlwaysOnTop");
    emit qslotAlwaysOnTopToggled(m_isAlwaysOnTop);

    // ConnectRenderCallback
    bool isMultiview = m_type == ProjectorType::Multiview;
    obs_display_add_draw_callback(GetDisplay(),
                                    isMultiview ? _OBSRenderMultiview :
                                    _OBSRender, this);

    if (m_type == ProjectorType::Scene)
        obs_display_set_background_color(GetDisplay(), 0x000000);
    else
        obs_display_set_background_color(GetDisplay(), 0x2D2724);
}

void AFQProjector::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);

    DisconnectRenderCallback();
}

void AFQProjector::_OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy)
{
    AFQProjector *window = (AFQProjector *)data;

    if (updatingMultiview || !window->m_ready)
        return;

    if (window->m_pMultiview->GetDpi() != window->devicePixelRatioF())
        window->m_pMultiview->SetDpi(window->devicePixelRatioF());
    
    window->m_pMultiview->Render(cx, cy);
}

void AFQProjector::_OBSRender(void *data, uint32_t cx, uint32_t cy)
{
    AFQProjector *window = reinterpret_cast<AFQProjector *>(data);

    if (!window->m_ready)
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

    startRegion(x, y, newCX, newCY, 0.0f, float(targetCX), 0.0f, float(targetCY));

    if (window->m_type == ProjectorType::Preview &&
        STATEAPP.IsPreviewProgramMode())
    {
        OBSSource tmpCurrSceneSrc = AFSceneUtil::CnvtToOBSSource(SCENE_CONTEXT.GetCurrentScene());

        if (source != tmpCurrSceneSrc) 
        {
            obs_source_dec_showing(source);
            obs_source_inc_showing(tmpCurrSceneSrc);
            source = tmpCurrSceneSrc;
            window->m_weakSource = OBSGetWeakRef(source);
        }
    } 
    else if (window->m_type == ProjectorType::Preview &&
             STATEAPP.IsPreviewProgramMode() == false)
    {
        window->m_weakSource = nullptr;
    }

    if (source)
        obs_source_video_render(source);
    else
        obs_render_main_texture();

    endRegion();
}

void AFQProjector::_OBSSourceDestroyed(void *data, calldata_t *)
{
    AFQProjector *window = reinterpret_cast<AFQProjector *>(data);
    QMetaObject::invokeMethod(window, "qslotEscapeTriggered");
}

void AFQProjector::_UpdateMultiview()
{
    bool drawLabel = config_get_bool(USERCONFIG, "BasicWindow", "MultiviewDrawNames");
    mouseSwitching = config_get_bool(USERCONFIG, "BasicWindow", "MultiviewMouseSwitch");

    //Double Click Transition Hide
    transitionOnDoubleClick = false;
    //config_get_bool(USERCONFIG, "BasicWindow", "TransitionOnDoubleClick");
    //Double Click Transition Hide

    m_pMultiview->Update(drawLabel);
}

void AFQProjector::_UpdateProjectorTitle(QString name)
{ 
    bool window = (GetMonitor() == -1);

    QString title = nullptr;
    switch (m_type) {
    case ProjectorType::Scene:
        if (!window)
            title = QTStr("SceneProjector") + " - " + name;
        else
            title = QTStr("SceneWindow") + " - " + name;
        break;
    case ProjectorType::Source:
        if (!window)
            title = QTStr("SourceProjector") + " - " + name;
        else
            title = QTStr("SourceWindow") + " - " + name;
        break;
    case ProjectorType::Preview:
        if (!window)
            title = QTStr("PreviewProjector");
        else
            title = QTStr("PreviewWindow");
        break;
    case ProjectorType::StudioProgram:
        if (!window)
            title = QTStr("StudioProgramProjector");
        else
            title = QTStr("StudioProgramWindow");
        break;
    case ProjectorType::Multiview:
        if (!window)
            title = QTStr("MultiviewProjector");
        else
            title = QTStr("MultiviewWindowed");
        break;
    default:
        title = name;
        break;
    }

    setWindowTitle(title);
}

void AFQProjector::_SetMonitor(int monitor)
{
    m_savedMonitor = monitor;
    m_pScreen = QGuiApplication::screens()[monitor];
    this->window()->setGeometry(m_pScreen->geometry());
    showFullScreen();
    emit qsignalFullScreenProjector();
    SetHideCursor();
}
