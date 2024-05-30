#include "CBalloonWidget.h"
#include "ui_balloon-widget.h"

#include <QLabel>
#include <QStyleOption>
#include <QPainter>
#include <QTimer>

AFQBalloonWidget::AFQBalloonWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQBalloonWidget)
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    ui->setupUi(this);    
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
}


AFQBalloonWidget::~AFQBalloonWidget()
{
    delete ui;
}

void AFQBalloonWidget::BalloonWidgetInit(QString tooltip)
{
    setWindowFlags(windowFlags() | Qt::Tool);
    
    m_bIsTooltip = true;
    
    if (m_Tooltiplabel == nullptr)
    {
        m_Tooltiplabel = new QLabel(this);
        m_Tooltiplabel->setAlignment(Qt::AlignCenter);
        m_Tooltiplabel->setText(tooltip);
        m_Tooltiplabel->adjustSize();
    }

    QSize tooltipsize = m_Tooltiplabel->size();
    ui->widget_Contents->layout()->addWidget(m_Tooltiplabel);

    QMargins contentsMargin = ui->widget_Contents->layout()->contentsMargins();
    int w = contentsMargin.left() + contentsMargin.right()
        + m_Tooltiplabel->width();
    int h = contentsMargin.top() + contentsMargin.bottom()
        + m_Tooltiplabel->height() + ui->label_balloonPoint->height();
    resize(w, h);
    
    _AdjustBalloonGeometry();
}

void AFQBalloonWidget::BalloonWidgetInit(QMargins margin, int spacing)
{
    setWindowFlags(windowFlags() | Qt::Popup | Qt::NoDropShadowWindowHint);

    ui->widget_Contents->layout()->setContentsMargins(margin);
    ui->widget_Contents->layout()->setSpacing(spacing);
    resize(minimumSize() + QSize(margin.left() + margin.right(), margin.top() + margin.bottom()));

    _AdjustBalloonGeometry();
}

void AFQBalloonWidget::ChangeTooltipText(QString text)
{
    if (m_bIsTooltip && m_Tooltiplabel != nullptr)
    {
        m_Tooltiplabel->setText(text);
        m_Tooltiplabel->adjustSize();

        QMargins contentsMargin = ui->widget_Contents->layout()->contentsMargins();
        int w = m_Tooltiplabel->width() + contentsMargin.left() + contentsMargin.right();
        int h = m_Tooltiplabel->height() + contentsMargin.top() + contentsMargin.bottom() + 10;
        resize(w, h);
        m_Tooltiplabel->repaint();

        _AdjustBalloonGeometry();
    }
}

void AFQBalloonWidget::ShowBalloon(QPoint pos)
{
    move(pos);
    show();
    setFocus();
    activateWindow();
}

void AFQBalloonWidget::paintEvent(QPaintEvent* e)
{
    QWidget::paintEvent(e);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter paint(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &paint, this);

    uint32_t thisHeight = height();

}

bool AFQBalloonWidget::AddWidgetToBalloon(QWidget* widget)
{
    if (!m_bIsTooltip)
    {
        ui->widget_Contents->layout()->addWidget(widget);

        QMargins contentsMargin = ui->widget_Contents->layout()->contentsMargins();
        int w = widget->width() + contentsMargin.left() + contentsMargin.right();
        int h = height() + widget->height() + ui->widget_Contents->layout()->spacing();
        resize(w, h);
        _AdjustBalloonGeometry();
    }
    return m_bIsTooltip ? false : true;
}

bool AFQBalloonWidget::AddTextToBalloon(QString text)
{
    if (!m_bIsTooltip)
    {
        QLabel* label = new QLabel(this);
        label->setText(text);
        label->adjustSize();

        QSize tooltipsize = label->size();
        resize(width() + label->width(), height() + label->height());
        _AdjustBalloonGeometry();
    }
    return m_bIsTooltip ? false : true;
}

void AFQBalloonWidget::_AdjustBalloonGeometry()
{
    ui->widget_Border->move(0, 0);
    ui->widget_Border->resize(width(), height() - ui->label_balloonPoint->height());
    ui->label_balloonPoint->move(width() / 2 - (ui->label_balloonPoint->width() / 2), ui->widget_Border->height() - 1);
}