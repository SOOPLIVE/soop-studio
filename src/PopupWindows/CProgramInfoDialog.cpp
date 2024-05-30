#include "CProgramInfoDialog.h"

#include "ui_program-info-dialog.h"

#include "qt-wrapper.h"
#include "platform/platform.hpp"

#include <QFile>

AFQProgramInfoDialog::AFQProgramInfoDialog(QWidget* parent) :
	AFQRoundedDialogBase(parent),
	ui(new Ui::AFQProgramInfoDialog)
{
	ui->setupUi(this);

    QString styleSheet = R"(

    QScrollBar:vertical {
        border:none;
        border: 1px solid #999999;
        background: #24272D;
		border: 0px solid transparent;
        width: 6px;
    }

    QScrollBar::handle:vertical {
        background-color: rgba(255,255,255,30%);
	    border-radius:3px;
		min-height : 40px;
    }

    QScrollBar::add-line:vertical {
        border:none;
	    width: 0px;
	    subcontrol-position: right;
	    subcontrol-origin: margin;
    }

    QScrollBar::sub-line:vertical {
        border:none;
		width: 0px;
		subcontrol-position: left;
		subcontrol-origin: margin;
    }

    QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical {
        background: none;
    }

    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
        background: none;
    })";

	ui->textBrowser->setStyleSheet(styleSheet);

	ui->textBrowser_2->setStyleSheet(styleSheet);

	std::string path;
	if (GetDataFilePath("license/privacy-policy.html", path)) {
		QString filePath = QString::fromStdString(path);
		QString text= _ReadHtmlFile(filePath);
		ui->textBrowser->setHtml(text);
	}

	if (GetDataFilePath("license/program-terms-conditions.html", path)) {
		QString filePath = QString::fromStdString(path);
		QString text = _ReadHtmlFile(filePath);
		ui->textBrowser_2->setHtml(text);
	}

	ui->button_PrivacyPolicy->setChecked(true);
	ui->button_TermsConditions->setChecked(false);

	connect(ui->button_PrivacyPolicy, &QPushButton::clicked,
		this, &AFQProgramInfoDialog::qslotShowPrivacyPolicy);

	connect(ui->button_TermsConditions, &QPushButton::clicked,
		this, &AFQProgramInfoDialog::qslotShowTermsConditions);

	connect(ui->closeButton, &QPushButton::clicked,
		this, &AFQProgramInfoDialog::close);

}

AFQProgramInfoDialog::~AFQProgramInfoDialog()
{

}

void AFQProgramInfoDialog::qslotShowPrivacyPolicy()
{
	ui->button_PrivacyPolicy->setChecked(true);
	ui->button_TermsConditions->setChecked(false);
	ui->stackedWidget->setCurrentIndex(0);
}

void AFQProgramInfoDialog::qslotShowTermsConditions()
{
	ui->button_PrivacyPolicy->setChecked(false);
	ui->button_TermsConditions->setChecked(true);
	ui->stackedWidget->setCurrentIndex(1);
}

QString AFQProgramInfoDialog::_ReadHtmlFile(const QString& filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		qDebug() << "Cannot open file for reading:" << file.errorString();
		return QString();
	}

	QTextStream in(&file);
	QString fileContent = in.readAll();

	file.close();
	return fileContent;
}