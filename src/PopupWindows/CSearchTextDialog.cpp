#include <obs-module.h>
#include <obs.hpp>

#include "ui_search-text-dialog.h"
#include "CSearchTextDialog.h"

AFQSearchDialog::AFQSearchDialog(QWidget *parent)
	: AFTTopBaseDialog(parent, Qt::Dialog),
	  ui(new Ui::AFQSearchDialog),
	  m_firstSearch(true)
{
	ui->setupUi(this);

	connect(ui->pushButton_FindNext, &QPushButton::clicked, 
			this, &AFQSearchDialog::qslotSearchNextClicked);

	connect(ui->pushButton_Close, &QPushButton::clicked,
			this, &AFQSearchDialog::qslotCloseButtonClicked);

	connect(ui->lineEdit_Text, &QLineEdit::textChanged,
			this, &AFQSearchDialog::qslotResetSearchText);

	connect(ui->lineEdit_Text, &QLineEdit::returnPressed,
			this, &AFQSearchDialog::qslotSearchNextClicked);

	ui->radioButton_Forward->setChecked(true);
}

AFQSearchDialog::~AFQSearchDialog()
{
	delete ui;
}

void AFQSearchDialog::closeEvent(QCloseEvent* event)
{
	emit qsignalStopSearchText(true);
}

void AFQSearchDialog::reject()
{
	close();
}

void AFQSearchDialog::qslotSearchNextClicked()
{
	QString searchText = ui->lineEdit_Text->text();
	if (searchText.isEmpty()) {
		emit qsignalStopSearchText(true);
		m_firstSearch = true;
		return;
	}

	emit qsignalSearchRequested(searchText,
								ui->checkBox_Upper->isChecked(), 
								ui->radioButton_Forward->isChecked(), 
								!m_firstSearch);
	m_firstSearch = false;
}

void AFQSearchDialog::qslotCloseButtonClicked()
{
	close();
}

void AFQSearchDialog::qslotResetSearchText()
{
	m_firstSearch = true;
}