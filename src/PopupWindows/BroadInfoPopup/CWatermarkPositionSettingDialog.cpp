#include "CWatermarkPositionSettingDialog.h"
#include "ui_watermark-setting-dialog.h"

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"
#include "Blocks/CBlockManager.h"

#define ICON_TOPBOTTOM_MARGIN 12
#define ICON_LEFTRIGHT_MARGIN 12

AFQWatermarkPositionSettingDialog::AFQWatermarkPositionSettingDialog(QWidget* parent, const std::vector<QString>& watermarkList, int watermarkPos) :
    AFTTopBaseDialog(parent),
    m_watermarkPos(watermarkPos),
    ui(new Ui::AFQWatermarkPositionSettingDialog)
{
    ui->setupUi(this);
 
    setAttribute(Qt::WA_DeleteOnClose, true);

#ifdef _WIN32
    AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowTitle(QTStr("BroadInfo.WatermarkPos"));
    ui->titleFrame->hide();
#endif
    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);

    _Init(watermarkList);
}

AFQWatermarkPositionSettingDialog::~AFQWatermarkPositionSettingDialog() 
{
    delete ui;
}

void AFQWatermarkPositionSettingDialog::_qslotClickedSaveButton()
{
    int newWatermarkPos = ui->comboBox_WatermarkPos->currentIndex();
    
    // If watermark position changed
    if (newWatermarkPos > -1 && newWatermarkPos != m_watermarkPos)
        emit qsignalWatermarkPositionChanged(newWatermarkPos);
    
    close();
}

void AFQWatermarkPositionSettingDialog::_qslotWatermarkPositionChanged(int index)
{
    _SetWatermarkPosition(index);
}

void AFQWatermarkPositionSettingDialog::_qslotWatermarkInitSetting()
{
    int nWatermarkCount = ui->comboBox_WatermarkPos->count();

    if (nWatermarkCount < 1)
    {
        ui->comboBox_WatermarkPos->setEnabled(false);
    }
    else
    {
        if (m_watermarkPos < 0 || m_watermarkPos >= nWatermarkCount)
            m_watermarkPos = 0;

        ui->comboBox_WatermarkPos->setCurrentIndex(m_watermarkPos);
        _SetWatermarkPosition(m_watermarkPos);
    }
}

void AFQWatermarkPositionSettingDialog::_Init(const std::vector<QString>& watermarkList)
{
    ui->comboBox_WatermarkPos->clear();
    
    int nWatermarkCount = watermarkList.size();
    for (int idx = 0; idx < nWatermarkCount; idx++)
        ui->comboBox_WatermarkPos->addItem(watermarkList[idx]);

    // Style Property
    QPushButton* saveButton = ui->buttonBox->button(QDialogButtonBox::Save);
    if (saveButton) {
        saveButton->setProperty("pushButtonTheme", "type2");
        PolishStyleSheet(saveButton);
    }

    // Connect signal
    connect(ui->pushButton_Close, &QPushButton::clicked,
            this, &AFQWatermarkPositionSettingDialog::close);
    connect(ui->buttonBox->button(QDialogButtonBox::Save), &QPushButton::clicked,
            this, &AFQWatermarkPositionSettingDialog::_qslotClickedSaveButton);
    connect(ui->comboBox_WatermarkPos, &QComboBox::currentIndexChanged,
        this, &AFQWatermarkPositionSettingDialog::_qslotWatermarkPositionChanged);

    QMetaObject::invokeMethod(this, "_qslotWatermarkInitSetting", Qt::QueuedConnection);
}

void AFQWatermarkPositionSettingDialog::_SetWatermarkPosition(int position)
{
    int xPos = ICON_LEFTRIGHT_MARGIN;
    int yPos = ICON_TOPBOTTOM_MARGIN;

    switch (position)
    {
    case 1: //CenterTop
        xPos = ui->widget_IconContainer->width() / 2 - ui->widget_LeftTop->width() / 2;
        break;
    case 2: //RightTop
        xPos = ui->widget_IconContainer->width() - ICON_LEFTRIGHT_MARGIN - ui->widget_LeftTop->width();
        break;
    case 3: //LeftBottom
        yPos = ui->widget_IconContainer->height() - ICON_TOPBOTTOM_MARGIN - ui->widget_LeftTop->height();
        break;
    case 4: //CenterBottom       
        xPos = ui->widget_IconContainer->width() / 2 - ui->widget_LeftTop->width() / 2;
        yPos = ui->widget_IconContainer->height() - ICON_TOPBOTTOM_MARGIN - ui->widget_LeftTop->height();
        break;
    case 5: //RightBottom
        xPos = ui->widget_IconContainer->width() - ICON_LEFTRIGHT_MARGIN - ui->widget_LeftTop->width();
        yPos = ui->widget_IconContainer->height() - ICON_TOPBOTTOM_MARGIN - ui->widget_LeftTop->height();
        break;
    default: //LeftTop
        break;
    }

    ui->widget_LeftTop->move(xPos, yPos);
}
