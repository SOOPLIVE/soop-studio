#pragma once

// {obs-studio path}\UI\slider-ignorewheel.hpp

#include "obs.hpp"
#include <QSlider>
#include <QInputEvent>
#include "UIComponent/CMouseClickSlider.h"

class AFQSliderIgnoreScroll : public QSlider {
	Q_OBJECT
public:
	AFQSliderIgnoreScroll(QWidget* parent = nullptr);
	AFQSliderIgnoreScroll(Qt::Orientation orientation,
						  QWidget* parent = nullptr);

protected:
	virtual void wheelEvent(QWheelEvent* event) override;
};

//class AFCVolumeSlider : public AFQSliderIgnoreScroll {
class AFCVolumeSlider : public AFQMouseClickSlider {
	Q_OBJECT

public:
	obs_fader_t* fad;

	//AFCVolumeSlider(obs_fader_t* fader, QWidget* parent = nullptr);
	AFCVolumeSlider(obs_fader_t* fader, Qt::Orientation orientation,
					QWidget* parent = nullptr);
};