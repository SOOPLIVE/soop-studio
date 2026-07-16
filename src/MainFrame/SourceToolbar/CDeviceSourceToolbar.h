#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQDeviceSourceToolbar;
}

class AFQDeviceSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQDeviceSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQDeviceSourceToolbar();

signals:
	void qsignalSplitFilterActived();

private slots:
	void _qslotVideoConfigClicked();
	void _qslotVideoDeviceActivateClicked();
	void _qslotSplitFilterToggle(bool checked);
	void _qslotSplitFilterSettingsClicked();

public:
	void SetSplitToggleButtonState(bool checked);
	void RefreshSplitToogleButtonState();

private:
	void _InitSplitFilterUI();

private:
	Ui::AFQDeviceSourceToolbar* ui;

	bool m_active;
};