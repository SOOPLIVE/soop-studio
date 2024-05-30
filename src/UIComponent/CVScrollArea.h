#pragma once

#include <QScrollArea>
#include <QResizeEvent>

class VScrollArea : public QScrollArea {
	Q_OBJECT

public:
	inline VScrollArea(QWidget* parent = nullptr) : QScrollArea(parent)
	{
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}

protected:
	virtual void resizeEvent(QResizeEvent* event) override {
		if (!!widget())
			widget()->setMaximumWidth(event->size().width());

		QScrollArea::resizeEvent(event);
	}
};