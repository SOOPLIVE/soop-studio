#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQAIManagerToolbar;
}

class AFQAIManagerToolbar : public CBaseSourceToolbar
{
	Q_OBJECT

public:
	explicit AFQAIManagerToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQAIManagerToolbar();

	virtual void SetContextBarSize(int /*size*/);

private slots:
	void qslotRefreshButtonClicked();
	void qslotRemoveSource();

private:
	Ui::AFQAIManagerToolbar* ui;
};