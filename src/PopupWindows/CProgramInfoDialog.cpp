#include "CProgramInfoDialog.h"

#include "ui_program-info-dialog.h"

#include "platform/platform.hpp"

#include "MainFrame/CMainFrame.h"

#include <QFile>

AFQProgramInfoDialog::AFQProgramInfoDialog(QWidget* parent) :
	AFTTopBaseDialog(parent),
	ui(new Ui::AFQProgramInfoDialog)
{
	ui->setupUi(this);

#ifdef __APPLE__
    setWindowTitle(QTStr("ProgramInfo"));
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
    
#endif
    
	std::string path;
	if (GetDataFilePath("license/privacy-policy.html", path)) {
		QString filePath = QString::fromStdString(path);
		QString text = _ReadHtmlFile(filePath);
		ui->textBrowser->setOpenExternalLinks(true);
		ui->textBrowser->setHtml(text);
	}

	QString programTermsPath;
	programTermsPath = "license/program-terms-conditions_kor.html";

	if (GetDataFilePath(programTermsPath.toStdString().c_str(), path)) {
		QString filePath = QString::fromStdString(path);
		QString text = _ReadHtmlFile(filePath);
		ui->textBrowser_2->setHtml(text);
	}

	if (GetDataFilePath("license/license.html", path)) {
		QString filePath = QString::fromStdString(path);
		QString text = _ReadHtmlFile(filePath);
		ui->textBrowser_3->setOpenExternalLinks(true);
		ui->textBrowser_3->setHtml(text);
	}

	ui->pushButton_PrivacyPolicy->hide();
	ui->pushButton_TermsConditions->setChecked(true);
	ui->stackedWidget->setCurrentIndex(1);
	ui->pushButton_OpenSourceLicense->setChecked(false);

	connect(ui->pushButton_PrivacyPolicy, &QPushButton::clicked,
		this, &AFQProgramInfoDialog::qslotShowPrivacyPolicy);

	connect(ui->pushButton_TermsConditions, &QPushButton::clicked,
		this, &AFQProgramInfoDialog::qslotShowTermsConditions);

	connect(ui->pushButton_OpenSourceLicense, &QPushButton::clicked,
		this, &AFQProgramInfoDialog::qslotShowOpenSourceLisenceInfo);

	connect(ui->closeButton, &QPushButton::clicked,
		this, &AFQProgramInfoDialog::close);

}

AFQProgramInfoDialog::~AFQProgramInfoDialog()
{

}

void AFQProgramInfoDialog::qslotShowPrivacyPolicy()
{
	ui->pushButton_PrivacyPolicy->setChecked(true);
	ui->pushButton_TermsConditions->setChecked(false);
	ui->pushButton_OpenSourceLicense->setChecked(false);
	ui->stackedWidget->setCurrentIndex(0);
}

void AFQProgramInfoDialog::qslotShowTermsConditions()
{
	ui->pushButton_PrivacyPolicy->setChecked(false);
	ui->pushButton_TermsConditions->setChecked(true);
	ui->pushButton_OpenSourceLicense->setChecked(false);
	ui->stackedWidget->setCurrentIndex(1);
}

void AFQProgramInfoDialog::qslotShowOpenSourceLisenceInfo()
{
	ui->pushButton_PrivacyPolicy->setChecked(false);
	ui->pushButton_TermsConditions->setChecked(false);
	ui->pushButton_OpenSourceLicense->setChecked(true);
	ui->stackedWidget->setCurrentIndex(2);
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
