#include "CColorSourceToolbar.h"
#include "ui_color-source-toolbar.h"

#include <QPushButton>
#include <QComboBox>

#include "qt-wrappers.hpp"

#include "MainFrame/CMainFrame.h"
#include "PopupWindows/CCustomColorDialog.h"

static inline QColor color_from_int(long long val)
{
	return QColor(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff,
		(val >> 24) & 0xff);
}

static inline long long color_to_int(QColor color)
{
	auto shift = [&](unsigned val, int shift) {
		return ((val & 0xff) << shift);
	};

	return shift(color.red(), 0) | shift(color.green(), 8) |
		shift(color.blue(), 16) | shift(color.alpha(), 24);
}

AFQColorSourceToolbar::AFQColorSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source),
	ui(new Ui::AFQColorSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->pushButton_ColorPick, &QPushButton::clicked,
		this, &AFQColorSourceToolbar::_qslotColorPickButtonClicked);

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	unsigned int val = (unsigned int)obs_data_get_int(settings, "color");

	m_color = color_from_int(val);
	_UpdateColor();
}

AFQColorSourceToolbar::~AFQColorSourceToolbar()
{
	delete ui;
}

void AFQColorSourceToolbar::_qslotColorPickButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "color");
	const char* desc = obs_property_description(p);

	QColorDialog::ColorDialogOptions options;

	options |= QColorDialog::ShowAlphaChannel;

	QColor newColor = AFQCustomColorDialog::getColor(m_color, MAINFRAME, desc, options);
	if (!newColor.isValid()) {
		return;
	}

	m_color = newColor;
	_UpdateColor();

	SaveOldProperties(source);
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "color", color_to_int(m_color));
	obs_source_update(source, settings);
	SetUndoProperties(source);

}

void AFQColorSourceToolbar::_UpdateColor()
{
	QPalette palette = QPalette(m_color);
	ui->label_Color->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	ui->label_Color->setText(m_color.name(QColor::HexRgb));
	ui->label_Color->setPalette(palette);
	ui->label_Color->setStyleSheet(
		QString("background-color :%1; color: %2; border-radius :4px; ")
		.arg(palette.color(QPalette::Window)
			.name(QColor::HexRgb))
		.arg(palette.color(QPalette::WindowText)
			.name(QColor::HexRgb)));
	ui->label_Color->setAutoFillBackground(true);
	ui->label_Color->setAlignment(Qt::AlignCenter);

	PolishStyleSheet(ui->label_Color);
}