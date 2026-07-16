#include "CStudioSettingDialog.h"
#include "ui_studio-setting-dialog.h"

#include <QMessageBox>
#include <QGraphicsOpacityEffect>

#include <obs-source.h>

#include "obs-properties.h"

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/Video/CVideo.h"

#include "CSettingTabButton.h"
#include "CSettingProgramAreaWidget.h"
#include "CSettingStreamAreaWidget.h"
#include "CSettingOutputAreaWidget.h"
#include "CSettingAudioAreaWidget.h"
#include "CSettingVideoAreaWidget.h"
#include "CSettingHotkeyAreaWidget.h"
#include "CSettingAccessibilityAreaWidget.h"
#include "CSettingAdvancedAreaWidget.h"
#include "UIComponent/CMessageBox.h"

#include "MainFrame/CMainFrame.h"
#include "MainFrame/Output/COutput.h"

AFQStudioSettingDialog::AFQStudioSettingDialog(QWidget *parent) :
    AFTTopBaseDialog(parent),
    ui(new Ui::AFQStudioSettingDialog)
{
    ui->setupUi(this);

#ifdef __APPLE__
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    setWindowTitle(QTStr("Settings"));
    ui->titleFrame->hide();
#endif
    
    SetWidthResizeEnabled(false);

    resetButton = ui->buttonBox->button(QDialogButtonBox::Reset);
    ChangeStyleSheet(resetButton, STYLESHEET_RESET_BUTTON);

    setAttribute(Qt::WA_DeleteOnClose);

    AFMainFrame* main = reinterpret_cast<AFMainFrame*>(parent);
    connect(main, &AFMainFrame::qsignalToggleUseVideo, this, &AFQStudioSettingDialog::ToggleStreamingUI);

   App()->DisableHotkeys();

   ToggleStreamingUI(false);
}

AFQStudioSettingDialog::~AFQStudioSettingDialog()
{
    App()->UpdateHotkeyFocusSetting();

    delete ui;
}

void AFQStudioSettingDialog::CloseSetting()
{
    if (AnyChanges())
        if (QueryChanges() == false)
            return;

    if (programWidget)
    {
        programWidget->SaveProgramPageSpreadState();
        config_save_safe(APPCONFIG, "tmp", nullptr);
    }

    close();
}

void AFQStudioSettingDialog::LogoutKR()
{
    g_bRestart = true;
    MAINFRAME->IsRestartConfirmationNeeded(true);

    close();
}

void AFQStudioSettingDialog::ToggleTabButton()
{
    videoSizeValid = true;
	 
#if 0
    if (_AnyChanges())
        _QueryChanges(true);

    AFQSettingTabButton* checkedButton = reinterpret_cast<AFQSettingTabButton*>(sender());
    if (m_currentTabNum == (int)TabType::VIDEO && !videoSizeValid) {
        checkedButton->setChecked(false);
        return;
    }
#else
    bool validChanges = true;
    if (AnyChanges())
        validChanges = QueryChanges(true);

    AFQSettingTabButton* checkedButton = reinterpret_cast<AFQSettingTabButton*>(sender());
    if (!validChanges) {
        checkedButton->setChecked(false);
        return;
    }
#endif

    QList<AFQSettingTabButton*> buttonList = ui->tabWidget->findChildren<AFQSettingTabButton*>();
    foreach(AFQSettingTabButton * button, buttonList)
        if (button != checkedButton)
            button->setChecked(false);

    ui->buttonBox->button(QDialogButtonBox::Cancel)->show();
    ui->buttonBox->button(QDialogButtonBox::Apply)->show();

    QMetaEnum tabTypeEnum = QMetaEnum::fromType<TabType>();
    int tabType = tabTypeEnum.keyToValue(checkedButton->GetButtonType());
    switch (tabType)
    {
    case TabType::PROGRAM:
    {
        programWidget->LoadMainAccount();
        break;
    }
    case TabType::STREAM:
    {
        streamWidget->LoadStreamAccountSaved();
         break;
    }
    case TabType::OUTPUT:
    {
        QString strAdvVEncoder = videoWidget->GetAdvVideoEncoder();
        QString strAdvAEncoder = audioWidget->GetAdvAudioEncoder();

        outputWidget->AdvOutEncoderData(strAdvVEncoder, strAdvAEncoder);
        break;
    }
    }
    ui->stackedWidget->setCurrentIndex(tabType);
    activeTabType = tabType;
    UpdateResetButtonVisible();
}

void AFQStudioSettingDialog::ResetButtonClicked()
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

    int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                                            QTStr("Reset"), QTStr(resetLocaleKey.c_str()));
    if (result == QDialog::Rejected) {
        return;
    }
    else if (result == QDialog::Accepted) {
        QScreen* primaryScreen = QGuiApplication::primaryScreen();
        uint32_t cx = primaryScreen->size().width();
        uint32_t cy = primaryScreen->size().height();

        switch (curTabType) {
            case TabType::PROGRAM:
                programWidget->ResetProgramSettings();
                break;
            case TabType::STREAM:
                break;
            case TabType::OUTPUT:
            case TabType::AUDIO:
            case TabType::VIDEO:
                CONFIG_CONTEXT.ResetAVOutputConfig(cx, cy);
                outputWidget->ResetOutputSettings();
                audioWidget->ResetAudioSettings();
                videoWidget->ResetVideoSettings();
                break;
            case TabType::HOTKEYS:
                hotkeyWidget->ClearHotkeyValues();
                hotkeyWidget->SaveHotkeysSettings();
                App()->UpdateHotkeyFocusSetting(true);
                hotkeyWidget->UpdateFocusBehaviorComboBox();
                break;
            case TabType::ACCESSIBILITY:
                CONFIG_CONTEXT.ResetAccessibilityConfig();
                accessWidget->LoadAccessibilitySettings();
                break;
            case TabType::ADVANCED:
                CONFIG_CONTEXT.ResetAdvancedConfig();
                advanceWidget->LoadAdvancedSettings();
                break;
        }

        AFVideoUtil::ResetVideo();

        ClearChanged();
        ApplyDisable();

        bool langChanged = programWidget->CheckLanguageRestartRequired();
        bool audioRestart = audioWidget->CheckAudioRestartRequired();
        bool hwAcceelChanged = advanceWidget->CheckBrowserHardwareAccelerationRestartRequired();

        g_bRestart = langChanged || audioRestart || hwAcceelChanged;
    }
}

void AFQStudioSettingDialog::ResetDownScales(int cx, int cy)
{
    uint32_t outCx = videoWidget->GetVideoOutputCx();
    uint32_t outCy = videoWidget->GetVideoOutputCy();
    QComboBox* outRescaleCombobox = videoWidget->GetOutResolutionComboBox();
    
    outputWidget->OutputResolution(outCx, outCy);
    //outputWidget->ResetDownscales(cx, cy, true);
    outputWidget->RefreshDownscales(cx, cy, outRescaleCombobox);
}

void AFQStudioSettingDialog::ChangeSettingPageData()
{
    ApplyEnable();
}

void AFQStudioSettingDialog::ChangeSimpleMode()
{
    if (audioWidget == nullptr ||
        videoWidget == nullptr ||
        outputWidget == nullptr)
        return;

    audioWidget->ChangeSettingModeToSimple();
    videoWidget->ChangeSettingModeToSimple();
    outputWidget->ChangeSettingModeToSimple();

    audioWidget->SetAudioDataChangedVal(true);
    videoWidget->SetVideoDataChanged(true);
    outputWidget->SetOutputDataChangedVal(true);

    MAIN_OUTPUT->ResetOutputs();
}

void AFQStudioSettingDialog::ChangeAdvanceMode()
{
    if (audioWidget == nullptr ||
        videoWidget == nullptr ||
        outputWidget == nullptr)
        return;

    audioWidget->ChangeSettingModeToAdvanced();
    videoWidget->ChangeSettingModeToAdvanced();
    outputWidget->ChangeSettingModeToAdvanced();

    audioWidget->SetAudioDataChangedVal(true);
    videoWidget->SetVideoDataChanged(true);
    outputWidget->SetOutputDataChangedVal(true);
    
    MAIN_OUTPUT->ResetOutputs();
}

void AFQStudioSettingDialog::ChangeSimpleReplayBuffer()
{
    if (outputWidget == nullptr)
        return;

    outputWidget->qslotSimpleReplayBufferChanged();
}

void AFQStudioSettingDialog::ChangeAdvanceReplayBuffer()
{
    if (outputWidget == nullptr)
        return;

    outputWidget->qslotAdvReplayBufferChanged();
}

void AFQStudioSettingDialog::ChangeSimpleOutVEncoder(QString vEncoder)
{
    if (outputWidget == nullptr)
        return;

    outputWidget->SimpleOutVEncoder(vEncoder);
}

void AFQStudioSettingDialog::ChangeSimpleOutAEncoder(QString aEncoder)
{
    if (outputWidget == nullptr)
        return;

    outputWidget->SimpleOutAEncoder(aEncoder);
}

void AFQStudioSettingDialog::ChangeSimpleOutVBitrate(int vBitrate)
{
    if (outputWidget == nullptr)
        return;

    outputWidget->SimpleOutVBitrate(vBitrate);
}

void AFQStudioSettingDialog::ChangeSimpleOutABitrate(int aBitrate)
{
    if (outputWidget == nullptr)
        return;

    outputWidget->SimpleOutABitrate(aBitrate);
}
void AFQStudioSettingDialog::ChangeImpleRecordingEncoder()
{
    if (outputWidget == nullptr)
        return;

    outputWidget->qslotSimpleRecordingEncoderChanged();
}

void AFQStudioSettingDialog::ChangeStreamEncoderProps()
{
    if (videoWidget == nullptr ||
        outputWidget == nullptr)
        return;
    
   int vbitrate = videoWidget->VideoAdvBitrate();
   const char* rateControl = videoWidget->VideoAdvRateControl();

    if (vbitrate == 0 && rateControl == "")
    {
        outputWidget->HasStreamEncoder(false);
    }
    else
    {
        outputWidget->HasStreamEncoder(true);
        outputWidget->StreamEncoderData(vbitrate, rateControl);
    }

    outputWidget->qslotUpdateStreamDelayEstimate();
    outputWidget->qslotAdvReplayBufferChanged();
}

void AFQStudioSettingDialog::UpdateStreamDelayEstimate()
{
    if (outputWidget == nullptr)
        return;

    outputWidget->qslotUpdateStreamDelayEstimate();
}

void AFQStudioSettingDialog::ButtonBoxClicked(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);
    bool saved = true;

    if (val == QDialogButtonBox::ApplyRole || val == QDialogButtonBox::AcceptRole)
    {
        if (!QueryAllowedToClose())
            return;

        saved = SaveSettings();

        //UpdateYouTubeAppDockSettings();
        if (saved)
        {
            ClearChanged();
            ApplyDisable();

            bool langChanged = programWidget->CheckLanguageRestartRequired();
            if (langChanged) {
                int result = AFQMessageBox::ShowMessageWithButtonText(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                                                                      QTStr(""), QTStr("LanguageChange.Restart"), QTStr("Restart"));

                if (result == QDialog::Accepted) 
                {
                    g_bRestart = true;
                    MAINFRAME->IsRestartConfirmationNeeded(true);
                    CloseSetting();
                }
            }
        }
        else
        {
            return;
        }
    }

    if (val == QDialogButtonBox::AcceptRole || val == QDialogButtonBox::RejectRole) 
    {
        CloseSetting();
    }
}

void AFQStudioSettingDialog::SetAutoRemuxText(QString autoRemuxText)
{
    if (!advanceWidget)
        return;
    advanceWidget->SetAutoRemuxText(autoRemuxText);
}

void AFQStudioSettingDialog::ToggleStreamingUI(bool streaming)
{
    if (programWidget)
        programWidget->ToggleOnStreaming(streaming);

    if (streamWidget)
        streamWidget->ToggleOnStreaming(streaming);

    if (outputWidget)
        outputWidget->ToggleOnStreaming(streaming);

    if (audioWidget)
        audioWidget->ToggleOnStreaming(streaming);

    if (videoWidget)
        videoWidget->ToggleOnStreaming(streaming);

    if (advanceWidget)
        advanceWidget->ToggleOnStreaming(streaming);

    bool useVideo = obs_video_active() ? false : true;
    ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(useVideo);
}

void AFQStudioSettingDialog::AFQStudioSettingDialogInit(int type)
{
    ui->titleLabel->setText(QTStr("Settings"));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QTStr("OK"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QTStr("Cancel"));
    ui->buttonBox->button(QDialogButtonBox::Apply)->setText(QTStr("Apply"));
    ui->buttonBox->button(QDialogButtonBox::Reset)->setText(QTStr("Reset"));

    SetTabButtons();
    SetSettingAreaWidget();
    SetSettingDialogSignal();

    switch (type)
    {
    case 0:
        ui->programButton->clicked();
        break;
    case 1:
        ui->streamButton->clicked();
        break;
    case 2:
        ui->outputButton->clicked();
        break;
    case 3:
        ui->audioButton->clicked();
        break;
    case 4:
        ui->videoButton->clicked();
        break;
    case 5:
        ui->hotkeyButton->clicked();
        break;
    case 6:
        ui->accessButton->clicked();
        break;
    case 7:
        ui->advanceButton->clicked();
        break;
    }

    if (type == 5) // Call From OverlayWidget
    {
        if (hotkeyWidget != nullptr)
            hotkeyWidget->SetVerticalScrollBarPosition();
    }
}

QString AFQStudioSettingDialog::GetCurrentAudioBitrate()
{
    if (audioWidget != nullptr)
    {
        return audioWidget->CurrentAudioBitRate();
    }
    return "";
}

void AFQStudioSettingDialog::SetID(QString id, QString platform)
{
    if (streamWidget != nullptr)
    {
        streamWidget->FindAccountButtonWithID(id, platform);
    }
}

#if 0
void AFQStudioSettingDialog::reject() 
{
    if (_AnyChanges()) {
        _QueryChanges();
    }
    QDialog::reject();
}
#endif

void AFQStudioSettingDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Enter || 
        event->key() == Qt::Key_Escape ||
        event->key() == Qt::Key_Space) {
        event->ignore();
    }
    else {
        QWidget::keyPressEvent(event);
    }
}

void AFQStudioSettingDialog::showEvent(QShowEvent* event)
{
    if (MAINFRAME->IsSmallResolution())
    {
        resize(800, 550);
        setFixedWidth(800);
    }
    else
    {    
        resize(940, 760);
        setFixedWidth(940);
    }

    setMinimumHeight(550);
    int posX = MAINFRAME->x() + (MAINFRAME->width() / 2) - (this->width() / 2);
    int posY = MAINFRAME->y() + (MAINFRAME->height() / 2) - (this->height() / 2);

    move(posX, posY);

    EnsureDialogVisible(this);

}


void AFQStudioSettingDialog::SetSettingAreaWidget() {

    // Create Widgets (0. Program ~ 7. Advanced)
    if (!programWidget)
        programWidget = new AFQProgramSettingAreaWidget(this);

    if (!streamWidget)
        streamWidget = new AFQStreamSettingAreaWidget(this);

    if (!outputWidget)
        outputWidget = new AFQOutputSettingAreaWidget(this);

    if (!audioWidget)
        audioWidget = new AFQAudioSettingAreaWidget(this);

    if (!videoWidget)
        videoWidget = new AFQVideoSettingAreaWidget(this);

    if (!hotkeyWidget)
        hotkeyWidget = new AFQHotkeySettingAreaWidget(this);

    if (!accessWidget)
        accessWidget = new AFQAccessibilitySettingAreaWidget(this);

    if (!advanceWidget)
        advanceWidget = new AFQAdvancedSettingAreaWidget(this);

    // Connect Signals (0. Program ~ 7. Advanced)
    connect(programWidget, &AFQProgramSettingAreaWidget::qsignalProgramDataChanged, 
            this, &AFQStudioSettingDialog::ChangeSettingPageData);
    connect(programWidget, &AFQProgramSettingAreaWidget::qsignalLogoutKR,
            this, &AFQStudioSettingDialog::LogoutKR);

    connect(streamWidget, &AFQStreamSettingAreaWidget::qsignalStreamDataChanged,
            this, &AFQStudioSettingDialog::ChangeSettingPageData);

    connect(outputWidget, &AFQOutputSettingAreaWidget::qsignalUpdateReplayBufferStream,
            programWidget, &AFQProgramSettingAreaWidget::UpdateAutomaticReplayBufferCheckboxes);
    connect(outputWidget, &AFQOutputSettingAreaWidget::qsignalSimpleModeClicked,
            this, &AFQStudioSettingDialog::ChangeSimpleMode);
    connect(outputWidget, &AFQOutputSettingAreaWidget::qsignalAdvancedModeClicked,
            this, &AFQStudioSettingDialog::ChangeAdvanceMode);
    connect(outputWidget, &AFQOutputSettingAreaWidget::qsignalOutputDataChanged,
            this, &AFQStudioSettingDialog::ChangeSettingPageData);
    connect(outputWidget, &AFQOutputSettingAreaWidget::qsignalSetAutoRemuxText,
            this, &AFQStudioSettingDialog::SetAutoRemuxText);

    connect(audioWidget, &AFQAudioSettingAreaWidget::qsignalCallSimpleReplayBufferChanged,
            this, &AFQStudioSettingDialog::ChangeSimpleReplayBuffer);
    connect(audioWidget, &AFQAudioSettingAreaWidget::qsignalCallSimpleRecordingEncoderChanged,
            this, &AFQStudioSettingDialog::ChangeImpleRecordingEncoder);
    connect(audioWidget, &AFQAudioSettingAreaWidget::qsignalCallUpdateStreamDelayEstimate,
            this, &AFQStudioSettingDialog::UpdateStreamDelayEstimate);
    connect(audioWidget, &AFQAudioSettingAreaWidget::qsignalSimpleModeClicked,
            this, &AFQStudioSettingDialog::ChangeSimpleMode);
    connect(audioWidget, &AFQAudioSettingAreaWidget::qsignalAdvancedModeClicked,
            this, &AFQStudioSettingDialog::ChangeAdvanceMode);
    connect(audioWidget, &AFQAudioSettingAreaWidget::qsignalSimpleEncoderChanged,
            this, &AFQStudioSettingDialog::ChangeSimpleOutAEncoder);
    connect(audioWidget, &AFQAudioSettingAreaWidget::qsignalSimpleBitrateChanged,
            this, &AFQStudioSettingDialog::ChangeSimpleOutABitrate);
    connect(audioWidget, &AFQAudioSettingAreaWidget::qsignalAudioDataChanged,
            this, &AFQStudioSettingDialog::ChangeSettingPageData);

    connect(videoWidget, &AFQVideoSettingAreaWidget::qsignalBaseResolutionChanged,
            this, &AFQStudioSettingDialog::ResetDownScales);
    connect(videoWidget, &AFQVideoSettingAreaWidget::qsignalCallSimpleReplayBufferChanged,
            this, &AFQStudioSettingDialog::ChangeSimpleReplayBuffer);
    connect(videoWidget, &AFQVideoSettingAreaWidget::qsignalCallSimpleRecordingEncoderChanged,
            this, &AFQStudioSettingDialog::ChangeImpleRecordingEncoder);
    connect(videoWidget, &AFQVideoSettingAreaWidget::qsignalCallStreamEncoderPropChanged,
            this, &AFQStudioSettingDialog::ChangeStreamEncoderProps);
    connect(videoWidget, &AFQVideoSettingAreaWidget::qsignalCallUpdateStreamDelayEstimate,
            this, &AFQStudioSettingDialog::UpdateStreamDelayEstimate);
    connect(videoWidget, &AFQVideoSettingAreaWidget::qsignalSimpleModeClicked,
            this, &AFQStudioSettingDialog::ChangeSimpleMode);
    connect(videoWidget, &AFQVideoSettingAreaWidget::qsignalAdvancedModeClicked,
            this, &AFQStudioSettingDialog::ChangeAdvanceMode);
    connect(videoWidget, &AFQVideoSettingAreaWidget::qsignalSimpleEncoderChanged,
            this, &AFQStudioSettingDialog::ChangeSimpleOutVEncoder);
    connect(videoWidget, &AFQVideoSettingAreaWidget::qsignalSimpleBitrateChanged,
            this,  &AFQStudioSettingDialog::ChangeSimpleOutVBitrate);
    connect(videoWidget, &AFQVideoSettingAreaWidget::qsignalVideoDataChanged,
            this, &AFQStudioSettingDialog::ChangeSettingPageData);

    connect(hotkeyWidget, &AFQHotkeySettingAreaWidget::qsignalHotkeyChanged,
            this, &AFQStudioSettingDialog::ChangeSettingPageData);

    connect(accessWidget, &AFQAccessibilitySettingAreaWidget::qsignalA11yDataChanged,
            this, &AFQStudioSettingDialog::ChangeSettingPageData);

    connect(advanceWidget, &AFQAdvancedSettingAreaWidget::qsignalCallOutputSettingUpdateStreamDelayEstimate,
            this, &AFQStudioSettingDialog::UpdateStreamDelayEstimate);
    connect(advanceWidget, &AFQAdvancedSettingAreaWidget::qsignalAdvancedDataChanged,
            this, &AFQStudioSettingDialog::ChangeSettingPageData);

    // Load
    programWidget->ProgramSettingAreaInit();
    QVBoxLayout* ProgramLayout = new QVBoxLayout(ui->programPage);
    ProgramLayout->setContentsMargins(0, 0, 0, 0);
    ProgramLayout->addWidget(programWidget);
    ui->programPage->setLayout(ProgramLayout);

    streamWidget->StreamSettingAreaInit();
    QVBoxLayout* StreamLayout = new QVBoxLayout(ui->streamPage);
    StreamLayout->setContentsMargins(0, 0, 0, 0);
    StreamLayout->addWidget(streamWidget);
    ui->streamPage->setLayout(StreamLayout);

    outputWidget->LoadOutputSettings();
    QVBoxLayout* OutputLayout = new QVBoxLayout(ui->outputPage);
    OutputLayout->addWidget(outputWidget);
    OutputLayout->setContentsMargins(0, 0, 0, 0);
    ui->outputPage->setLayout(OutputLayout);

    audioWidget->LoadAudioSettings();
    QVBoxLayout* vLayoutAudio = new QVBoxLayout(ui->audioPage);
    vLayoutAudio->addWidget(audioWidget);
    vLayoutAudio->setContentsMargins(0, 0, 0, 0);
    ui->audioPage->setLayout(vLayoutAudio);

    videoWidget->LoadVideoSettings();
    QVBoxLayout* vLayoutVideo = new QVBoxLayout(ui->videoPage);
    vLayoutVideo->addWidget(videoWidget);
    vLayoutVideo->setContentsMargins(0, 0, 0, 0);
    ui->videoPage->setLayout(vLayoutVideo);

    hotkeyWidget->LoadHotkeysSettings();
    QVBoxLayout* HotkeyLayout = new QVBoxLayout(ui->hotkeyPage);
    HotkeyLayout->addWidget(hotkeyWidget);
    HotkeyLayout->setContentsMargins(0, 0, 0, 0);
    ui->hotkeyPage->setLayout(HotkeyLayout);

    accessWidget->LoadAccessibilitySettings();
    QVBoxLayout* AccessLayout = new QVBoxLayout(ui->accessPage);
    AccessLayout->addWidget(accessWidget);
    AccessLayout->setContentsMargins(0, 0, 0, 0);
    ui->accessPage->setLayout(AccessLayout);

    advanceWidget->AdvancedSettingAreaInit();
    QVBoxLayout* AdvancedLayout = new QVBoxLayout(ui->advancePage);
    AdvancedLayout->setContentsMargins(0, 0, 0, 0);
    AdvancedLayout->addWidget(advanceWidget);
    ui->advancePage->setLayout(AdvancedLayout);
    //
}

void AFQStudioSettingDialog::SetTabButtons()
{
    if (MAINFRAME->IsSmallResolution())
    {
        ui->programButton->setFixedSize(100, 84);
        ui->streamButton->setFixedSize(100, 84);
        ui->outputButton->setFixedSize(100, 84);
        ui->audioButton->setFixedSize(100, 84);
        ui->videoButton->setFixedSize(100, 84);
        ui->hotkeyButton->setFixedSize(100, 84);
        ui->accessButton->setFixedSize(100, 84);
        ui->advanceButton->setFixedSize(100, 84);
    }

    ui->programButton->SetButton("PROGRAM", QTStr("Basic.Settings.Program"));
    ui->streamButton->SetButton("STREAM", QTStr("Basic.Settings.Stream.Stream"));
    ui->outputButton->SetButton("OUTPUT", QTStr("Basic.Settings.Output.Adv.Recording"));
    ui->audioButton->SetButton("AUDIO", QTStr("Basic.Settings.Audio"));
    ui->videoButton->SetButton("VIDEO", QTStr("Basic.Settings.Video"));
    ui->hotkeyButton->SetButton("HOTKEYS", QTStr("Basic.Settings.Hotkeys"));
    ui->accessButton->SetButton("ACCESSIBILITY", QTStr("Basic.Settings.Accessibility"));
    ui->advanceButton->SetButton("ADVANCED", QTStr("Basic.Settings.Advanced"));
}

void AFQStudioSettingDialog::SetSettingDialogSignal()
{
    ui->buttonBox->button(QDialogButtonBox::Apply)->setFixedSize(128,40);
    
    ApplyDisable();

    QList<AFQSettingTabButton*> buttonLists = ui->tabWidget->findChildren<AFQSettingTabButton*>();
    foreach(AFQSettingTabButton * button, buttonLists)
    {
        connect(button, &AFQSettingTabButton::ButtonClicked, this, &AFQStudioSettingDialog::ToggleTabButton);
    }

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &AFQStudioSettingDialog::ButtonBoxClicked);
    connect(resetButton, &QPushButton::clicked, this, &AFQStudioSettingDialog::ResetButtonClicked);
    connect(ui->closeButton, &QPushButton::clicked, this, &AFQStudioSettingDialog::CloseSetting);

    ui->buttonBox->button(QDialogButtonBox::Reset)->setFixedSize(128, 40);
}

bool AFQStudioSettingDialog::SaveSettings()
{
    bool retVal = true;

    if(programWidget)
        if (programWidget->ProgramDataChanged())
        {
            programWidget->SaveProgramSettings();

            //System Tray Disable
            /*if (programWidget->CheckSystemTrayToggle())
            {
                m_pMainFrame->SystemTray(false);
            }*/
        }

    if (outputWidget)
            outputWidget->SaveOutputSettings();

    if (audioWidget)
            audioWidget->SaveAudioSettings();

    if (videoWidget)
            videoWidget->SaveVideoSettings();

    if (hotkeyWidget)
            hotkeyWidget->SaveHotkeysSettings();

    if (accessWidget)
            accessWidget->SaveAccessibilitySettings();

    if (advanceWidget)
            advanceWidget->SaveAdvancedSettings();

    if (streamWidget)
        if (activeTabType == TabType::STREAM)
            streamWidget->SaveStreamSettings();

    // KR Broad Info
    if (streamWidget && streamWidget->BroadDataChanged())
        streamWidget->SaveBroadInfoSettings();
    
    
    //Save Project
    //main->SaveProject();

    //log
    /*if (Changed()) {
        std::string changed;
        if (generalChanged)
            AddChangedVal(changed, "general");
        if (stream1Changed)
            AddChangedVal(changed, "stream 1");
        if (outputsChanged)
            AddChangedVal(changed, "outputs");
        if (audioChanged)
            AddChangedVal(changed, "audio");
        if (videoChanged)
            AddChangedVal(changed, "video");
        if (hotkeysChanged)
            AddChangedVal(changed, "hotkeys");
        if (a11yChanged)
            AddChangedVal(changed, "a11y");
        if (advancedChanged)
            AddChangedVal(changed, "advanced");

        blog(LOG_INFO, "Settings changed (%s)", changed.c_str());
        blog(LOG_INFO, MINOR_SEPARATOR);
    }*/

    if (outputWidget->OutputDataChanged() ||
        audioWidget->AudioDataChanged() ||
        videoWidget->VideoDataChanged())
        MAIN_OUTPUT->ResetOutputs();

    if (videoWidget->VideoDataChanged() || 
        advanceWidget->AdvancedDataChanged())
    {
        AFVideoUtil::ResetVideo();
    }

    // Save Config
    config_save_safe(ACTIVECONFIG, "tmp", nullptr);
    config_save_safe(USERCONFIG, "tmp", nullptr);
    MAINFRAME->qslotSaveProject();

    auto& auth = AUTH_CONTEXT;
    //auth.FlushAuthCache();
    auth.SaveAllAuthed();

    MAIN_OUTPUT->SetStreamingOutput();
    MAINFRAME->LoadAccounts();

    bool bLanguageChanged = programWidget->CheckLanguageRestartRequired();
    bool bAudioRestart = audioWidget->CheckAudioRestartRequired();
    bool bHWAcceelChanged = advanceWidget->CheckBrowserHardwareAccelerationRestartRequired();

    g_bRestart = bLanguageChanged || bAudioRestart || bHWAcceelChanged;

    return retVal;
}

void AFQStudioSettingDialog::SaveStreamSettings()
{
    if (streamWidget && activeTabType == TabType::STREAM)
        streamWidget->SaveStreamSettings();

    AUTH_CONTEXT.SaveAllAuthed();
}

void AFQStudioSettingDialog::ApplyDisable()
{
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void AFQStudioSettingDialog::ApplyEnable()
{
    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
}

void AFQStudioSettingDialog::ClearChanged()
{
    if (programWidget)
        programWidget->SetProgramDataChangedVal(false);
    if (streamWidget) {
        streamWidget->SetStreamDataChangedVal(false);
        streamWidget->SetBroadDataChangedVal(false);
    }
    if (outputWidget)
        outputWidget->SetOutputDataChangedVal(false);
    if (audioWidget)
        audioWidget->SetAudioDataChangedVal(false);
    if (videoWidget)
        videoWidget->SetVideoDataChangedVal(false);
    if (hotkeyWidget)
        hotkeyWidget->SetHotkeysDataChangedVal(false);
    if (accessWidget)
        accessWidget->SetAccessibilityDataChangedVal(false);
    if (advanceWidget)
        advanceWidget->SetAdvancedDataChangedVal(false);

    ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

bool AFQStudioSettingDialog::QueryChanges(bool isTriggeredByTabChange)
{
    int result = QDialog::Accepted;

#if 1
    SaveStreamSettings();

    if (isTriggeredByTabChange) {
        if (!QueryAllowedToClose())
            return false;
        return true;
    }

    if (ui->buttonBox->button(QDialogButtonBox::Apply)->isEnabled())
    {
        result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                                            QTStr("Basic.Settings.ConfirmTitle"), QTStr("Basic.Settings.Confirm"));
    }

#else
    if (m_currentTabNum != TabType::STREAM)
    {
        result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                                            QTStr("Basic.Settings.ConfirmTitle"), QTStr("Basic.Settings.Confirm"));
    }
    else 
    {
        // KR
        if (MAINFRAME->IsGlobal() == false)
        {
            if (streamWidget &&
                streamWidget->BroadDataChanged()) 
            {
                result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this,
                                                    QTStr("Basic.Settings.ConfirmTitle"), QTStr("Basic.Settings.Confirm"));

                // Save Stream Settings
                if (result == QDialog::Rejected) 
                    _SaveStreamSettings();
            }
        }
    }
#endif

    if (result == QDialog::Rejected) {
        ReloadTabConfig();
        ClearChanged();
        return true;
    }
    else if (result == QDialog::Accepted) {
        if (!QueryAllowedToClose())
            return false;

        if (!SaveSettings())
            return false;
        ClearChanged();
    }
    else {
        //if (savedTheme != App()->GetTheme())
        //    App()->SetTheme(savedTheme);

        //LoadSettings(true);
        //restart = false;
    }

    return true;
}

bool AFQStudioSettingDialog::QueryAllowedToClose()
{
    bool simple = outputWidget->IsAdvancedMode() == false;

    bool invalidEncoder = false;
    bool invalidFormat = false;
    bool invalidTracks = false;
    bool invalidRecPath = false;
    bool invalidOutputResolution = false;

    if (simple) 
    {
        QString strSimpleVEncoder = videoWidget->GetSimpleVideoEncoder();
        QString strSimpleAEncoder = audioWidget->GetSimpleAudioEncoder();
        QString strSimpleRecVEncoder = outputWidget->GetSimpleVideoRecEncoder();
        QString strSimpleRecAEncoder = outputWidget->GetSimpleAudioRecEncoder();

        if (strSimpleRecVEncoder == "" || strSimpleVEncoder == "" ||
            strSimpleRecAEncoder == "" || strSimpleAEncoder == "")
            invalidEncoder = true;

        if (strSimpleRecVEncoder == "")
            invalidFormat = true;

        QString qual = outputWidget->GetSimpleRecQuality();
        QString format = outputWidget->GetSimpleRecFormat();
        if (outputWidget->qslotSimpleOutGetSelectedAudioTracks() == 0 && qual != "Stream" && format != "flv")
            invalidTracks = true;
    
        if (outputWidget->GetSimpleOutputPath() == "")
            invalidRecPath = true;
    }
    else 
    {
        QString strAdvVEncoder = videoWidget->GetAdvVideoEncoder();
        QString strAdvAEncoder = audioWidget->GetAdvAudioEncoder();
        QString strAdvRecVEncoder = outputWidget->GetAdvVideoRecEncoder();
        QString strAdvRecAEncoder = outputWidget->GetAdvAudioRecEncoder();

        if (strAdvRecVEncoder == "" || strAdvVEncoder == "" ||
            strAdvRecAEncoder == "" || strAdvAEncoder == "")
            invalidEncoder = true;

        QString format = outputWidget->GetAdvRecFormat();
        if (outputWidget->qslotAdvOutGetSelectedAudioTracks() == 0 && format != "flv")
            invalidTracks = true;

        if (outputWidget->IsCustomFFmpeg())
        {
            if (outputWidget->GetAdvOutFFPath() == "")
                invalidRecPath = true;
        }
        else
        {
            if (outputWidget->GetAdvOutRecPath() == "")
                invalidRecPath = true;
        }
    }

    // Check Video Output Resolution
    videoSizeValid = videoWidget->IsValidAspectRatios();
    invalidOutputResolution = !videoSizeValid;

    if (invalidEncoder) {
        // Warning
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   //QTStr("CodecCompat.CodecMissingOnExit.Title"),
                                   "", QTStr("CodecCompat.CodecMissingOnExit.Text"));
        return false;
    }
    else if (invalidFormat) {
        // Warning
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   //QTStr("CodecCompat.ContainerMissingOnExit.Title"),
                                   "", QTStr("CodecCompat.ContainerMissingOnExit.Text"));
        return false;
    }
    else if (invalidTracks) {
        // Warning
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   //QTStr("OutputWarnings.NoTracksSelectedOnExit.Title"),
                                   "", QTStr("OutputWarnings.NoTracksSelectedOnExit.Text"));
        return false;
    }
    else if (invalidRecPath)
    {
        // Warning
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   //QTStr("OutputWarnings.NoTracksSelectedOnExit.Title"),
                                   "", QTStr("OutputWarnings.NoRecPathOnExit.Text"));
        return false;
    }
    else if (invalidOutputResolution)
    {
        // Warning
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
                                   "", QTStr("Basic.Settings.Video.InvalidResolutionRatio"));

        videoWidget->HighLightResolution();
        return false;
    }

    return true;
}

QString AFQStudioSettingDialog::GetTabName(TabType type) {
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

    return QTStr(key.data());
}

bool AFQStudioSettingDialog::AnyChanges() {
    if (programWidget && programWidget->ProgramDataChanged())
        return true;
    if (streamWidget && streamWidget->StreamDataChanged())
        return true;
    if (streamWidget && streamWidget->BroadDataChanged())
        return true;
    if (outputWidget && outputWidget->OutputDataChanged())
        return true;
    if (audioWidget && audioWidget->AudioDataChanged())
        return true;
    if (videoWidget && videoWidget->VideoDataChanged())
        return true;
    if (hotkeyWidget && hotkeyWidget->HotkeysDataChanged())
        return true;
    if (accessWidget && accessWidget->AccessibilityDataChanged())
        return true;
    if (advanceWidget && advanceWidget->AdvancedDataChanged())
        return true;

    return false;
}

void AFQStudioSettingDialog::ReloadTabConfig() {
    int curTabIndex = ui->stackedWidget->currentIndex();
    TabType curTabType = static_cast<TabType>(curTabIndex);
    std::string key;

    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    uint32_t cx = primaryScreen->size().width();
    uint32_t cy = primaryScreen->size().height();

    switch (curTabType) {
        case TabType::PROGRAM:
            CONFIG_CONTEXT.SetProgramConfig();
            programWidget->LoadProgramSettings();
            break;
        case TabType::STREAM:
            CONFIG_CONTEXT.SetStreamConfig();
            streamWidget->ReloadBroadInfo();
            break;
        case TabType::OUTPUT:
            CONFIG_CONTEXT.SetOutputConfig();
            outputWidget->ResetOutputSettings();
            break;
        case TabType::AUDIO:
            CONFIG_CONTEXT.SetAudioConfig();
            audioWidget->ResetAudioSettings();
            break;
        case TabType::VIDEO:
            CONFIG_CONTEXT.SetVideoConfig(cx, cy);
            videoWidget->ResetVideoSettings();
            break;
        case TabType::HOTKEYS:
            // TODO: implement?
            break;
        case TabType::ACCESSIBILITY:
            CONFIG_CONTEXT.InitAccessibilityConfig();
            accessWidget->LoadAccessibilitySettings();
            break;
        case TabType::ADVANCED:
            CONFIG_CONTEXT.SetAdvancedConfig();
            advanceWidget->LoadAdvancedSettings();
            break;
    }
}

void AFQStudioSettingDialog::UpdateResetButtonVisible() {
    int curTabIndex = ui->stackedWidget->currentIndex();
    TabType curTabType = static_cast<TabType>(curTabIndex);
    switch (curTabType) {
        case TabType::STREAM:
            resetButton->setVisible(false);
            break;
        default:
            resetButton->setVisible(true);
            break;
    }
}
