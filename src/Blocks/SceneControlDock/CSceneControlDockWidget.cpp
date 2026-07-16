#include "CSceneControlDockWidget.h"


#include <QSpacerItem>
#include <QSvgRenderer>
#include <QPainter>
#include <QImage>

#include "Application/CApplication.h"
#include "CProjector.h"

#include "MainFrame/CMainFrame.h"


AFSceneControlWidget::AFSceneControlWidget(QWidget *parent) :
    QWidget(parent)
{
    if (objectName().isEmpty())
        setObjectName("AFSceneControlWidget");
    //resize(SCENE_CONTROL_MIN_SIZE_WIDTH, SCENE_CONTROL_MIN_SIZE_HEIGTH);
    //setMinimumSize(SCENE_CONTROL_MIN_SIZE_WIDTH, SCENE_CONTROL_MIN_SIZE_HEIGTH);
    //setAutoFillBackground(false);
    //setStyleSheet(QString::fromUtf8(""));
    

    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);
    layout->addWidget(&m_contents);

    setLayout(layout);
    
    _InitLayout();
}

AFSceneControlWidget::~AFSceneControlWidget()
{
    _ReleaseLayoutObj();
}

void AFSceneControlWidget::qslotStudioModeToggled()
{
    ChangeLayoutStudioMode();
}

void AFSceneControlWidget::ChangeLayoutStudioMode()
{
    bool studioMode = MAINFRAME->IsPreviewProgramMode();
    DYNAMIC_COMPOSIT->SceneControlStudioModeSignal(this, studioMode);
    m_pExpandedAreaContents->setVisible(studioMode);
    if (studioMode)
    {
        m_mainLayout.addWidget(m_pExpandedAreaContents);
        m_pExpandedAreaContents->setParent(&m_contents);

        m_mainLayout.update();
        m_pExpandedAreaLayout->update();
        
        m_pExpandedAreaContents->repaint();
        m_contents.repaint();
    }
    else
    {
        m_mainLayout.removeWidget(m_pExpandedAreaContents);
        m_pExpandedAreaContents->setParent(this);
    }
}

void AFSceneControlWidget::_InitLayout()
{
    m_pTransitionButton = new QPushButton();
    m_pTransitionButton->setObjectName("pushButton_Transition");
    m_pTransitionButton->setText(QTStr("StudioMode.TransitionButton"));
    m_pTransitionButton->setFixedSize(130, 40);

    connect(m_pTransitionButton, &QPushButton::clicked, this, &AFSceneControlWidget::qSignalTransitionButtonClicked);

    m_pExpandedAreaLayout = new QHBoxLayout();
    m_pExpandedAreaLayout->setContentsMargins(0, 0, 0, 0);
    m_pExpandedAreaLayout->setSpacing(0);
    m_pExpandedAreaLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    m_pExpandedAreaLayout->addWidget(m_pTransitionButton);
    m_pExpandedAreaLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    m_pExpandedAreaContents = new QWidget();
    m_pExpandedAreaContents->setParent(this);
    m_pExpandedAreaContents->setLayout(m_pExpandedAreaLayout);
    m_pExpandedAreaContents->setFixedHeight(40);

    m_mainLayout.setContentsMargins(30, 20, 30, 20);
    
    m_contents.setParent(this);
    m_contents.setLayout(&m_mainLayout);
    
    m_multiView = qobject_cast<QWidget*>(new AFQProjector(nullptr, nullptr, -1, ProjectorType::Multiview));
    
    if (m_multiView != nullptr)
        m_mainLayout.addWidget(m_multiView);

    connect(DYNAMIC_COMPOSIT, &AFMainDynamicComposit::StudioModeToggled, this, &AFSceneControlWidget::qslotStudioModeToggled);
    ChangeLayoutStudioMode();
}

void AFSceneControlWidget::_ReleaseLayoutObj()
{
}
