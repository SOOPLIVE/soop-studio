#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>


#include "UIComponent/CRoundedDialogBase.h"

class AFQNameDialog : public AFQRoundedDialogBase
{

#pragma region QT Field
    Q_OBJECT
#pragma endregion QT Field

#pragma region class initializer, destructor
public:
    explicit AFQNameDialog(QWidget* parent = nullptr);
    ~AFQNameDialog();

#pragma endregion class initializer, destructor

    // Returns true if user clicks OK, false otherwise
    // userTextInput returns string that user typed into dialog
    static bool AskForName(QWidget* parent, const QString& title,
                           const QString& text, std::string& userTextInput,
                           const QString& placeHolder = QString(""),
                           int maxSize = 170, bool isNotParent = false);

    // Returns true if user clicks OK, false otherwise
    // userTextInput returns string that user typed into dialog
    // userOptionReturn the checkbox was ticked user accepted
    static bool
        AskForNameWithOption(QWidget* parent, const QString& title,
            const QString& text, std::string& userTextInput,
            const QString& optionLabel, bool& optionChecked,
            const QString& placeHolder = QString(""));

private:
    QLabel*     m_pLabelTitle;

    QLabel*     m_pLabel;
    QLineEdit*  m_pUserText;
    QCheckBox*  m_pCheckbox;
};