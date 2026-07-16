#include "CProgramViewHorizontal.h"
#include "ui_main-horizontal-studio-mode-view.h"

#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "Application/CApplication.h"

#include "platform/platform.hpp"

#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Graphics/CGraphicsContext.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"


AFQProgramView::AFQProgramView(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::AFQProgramView)
{
    ui->setupUi(this);

    /*connect(ui->widget_HoverVisible, &AFQHoverWidget::qsignalHoverEnter,
            DYNAMIC_COMPOSIT, &AFMainDynamicComposit::BlockShowRequested);*/

}

AFQProgramView::~AFQProgramView()
{
    delete ui;
}

void AFQProgramView::qslotChangeLayoutStrech()
{
    QVBoxLayout* thisLayout = qobject_cast<QVBoxLayout*>(this->layout());

    int32_t previewY = GRAPHIC_CONTEXT.GetMainPreviewY();
    int32_t previewCY = GRAPHIC_CONTEXT.GetMainPreviewCY();

    int32_t firstValue = thisLayout->stretch(0);


    if (firstValue != 0 &&
        std::abs(thisLayout->stretch(0) - previewY) <= 1)
        return;



    thisLayout->setStretch(0, previewY);
    thisLayout->setStretch(1, previewY + previewCY);
}

void AFQProgramView::_qslotTransitionClicked() {
    //
    SCENE_CONTEXT.TransitionToScene(AFSceneUtil::CnvtToOBSSource(SCENE_CONTEXT.GetCurrentScene()));
    ChangeLiveSceneName(ui->label_EditSceneName->text());
}

void AFQProgramView::qslotToggleSwapScenesMode()
{
    auto& stateApp = STATEAPP;
    //
    bool tmpValue = stateApp.GetSwapScenesMode();
    stateApp.SetSwapScenesMode(!tmpValue);
}

void AFQProgramView::qslotToggleEditProperties()
{
    auto& stateApp = STATEAPP;
    //
    bool tmpValue = stateApp.GetEditPropertiesMode();
    stateApp.SetEditPropertiesMode(!tmpValue);

    OBSSource actualScene = OBSGetStrongRef(SCENE_CONTEXT.GetProgramScene());
    if (actualScene)
        SCENE_CONTEXT.TransitionToScene(actualScene, true);
}

void AFQProgramView::qslotToggleSceneDuplication()
{
    auto& stateApp = STATEAPP;
    //
    bool tmpValue = stateApp.GetSceneDuplicationMode();
    stateApp.SetSceneDuplicationMode(!tmpValue);

    OBSSource actualScene = OBSGetStrongRef(SCENE_CONTEXT.GetProgramScene());
    if (actualScene)
        SCENE_CONTEXT.TransitionToScene(actualScene, true);

    m_pEditProperties->setEnabled(m_pDuplicateScene->isChecked());

}

void AFQProgramView::_qslotMenuTransitionClicked()
{
    if (m_transitionMenu != nullptr)
        m_transitionMenu->exec(QCursor::pos());
}

bool AFQProgramView::Initialize()
{
    _SetButtons();
    _InitValueLabels();
    _InitTransitionMenu();

    return true;
}

void AFQProgramView::InsertDisplays(QWidget* mainDisplay, QWidget* programDisplay)
{
    QHBoxLayout* tmpHBoxLayout = new QHBoxLayout();
    tmpHBoxLayout->setContentsMargins(16, 0, 16, 0);
    tmpHBoxLayout->setSpacing(16);
    tmpHBoxLayout->addWidget(mainDisplay);
    tmpHBoxLayout->addWidget(programDisplay);

#ifdef __APPLE__
    programDisplay->show();
#endif

    ui->widget_ContainerDisplays->setLayout(tmpHBoxLayout);
}

void AFQProgramView::ChangeEditSceneName(QString name)
{
    ui->label_EditSceneName->setText(name);
}

void AFQProgramView::ChangeLiveSceneName(QString name)
{
    ui->label_LiveSceneName->setText(name);
    if (name.isEmpty())
        ui->label_LiveSceneName->setText(ui->label_EditSceneName->text());
}

void AFQProgramView::ToggleSceneLabel(bool visible)
{
    ui->label_EditDisplay->setVisible(visible);
    ui->label_EditSceneName->setVisible(visible);
    ui->label_LiveDisplay->setVisible(visible);
    ui->label_LiveSceneName->setVisible(visible);
}

void AFQProgramView::qslotTransitionTriggered() {
    _qslotTransitionClicked();
}

void AFQProgramView::_SetButtons()
{
    connect(ui->pushButton_Transition, &AFQCustomPushbutton::clicked,
        this, &AFQProgramView::_qslotTransitionClicked);

    connect(ui->pushButton_MenuTransition, &AFQCustomPushbutton::clicked,
        this, &AFQProgramView::_qslotMenuTransitionClicked);

    ui->pushButton_Transition->setStyleSheet("background-color:#0182FF");

    std::string absPath;
    GetDataFilePath("assets", absPath);
    QString imgPath = QString("%1/mainview/preview/horizontal-transition.svg").arg(absPath.data());
    ui->pushButton_Transition->setIcon(QIcon(imgPath));
    ui->pushButton_Transition->setIconSize(QSize(32, 32));

    ui->pushButton_MenuTransition->setStyleSheet("background-color:#505257");

    imgPath = QString("%1/mainview/preview/menu-transition.svg").arg(absPath.data());
    ui->pushButton_MenuTransition->setIcon(QIcon(imgPath));
    ui->pushButton_MenuTransition->setIconSize(QSize(24, 24));
}

void AFQProgramView::_InitValueLabels()
{
    ui->label_EditDisplay->setText(QTStr("StudioMode.Preview"));
    ui->label_LiveDisplay->setText(QTStr("StudioMode.Program"));
    
    int widthEditText = ui->label_EditDisplay->sizeHint().width();
    int widthLiveText = ui->label_LiveDisplay->sizeHint().width();

    ui->label_EditDisplay->setMinimumWidth(widthEditText);
    ui->label_LiveDisplay->setMinimumWidth(widthLiveText);
}

void AFQProgramView::_InitTransitionMenu()
{
    if (m_transitionMenu == nullptr)
    {
        m_transitionMenu = new AFQCustomMenu(this);
        m_transitionMenu->setStyleSheet("background-color: rgb(36,39,45);");

        auto& stateApp = STATEAPP;
        //
        QAction* action = nullptr;

        m_pDuplicateScene = m_transitionMenu->addAction(QTStr("QuickTransitions.DuplicateScene"));
        m_pDuplicateScene->setToolTip(QTStr("QuickTransitions.DuplicateSceneTT"));
        m_pDuplicateScene->setCheckable(true);
        m_pDuplicateScene->setChecked(stateApp.GetSceneDuplicationMode());
        connect(m_pDuplicateScene, &QAction::triggered, this, &AFQProgramView::qslotToggleSceneDuplication);
        //connect(action, &QAction::hovered, showToolTip);

        m_pEditProperties = m_transitionMenu->addAction(QTStr("QuickTransitions.EditProperties"));
        m_pEditProperties->setToolTip(QTStr("QuickTransitions.EditPropertiesTT"));
        m_pEditProperties->setCheckable(true);
        m_pEditProperties->setChecked(stateApp.GetEditPropertiesMode());
        m_pEditProperties->setEnabled(stateApp.GetSceneDuplicationMode());
        connect(m_pEditProperties, &QAction::triggered, this, &AFQProgramView::qslotToggleEditProperties);
        //connect(action, &QAction::hovered, showToolTip);

        m_pSwapScenesAction = m_transitionMenu->addAction(QTStr("QuickTransitions.SwapScenes"));
        m_pSwapScenesAction->setToolTip(QTStr("QuickTransitions.SwapScenesTT"));
        m_pSwapScenesAction->setCheckable(true);
        m_pSwapScenesAction->setChecked(stateApp.GetSwapScenesMode());
        connect(m_pSwapScenesAction, &QAction::triggered, this, &AFQProgramView::qslotToggleSwapScenesMode);
        //connect(action, &QAction::hovered, showToolTip);


        QSize menuSize = m_transitionMenu->sizeHint();
        m_transitionMenu->setMinimumWidth(menuSize.width() + 10);
    }
}
