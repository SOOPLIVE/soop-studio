#include "CCustomPushbutton.h"

#include <QHoverEvent>

AFQCustomPushbutton::AFQCustomPushbutton(QWidget *parent)
    : QPushButton{parent}
{
	this->setMouseTracking(true);
	this->setAttribute(Qt::WA_Hover);
	this->installEventFilter(this);
}

void AFQCustomPushbutton::qslotDelayTimeout()
{
	emit qsignalMouseStop();
	m_iDelayTime->stop();
}

bool AFQCustomPushbutton::event(QEvent* event)
{
	switch (event->type())
	{
	case QEvent::HoverEnter:
		emit qsignalButtonEnter();
		return true;
	case QEvent::HoverMove:
		emit qsignalMouseMove();
		break;
	case QEvent::HoverLeave:
		emit qsignalButtonLeave();
		return true;
	case QEvent::MouseButtonDblClick:
		emit qsignalButtonDoubleClicked();
		return true;
	};
	return QPushButton::event(event);
}

void AFQCustomPushbutton::_ChangeOpacity()
{
}
