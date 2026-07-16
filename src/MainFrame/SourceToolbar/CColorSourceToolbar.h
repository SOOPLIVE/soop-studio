#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQColorSourceToolbar;
}

class AFQColorSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT

public:
	explicit AFQColorSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQColorSourceToolbar();

private slots:
	void _qslotColorPickButtonClicked();

private:
	void _UpdateColor();

private:
	Ui::AFQColorSourceToolbar* ui;

	QColor m_color;
};