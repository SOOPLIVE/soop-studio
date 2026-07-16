#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQCommerceSourceToolbar;
}

class AFQCommerceSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQCommerceSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQCommerceSourceToolbar();

	void SetContextBarSize(int contextBarSize);

private slots:
	void _qslotBrowserRefreshClicked();
	void _qslotCommerceSettingClicked();

private:
	Ui::AFQCommerceSourceToolbar* ui;
};