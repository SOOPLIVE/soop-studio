#include "CProgramSettingAreaWidget.h"
#include "ui_program-setting-area.h"

#include <QAbstractItemView>

#include "UIComponent/CBasicHoverWidget.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "include/qt-wrapper.h"
#include "CSettingAreaUtils.h"
#include "PopupWindows/SettingPopup/CAddStreamWidget.h"
#include "UIComponent/CMessageBox.h"

#define GENERAL_CHANGED &AFQProgramSettingAreaWidget::qslotGeneralChanged
#define DEFAULT_LANG "en-US"

AFQProgramSettingAreaWidget::AFQProgramSettingAreaWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::AFQProgramSettingAreaWidget)
{
    ui->setupUi(this);
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

        const char* translate = AFLocaleTextManager::GetSingletonInstance().Str("Basic.SettingPopup.Program.Extend");
        QString qtranslate = QString::fromUtf8(translate);
        ui->label_ChangeSettings->setText(qtranslate);
    }
    else
    {
        ui->widget_AdvanceProgramSettings->setVisible(true);
		ui->label_Triangle->setProperty("spread", true);
        
		const char* translate = AFLocaleTextManager::GetSingletonInstance().Str("Basic.SettingPopup.Program.Reduce");
        QString qtranslate = QString::fromUtf8(translate);
        ui->label_ChangeSettings->setText(qtranslate);
    }

	style()->unpolish(ui->label_Triangle);
	style()->polish(ui->label_Triangle);
}

void AFQProgramSettingAreaWidget::qslotHideAnetaWindowWarning(int state)
{
	config_t* configGlobalFile = AFConfigManager::GetSingletonInstance().GetGlobal();
    if (m_bLoading || state == Qt::Unchecked)
        return;

    if (config_get_bool(configGlobalFile, "General", "WarnedAboutHideOBSFromCapture"))
        return;

    config_set_bool(configGlobalFile, "General", "WarnedAboutHideOBSFromCapture", true);
    config_save_safe(configGlobalFile, "tmp", nullptr);
}

void AFQProgramSettingAreaWidget::qslotGeneralChanged()
{
	if (!m_bLoading)
	{
		m_bProgramDataChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalProgramDataChanged();
	}
}

void AFQProgramSettingAreaWidget::qslotResetProgramSettingUi()
{
	m_bRestartNeeded = false;
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
	
	style()->unpolish(ui->label_Triangle);
	style()->polish(ui->label_Triangle);
}

void AFQProgramSettingAreaWidget::qslotSpreadAreaHoverLeave() {
	ui->label_Triangle->setProperty("hover", false);

	style()->unpolish(ui->label_Triangle);
	style()->polish(ui->label_Triangle);
}

void AFQProgramSettingAreaWidget::qslotLoginMainAccount()
{
	AFAddStreamWidget* addStream = new AFAddStreamWidget(this);
	addStream->AddStreamWidgetInit("SOOP Global");

	if (addStream->exec() == QDialog::Accepted)
	{
		AFBasicAuth& resAuth = addStream->GetRawAuth();
		AFChannelData* newChannel = new AFChannelData();
		resAuth.strPlatform = addStream->GetPlatform().toStdString();
        resAuth.strUuid = QUuid::createUuid().toString().toStdString();
		newChannel->bIsStreaming = true;
		auto& authManager = AFAuthManager::GetSingletonInstance();
		authManager.RegisterChannel(resAuth.strUuid.c_str(), newChannel);

		auto& auth = AFAuthManager::GetSingletonInstance();
		auth.SaveAllAuthed();

		App()->GetMainView()->SetStreamingOutput();

		ui->stackedWidget_Account->setCurrentIndex(1);
		ui->label_Nickname->setText(resAuth.strChannelNick.c_str());
		ui->label_IdNumber->setText(resAuth.strChannelID.c_str());
		App()->GetMainView()->LoadAccounts();
	}
}

void AFQProgramSettingAreaWidget::qslotLogoutMainAccount()
{
	auto& localeMan = AFLocaleTextManager::GetSingletonInstance();
	int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		this, "",
		QT_UTF8(localeMan.Str("Basic.Settings.Stream.DeleteAccountAlert")));

	if (result == QDialog::Rejected)
	{
		return;
	}
	else
	{
		auto& authManager = AFAuthManager::GetSingletonInstance();
		authManager.SetSoopGlobalPreregistered(false);
    	authManager.SetSoopGlobalPreregistered(false);
		authManager.RemoveMainChannel();
		App()->GetMainView()->qslotLogoutGlobalPage();
		App()->GetMainView()->LoadAccounts();
		ui->stackedWidget_Account->setCurrentIndex(0);
	}
}

void AFQProgramSettingAreaWidget::ProgramSettingAreaInit()
{
	ui->widget_Importers->hide();
	ui->checkBox_HideANETAWindowsFromCapture->hide();
    ui->widget_AdvanceProgramSettings->setVisible(false);
    ui->label_ChangeSettings->setText("Basic.SettingPopup.Program.Extend"); //locale
	ui->label_Triangle->setProperty("spread", false);

	connect(ui->widget_Spread, &AFQHoverWidget::qsignalMouseClick, this, &AFQProgramSettingAreaWidget::qslotToggleAdvancedSetting);
	connect(ui->widget_Spread, &AFQHoverWidget::qsignalHoverEnter, this, &AFQProgramSettingAreaWidget::qslotSpreadAreaHoverEnter);
	connect(ui->widget_Spread, &AFQHoverWidget::qsignalHoverLeave, this, &AFQProgramSettingAreaWidget::qslotSpreadAreaHoverLeave);

	connect(ui->checkBox_HideANETAWindowsFromCapture, &QCheckBox::stateChanged,
		this, &AFQProgramSettingAreaWidget::qslotHideAnetaWindowWarning);

	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->label_SnapDistance, &QLabel::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->doubleSpinBox_SnapDistance, &QDoubleSpinBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_ScreenSnapping, &QCheckBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_SourceSnapping, &QCheckBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_CenterSnapping, &QCheckBox::setEnabled);

	connect(ui->widget_Login, &AFQHoverWidget::qsignalMouseClick, 
		this, &AFQProgramSettingAreaWidget::qslotLoginMainAccount);
	connect(ui->pushButton_Logout, &QPushButton::clicked,
		this, &AFQProgramSettingAreaWidget::qslotLogoutMainAccount);

	ui->widget_Systray->hide();
	ui->comboBox_Theme->hide();
	ui->label_Theme->hide();
	ui->checkBox_MultiviewDrawSafeAreas->hide();

    _ChangeLanguage();
	_SetProgramSettingSignal();
    LoadProgramSettings();
	ToggleOnStreaming(true);
}

void AFQProgramSettingAreaWidget::LoadProgramSettings()
{
	m_bSystemTrayToggle = false;
	_LoadLanguageList();
	_LoadThemeList();

	LoadMainAccount();

	config_t* configGlobalFile = AFConfigManager::GetSingletonInstance().GetGlobal();

	bool openStatsOnStartup = 
		config_get_bool(AFConfigManager::GetSingletonInstance().GetBasic(), "General", "OpenStatsOnStartup");
	ui->checkBox_GeneralOpenStatsOnStartup->setChecked(openStatsOnStartup);

	bool programSettingPageSpread = config_get_bool(configGlobalFile, "BasicWindow", "ProgramSettingPageSpread");
	if (programSettingPageSpread)
		qslotToggleAdvancedSetting();

#if defined(_WIN32)
	if (ui->checkBox_HideANETAWindowsFromCapture) {
		bool hideWindowFromCapture = config_get_bool(configGlobalFile, "BasicWindow", "HideOBSWindowsFromCapture");
		ui->checkBox_HideANETAWindowsFromCapture->setChecked(hideWindowFromCapture);
	}
#endif
	bool warnBeforeStreamStart = config_get_bool(configGlobalFile, "BasicWindow", "WarnBeforeStartingStream");
	ui->checkBox_WarnBeforeStartingStream->setChecked(warnBeforeStreamStart);

	bool warnBeforeStreamStop = config_get_bool(configGlobalFile, "BasicWindow", "WarnBeforeStoppingStream");
	ui->checkBox_WarnBeforeStoppingStream->setChecked(warnBeforeStreamStop);

	bool warnBeforeRecordStop = config_get_bool(configGlobalFile, "BasicWindow", "WarnBeforeStoppingRecord");
	ui->checkBox_WarnBeforeStoppingRecord->setChecked(warnBeforeRecordStop);

	bool recordWhenStreaming = config_get_bool(configGlobalFile, "BasicWindow", "RecordWhenStreaming");
	ui->checkBox_RecordWhenStreaming->setChecked(recordWhenStreaming);
	ui->checkBox_RecordWhenStreaming->toggled(recordWhenStreaming);

	bool keepRecordStreamStops = config_get_bool(configGlobalFile, "BasicWindow", "KeepRecordingWhenStreamStops");
	ui->checkBox_KeepRecordingWhenStreamStops->setChecked(keepRecordStreamStops);

	bool replayWhileStreaming = config_get_bool(
		configGlobalFile, "BasicWindow", "ReplayBufferWhileStreaming");
	ui->checkBox_ReplayBufferWhileStreaming->setChecked(replayWhileStreaming);
	ui->checkBox_ReplayBufferWhileStreaming->toggled(replayWhileStreaming);

	bool keepReplayStreamStops = config_get_bool(configGlobalFile, "BasicWindow", "KeepReplayBufferStreamStops");
	ui->checkBox_KeepReplayBufferStreamStops->setChecked(keepReplayStreamStops);

	bool snappingEnabled = config_get_bool(configGlobalFile, "BasicWindow", "SnappingEnabled");
	ui->checkBox_SnappingEnabled->setChecked(snappingEnabled);
	ui->checkBox_SnappingEnabled->toggled(snappingEnabled);

	double snapDistance = config_get_double(configGlobalFile, "BasicWindow", "SnapDistance");
	ui->doubleSpinBox_SnapDistance->setValue(snapDistance);

	bool screenSnapping = config_get_bool(configGlobalFile, "BasicWindow", "ScreenSnapping");
	ui->checkBox_ScreenSnapping->setChecked(screenSnapping);

	bool sourceSnapping = config_get_bool(configGlobalFile, "BasicWindow", "SourceSnapping");
	ui->checkBox_SourceSnapping->setChecked(sourceSnapping);

	bool centerSnapping = config_get_bool(configGlobalFile, "BasicWindow", "CenterSnapping");
	ui->checkBox_CenterSnapping->setChecked(centerSnapping);

	bool overflowHide = config_get_bool(configGlobalFile, "BasicWindow", "OverflowHidden");
	ui->checkBox_OverflowHidden->setChecked(overflowHide);
	ui->checkBox_OverflowHidden->toggled(overflowHide);

	bool overflowAlwaysVisible = config_get_bool(configGlobalFile, "BasicWindow", "OverflowAlwaysVisible");
	ui->checkBox_OverflowAlwaysVisible->setChecked(overflowAlwaysVisible);

	bool overflowSelectionHide = config_get_bool(configGlobalFile, "BasicWindow", "OverflowSelectionHidden");
	ui->checkBox_OverflowSelectionHidden->setChecked(overflowSelectionHide);

	bool safeAreas = config_get_bool(configGlobalFile, "BasicWindow", "ShowSafeAreas");
	ui->checkBox_PreviewDrawSafeAreas->setChecked(safeAreas);

	bool spacingHelpersEnabled = config_get_bool(configGlobalFile, "BasicWindow", "SpacingHelpersEnabled");
	ui->checkBox_SpacingHelpers->setChecked(spacingHelpersEnabled);

	bool automaticSearch = config_get_bool(configGlobalFile, "General", "AutomaticCollectionSearch");
	ui->checkBox_AutomaticCollectionSearch->setChecked(automaticSearch);

	bool doubleClickSwitch = config_get_bool(configGlobalFile, "BasicWindow", "TransitionOnDoubleClick");
	ui->checkBox_SwitchOnDoubleClick->setChecked(doubleClickSwitch);

	bool studioPortraitLayout = config_get_bool(configGlobalFile, "BasicWindow", "StudioPortraitLayout");
	ui->checkBox_StudioPortraitLayout->setChecked(studioPortraitLayout);
	m_bStudioModeVertical = studioPortraitLayout;

	bool prevProgLabels = config_get_bool(configGlobalFile, "BasicWindow", "StudioModeLabels");
	ui->checkBox_TogglePreviewProgramLabels->setChecked(prevProgLabels);

	bool multiviewMouseSwitch = config_get_bool(configGlobalFile, "BasicWindow", "MultiviewMouseSwitch");
	ui->checkBox_MouseSwitch->setChecked(multiviewMouseSwitch);

	bool multiviewDrawNames = config_get_bool(configGlobalFile, "BasicWindow", "MultiviewDrawNames");
	ui->checkBox_DrawSourceNames->setChecked(multiviewDrawNames);

	m_bLoading = false;
}

void AFQProgramSettingAreaWidget::LoadMainAccount()
{
	auto& authManager = AFAuthManager::GetSingletonInstance();
	AFChannelData* mainChannel = nullptr;
	bool regMainAccount = authManager.GetMainChannelData(mainChannel);
	if (regMainAccount)
	{
		ui->stackedWidget_Account->setCurrentIndex(1);
		ui->label_Nickname->setText(mainChannel->pAuthData->strChannelNick.c_str());
		ui->label_IdNumber->setText(mainChannel->pAuthData->strChannelID.c_str());

        if (mainChannel && mainChannel->pObjQtPixmap != nullptr)
        {
            QPixmap* savedPixmap = (QPixmap*)mainChannel->pObjQtPixmap;
            ui->label_ProfilePicture->setPixmap(*savedPixmap);
        }
        else if(mainChannel && mainChannel->pAuthData)
         {
            QPixmap* newPixmap = App()->GetMainView()->MakePixmapFromAuthData(mainChannel->pAuthData);
            if (newPixmap != nullptr) {
                mainChannel->pObjQtPixmap = newPixmap;
                ui->label_ProfilePicture->setPixmap(*newPixmap);
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
	config_t* configGlobalFile = AFConfigManager::GetSingletonInstance().GetGlobal();

	int languageIndex = ui->comboBox_Language->currentIndex();
	QVariant langData = ui->comboBox_Language->itemData(languageIndex);
	std::string language = langData.toString().toStdString();

	

	if (AFSettingUtils::WidgetChanged(ui->comboBox_Language))
	{
		config_set_string(configGlobalFile, "General", "Language",
			language.c_str());
		if (m_iCurrentLanguageIndex != languageIndex)
		{
			m_bRestartNeeded = true;
		}
	}

	int themeIndex = ui->comboBox_Theme->currentIndex();
	QString themeData = ui->comboBox_Theme->itemData(themeIndex).toString();

	if (AFSettingUtils::WidgetChanged(ui->comboBox_Theme)) {
		m_sSavedTheme = themeData.toStdString();
		config_set_string(configGlobalFile, "General", "CurrentTheme3",
			QT_TO_UTF8(themeData));
	}

#ifdef _WIN32
	if (ui->checkBox_HideANETAWindowsFromCapture && AFSettingUtils::WidgetChanged(ui->checkBox_HideANETAWindowsFromCapture)) {
		bool hide_window = ui->checkBox_HideANETAWindowsFromCapture->isChecked();
		config_set_bool(configGlobalFile, "BasicWindow",
			"HideOBSWindowsFromCapture", hide_window);

		QWindowList windows = QGuiApplication::allWindows();
		for (auto window : windows) {
			if (window->isVisible()) {
				App()->GetMainView()->SetDisplayAffinity(window);
			}
		}

		blog(LOG_INFO, "Hide OBS windows from screen capture: %s",
			hide_window ? "true" : "false");
	}
#endif
	if (AFSettingUtils::WidgetChanged(ui->checkBox_GeneralOpenStatsOnStartup))
		config_set_bool(AFConfigManager::GetSingletonInstance().GetBasic(), "General", "OpenStatsOnStartup",
			ui->checkBox_GeneralOpenStatsOnStartup->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_SnappingEnabled))
		config_set_bool(configGlobalFile, "BasicWindow",
			"SnappingEnabled",
			ui->checkBox_SnappingEnabled->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_ScreenSnapping))
		config_set_bool(configGlobalFile, "BasicWindow",
			"ScreenSnapping",
			ui->checkBox_ScreenSnapping->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_CenterSnapping))
		config_set_bool(configGlobalFile, "BasicWindow",
			"CenterSnapping",
			ui->checkBox_CenterSnapping->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_SourceSnapping))
		config_set_bool(configGlobalFile, "BasicWindow",
			"SourceSnapping",
			ui->checkBox_SourceSnapping->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->doubleSpinBox_SnapDistance))
		config_set_double(configGlobalFile, "BasicWindow",
			"SnapDistance", ui->doubleSpinBox_SnapDistance->value());

	if (AFSettingUtils::WidgetChanged(ui->checkBox_OverflowAlwaysVisible) ||
		AFSettingUtils::WidgetChanged(ui->checkBox_OverflowHidden) ||
		AFSettingUtils::WidgetChanged(ui->checkBox_OverflowSelectionHidden)) {
		config_set_bool(configGlobalFile, "BasicWindow",
			"OverflowAlwaysVisible",
			ui->checkBox_OverflowAlwaysVisible->isChecked());
		config_set_bool(configGlobalFile, "BasicWindow",
			"OverflowHidden",
			ui->checkBox_OverflowHidden->isChecked());
		config_set_bool(configGlobalFile, "BasicWindow",
			"OverflowSelectionHidden",
			ui->checkBox_OverflowSelectionHidden->isChecked());

		App()->GetMainView()->GetMainWindow()->UpdatePreviewOverflowSettings();
	}

	if (AFSettingUtils::WidgetChanged(ui->checkBox_PreviewDrawSafeAreas)) {
		config_set_bool(configGlobalFile, "BasicWindow",
			"ShowSafeAreas",
			ui->checkBox_PreviewDrawSafeAreas->isChecked());

		App()->GetMainView()->GetMainWindow()->UpdatePreviewSafeAreas();
	}

	if (AFSettingUtils::WidgetChanged(ui->checkBox_SpacingHelpers)) {
		config_set_bool(configGlobalFile, "BasicWindow",
			"SpacingHelpersEnabled",
			ui->checkBox_SpacingHelpers->isChecked());

		App()->GetMainView()->GetMainWindow()->UpdatePreviewSpacingHelpers();
	}

	if (AFSettingUtils::WidgetChanged(ui->checkBox_SwitchOnDoubleClick))
		config_set_bool(configGlobalFile, "BasicWindow",
			"TransitionOnDoubleClick",
			ui->checkBox_SwitchOnDoubleClick->isChecked());

	if (AFSettingUtils::WidgetChanged(ui->checkBox_AutomaticCollectionSearch))
		config_set_bool(configGlobalFile, "General",
			"AutomaticCollectionSearch",
			false);

	config_set_bool(configGlobalFile, "BasicWindow",
		"WarnBeforeStartingStream",
		ui->checkBox_WarnBeforeStartingStream->isChecked());
	config_set_bool(configGlobalFile, "BasicWindow",
		"WarnBeforeStoppingStream",
		ui->checkBox_WarnBeforeStoppingStream->isChecked());
	config_set_bool(configGlobalFile, "BasicWindow",
		"WarnBeforeStoppingRecord",
		ui->checkBox_WarnBeforeStoppingRecord->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_RecordWhenStreaming))
		config_set_bool(configGlobalFile, "BasicWindow",
			"RecordWhenStreaming",
			ui->checkBox_RecordWhenStreaming->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_KeepRecordingWhenStreamStops))
		config_set_bool(configGlobalFile, "BasicWindow",
			"KeepRecordingWhenStreamStops",
			ui->checkBox_KeepRecordingWhenStreamStops->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_ReplayBufferWhileStreaming))
		config_set_bool(configGlobalFile, "BasicWindow",
			"ReplayBufferWhileStreaming",
			ui->checkBox_ReplayBufferWhileStreaming->isChecked());
	if (AFSettingUtils::WidgetChanged(ui->checkBox_KeepReplayBufferStreamStops))
		config_set_bool(configGlobalFile, "BasicWindow",
			"KeepReplayBufferStreamStops",
			ui->checkBox_KeepReplayBufferStreamStops->isChecked());

	bool studioModeReset = false;

	if (AFSettingUtils::WidgetChanged(ui->checkBox_StudioPortraitLayout)) {
		config_set_bool(configGlobalFile, "BasicWindow",
			"StudioPortraitLayout",
			ui->checkBox_StudioPortraitLayout->isChecked());
		studioModeReset = true;
	}

	if (AFSettingUtils::WidgetChanged(ui->checkBox_TogglePreviewProgramLabels)) {
		config_set_bool(configGlobalFile, "BasicWindow",
			"StudioModeLabels",
			ui->checkBox_TogglePreviewProgramLabels->isChecked());
		studioModeReset = true;
	}

	if (studioModeReset)
	{
		bool changeLayout = (m_bStudioModeVertical == ui->checkBox_StudioPortraitLayout->isChecked()) ? false : true;

		App()->GetMainView()->ResetStudioModeUI(changeLayout);
	}
	m_bStudioModeVertical = ui->checkBox_StudioPortraitLayout->isChecked();

	bool multiviewChanged = false;
	if (AFSettingUtils::WidgetChanged(ui->checkBox_MouseSwitch)) {
		config_set_bool(configGlobalFile, "BasicWindow",
			"MultiviewMouseSwitch",
			ui->checkBox_MouseSwitch->isChecked());
		multiviewChanged = true;
	}
	if (AFSettingUtils::WidgetChanged(ui->checkBox_DrawSourceNames)) {
		config_set_bool(configGlobalFile, "BasicWindow",
			"MultiviewDrawNames",
			ui->checkBox_DrawSourceNames->isChecked());
		multiviewChanged = true;
	}

	if (multiviewChanged)
		App()->GetMainView()->RefreshSceneUI();
}

void AFQProgramSettingAreaWidget::ResetProgramSettings() {
    int idxBefore = ui->comboBox_Language->currentIndex();
    AFConfigManager::GetSingletonInstance().ResetProgramConfig();
    LoadProgramSettings();
    int idxAfter = ui->comboBox_Language->currentIndex();

    if (idxBefore != idxAfter) {
        m_bRestartNeeded = true;
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

	config_set_bool(AFConfigManager::GetSingletonInstance().GetGlobal(), "BasicWindow", "ProgramSettingPageSpread", bPageSpread);
}

void AFQProgramSettingAreaWidget::_LoadLanguageList()
{
    const char* currentLang = config_get_string(AFConfigManager::GetSingletonInstance().GetGlobal(), "General", "Language");
    if (!currentLang || currentLang[0] == '\0')
		currentLang = AFLocaleTextManager::GetSingletonInstance().GetCurrentLocale();

    ui->comboBox_Language->clear();

    for (const auto& locale : AFLocaleTextManager::GetSingletonInstance().GetLocaleNames()) {
        int idx = ui->comboBox_Language->count();

        ui->comboBox_Language->addItem(QT_UTF8(locale.second.c_str()),
            QT_UTF8(locale.first.c_str()));

		qDebug() << QT_UTF8(locale.first.c_str());


		if (locale.first == currentLang)
		{
			ui->comboBox_Language->setCurrentIndex(idx);
		}
    }
    ui->comboBox_Language->model()->sort(0);
	m_iCurrentLanguageIndex = ui->comboBox_Language->currentIndex();

}
void AFQProgramSettingAreaWidget::_LoadThemeList()
{
}

void AFQProgramSettingAreaWidget::_SetProgramSettingSignal()
{
	AFSettingUtils::HookWidget(ui->comboBox_Language, this, COMBO_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->comboBox_Theme, this, COMBO_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_GeneralOpenStatsOnStartup, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_HideANETAWindowsFromCapture, this, CHECK_CHANGED, GENERAL_CHANGED);

	AFSettingUtils::HookWidget(ui->checkBox_WarnBeforeStartingStream, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_WarnBeforeStoppingStream, this, CHECK_CHANGED, GENERAL_CHANGED);
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

	AFSettingUtils::HookWidget(ui->checkBox_SwitchOnDoubleClick, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_StudioPortraitLayout, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_TogglePreviewProgramLabels, this, CHECK_CHANGED, GENERAL_CHANGED);

	AFSettingUtils::HookWidget(ui->checkBox_MouseSwitch, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_DrawSourceNames, this, CHECK_CHANGED, GENERAL_CHANGED);
	AFSettingUtils::HookWidget(ui->checkBox_MultiviewDrawSafeAreas, this, CHECK_CHANGED, GENERAL_CHANGED);

	AFSettingUtils::HookWidget(ui->checkBox_PopupPos, this, CHECK_CHANGED, GENERAL_CHANGED);

	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->label_SnapDistance, &QLabel::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->doubleSpinBox_SnapDistance, &QDoubleSpinBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_ScreenSnapping, &QCheckBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_SourceSnapping, &QCheckBox::setEnabled);
	connect(ui->checkBox_SnappingEnabled, &QCheckBox::toggled, ui->checkBox_CenterSnapping, &QCheckBox::setEnabled);

	connect(ui->checkBox_OverflowHidden, &QCheckBox::toggled, this, &AFQProgramSettingAreaWidget::qslotDisableOverFlowCheckBox);
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
		label->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(label->text().toUtf8().constData())));
    }

    foreach(QCheckBox * checkbox, checkboxList)
    {
        checkbox->setText(QT_UTF8(AFLocaleTextManager::GetSingletonInstance().Str(checkbox->text().toUtf8().constData())));

    }
}
