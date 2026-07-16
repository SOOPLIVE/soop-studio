#include "CStudioUpdateLogDialog.h"

#include <qdir.h>

#include "ViewModel/Auth/Soop/auth-soop.hpp"

#include "ui_studio_update_log_dialog.h"

AFQStudioUpdateLogDialog::AFQStudioUpdateLogDialog(QWidget* parent) :
	AFTTopBaseDialog(parent),
	ui(new Ui::AFQStudioUpdateLogDialog)
{
	ui->setupUi(this);
	SetWidthResizeEnabled(false);
	SetHeightResizeEnabled(false);

	setModal(false);

    int updateLogheight = 670;
    if (MAINFRAME->IsSmallResolution())
        updateLogheight = 550;

	this->setFixedSize(560, updateLogheight);


#ifdef __APPLE__
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif

    connect(ui->pushButton_Close, &QPushButton::clicked, this, &QDialog::close);
    connect(ui->pushButton_Refresh, &QPushButton::clicked,
        this, &AFQStudioUpdateLogDialog::qslotRefreshButtonClicked);

}

AFQStudioUpdateLogDialog::~AFQStudioUpdateLogDialog()
{
	delete ui;
}

void AFQStudioUpdateLogDialog::qslotRefreshButtonClicked()
{
	ui->pushButton_Refresh->setDisabled(true);
	QTimer::singleShot(3000, this, [this]() {
		ui->pushButton_Refresh->setEnabled(true);
		});

	if (m_cefWidget)
        m_cefWidget->reloadPage();
}

void AFQStudioUpdateLogDialog::setMode(int mode)
{
    m_mode = mode;

    QString pageUrl;
    if (m_mode == FAQ) {

        ui->pushButton_Refresh->hide();

        setWindowTitle(QTStr("Popup.StudioFAQ.Caption"));
        ui->labelCaption->setText(QTStr("Popup.StudioFAQ.Caption"));

        QString localBase = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
        int lastSlashIndex = localBase.lastIndexOf("/");
        if (lastSlashIndex != -1) {
            localBase = localBase.left(lastSlashIndex);
        }
                
        QString faqPath = QDir(localBase).filePath(QString::fromStdString("SOOPStudio/SOOPStudio/data/obs-studio/assets/FAQ.html"));
        pageUrl = QUrl::fromLocalFile(faqPath).toString();
    }
    else {

        ui->pushButton_Refresh->show();

        setWindowTitle(QTStr("Popup.StudioUpdateLog.Caption"));
        ui->labelCaption->setText(QTStr("Popup.StudioUpdateLog.Caption"));
        const char* currentLang = config_get_string(USERCONFIG, "General", "Language");        
        pageUrl = QString::fromStdString(SOOP_STUDIO_UPDATE_LOG_PAGE).arg(currentLang);
    }

    if (m_cefWidget) {
        ui->verticalLayoutUpdateLog->removeWidget(m_cefWidget);
        m_cefWidget->deleteLater();
        m_cefWidget = nullptr;
    }

    m_cefWidget = CEFMANAGER.createWidget(this, pageUrl.toStdString(), CEFMANAGER.GetCefCookieManager());
    ui->verticalLayoutUpdateLog->addWidget(m_cefWidget);

}