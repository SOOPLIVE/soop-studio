#include "CSavvyStyleButton.h"
#include "ui_savvy-style-button.h"

#include <QPainter>
#include <QMouseEvent>

AFQSavvyStyleButton::AFQSavvyStyleButton(std::string styleNum, QString styleName, QString styleText, QWidget *parent) :
    AFQHoverWidget(parent),
    ui(new Ui::AFQSavvyStyleButton)
{
    ui->setupUi(this);

    m_styleNum = styleNum;
    m_styleName = styleName;

    QString styleSheet = QString("image:url(%1)").arg(m_styleName);
    ui->widget_Style->setStyleSheet(styleSheet);
    ui->label_Style->setText(styleText);
}

AFQSavvyStyleButton::~AFQSavvyStyleButton()
{
    delete ui;
}

void AFQSavvyStyleButton::CheckStyle(bool checked)
{
    m_clicked = checked;
    QString styleSheet = "background:transparent;";
    if (checked)
        styleSheet += "border:3px solid #0182FF;";
    ui->widget_Border->setStyleSheet(styleSheet);
}

void AFQSavvyStyleButton::TriggerClick()
{
    emit qsignalMouseClick();
}

