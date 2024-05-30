#include "CAutoConfigFinishPage.h"
#include "ui_auto-config-finish-page.h"

AFQAutoConfigFinishPage::AFQAutoConfigFinishPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQAutoConfigFinishPage)
{
    ui->setupUi(this);
}

AFQAutoConfigFinishPage::~AFQAutoConfigFinishPage()
{
    delete ui;
}
