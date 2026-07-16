#pragma once

#include <QMainWindow>
#include <QFrame>
#include <QPushButton>
#include <QTimer>
#include <QPushButton>
#include <QPointer>
#include <QMetaEnum>

#include <vector>

#include <obs.h>
#include <obs.hpp>
#include <util/platform.h>

#include "window-main.hpp"

namespace Ui {
    class AFMainDynamicComposit;
}

// Qt UI Class Forward
class CBasicPreview;
class AFQProgramView;
class AFQVerticalProgramView;
class AFQVolControl;
class AFSceneControlWidget;
class AFMainWindowAccesser;

// class FSBasic : pbulic OBSMainWindow
class AFMainDynamicComposit : public OBSMainWindow
{
    enum ContextBarSize {
        ContextBarSize_Minimized,
        ContextBarSize_Reduced,
        ContextBarSize_Normal
    };
    
    Q_OBJECT

public:
    explicit AFMainDynamicComposit(QWidget* parent = nullptr);
    ~AFMainDynamicComposit();

public slots:
    void ToggleStudioModeBlock(bool enable);
    void ChangeSceneOnDoubleClick();

    void ShowPreview();

    void UpdateVideoCaptureSplitAcitved();
    void UpdateSourceToolbar(QString dataType);

    void SetSideDock(bool side);
    void RenameSource(QString name);

private slots:
    void TransitionStopped();
    void ShowToolbarSourceProps();
    void ShowBrowserInteraction();

signals:
    void PreviewShowRequested();
    void BlockShowRequested();
    void StudioModeToggled();

public:
    bool MainWindowInit();
    void CreateObsDisplay(QWidget* previewWidget);
    
    CBasicPreview* GetMainPreview();
    int GetMainPreviewYPos();
    QFrame* GetNotPreviewFrame();

    inline AFQProgramView* GetStudioModeViewLayout() { return studioModeView; };
    inline AFQVerticalProgramView*  GetVerticalStudioModeViewLayout() { return studioModeVerticalView; };
    
    // For Scene Source
    void SetCurrentScene(OBSSource scene, bool force = false);
    void SetCurrentScene(obs_scene_t* scene, bool force);

    void RefreshSourceBorderColor();
    void UpdateSceneNameStudioMode(bool vertical);

    void UpdatePreviewSafeAreas();
    void UpdatePreviewSpacingHelpers();
    void UpdatePreviewOverflowSettings();

    void SwitchStudioModeLayout(bool vertical);
    void ToggleStudioModeLabels(bool vertical, bool visible);
    void SceneControlStudioModeSignal(AFSceneControlWidget* sceneControl, bool connectSignal);
    void TransitionStudioModeScene();


    void UpdateContextBarVisibility();
    void SetVisibleSourceToolBar(bool visible);
    void UpdateSourceToolBar(bool force);
    void ClearSourceToolBar();
    OBSSource GetToolbarSource() { return OBSGetStrongRef(weakToolbarSource); }
    void RefreshToolBarSplitFilterToggleButton();

    void CheckDocksState();
protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    void MakeVerticalStudioMode();
    void MakeHorizontalStudioMode();
    void DestroyVerticalStudioMode();
    void DestroyHorizontalStudioMode();
    
    void InitMainDisplay(AFMainWindowAccesser* viewModels);
    void InitProgramDisplay(AFMainWindowAccesser* viewModels);

    static void SourceRenamed(void* data, calldata_t* params);

private:
    Ui::AFMainDynamicComposit* ui;
    
    AFQProgramView* studioModeView = nullptr;
    AFQVerticalProgramView* studioModeVerticalView = nullptr;

    OBSWeakSource weakToolbarSource = nullptr;
    OBSSignal signalRenamed;

    bool isSideDock = false;

    ContextBarSize contextBarSize = ContextBarSize_Normal;
};
