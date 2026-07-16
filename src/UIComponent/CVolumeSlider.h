#pragma once

#include "obs.hpp"
#include <QSlider>
#include <QInputEvent>
#include "UIComponent/CMouseClickSlider.h"

class AFCVolumeSlider : public AFQMouseClickSlider {
	Q_OBJECT

public:
	obs_fader_t* fad;

	//AFCVolumeSlider(obs_fader_t* fader, QWidget* parent = nullptr);
	AFCVolumeSlider(QWidget* parent = nullptr, obs_fader_t* fader = nullptr, Qt::Orientation orientation = Qt::Orientation::Horizontal);

protected:
	bool event(QEvent* e) override;

private:
	void setSliderHoverProperty(bool hover);
};
