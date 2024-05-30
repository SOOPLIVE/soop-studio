#pragma once


// ARGB
#define GREY_COLOR_BACKGROUND 0x111111
#define GREY_TONDOWN_COLOR_BACKGROUND 0x0A0A0A


#define MAX_SCALING_LEVEL 20
#define MAX_SCALING_AMOUNT 10.0f
#define ZOOM_SENSITIVITY pow(MAX_SCALING_AMOUNT, 1.0f / MAX_SCALING_LEVEL)


static inline QSize GetPixelSize(QWidget* widget)
{
	return widget->size() * widget->devicePixelRatioF();
}