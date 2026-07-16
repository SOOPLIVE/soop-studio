#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQGameCaptureSourceToolbar;
}

class AFQGameCaptureSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQGameCaptureSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQGameCaptureSourceToolbar();

	void UpdateWindowVisibility();

private slots:
	void _qslotModeCurrentIndexChanged(int idx);
	void _qslotWindowCurrentIndexChanged(int idx);

	void _qslotFitScreenSourceClicked();

private:
	Ui::AFQGameCaptureSourceToolbar* ui;
};