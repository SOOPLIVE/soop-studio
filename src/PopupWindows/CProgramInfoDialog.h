#pragma once

#include <QDialog>

#include "UIComponent/CRoundedDialogBase.h"

namespace Ui {
    class AFQProgramInfoDialog;
}

class AFQProgramInfoDialog : public AFQRoundedDialogBase
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    AFQProgramInfoDialog(QWidget* parent);
    ~AFQProgramInfoDialog();

private slots:
    void qslotShowPrivacyPolicy();
    void qslotShowTermsConditions();

#pragma endregion QT Field, CTOR/DTOR

private:
    QString _ReadHtmlFile(const QString& filePath);

#pragma region private member var
private:
    std::unique_ptr<Ui::AFQProgramInfoDialog> ui;

#pragma endregion private member var
};