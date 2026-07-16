#pragma once

#include <QDialog>

#include "UIComponent/CTopBaseWindow.h"


namespace Ui {
    class AFQMessagBoxAlert;
}

class AFQMessagBoxAlert : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    explicit AFQMessagBoxAlert(QWidget* parent, QString title, QString info, 
        QString acceptText, QString cancelText = "", bool textInfoRichEdit = false);
    ~AFQMessagBoxAlert();

    void SetOneButtonAlert(QString typeButton);
private slots:
    void qslotAcceptButtonClicked();
    void qslotCancelButtonClicked();

private:
    Ui::AFQMessagBoxAlert* ui;

};