#include <QResizeEvent>
#include <QScrollBar>
#include "moc_horizontal-scroll-area.cpp"

void HScrollArea::resizeEvent(QResizeEvent *event)
{
	if (!!widget())
		widget()->setMaximumHeight(event->size().height());

	QScrollArea::resizeEvent(event);
}

void HScrollArea::wheelEvent(QWheelEvent* event)
{
	int delta = event->angleDelta().y() / 2;
	horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta);
	event->accept();
}
