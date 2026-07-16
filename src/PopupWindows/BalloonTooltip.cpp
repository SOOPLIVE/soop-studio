
#include "BalloonTooltip.h"
#include "ui_balloon-tooltip.h"


BalloonTooltip::BalloonTooltip(QString title, QString msg, QWidget* parent)
    : QWidget(parent),
    ui(new Ui::BalloonTooltip)
{
    ui->setupUi(this);

    ui->pointHeightFrame->hide();

#if defined(_WIN32)
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
#endif
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->titleLabel->setText(title);
    ui->msgLabel->setText(msg);
    //
    ui->titleLabel->adjustSize();
    ui->msgLabel->adjustSize();
    ui->balloonWidget->adjustSize();
    adjustSize();

    connect(ui->closeBtn, &QPushButton::clicked, this, &BalloonTooltip::closeTooltip);
    hide();
}
BalloonTooltip::~BalloonTooltip()
{
    delete ui;
}

void BalloonTooltip::SetTooltipPointPosY(int posY)
{
    ui->pointHeightFrame->setFixedHeight(posY);
    ui->pointHeightFrame->show();
}

//
void BalloonTooltip::closeTooltip()
{
    emit closeTooltipEvent();
    //
    close();
}
