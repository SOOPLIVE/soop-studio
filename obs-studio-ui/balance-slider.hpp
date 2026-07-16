#pragma once

#include <QSlider>
#include <QStyle>
#include <QMouseEvent>

class BalanceSlider : public QSlider {
	Q_OBJECT

public:
	inline BalanceSlider(QWidget *parent = 0) : QSlider(parent) {}

signals:
	void doubleClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            int newValue = QStyle::sliderValueFromPosition(
                minimum(),
                maximum(),
                event->position().toPoint().x(),
                width()
            );

            setValue(newValue);
            event->accept();
        }
        QSlider::mousePressEvent(event);
    }

	void mouseDoubleClickEvent(QMouseEvent *event)
	{
		emit doubleClicked();
		event->accept();
	}
};
