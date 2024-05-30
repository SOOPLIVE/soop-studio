#pragma once

#include <QDialog>

#include "obs.hpp"
#include "UIComponent/CRoundedDialogBase.h"

static bool is_network_media_source(obs_source_t* source, const char* id)
{
	if (strcmp(id, "ffmpeg_source") != 0)
		return false;

	OBSDataAutoRelease s = obs_source_get_settings(source);
	bool is_local_file = obs_data_get_bool(s, "is_local_file");

	return !is_local_file;
}

namespace Ui {
	class AFQSourceControlDialog;
}

class AFQSourceControlDialog : public AFQRoundedDialogBase {
	Q_OBJECT

public:
	explicit AFQSourceControlDialog(QWidget* parent = nullptr);
	~AFQSourceControlDialog();

signals:
	void qsignalClearContext(OBSSource source);

private slots:
	void qslotCloseButtonClicked();

protected:
	virtual void closeEvent(QCloseEvent* event) override;

public:
	void SetOBSSource(OBSSource source, bool force = false);
	void SelectedContextSource(bool selected);
	void ClearContextPanel();

	OBSSource GetOBSSource();

private:
	Ui::AFQSourceControlDialog* ui;

	OBSSource m_obsSource;
};