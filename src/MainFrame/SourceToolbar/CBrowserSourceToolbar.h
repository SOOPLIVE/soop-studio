#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQBrowserSourceToolbar;
}

class AFQBrowserSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT

public:
	explicit AFQBrowserSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQBrowserSourceToolbar();

	virtual void SetContextBarSize(int /*size*/);

private slots:
	void qslotRefreshButtonClicked();

private:
	Ui::AFQBrowserSourceToolbar* ui;
};