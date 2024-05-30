#include "CFocusList.h"
#include <QDragMoveEvent>

AFQFocusList::AFQFocusList(QWidget* parent) : QListWidget(parent) 
{
	NoFocusListDelegate* delegate = new NoFocusListDelegate(this);
	setItemDelegate(delegate);
}

void AFQFocusList::focusInEvent(QFocusEvent* event)
{
	QListWidget::focusInEvent(event);

	emit GotFocus();
}

void AFQFocusList::dragMoveEvent(QDragMoveEvent* event)
{
	QPoint pos = event->position().toPoint();
	int itemRow = row(itemAt(pos));

	if ((itemRow == currentRow() + 1) ||
		(currentRow() == count() - 1 && itemRow == -1))
		event->ignore();
	else
		QListWidget::dragMoveEvent(event);
}
