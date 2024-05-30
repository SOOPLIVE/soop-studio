#pragma once

#include <QLabel>
#include <QMouseEvent>

class AFQBaseClickableLabel : public QLabel 
{
#pragma region QT Field, CTOR/DTOR
	Q_OBJECT

#pragma region class initializer, destructor
public:
	inline AFQBaseClickableLabel(QWidget *parent = 0) : QLabel(parent) {}
#pragma endregion class initializer, destructor

signals:
	void clicked();
#pragma endregion QT Field

#pragma region protected func
protected:
	void mousePressEvent(QMouseEvent *event)
	{
		emit clicked();
		event->accept();
	}
#pragma endregion protected func
};
