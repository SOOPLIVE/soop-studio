#include "CSpoutSourceToolbar.h"
#include "ui_spout2-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

AFQSpoutSourceToolbar::AFQSpoutSourceToolbar(QWidget* parent, OBSSource source_) :
	CBaseSourceToolbar(parent, source_),
	ui(new Ui::AFQSpoutSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->comboBox_Sender, &QComboBox::currentIndexChanged,
		this, &AFQSpoutSourceToolbar::_qslotWindowCurrentIndexChanged);

	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	UpdateSourceComboToolbarProperties(ui->comboBox_Sender, source, props.get(), "spout2_sender", false);
}

AFQSpoutSourceToolbar::~AFQSpoutSourceToolbar()
{
	delete ui;
}

void AFQSpoutSourceToolbar::_qslotWindowCurrentIndexChanged(int idx)
{
	OBSSource source = GetSource();
	if (idx == -1 || !source) {
		return;
	}

	SaveOldProperties(source);
	UpdateSourceComboToolbarValue(ui->comboBox_Sender, source, idx, "spout2_sender", false);
	SetUndoProperties(source);
}