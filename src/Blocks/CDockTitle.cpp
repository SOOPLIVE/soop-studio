#include "CDockTitle.h"

#include "ui_block-basic-dock-title.h"

#include <QWindow>
#include <QVBoxLayout>
#include <QStyle>
#include <QMouseEvent>

#include "qt-wrapper.h"

#include "CoreModel/Locale/CLocaleTextManager.h"

#include "UIComponent/CBasicHoverWidget.h"
#include "UIComponent/CCustomMenu.h"

#define DOCK_TITLE_HEIGHT   (40)

AFDockTitle::AFDockTitle(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::AFDockTitle)
{
    setMouseTracking(true);

    ui->setupUi(this);
    ui->pushButton_ShowSceneControlDock->hide();
}

AFDockTitle::~AFDockTitle()
{
    delete ui;
}


void AFDockTitle::qslotShowSceneSourceMenu()
{
    if (!m_qAddSceneSourceMenu->isVisible()) {
        QPushButton* plusButton = reinterpret_cast<QPushButton*>(sender());
        QPoint globalposition = mapToGlobal(pos());
        QPoint menuPosition = QPoint(globalposition.x() + plusButton->x() + plusButton->width() - m_qMenu->width(),
            globalposition.y() + height() - 2); //(8) menu bottom margin - (6) menu margin:design

        m_qAddSceneSourceMenu->show(menuPosition);
    }
}

void AFDockTitle::qslotHideSceneSourceMenu()
{
    if (m_qAddSceneSourceMenu != nullptr)
        m_qAddSceneSourceMenu->hide();
}

void AFDockTitle::qslotToggleDock()
{
    QPushButton* ToggleDockButton = reinterpret_cast<QPushButton*>(sender());

    m_bPopup = m_bPopup ? false : true;
    if(m_qMenu != nullptr)
        _ToggleWindowToDock(m_bPopup);
    emit qsignalToggleDock(m_bPopup, m_sBlockType);
}

void AFDockTitle::qslotMaximumPopupTriggered()
{
    emit qsignalMaximumPopup(m_sBlockType);
}

void AFDockTitle::qslotMinimumPopupTriggered()
{
    emit qsignalMinimumPopup(m_sBlockType);
}

void AFDockTitle::qslotCloseButtonTriggered()
{
    emit qsignalClose(m_sBlockType);
}

void AFDockTitle::qslotCloseCustomBroserTriggered()
{
    QString uuid = property("uuid").toString();
    emit qsignalCustomBrowserClose(uuid);
}

void AFDockTitle::qslotMaximizeCustomBrowserTriggered() 
{
    QString uuid = property("uuid").toString();
    emit qsignalCustomBrowserMaximize(uuid);
}

void AFDockTitle::qslotMinimizeCustomBrowserTriggered() 
{
    QString uuid = property("uuid").toString();
    emit qsignalCustomBrowserMinimize(uuid);
}

void AFDockTitle::qslotAddSceneTriggered()
{
    emit qsignalAddScene();
}

void AFDockTitle::qslotAddSourceTriggered()
{
    emit qsignalAddSource();
}

void AFDockTitle::qslotTransitionScenePopup()
{
    emit qsignalTransitionScenePopup();
}

void AFDockTitle::qslotShowSceneControlDock()
{
    emit qsignalShowSceneControlDock();
}

void AFDockTitle::qslotChangeMaximizeIcon(bool maximize)
{
    ChangeMaximizedIcon(maximize);
}

void AFDockTitle::qslotShowMenu()
{
    if (m_qMenu)
    {
        QPushButton* threedotbutton = reinterpret_cast<QPushButton*>(sender());
        QPoint globalposition = mapToGlobal(pos());
        QPoint menuPosition = QPoint(globalposition.x() +  threedotbutton->x() + threedotbutton->width() - m_qMenu->width(),
            globalposition.y() + height() - 2); //(8) menu bottom margin

        m_qMenu->show(menuPosition);
    }
}

void AFDockTitle::Initialize(bool onlyPopup, QString text, const char* BlockType)
{
    ui->pushButton_MinimumWindow->hide();
    ui->pushButton_MaximumWindow->hide();

    m_sBlockType = BlockType;
    if (onlyPopup)
    {
        ui->pushButton_Close->hide();

        _MakeCustomMenu();
        connect(ui->pushButton_ThreeDots, &QPushButton::clicked, this, &AFDockTitle::qslotShowMenu);
    }
    else
    {
        DeleteTreeDotsButton();
    }
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFDockTitle::qslotCloseButtonTriggered);
    connect(ui->pushButton_MinimumWindow, &QPushButton::clicked, this, &AFDockTitle::qslotMinimumPopupTriggered);
    connect(ui->pushButton_MaximumWindow, &QPushButton::clicked, this, &AFDockTitle::qslotMaximumPopupTriggered);

    ChangeLabelText(text);
}

void AFDockTitle::Initialize(QString customName)
{    
    DeleteTreeDotsButton();
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFDockTitle::qslotCloseCustomBroserTriggered);
    connect(ui->pushButton_MinimumWindow, &QPushButton::clicked, this, &AFDockTitle::qslotMinimizeCustomBrowserTriggered);
    connect(ui->pushButton_MaximumWindow, &QPushButton::clicked, this, &AFDockTitle::qslotMaximizeCustomBrowserTriggered);

    ChangeLabelText(customName);
}

void AFDockTitle::AddButton(QAbstractButton* button)
{
    button->setMinimumSize(20, 20);
    button->setMaximumSize(20, 20);
    ui->horizontalLayout->addWidget(button);
}

void AFDockTitle::AddButton(QList<QAbstractButton*> buttons)
{
    foreach(QAbstractButton * b, buttons)
    {
        b->setMinimumSize(25, 25);
        b->setMaximumSize(25, 25);
        ui->horizontalLayout->addWidget(b);
    }
}

void AFDockTitle::ChangeLabelText(QString text)
{
    ui->label->setText(text);
}

QString AFDockTitle::GetLabelText()
{
    return ui->label->text();
}

void AFDockTitle::SetToggleWindowToDockButton(bool checked)
{
    m_bPopup = checked;
    if (m_qMenu != nullptr)
        _ToggleWindowToDock(m_bPopup);
}

void AFDockTitle::DeleteTreeDotsButton()
{
    ui->pushButton_ThreeDots->close();
    delete ui->pushButton_ThreeDots;
    ui->pushButton_ThreeDots = nullptr;
}

void AFDockTitle::DeleteSceneCollection()
{
    ui->pushButton_ShowSceneControlDock->close();
    delete ui->pushButton_ShowSceneControlDock;
    ui->pushButton_ShowSceneControlDock = nullptr;
}

bool AFDockTitle::IsMoving() const
{
    return m_bMoving;
}

void AFDockTitle::SetMoving(bool moving)
{
    if (m_bMoving == moving) {
        return;
    }

    m_bMoving = moving;
}

bool AFDockTitle::IsFloating() const
{
    return m_bFloating;
}

void AFDockTitle::SetFloating(bool floating)
{
    if (m_bFloating == floating) {
        return;
    }
    m_bFloating = floating;

    style()->polish(this);
}

bool AFDockTitle::IsMoveAndFloat() const
{
    return IsMoving() && m_bFloating;
}


void AFDockTitle::ChangeMaximizedIcon(bool isMaximized)
{
    if (!isMaximized) {
        ui->pushButton_MaximumWindow->setObjectName("pushButton_MaximumWindow");
    }
    else {
        ui->pushButton_MaximumWindow->setObjectName("pushButton_MaxedWindow");
    }

    m_bIsMaximized = isMaximized;
    ui->pushButton_MaximumWindow->style()->unpolish(ui->pushButton_MaximumWindow);
    ui->pushButton_MaximumWindow->style()->polish(ui->pushButton_MaximumWindow);
}


void AFDockTitle::_ToggleWindowToDock(bool toggle)
{
    auto& locale = AFLocaleTextManager::GetSingletonInstance();
    
    if (toggle)
    {
        m_ToggleDockAction->setText(QT_UTF8(locale.Str("Block.Title.ToPopup")));
        m_CloseAction->setVisible(true);
        ui->pushButton_Close->hide();
        ui->pushButton_MinimumWindow->hide();
        ui->pushButton_MaximumWindow->hide();
    }
    else
    {
        m_ToggleDockAction->setText(QT_UTF8(locale.Str("Block.Title.ToDock")));
        m_CloseAction->setVisible(false);
        ui->pushButton_Close->show();
        ui->pushButton_MinimumWindow->show();
        ui->pushButton_MaximumWindow->show();
    }
}

void AFDockTitle::_MakeCustomMenu()
{
    auto& locale = AFLocaleTextManager::GetSingletonInstance();
    
    m_qMenu = new AFQCustomMenu(this);
    m_qMenu->setFixedWidth(200);

    m_ToggleDockAction = new QAction(m_qMenu);
    m_ToggleDockAction->setText("Switch To Dock");
    connect(m_ToggleDockAction, &QAction::triggered, this, &AFDockTitle::qslotToggleDock);
    m_qMenu->addAction(m_ToggleDockAction);

    m_CloseAction = new QAction(m_qMenu);
    m_CloseAction->setText(QT_UTF8(locale.Str("Block.Title.CloseDock")));
    connect(m_CloseAction, &QAction::triggered, this, &AFDockTitle::qslotCloseButtonTriggered);
    m_qMenu->addAction(m_CloseAction);

    if (0 == strcmp(m_sBlockType, "SceneSource")) {

        m_TransitionScene = new QAction(m_qMenu);
        m_TransitionScene->setText(QT_UTF8(locale.Str("Basic.TransitionEffect")));
        m_TransitionScene->setObjectName("action_TransitionSceneEffect");
        connect(m_TransitionScene, &QAction::triggered, this, &AFDockTitle::qslotTransitionScenePopup);
        m_qMenu->addAction(m_TransitionScene);

        ui->pushButton_ShowSceneControlDock->show();
        connect(ui->pushButton_ShowSceneControlDock, &QPushButton::clicked,
                this, &AFDockTitle::qslotShowSceneControlDock);

        //_AddSceneSourceButton();
    }
    else
    {
        DeleteSceneCollection();
    }
}