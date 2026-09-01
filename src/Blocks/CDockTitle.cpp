#include "CDockTitle.h"

#include "ui_block-basic-dock-title.h"

#include <QWindow>
#include <QVBoxLayout>
#include <QStyle>
#include <QMouseEvent>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "Application/CApplication.h"

#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Auth/CAuthManager.h"

#include "UIComponent/CBasicHoverWidget.h"
#include "UIComponent/CCustomMenu.h"
#include "Utils/BreaktimeManager.h"

#include "MainFrame/CMainFrame.h"

#define DOCK_TITLE_HEIGHT   (40)

AFDockTitle::AFDockTitle(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::AFDockTitle)
{
    setMouseTracking(true);

    ui->setupUi(this);

    ui->pushButton_Refresh->hide();
    ui->stackedWidget->hide();
    ui->pushButton_FAQ->hide();

    ui->widget_Icon->setFixedSize(24, 24);

    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFDockTitle::qslotCloseButtonTriggered);
    connect(ui->pushButton_MinimumWindow, &QPushButton::clicked, this, &AFDockTitle::qslotMinimumPopupTriggered);
    connect(ui->pushButton_MaximumWindow, &QPushButton::clicked, this, &AFDockTitle::qslotMaximumPopupTriggered);
    connect(parent, &QWidget::windowTitleChanged, this, &AFDockTitle::qslotUpdateWindowTitle);

    MAIN_BLOCKMANAGER->ApplyMoveInAllArea(this);
    
    raise();
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
}

AFDockTitle::~AFDockTitle()
{
    delete ui;
}


void AFDockTitle::qslotShowSceneSourceMenu()
{
    if (!m_addSceneSourceMenu->isVisible()) {
        QPushButton* plusButton = reinterpret_cast<QPushButton*>(sender());
        QPoint globalposition = mapToGlobal(pos());
        QPoint menuPosition = QPoint(globalposition.x() + plusButton->x() + plusButton->width() - m_menu->width(),
            globalposition.y() + height() - 2); //(8) menu bottom margin - (6) menu margin:design

        m_addSceneSourceMenu->show(menuPosition);
    }
}

void AFDockTitle::qslotHideSceneSourceMenu()
{
    if (m_addSceneSourceMenu != nullptr)
        m_addSceneSourceMenu->hide();
}

void AFDockTitle::qslotToggleDock()
{
    m_popup = m_popup ? false : true;

    QString custom = property("customUuid").toString();

    if(m_blockType < 0)
        emit qsignalToggleCustomDock(m_popup, custom);
    else
        emit qsignalToggleDock(m_popup, m_blockType);

}

void AFDockTitle::qslotMaximumPopupTriggered()
{
    QWidget* topLevelWidget = window();
    if (topLevelWidget)
        if (topLevelWidget->isMaximized())
            topLevelWidget->showNormal();
        else
            topLevelWidget->showMaximized();
}

void AFDockTitle::qslotMinimumPopupTriggered()
{
    QWidget* topLevelWidget = window();
    if (topLevelWidget)
        topLevelWidget->showMinimized();
}

void AFDockTitle::qslotRefreshButtonTriggered()
{
    ui->pushButton_Refresh->setDisabled(true);
    QTimer::singleShot(3000, this, [this]() {
        ui->pushButton_Refresh->setEnabled(true);
        });

    emit qsignalRefreshButton(m_blockType);
}

void AFDockTitle::qslotCloseButtonTriggered()
{
    if (m_popup)
    {
        QWidget* topLevelWidget = window();
        if (topLevelWidget) {
            if (QTStr("Block.Tooltip.SavvyReaction") == topLevelWidget->windowTitle()) {
                QWidget* widget = nullptr;
                MAIN_BLOCKMANAGER->FindBlock(ENUM_WINDOW_TYPE::SAVVYReaction, widget);
                if (nullptr != widget) {
                    QCefWidget* cefWidget = reinterpret_cast<QCefWidget*>(widget);
                    cefWidget->executeJavaScript("window.postMessage({ type: 'close' }, '*');");
                    return;
                }
            }
            if(m_hidePopup)
                topLevelWidget->hide();
            else
                topLevelWidget->close();
        }            
    }
    else
    {
        QDockWidget* parentW = _CheckDock();
        if (parentW)
        {
            if(m_blockType > -1)
                MAIN_BLOCKMANAGER->qslotHideDock(m_blockType);
            else
            {
                QString customUuid = property("customUuid").toString();
                MAIN_BLOCKMANAGER->qslotCloseUuidDock(customUuid, true);
            }
        }
    }
}

void AFDockTitle::qslotTransitionScenePopup()
{
    emit qsignalTransitionScenePopup();
}

void AFDockTitle::qslotAdvAudioMixerPopup()
{
    emit qsignalAdvAudioMixerPopup();
}

void AFDockTitle::qslotChangeMaximizeIcon(bool maximize)
{
    ChangeMaximizedIcon(maximize);
}

void AFDockTitle::qslotShowMenu()
{
    if (m_menu)
    {
        QPushButton* threedotbutton = reinterpret_cast<QPushButton*>(sender());
        QPoint globalposition = mapToGlobal(pos());
        QPoint menuPosition = QPoint(globalposition.x() +  threedotbutton->x() + threedotbutton->width() - m_menu->width(),
                                     globalposition.y() + height() - 2); //(8) menu bottom margin

        if (ENUM_WINDOW_TYPE::AudioMixer == m_blockType)
        {
            bool vertical = config_get_bool(USERCONFIG, "BasicWindow", "VerticalVolControl");
            if (!vertical)
                m_changeAudioMixerLayout->setText(QTStr("AudioMixer.VerticalLayout"));
            else 
                m_changeAudioMixerLayout->setText(QTStr("AudioMixer.HorizontalLayout"));
        }

        m_menu->show(menuPosition);
    }
}

void AFDockTitle::Initialize(bool onlyPopup, QString text, int BlockType, bool needQuestionMark, const QString& questionMarkToolTip)
{
    m_blockType = BlockType;
    setProperty("customUuid", "");

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
    
    if (!needQuestionMark)
        DeleteQuestionMarkButton();
    else
        ui->pushButton_QuestionMark->SetExplanationText(questionMarkToolTip);

    if(BlockType == ENUM_WINDOW_TYPE::BroadInfo &&
       config_get_bool(APPCONFIG, "General", "MigrationUser")) {

        ui->stackedWidget->show();
        ui->stackedWidget->setCurrentIndex(1);
        ui->pushButton_FAQ->show();
        ui->pushButton_FAQ->setText(QTStr("Block.FAQ"));

        QString imgPath = "";
        std::string absPath;
        GetDataFilePath("assets", absPath);
        imgPath = QString("%1/platform/FreecShotIcon.png")
            .arg(absPath.data());
        ui->pushButton_FAQ->setIcon(QIcon(imgPath));
        //
        connect(ui->pushButton_FAQ, &QPushButton::clicked, MAINFRAME, &AFMainFrame::qslotShowMigrationGuide);             
    }
    
    auto& breaktime = BREAKTIME_MANAGER;
    connect(&breaktime, &BreaktimeManager::signalTick, this, &AFDockTitle::qslotBreaktimeTicked);
    connect(&breaktime, &BreaktimeManager::signalFinished, this, &AFDockTitle::qslotBreaktimeFinished);

    connect(ui->widget_breaktime, &AFQHoverWidget::qsignalMousePressed, MAIN_BLOCKMANAGER, &AFQBlockManager::qslotBreaktimeShow);

    const QString title = QTStr("breaktime.broadtitle.title");
    const QString timeText = QTStr("breaktime.broadtitle.time").arg(9).arg(59);

    QFontMetrics fmTitle(ui->label_breaktime_title->font());
    QFontMetrics fmTime(ui->label_breaktime_time->font());

    int titleWidth = fmTitle.horizontalAdvance(title);
    int timeWidth = fmTime.horizontalAdvance(timeText);

    int spacing = 6;
    int padding = 20;

    int totalWidth = titleWidth + timeWidth + spacing + padding;

    ui->stackedWidget->setFixedWidth(totalWidth);

    if (BlockType == ENUM_WINDOW_TYPE::SceneSource ||
        BlockType == ENUM_WINDOW_TYPE::AudioMixer ||
        BlockType == ENUM_WINDOW_TYPE::BroadInfo) {
        QMetaEnum BlockTypeEnum = QMetaEnum::fromType<ENUM_WINDOW_TYPE>();
        const char* blockType = BlockTypeEnum.valueToKey(BlockType);
        ui->label_BlockIcon->setProperty("blockType", blockType);

    }
    else
        DeleteTitleIcon();

    setWindowTitle(text);
}

void AFDockTitle::InitializeCustom(QString customName, QString customUuid, bool minmax, bool threeDots, bool closeButton, bool needQuestionMark, const QString& questionMarkToolTip) 
{
    setProperty("customUuid", customUuid);
    m_blockType = -1;

    if(!threeDots)
        DeleteTreeDotsButton();
    else
    {
        _MakeCustomMenu();
        connect(ui->pushButton_ThreeDots, &QPushButton::clicked, this, &AFDockTitle::qslotShowMenu);
    }
    
    if (closeButton)
        ui->pushButton_Close->show();

    if (!needQuestionMark)
        DeleteQuestionMarkButton();
    else
        ui->pushButton_QuestionMark->SetExplanationText(questionMarkToolTip, AFQInfoTooltipButton::ToolTipPos::BottomLeft);

    setWindowTitle(customName);
    MinMaxButton(minmax);
}

void AFDockTitle::ShowRefreshButton()
{
    ui->pushButton_Refresh->show();

    connect(ui->pushButton_Refresh, &QPushButton::clicked,
            this, &AFDockTitle::qslotRefreshButtonTriggered);
}

void AFDockTitle::AddButton(QAbstractButton* button)
{
    button->setMinimumSize(20, 20);
    button->setMaximumSize(20, 20);
    ui->widget_ExtraButtons->layout()->addWidget(button);
}

void AFDockTitle::AddButton(QList<QAbstractButton*> buttons)
{
    foreach(QAbstractButton * b, buttons)
    {
        b->setMinimumSize(20, 20);
        b->setMaximumSize(20, 20);
        ui->widget_ExtraButtons->layout()->addWidget(b);
    }
}

void AFDockTitle::UpdateTitleLabel()
{
    int emptyWidth = 0;
    auto adjustLabelWidth = [&](QWidget* w) {
        if (w && w->isVisible()) emptyWidth += w->sizeHint().width();
        };

    adjustLabelWidth(ui->widget_Icon);
    adjustLabelWidth(ui->pushButton_Refresh);
    adjustLabelWidth(ui->pushButton_QuestionMark);
    adjustLabelWidth(ui->stackedWidget);
    adjustLabelWidth(ui->pushButton_ThreeDots);
    adjustLabelWidth(ui->pushButton_MinimumWindow);
    adjustLabelWidth(ui->pushButton_MaximumWindow);
    adjustLabelWidth(ui->pushButton_Close);

    QFontMetrics metricsTitle(ui->label->font());
    int titleWidth = metricsTitle.horizontalAdvance(windowTitle());
    QString x = windowTitle();
    int adjustTitleWidth = width() - emptyWidth - 20;

    int finalWidth = qMin(titleWidth, adjustTitleWidth);

    if ((finalWidth < 140) && !ui->pushButton_QuestionMark)
        finalWidth = 140;

    if (ui->label->width() != finalWidth)
        ui->label->setFixedWidth(finalWidth);

    QString elidedTitle = metricsTitle.elidedText(windowTitle(), Qt::ElideRight, finalWidth);
    ui->label->setText(elidedTitle);
}

QString AFDockTitle::GetLabelText()
{
    return ui->label->text();
}

void AFDockTitle::SetToggleWindowToDockButton(bool checked)
{
    m_popup = checked;
    if (m_menu != nullptr)
        _ToggleWindowToDock(m_popup);
}

void AFDockTitle::MinMaxButton(bool visible)
{    
    if (visible)
        _ConnectMaxIconChanged();

    ui->pushButton_MinimumWindow->setVisible(visible);
    ui->pushButton_MaximumWindow->setVisible(visible);
}

void AFDockTitle::DeleteTreeDotsButton()
{
    ui->pushButton_ThreeDots->close();
    delete ui->pushButton_ThreeDots;
    ui->pushButton_ThreeDots = nullptr;
}

void AFDockTitle::DeleteQuestionMarkButton()
{
    ui->pushButton_QuestionMark->close();
    delete ui->pushButton_QuestionMark;
    ui->pushButton_QuestionMark = nullptr;
}

void AFDockTitle::DeleteTitleIcon()
{
    ui->label_BlockIcon->close();
    delete ui->label_BlockIcon;
    ui->label_BlockIcon = nullptr;

    ui->widget_Icon->close();
    delete ui->widget_Icon;
    ui->widget_Icon = nullptr;
}

bool AFDockTitle::IsMoving() const
{
    return m_moving;
}

void AFDockTitle::SetMoving(bool moving)
{
    if (m_moving == moving) {
        return;
    }

    m_moving = moving;
}

bool AFDockTitle::IsFloating() const
{
    return m_floating;
}

void AFDockTitle::SetFloating(bool floating)
{
    if (m_floating == floating) {
        return;
    }
    m_floating = floating;

    style()->polish(this);
}

bool AFDockTitle::IsMoveAndFloat() const
{
    return IsMoving() && m_floating;
}


void AFDockTitle::ChangeMaximizedIcon(bool isMaximized)
{
    if (!isMaximized) {
        ui->pushButton_MaximumWindow->setObjectName("pushButton_MaximumWindow");
    }
    else {
        ui->pushButton_MaximumWindow->setObjectName("pushButton_MaxedWindow");
    }

    m_isMaximized = isMaximized;
    PolishStyleSheet(ui->pushButton_MaximumWindow);
}

void AFDockTitle::_ToggleWindowToDock(bool popup)
{
    if (popup)
    {
        m_toggleDockAction->setText(QTStr("Block.Title.ToDock"));
        m_closeAction->setVisible(false);
        ui->pushButton_Close->show();
        ui->pushButton_MinimumWindow->show();
        ui->pushButton_MaximumWindow->show();
    }
    else
    {
        m_toggleDockAction->setText(QTStr("Block.Title.ToPopup"));
        m_closeAction->setVisible(true);
        ui->pushButton_Close->hide();
        ui->pushButton_MinimumWindow->hide();
        ui->pushButton_MaximumWindow->hide();
    }
}

void AFDockTitle::_MakeCustomMenu()
{
    m_menu = new AFQCustomMenu(this);
    m_menu->setFixedWidth(200);

    if (m_blockType == ENUM_WINDOW_TYPE::BroadInfo)
    {
        AFChannelData* data = nullptr;
        if (AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, data))
        {
            if (data)
            {
                m_broadInfoSettingAction = new QAction(m_menu);
                m_broadInfoSettingAction->setText(QTStr("Block.Title.BroadInfo.Setting"));
                m_broadInfoSettingAction->setProperty("type", 1);
                m_broadInfoSettingAction->setProperty("channelID", QString::fromStdString(data->pAuthData->channelID));
                m_broadInfoSettingAction->setProperty("platformName", QString::fromStdString(data->pAuthData->platform));
                connect(m_broadInfoSettingAction, &QAction::triggered, MAINFRAME, &AFMainFrame::qslotShowStudioSettingWithButtonSender);
                m_menu->addAction(m_broadInfoSettingAction);
            }
        }
    }
    else if (ENUM_WINDOW_TYPE::SceneSource == m_blockType) //SceneSource
    {
        m_transitionScene = new QAction(m_menu);
        m_transitionScene->setText(QTStr("Basic.TransitionEffect"));
        m_transitionScene->setObjectName("action_TransitionSceneEffect");
        connect(m_transitionScene, &QAction::triggered, this, &AFDockTitle::qslotTransitionScenePopup);
        m_menu->addAction(m_transitionScene);
    }
    else if (ENUM_WINDOW_TYPE::AudioMixer == m_blockType) // AudioMixer
    {
        m_advAudioMixerShow = new QAction(m_menu);
        m_advAudioMixerShow->setText(QTStr("Basic.MainMenu.Edit.AdvAudio_NotShortCut"));
        connect(m_advAudioMixerShow, &QAction::triggered, this, &AFDockTitle::qslotAdvAudioMixerPopup);
        m_menu->addAction(m_advAudioMixerShow);

        bool vertical = config_get_bool(USERCONFIG, "BasicWindow", "VerticalVolControl");

        m_changeAudioMixerLayout = new QAction(m_menu);
        if (vertical) {
            m_changeAudioMixerLayout->setText(QTStr("AudioMixer.VerticalLayout"));
        }
        else {
            m_changeAudioMixerLayout->setText(QTStr("AudioMixer.HorizontalLayout"));
        }
        connect(m_changeAudioMixerLayout, &QAction::triggered, MAINFRAME,
            &AFMainFrame::qslotToggleVolControlLayout, Qt::DirectConnection);
        m_menu->addAction(m_changeAudioMixerLayout);
    }

    m_toggleDockAction = new QAction(m_menu);
    m_toggleDockAction->setText("Switch To Dock");
    connect(m_toggleDockAction, &QAction::triggered, this, &AFDockTitle::qslotToggleDock);
    m_menu->addAction(m_toggleDockAction);

    m_closeAction = new QAction(m_menu);
    m_closeAction->setText(QTStr("Block.Title.CloseDock"));
    connect(m_closeAction, &QAction::triggered, this, &AFDockTitle::qslotCloseButtonTriggered);
    m_menu->addAction(m_closeAction);

}

void AFDockTitle::_ConnectMaxIconChanged()
{
    QWidget* parentW = parentWidget();
    while (parentW)
    {
        AFTTopBaseWidget* topLevelWidget = reinterpret_cast<AFTTopBaseWidget*>(parentW);
        if (topLevelWidget)
        {
            connect(topLevelWidget->GetController(), &AFQBaseWindowController::qsignalMaximized,
                this, &AFDockTitle::qslotChangeMaximizeIcon);
            return;
        }

        AFTTopBaseDialog* topLevelDialog = reinterpret_cast<AFTTopBaseDialog*>(parentW);
        if (topLevelDialog)
        {
            connect(topLevelDialog->GetController(), &AFQBaseWindowController::qsignalMaximized,
                this, &AFDockTitle::qslotChangeMaximizeIcon);
            return;
        }
            
        parentW = parentW->parentWidget();
    }
}

QDockWidget* AFDockTitle::_CheckDock()
{
    QWidget* parentW = parentWidget();
    while (parentW)
    {
        QDockWidget* dock = qobject_cast<QDockWidget*>(parentW);
        if (dock)
            return dock;
        parentW = parentW->parentWidget();
    }
    return nullptr;
}


void AFDockTitle::TitleChangePage(int nPage)
{
    if (m_blockType == ENUM_WINDOW_TYPE::BroadInfo)
    {
        if(nPage == 0)
            ui->stackedWidget->show();
        else
        {
            if (config_get_bool(APPCONFIG, "General", "MigrationUser"))
            {
                ui->stackedWidget->show();
            }
            else
            {
                ui->stackedWidget->hide();
            }
        }
        ui->stackedWidget->setCurrentIndex(nPage);
    }    
}

void AFDockTitle::ChangeLabelFontSize(int fontSize)
{
    QString titleStyle = QString("AFDockTitle #label { font-size: %1px; font-weight: 500; color: palette(text);}").arg(QString::number(fontSize));
    ui->label->setStyleSheet(titleStyle);
}

QFont AFDockTitle::GetFont()
{
    return ui->label->font();
}

void AFDockTitle::resizeEvent(QResizeEvent* event)
{
    if(m_blockType <= -1)
        UpdateTitleLabel();
    QFrame::resizeEvent(event);
}

QSize AFDockTitle::minimumSizeHint() const
{
    return QSize(20, height());
}

void AFDockTitle::qslotBreaktimeTicked(int remainingSec, int /*totalSec*/)
{   
    int sec = qMax(0, remainingSec);
    const int m = remainingSec / 60, s = remainingSec % 60;
    
    ui->label_breaktime_title->setText(QTStr("breaktime.broadtitle.title"));
    ui->label_breaktime_time->setText(QTStr("breaktime.broadtitle.time").arg(m).arg(s));

    TitleChangePage(0);
}

void AFDockTitle::qslotBreaktimeFinished() 
{    
    ui->label_breaktime_title->setText(QTStr("breaktime.broadtitle.title"));
    ui->label_breaktime_time->setText(QTStr("breaktime.broadtitle.time").arg(0).arg(0));
    TitleChangePage(1);
}

void AFDockTitle::qslotUpdateWindowTitle(QString title)
{
    if (m_blockType <= -1)
        UpdateTitleLabel();
    else
        ui->label->setText(windowTitle());
}