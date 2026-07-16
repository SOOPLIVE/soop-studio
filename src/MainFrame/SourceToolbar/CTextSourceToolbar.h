#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQTextSourceToolbar;
}

class AFQTextSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQTextSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQTextSourceToolbar();

private slots:
	void _qslotFontButtonClicked();
	void _qslotColorButtonClicked();
	void _qslotTextEditChanged();

private:
	Ui::AFQTextSourceToolbar* ui;

	QFont  m_font;
	QColor m_color;

};