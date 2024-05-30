#include "CCustomColorDialog.h"

#include "ui_custom-color-dialog.h"

AFQCustomColorDialog::AFQCustomColorDialog(const QColor& initial, const QString& title, QWidget* parent) :
	AFQRoundedDialogBase(parent),
	ui(new Ui::AFQCustomColorDialog)
{
    ui->setupUi(this);

	m_colorPicker = new QColorDialog(initial, this);
	m_colorPicker->setWindowFlags(Qt::Widget);

	connect(m_colorPicker, &QDialog::accepted, this, &AFQCustomColorDialog::qSlotAccept);
	connect(m_colorPicker, &QDialog::rejected, this, &AFQCustomColorDialog::qSlotAccept);
	connect(ui->closeButton, &QPushButton::clicked , this, &AFQCustomColorDialog::qSlotReject);
	
	connect(m_colorPicker, &QColorDialog::currentColorChanged, 
			this, &AFQCustomColorDialog::qSlotCurrentColorChanged);

	connect(m_colorPicker, &QColorDialog::colorSelected,
		this, &AFQCustomColorDialog::qSignalColorSelected);

	connect(m_colorPicker, &QColorDialog::rejected,
			this, &AFQCustomColorDialog::qSlotReject);

	ui->labelTitle->setText(title);
	ui->layoutColorPicker->addWidget((QWidget*)m_colorPicker);

	setStyleSheet("AFQCustomColorDialog { border:1px solid #111 }");

    this->SetHeightFixed(true);
    this->SetWidthFixed(true);
}

AFQCustomColorDialog::~AFQCustomColorDialog()
{
	delete ui;
}

QColor AFQCustomColorDialog::getColor(const QColor& initial, QWidget* parent,
									  const QString& title,
									  QColorDialog::ColorDialogOptions options)
{
	AFQCustomColorDialog dlg(initial, title, parent);

	dlg.setOptions(options);

	dlg.m_colorPicker->setOptions(options);
	dlg.m_colorPicker->setCurrentColor(initial);
	dlg.exec();

	QColor color = dlg.m_colorPicker->selectedColor();

	return color;
}

void AFQCustomColorDialog::setOptions(QColorDialog::ColorDialogOptions options)
{
	if (options == QColorDialog::ShowAlphaChannel)
		setFixedHeight(578);

	m_colorPicker->setOptions(options);
}

void AFQCustomColorDialog::qSlotAccept()
{
	this->accept();
}

void AFQCustomColorDialog::qSlotReject()
{
	this->reject();

	emit qSignalReject();
}

void AFQCustomColorDialog::qSlotCurrentColorChanged(const QColor& color)
{
	emit qSignalCurrentColorChanged(color);
}


void AFQCustomColorDialog::qSlotColorSelected(const QColor& color)
{
	emit qSignalColorSelected(color);
}