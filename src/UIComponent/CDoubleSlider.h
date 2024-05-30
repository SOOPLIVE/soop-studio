// [copy-obs]


#pragma once

#include <QSlider>
#include "CSliderIgnoreWheel.h"

class AFQDoubleSlider : public AFQSliderIgnoreScroll {
	Q_OBJECT

	double minVal, maxVal, minStep;

public:
	AFQDoubleSlider(QWidget *parent = nullptr);

	void setDoubleConstraints(double newMin, double newMax, double newStep,
				  double val);

signals:
	void doubleValChanged(double val);

public slots:
	void setDoubleVal(double val);
};
