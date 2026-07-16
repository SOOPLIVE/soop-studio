#pragma once

#include <QLabel>
#include <QMouseEvent>

class ClickableLabel : public QLabel
{
	Q_OBJECT

public:
	inline ClickableLabel(QWidget *parent = 0) : QLabel(parent) {}

signals:
	void clicked();
    void mouseReleased();

protected:
	void mousePressEvent(QMouseEvent *event)
	{
		emit clicked();
		event->accept();
	}

	void mouseReleaseEvent(QMouseEvent* event) 
	{
		emit mouseReleased();
		event->accept();
	}
};