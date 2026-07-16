#include "CSystemAlert.h"
#include "ui_system-alert.h"

#include "qt-wrappers.hpp"

#define SYSTEM_ALERT_INTERVAL_WARNING 3000
#define SYSTEM_ALERT_INTERVAL_SUCCESS 10000

AFQSystemAlert::AFQSystemAlert(QWidget* parent, 
                               const QString& alertText, 
                               const QString& channelID, 
                               bool showInCorner, 
                               int mainFrameWidth,
                               AlertIcon icon) :
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
    setAttribute(Qt::WA_ShowWithoutActivating, true);

    m_showInterval = (Warning == icon ? SYSTEM_ALERT_INTERVAL_WARNING : SYSTEM_ALERT_INTERVAL_SUCCESS);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AFQSystemAlert::hide);

    if (!showInCorner)
        ui->frame_SystemAlertTitle->setVisible(false);

    if (channelID == "")
        ui->label_channelID->setVisible(false);
    else {
        TruncateLabel(ui->label_channelID, channelID, 12);
        ui->label_channelID->setText(ui->label_channelID->text() + " : ");
    }

    ui->label_AlertIcon->setProperty("alertType", (int)icon);
    PolishStyleSheet(ui->label_AlertIcon);

    ui->label_AlertText->setMaximumWidth(mainFrameWidth * 0.7f);
    ui->label_AlertText->setText(alertText);
    ui->label_AlertText->adjustSize();

    QFontMetrics fm(ui->label_AlertText->font());
    int textWidth = fm.horizontalAdvance(alertText);
    int labelWidth = ui->label_AlertText->width();

    if (textWidth > labelWidth) {
        QString elided = fm.elidedText(alertText, Qt::ElideMiddle, labelWidth);
        ui->label_AlertText->setText(elided);
    }
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
}

AFQSystemAlert::~AFQSystemAlert()
{
    m_timer->deleteLater();
    m_timer = nullptr;

    delete ui;
}

void AFQSystemAlert::mousePressEvent(QMouseEvent* event)
{
    hide();
}

void AFQSystemAlert::showEvent(QShowEvent* event)
{
    m_timer->start(m_showInterval);
}

void AFQSystemAlert::hideEvent(QHideEvent* event)
{
    m_timer->stop();
}
