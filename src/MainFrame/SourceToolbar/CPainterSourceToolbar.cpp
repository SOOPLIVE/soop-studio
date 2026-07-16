#include "CPainterSourceToolbar.h"
#include "ui_painter-source-toolbar.h"

#include <QPushButton>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "MainFrame/CMainFrame.h"

#include "slider-ignorewheel.hpp"
#include "PopupWindows/CCustomColorDialog.h"

AFQPainterSourceToolbar::AFQPainterSourceToolbar(QWidget* parent, OBSSource source) :
	CBaseSourceToolbar(parent, source),
	ui(new Ui::AFQPainterSourceToolbar)
{
	ui->setupUi(this);

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	int r = obs_data_get_int(settings, "line_color_r");
	int g = obs_data_get_int(settings, "line_color_g");
	int b = obs_data_get_int(settings, "line_color_b");

	setToolColorButton(r, g, b);

	auto* toolGroup = new QButtonGroup(this);
	toolGroup->setExclusive(true);

	QStringList toolNames = { "buttonPencilTool", "buttonEraserTool", "buttonRectangleTool", "buttonCircleTool" };
	for (int i = 0; i < toolNames.size(); i++) {
		QPushButton* button = findChild<QPushButton*>(toolNames.at(i));

		toolGroup->addButton(button, i);
	}

	int toolType = (int)obs_data_get_int(settings, "tool");
	if (auto* b = toolGroup->button(toolType))
		b->setChecked(true);

	connect(toolGroup, &QButtonGroup::idClicked, this, &AFQPainterSourceToolbar::qslotToolClicked);

	connect(ui->buttonToolThickness, &QPushButton::toggled, this, &AFQPainterSourceToolbar::qslotShowThicknessControl);
	connect(ui->buttonToolColor, &QPushButton::toggled, this, &AFQPainterSourceToolbar::qslotShowToolColorControl);

	connect(ui->buttonUndo, &QPushButton::clicked, this, &AFQPainterSourceToolbar::qslotUndoDraw);
	connect(ui->buttonRedo, &QPushButton::clicked, this, &AFQPainterSourceToolbar::qslotRedoDraw);
	connect(ui->buttonReset, &QPushButton::clicked, this, &AFQPainterSourceToolbar::qslotResetPainter);
}

AFQPainterSourceToolbar::~AFQPainterSourceToolbar()
{
	delete ui;
}

void AFQPainterSourceToolbar::qslotToolClicked(int id)
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	int toolType = id;

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_set_int(settings, "tool", toolType);

	obs_property_t* p = obs_properties_get(props.get(), "update_tool");
	obs_property_button_clicked(p, source);
}

void AFQPainterSourceToolbar::qslotUndoDraw()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "undo_painter");
	obs_property_button_clicked(p, source);
}

void AFQPainterSourceToolbar::qslotRedoDraw()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "redo_painter");
	obs_property_button_clicked(p, source);
}

void AFQPainterSourceToolbar::qslotResetPainter()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t* p = obs_properties_get(props.get(), "refreshnocache");
	obs_property_button_clicked(p, source);
}

void AFQPainterSourceToolbar::qslotShowThicknessControl(int checked)
{
	if (!checked) {
		if (thicknessPopup) 
			thicknessPopup->close();
		return;
	}

	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	int thickness = obs_data_get_int(settings, "thickness");

	auto* popup = new CMenuStyleFrame(this);
	thicknessPopup = popup;
	popup->setFixedSize(160, 40);
	popup->setMargins(16, 8, 16, 8);

	connect(popup, &QObject::destroyed, this, [this]() {
		thicknessPopup = nullptr;
		ui->buttonToolThickness->setChecked(false);
		});

	auto* slider = new SliderIgnoreScroll(Qt::Horizontal, popup);
	slider->setFocusPolicy(Qt::StrongFocus);
	slider->setRange(1, 10);
	slider->setValue(thickness);
	popup->addWidget(slider);


	connect(slider, &QSlider::valueChanged, popup, [source, this](int value) {
		OBSDataAutoRelease s = obs_source_get_settings(source);
		obs_data_set_int(s, "thickness", value);
		obs_property_t* p = obs_properties_get(props.get(), "update_thickness");
		if (p) obs_property_button_clicked(p, source);
		});

	popup->move(popupPosUnderCenter(ui->buttonToolThickness, popup));
	popup->show();
	popup->setFocus();
	slider->setFocus();
}

void AFQPainterSourceToolbar::qslotShowToolColorControl(int checked)
{
	if (!checked) {
		if (colorPopup)
			colorPopup->close();
		return;
	}

	OBSSource source = GetSource();
	if (!source)
		return;

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	const int curR = (int)obs_data_get_int(settings, "line_color_r");
	const int curG = (int)obs_data_get_int(settings, "line_color_g");
	const int curB = (int)obs_data_get_int(settings, "line_color_b");

	const QVector<QColor> palette = {
		{255, 36, 36}, {249, 187, 0}, {0, 193, 17}, {18, 132, 239},
		{194, 33, 247}, {0, 0, 0}, {255, 255, 255}, {-1, -1, -1}	// final index custom color
	};

	auto* popup = new CMenuStyleFrame(this);
	colorPopup = popup;
	popup->setFixedSize(248, 40);
	popup->setMargins(4, 4, 4, 4);

	connect(popup, &QObject::destroyed, this, [this]() {
		colorPopup = nullptr;
		ui->buttonToolColor->setChecked(false);
		});

	auto* colorGroup = new QButtonGroup(popup);
	colorGroup->setExclusive(true);

	int checkedId = -1;
	for (int i = 0; i < palette.size(); ++i) {
		if (palette[i].red() == curR && palette[i].green() == curG && palette[i].blue() == curB) {
			checkedId = i;
			break;
		}
	}

	const int btnSize = 30;
	for (int i = 0; i < palette.size() - 1; ++i) {
		const auto c = palette[i];

		auto* btn = new QPushButton(popup);
		btn->setCheckable(true);
		btn->setFixedSize(btnSize, btnSize);
		btn->setProperty("toolbarType", "painter-source");

		QIcon icon = getToolColorIcon(c.red(), c.green(), c.blue(), btnSize, 14);
		btn->setIcon(icon);
		btn->setIconSize(QSize(btnSize, btnSize));

		colorGroup->addButton(btn, i);
		popup->addWidget(btn);

		if (i == checkedId)
			btn->setChecked(true);
	}

	auto* btnSelectColor = new QPushButton(popup);
	btnSelectColor->setCheckable(true);
	btnSelectColor->setFixedSize(btnSize, btnSize);
	btnSelectColor->setProperty("toolbarType", "painter-source");

	std::string absPath;
	GetDataFilePath("assets", absPath);
	QString iconPath = QString("%1/source-toolbar/painter-source/bt_select_color.png")
		.arg(absPath.data());

	QIcon icon = QIcon(iconPath);
	btnSelectColor->setIconSize(QSize(30, 30));
	btnSelectColor->setIcon(icon);
	colorGroup->addButton(btnSelectColor, palette.size());
	popup->addWidget(btnSelectColor);

	if (-1 == checkedId) {
		btnSelectColor->setChecked(true);
	}

	connect(colorGroup, &QButtonGroup::idClicked, popup, [=](int id) {
		QColor color;
		OBSDataAutoRelease s = obs_source_get_settings(source);
		if (id == palette.size()) {

			int r = obs_data_get_int(s, "line_color_r");
			int g = obs_data_get_int(s, "line_color_g");
			int b = obs_data_get_int(s, "line_color_b");
			color.setRgb(r, g, b);

			QColorDialog::ColorDialogOptions options;
			QColor chosenColor = AFQCustomColorDialog::getColor(color, MAINFRAME, Str("CustomColorDialog.title"), options);

			if (!chosenColor.isValid()) {
				return;
			}
			color = chosenColor;
		}
		else {
			color = palette[id];
		}

		obs_data_set_int(s, "line_color_r", color.red());
		obs_data_set_int(s, "line_color_g", color.green());
		obs_data_set_int(s, "line_color_b", color.blue());

		obs_property_t* p = obs_properties_get(props.get(), "update_linecolor");
		if (p)
			obs_property_button_clicked(p, source);

		setToolColorButton(color.red(), color.green(), color.blue());

		});

	popup->move(popupPosUnderCenter(ui->buttonToolColor, popup));
	popup->show();
	popup->setFocus();

}

QIcon AFQPainterSourceToolbar::getToolColorIcon(int r, int g, int b, int buttonSize, int circleSize)
{
	const int iconW = buttonSize;
	const int iconH = buttonSize;
	const int circleD = circleSize;
	const qreal border = 1.0;

	QSize sz(iconW, iconH);
	QPixmap pm(sz);
	pm.fill(Qt::transparent);

	QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing, true);

	QPen pen(QColor(255, 255, 255, 255));
	pen.setWidthF(border);
	pen.setJoinStyle(Qt::RoundJoin);
	p.setPen(pen);

	p.setBrush(QColor(r, g, b, 255));

	const qreal inset = border / 2.0;
	const qreal x = (iconW - circleD) / 2.0 + inset;
	const qreal y = (iconH - circleD) / 2.0 + inset;
	const qreal w = circleD - border;
	const qreal h = circleD - border;

	p.drawEllipse(QRectF(x, y, w, h));
	p.end();

	return QIcon(pm);
}

void AFQPainterSourceToolbar::setToolColorButton(int r, int g, int b)
{
	QIcon icon = getToolColorIcon(r, g, b, 30, 14);
	ui->buttonToolColor->setIcon(icon);
	ui->buttonToolColor->setIconSize(QSize(30,30));
}

QPoint AFQPainterSourceToolbar::popupPosUnderCenter(QWidget* widget, QWidget* popup)
{
	const QRect br = widget->rect();
	const QPoint bottomCenter = widget->mapToGlobal(QPoint(br.center().x(), br.height()));
	QPoint pos(bottomCenter.x() - popup->width() / 2, bottomCenter.y());

	if (QScreen* sc = widget->screen()) {
		const QRect avail = sc->availableGeometry();
		pos.setX(qBound(avail.left(), pos.x(), avail.right() - popup->width()));
		pos.setY(qBound(avail.top(), pos.y(), avail.bottom() - popup->height()));
	}
	return pos;
}