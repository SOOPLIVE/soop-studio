#include "CParticleThumbnailWidget.h"
#include "ui_particle-thumbnail-widget.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#define CHECKED_PROPERTY "checked"

AFQParticleThumbnailWidget::AFQParticleThumbnailWidget(QWidget* parent, QString particleId) :
	m_particleId(particleId),
	ui(new Ui::AFQParticleThumbnailWidget)
{
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);
	setAttribute(Qt::WA_StyledBackground);

	_Init();
}

AFQParticleThumbnailWidget::~AFQParticleThumbnailWidget() 
{
	delete ui;
}

void AFQParticleThumbnailWidget::_Init() 
{
	ui->pushButton_Favorites->setVisible(false);

	_SetStyle();
	SetChecked(false);

	connect(ui->widget_HoverWidget, &AFQHoverWidget::qsignalMouseClick,
			this, &AFQParticleThumbnailWidget::_qslotParticleThumbnailClicked);
}

void AFQParticleThumbnailWidget::_SetStyle()
{
	if (m_particleId == "")
		return;

	std::string absPath;
	GetDataFilePath("assets", absPath);

	QString thumbnailPath = QString("%1/Popup/effect/particle/thumbnail/%2.png").arg(absPath.data(), m_particleId);
	QString styleSheet = QString("#widget_HoverWidget {"
								 "border-image: url(%1) 0 0 stretch stretch;"
								 "}").arg(thumbnailPath);

	ui->widget_HoverWidget->setStyleSheet(styleSheet);
}

void AFQParticleThumbnailWidget::_qslotParticleThumbnailClicked()
{
	bool isChecked = property(CHECKED_PROPERTY).toBool();
	SetChecked(!isChecked);

	if (isChecked == false) // unchecked -> checked
		emit qsignalParticleThumbnailClicked(m_particleId);
	else // checked -> unchecked
		emit qsignalParticleThumbnailClicked("None");
}

void AFQParticleThumbnailWidget::SetChecked(bool checked)
{
	ui->label_CheckedImg->setVisible(checked);
	setProperty(CHECKED_PROPERTY, checked);
	PolishStyleSheet(this);
}