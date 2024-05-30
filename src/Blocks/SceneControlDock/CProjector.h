#pragma once

#include <QFrame>


#include <obs.hpp>
#include "UIComponent/CQtDisplay.h"

enum class ProjectorType
{
    Source,
    Scene,
    Preview,
    StudioProgram,
    Multiview,
};

// Forward
class QMouseEvent;
class AFMultiview;

class AFQProjector : public AFQTDisplay
{
#pragma region QT Field
	Q_OBJECT

#pragma region class initializer, destructor
public:
    explicit AFQProjector(QWidget* widget, obs_source_t* source_,
                          int monitor, ProjectorType type_);
    ~AFQProjector();
#pragma endregion class initializer, destructor

private slots:
    void                        qslotEscapeTriggered();
    void                        qslotOpenFullScreenProjector();
    void                        qslotResizeToContent();
    void                        qslotOpenWindowedProjector();
    void                        qslotAlwaysOnTopToggled(bool alwaysOnTop);
    void                        qslotScreenRemoved(QScreen *screen_);
#pragma endregion QT Field

#pragma region public func
public:
    OBSSource                   GetSource() { return OBSGetStrongRef(weakSource); };
    ProjectorType               GetProjectorType() { return type; };
    int                         GetMonitor() { return savedMonitor; };
    
    static void                 UpdateMultiviewProjectors();
    void                        RenameProjector(QString oldName, QString newName);
    void                        SetHideCursor();

    bool                        IsAlwaysOnTop() const { return isAlwaysOnTop; };
    bool                        IsAlwaysOnTopOverridden() const { return isAlwaysOnTopOverridden; };
    void                        SetIsAlwaysOnTop(bool isAlwaysOnTop, bool isOverridden);
    
    
    void                        RenderModeOnlySources();
    void                        RenderModeDefault();
    
    void                        DisconnectRenderCallback();
#pragma endregion public func

#pragma region protected func
protected:
    void                        mousePressEvent(QMouseEvent *event) override;
    void                        mouseDoubleClickEvent(QMouseEvent *event) override;
    void                        moveEvent(QMoveEvent *event) override;
    void                        resizeEvent(QResizeEvent *event) override;
    void                        closeEvent(QCloseEvent *event) override;
    void                        showEvent(QShowEvent *event) override;
    void                        hideEvent(QHideEvent *event) override;
#pragma endregion protected func
    
#pragma region private func
private:
    static void                 _OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy);
    static void                 _OBSRender(void *data, uint32_t cx, uint32_t cy);
    static void                 _OBSSourceDestroyed(void *data, calldata_t *params);

    template <typename Receiver, typename... Args>
    void                        _AddProjectorMenuMonitors(QMenu *parent, Receiver *target,
                                          void (Receiver::*slot)(Args...));
    QList<QString>              _GetProjectorMenuMonitorsFormatted();
    
    void                        _UpdateMultiview();
    void                        _UpdateProjectorTitle(QString name);
    
    void                        _SetMonitor(int monitor);
#pragma endregion private func

#pragma region private var
private:
    OBSWeakSourceAutoRelease    weakSource;
    OBSSignal                   destroyedSignal;
    
    bool                        isAlwaysOnTop;
    bool                        isAlwaysOnTopOverridden = false;
    int                         savedMonitor = -1;
    ProjectorType               type = ProjectorType::Source;

    AFMultiview*                multiview = nullptr;

    bool                        ready = false;
    
    QRect                       prevGeometry;
    QScreen*                    screen = nullptr;
#pragma endregion private var
};
