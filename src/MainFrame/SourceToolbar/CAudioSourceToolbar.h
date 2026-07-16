#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQAudioSourceToolbar;
}

class AFQAudioSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQAudioSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQAudioSourceToolbar();

private slots:
	void _qslotAudioListChanged(int idx);

private:
	Ui::AFQAudioSourceToolbar* ui;
};