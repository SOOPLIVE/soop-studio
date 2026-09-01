#include "CCustomColorDialog.h"

#include "ui_custom-color-dialog.h"

AFQCustomColorDialog::AFQCustomColorDialog(const QColor& initial, const QString& title, QWidget* parent) :
	AFTTopBaseDialog(parent),
	ui(new Ui::AFQCustomColorDialog)
{
    ui->setupUi(this);

#ifdef _WIN32    
	ui->titleFrame->setProperty("MoveInAllArea", true);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
	ui->titleFrame->hide();
#endif
    

	m_pColorPicker = new QColorDialog(initial, this);
	m_pColorPicker->setWindowFlags(Qt::Widget);

	// set colorLuminancepicker boder transparent
	QList<QWidget*> children = m_pColorPicker->findChildren<QWidget*>();
	for (QWidget* widget : children) {
		const QMetaObject* metaObject = widget->metaObject();
		const char* className = metaObject->className();
		
		std::string classNameStr = className;
		if (classNameStr.find("QColorLuminancePicker") != std::string::npos) {
			QPalette pal = m_pColorPicker->palette();
			pal.setColor(QPalette::Light, Qt::transparent);
			pal.setColor(QPalette::Midlight, Qt::transparent);
			pal.setColor(QPalette::Dark, Qt::transparent);
			widget->setPalette(pal);
			break;
		}
	}
	//

	connect(m_pColorPicker, &QDialog::accepted, this, &AFQCustomColorDialog::qslotAccept);
	connect(m_pColorPicker, &QDialog::rejected, this, &AFQCustomColorDialog::qslotAccept);
	connect(ui->closeButton, &QPushButton::clicked , this, &AFQCustomColorDialog::qslotReject);
	
	connect(m_pColorPicker, &QColorDialog::currentColorChanged, 
			this, &AFQCustomColorDialog::qslotCurrentColorChanged);

	connect(m_pColorPicker, &QColorDialog::colorSelected,
		this, &AFQCustomColorDialog::qsignalColorSelected);

	connect(m_pColorPicker, &QColorDialog::rejected,
			this, &AFQCustomColorDialog::qslotReject);

	ui->labelTitle->setText(title);
	ui->layoutColorPicker->addWidget((QWidget*)m_pColorPicker);

    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);
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

	dlg.m_pColorPicker->setOptions(options);
	dlg.m_pColorPicker->setCurrentColor(initial);
	dlg.exec();

	QColor color = dlg.m_pColorPicker->selectedColor();

	return color;
}

void AFQCustomColorDialog::setOptions(QColorDialog::ColorDialogOptions options)
{
	if (options == QColorDialog::ShowAlphaChannel)
		setFixedHeight(578);

	m_pColorPicker->setOptions(options);
}

void AFQCustomColorDialog::qslotAccept()
{
	this->accept();
}

void AFQCustomColorDialog::qslotReject()
{
	this->reject();

	emit qsignalReject();
}

void AFQCustomColorDialog::qslotCurrentColorChanged(const QColor& color)
{
	emit qsignalCurrentColorChanged(color);
}


void AFQCustomColorDialog::qslotColorSelected(const QColor& color)
{
	emit qsignalColorSelected(color);
}
