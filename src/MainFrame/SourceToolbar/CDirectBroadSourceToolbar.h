#pragma once

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQDirectBroadSourceToolbar;
}

class AFQDirectBroadSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQDirectBroadSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQDirectBroadSourceToolbar();

	void SetContextBarSize(int contextBarSize);

private slots:
	void _qslotRefreshButtonClicked();

private:
	Ui::AFQDirectBroadSourceToolbar* ui;
};