#pragma once
#ifndef CDOWOOMIPOPUP_H
#define CDOWOOMIPOPUP_H
#include "obs.hpp"
#include "UIComponent/CTopBaseWindow.h"
#include "ui_dowoomi-popup.h"
#include <QDialog>

namespace Ui {
    class AFQDowoomiPopup;
}

class AFQDowoomiPopup : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    AFQDowoomiPopup(QWidget *parent = nullptr, int type = 0);
    ~AFQDowoomiPopup();

protected:
    virtual void showEvent(QShowEvent* event) override;

private:
    Ui::AFQDowoomiPopup *ui = nullptr;
};

#endif // CDOWOOMIPOPUP_H

