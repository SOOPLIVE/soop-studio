#include "CBaseClickWidget.h"

#include <QStyleOption>
#include <QPainter>

AFQBaseClickWidget::AFQBaseClickWidget(QWidget* parent)
:QWidget(parent) 
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    installEventFilter(this);

    this->setProperty("hover", false);
}

void AFQBaseClickWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_bCheckable)
        m_bChecked = !m_bChecked;

    if (event->button() == Qt::LeftButton)
    {
        emit qsignalLeftClicked(m_bChecked);
        emit qsignalLeftClick();
    }
    else if (event->button() == Qt::RightButton)
    {
        emit qsignalRightClicked(m_bChecked);
        emit qsignalRightClick();
    }

    QWidget::mousePressEvent(event);
}

void AFQBaseClickWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        emit qsignalLeftReleased();
    else if (event->button() == Qt::RightButton)
        emit qsignalRightReleased();
    
    QWidget::mouseReleaseEvent(event);
}

void AFQBaseClickWidget::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

bool AFQBaseClickWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::HoverEnter) {
        this->setProperty("hover", true);
        //style()->unpolish(this);
        //style()->polish(this);
        emit qsignalHoverEntered();
    }
    else if (event->type() == QEvent::HoverLeave) {
        this->setProperty("hover", false);
        //style()->unpolish(this);
        //style()->polish(this);
        emit qsignalHoverLeaved();
    }
    return QWidget::eventFilter(obj, event);
}
