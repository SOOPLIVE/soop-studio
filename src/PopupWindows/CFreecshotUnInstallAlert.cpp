#include "CFreecshotUnInstallAlert.h"
#include "ui_freecshot-uninstall-alert.h"

#include "MainFrame/CMainFrame.h"

AFQFreecshotUninstallAlert::AFQFreecshotUninstallAlert(QWidget *parent) :
    AFTTopBaseDialog(parent),
    ui(new Ui::AFQFreecshotUninstallAlert)
{
    ui->setupUi(this);    
    setProperty("MoveInAllArea", true);

    setModal(false);

    connect(ui->pushButton_Cancel, &QPushButton::clicked, 
        this, &AFQFreecshotUninstallAlert::qslotCancelClickedButton);

    connect(ui->pushButton_UnInstall, &QPushButton::clicked, 
        this, &AFQFreecshotUninstallAlert::qslotAcceptClickedButton);
}

AFQFreecshotUninstallAlert::~AFQFreecshotUninstallAlert()
{
    delete ui;
}

void AFQFreecshotUninstallAlert::qslotCancelClickedButton()
{
    this->close();
}

void AFQFreecshotUninstallAlert::qslotAcceptClickedButton()
{
    this->hide();

    emit qsignalninstallFreecshotAccept();

    this->close();
}