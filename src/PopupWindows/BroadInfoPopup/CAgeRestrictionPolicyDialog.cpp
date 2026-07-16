#include "CAgeRestrictionPolicyDialog.h"
#include "ui_age-restriction-policy-dialog.h"

#include "qt-wrappers.hpp"

AFQAgeRestrictionPolicyDialog::AFQAgeRestrictionPolicyDialog(QWidget* parent) :
    AFTTopBaseDialog(parent),
    ui(new Ui::AFQAgeRestrictionPolicyDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

#ifdef _WIN32
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Dialog|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    setWindowTitle(QTStr("Caution.AgeRestricted"));
    ui->titleFrame->hide();
#endif
    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);

    _Init();
}

AFQAgeRestrictionPolicyDialog::~AFQAgeRestrictionPolicyDialog()
{
    delete ui;
}

void AFQAgeRestrictionPolicyDialog::_qslotClickedButtonBox(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

    if (val == QDialogButtonBox::AcceptRole)
    {
        if (ui->checkBox_HideToday->isChecked()) 
        {
            // Save Date
            QDate currentDate = QDateTime::currentDateTime().date();

            QString strCurrentDate = currentDate.toString("yyyy-MM-dd");
            config_set_string(USERCONFIG, "BroadInfo", AGE_RESTRICTION_POLICY_DATE_CHECK, QT_TO_UTF8(strCurrentDate));
            config_save_safe(USERCONFIG, "tmp", nullptr);
        }

        accept();
    }
    else
    {

        close();
    }
    close();
}

void AFQAgeRestrictionPolicyDialog::_Init()
{
    connect(ui->pushButton_Close, &QPushButton::clicked,
            this, &AFQAgeRestrictionPolicyDialog::close);
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
        this, &AFQAgeRestrictionPolicyDialog::_qslotClickedButtonBox);
}
