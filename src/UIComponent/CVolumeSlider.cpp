#include "CVolumeSlider.h"
#include "qt-wrappers.hpp"


AFCVolumeSlider::AFCVolumeSlider(QWidget* parent, obs_fader_t* fader, Qt::Orientation orientation)
	: AFQMouseClickSlider(parent)
{
	fad = fader;
	setFocusPolicy(Qt::StrongFocus);
	setOrientation(orientation);
	setSliderHoverProperty(false);
}

bool AFCVolumeSlider::event(QEvent* e)
{
	switch (e->type())
	{
	case QEvent::HoverLeave:
		setSliderHoverProperty(false);
		break;
	case QEvent::HoverEnter:
		setSliderHoverProperty(true);
		break;
	default:
		break;
	}
	return QWidget::event(e);
}

#define HOVER_STATE_PROPERTY "hover"
void AFCVolumeSlider::setSliderHoverProperty(bool hover)
{
	setProperty(HOVER_STATE_PROPERTY, hover);
	PolishStyleSheet(this);
}