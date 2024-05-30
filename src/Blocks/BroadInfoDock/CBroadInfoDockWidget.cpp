#include "CBroadInfoDockWidget.h"
#include "ui_broad-info-dock.h"

AFBroadInfoDockWidget::AFBroadInfoDockWidget(QWidget *parent) : AFQBaseDockWidget(parent),
    ui(new Ui::AFBroadInfoDockWidget)
{
    ui->setupUi(this);
}

AFBroadInfoDockWidget::~AFBroadInfoDockWidget()
{
    delete ui;
}
