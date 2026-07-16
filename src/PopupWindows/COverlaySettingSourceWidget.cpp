
#include "COverlaySettingSourceWidget.h"

#include <QPushButton>

#include <util/base.h>

#include "Utils/OverlayManager.h" 
#include "platform/platform.hpp" 
#include "MainFrame/CMainFrame.h"


OverlaySettingSourceWidget::OverlaySettingSourceWidget(QWidget* parent) : OverlaySourceWidget(parent)
{
	config_set_default_double(ACTIVECONFIG, "Overlay", "Setting.X", (double)-1);
	auto x = config_get_double(ACTIVECONFIG, "Overlay", "Setting.X");
	if (x < 0)
	{
		auto w = OVERLAY_MANAGER.TargetRect(this).width();
		x = (double)(w - fix_w) / (double)2; 
		x /= (double)w;
	}

	config_set_default_double(ACTIVECONFIG, "Overlay", "Setting.Y", (double)-1); 
	auto y = config_get_double(ACTIVECONFIG, "Overlay", "Setting.Y");
	if (y < 0)
	{
		y = (double)8;
		auto h = OVERLAY_MANAGER.TargetRect(this).height();
		y /= (double)h;
	}

	moveRelative(QPointF((qreal)x, (qreal)y));
	
	setFixedSize(fix_w, fix_h);

	setAttribute(Qt::WA_TranslucentBackground, true);
	setAttribute(Qt::WA_NoSystemBackground, true);
	setStyleSheet("background-color : rgba(0, 0, 0, 0.7); border-radius : 10px;");

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(8, 8, 8, 8);
	setLayout(layout);

	//

	QFrame* frame = new QFrame(this);
	frame->setStyleSheet("background-color : rgb(0, 0, 0); border-radius : 10px;");
	layout->addWidget(frame);

	QHBoxLayout* layoutFrame = new QHBoxLayout(frame);
	layoutFrame->setContentsMargins(10, 10, 10, 10);
	layoutFrame->setSpacing(6);
	frame->setLayout(layoutFrame);

	//

	QFrame* frameOpacity = new QFrame(frame);
	frameOpacity->setStyleSheet("background-color : rgba(255, 255, 255, 0.1); border-radius : 10px;");
	layoutFrame->addWidget(frameOpacity);

	QVBoxLayout* layoutOpacity = new QVBoxLayout(frameOpacity);
	layoutOpacity->setContentsMargins(10, 4, 10, 4);
	layoutOpacity->setSpacing(2);
	frameOpacity->setLayout(layoutOpacity);

	opacityLabel = new QLabel(frameOpacity);
	config_set_default_int(ACTIVECONFIG, "Overlay", "Setting.Opacity", (int64_t)30); // 30%
	auto opacity = (int)config_get_int(ACTIVECONFIG, "Overlay", "Setting.Opacity");
	opacityLabel->setText(QTStr("Popup.LiveOverlay.Opacity").arg(opacity));
	opacityLabel->setStyleSheet("background-color : transparent; font-size : 14px; color : #D5D7DC;");
	layoutOpacity->addWidget(opacityLabel);

	opacitySlider = new AFQMouseClickSlider(frameOpacity);
	opacitySlider->setStyleSheet("background-color : transparent;");
	opacitySlider->setOrientation(Qt::Orientation::Horizontal);
	opacitySlider->setRange(0, 100);
	opacitySlider->setValue((int)opacity);
	connect(opacitySlider, &AFQMouseClickSlider::valueChanged,
		qobject_cast<OverlaySceneWidget*>(this->parentWidget()), &OverlaySceneWidget::Opacity);

	layoutOpacity->addWidget(opacitySlider);

	//

	std::string iconPath;
	GetDataFilePath("assets/Popup/overlay", iconPath);

	QIcon settingIcon = QIcon(std::string(iconPath + "/icon_setting.svg").data());
	auto settingButton = new QPushButton(frame);
	settingButton->setStyleSheet(R"(
		QPushButton {
			background-color : rgba(255, 255, 255, 0.1); border-radius : 10px;
		}
		QPushButton QToolTip {
			border-radius: 0px;
		}
	)");
	settingButton->setIconSize(QSize(24, 24));
	settingButton->setFixedSize(QSize(44, 44));
	settingButton->setIcon(settingIcon);
	settingButton->setToolTip(QTStr("Settings"));
	connect(settingButton, &QPushButton::clicked,
		this, [&]()
		{
			AFQBorderPopupBaseWidget* popup = nullptr;
			MAIN_BLOCKMANAGER->MakePopup(ENUM_WINDOW_TYPE::SoopOverlay, popup);
		});
	layoutFrame->addWidget(settingButton);

	//

	QIcon resetIcon = QIcon(std::string(iconPath + "/icon_reset.svg").data());
	auto resetButton = new QPushButton(frame);
	resetButton->setStyleSheet(R"(
		QPushButton {
			background-color : rgba(255, 255, 255, 0.1); border-radius : 10px;
		}
		QPushButton QToolTip {
			border-radius: 0px;
		}
	)");
	resetButton->setIconSize(QSize(24, 24));
	resetButton->setFixedSize(QSize(44, 44));
	resetButton->setIcon(resetIcon);
	resetButton->setToolTip(QTStr("Reset"));
	connect(resetButton, &QPushButton::clicked,
		this, []() {OVERLAY_MANAGER.Reset(nullptr); });
	layoutFrame->addWidget(resetButton);

	//

	QIcon closeIcon = QIcon(std::string(iconPath + "/icon_close.svg").data());
	auto closeButton = new QPushButton(frame);
	closeButton->setStyleSheet(R"(
		QPushButton {
			background-color : rgba(255, 255, 255, 0.1); border-radius : 10px;
		}
		QPushButton QToolTip {
			border-radius: 0px;
		}
	)");
	closeButton->setIconSize(QSize(24, 24));
	closeButton->setFixedSize(QSize(44, 44));
	closeButton->setIcon(closeIcon);
	closeButton->setToolTip(QTStr("Close"));
	connect(closeButton, &QPushButton::clicked,
		this, []() {OVERLAY_MANAGER.Editable(false); });
	layoutFrame->addWidget(closeButton);
}

OverlaySettingSourceWidget::~OverlaySettingSourceWidget()
{
	config_set_int(ACTIVECONFIG, "Overlay", "Setting.Opacity", opacitySlider->value());

	auto y = (double)posRelative().y();
	config_set_double(ACTIVECONFIG, "Overlay", "Setting.Y", y);
	
	auto x = (double)posRelative().x();
	config_set_double(ACTIVECONFIG, "Overlay", "Setting.X", x);
}

void OverlaySettingSourceWidget::Opacity(int opacity)
{
	opacityLabel->setText(QTStr("Popup.LiveOverlay.Opacity").arg(opacity));
}

void OverlaySettingSourceWidget::paintEvent(QPaintEvent* event)
{
	//if (editable == true)
	//{
		Q_UNUSED(event);

		QStyleOption opt;
		opt.initFrom(this);

		QPainter painter(this);

		style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

		QPen pen;
		pen.setWidth(2);
		pen.setStyle(Qt::DashLine);
		pen.setDashPattern({ 2, 2 });
		pen.setColor(QColor(255, 255, 255, 127)); // RGBA: alpha 127 = 0.5
		pen.setCapStyle(Qt::FlatCap);

		painter.setPen(pen);
		painter.setBrush(Qt::NoBrush);

		QRect r = rect().adjusted(1, 1, -1, -1);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.drawRoundedRect(r, 10, 10); // border-radius: 10px
	//}
}