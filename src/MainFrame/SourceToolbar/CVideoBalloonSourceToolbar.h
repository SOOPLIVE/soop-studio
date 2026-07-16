#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQVideoBalloonSourceToolbar;
}

class AFQVideoBalloonSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQVideoBalloonSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQVideoBalloonSourceToolbar();

	void SetContextBarSize(int contextBarSize);

private slots:
	void _qslotShowVideoBalloonListsButtonClicked();
	void _qslotRefreshButtonClicked();

private:
	Ui::AFQVideoBalloonSourceToolbar* ui;
};