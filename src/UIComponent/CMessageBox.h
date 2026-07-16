#ifndef AFQMESSAGEBOX_H
#define AFQMESSAGEBOX_H

#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>


#include "UIComponent/CTopBaseWindow.h"

class AFQMessageBox : public AFTTopBaseDialog
{
#pragma region QT Field
    Q_OBJECT

public slots:
    //void accept();


signals:
    void ExtraButtonClicked();
#pragma endregion QT Field

#pragma region class initializer, destructor
public:
    explicit AFQMessageBox(QDialogButtonBox::StandardButtons buttons, 
                           QWidget* parent = nullptr , 
                           const QString& title = "", 
                           const QString& text = "", 
                           bool useWordWrap = true,
                           const QString& buttonText = "",
                           const QString& topText = "",
                           int fixedWidth = 0, 
                           int fixedHeight = 0,
                           QString buttonType = "",
                           QString checkBoxText = "");
    ~AFQMessageBox() {};

    void ChangeButtonText(QString buttonText);
    bool IsCheckBoxChecked() const { return m_checkBox ? m_checkBox->isChecked() : false; }

    static int ShowMessage(QDialogButtonBox::StandardButtons buttons,
        QWidget* parent,
        const QString& title,
        const QString& text,
        bool useWordWrap = true,
        bool isNotParent = false,
        const QString& topText = "",
        int fixedWidth = 0, int fixedHeight = 0, QString buttonType = "",
        QString checkBoxText = "", bool* checkBoxResult = nullptr);

    static int ShowMessageWithButtonText(QDialogButtonBox::StandardButtons buttons,
        QWidget* parent,
        const QString& title,
        const QString& text,
        QString changeButtonText = "",
        bool useWordWrap = true,
        bool isNotParent = false);

    static void ShowModalessOnButtonAlert(
        QWidget* parent,
        const QString& text, int fixedWidth = 0, int fixedHeight = 0);

    static void ShowModalessOneButtonMessage(QDialogButtonBox::StandardButtons buttons,
        QWidget* parent,
        const QString& title,
        const QString& text,
        bool useWordWrap = true,
        bool isNotParent = false,
        const QString& topText = "",
        int fixedWidth = 0, int fixedHeight = 0, QString buttonType = "");

protected:
    void showEvent(QShowEvent* event) override;

private:
    QCheckBox* m_checkBox = nullptr;
#pragma endregion class initializer, destructor
};

#endif // AFQMESSAGEBOX_H
