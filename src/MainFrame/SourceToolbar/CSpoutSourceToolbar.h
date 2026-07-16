#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQSpoutSourceToolbar;
}

class AFQSpoutSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQSpoutSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQSpoutSourceToolbar();

private slots:
	void _qslotWindowCurrentIndexChanged(int idx);

private:
	Ui::AFQSpoutSourceToolbar* ui;
};