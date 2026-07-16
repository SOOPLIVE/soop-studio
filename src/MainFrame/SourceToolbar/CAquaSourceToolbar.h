#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQAquaSourceToolbar;
}

class AFQAquaSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQAquaSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQAquaSourceToolbar();

	void SetContextBarSize(int contextBarSize);

private slots:
	void _qslotRefreshButtonClicked();
	void _qslotCurrentIndexChanged(int idx);

private:
	Ui::AFQAquaSourceToolbar* ui;
};