#include "CSettingTabButton.h"
#include "ui_setting-tab-button.h"

#include <QStyleOption>
#include <QPainter>

#include "platform/platform.hpp"


AFQSettingTabButton::AFQSettingTabButton(QWidget *parent) :
    QPushButton(parent),
    ui(new Ui::AFQSettingTabButton)
{
    installEventFilter(this);

    ui->setupUi(this);

    connect(ui->pushButton_Icon, &QPushButton::clicked, this, &AFQSettingTabButton::qslotButtonClicked);
    connect(ui->pushButton_Title, &QPushButton::clicked, this, &AFQSettingTabButton::qslotButtonClicked);
    connect(this, &QPushButton::clicked, this, &AFQSettingTabButton::qslotButtonClicked);
    connect(this, &QPushButton::toggled, this, &AFQSettingTabButton::qslotButtonUnchecked);
}

AFQSettingTabButton::~AFQSettingTabButton()
{
    delete ui;
}

void AFQSettingTabButton::SetButton(const char* type, QString name)
{
    m_pTypeName = type;
    ui->pushButton_Title->setText(name);
    std::string absPath;
    GetDataFilePath("assets", absPath);

    QString styleSheet = QString("QPushButton#pushButton_Icon{background: transparent; "\
        "image: url(%1/setting-dialog/tabbutton/normal/%2.svg);}"\
        "QPushButton#pushButton_Icon[hover=false]:checked,"\
        "QPushButton#pushButton_Icon[hover=true]:checked{background: transparent; "\
        "image: url(%1/setting-dialog/tabbutton/clicked/%2.svg);}"\
        "QPushButton#pushButton_Icon[hover=true]{background: transparent; "\
        "image: url(%1/setting-dialog/tabbutton/hover/%2.svg);}")
            .arg(absPath.data(), type);
    ui->pushButton_Icon->setStyleSheet(styleSheet);
}

bool AFQSettingTabButton::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::HoverEnter) {
        ui->pushButton_Icon->setProperty("hover", true);
        ui->pushButton_Title->setProperty("hover", true);

        style()->unpolish(ui->pushButton_Icon);
        style()->unpolish(ui->pushButton_Title);
        style()->polish(ui->pushButton_Icon);
        style()->polish(ui->pushButton_Title);
    }
    else if (event->type() == QEvent::HoverLeave) {
        ui->pushButton_Icon->setProperty("hover", false);
        ui->pushButton_Title->setProperty("hover", false);
        
        style()->unpolish(ui->pushButton_Icon);
        style()->unpolish(ui->pushButton_Title);
        style()->polish(ui->pushButton_Icon);
        style()->polish(ui->pushButton_Title);
    }
    return QPushButton::eventFilter(obj, event);
}

void AFQSettingTabButton::qslotButtonUnchecked()
{
    ui->pushButton_Title->setChecked(false);
    ui->pushButton_Icon->setChecked(false);
}

void AFQSettingTabButton::qslotButtonClicked()
{
    setChecked(true);
    ui->pushButton_Title->setChecked(true);
    ui->pushButton_Icon->setChecked(true);
    emit qsignalButtonClicked();
}