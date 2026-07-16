#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQAquaScoreSourceToolbar;
}

class AFQAquaScoreSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQAquaScoreSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQAquaScoreSourceToolbar();

private slots:
	void _qslotTimerStartButtonClicked();
	void _qslotTimerPauseButtonClicked();
	void _qslotTimerResetButtonClicked();
	void _qslotLeftScoreValueChanged(int value);
	void _qslotRightScoreValueChanged(int value);
	void _qslotLeftSetValueChanged(int value);
	void _qslotRightSetValueChanged(int value);

private:
	Ui::AFQAquaScoreSourceToolbar* ui;
};