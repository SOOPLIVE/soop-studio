#include "CCustomBrowserWidget.h"
#include "ui_custom-browser-widget.h"

AFQCustomBrowserWidget::AFQCustomBrowserWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQCustomBrowserWidget)
{
    ui->setupUi(this);
}

AFQCustomBrowserWidget::~AFQCustomBrowserWidget()
{
    delete ui;
}
