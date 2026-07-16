#include "CTextSourceToolbar.h"
#include "ui_text-source-toolbar.h"

#include <QPushButton>
#include <QLineEdit>

#include "qt-wrappers.hpp"

#include "MainFrame/CMainFrame.h"
#include "PopupWindows/CCustomFontDialog.h"
#include "PopupWindows/CCustomColorDialog.h"

extern void MakeQFont(obs_data_t* font_obj, QFont& font, bool limit = false);

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

AFQTextSourceToolbar::AFQTextSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source),
	ui(new Ui::AFQTextSourceToolbar)
{
	ui->setupUi(this);

	connect(ui->pushButton_Font, &QPushButton::clicked,
			this, &AFQTextSourceToolbar::_qslotFontButtonClicked);

	connect(ui->pushButton_Color, &QPushButton::clicked,
		this, &AFQTextSourceToolbar::_qslotColorButtonClicked);

	connect(ui->lineEdit_Text, &QLineEdit::textChanged,
		this, &AFQTextSourceToolbar::_qslotTextEditChanged);

	//
	OBSDataAutoRelease settings = obs_source_get_settings(source);

	const char* id = obs_source_get_unversioned_id(source);
	bool ft2 = strcmp(id, "text_ft2_source") == 0;
	bool read_from_file = obs_data_get_bool(
		settings, ft2 ? "from_file" : "read_from_file");

	OBSDataAutoRelease font_obj = obs_data_get_obj(settings, "font");
	MakeQFont(font_obj, m_font);

	// Use "color1" if it's a freetype source and "color" elsewise
	unsigned int val = (unsigned int)obs_data_get_int(
		settings,
		(strncmp(obs_source_get_id(source), "text_ft2_source", 15) == 0)
		? "color1"
		: "color");

	m_color = color_from_int(val);

	const char* text = obs_data_get_string(settings, "text");

	bool single_line = !read_from_file &&
		(!text || (strchr(text, '\n') == nullptr));
	ui->label_Text->setVisible(single_line);
	ui->lineEdit_Text->setVisible(single_line);
	ui->lineEdit_Text->setPlaceholderText(QTStr("Text.Toolbar.Placeholder"));
	ui->emptyFrame->setVisible(!single_line);
	if (single_line)
		ui->lineEdit_Text->setText(text);
}

AFQTextSourceToolbar::~AFQTextSourceToolbar()
{

}

void AFQTextSourceToolbar::_qslotFontButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	QFontDialog::FontDialogOptions options;
	uint32_t flags;
	bool success;

#ifndef _WIN32
	options = QFontDialog::DontUseNativeDialog;
#endif
	m_font = AFQCustomFontDialog::getFont(
		&success, m_font, MAINFRAME,
		QT_UTF8(Str("Basic.PropertiesWindow.SelectFont.WindowTitle")),
		options);
	if (!success) {
		return;
	}

	OBSDataAutoRelease font_obj = obs_data_create();

	obs_data_set_string(font_obj, "face", QT_TO_UTF8(m_font.family()));
	obs_data_set_string(font_obj, "style", QT_TO_UTF8(m_font.styleName()));
	obs_data_set_int(font_obj, "size", m_font.pointSize());
	flags = m_font.bold() ? OBS_FONT_BOLD : 0;
	flags |= m_font.italic() ? OBS_FONT_ITALIC : 0;
	flags |= m_font.underline() ? OBS_FONT_UNDERLINE : 0;
	flags |= m_font.strikeOut() ? OBS_FONT_STRIKEOUT : 0;
	obs_data_set_int(font_obj, "flags", flags);

	//SaveOldProperties(source);

	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_obj(settings, "font", font_obj);

	obs_source_update(source, settings);

	//SetUndoProperties(source);

}

void AFQTextSourceToolbar::_qslotColorButtonClicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	bool freetype =
		strncmp(obs_source_get_id(source), "text_ft2_source", 15) == 0;

	obs_property_t* p =
		obs_properties_get(props.get(), freetype ? "color1" : "color");

	const char* desc = obs_property_description(p);

	QColorDialog::ColorDialogOptions options;

	options |= QColorDialog::ShowAlphaChannel;
#ifdef __linux__
	// TODO: Revisit hang on Ubuntu with native dialog
	options |= QColorDialog::DontUseNativeDialog;
#endif

	QColor newColor = AFQCustomColorDialog::getColor(m_color, MAINFRAME, desc, options);
	if (!newColor.isValid()) {
		return;
	}

	m_color = newColor;

	SaveOldProperties(source);
	OBSDataAutoRelease settings = obs_data_create();
	if (freetype) {
		obs_data_set_int(settings, "color1", color_to_int(m_color));
		obs_data_set_int(settings, "color2", color_to_int(m_color));
	}
	else {
		obs_data_set_int(settings, "color", color_to_int(m_color));
	}
	obs_source_update(source, settings);
	SetUndoProperties(source);
}

void AFQTextSourceToolbar::_qslotTextEditChanged()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	std::string newText = QT_TO_UTF8(ui->lineEdit_Text->text());
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	if (newText == obs_data_get_string(settings, "text")) {
		return;
	}
	SaveOldProperties(source);
	obs_data_set_string(settings, "text", newText.c_str());
	obs_source_update(source, nullptr);
	SetUndoProperties(source, true);
}