#pragma once

#include <QFrame>
#include <QTimer>

#include "obs.hpp"

#include "CBaseSourceToolbar.h"

namespace Ui {
	class AFQImageSourceToolbar;
}

class AFQImageSourceToolbar : public CBaseSourceToolbar
{
	Q_OBJECT
public:
	explicit AFQImageSourceToolbar(QWidget* parent = nullptr, OBSSource source = nullptr);
	~AFQImageSourceToolbar();

private:
	static void FSMediaFileLoaded(void* data, calldata_t* params);

private slots:
	void _qslotBrowseImagePathClicked();

private:
	Ui::AFQImageSourceToolbar* ui;

	OBSSignal m_signalFileLoad;
};