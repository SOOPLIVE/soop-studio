#ifndef AFQMESSAGEBOX_H
#define AFQMESSAGEBOX_H

#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>


#include "UIComponent/CRoundedDialogBase.h"

class AFQMessageBox : public AFQRoundedDialogBase
{
#pragma region QT Field
    Q_OBJECT
#pragma endregion QT Field

#pragma region class initializer, destructor
public:
    explicit AFQMessageBox(QDialogButtonBox::StandardButtons buttons, 
                           QWidget* parent = nullptr , 
                           const QString& title = "", 
                           const QString& text = "", 
                           bool useWordWrap = true,
                           const QString& buttonText = "");
    ~AFQMessageBox() {};

    void ChangeButtonText(QString buttonText);

	static int ShowMessage(QDialogButtonBox::StandardButtons buttons, 
                           QWidget* parent, 
                           const QString& title, 
                           const QString& text, 
                           bool useWordWrap = true, 
                           bool isNotParent = false);

    static int ShowMessageWithButtonText(QDialogButtonBox::StandardButtons buttons,
        QWidget* parent,
        const QString& title,
        const QString& text,
        QString changeButtonText = "",
        bool useWordWrap = true,
        bool isNotParent = false);

#pragma endregion class initializer, destructor
};

#endif // AFQMESSAGEBOX_H
