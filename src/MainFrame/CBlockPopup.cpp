#include "CBlockPopup.h"
#include "ui_block-popup.h"

#include <QStyleOption>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QScrollBar>

#include "include/qt-wrapper.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "MainFrame/CMustRaiseMainFrameEventF.h"
#include "DynamicCompose/CMainDynamicComposit.h"
#include "PopupWindows/CBalloonWidget.h"
#include "UIComponent/CCustomPushButton.h"
#include "UIComponent/CBlockButtonWidget.h"
#include "platform/platform.hpp"


AFQBlockPopup::AFQBlockPopup(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQBlockPopup)
{
    ui->setupUi(this); 
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    installEventFilter(this);

    this->setFixedWidth(272);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

#ifdef __APPLE__
    setWindowFlags(windowFlags() | Qt::Tool);
#endif
    
    setAttribute(Qt::WA_TranslucentBackground);

    
    m_pMainRaiseEventFilter = new AFQMustRaiseMainFrameEventFilter();
}

AFQBlockPopup::~AFQBlockPopup()
{
    delete m_pMainRaiseEventFilter;
    
    if (m_BalloonToolTip)
    {
        m_BalloonToolTip->close();
        delete  m_BalloonToolTip;
    }
    delete ui;
}

void AFQBlockPopup::qslotShowTooltip(int key)
{
    QWidget* HoveredButton = reinterpret_cast<QWidget*>(sender());
    QString TooltipText = "";

    AFLocaleTextManager& localeManager = AFLocaleTextManager::GetSingletonInstance();

    //LOCALE
    switch (key)
    {
    case ENUM_BLOCK_TYPE::SceneSource:
        TooltipText = QString::fromUtf8(localeManager.Str("Block.Tooltip.SceneSourceList"), -1);
        break;
    case ENUM_BLOCK_TYPE::Chat:
        TooltipText = QString::fromUtf8(localeManager.Str("Block.Tooltip.Chat"), -1);;
        break;
    case ENUM_BLOCK_TYPE::Dashboard:
        TooltipText = QString::fromUtf8(localeManager.Str("Block.Tooltip.Newsfeed"), -1);;
        break;
    case ENUM_BLOCK_TYPE::AudioMixer:
        TooltipText = QString::fromUtf8(localeManager.Str("Block.Tooltip.AudioMixer"), -1);;
        break;
        break;
    }
    
    m_BalloonToolTip->ChangeTooltipText(TooltipText);


    QPoint HoveredButtonPos = HoveredButton->pos();
    qDebug() << pos().x();
    qDebug() << HoveredButtonPos;

    int x = pos().x() + HoveredButtonPos.x() - (m_BalloonToolTip->width() / 2) + (HoveredButton->width() / 2) + 5;
    int y = pos().y() - m_BalloonToolTip->height() + 11;

    QPoint balloonPos = QPoint(x, y);
    m_BalloonToolTip->move(balloonPos);
    m_BalloonToolTip->show();
}

void AFQBlockPopup::qslotCloseTooltip()
{
    m_BalloonToolTip->ChangeTooltipText("");
    if (m_BalloonToolTip->isVisible())
    {
        m_BalloonToolTip->hide();
    }
}

void AFQBlockPopup::qslotBlockButtonClicked()
{
    m_BalloonToolTip->ChangeTooltipText("");
    if (m_BalloonToolTip->isVisible())
    {
        m_BalloonToolTip->hide();
    }

    AFBlockButtonWidget* blockbutton = reinterpret_cast<AFBlockButtonWidget*>(sender());
    emit qsignalBlockButtonTriggered(true, blockbutton->BlockButtonType());
}

bool AFQBlockPopup::BlockWindowInit(bool locked, AFMainDynamicComposit* mainWindowBasic)
{    
    m_iButtonCount = 0;
    _SetBlockButtons();

    m_BalloonToolTip = new AFQBalloonWidget();
    m_BalloonToolTip->BalloonWidgetInit();
    
    return true;
}

void AFQBlockPopup::BlockButtonToggled(bool show, int blockType)
{
    foreach(AFBlockButtonWidget * widget, m_ButtonWidgetList)
    {
        if (widget->BlockButtonType() == blockType)
        {
            widget->SetOnLabelVisible(show);
        }
    }
}

bool AFQBlockPopup::event(QEvent* e)
{
    switch (e->type())
    {
    case QEvent::Show:
    {
        if (m_bInstalledEventFilter == false)
        {
            m_bInstalledEventFilter = true;
            // Must Get windowHandle, This First Show
            this->windowHandle()->installEventFilter(m_pMainRaiseEventFilter);
        }
        break;
    }
    case QEvent::HoverEnter:
        emit qsignalMouseEntered();
        return true;
    case QEvent::HoverLeave:
        emit qsignalHidePopup();
        return true;
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void AFQBlockPopup::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);

    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void AFQBlockPopup::wheelEvent(QWheelEvent* e)
{
    QScrollBar* hScrollBar = ui->scrollArea->horizontalScrollBar();
    if (e->angleDelta().y() > 0) // up Wheel
    {
        hScrollBar->setValue(hScrollBar->value() + 8);
    }
    else if (e ->angleDelta().y() < 0) //down Wheel
    {
        hScrollBar->setValue(hScrollBar->value() - 8);
    }
}

void AFQBlockPopup::_SetBlockButtons()
{
    /*ui->pushButton_LockButton->setIcon(QIcon(":/image/resource/MainView/lockblock.png"));
    connect(ui->pushButton_LockButton, &QPushButton::clicked,
        this, &AFQBlockPopup::qsignalLockTriggered);*/

    std::string imgPath;
    
    _SetBlockButton("SceneSource");
    _SetBlockButton("Chat");
    _SetBlockButton("Dashboard");
    _SetBlockButton("AudioMixer");

    int buttonAreaWidth = 46 * m_iButtonCount + 16 * (m_iButtonCount + 1);
    ui->scrollArea->setFixedWidth(buttonAreaWidth);
    ui->scrollAreaWidgetContents->setFixedWidth(buttonAreaWidth);
}

void AFQBlockPopup::_SetBlockButton(QString buttonType)
{
    AFBlockButtonWidget* buttonWidget = new AFBlockButtonWidget(this);

    std::string imgPath;
    QString path = QString("assets/block-icon/popup/default/%1.svg").arg(buttonType);
    GetDataFilePath(path.toUtf8().constData(), imgPath);

    QMetaEnum BlockTypeEnum = QMetaEnum::fromType<AFMainDynamicComposit::BlockType>();
    int blockType = BlockTypeEnum.keyToValue(buttonType.toUtf8().constData());

    buttonWidget->AFBlockButtonWidgetInit(blockType, buttonType.toUtf8().constData());
    connect(buttonWidget, &AFBlockButtonWidget::qsignalBlockButtonClicked, this, &AFQBlockPopup::qslotBlockButtonClicked);
    connect(buttonWidget, &AFBlockButtonWidget::qsignalShowTooltip, this, &AFQBlockPopup::qslotShowTooltip);
    connect(buttonWidget, &AFBlockButtonWidget::qsignalHideTooltip, this, &AFQBlockPopup::qslotCloseTooltip);
    ui->widget_ButtonArea->layout()->addWidget(buttonWidget);
    m_ButtonWidgetList.append(buttonWidget);

    m_iButtonCount++;
}