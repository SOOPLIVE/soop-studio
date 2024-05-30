#include "CVerticalProgramView.h"
#include "ui_main-vertical-studio-mode-view.h"

#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "qt-wrapper.h"

#include "Application/CApplication.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Scene/CScene.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Graphics/CGraphicsContext.h"

AFQVerticalProgramView::AFQVerticalProgramView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQVerticalProgramView)
{
    ui->setupUi(this);
}

AFQVerticalProgramView::~AFQVerticalProgramView()
{
    delete ui;
}

//void AFQVerticalProgramView::qslotChangeLayoutStrech()
//{
//    auto& graphicsContext = AFGraphicsContext::GetSingletonInstance();
//
//    QVBoxLayout* thisLayout = qobject_cast<QVBoxLayout*>(this->layout());
//
//    int32_t previewY = graphicsContext.GetMainPreviewY();
//    int32_t previewCY = graphicsContext.GetMainPreviewCY();
//
//    int32_t firstValue = thisLayout->stretch(0);
//
//
//    if (firstValue != 0 &&
//        std::abs(thisLayout->stretch(0) - previewY) <= 1)
//        return;
//
//    thisLayout->setStretch(0, previewY);
//    thisLayout->setStretch(1, previewY + previewCY);
//}

void AFQVerticalProgramView::_qslotTransitionClicked() {
// if(tmpStateApp->JustCheckPreviewProgramMode());
//
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    AFSceneUtil::TransitionToScene(
        AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrOBSScene()));

    ChangeLiveSceneName(ui->label_EditSceneName->text());
}

void AFQVerticalProgramView::_qslotMenuTransitionClicked()
{
    if (m_qTransitionMenu != nullptr)
        m_qTransitionMenu->exec(QCursor::pos());
}

void AFQVerticalProgramView::_qslotSetTransitionWidgetStyleDefault()
{
    ui->widget_Transition->setStyleSheet("#widget_Transition{ border-radius: 5px;\
                                                   border: 1px solid #007080;\
                                                   background-color: transparent; }");
    style()->unpolish(ui->widget_Transition);
    style()->polish(ui->widget_Transition);
}

void AFQVerticalProgramView::_qslotSetTransitionWidgetStyleHovered()
{
    ui->label_TransitionProgram->setStyleSheet("QLabel{ background-color: transparent;}");
    ui->label_TransitionImg->setStyleSheet("QLabel{ background-color: transparent;}");
    ui->widget_Transition->setStyleSheet("#widget_Transition{ background: #333333; }");
    //style()->unpolish(ui->widget_Transition);
    //style()->polish(ui->widget_Transition);
}

void AFQVerticalProgramView::_qslotSetTransitionButtonStyleDefault()
{
    ui->pushButton_MenuTransition->setStyleSheet("QPushButton{ background: transparent; }");
}

void AFQVerticalProgramView::_qslotSetTransitionButtonStyleHovered()
{
    ui->pushButton_MenuTransition->setStyleSheet("QPushButton{ background: #333333; }");
}

void AFQVerticalProgramView::qslotToggleSwapScenesMode()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto* tmpStateApp = confManager.GetStates();

    bool tmpValue = tmpStateApp->GetSwapScenesMode();
    tmpStateApp->SetSwapScenesMode(!tmpValue);
}

void AFQVerticalProgramView::qslotToggleEditProperties()
{
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto* tmpStateApp = confManager.GetStates();

    bool tmpValue = tmpStateApp->GetEditPropertiesMode();
    tmpStateApp->SetEditPropertiesMode(!tmpValue);

    OBSSource actualScene = OBSGetStrongRef(sceneContext.GetProgramOBSScene());
    if (actualScene)
        AFSceneUtil::TransitionToScene(actualScene, true);
}

void AFQVerticalProgramView::qslotToggleSceneDuplication()
{
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto* tmpStateApp = confManager.GetStates();

    bool tmpValue = tmpStateApp->GetSceneDuplicationMode();
    tmpStateApp->SetSceneDuplicationMode(!tmpValue);

    OBSSource actualScene = OBSGetStrongRef(sceneContext.GetProgramOBSScene());
    if (actualScene)
        AFSceneUtil::TransitionToScene(actualScene, true);

    m_qEditProperties->setEnabled(m_qDuplicateScene->isChecked());
}

bool AFQVerticalProgramView::Initialize()
{
    _SetButtons();
    _InitValueLabels();
    _InitTransitionMenu();

    return true;
}

void AFQVerticalProgramView::InsertDisplays(QWidget* mainDisplay, QWidget* programDisplay)
{
    QHBoxLayout* tmpMainHBoxLayout = new QHBoxLayout();
    tmpMainHBoxLayout->setContentsMargins(16, 0, 16, 0);
    tmpMainHBoxLayout->setSpacing(16);
    tmpMainHBoxLayout->addWidget(mainDisplay);

    QHBoxLayout* tmpProgramHBoxLayout = new QHBoxLayout();
    tmpProgramHBoxLayout->setContentsMargins(16, 0, 16, 0);
    tmpProgramHBoxLayout->setSpacing(16);
    tmpProgramHBoxLayout->addWidget(programDisplay);

#ifdef __APPLE__
    programDisplay->show();
#endif

    ui->widget_ContainerEditDisplay->setLayout(tmpMainHBoxLayout);
    ui->widget_ContainerLiveDisplay->setLayout(tmpProgramHBoxLayout);
}

void AFQVerticalProgramView::ChangeEditSceneName(QString name)
{
    ui->label_EditSceneName->setText(name);
}

void AFQVerticalProgramView::ChangeLiveSceneName(QString name)
{
    ui->label_LiveSceneName->setText(name);
}

void AFQVerticalProgramView::ToggleSceneLabel(bool visible)
{
    ui->widget_EditLabel->setVisible(visible);
    ui->widget_DisplayName->setVisible(visible);
}

void AFQVerticalProgramView::qslotTransitionTriggered()
{
    _qslotTransitionClicked();
}

void AFQVerticalProgramView::_SetButtons()
{
    connect(ui->widget_Transition, &AFQBaseClickWidget::qsignalLeftClicked,
        this, &AFQVerticalProgramView::_qslotTransitionClicked);

    connect(ui->widget_Transition, &AFQBaseClickWidget::qsignalHoverEntered,
        this, &AFQVerticalProgramView::_qslotSetTransitionWidgetStyleHovered);
    connect(ui->widget_Transition, &AFQBaseClickWidget::qsignalHoverLeaved,
        this, &AFQVerticalProgramView::_qslotSetTransitionWidgetStyleDefault);
    connect(ui->pushButton_MenuTransition, &AFQCustomPushbutton::qsignalButtonEnter,
        this, &AFQVerticalProgramView::_qslotSetTransitionButtonStyleHovered);
    connect(ui->pushButton_MenuTransition, &AFQCustomPushbutton::qsignalButtonLeave,
        this, &AFQVerticalProgramView::_qslotSetTransitionButtonStyleDefault);
    connect(ui->pushButton_MenuTransition, &AFQCustomPushbutton::clicked,
        this, &AFQVerticalProgramView::_qslotSetTransitionButtonStyleDefault);
    //

    //connect(ui->pushButton_MenuTransition, &QPushButton::clicked,
    //    this, &AFQVerticalProgramView::_qslotMenuTransitionClicked);
    connect(ui->pushButton_MenuTransition, &AFQCustomPushbutton::clicked,
        this, &AFQVerticalProgramView::_qslotMenuTransitionClicked);
}

void AFQVerticalProgramView::_InitValueLabels()
{
    auto& localeTextManager = AFLocaleTextManager::GetSingletonInstance();

    //ui->label_EditDisplay->SetContentMargins(10, 6, 10, 6);
    //ui->label_LiveDisplay->SetContentMargins(10, 6, 10, 6);

    ui->label_EditDisplay->setText(QT_UTF8(localeTextManager.Str("StudioMode.Preview")));
    ui->label_LiveDisplay->setText(QT_UTF8(localeTextManager.Str("StudioMode.Program")));

    int widthEditText = ui->label_EditDisplay->sizeHint().width();
    int widthLiveText = ui->label_LiveDisplay->sizeHint().width();

    ui->label_EditDisplay->setMinimumWidth(widthEditText);
    ui->label_LiveDisplay->setMinimumWidth(widthLiveText);


    ui->label_TransitionProgram->setText(QT_UTF8(localeTextManager.Str("Transition")));

    int widthTransition = ui->label_TransitionProgram->sizeHint().width();
    ui->label_TransitionProgram->setMinimumWidth(widthTransition);
}

void AFQVerticalProgramView::_InitTransitionMenu()
{
    if (m_qTransitionMenu == nullptr)
    {
        m_qTransitionMenu = new AFQCustomMenu(this);
        m_qTransitionMenu->setStyleSheet("background-color: rgb(36,39,45);");

        auto& locale = AFLocaleTextManager::GetSingletonInstance();
        auto& confManager = AFConfigManager::GetSingletonInstance();
        auto* tmpStateApp = confManager.GetStates();

        QAction* action = nullptr;

        m_qDuplicateScene = m_qTransitionMenu->addAction(QT_UTF8(locale.Str("QuickTransitions.DuplicateScene")));
        m_qDuplicateScene->setToolTip(QT_UTF8(locale.Str("QuickTransitions.DuplicateSceneTT")));
        m_qDuplicateScene->setCheckable(true);
        m_qDuplicateScene->setChecked(tmpStateApp->GetSceneDuplicationMode());
        connect(m_qDuplicateScene, &QAction::triggered, this, &AFQVerticalProgramView::qslotToggleSceneDuplication);
        //connect(action, &QAction::hovered, showToolTip);

        m_qEditProperties = m_qTransitionMenu->addAction(QT_UTF8(locale.Str("QuickTransitions.EditProperties")));
        m_qEditProperties->setToolTip(QT_UTF8(locale.Str("QuickTransitions.EditPropertiesTT")));
        m_qEditProperties->setCheckable(true);
        m_qEditProperties->setChecked(tmpStateApp->GetEditPropertiesMode());
        m_qEditProperties->setEnabled(tmpStateApp->GetSceneDuplicationMode());
        connect(m_qEditProperties, &QAction::triggered, this, &AFQVerticalProgramView::qslotToggleEditProperties);
        //connect(action, &QAction::hovered, showToolTip);

        m_qSwapScenesAction = m_qTransitionMenu->addAction(QT_UTF8(locale.Str("QuickTransitions.SwapScenes")));
        m_qSwapScenesAction->setToolTip(QT_UTF8(locale.Str("QuickTransitions.SwapScenesTT")));
        m_qSwapScenesAction->setCheckable(true);
        m_qSwapScenesAction->setChecked(tmpStateApp->GetSwapScenesMode());
        connect(m_qSwapScenesAction, &QAction::triggered, this, &AFQVerticalProgramView::qslotToggleSwapScenesMode);
        //connect(action, &QAction::hovered, showToolTip);


        QSize menuSize = m_qTransitionMenu->sizeHint();
        m_qTransitionMenu->setMinimumWidth(menuSize.width() + 10);
    }
}
