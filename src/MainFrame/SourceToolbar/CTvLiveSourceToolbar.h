#pragma once

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQTvLiveSourceToolbar;
}

class AFQTvLiveSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQTvLiveSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQTvLiveSourceToolbar();

	void SetContextBarSize(int contextBarSize);

private slots:
	void _qslotRefreshButtonClicked();

private:
	Ui::AFQTvLiveSourceToolbar* ui;
};