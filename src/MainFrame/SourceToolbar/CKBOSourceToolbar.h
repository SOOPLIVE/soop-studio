#pragma once

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQKBOSourceToolbar;
}

class AFQKBOSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQKBOSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQKBOSourceToolbar();

	void SetContextBarSize(int contextBarSize);

private slots:
	void _qslotRefreshButtonClicked();

private:
	Ui::AFQKBOSourceToolbar* ui;
};