#include "CSystemAlert.h"
#include "include/qt-wrapper.h"
#include "ui_system-alert.h"

#define SYSTEM_ALERT_INTERVAL 3000

AFQSystemAlert::AFQSystemAlert(QWidget* parent, 
                               const QString& alertText, 
                               const QString& channelID, 
                               bool showInCorner, 
                               int mainFrameWidth) :
    QWidget(parent),
    ui(new Ui::AFQSystemAlert)
{
    ui->setupUi(this);
    setMouseTracking(true);
    installEventFilter(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowDoesNotAcceptFocus);
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose, true);

    m_qTimer = new QTimer(this);
    connect(m_qTimer, &QTimer::timeout, this, &AFQSystemAlert::hide);

    if (!showInCorner)
        ui->frame_SystemAlertTitle->setVisible(false);

    if (channelID == "")
        ui->label_channelID->setVisible(false);
    else {
        TruncateLabel(ui->label_channelID, channelID, 12);
        ui->label_channelID->setText(ui->label_channelID->text() + " : ");
    }

    ui->label_AlertText->setText(alertText);
    this->adjustSize();

    // change horizontal layout to vertical layout
    if (channelID != "")
    {
        int alertWidth = this->width();
        if (alertWidth >= mainFrameWidth * 0.6f) 
        {
            QLayout* oldLayout = ui->frame_TextArea->layout();
            QVBoxLayout* newLayout = new QVBoxLayout();
            newLayout->setSpacing(4);
            newLayout->addWidget(ui->label_channelID);
            newLayout->addWidget(ui->label_AlertText);

            delete oldLayout;

            ui->frame_TextArea->setLayout(newLayout);
            ui->frame_TextArea->adjustSize();
            ui->frame_Contents->adjustSize();
            ui->frame_SystemAlertBG->adjustSize();
            this->adjustSize();
        }
    }
    //
}

AFQSystemAlert::~AFQSystemAlert()
{
    m_qTimer->deleteLater();
    m_qTimer = nullptr;

    delete ui;
}

void AFQSystemAlert::mousePressEvent(QMouseEvent* event)
{
    hide();
}

void AFQSystemAlert::showEvent(QShowEvent* event)
{
    m_qTimer->start(SYSTEM_ALERT_INTERVAL);
}

void AFQSystemAlert::hideEvent(QHideEvent* event)
{
    m_qTimer->stop();
}