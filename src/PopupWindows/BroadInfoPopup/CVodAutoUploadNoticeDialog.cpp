#include "CVodAutoUploadNoticeDialog.h"
#include "ui_vod-auto-upload-notice-dialog.h"


AFQVodAutoUploadNoticeDialog::AFQVodAutoUploadNoticeDialog(QWidget* parent) :
    AFTTopBaseDialog(parent),
    ui(new Ui::AFQVodAutoUploadNoticeDialog)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

    AFQBlockManager::ApplyMoveInAllArea(this);
    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);

    _Init();
}

AFQVodAutoUploadNoticeDialog::~AFQVodAutoUploadNoticeDialog()
{
    delete ui;
}

void AFQVodAutoUploadNoticeDialog::_qslotClickedButtonBox(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

    if (val == QDialogButtonBox::AcceptRole)
    {
    }

    close();
}

void AFQVodAutoUploadNoticeDialog::_Init()
{
    connect(ui->pushButton_Close, &QPushButton::clicked,
            this, &AFQVodAutoUploadNoticeDialog::close);
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
        this, &AFQVodAutoUploadNoticeDialog::_qslotClickedButtonBox);
}