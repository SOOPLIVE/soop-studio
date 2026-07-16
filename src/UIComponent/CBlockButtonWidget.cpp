#include "CBlockButtonWidget.h"
#include "ui_block-button-widget.h"

#include "platform/platform.hpp"

AFBlockButtonWidget::AFBlockButtonWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFBlockButtonWidget)
{
    ui->setupUi(this);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    installEventFilter(this);
}

AFBlockButtonWidget::~AFBlockButtonWidget()
{
    delete ui;
}

void AFBlockButtonWidget::qslotButtonPressedTriggered()
{
    //ui->pushButton->setIconSize(QSize(24,24));
    emit qsignalHideTooltip();
}

//void AFBlockButtonWidget::qslotButtonReleasedTriggered()
//{
//    ui->pushButton->setIconSize(ui->pushButton->rect().size());
//}

void AFBlockButtonWidget::AFBlockButtonWidgetInit(int type, QString key)
{
    //Button Image in QSS by Property
    ui->pushButton->setProperty("buttonType", key);

    ui->pushButton->setChecked(false);
    SetOnLabelVisible(false);
    connect(ui->pushButton, &QPushButton::pressed, this, &AFBlockButtonWidget::qslotButtonPressedTriggered);
    connect(ui->pushButton, &QPushButton::released, this, &AFBlockButtonWidget::qslotButtonPressedTriggered);
    connect(ui->pushButton, &QPushButton::clicked, this, &AFBlockButtonWidget::qsignalBlockButtonClicked);
    m_blockButtonType = type;
}

bool AFBlockButtonWidget::IsOnLabelVisible()
{
    return ui->label_Toggle->isVisible();
}

void AFBlockButtonWidget::SetOnLabelVisible(bool visible)
{
    ui->label_Toggle->setVisible(visible);
}

bool AFBlockButtonWidget::event(QEvent* e)
{
    switch (e->type())
    {
    case QEvent::HoverEnter:
        m_checkHover = true;
        emit qsignalShowTooltip(m_blockButtonType);
        break;
    case QEvent::HoverLeave:
        m_checkHover = false;
        emit qsignalHideTooltip();
        break;
    case QEvent::HoverMove:
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

