// [copy-obs]



#include "CDoubleSlider.h"

#include <cmath>

AFQDoubleSlider::AFQDoubleSlider(QWidget *parent) : AFQSliderIgnoreScroll(parent)
{
	connect(this, &AFQDoubleSlider::valueChanged, [this](int val) {
		emit doubleValChanged((minVal / minStep + val) * minStep);
	});
}

void AFQDoubleSlider::setDoubleConstraints(double newMin, double newMax,
					double newStep, double val)
{
	minVal = newMin;
	maxVal = newMax;
	minStep = newStep;

	double total = maxVal - minVal;
	int intMax = int(total / minStep);

	setMinimum(0);
	setMaximum(intMax);
	setSingleStep(1);
	setDoubleVal(val);
}

void AFQDoubleSlider::setDoubleVal(double val)
{
	setValue(lround((val - minVal) / minStep));
}
