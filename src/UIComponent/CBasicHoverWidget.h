#pragma once

#include <QWidget>
#include <QEvent>

class AFQHoverWidget : public QWidget
{
	Q_OBJECT

public:
	explicit AFQHoverWidget(QWidget* parent = nullptr);
	~AFQHoverWidget() {};

signals:
	void qsignalHoverEnter();
	void qsignalHoverLeave();
	void qsignalMouseClick();
	void qsignalMousePressed();
	void qsignalMouseReleased();

protected:
	bool event(QEvent* e) override;
	void paintEvent(QPaintEvent*) override;
};