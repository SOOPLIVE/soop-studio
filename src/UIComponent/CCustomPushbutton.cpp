#include "CCustomPushbutton.h"

#include <QHoverEvent>

AFQCustomPushbutton::AFQCustomPushbutton(QWidget *parent)
    : QPushButton{parent}
{
	this->setMouseTracking(true);
	this->setAttribute(Qt::WA_Hover);
	this->installEventFilter(this);
}

bool AFQCustomPushbutton::event(QEvent* event)
{
	switch (event->type())
	{
	case QEvent::HoverEnter:
		emit qsignalButtonEnter();
		break;
	case QEvent::MouseButtonPress:
		emit qsignalMousePressed();
		break;
	case QEvent::MouseButtonRelease:
		emit qsignalMouseReleased();
		break;
	case QEvent::HoverMove:
		emit qsignalMouseMove();
		break;
	case QEvent::HoverLeave:
		emit qsignalButtonLeave();
		break;
	case QEvent::MouseButtonDblClick:
		emit qsignalButtonDoubleClicked();
		break;
	};
	return QPushButton::event(event);
}

AFQHoverOnlyPushButton::AFQHoverOnlyPushButton(QWidget* parent) 
	: QPushButton(parent)
{
	this->installEventFilter(this);
}

bool AFQHoverOnlyPushButton::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::MouseButtonPress)
		return true;
	if (event->type() == QEvent::MouseButtonDblClick)
		return true;
	return QObject::eventFilter(obj, event);
}
