#include "CCustomSpinbox.h"

#include <QWheelEvent>

AFQCustomSpinbox::AFQCustomSpinbox(QWidget *parent)
    : QSpinBox{parent}
{
	setFocusPolicy(Qt::StrongFocus);
}

void AFQCustomSpinbox::wheelEvent(QWheelEvent* event)
{
	if (!hasFocus())
		event->ignore();
	else
		QSpinBox::wheelEvent(event);
}
