#include "CSliderIgnoreWheel.h"

AFQSliderIgnoreScroll::AFQSliderIgnoreScroll(QWidget* parent) : QSlider(parent)
{
	setFocusPolicy(Qt::StrongFocus);
}

AFQSliderIgnoreScroll::AFQSliderIgnoreScroll(Qt::Orientation orientation,
	QWidget* parent)
	: QSlider(parent)
{
	setFocusPolicy(Qt::StrongFocus);
	setOrientation(orientation);
}

void AFQSliderIgnoreScroll::wheelEvent(QWheelEvent* event)
{
	if (!hasFocus())
		event->ignore();
	else
		QSlider::wheelEvent(event);
}

//AFCVolumeSlider::AFCVolumeSlider(obs_fader_t* fader, QWidget* parent)
//	: AFQSliderIgnoreScroll(parent)
//{
//	fad = fader;
//}
//
//AFCVolumeSlider::AFCVolumeSlider(obs_fader_t* fader, Qt::Orientation orientation,
//								 QWidget* parent)
//	: AFQSliderIgnoreScroll(orientation, parent)
//{
//	fad = fader;
//}

AFCVolumeSlider::AFCVolumeSlider(obs_fader_t* fader, Qt::Orientation orientation,
								 QWidget* parent)
	: AFQMouseClickSlider(parent)
{
	fad = fader;
	setFocusPolicy(Qt::StrongFocus);
	setOrientation(orientation);
}