#include "CStudioSettingDialog.h"
#include "ui_studio-setting-dialog.h"

#include <QMessageBox>
#include <QGraphicsOpacityEffect>

#include "obs-properties.h"
#include "obs-source.h"

#include "Application/CApplication.h"
#include "./MainFrame/CMainFrame.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Config/CConfigManager.h"

#include "CSettingTabButton.h"
#include "CProgramSettingAreaWidget.h"
#include "CStreamSettingAreaWidget.h"
#include "COutputSettingAreaWidget.h"
#include "CAudioSettingAreaWidget.h"
#include "CVideoSettingAreaWidget.h"
#include "CHotkeySettingAreaWidget.h"
#include "CAccessibilitySettingAreaWidget.h"
#include "CAdvancedSettingAreaWidget.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "UIComponent/CMessageBox.h"

#include "qt-wrapper.h"

AFQStudioSettingDialog::AFQStudioSettingDialog(QWidget *parent) :
    AFQRoundedDialogBase(parent),
    m_MainFrame(qobject_cast<AFMainFrame*>(parent)),
    ui(new Ui::AFQStudioSettingDialog)
{
    ui->setupUi(this);

    m_pResetButton = ui->buttonBox->button(QDialogButtonBox::Reset);
    ChangeStyleSheet(m_pResetButton, STYLESHEET_RESET_BUTTON);

    SetWidthFixed(true);
    setAttribute(Qt::WA_DeleteOnClose);


    AFMainFrame* main = reinterpret_cast<AFMainFrame*>(parent);
    connect(main, &AFMainFrame::qsignalToggleUseVideo, this, &AFQStudioSettingDialog::qslotToggleStreamingUI);

   App()->DisableHotkeys();

   emit qslotToggleStreamingUI(false);
}

AFQStudioSettingDialog::~AFQStudioSettingDialog()
{
    App()->UpdateHotkeyFocusSetting();

    delete ui;
}

void AFQStudioSettingDialog::qslotCloseSetting()
{
    if (_AnyChanges())
        _QueryChanges();

    if (m_pProgramSettingAreaWidget)
    {
        m_pProgramSettingAreaWidget->SaveProgramPageSpreadState();

        config_save_safe(AFConfigManager::GetSingletonInstance().GetGlobal(), "tmp", nullptr);
    }
    
    close();
}

void AFQStudioSettingDialog::qslotTabButtonToggled()
{
    if (_AnyChanges())
        _QueryChanges();

    AFQSettingTabButton* checkedButton = reinterpret_cast<AFQSettingTabButton*>(sender());
    QList<AFQSettingTabButton*> buttonList = ui->widget_SettingTab->findChildren<AFQSettingTabButton*>();
    foreach(AFQSettingTabButton * button, buttonList)
    {
        if (button != checkedButton)
        {
            button->setChecked(false);
        }
    }

    ui->buttonBox->button(QDialogButtonBox::Cancel)->show();
    ui->buttonBox->button(QDialogButtonBox::Apply)->show();

    QMetaEnum TabTypeEnum = QMetaEnum::fromType<TabType>();
    int TabTypeNum = TabTypeEnum.keyToValue(checkedButton->GetButtonType());
    switch (TabTypeNum)
    {
    case TabType::PROGRAM:
    {
        m_pProgramSettingAreaWidget->LoadMainAccount();
        break;
    }
    case TabType::STREAM:
    {
        m_pStreamSettingAreaWidget->LoadStreamAccountSaved();
        ui->buttonBox->button(QDialogButtonBox::Cancel)->hide();
        ui->buttonBox->button(QDialogButtonBox::Apply)->hide();
        break;
    }
    case TabType::OUTPUT:
    {
        QString strAdvVEncoder = m_pVideoSettingAreaWidget->GetAdvVideoEncoder();
        QString strAdvAEncoder = m_pAudioSettingAreaWidget->GetAdvAudioEncoder();

        m_pOutputSettingAreaWidget->AdvOutEncoderData(strAdvVEncoder, strAdvAEncoder);

        break;
    }
    }
    ui->stackedWidget->setCurrentIndex(TabTypeNum);
    m_eCurrentTabNum = TabTypeNum;
    _UpdateResetButtonVisible();
}

void AFQStudioSettingDialog::qslotResetButtonClicked()
{
    std::string resetLocaleKey = "Basic.Settings.Reset";

    int curTabIndex = ui->stackedWidget->currentIndex();
    TabType curTabType = static_cast<TabType>(curTabIndex);
    switch (curTabType) {
        case TabType::OUTPUT:
        case TabType::AUDIO:
        case TabType::VIDEO:
            resetLocaleKey = "Basic.Settings.ResetAVOutput";
            break;
    }

    int result = AFQMessageBox::ShowMessage(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this, QTStr("Reset"),
        QTStr(resetLocaleKey.c_str()));
    if (result == QDialog::Rejected) {
        return;
    }
    else if (result == QDialog::Accepted) {
        QScreen* primaryScreen = QGuiApplication::primaryScreen();
        uint32_t cx = primaryScreen->size().width();
        uint32_t cy = primaryScreen->size().height();

        int curTabIndex = ui->stackedWidget->currentIndex();
        TabType curTabType = static_cast<TabType>(curTabIndex);
        switch (curTabType) {
            case TabType::PROGRAM:
                m_pProgramSettingAreaWidget->ResetProgramSettings();
                break;
            case TabType::STREAM:
                break;
            case TabType::OUTPUT:
            case TabType::AUDIO:
            case TabType::VIDEO:
                AFConfigManager::GetSingletonInstance().ResetAVOutputConfig(cx, cy);
                m_pOutputSettingAreaWidget->ResetOutputSettings();
                m_pAudioSettingAreaWidget->ResetAudioSettings();
                m_pVideoSettingAreaWidget->ResetVideoSettings();
                break;
            case TabType::HOTKEYS:
                m_pHotkeysSettingAreaWidget->ClearHotkeyValues();
                m_pHotkeysSettingAreaWidget->SaveHotkeysSettings();
                AFConfigManager::GetSingletonInstance()
                    .UpdateHotkeyFocusSetting(true, true);
                m_pHotkeysSettingAreaWidget->UpdateFocusBehaviorComboBox();
                break;
            case TabType::ACCESSIBILITY:
                AFConfigManager::GetSingletonInstance()
                    .ResetAccessibilityConfig();
                m_pAccesibilitySettingAreaWidget->LoadAccessibilitySettings();
                break;
            case TabType::ADVANCED:
                AFConfigManager::GetSingletonInstance().ResetAdvancedConfig();
                m_pAdvancedSettingAreaWidget->LoadAdvancedSettings();
                break;
        }

        App()->GetMainView()->GetMainWindow()->GetVideoUtil()->ResetVideo();

        _ClearChanged();
        _ApplyDisable();

        bool bLanguageChanged =
            m_pProgramSettingAreaWidget->CheckLanguageRestartRequired();
        bool bAudioRestart =
            m_pAudioSettingAreaWidget->CheckAudioRestartRequired();
        bool bHWAcceelChanged =
            m_pAdvancedSettingAreaWidget
                ->CheckBrowserHardwareAccelerationRestartRequired();

        g_bRestart = bLanguageChanged || bAudioRestart || bHWAcceelChanged;
    }
}

void AFQStudioSettingDialog::qslotResetDownscales(int cx, int cy)
{
    uint32_t outCx = m_pVideoSettingAreaWidget->GetVideoOutputCx();
    uint32_t outCy = m_pVideoSettingAreaWidget->GetVideoOutputCy();
    QComboBox* outRescaleCombobox = m_pVideoSettingAreaWidget->GetOutResolutionComboBox();
    
    m_pOutputSettingAreaWidget->OutputResolution(outCx, outCy);
    m_pOutputSettingAreaWidget->RefreshDownscales(cx, cy, outRescaleCombobox);
}

void AFQStudioSettingDialog::qslotSettingPageDataChanged()
{
    _ApplyEnable();
}

void AFQStudioSettingDialog::qslotChangeSettingModeToSimple()
{
    if (m_pAudioSettingAreaWidget == nullptr ||
        m_pVideoSettingAreaWidget == nullptr ||
        m_pOutputSettingAreaWidget == nullptr)
        return;

    m_pAudioSettingAreaWidget->ChangeSettingModeToSimple();
    m_pVideoSettingAreaWidget->ChangeSettingModeToSimple();
    m_pOutputSettingAreaWidget->ChangeSettingModeToSimple();
    
    App()->GetMainView()->ResetOutputs();
}

void AFQStudioSettingDialog::qslotChangeSettingModeToAdvanced()
{
    if (m_pAudioSettingAreaWidget == nullptr ||
        m_pVideoSettingAreaWidget == nullptr ||
        m_pOutputSettingAreaWidget == nullptr)
        return;

    m_pAudioSettingAreaWidget->ChangeSettingModeToAdvanced();
    m_pVideoSettingAreaWidget->ChangeSettingModeToAdvanced();
    m_pOutputSettingAreaWidget->ChangeSettingModeToAdvanced();
    
    App()->GetMainView()->ResetOutputs();
}

void AFQStudioSettingDialog::qslotSimpleReplayBufferChanged()
{
    if (m_pOutputSettingAreaWidget == nullptr)
        return;

    m_pOutputSettingAreaWidget->qslotSimpleReplayBufferChanged();
}

void AFQStudioSettingDialog::qslotAdvReplayBufferChanged()
{
    if (m_pOutputSettingAreaWidget == nullptr)
        return;

    m_pOutputSettingAreaWidget->qslotAdvReplayBufferChanged();
}

void AFQStudioSettingDialog::qslotSimpleOutVEncoderChanged(QString vEncoder)
{
    if (m_pOutputSettingAreaWidget == nullptr)
        return;

    m_pOutputSettingAreaWidget->SimpleOutVEncoder(vEncoder);
}

void AFQStudioSettingDialog::qslotSimpleOutAEncoderChanged(QString aEncoder)
{
    if (m_pOutputSettingAreaWidget == nullptr)
        return;

    m_pOutputSettingAreaWidget->SimpleOutAEncoder(aEncoder);
}

void AFQStudioSettingDialog::qslotSimpleOutVBitrateChanged(int vBitrate)
{
    if (m_pOutputSettingAreaWidget == nullptr)
        return;

    m_pOutputSettingAreaWidget->SimpleOutVBitrate(vBitrate);
}

void AFQStudioSettingDialog::qslotSimpleOutABitrateChanged(int aBitrate)
{
    if (m_pOutputSettingAreaWidget == nullptr)
        return;

    m_pOutputSettingAreaWidget->SimpleOutABitrate(aBitrate);
}
void AFQStudioSettingDialog::qslotSimpleRecordingEncoderChanged()
{
    if (m_pOutputSettingAreaWidget == nullptr)
        return;

    m_pOutputSettingAreaWidget->qslotSimpleRecordingEncoderChanged();
}

void AFQStudioSettingDialog::qslotStreamEncoderPropsChanged()
{
    if (m_pVideoSettingAreaWidget == nullptr ||
        m_pOutputSettingAreaWidget == nullptr)
        return;
    
   int vbitrate = m_pVideoSettingAreaWidget->VideoAdvBitrate();
   const char* rateControl = m_pVideoSettingAreaWidget->VideoAdvRateControl();

    if (vbitrate == 0 && rateControl == "")
    {
        m_pOutputSettingAreaWidget->HasStreamEncoder(false);
    }
    else
    {
        m_pOutputSettingAreaWidget->HasStreamEncoder(true);
        m_pOutputSettingAreaWidget->StreamEncoderData(vbitrate, rateControl);
    }

    m_pOutputSettingAreaWidget->qslotUpdateStreamDelayEstimate();
    m_pOutputSettingAreaWidget->qslotAdvReplayBufferChanged();
}

void AFQStudioSettingDialog::qslotUpdateStreamDelayEstimate()
{
    if (m_pOutputSettingAreaWidget == nullptr)
        return;

    m_pOutputSettingAreaWidget->qslotUpdateStreamDelayEstimate();
}

void AFQStudioSettingDialog::qslotButtonBoxClicked(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

    if (val == QDialogButtonBox::ApplyRole ||
        val == QDialogButtonBox::AcceptRole)
    {
        if (!_QueryAllowedToClose())
            return;

        _SaveSettings();
        _ClearChanged();
        _ApplyDisable();
    }

    if (val == QDialogButtonBox::AcceptRole ||
        val == QDialogButtonBox::RejectRole) 
    {
        if (val == QDialogButtonBox::RejectRole)
        {
        }
        qslotCloseSetting();
    }
}

void AFQStudioSettingDialog::qslotSetAutoRemuxText(QString autoRemuxText) 
{
    if (!m_pAdvancedSettingAreaWidget)
        return;
    m_pAdvancedSettingAreaWidget->SetAutoRemuxText(autoRemuxText);
}

void AFQStudioSettingDialog::qslotToggleStreamingUI(bool streaming)
{
    if (m_pProgramSettingAreaWidget)
        m_pProgramSettingAreaWidget->ToggleOnStreaming(streaming);

    if (m_pStreamSettingAreaWidget)
        m_pStreamSettingAreaWidget->ToggleOnStreaming(streaming);

    if (m_pOutputSettingAreaWidget)
        m_pOutputSettingAreaWidget->ToggleOnStreaming(streaming);

    if (m_pAudioSettingAreaWidget)
        m_pAudioSettingAreaWidget->ToggleOnStreaming(streaming);

    if (m_pVideoSettingAreaWidget)
        m_pVideoSettingAreaWidget->ToggleOnStreaming(streaming);

    if (m_pAdvancedSettingAreaWidget)
        m_pAdvancedSettingAreaWidget->ToggleOnStreaming(streaming);

    bool useVideo = obs_video_active() ? false : true;
    ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(useVideo);
}

void AFQStudioSettingDialog::AFQStudioSettingDialogInit(int type)
{
    ui->label_Title->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Settings")));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("OK")));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Cancel")));
    ui->buttonBox->button(QDialogButtonBox::Apply)->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Apply")));
    ui->buttonBox->button(QDialogButtonBox::Reset)->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Reset")));

    _SetTabButtons();
    _SetSettingAreaWidget();
    _SetSettingDialogSignal();

    switch (type)
    {
    case 0:
        ui->tabButtonProgram->clicked();
        break;
    case 1:
        ui->tabButtonStream->clicked();
        break;
    case 2:
        ui->tabButtonOutput->clicked();
        break;
    case 3:
        ui->tabButtonAudio->clicked();
        break;
    case 4:
        ui->tabButtonVideo->clicked();
        break;
    case 5:
        ui->tabButtonHotkeys->clicked();
        break;
    case 6:
        ui->tabButtonAccessibilty->clicked();
        break;
    case 7:
        ui->tabButtonAdvanced->clicked();
        break;
    }
}

QString AFQStudioSettingDialog::GetCurrentAudioBitrate()
{
    if (m_pAudioSettingAreaWidget != nullptr)
    {
        return m_pAudioSettingAreaWidget->CurrentAudioBitRate();
    }
    return "";
}

void AFQStudioSettingDialog::SetID(QString id)
{
    if (m_pStreamSettingAreaWidget != nullptr)
    {
        m_pStreamSettingAreaWidget->FindAccountButtonWithID(id);
    }
}

void AFQStudioSettingDialog::reject() {
    if (_AnyChanges()) {
        _QueryChanges();
    }

    QDialog::reject();
}

void AFQStudioSettingDialog::_SetSettingAreaWidget() {
    if (!m_pProgramSettingAreaWidget)
        m_pProgramSettingAreaWidget = new AFQProgramSettingAreaWidget(this);

    if (!m_pStreamSettingAreaWidget)
        m_pStreamSettingAreaWidget = new AFQStreamSettingAreaWidget(this);

    if (!m_pOutputSettingAreaWidget)
        m_pOutputSettingAreaWidget = new AFQOutputSettingAreaWidget(this);

    if (!m_pAudioSettingAreaWidget)
        m_pAudioSettingAreaWidget = new AFQAudioSettingAreaWidget(this);

    if (!m_pVideoSettingAreaWidget)
        m_pVideoSettingAreaWidget = new AFQVideoSettingAreaWidget(this);

    if (!m_pHotkeysSettingAreaWidget)
        m_pHotkeysSettingAreaWidget = new AFQHotkeySettingAreaWidget(this);

    if (!m_pAccesibilitySettingAreaWidget)
        m_pAccesibilitySettingAreaWidget =
            new AFQAccessibilitySettingAreaWidget(this);

    if (!m_pAdvancedSettingAreaWidget)
        m_pAdvancedSettingAreaWidget = new AFQAdvancedSettingAreaWidget(this);

    connect(m_pProgramSettingAreaWidget,
            &AFQProgramSettingAreaWidget::qsignalProgramDataChanged, this,
            &AFQStudioSettingDialog::qslotSettingPageDataChanged);
    connect(
        m_pOutputSettingAreaWidget,
        &AFQOutputSettingAreaWidget::qsignalUpdateReplayBufferStream,
        m_pProgramSettingAreaWidget,
        &AFQProgramSettingAreaWidget::UpdateAutomaticReplayBufferCheckboxes);
    connect(m_pOutputSettingAreaWidget,
            &AFQOutputSettingAreaWidget::qsignalSimpleModeClicked, this,
            &AFQStudioSettingDialog::qslotChangeSettingModeToSimple);
    connect(m_pOutputSettingAreaWidget,
            &AFQOutputSettingAreaWidget::qsignalAdvancedModeClicked, this,
            &AFQStudioSettingDialog::qslotChangeSettingModeToAdvanced);
    connect(m_pOutputSettingAreaWidget,
            &AFQOutputSettingAreaWidget::qsignalOutputDataChanged, this,
            &AFQStudioSettingDialog::qslotSettingPageDataChanged);
    connect(m_pOutputSettingAreaWidget,
            &AFQOutputSettingAreaWidget::qsignalSetAutoRemuxText, this,
            &AFQStudioSettingDialog::qslotSetAutoRemuxText);

    connect(m_pAudioSettingAreaWidget,
            &AFQAudioSettingAreaWidget::qsignalCallSimpleReplayBufferChanged,
            this, &AFQStudioSettingDialog::qslotSimpleReplayBufferChanged);
    connect(
        m_pAudioSettingAreaWidget,
        &AFQAudioSettingAreaWidget::qsignalCallSimpleRecordingEncoderChanged,
        this, &AFQStudioSettingDialog::qslotSimpleRecordingEncoderChanged);
    connect(m_pAudioSettingAreaWidget,
            &AFQAudioSettingAreaWidget::qsignalCallUpdateStreamDelayEstimate,
            this, &AFQStudioSettingDialog::qslotUpdateStreamDelayEstimate);
    connect(m_pAudioSettingAreaWidget,
            &AFQAudioSettingAreaWidget::qsignalSimpleModeClicked, this,
            &AFQStudioSettingDialog::qslotChangeSettingModeToSimple);
    connect(m_pAudioSettingAreaWidget,
            &AFQAudioSettingAreaWidget::qsignalAdvancedModeClicked, this,
            &AFQStudioSettingDialog::qslotChangeSettingModeToAdvanced);
    connect(m_pAudioSettingAreaWidget,
            &AFQAudioSettingAreaWidget::qsignalSimpleEncoderChanged, this,
            &AFQStudioSettingDialog::qslotSimpleOutAEncoderChanged);
    connect(m_pAudioSettingAreaWidget,
            &AFQAudioSettingAreaWidget::qsignalSimpleBitrateChanged, this,
            &AFQStudioSettingDialog::qslotSimpleOutABitrateChanged);
    connect(m_pAudioSettingAreaWidget,
            &AFQAudioSettingAreaWidget::qsignalAudioDataChanged, this,
            &AFQStudioSettingDialog::qslotSettingPageDataChanged);

    connect(m_pVideoSettingAreaWidget,
            &AFQVideoSettingAreaWidget::qsignalBaseResolutionChanged, this,
            &AFQStudioSettingDialog::qslotResetDownscales);
    connect(m_pVideoSettingAreaWidget,
            &AFQVideoSettingAreaWidget::qsignalCallSimpleReplayBufferChanged,
            this, &AFQStudioSettingDialog::qslotSimpleReplayBufferChanged);
    connect(
        m_pVideoSettingAreaWidget,
        &AFQVideoSettingAreaWidget::qsignalCallSimpleRecordingEncoderChanged,
        this, &AFQStudioSettingDialog::qslotSimpleRecordingEncoderChanged);
    connect(m_pVideoSettingAreaWidget,
            &AFQVideoSettingAreaWidget::qsignalCallStreamEncoderPropChanged,
            this, &AFQStudioSettingDialog::qslotStreamEncoderPropsChanged);
    connect(m_pVideoSettingAreaWidget,
            &AFQVideoSettingAreaWidget::qsignalCallUpdateStreamDelayEstimate,
            this, &AFQStudioSettingDialog::qslotUpdateStreamDelayEstimate);
    connect(m_pVideoSettingAreaWidget,
            &AFQVideoSettingAreaWidget::qsignalSimpleModeClicked, this,
            &AFQStudioSettingDialog::qslotChangeSettingModeToSimple);
    connect(m_pVideoSettingAreaWidget,
            &AFQVideoSettingAreaWidget::qsignalAdvancedModeClicked, this,
            &AFQStudioSettingDialog::qslotChangeSettingModeToAdvanced);
    connect(m_pVideoSettingAreaWidget,
            &AFQVideoSettingAreaWidget::qsignalSimpleEncoderChanged, this,
            &AFQStudioSettingDialog::qslotSimpleOutVEncoderChanged);
    connect(m_pVideoSettingAreaWidget,
            &AFQVideoSettingAreaWidget::qsignalSimpleBitrateChanged, this,
            &AFQStudioSettingDialog::qslotSimpleOutVBitrateChanged);
    connect(m_pVideoSettingAreaWidget,
            &AFQVideoSettingAreaWidget::qsignalVideoDataChanged, this,
            &AFQStudioSettingDialog::qslotSettingPageDataChanged);

    connect(m_pHotkeysSettingAreaWidget,
            &AFQHotkeySettingAreaWidget::qsignalHotkeyChanged, this,
            &AFQStudioSettingDialog::qslotSettingPageDataChanged);

    connect(m_pAccesibilitySettingAreaWidget,
            &AFQAccessibilitySettingAreaWidget::qsignalA11yDataChanged, this,
            &AFQStudioSettingDialog::qslotSettingPageDataChanged);

    connect(m_pAdvancedSettingAreaWidget,
            &AFQAdvancedSettingAreaWidget::
                qsignalCallOutputSettingUpdateStreamDelayEstimate,
            this, &AFQStudioSettingDialog::qslotUpdateStreamDelayEstimate);
    connect(m_pAdvancedSettingAreaWidget,
            &AFQAdvancedSettingAreaWidget::qsignalAdvancedDataChanged, this,
            &AFQStudioSettingDialog::qslotSettingPageDataChanged);

    m_pProgramSettingAreaWidget->ProgramSettingAreaInit();
    QVBoxLayout* ProgramLayout = new QVBoxLayout(ui->programSettingArea);
    ProgramLayout->setContentsMargins(0, 0, 0, 0);
    ProgramLayout->addWidget(m_pProgramSettingAreaWidget);
    ui->programSettingArea->setLayout(ProgramLayout);

    m_pStreamSettingAreaWidget->StreamSettingAreaInit();
    QVBoxLayout* StreamLayout = new QVBoxLayout(ui->streamSettingArea);
    StreamLayout->setContentsMargins(0, 0, 0, 0);
    StreamLayout->addWidget(m_pStreamSettingAreaWidget);
    ui->streamSettingArea->setLayout(StreamLayout);

    m_pOutputSettingAreaWidget->LoadOutputSettings();
    QVBoxLayout* OutputLayout = new QVBoxLayout(ui->outputSettingArea);
    OutputLayout->addWidget(m_pOutputSettingAreaWidget);
    OutputLayout->setContentsMargins(0, 0, 0, 0);
    ui->outputSettingArea->setLayout(OutputLayout);

    m_pAudioSettingAreaWidget->LoadAudioSettings();
    QVBoxLayout* vLayoutAudio = new QVBoxLayout(ui->audioSettingArea);
    vLayoutAudio->addWidget(m_pAudioSettingAreaWidget);
    vLayoutAudio->setContentsMargins(0, 0, 0, 0);
    ui->audioSettingArea->setLayout(vLayoutAudio);

    m_pVideoSettingAreaWidget->LoadVideoSettings();
    QVBoxLayout* vLayoutVideo = new QVBoxLayout(ui->videoSettingArea);
    vLayoutVideo->addWidget(m_pVideoSettingAreaWidget);
    vLayoutVideo->setContentsMargins(0, 0, 0, 0);
    ui->videoSettingArea->setLayout(vLayoutVideo);

    m_pHotkeysSettingAreaWidget->LoadHotkeysSettings();
    QVBoxLayout* HotkeyLayout = new QVBoxLayout(ui->accessibiltySettingArea);
    HotkeyLayout->addWidget(m_pHotkeysSettingAreaWidget);
    HotkeyLayout->setContentsMargins(0, 0, 0, 0);
    ui->hotkeysSettingArea->setLayout(HotkeyLayout);

    m_pAccesibilitySettingAreaWidget->LoadAccessibilitySettings();
    QVBoxLayout* AccessLayout = new QVBoxLayout(ui->accessibiltySettingArea);
    AccessLayout->addWidget(m_pAccesibilitySettingAreaWidget);
    AccessLayout->setContentsMargins(0, 0, 0, 0);
    ui->accessibiltySettingArea->setLayout(AccessLayout);

    m_pAdvancedSettingAreaWidget->AdvancedSettingAreaInit();
    QVBoxLayout* AdvancedLayout = new QVBoxLayout(ui->advancedSettingArea);
    AdvancedLayout->setContentsMargins(0, 0, 0, 0);
    AdvancedLayout->addWidget(m_pAdvancedSettingAreaWidget);
    ui->advancedSettingArea->setLayout(AdvancedLayout);
}

void AFQStudioSettingDialog::_SetTabButtons()
{
    ui->tabButtonProgram->SetButton("PROGRAM", 
        QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Program")));
    ui->tabButtonStream->SetButton("STREAM",
        QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Stream.Stream")));
    ui->tabButtonOutput->SetButton("OUTPUT",
        QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Output.Adv.Recording")));
    ui->tabButtonAudio->SetButton("AUDIO",
        QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Audio")));
    ui->tabButtonVideo->SetButton("VIDEO",
        QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Video")));
    ui->tabButtonHotkeys->SetButton("HOTKEYS",
        QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Hotkeys")));
    ui->tabButtonAccessibilty->SetButton("ACCESSIBILITY",
        QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Accessibility")));
    ui->tabButtonAdvanced->SetButton("ADVANCED",
        QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str("Basic.Settings.Advanced")));
}

void AFQStudioSettingDialog::_SetSettingDialogSignal()
{
    ui->buttonBox->button(QDialogButtonBox::Apply)->setFixedSize(128,40);
    
    _ApplyDisable();

    QList<AFQSettingTabButton*> buttonList = ui->widget_SettingTab->findChildren<AFQSettingTabButton*>();
    foreach(AFQSettingTabButton * button, buttonList)
    {
        connect(button, &AFQSettingTabButton::qsignalButtonClicked, this, &AFQStudioSettingDialog::qslotTabButtonToggled);
    }

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this,
            &AFQStudioSettingDialog::qslotButtonBoxClicked);
    ui->buttonBox->button(QDialogButtonBox::Reset)->setFixedSize(128, 40);
    connect(m_pResetButton, &QPushButton::clicked, this,
            &AFQStudioSettingDialog::qslotResetButtonClicked);

    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQStudioSettingDialog::qslotCloseSetting);
}

void AFQStudioSettingDialog::_SaveSettings()
{
    if(m_pProgramSettingAreaWidget)
        if (m_pProgramSettingAreaWidget->ProgramDataChanged())
        {
            m_pProgramSettingAreaWidget->SaveProgramSettings();
        }

    if (m_pOutputSettingAreaWidget)
        if (m_pOutputSettingAreaWidget->OutputDataChanged())
            m_pOutputSettingAreaWidget->SaveOutputSettings();

    if (m_pAudioSettingAreaWidget)
        if(m_pAudioSettingAreaWidget->AudioDataChanged())
            m_pAudioSettingAreaWidget->SaveAudioSettings();

    if (m_pVideoSettingAreaWidget)
        if(m_pVideoSettingAreaWidget->VideoDataChanged())
            m_pVideoSettingAreaWidget->SaveVideoSettings();

    if (m_pHotkeysSettingAreaWidget)
        if (m_pHotkeysSettingAreaWidget->HotkeysDataChanged())
            m_pHotkeysSettingAreaWidget->SaveHotkeysSettings();

    if (m_pAccesibilitySettingAreaWidget)
        if (m_pAccesibilitySettingAreaWidget->AccessibilityDataChanged())
            m_pAccesibilitySettingAreaWidget->SaveAccessibilitySettings();

    if (m_pAdvancedSettingAreaWidget)
        if (m_pAdvancedSettingAreaWidget->AdvancedDataChanged())
            m_pAdvancedSettingAreaWidget->SaveAdvancedSettings();

    if (m_pStreamSettingAreaWidget)
        if (m_eCurrentTabNum == TabType::STREAM)
            m_pStreamSettingAreaWidget->SaveStreamSettings();

    if (m_pOutputSettingAreaWidget->OutputDataChanged() ||
        m_pAudioSettingAreaWidget->AudioDataChanged() ||
        m_pVideoSettingAreaWidget->VideoDataChanged())
        App()->GetMainView()->ResetOutputs();

    if (m_pVideoSettingAreaWidget->VideoDataChanged() || 
        m_pAdvancedSettingAreaWidget->AdvancedDataChanged())
    {
        App()->GetMainView()->GetMainWindow()->GetVideoUtil()->ResetVideo();
    }

    config_save_safe(AFConfigManager::GetSingletonInstance().GetGlobal(), "tmp", nullptr);
    config_save_safe(AFConfigManager::GetSingletonInstance().GetBasic(), "tmp", nullptr);
    App()->GetMainView()->qslotSaveProject();

    auto& auth = AFAuthManager::GetSingletonInstance();
    auth.FlushAuthMain();
    auth.SaveAllAuthed();

    App()->GetMainView()->SetStreamingOutput();
    App()->GetMainView()->LoadAccounts();

    bool bLanguageChanged = m_pProgramSettingAreaWidget->CheckLanguageRestartRequired();
    bool bAudioRestart = m_pAudioSettingAreaWidget->CheckAudioRestartRequired();
    bool bHWAcceelChanged = m_pAdvancedSettingAreaWidget->CheckBrowserHardwareAccelerationRestartRequired();

    g_bRestart = bLanguageChanged || bAudioRestart || bHWAcceelChanged;
}

void AFQStudioSettingDialog::_ApplyDisable()
{
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect();
    effect->setOpacity(0.3);
    ui->buttonBox->button(QDialogButtonBox::Apply)->setGraphicsEffect(effect);
}

void AFQStudioSettingDialog::_ApplyEnable()
{
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
    QGraphicsOpacityEffect* opacityEffect = reinterpret_cast<QGraphicsOpacityEffect*>
        (ui->buttonBox->button(QDialogButtonBox::Apply)->graphicsEffect());
    opacityEffect->setOpacity(1);
}

void AFQStudioSettingDialog::_ClearChanged()
{
    if (m_pProgramSettingAreaWidget)
        m_pProgramSettingAreaWidget->SetProgramDataChangedVal(false);
    if(m_pStreamSettingAreaWidget)
        m_pStreamSettingAreaWidget->SetStreamDataChangedVal(false);
    if (m_pOutputSettingAreaWidget)
        m_pOutputSettingAreaWidget->SetOutputDataChangedVal(false);
    if (m_pAudioSettingAreaWidget)
        m_pAudioSettingAreaWidget->SetAudioDataChangedVal(false);
    if (m_pVideoSettingAreaWidget)
        m_pVideoSettingAreaWidget->SetVideoDataChangedVal(false);
    if (m_pHotkeysSettingAreaWidget)
        m_pHotkeysSettingAreaWidget->SetHotkeysDataChangedVal(false);
    if (m_pAccesibilitySettingAreaWidget)
        m_pAccesibilitySettingAreaWidget->SetAccessibilityDataChangedVal(false);
    if (m_pAdvancedSettingAreaWidget)
        m_pAdvancedSettingAreaWidget->SetAdvancedDataChangedVal(false);

    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

bool AFQStudioSettingDialog::_QueryChanges()
{
    int result = QDialog::Accepted;
    if (m_eCurrentTabNum != TabType::STREAM)
    {
        result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            this, QTStr("Basic.Settings.ConfirmTitle"),
            QTStr("Basic.Settings.Confirm"));
    }

    if (result == QDialog::Rejected) {
        _ReloadTabConfig();
        _ClearChanged();
        return false;
    }
    else if (result == QDialog::Accepted) {
        if (!_QueryAllowedToClose())
            return false;

        _SaveSettings();
        _ClearChanged();
    }
    else {
    }

    return true;
}

bool AFQStudioSettingDialog::_QueryAllowedToClose()
{
    bool simple = m_pOutputSettingAreaWidget->IsAdvancedMode() == false;

    bool invalidEncoder = false;
    bool invalidFormat = false;
    bool invalidTracks = false;
    bool invalidRecPath = false;

    if (simple) 
    {
        QString strSimpleVEncoder = m_pVideoSettingAreaWidget->GetSimpleVideoEncoder();
        QString strSimpleAEncoder = m_pAudioSettingAreaWidget->GetSimpleAudioEncoder();
        QString strSimpleRecVEncoder = m_pOutputSettingAreaWidget->GetSimpleVideoRecEncoder();
        QString strSimpleRecAEncoder = m_pOutputSettingAreaWidget->GetSimpleAudioRecEncoder();

        if (strSimpleRecVEncoder == "" ||
            strSimpleVEncoder == "" ||
            strSimpleRecAEncoder == "" ||
            strSimpleAEncoder == "")
            invalidEncoder = true;

        if (strSimpleRecVEncoder == "")
            invalidFormat = true;

        QString qual = m_pOutputSettingAreaWidget->GetSimpleRecQuality();
        QString format = m_pOutputSettingAreaWidget->GetSimpleRecFormat();
        if (m_pOutputSettingAreaWidget->qslotSimpleOutGetSelectedAudioTracks() == 0 &&
            qual != "Stream" && format != "flv")
            invalidTracks = true;
    
        if (m_pOutputSettingAreaWidget->GetSimpleOutputPath() == "")
            invalidRecPath = true;
    }
    else 
    {
        QString strAdvVEncoder = m_pVideoSettingAreaWidget->GetAdvVideoEncoder();
        QString strAdvAEncoder = m_pAudioSettingAreaWidget->GetAdvAudioEncoder();
        QString strAdvRecVEncoder = m_pOutputSettingAreaWidget->GetAdvVideoRecEncoder();
        QString strAdvRecAEncoder = m_pOutputSettingAreaWidget->GetAdvAudioRecEncoder();

        if (strAdvRecVEncoder == "" ||
            strAdvVEncoder == "" ||
            strAdvRecAEncoder == "" ||
            strAdvAEncoder == "")
            invalidEncoder = true;

        QString format = m_pOutputSettingAreaWidget->GetAdvRecFormat();
        if (m_pOutputSettingAreaWidget->qslotAdvOutGetSelectedAudioTracks() == 0 && format != "flv")
            invalidTracks = true;

        if (m_pOutputSettingAreaWidget->IsCustomFFmpeg())
        {
            if (m_pOutputSettingAreaWidget->GetAdvOutFFPath() == "")
                invalidRecPath = true;
        }
        else
        {
            if (m_pOutputSettingAreaWidget->GetAdvOutRecPath() == "")
                invalidRecPath = true;
        }
    }

    if (invalidEncoder) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
            this, "",
            QTStr("CodecCompat.CodecMissingOnExit.Text"));
        return false;
    }
    else if (invalidFormat) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
            this, "",
            QTStr("CodecCompat.ContainerMissingOnExit.Text"));
        return false;
    }
    else if (invalidTracks) {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
            this,
            "",
            QTStr("OutputWarnings.NoTracksSelectedOnExit.Text"));
        return false;
    }
    else if (invalidRecPath)
    {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
            this,
            "",
            QTStr("OutputWarnings.NoRecPathOnExit.Text"));
        return false;
    }

    return true;
}

QString AFQStudioSettingDialog::_GetTabName(TabType type) {
    std::string key;

    switch (type) {
        case TabType::PROGRAM:
            key = "Basic.Settings.Program";
            break;
        case TabType::STREAM:
            key = "Basic.Settings.Stream.Stream";
            break;
        case TabType::OUTPUT:
            key = "Basic.Settings.Output.Adv.Recording";
            break;
        case TabType::AUDIO:
            key = "Basic.Settings.Audio";
            break;
        case TabType::VIDEO:
            key = "Basic.Settings.Video";
            break;
        case TabType::HOTKEYS:
            key = "Basic.Settings.Hotkeys";
            break;
        case TabType::ACCESSIBILITY:
            key = "Basic.Settings.Accessibility";
            break;
        case TabType::ADVANCED:
            key = "Basic.Settings.Advanced";
            break;
    }

    return QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(key.data()));
}

bool AFQStudioSettingDialog::_AnyChanges() {
    if (m_pProgramSettingAreaWidget &&
        m_pProgramSettingAreaWidget->ProgramDataChanged())
        return true;
    if (m_pStreamSettingAreaWidget &&
        m_pStreamSettingAreaWidget->StreamDataChanged())
        return true;
    if (m_pOutputSettingAreaWidget &&
        m_pOutputSettingAreaWidget->OutputDataChanged())
        return true;
    if (m_pAudioSettingAreaWidget &&
        m_pAudioSettingAreaWidget->AudioDataChanged())
        return true;
    if (m_pVideoSettingAreaWidget &&
        m_pVideoSettingAreaWidget->VideoDataChanged())
        return true;
    if (m_pHotkeysSettingAreaWidget &&
        m_pHotkeysSettingAreaWidget->HotkeysDataChanged())
        return true;
    if (m_pAccesibilitySettingAreaWidget &&
        m_pAccesibilitySettingAreaWidget->AccessibilityDataChanged())
        return true;
    if (m_pAdvancedSettingAreaWidget &&
        m_pAdvancedSettingAreaWidget->AdvancedDataChanged())
        return true;

    return false;
}

void AFQStudioSettingDialog::_ReloadTabConfig() {
    int curTabIndex = ui->stackedWidget->currentIndex();
    TabType curTabType = static_cast<TabType>(curTabIndex);
    std::string key;

    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    uint32_t cx = primaryScreen->size().width();
    uint32_t cy = primaryScreen->size().height();

    switch (curTabType) {
        case TabType::PROGRAM:
            AFConfigManager::GetSingletonInstance().SetProgramConfig();
            m_pProgramSettingAreaWidget->LoadProgramSettings();
            break;
        case TabType::STREAM:
            AFConfigManager::GetSingletonInstance().SetStreamConfig();
            break;
        case TabType::OUTPUT:
            AFConfigManager::GetSingletonInstance().SetOutputConfig();
            m_pOutputSettingAreaWidget->ResetOutputSettings();
            break;
        case TabType::AUDIO:
            AFConfigManager::GetSingletonInstance().SetAudioConfig();
            m_pAudioSettingAreaWidget->ResetAudioSettings();
            break;
        case TabType::VIDEO:
            AFConfigManager::GetSingletonInstance().SetVideoConfig(cx, cy);
            m_pVideoSettingAreaWidget->ResetVideoSettings();
            break;
        case TabType::HOTKEYS:
            break;
        case TabType::ACCESSIBILITY:
            AFConfigManager::GetSingletonInstance().SetAccessibilityConfig();
            m_pAccesibilitySettingAreaWidget->LoadAccessibilitySettings();
            break;
        case TabType::ADVANCED:
            AFConfigManager::GetSingletonInstance().SetAdvancedConfig();
            m_pAdvancedSettingAreaWidget->LoadAdvancedSettings();
            break;
    }
}

void AFQStudioSettingDialog::_UpdateResetButtonVisible() {
    int curTabIndex = ui->stackedWidget->currentIndex();
    TabType curTabType = static_cast<TabType>(curTabIndex);
    switch (curTabType) {
        case TabType::STREAM:
            m_pResetButton->setVisible(false);
            break;
        default:
            m_pResetButton->setVisible(true);
            break;
    }
}
