#include "CEmptyDialog.h"
#include "ui_empty-dialog.h"

#include "Blocks/CBlockManager.h"

#include "json11/json11.hpp"

AFQEmptyDialog::AFQEmptyDialog(QWidget* parent) :
	AFTTopBaseDialog((QDialog*)parent),
	ui(new Ui::AFQEmptyDialog)
{

	ui->setupUi(this); 
#ifdef _WIN32
		AFQBlockManager::ApplyMoveInAllArea(this);
#endif

	connect(ui->pushButton_Close, &QPushButton::clicked, 
		this, &AFQEmptyDialog::qslotCloseButtonClicked);
}

AFQEmptyDialog::~AFQEmptyDialog()
{
	delete ui;
}

void AFQEmptyDialog::qslotQueryRecieved(const QCefQuery& query)
{
	std::string jsonAllString = query.reqeust().toStdString();
	std::string err;
}

void AFQEmptyDialog::qslotCloseButtonClicked()
{
	close();
}

void AFQEmptyDialog::addWidget(QWidget* widget)
{
	ui->verticalLayout->addWidget(widget);
}

void AFQEmptyDialog::setTitle(QString title)
{
	ui->label_Title->setText(title);
}