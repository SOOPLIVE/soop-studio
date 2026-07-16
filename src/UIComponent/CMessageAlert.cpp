#include "CMessageAlert.h"
#include "ui_message-alert.h"

#include <QVBoxLayout>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "Application/CApplication.h"

AFQMessagBoxAlert::AFQMessagBoxAlert(QWidget* parent, QString title, QString info, 
	QString acceptText, QString cancelText, bool textInfoRichEdit)
	: AFTTopBaseDialog((QDialog*)parent),
	ui(new Ui::AFQMessagBoxAlert)
{
	ui->setupUi(this);

	if (title.isEmpty())
		ui->label_Title->hide();

	ui->label_Title->setText(title);
	ui->label_Info->setText(info);
	if(textInfoRichEdit)
		ui->label_Info->setTextFormat(Qt::RichText);

	ui->pushButton_Accept->setText(acceptText);
	ui->pushButton_Accept->setDefault(true);

	if (cancelText.isEmpty()) {
		ui->pushButton_Cancel->setText(QTStr("Cancel"));
	}
	else {
		ui->pushButton_Cancel->setText(cancelText);
	}

	connect(ui->pushButton_Accept, &QPushButton::clicked, 
		this, &AFQMessagBoxAlert::qslotAcceptButtonClicked);

	connect(ui->pushButton_Cancel, &QPushButton::clicked,
		this, &AFQMessagBoxAlert::qslotCancelButtonClicked);

	adjustSize();

}

AFQMessagBoxAlert::~AFQMessagBoxAlert()
{
	delete ui;
}

void AFQMessagBoxAlert::SetOneButtonAlert(QString buttonType)
{
	ui->pushButton_Cancel->hide();
	ui->pushButton_Cancel->close();

	ui->pushButton_Accept->setProperty("pushButtonTheme", buttonType);
	PolishStyleSheet(ui->pushButton_Accept);
}

void AFQMessagBoxAlert::qslotAcceptButtonClicked()
{
	accept();
}

void AFQMessagBoxAlert::qslotCancelButtonClicked()
{
	close();
}