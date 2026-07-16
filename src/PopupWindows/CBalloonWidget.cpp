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
    
    m_isTooltip = true;
    
    if (m_pTooltiplabel == nullptr)
    {
        m_pTooltiplabel = new QLabel(this);
        m_pTooltiplabel->setAlignment(Qt::AlignCenter);
        m_pTooltiplabel->setText(tooltip);
        m_pTooltiplabel->adjustSize();
    }

    QSize tooltipsize = m_pTooltiplabel->size();
    ui->widget_Contents->layout()->addWidget(m_pTooltiplabel);

    QMargins contentsMargin = ui->widget_Contents->layout()->contentsMargins();
    int w = contentsMargin.left() + contentsMargin.right()
        + m_pTooltiplabel->width();
    int h = contentsMargin.top() + contentsMargin.bottom()
        + m_pTooltiplabel->height() + ui->label_balloonPoint->height();
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
    if (m_isTooltip && m_pTooltiplabel != nullptr)
    {
        m_pTooltiplabel->setText(text);
        m_pTooltiplabel->adjustSize();

        QMargins contentsMargin = ui->widget_Contents->layout()->contentsMargins();
        int w = m_pTooltiplabel->width() + contentsMargin.left() + contentsMargin.right();
        int h = m_pTooltiplabel->height() + contentsMargin.top() + contentsMargin.bottom() + 10;
        resize(w, h);
        m_pTooltiplabel->repaint();

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
    if (!m_isTooltip)
    {
        ui->widget_Contents->layout()->addWidget(widget);

        QMargins contentsMargin = ui->widget_Contents->layout()->contentsMargins();
        int w = widget->width() + contentsMargin.left() + contentsMargin.right();
        int h = widget->height() + ui->widget_Contents->layout()->spacing();
        h += height();

        resize(w, h);
        _AdjustBalloonGeometry();
    }
    return m_isTooltip ? false : true;
}

bool AFQBalloonWidget::AddTextToBalloon(QString text)
{
    if (!m_isTooltip)
    {
        QLabel* label = new QLabel(this);
        label->setText(text);
        label->adjustSize();

        QSize tooltipsize = label->size();
        resize(width() + label->width(), height() + label->height());
        _AdjustBalloonGeometry();
    }
    return m_isTooltip ? false : true;
}

void AFQBalloonWidget::_AdjustBalloonGeometry()
{
    ui->widget_Border->move(0, 0);
    ui->widget_Border->resize(width(), height() - ui->label_balloonPoint->height());
    ui->label_balloonPoint->move(width() / 2 - (ui->label_balloonPoint->width() / 2), ui->widget_Border->height() - 1);
}
