#pragma once

#include <QDialog>

#include "UIComponent/CTopBaseWindow.h"

namespace Ui {
    class AFQProgramInfoDialog;
}

class AFQProgramInfoDialog : public AFTTopBaseDialog
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    AFQProgramInfoDialog(QWidget* parent);
    ~AFQProgramInfoDialog();

private slots:
    void qslotShowPrivacyPolicy();
    void qslotShowTermsConditions();
    void qslotShowOpenSourceLisenceInfo();

#pragma endregion QT Field, CTOR/DTOR

private:
    QString _ReadHtmlFile(const QString& filePath);

#pragma region private member var
private:
    std::unique_ptr<Ui::AFQProgramInfoDialog> ui;

#pragma endregion private member var
};