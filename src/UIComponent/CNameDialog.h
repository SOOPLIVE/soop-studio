#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>


#include "UIComponent/CTopBaseWindow.h"

class AFQNameDialog : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    explicit AFQNameDialog(QWidget* parent = nullptr);
    ~AFQNameDialog();

    // Returns true if user clicks OK, false otherwise
    // userTextInput returns string that user typed into dialog
    static bool AskForName(QWidget* parent, const QString& title,
                           const QString& text, std::string& userTextInput,
                           const QString& placeHolder = QString(""),
                           int maxSize = 170, bool isNotParent = false);

    // Returns true if user clicks OK, false otherwise
    // userTextInput returns string that user typed into dialog
    // userOptionReturn the checkbox was ticked user accepted
    static bool AskForNameWithOption(QWidget* parent, const QString& title,
                                     const QString& text, std::string& userTextInput,
                                     const QString& optionLabel, bool& optionChecked,
                                     const QString& placeHolder = QString(""));

protected:
    void showEvent(QShowEvent* event) override;

private:
    QLabel* m_pLabelTitle = nullptr;

    QLabel* m_pLabel = nullptr;
    QLineEdit* m_pUserText = nullptr;
    QCheckBox* m_pCheckbox = nullptr;
};