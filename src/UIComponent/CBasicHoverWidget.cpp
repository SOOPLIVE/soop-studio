
#include "CBasicHoverWidget.h"

#include <QStyleOption>
#include <QPainter>
#include <QHBoxLayout>
#include <QLabel>

#include <qevent.h>

AFQHoverWidget::AFQHoverWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    installEventFilter(this);
}

bool AFQHoverWidget::event(QEvent* e)
{
    QMouseEvent *mouseEvent;
    QPoint mousePos;
    QRect widgetRect;

    if (this->isEnabled()) {
        switch (e->type())
        {
        case QEvent::HoverEnter:
            emit qsignalHoverEnter();
            break;
        case QEvent::HoverLeave:
            emit qsignalHoverLeave();
            break;
        case QEvent::MouseButtonRelease:
            mouseEvent = dynamic_cast<QMouseEvent *>(e);
            if (mouseEvent) {
                mousePos = mouseEvent->pos();
                widgetRect = rect();
                if (!widgetRect.contains(mousePos)) {
                    return false;
                }
                if (mouseEvent->button() == Qt::LeftButton) {
                    emit qsignalMouseClick();
                }
                return true;
            }
            break;
        case QEvent::MouseButtonPress:
            return true;
        case QEvent::MouseButtonDblClick:
            // Without double click event, the back of the widget can be clicked
            return true;
        default:
            break;
        }
    }

    return QWidget::event(e);
}

void AFQHoverWidget::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

