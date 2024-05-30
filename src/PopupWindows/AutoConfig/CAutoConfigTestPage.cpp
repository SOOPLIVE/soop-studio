#include "CAutoConfigTestPage.h"
#include "ui_auto-config-test-page.h"

AFQAutoConfigTestPage::AFQAutoConfigTestPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQAutoConfigTestPage)
{
    ui->setupUi(this);
}

AFQAutoConfigTestPage::~AFQAutoConfigTestPage()
{
    delete ui;
}
