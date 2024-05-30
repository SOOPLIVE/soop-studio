#include "CMediaSlider.h"
#include <QStyleFactory>

AFQMediaSlider::AFQMediaSlider(QWidget* parent) :
	AFQSliderIgnoreScroll(parent)
{
	installEventFilter(this);
	setMouseTracking(true);
}

void AFQMediaSlider::mousePressEvent(QMouseEvent* event)
{
	int val = minimum() +
		((maximum() - minimum()) * event->pos().x()) / width();

	if (val > maximum())
		val = maximum();
	else if (val < minimum())
		val = minimum();

	emit mediaSliderPress(val);
	QSlider::mousePressEvent(event);
}

void AFQMediaSlider::mouseReleaseEvent(QMouseEvent* event)
{
	emit mediaSliderRelease();
	QSlider::mouseReleaseEvent(event);
}

void AFQMediaSlider::mouseMoveEvent(QMouseEvent* event)
{
	int val = minimum() +
		((maximum() - minimum()) * event->pos().x()) / width();

	if (val > maximum())
		val = maximum();
	else if (val < minimum())
		val = minimum();

	emit mediaSliderHovered(val);
	event->accept();
	QSlider::mouseMoveEvent(event);
}

bool AFQMediaSlider::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent->key() == Qt::Key_Up ||
			keyEvent->key() == Qt::Key_Down) {
			return true;
		}
	}

	if (event->type() == QEvent::Wheel)
		return true;

	return QSlider::eventFilter(obj, event);
}
