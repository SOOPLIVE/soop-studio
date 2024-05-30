#pragma once

#include "CSliderIgnoreWheel.h"

class AFQMediaSlider : public AFQSliderIgnoreScroll {
	Q_OBJECT
public:
	AFQMediaSlider(QWidget* parent = nullptr);

signals:
	void mediaSliderPress(int value);
	void mediaSliderRelease();
	void mediaSliderHovered(int value);

protected:
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual bool eventFilter(QObject* obj, QEvent* event) override;
};