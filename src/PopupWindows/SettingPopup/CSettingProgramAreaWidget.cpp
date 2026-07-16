#include "CSettingProgramAreaWidget.h"
#include "ui_setting-program-area.h"

#include <QAbstractItemView>

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"

#include "Common/StudioDefine.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "UIComponent/CMessageAlert.h"
#include "UIComponent/CBasicHoverWidget.h"
#include "CSettingUtils.h"
#include "PopupWindows/SettingPopup/CAddStreamWidget.h"
#include "UIComponent/CMessageBox.h"

#include "MainFrame/Output/COutput.h"


#define GENERAL_CHANGED &AFQProgramSettingAreaWidget::qslotGeneralChanged
#define DEFAULT_LANG "en-US"

AFQProgramSettingAreaWidget::AFQProgramSettingAreaWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::AFQProgramSettingAreaWidget)
{
    ui->setupUi(this);

	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
}

AFQProgramSettingAreaWidget::~AFQProgramSettingAreaWidget()
{
	delete ui;
}

void AFQProgramSettingAreaWidget::qslotToggleAdvancedSetting()
{
    if (ui->widget_AdvanceProgramSettings->isVisible())
    {
        ui->widget_AdvanceProgramSettings->setVisible(false);
		ui->label_Triangle->setProperty("spread", false);
        //ui->label_Triangle->setStyleSheet("background:url(:/image/resource/Settings/Component/SpreadTriangle.svg) no-repeat center center fixed; background-color:rgba(255,255,255,0%);");

        const char* translate = Str("Basic.SettingPopup.Program.Extend");
        QString qtranslate = QString::fromUtf8(translate);
        ui->label_ChangeSettings->setText(qtranslate);
    }
    else
    {
        ui->widget_AdvanceProgramSettings->setVisible(true);
		ui->label_Triangle->setProperty("spread", true);
        //ui->label_Triangle->setStyleSheet("background:url(:/image/resource/Settings/Component/FoldTriangle.svg) no-repeat center center fixed; background-color:rgba(255,255,255,0%);");

        const char* translate = Str("Basic.SettingPopup.Program.Reduce");
        QString qtranslate = QString::fromUtf8(translate);
        ui->label_ChangeSettings->setText(qtranslate);
    }

	PolishStyleSheet(ui->label_Triangle);
}

void AFQProgramSettingAreaWidget::qslotHideAnetaWindowWarning(int state)
{
    if (m_loading || state == Qt::Unchecked)
        return;

    if (config_get_bool(USERCONFIG, "General", "WarnedAboutHideOBSFromCapture"))
        return;

    //AFCMessageBox::information(this, QTStr("Basic.Settings.General.HideOBSWindowsFromCapture"), QTStr("Basic.Settings.General.HideOBSWindowsFromCapture.Message"));

    config_set_bool(USERCONFIG, "General", "WarnedAboutHideOBSFromCapture", true);
    config_save_safe(USERCONFIG, "tmp", nullptr);
}

void AFQProgramSettingAreaWidget::qslotGeneralChanged()
{
	if (!m_loading)
	{
		m_programDataChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalProgramDataChanged();
	}
}

void AFQProgramSettingAreaWidget::qslotResetProgramSettingUi()
{
	m_restartNeeded = false;
}

void AFQProgramSettingAreaWidget::qslotDisableOverFlowCheckBox(bool check)
{
	ui->checkBox_OverflowAlwaysVisible->setEnabled(!check);
	ui->checkBox_OverflowSelectionHidden->setEnabled(!check);
}

void AFQProgramSettingAreaWidget::UpdateAutomaticReplayBufferCheckboxes(bool state)
{
	ui->checkBox_ReplayBufferWhileStreaming->setEnabled(state);
	ui->checkBox_KeepReplayBufferStreamStops->setEnabled(state);
}

void AFQProgramSettingAreaWidget::qslotSpreadAreaHoverEnter() {
	ui->label_Triangle->setProperty("hover", true);
	
	PolishStyleSheet(ui->label_Triangle);
}

void AFQProgramSettingAreaWidget::qslotSpreadAreaHoverLeave() {
	ui->label_Triangle->setProperty("hover", false);

	PolishStyleSheet(ui->label_Triangle);
}

void AFQProgramSettingAreaWidget::qslotLoginMainAccount()
{
	AFAddStreamWidget* addStream = new AFAddStreamWidget(this);

	addStream->AddStreamWidgetInit(PLATFORM_SOOP);

	if (addStream->exec() == QDialog::Accepted)
	{
		AFBasicAuth& resAuth = addStream->GetRawAuth();
		AFChannelData* newChannel = new AFChannelData();
		resAuth.platform = addStream->GetPlatform().toStdString();
        resAuth.uuid = QUuid::createUuid().toString().toStdString();
		newChannel->isStreaming = true;
		AUTH_CONTEXT.RegisterChannel(resAuth.uuid.c_str(), newChannel);

		auto& auth = AUTH_CONTEXT;
		//auth.FlushAuthCache();
		//auth.FlushAuthMain();
		auth.SaveAllAuthed();

		MAIN_OUTPUT->SetStreamingOutput();

		ui->stackedWidget_Account->setCurrentIndex(1);
		ui->label_Nickname->setText(resAuth.channelNick.c_str());
		ui->label_IdNumber->setText(resAuth.channelID.c_str());

		QPixmap* newObj = MAINFRAME->MakePixmapFromAuthData(newChannel->pAuthData);
		if (newObj != nullptr) {
			newChannel->pObjQtPixmap = newObj;
			ui->label_ProfilePicture->setPixmap(*newObj);
		}

		MAINFRAME->LoadAccounts();
	}
}

void AFQProgramSettingAreaWidget::qslotLogoutMainAccount()
{
	AFQMessagBoxAlert dlg(this, QTStr("ConfirmLogout.Text"), QTStr("ConfirmLogout.TextInfo"), QTStr("Logout"));

	if (QDialog::Accepted == dlg.exec())
	{
		AUTH_CONTEXT.SetSoopRegistered(false);
		AUTH_CONTEXT.RemoveMainChannel();
		MAIN_BLOCKMANAGER->DeletePlatformPage();
		MAINFRAME->LoadAccounts();
		ui->stackedWidget_Account->setCurrentIndex(0);

		emit qsignalLogoutKR();
	}
}

void AFQProgramSettingAreaWidget::ProgramSettingAreaInit()
{
	//Scene Collection Auto Hide
	ui->widget_Importers->hide();
	//Scene Collection Auto Hide

	//Double Click Transition Hide
	ui->checkBox_SwitchOnDoubleClick->hide();
	//Double Click Transition Hide
	
	//Rounded Transparent Issue - SetDisplayAffinity Error
	//ui->checkBox_HideANETAWindowsFromCapture->hide();
	//Rounded Transparent Issue - SetDisplayAffinity Error

    ui->widget_AdvanceProgramSettings->setVisible(false);
    ui->label_ChangeSettings->setText(QTStr("Basic.SettingPopup.Program.Extend"));
	ui->label_Triangle->setProperty("spread", false);

	connect(ui->widget_Spread, &AFQHoverWidget::qsignalMouseClick, this, &AFQProgramSettingAreaWidget::qslotToggleAdvancedSetting);
	connect(ui->widget_Spread, &AFQHoverWidget::qsignalHoverEnter, this, &AFQProgramSettingAreaWidget::qslotSpreadAreaHoverEnter);
	connect(ui->widget_Spread, &AFQHoverWidget::qsignalHoverLeave, this, &AFQProgramSettingAreaWidget::qslotSpreadAreaHoverLeave);

#if defined(_WIN32)
	if (!SetDisplayAffinitySupported()) {
		delete ui->checkBox_HideANETAWindowsFromCapture;
		ui->checkBox_HideANETAWindowsFromCapture = nullptr;
	}

	if (ui->checkBox_HideANETAWindowsFromCapture) {
		connect(ui->checkBox_HideANETAWindowsFromCapture, &QCheckBox::stateChanged,
				this, &AFQProgramSettingAreaWidget::qslotHideAnetaWindowWarning);
	}
#else
	delete ui->checkBox_HideANETAWindowsFromCapture;
	ui->checkBox_HideANETAWindowsFromCapture = nullptr;
#endif

	/*connect(ui->checkBox_RecordWhenStreaming, &QCheckBox::toggled, 
		ui->checkBox_KeepRecordingWhenStreamStops, &QCheckBox::setEnabled);*/

	/*connect(ui->checkBox_ReplayBufferWhileStreaming, &QCheckBox::toggled,
		ui->checkBox_KeepReplayBufferStreamStops, &QCheckBox::setEnabled);*/

	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->label_SnapDistance, &QLabel::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->doubleSpinBox_SnapDistance, &QDoubleSpinBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_ScreenSnapping, &QCheckBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_SourceSnapping, &QCheckBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_CenterSnapping, &QCheckBox::setEnabled);

	connect(ui->widget_Login, &AFQHoverWidget::qsignalMouseClick, this, &AFQProgramSettingAreaWidget::qslotLoginMainAccount);
	connect(ui->pushButton_Logout, &QPushButton::clicked, this, &AFQProgramSettingAreaWidget::qslotLogoutMainAccount);

	//system tray disabled
	ui->widget_Systray->hide();
	//system tray disabled
	
	//theme disabled
	ui->comboBox_Theme->hide();
	ui->label_Theme->hide();
	//theme disabeld

	//Multiview Safe Area Disabled
	ui->checkBox_MultiviewDrawSafeAreas->hide();
	//Multiview Safe Area Disabled

	if (MAINFRAME->IsSmallResolution())
		ui->scrollAreaWidgetContents->setFixedWidth(800);

	_SetProgramSettingSignal();
    LoadProgramSettings();
	ToggleOnStreaming(true);
}

void AFQProgramSettingAreaWidget::LoadProgramSettings()
{
	m_systemTrayToggle = false;
	_LoadLanguageList();
	_LoadThemeList();

	LoadMainAccount();

	bool openStatsOnStartup =  config_get_bool(ACTIVECONFIG, "General", "OpenStatsOnStartup");
	ui->checkBox_GeneralOpenStatsOnStartup->setChecked(openStatsOnStartup);

	bool programSettingPageSpread = config_get_bool(USERCONFIG, "BasicWindow", "ProgramSettingPageSpread");
	if (programSettingPageSpread)
		qslotToggleAdvancedSetting();

#if defined(_WIN32)
	if (ui->checkBox_HideANETAWindowsFromCapture) {
		bool hideWindowFromCapture = config_get_bool(USERCONFIG, "BasicWindow", "HideOBSWindowsFromCapture");
		ui->checkBox_HideANETAWindowsFromCapture->setChecked(hideWindowFromCapture);
	}
#endif

	ui->checkBox_WarnBeforeStartingStream->hide();
	ui->checkBox_WarnBeforeStoppingStream->hide();

	bool warnBeforeRecordStop = config_get_bool(USERCONFIG, "BasicWindow", "WarnBeforeStoppingRecord");
	ui->checkBox_WarnBeforeStoppingRecord->setChecked(warnBeforeRecordStop);

	bool recordWhenStreaming = config_get_bool(USERCONFIG, "BasicWindow", "RecordWhenStreaming");
	ui->checkBox_RecordWhenStreaming->setChecked(recordWhenStreaming);
	ui->checkBox_RecordWhenStreaming->toggled(recordWhenStreaming);

	bool keepRecordStreamStops = config_get_bool(USERCONFIG, "BasicWindow", "KeepRecordingWhenStreamStops");
	ui->checkBox_KeepRecordingWhenStreamStops->setChecked(keepRecordStreamStops);

	bool replayWhileStreaming = config_get_bool(USERCONFIG, "BasicWindow", "ReplayBufferWhileStreaming");
	ui->checkBox_ReplayBufferWhileStreaming->setChecked(replayWhileStreaming);
	ui->checkBox_ReplayBufferWhileStreaming->toggled(replayWhileStreaming);

	bool keepReplayStreamStops = config_get_bool(USERCONFIG, "BasicWindow", "KeepReplayBufferStreamStops");
	ui->checkBox_KeepReplayBufferStreamStops->setChecked(keepReplayStreamStops);

	bool snappingEnabled = config_get_bool(USERCONFIG, "BasicWindow", "SnappingEnabled");
	ui->checkBox_SnappingEnabled->setChecked(snappingEnabled);
	ui->checkBox_SnappingEnabled->toggled(snappingEnabled);

	double snapDistance = config_get_double(USERCONFIG, "BasicWindow", "SnapDistance");
	ui->doubleSpinBox_SnapDistance->setValue(snapDistance);

	bool screenSnapping = config_get_bool(USERCONFIG, "BasicWindow", "ScreenSnapping");
	ui->checkBox_ScreenSnapping->setChecked(screenSnapping);

	bool sourceSnapping = config_get_bool(USERCONFIG, "BasicWindow", "SourceSnapping");
	ui->checkBox_SourceSnapping->setChecked(sourceSnapping);

	bool centerSnapping = config_get_bool(USERCONFIG, "BasicWindow", "CenterSnapping");
	ui->checkBox_CenterSnapping->setChecked(centerSnapping);

	//bool systemTrayEnabled = config_get_bool(USERCONFIG, "BasicWindow", "SysTrayEnabled");
	//ui->checkBox_SystemTrayEnabled->setChecked(systemTrayEnabled);

	//bool systemTrayWhenStarted = config_get_bool(userConfig, "BasicWindow", "SysTrayWhenStarted");
	//ui->checkBox_SysTrayWhenStarted->setChecked(systemTrayWhenStarted);

	//bool systemTrayAlways = config_get_bool(userConfig, "BasicWindow", "SysTrayMinimizeToTray");
	//ui->checkBox_SystemTrayHideMinimize->setChecked(systemTrayAlways);

	bool overflowHide = config_get_bool(USERCONFIG, "BasicWindow", "OverflowHidden");
	ui->checkBox_OverflowHidden->setChecked(overflowHide);
	ui->checkBox_OverflowHidden->toggled(overflowHide);

	bool overflowAlwaysVisible = config_get_bool(USERCONFIG, "BasicWindow", "OverflowAlwaysVisible");
	ui->checkBox_OverflowAlwaysVisible->setChecked(overflowAlwaysVisible);

	bool overflowSelectionHide = config_get_bool(USERCONFIG, "BasicWindow", "OverflowSelectionHidden");
	ui->checkBox_OverflowSelectionHidden->setChecked(overflowSelectionHide);

	bool safeAreas = config_get_bool(USERCONFIG, "BasicWindow", "ShowSafeAreas");
	ui->checkBox_PreviewDrawSafeAreas->setChecked(safeAreas);

	bool spacingHelpersEnabled = config_get_bool(USERCONFIG, "BasicWindow", "SpacingHelpersEnabled");
	ui->checkBox_SpacingHelpers->setChecked(spacingHelpersEnabled);

	bool automaticSearch = config_get_bool(USERCONFIG, "General", "AutomaticCollectionSearch");
	ui->checkBox_AutomaticCollectionSearch->setChecked(automaticSearch);

	//Double Click Transition Hide
	/*bool doubleClickSwitch = config_get_bool(userConfig, "BasicWindow", "TransitionOnDoubleClick");
	ui->checkBox_SwitchOnDoubleClick->setChecked(doubleClickSwitch);*/
	//Double Click Transition Hide

	bool studioPortraitLayout = config_get_bool(USERCONFIG, "BasicWindow", "StudioPortraitLayout");
	ui->checkBox_StudioPortraitLayout->setChecked(studioPortraitLayout);
	m_studioModeVertical = studioPortraitLayout;

	bool prevProgLabels = config_get_bool(USERCONFIG, "BasicWindow", "StudioModeLabels");
	ui->checkBox_TogglePreviewProgramLabels->setChecked(prevProgLabels);

	bool multiviewMouseSwitch = config_get_bool(USERCONFIG, "BasicWindow", "MultiviewMouseSwitch");
	ui->checkBox_MouseSwitch->setChecked(multiviewMouseSwitch);

	bool multiviewDrawNames = config_get_bool(USERCONFIG, "BasicWindow", "MultiviewDrawNames");
	ui->checkBox_DrawSourceNames->setChecked(multiviewDrawNames);


	//Multiview Safe Area Disabled
	/*bool multiviewDrawAreas = config_get_bool(USERCONFIG, "BasicWindow", "MultiviewDrawAreas");
	ui->checkBox_MultiviewDrawSafeAreas->setChecked(multiviewDrawAreas);*/
	//Multiview Safe Area Disabled

	// [Temp Alpha Test]
	/*bool showPopupLeft = config_get_bool(USERCONFIG, "BasicWindow", "ShowPopupLeft");
	ui->checkBox_PopupPos->setChecked(showPopupLeft);*/
	//

	m_loading = false;
}

void AFQProgramSettingAreaWidget::LoadMainAccount()
{
	AFChannelData* mainChannel = nullptr;

	bool regMainAccount = AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, mainChannel);
	if (regMainAccount)
	{
		if (mainChannel)
		{
			ui->stackedWidget_Account->setCurrentIndex(1);
			ui->label_Nickname->setText(mainChannel->pAuthData->channelNick.c_str());
			ui->label_IdNumber->setText(mainChannel->pAuthData->channelID.c_str());

			if (mainChannel && mainChannel->pObjQtPixmap != nullptr)
			{
				QPixmap* savedPixmap = (QPixmap*)mainChannel->pObjQtPixmap;
				ui->label_ProfilePicture->setPixmap(*savedPixmap);
			}
			else if (mainChannel && mainChannel->pAuthData)
			{
				QPixmap* newPixmap = MAINFRAME->MakePixmapFromAuthData(mainChannel->pAuthData);
				if (newPixmap != nullptr) {
					mainChannel->pObjQtPixmap = newPixmap;
					ui->label_ProfilePicture->setPixmap(*newPixmap);
				}
			}
		}
	}
	else
	{
		ui->stackedWidget_Account->setCurrentIndex(0);
	}
}

void AFQProgramSettingAreaWidget::SaveProgramSettings()
{
	int languageIndex = ui->comboBox_Language->currentIndex();
	QVariant langData = ui->comboBox_Language->itemData(languageIndex);
	std::string language = langData.toString().toStdString();

	if (AFSettingUtils::WidgetChanged(ui->comboBox_Language))
	{
		config_set_string(USERCONFIG, "General", "Language", language.c_str());
		if (m_currentLanguageIndex != languageIndex)
		{
			m_restartNeeded = true;
		}
	}

	int themeIndex = ui->comboBox_Theme->currentIndex();
	QString themeData = ui->comboBox_Theme->itemData(themeIndex).toString();

	if (AFSettingUtils::WidgetChanged(ui->comboBox_Theme)) {
		m_savedTheme = themeData.toStdString();
		config_set_string(USERCONFIG, "General", "CurrentTheme3", QT_TO_UTF8(themeData));
	}

	//Need Check
#ifdef _WIN32
	if (ui->checkBox_HideANETAWindowsFromCapture && AFSettingUtils::WidgetChanged(ui->checkBox_HideANETAWindowsFromCapture)) {
		bool hide_window = ui->checkBox_HideANETAWindowsFromCapture->isChecked();
		config_set_bool(USERCONFIG, "BasicWindow", "HideOBSWindowsFromCapture", hide_window);

		QWindowList windows = QGuiApplication::allWindows();
		for (auto window : windows) {
			if (window->isVisible()) {
				MAINFRAME->SetDisplayAffinity(window);
			}
		}

		blog(LOG_INFO, "Hide OBS windows from screen capture: %s",
			hide_window ? "true" : "false");
	}
#endif

	if (AFSettingUtils::WidgetChanged(ui->checkBox_GeneralOpenStatsOnStartup))
		config_set_bool(ACTIVECONFIG, "General", "OpenStatsOnStartup", ui->checkBox_GeneralOpenStatsOnStartup->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_SnappingEnabled))
		config_set_bool(USERCONFIG, "BasicWindow", "SnappingEnabled", ui->checkBox_SnappingEnabled->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_ScreenSnapping))
		config_set_bool(USERCONFIG, "BasicWindow", "ScreenSnapping", ui->checkBox_ScreenSnapping->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_CenterSnapping))
		config_set_bool(USERCONFIG, "BasicWindow", "CenterSnapping", ui->checkBox_CenterSnapping->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_SourceSnapping))
		config_set_bool(USERCONFIG, "BasicWindow", "SourceSnapping", ui->checkBox_SourceSnapping->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->doubleSpinBox_SnapDistance))
		config_set_double(USERCONFIG, "BasicWindow", "SnapDistance", ui->doubleSpinBox_SnapDistance->value());

	if (AFSettingUtils::WidgetChanged(ui->checkBox_OverflowAlwaysVisible) ||
		AFSettingUtils::WidgetChanged(ui->checkBox_OverflowHidden) ||
		AFSettingUtils::WidgetChanged(ui->checkBox_OverflowSelectionHidden)) {
		config_set_bool(USERCONFIG, "BasicWindow", "OverflowAlwaysVisible", ui->checkBox_OverflowAlwaysVisible->isChecked());
		config_set_bool(USERCONFIG, "BasicWindow", "OverflowHidden", ui->checkBox_OverflowHidden->isChecked());
		config_set_bool(USERCONFIG, "BasicWindow", "OverflowSelectionHidden", ui->checkBox_OverflowSelectionHidden->isChecked());

		DYNAMIC_COMPOSIT->UpdatePreviewOverflowSettings();
	}

	if (AFSettingUtils::WidgetChanged(ui->checkBox_PreviewDrawSafeAreas)) {
		config_set_bool(USERCONFIG, "BasicWindow", "ShowSafeAreas", ui->checkBox_PreviewDrawSafeAreas->isChecked());

		DYNAMIC_COMPOSIT->UpdatePreviewSafeAreas();
	}

	if (AFSettingUtils::WidgetChanged(ui->checkBox_SpacingHelpers)) {
		config_set_bool(USERCONFIG, "BasicWindow", "SpacingHelpersEnabled", ui->checkBox_SpacingHelpers->isChecked());

		DYNAMIC_COMPOSIT->UpdatePreviewSpacingHelpers();
	}

	//Double Click Transition Hide
	/*if (AFSettingUtils::WidgetChanged(ui->checkBox_SwitchOnDoubleClick))
		config_set_bool(USERCONFIG, "BasicWindow", "TransitionOnDoubleClick", ui->checkBox_SwitchOnDoubleClick->isChecked());*/
	//Double Click Transition Hide

	//Scene Collection Auto Hide
	if (AFSettingUtils::WidgetChanged(ui->checkBox_AutomaticCollectionSearch))
		config_set_bool(USERCONFIG, "General", "AutomaticCollectionSearch", false);
	//Scene Collection Auto Hide

	config_set_bool(USERCONFIG, "BasicWindow", "WarnBeforeStoppingRecord", ui->checkBox_WarnBeforeStoppingRecord->isChecked());

	if (AFSettingUtils::WidgetChanged(ui->checkBox_RecordWhenStreaming))
		config_set_bool(USERCONFIG, "BasicWindow", "RecordWhenStreaming", ui->checkBox_RecordWhenStreaming->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_KeepRecordingWhenStreamStops))
		config_set_bool(USERCONFIG, "BasicWindow", "KeepRecordingWhenStreamStops", ui->checkBox_KeepRecordingWhenStreamStops->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_ReplayBufferWhileStreaming))
		config_set_bool(USERCONFIG, "BasicWindow", "ReplayBufferWhileStreaming", ui->checkBox_ReplayBufferWhileStreaming->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_KeepReplayBufferStreamStops))
		config_set_bool(USERCONFIG, "BasicWindow", "KeepReplayBufferStreamStops", ui->checkBox_KeepReplayBufferStreamStops->isChecked());

	//System Tray Disable
	//if (AFSettingUtils::WidgetChanged(ui->checkBox_SystemTrayEnabled)) {
	//	config_set_bool(USERCONFIG, "BasicWindow", "SysTrayEnabled", ui->checkBox_SystemTrayEnabled->isChecked());
	//	m_systemTrayToggle = true;
	//}
	////MainFrame
	//if (AFSettingUtils::WidgetChanged(ui->checkBox_SysTrayWhenStarted))
	//	config_set_bool(USERCONFIG, "BasicWindow", "SysTrayWhenStarted", ui->checkBox_SysTrayWhenStarted->isChecked());
	////MainFrame
	//if (AFSettingUtils::WidgetChanged(ui->checkBox_SystemTrayHideMinimize))
	//	config_set_bool(USERCONFIG, "BasicWindow", "SysTrayMinimizeToTray", ui->checkBox_SystemTrayHideMinimize->isChecked());
	//System Tray Disable

	bool studioModeReset = false;

	if (AFSettingUtils::WidgetChanged(ui->checkBox_StudioPortraitLayout)) {
		config_set_bool(USERCONFIG, "BasicWindow", "StudioPortraitLayout", ui->checkBox_StudioPortraitLayout->isChecked());
		studioModeReset = true;
	}

	if (AFSettingUtils::WidgetChanged(ui->checkBox_TogglePreviewProgramLabels)) {
		config_set_bool(USERCONFIG, "BasicWindow", "StudioModeLabels", ui->checkBox_TogglePreviewProgramLabels->isChecked());
		studioModeReset = true;
	}

	if (studioModeReset)
	{
		bool changeLayout = (m_studioModeVertical == ui->checkBox_StudioPortraitLayout->isChecked()) ? false : true;

		MAINFRAME->ResetStudioModeUI(changeLayout);
	}
	m_studioModeVertical = ui->checkBox_StudioPortraitLayout->isChecked();

	bool multiviewChanged = false;
	if (AFSettingUtils::WidgetChanged(ui->checkBox_MouseSwitch)) {
		config_set_bool(USERCONFIG, "BasicWindow", "MultiviewMouseSwitch", ui->checkBox_MouseSwitch->isChecked());
		multiviewChanged = true;
	}
	if (AFSettingUtils::WidgetChanged(ui->checkBox_DrawSourceNames)) {
		config_set_bool(USERCONFIG, "BasicWindow", "MultiviewDrawNames", ui->checkBox_DrawSourceNames->isChecked());
		multiviewChanged = true;
	}

	//Multiview Safe Area Disabled
	/*if (AFSettingUtils::WidgetChanged(ui->checkBox_MultiviewDrawSafeAreas)) {
		config_set_bool(USERCONFIG, "BasicWindow", "MultiviewDrawAreas", ui->checkBox_MultiviewDrawSafeAreas->isChecked());
		multiviewChanged = true;
	}*/
	//Multiview Safe Area Disabled

	if (multiviewChanged)
		MAINFRAME->RefreshSceneUI();
}

void AFQProgramSettingAreaWidget::ResetProgramSettings() {
    int idxBefore = ui->comboBox_Language->currentIndex();
    CONFIG_CONTEXT.ResetProgramConfig();
    LoadProgramSettings();
    int idxAfter = ui->comboBox_Language->currentIndex();

    if (idxBefore != idxAfter) {
        m_restartNeeded = true;
    }
}

void AFQProgramSettingAreaWidget::ToggleOnStreaming(bool streaming)
{
	bool useVideo = obs_video_active() ? false : true;
    ui->comboBox_Language->setEnabled(useVideo);
    ui->widget_Login->setEnabled(useVideo);
    ui->pushButton_Logout->setEnabled(useVideo);
    ui->comboBox_Language->setEnabled(useVideo);
}

void AFQProgramSettingAreaWidget::SaveProgramPageSpreadState()
{
	bool bPageSpread = false;
	if (ui->widget_AdvanceProgramSettings->isVisible())
		bPageSpread = true;

	config_set_bool(USERCONFIG, "BasicWindow", "ProgramSettingPageSpread", bPageSpread);
}

void AFQProgramSettingAreaWidget::_LoadLanguageList()
{
    const char* currentLang = config_get_string(USERCONFIG, "General", "Language");
    if (!currentLang || currentLang[0] == '\0')
		currentLang = LOCALE_CONTEXT.GetCurrentLocale();

    ui->comboBox_Language->clear();

    for (const auto& locale : LOCALE_CONTEXT.GetLocaleNames()) {
        int idx = ui->comboBox_Language->count();

        ui->comboBox_Language->addItem(QT_UTF8(locale.second.c_str()), QT_UTF8(locale.first.c_str()));

		qDebug() << QT_UTF8(locale.first.c_str());

		if (locale.first == currentLang)
		{
			ui->comboBox_Language->setCurrentIndex(idx);
		}
    }
    ui->comboBox_Language->model()->sort(0);
	m_currentLanguageIndex = ui->comboBox_Language->currentIndex();

}
void AFQProgramSettingAreaWidget::_LoadThemeList()
{
    //Theme List
    //m_savedTheme = config_get_string(USERCONFIG, "General", "CurrentTheme3");

    //ui->comboBox_Theme->clear();
    //QSet<QString> uniqueSet;
    //string themeDir;
    //char userThemeDir[512];
    //int ret = GetAppConfigPath(userThemeDir, sizeof(userThemeDir), LOCAL_FOLDER_NAME "/themes/");
    //GetDataFilePath("themes/", themeDir);

    ///* Check user dir first. */
    //if (ret > 0) {
    //    QDirIterator it(QString(userThemeDir), QStringList() << "*.qss",
    //        QDir::Files);
    //    while (it.hasNext()) {
    //        it.next();
    //        QString name = it.fileInfo().completeBaseName();
    //        ui->theme->addItem(name, name);
    //        uniqueSet.insert(name);
    //    }
    //}

    ///* Check shipped themes. */
    //QDirIterator uIt(QString(themeDir.c_str()), QStringList() << "*.qss",
    //    QDir::Files);
    //while (uIt.hasNext()) {
    //    uIt.next();
    //    QString name = uIt.fileInfo().completeBaseName();
    //    QString value = name;

    //    if (name == DEFAULT_THEME)
    //        name += " " + QTStr("Default");

    //    if (!uniqueSet.contains(value) && name != "Default")
    //        ui->comboBox_Theme->addItem(name, value);
    //}

    //int idx = ui->comboBox_Theme->findData(QT_UTF8(App()->GetTheme()));
    //if (idx != -1)
    //    ui->comboBox_Theme->setCurrentIndex(idx);
}

void AFQProgramSettingAreaWidget::_SetProgramSettingSignal()
{
	AFSettingUtils::HookWidget(ui->comboBox_Language, this, COMBO_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_Theme, this, COMBO_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_GeneralOpenStatsOnStartup, this, CHECK_CHANGED, GENERAL_CHANGED);
	
#if defined(_WIN32)
	if (ui->checkBox_HideANETAWindowsFromCapture)
		AFSettingUtils::HookWidget(ui->checkBox_HideANETAWindowsFromCapture, this, CHECK_CHANGED, GENERAL_CHANGED);
#endif

	AFSettingUtils::HookWidget(ui->checkBox_WarnBeforeStoppingRecord, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_RecordWhenStreaming, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_KeepRecordingWhenStreamStops, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_ReplayBufferWhileStreaming, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_KeepReplayBufferStreamStops, this, CHECK_CHANGED, GENERAL_CHANGED);

	AFSettingUtils::HookWidget(ui->checkBox_SnappingEnabled, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->doubleSpinBox_SnapDistance, this, DSCROLL_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_ScreenSnapping, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SourceSnapping, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_CenterSnapping, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SystemTrayEnabled, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SysTrayWhenStarted, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SystemTrayHideMinimize, this, CHECK_CHANGED, GENERAL_CHANGED);

	AFSettingUtils::HookWidget(ui->checkBox_OverflowHidden, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_OverflowAlwaysVisible, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_OverflowSelectionHidden, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_PreviewDrawSafeAreas, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_SpacingHelpers, this, CHECK_CHANGED, GENERAL_CHANGED);

	AFSettingUtils::HookWidget(ui->checkBox_AutomaticCollectionSearch, this, CHECK_CHANGED, GENERAL_CHANGED);

	//Double Click Transition Hide
	//AFSettingUtils::HookWidget(ui->checkBox_SwitchOnDoubleClick, this, CHECK_CHANGED, GENERAL_CHANGED);
	//Double Click Transition Hide

	AFSettingUtils::HookWidget(ui->checkBox_StudioPortraitLayout, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_TogglePreviewProgramLabels, this, CHECK_CHANGED, GENERAL_CHANGED);

	AFSettingUtils::HookWidget(ui->checkBox_MouseSwitch, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_DrawSourceNames, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_MultiviewDrawSafeAreas, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_PopupPos, this, CHECK_CHANGED, GENERAL_CHANGED);
	//

	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->label_SnapDistance, &QLabel::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->doubleSpinBox_SnapDistance, &QDoubleSpinBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_ScreenSnapping, &QCheckBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_SourceSnapping, &QCheckBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_CenterSnapping, &QCheckBox::setEnabled);

	connect(ui->checkBox_OverflowHidden, &QCheckBox::toggled, this, &AFQProgramSettingAreaWidget::qslotDisableOverFlowCheckBox);
	
	//connect(ui->checkBox_ReplayBufferWhileStreaming, &QCheckBox::toggled, ui->checkBox_KeepReplayBufferStreamStops, &QCheckBox::setEnabled);
	//connect(ui->checkBox_RecordWhenStreaming, &QCheckBox::toggled, ui->checkBox_KeepRecordingWhenStreamStops, &QCheckBox::setEnabled);
}

void AFQProgramSettingAreaWidget::_ChangeLanguage()
{
    QList<QLabel*> labelList = findChildren<QLabel*>();
    QList<QCheckBox*> checkboxList = findChildren<QCheckBox*>();

    foreach(QLabel * label, labelList)
    {
        if (label == ui->label_Nickname || label == ui->label_IdNumber)
        {
            continue;
        }
		label->setText(QTStr(label->text().toUtf8().constData()));
    }

    foreach(QCheckBox * checkbox, checkboxList)
    {
        checkbox->setText(QTStr(checkbox->text().toUtf8().constData()));
    }

	ui->label_SysTray->setText(QT_UTF8("팝업 위치 설정"));
	//
}
