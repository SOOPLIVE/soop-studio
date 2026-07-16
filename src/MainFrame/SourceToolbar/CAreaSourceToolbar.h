#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQAreaSourceToolbar;
}

class AFQAreaSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQAreaSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQAreaSourceToolbar();

private slots:
	void qslotFindWindowButtonClicked();

private:
	Ui::AFQAreaSourceToolbar* ui;
};