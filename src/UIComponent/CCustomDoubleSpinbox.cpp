#include "CCustomDoubleSpinbox.h"

#include <QWheelEvent>

AFQCustomDoubleSpinbox::AFQCustomDoubleSpinbox(QWidget *parent)
    : QDoubleSpinBox{parent}
{
	setFocusPolicy(Qt::StrongFocus);
}

void AFQCustomDoubleSpinbox::wheelEvent(QWheelEvent* event)
{
	if (!hasFocus())
		event->ignore();
	else
		QDoubleSpinBox::wheelEvent(event);
}
