#include "CSettingTabButton.h"
#include "ui_setting-tab-button.h"

#include <QStyleOption>
#include <QPainter>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

AFQSettingTabButton::AFQSettingTabButton(QWidget *parent) :
    QPushButton(parent),
    ui(new Ui::AFQSettingTabButton)
{
    installEventFilter(this);

    ui->setupUi(this);

    connect(ui->iconButton, &QPushButton::clicked, 
        this, &AFQSettingTabButton::OnButtonClicked);
    connect(ui->titleButton, &QPushButton::clicked, 
        this, &AFQSettingTabButton::OnButtonClicked);

    connect(this, &QPushButton::clicked, 
        this, &AFQSettingTabButton::OnButtonClicked);
    connect(this, &QPushButton::toggled, 
        this, &AFQSettingTabButton::OnButtonUnchecked);
}

AFQSettingTabButton::~AFQSettingTabButton()
{
    delete ui;
}

void AFQSettingTabButton::SetButton(const char* type, QString name)
{
    typeName = type;
    ui->titleButton->setText(name);
    std::string absPath;
    GetDataFilePath("assets", absPath);

    QString styleSheet = QString("QPushButton#iconButton{background: transparent; "\
        "image: url(%1/setting-dialog/tabbutton/normal/%2.svg);}"\
        "QPushButton#iconButton[hover=false]:checked,"\
        "QPushButton#iconButton[hover=true]:checked{background: transparent; "\
        "image: url(%1/setting-dialog/tabbutton/clicked/%2.svg);}"\
        "QPushButton#iconButton[hover=true]{background: transparent; "\
        "image: url(%1/setting-dialog/tabbutton/hover/%2.svg);}")
            .arg(absPath.data(), type);
    ui->iconButton->setStyleSheet(styleSheet);
}

bool AFQSettingTabButton::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::HoverEnter) {
        ui->iconButton->setProperty("hover", true);
        ui->titleButton->setProperty("hover", true);

        PolishStyleSheet(ui->iconButton);
        PolishStyleSheet(ui->titleButton);
    }
    else if (event->type() == QEvent::HoverLeave) {
        ui->iconButton->setProperty("hover", false);
        ui->titleButton->setProperty("hover", false);
        
        PolishStyleSheet(ui->iconButton);
        PolishStyleSheet(ui->titleButton);
    }
    return QPushButton::eventFilter(obj, event);
}

void AFQSettingTabButton::OnButtonUnchecked()
{
    ui->titleButton->setChecked(false);
    ui->iconButton->setChecked(false);
}

void AFQSettingTabButton::OnButtonClicked()
{
    setChecked(true);
    ui->titleButton->setChecked(true);
    ui->iconButton->setChecked(true);
    emit ButtonClicked();
}
