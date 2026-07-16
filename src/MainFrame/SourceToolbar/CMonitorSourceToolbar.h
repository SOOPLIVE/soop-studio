#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQMonitorSourceToolbar;
}

class AFQMonitorSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQMonitorSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQMonitorSourceToolbar();

private slots:
	void _qslotMonitorCurrentIndexChanged(int idx);

private:
	Ui::AFQMonitorSourceToolbar* ui;
};