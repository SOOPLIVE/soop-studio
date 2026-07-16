#include "CPasswordSettingDialog.h"
#include "ui_password-setting-dialog.h"

#include "qt-wrappers.hpp"
#include <Application/CApplication.h>

#include "Blocks/CBlockManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "MainFrame/CMainFrame.h"

AFQPasswordSettingDialog::AFQPasswordSettingDialog(QWidget* parent, bool usePassword, const QString& password)  :
    AFTTopBaseDialog(parent),
    ui(new Ui::AFQPasswordSettingDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

#ifdef _WIN32
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Dialog|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    setWindowTitle(QTStr("Basic.PasswordPopup.Caption"));
    ui->titleFrame->hide();
#endif
    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);

    QString locale = QString::fromStdString(LOCALE_CONTEXT.GetCurrentLocaleStr());
    if (locale != "ko-KR")
        setFixedHeight(410);

    _Init(usePassword, password);
}

AFQPasswordSettingDialog::~AFQPasswordSettingDialog() 
{
    delete ui;
}

void AFQPasswordSettingDialog::_Init(bool usePassword, const QString& password)
{
    ui->pushButton_UsePassword->SetChecked(usePassword);
    ui->widget_InputPassword->SetMaxLength(11);
    ui->widget_InputPassword->SetText(password);
    
    _qslotToggledUsePasswordButton();

    ui->widget_InputPassword->SetPlaceholderText(QTStr("Password.InputPassword"));
    ui->widget_InputPassword->AllowOnlyAscii();
    ui->label_Warning->hide();
    ui->label_Guidance->setText(QTStr("Password.Notice"));

    // Style Property
    ui->pushButton_Save->setProperty("pushButtonTheme", "type2");
    PolishStyleSheet(ui->pushButton_Save);

    ui->pushButton_Cancel->setProperty("pushButtonTheme", "type4");
    PolishStyleSheet(ui->pushButton_Cancel);

    // Connect signal
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQPasswordSettingDialog::close);
    connect(ui->pushButton_Save, &QPushButton::clicked, this, &AFQPasswordSettingDialog::_qslotClickedSaveButton);
    connect(ui->pushButton_Cancel, &QPushButton::clicked, this, &AFQPasswordSettingDialog::close);
    connect(ui->pushButton_UsePassword, &QPushButton::clicked, this, &AFQPasswordSettingDialog::_qslotToggledUsePasswordButton);
    connect(ui->widget_InputPassword, &AFQSecureLengthAwareLineEdit::qsignalTextChanged, this, [=]() 
        {
            _qslotSetWarningStyle(false); 
        });
    connect(ui->widget_InputPassword->GetLineEdit(), &AFQBasicLineEdit::qsignalAsciiFailed,
            this, &AFQPasswordSettingDialog::_qslotAsciiError);

    ui->widget_InputPassword->GetLineEdit()->RestorePreviousText(false);

}

void AFQPasswordSettingDialog::_qslotClickedSaveButton()
{
    QString password = ui->widget_InputPassword->GetText();

    if (ui->pushButton_UsePassword->isChecked()) {
        PasswordCheck checkResult = _CheckPassword(password);

        if (checkResult == PasswordCheck::Pass)
        {
            _qslotSetWarningStyle(false);
            emit qsignalClickedUsePassword(true, password);
            close();
        }
        else
        {
            _qslotSetWarningStyle(true);
            _SetWarningText(checkResult);
            return;
        }
    }
    else {
        emit qsignalClickedUsePassword(false, password);
        close();
    }
}

void AFQPasswordSettingDialog::_qslotToggledUsePasswordButton()
{
    bool checked = ui->pushButton_UsePassword->isChecked();
    ui->widget_InputPassword->setEnabled(checked);
    //ui->label_Warning->setVisible(checked);
    ui->line->setVisible(checked);

    ui->widget_InputPassword->SetMaxLengthVisible(checked);
    ui->widget_InputPassword->SetHideTextButtonVisible(checked);

    if (!checked)
        _qslotSetWarningStyle(false);
}

AFQPasswordSettingDialog::PasswordCheck AFQPasswordSettingDialog::_CheckPassword(const QString& password)
{
    int nPassLen = password.length();
    if (nPassLen < 1)
        return PasswordCheck::Empty;
    if (nPassLen < 6)
        return PasswordCheck::TooShort;

    bool bCheckNumber = false;
    bool bCheckAlphabet = false;

    for (int i = 0; i < nPassLen; ++i) {
        QChar ch = password[i];
        bool bCheckSpecialChar = false;

        if (ch.isDigit()) {
            bCheckNumber = true;
            bCheckSpecialChar = true;
        }
        else if (ch.isLetter()) {
            bCheckAlphabet = true;
            bCheckSpecialChar = true;
        }
        else if (ch.unicode() >= 33 && ch.unicode() <= 47 ||
            ch.unicode() >= 58 && ch.unicode() <= 64 ||
            ch.unicode() >= 91 && ch.unicode() <= 96 ||
            ch.unicode() >= 123 && ch.unicode() <= 126) {
            bCheckSpecialChar = true;
        }

        if (!bCheckSpecialChar)
            return PasswordCheck::InvalidCharacters;
    }

    if (!bCheckNumber || !bCheckAlphabet)
        return PasswordCheck::MustMixLettersAndNumbers;

    for (int i = 0; i < nPassLen - 2; ++i) {
        if (password[i] == password[i + 1] && password[i + 1] == password[i + 2])
            return PasswordCheck::RepeatedChars;

        ushort a = password[i].unicode();
        ushort b = password[i + 1].unicode();
        ushort c = password[i + 2].unicode();

        if (b == a + 1 && c == b + 1)
            return PasswordCheck::SequentialChars;

        if (b == a - 1 && c == b - 1)
            return PasswordCheck::SequentialChars;
    }

    return PasswordCheck::Pass;
}

void AFQPasswordSettingDialog::_SetWarningText(PasswordCheck checkResult)
{
    switch (checkResult)
    {
    case PasswordCheck::Empty:
        ui->label_Warning->setText(QTStr("Password.InputPassword"));
        break;
    case PasswordCheck::TooShort:
        ui->label_Warning->setText(QTStr("Password.Guide.AtLeast6"));
        break;
    case PasswordCheck::MustMixLettersAndNumbers:
        ui->label_Warning->setText(QTStr("Password.Warning.MustContain"));
        break;
    case PasswordCheck::RepeatedChars:
        ui->label_Warning->setText(QTStr("Password.Warning.Repeat"));
        break;
    case PasswordCheck::SequentialChars:
        ui->label_Warning->setText(QTStr("Password.Warning.Consecutive"));
        break;
    case PasswordCheck::InvalidCharacters:
        ui->label_Warning->setText(QTStr("Password.Warning.IncludeOnly"));
        break;
    default:
        return;
    }
}

void AFQPasswordSettingDialog::_qslotSetWarningStyle(bool isWarningState)
{
    if (m_warningState == isWarningState)
        return;

    m_warningState = isWarningState;

    // Set property
    if (isWarningState) 
    {
        ui->label_Warning->show();
        ui->label_Warning->setProperty("labelType", "warning");
        ui->widget_InputPassword->setProperty("widgetType", "warning");
        ui->widget_InputPassword->SetLineEditProperty("lineEditType", "warning");
    }
    else 
    {
        //ui->label_Warning->setProperty("labelType", QVariant());
        ui->label_Warning->hide();
        ui->widget_InputPassword->setProperty("widgetType", QVariant());
        ui->widget_InputPassword->SetLineEditProperty("lineEditType", QVariant());
    }
    
    // Polish style
    PolishStyleSheet(ui->label_Warning);
    PolishStyleSheet(ui->widget_InputPassword);
    ui->widget_InputPassword->PolishLineEditStyle();
}

void AFQPasswordSettingDialog::_qslotAsciiError()
{
    _qslotSetWarningStyle(true);
    ui->label_Warning->setText(QTStr("Password.Warning.IncludeOnly"));
}