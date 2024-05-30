#include "CLoginToggleFrame.h"
#include "ui_login-toggle-frame.h"

AFQLoginToggleFrame::AFQLoginToggleFrame(QWidget *parent) :
    QFrame(parent, Qt::FramelessWindowHint),
    ui(new Ui::AFQLoginToggleFrame)
{
    ui->setupUi(this);
}

AFQLoginToggleFrame::~AFQLoginToggleFrame()
{
    delete ui;
}

void AFQLoginToggleFrame::VisibleToggleButton(bool show)
{
    ui->pushButton_LoginToggle->setVisible(show);
}

void AFQLoginToggleFrame::LoginToggleFrameInit(QString name, QString path)
{
    ui->label_PlatformName->setText(name);
    ui->label_PlatformIcon->setPixmap(QPixmap(path));
}
