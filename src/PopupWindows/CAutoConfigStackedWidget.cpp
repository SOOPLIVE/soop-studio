#include "CAutoConfigStackedWidget.h"
#include "ui_auto-config-stacked-widget.h"

AFQAutoConfigStackedWidget::AFQAutoConfigStackedWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AFQAutoConfigStackedWidget)
{
    ui->setupUi(this);
}

AFQAutoConfigStackedWidget::~AFQAutoConfigStackedWidget()
{
    delete ui;
}
