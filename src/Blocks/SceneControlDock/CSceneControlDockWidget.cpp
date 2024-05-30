#include "CSceneControlDockWidget.h"


#include <QSpacerItem>
#include <QSvgRenderer>
#include <QPainter>
#include <QImage>


#include "CProjector.h"



AFSceneControlDockWidget::AFSceneControlDockWidget(QWidget *parent) :
    AFQBaseDockWidget(parent)
{
    if (this->objectName().isEmpty())
        this->setObjectName("AFSceneControlDockWidget");
    this->resize(SCENE_CONTROL_MIN_SIZE_WIDTH, SCENE_CONTROL_MIN_SIZE_HEIGTH);
    this->setMinimumSize(SCENE_CONTROL_MIN_SIZE_WIDTH, SCENE_CONTROL_MIN_SIZE_HEIGTH);
    this->setAutoFillBackground(false);
    this->setStyleSheet(QString::fromUtf8(""));
    this->setFeatures(QDockWidget::DockWidgetClosable|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetMovable);
    this->setAllowedAreas(Qt::AllDockWidgetAreas);
    
    this->setWidget(&m_Contents);
    this->setVisible(false);
    
    connect(this, &AFSceneControlDockWidget::topLevelChanged, this, &AFSceneControlDockWidget::qslotChangedTopLevel);
    connect(this, &AFSceneControlDockWidget::visibilityChanged, this, &AFSceneControlDockWidget::qslotChangedVisibility);

    
    _InitLayout();
}

AFSceneControlDockWidget::~AFSceneControlDockWidget()
{
    _ReleaseLayoutObj();
}

void AFSceneControlDockWidget::qslotChangedTopLevel(bool value)
{
    QWidget* tmpProjector = m_MultiView;
    AFQProjector* pProjector = static_cast<AFQProjector*>(tmpProjector);
    AFQTDisplay* pDisplayProjector = static_cast<AFQTDisplay*>(tmpProjector);
    
    if (m_bNeverDocked == false || value == false)
    {
        m_bNeverDocked = false;
        
        QSize dockSize = this->size();
        QWidget* titleBar = titleBarWidget();
        int titleH = 0;
        if (titleBar != nullptr)
            titleH = titleBar->size().height();
        
        pDisplayProjector->resize(dockSize.width() - 40,
                                  dockSize.height() - titleH - 70);
    }
}

void AFSceneControlDockWidget::qslotChangedVisibility(bool value)
{
    QWidget* tmpProjector = m_MultiView;
    AFQProjector* pProjector = static_cast<AFQProjector*>(tmpProjector);
}

void AFSceneControlDockWidget::qslotTransitionUIStyle(QEvent::Type type)
{
    if (type == QEvent::HoverEnter)
    {
        m_pTransitionButton->setStyleSheet("QPushButton {color:#00E0FF; font-size: 13px; \
                                                         background-color:#333333; }" );
        m_pTransitionImg->setStyleSheet("QLabel { background-color:#333333; }");
        m_pTransitionAreaContents->setStyleSheet("#widget_TransitionAreaContents { \
                                                    border: 1px solid #00E0FF; border-radius: 4px; \
                                                    background-color:#333333;}" );
    }
    else if (type == QEvent::HoverLeave)
    {
        m_pTransitionButton->setStyleSheet("QPushButton {color:#00E0FF; font-size: 13px; }" );
        m_pTransitionImg->setStyleSheet("QLabel { background-color:#000000; }");
        m_pTransitionAreaContents->setStyleSheet(
             "#widget_TransitionAreaContents {border: 1px solid #00E0FF; border-radius: 4px;}" );
    }
}

void AFSceneControlDockWidget::ChangeLayoutStudioMode(bool studioMode)
{
    if (studioMode)
    {
        m_MainLayout.addWidget(m_pExpandedAreaContents);
        m_pExpandedAreaContents->setParent(&m_Contents);
        m_pExpandedAreaContents->show();

        m_MainLayout.update();
        m_pExpandedAreaLayout->update();
        
        m_pExpandedAreaContents->repaint();
        m_Contents.repaint();
    }
    else
    {
        m_MainLayout.removeWidget(m_pExpandedAreaContents);
        m_pExpandedAreaContents->setParent(this);
    }
}

void AFSceneControlDockWidget::_InitLayout()
{
    m_pTransitionButton = new QPushButton();
    m_pTransitionButton->setText("선택화면을 송출화면으로 전환");
    m_pTransitionButton->setStyleSheet("QPushButton {color:#00E0FF; font-size: 13px; }" );
    m_pTransitionButton->setBaseSize(130, 13);

    connect(m_pTransitionButton, &QPushButton::clicked, this,
            &AFSceneControlDockWidget::qSignalTransitionButtonClicked);

    QString svgString = QStringLiteral(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"8\" height=\"8\" viewBox=\"0 0 8 8\" fill=\"none\">"
            "<path d=\"M4.05517 1.71774C3.76131 1.41638 3.76131 0.927773 4.05517 0.626412C4.34903 0.32505 4.82547 0.32505 5.11933 0.626412L7.77945 3.35443C8.05494 3.63696 8.07245 4.08435 7.83139 4.38745C7.81532 4.40765 7.7981 4.42722 7.77973 4.44605L5.11933 7.17437C4.82547 7.47573 4.34903 7.47573 4.05517 7.17437C3.76131 6.87301 3.76131 6.3844 4.05517 6.08304L5.43097 4.67213L0.752476 4.67213C0.336895 4.67213 -3.17145e-07 4.32663 0 3.90044C-1.7443e-07 3.47425 0.336895 3.12876 0.752476 3.12876L5.43107 3.12876L4.05517 1.71774Z\" fill=\"#00E0FF\"/>"
            "</svg>"
        );
    
    QSvgRenderer renderer(svgString.toUtf8());
    QSize imageSize(8, 8);
    QImage image(imageSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    renderer.render(&painter);
    painter.end();
    m_pTransitionImg = new QLabel();
    m_pTransitionImg->setMinimumSize(QSize(8,7));
    m_pTransitionImg->setMaximumSize(QSize(8,7));
    m_pTransitionImg->setPixmap(QPixmap::fromImage(image));
    
    
    m_pTransitionAreaLayout = new QHBoxLayout();
    m_pTransitionAreaLayout->setContentsMargins(10, 10, 10, 10);
    m_pTransitionAreaLayout->setSpacing(4);
    m_pTransitionAreaLayout->addWidget(m_pTransitionButton);
    m_pTransitionAreaLayout->addWidget(m_pTransitionImg);
    
    m_pTransitionAreaContents = new QWidget();
    m_pTransitionAreaContents->setObjectName("widget_TransitionAreaContents");
    m_pTransitionAreaContents->setLayout(m_pTransitionAreaLayout);
    m_pTransitionAreaContents->setMinimumHeight(33);
    m_pTransitionAreaContents->setMaximumHeight(33);
    m_pTransitionAreaContents->setStyleSheet(
         "#widget_TransitionAreaContents {border: 1px solid #00E0FF; border-radius: 4px;}" );
    
    AFSimpleHoverEventFilter* eventFilter = new AFSimpleHoverEventFilter();
    m_pTransitionButton->installEventFilter(eventFilter);
    
    connect(eventFilter, &AFSimpleHoverEventFilter::qsignalHoverEventOccurred,
            this, &AFSceneControlDockWidget::qslotTransitionUIStyle);
    
    
    m_pExpandedAreaLayout = new QHBoxLayout();
    m_pExpandedAreaLayout->setContentsMargins(0, 0, 0, 0);
    m_pExpandedAreaLayout->setSpacing(0);
    m_pExpandedAreaLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    m_pExpandedAreaLayout->addWidget(m_pTransitionAreaContents);
    m_pExpandedAreaLayout->addSpacerItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    m_pExpandedAreaContents = new QWidget();
    m_pExpandedAreaContents->setParent(this);
    m_pExpandedAreaContents->setLayout(m_pExpandedAreaLayout);
    m_pExpandedAreaContents->setMinimumHeight(33);
    m_pExpandedAreaContents->setMaximumHeight(33);
    
    m_pExpandedAreaLayout->setParent(m_pExpandedAreaContents);
    
    m_MainLayout.setContentsMargins(20, 35, 20, 35);
    
    m_Contents.setParent(this);
    m_Contents.setLayout(&m_MainLayout);
    m_Contents.setStyleSheet("background-color:#24272D;");
    
    m_MultiView = qobject_cast<QWidget*>(new AFQProjector(nullptr, nullptr, -1, ProjectorType::Multiview));
    
    if (m_MultiView != nullptr)
        m_MainLayout.addWidget(m_MultiView);
}

void AFSceneControlDockWidget::_ReleaseLayoutObj()
{
    // delete m_MultiView;
}
