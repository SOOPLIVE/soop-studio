#include "CSourceControlDialog.h"
#include "ui_source-control-dialog.h"

#include <QStyle>
#include <QCloseEvent>

#include "ControlPanel/CMediaControlPanel.h"
#include "ControlPanel/CSliderControlPanel.h"

AFQSourceControlDialog::AFQSourceControlDialog(QWidget* parent) :
	AFQRoundedDialogBase((QDialog*)parent),
	ui(new Ui::AFQSourceControlDialog)
{
	ui->setupUi(this);

	connect(ui->closeButton, &QPushButton::clicked,
			this, &AFQSourceControlDialog::qslotCloseButtonClicked);

}

AFQSourceControlDialog::~AFQSourceControlDialog()
{
	m_obsSource = nullptr;

	delete ui;
}

void AFQSourceControlDialog::qslotCloseButtonClicked()
{
	qsignalClearContext(m_obsSource);
}

void AFQSourceControlDialog::closeEvent(QCloseEvent* event)
{
	QDialog::closeEvent(event);
	if (!event->isAccepted())
		return;
}

void AFQSourceControlDialog::SetOBSSource(OBSSource source, bool force)
{
	// 
	bool updateNeeded = true;
	QLayoutItem* la = ui->emptyFrame->layout()->itemAt(0);
	if (la) {

		if (AFQMediaControlPanel* panel =
			dynamic_cast<AFQMediaControlPanel*>(la->widget()))
		{
			if (panel->GetSource() == source)
				updateNeeded = false;
		}
	}

	const char* id = obs_source_get_unversioned_id(source);
	uint32_t flags = obs_source_get_output_flags(source);

	if (updateNeeded || force)
	{
		const char* name = obs_source_get_name(source);
		ui->titleLabel->setText(name);

		ClearContextPanel();
		if (flags & OBS_SOURCE_CONTROLLABLE_MEDIA)
		{
			if (!is_network_media_source(source, id))
			{
				if (strcmp(id, "slideshow") == 0)
				{
					AFQSliderControlPanel* panel = new AFQSliderControlPanel(ui->emptyFrame);
					panel->SetSource(source);
					ui->emptyFrame->layout()->addWidget(panel);
				}
				else if (strcmp(id, "ffmpeg_source") == 0)
				{
					AFQMediaControlPanel* panel = new AFQMediaControlPanel(ui->emptyFrame);
					panel->SetSource(source);
					ui->emptyFrame->layout()->addWidget(panel);
				}

				m_obsSource = source;
			}
		}
	}
}


void AFQSourceControlDialog::SelectedContextSource(bool selected)
{
	this->setProperty("selectedContext", selected);

	style()->unpolish(this);
	style()->polish(this);
}

void AFQSourceControlDialog::ClearContextPanel()
{
	QLayoutItem* la = ui->emptyFrame->layout()->itemAt(0);
	if (la) {
		delete la->widget();
		ui->emptyFrame->layout()->removeItem(la);
	}
}


OBSSource AFQSourceControlDialog::GetOBSSource()
{
	return m_obsSource;
}