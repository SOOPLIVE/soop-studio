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

public slots:

private slots:
    void                        qslotEscapeTriggered();
    void                        qslotOpenFullScreenProjector();
    void                        qslotResizeToContent();
    void                        qslotOpenWindowedProjector();
    void                        qslotAlwaysOnTopToggled(bool alwaysOnTop);
    void                        qslotScreenRemoved(QScreen *screen_);

signals:
    void                        qsignalFullScreenProjector();
    void                        qsignalWindowProjector();

#pragma endregion QT Field

#pragma region public func
public:
    OBSSource                   GetSource() { return OBSGetStrongRef(m_weakSource); };
    ProjectorType               GetProjectorType() { return m_type; };
    int                         GetMonitor() { return m_savedMonitor; };
    
    static void                 UpdateMultiviewProjectors();
    void                        RenameProjector(QString oldName, QString newName);
    void                        SetHideCursor();

    bool                        IsAlwaysOnTop() const { return m_isAlwaysOnTop; };
    bool                        IsAlwaysOnTopOverridden() const { return m_isAlwaysOnTopOverridden; };
    void                        SetIsAlwaysOnTop(bool isAlwaysOnTop, bool isOverridden);
    
        
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
    OBSWeakSourceAutoRelease    m_weakSource;
    OBSSignal                   m_destroyedSignal;
    
    bool                        m_isAlwaysOnTop;
    bool                        m_isAlwaysOnTopOverridden = false;
    int                         m_savedMonitor = -1;
    ProjectorType               m_type = ProjectorType::Source;

    AFMultiview*                m_pMultiview = nullptr;

    bool                        m_ready = false;
    
    QRect                       m_prevGeometry;
    QScreen*                    m_pScreen = nullptr;
#pragma endregion private var
};
