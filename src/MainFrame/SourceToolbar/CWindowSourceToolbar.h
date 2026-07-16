#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQWindowSourceToolbar;
}

class AFQWindowSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQWindowSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQWindowSourceToolbar();

private slots:
	void _qslotWindowCurrentIndexChanged(int idx);

private:
	Ui::AFQWindowSourceToolbar* ui;
};