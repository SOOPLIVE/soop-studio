#include "CAutoConfigStreamPage.h"
#include "ui_auto-config-stream-page.h"

AFQAutoConfigStreamPage::AFQAutoConfigStreamPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQAutoConfigStreamPage)
{
    ui->setupUi(this);
}

AFQAutoConfigStreamPage::~AFQAutoConfigStreamPage()
{
    delete ui;
}
