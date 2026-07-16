#include "CSettingBroadInfoWidget.h"
#include "ui_setting-broad-info.h"

#include "qt-wrappers.hpp"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"

#include "CSettingUtils.h"
#include "PopupWindows/SettingPopup/CSettingStreamAreaWidget.h"
#include "PopupWindows/BroadInfoPopup/CPasswordSettingDialog.h"
#include "PopupWindows/BroadInfoPopup/CWatermarkPositionSettingDialog.h"
#include "PopupWindows/BroadInfoPopup/CAgeRestrictionPolicyDialog.h"
#include "PopupWindows/BroadInfoPopup/CCategoryDialog.h"
#include "PopupWindows/BroadInfoPopup/CAddTagDialog.h"
#include "Utils/BreaktimeManager.h"
#include "Common/StudioDefine.h"
#include "CoreModel/Source/CSource.h"

#define BROADINFO_CHANGED   &AFQSettingBroadInfoWidget::_qslotDataChanged
#define TEXT_CHANGED		&AFQLengthAwareLineEdit::qsignalTextChanged
#define BUTTON_TOGGLED		&QPushButton::toggled


AFQSettingBroadInfoWidget::AFQSettingBroadInfoWidget(QWidget* parent) :
	QWidget(parent),
	ui(new Ui::AFQSettingBroadInfoWidget)
{
	ui->setupUi(this);

	m_streamSetting = qobject_cast<AFQStreamSettingAreaWidget*>(parent);

	_Init();
}

AFQSettingBroadInfoWidget::~AFQSettingBroadInfoWidget() 
{
	_StopReceivingActiveItemInfoTimer();

	if (m_stickerItemsDialog) {
		m_stickerItemsDialog->close();
		m_stickerItemsDialog = nullptr;
	}

	m_tempStreamTags.clear();

	delete ui;
}

void AFQSettingBroadInfoWidget::_qslotSetCurrentPage(BroadInfoPage page)
{
	switch (page) 
	{
	case BroadInfoPage::BroadInfo_Page: 
	{
		ui->pushButton_BroadInfo->setChecked(true);
		ui->pushButton_BroadAttribute->setChecked(false);
		ui->pushButton_Permission->setChecked(false);

		ui->stackedWidget_SettingBroadInfo->setCurrentWidget(ui->page_BroadInfo);
		break;
	}
	case BroadInfoPage::BroadAttribute_Page: 
	{
		ui->pushButton_BroadInfo->setChecked(false);
		ui->pushButton_BroadAttribute->setChecked(true);
		ui->pushButton_Permission->setChecked(false);

		ui->stackedWidget_SettingBroadInfo->setCurrentWidget(ui->page_BroadAttribute);
		break;
	}
	case BroadInfoPage::Permission_Page: 
	{
		ui->pushButton_BroadInfo->setChecked(false);
		ui->pushButton_BroadAttribute->setChecked(false);
		ui->pushButton_Permission->setChecked(true);

		ui->stackedWidget_SettingBroadInfo->setCurrentWidget(ui->page_Permission);
		break;
	}
	default:
		return;
	}
}

void AFQSettingBroadInfoWidget::_qslotClickedManagePermission(Permission permission)
{
	AFChannelData* pSoopChannel = nullptr;
	AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannel);
	if (!pSoopChannel)
		return;

	std::string strID = pSoopChannel->pAuthData->channelID;

	QString url = QString::fromStdString(SOOP_PERMISSION_URL) + QString::fromStdString(strID);

	switch (permission) 
	{
	case Permission::FanClub:
		url += SOOP_FANCLUB_URL;
		break;
	case Permission::Subscription:
		url += SOOP_SUBSCRIPTION_URL;
		break;
	case Permission::BlackList:
		url += SOOP_BLACKLIST_URL;
		break;
	case Permission::VODAuth:
		url += SOOP_VODAUTH_URL;
		break;
	case Permission::Editor:
		url += SOOP_EDITOR_URL;
		break;
	case Permission::UserClip_BlackList:
		url += SOOP_USERCLIP_BLACKLIST_URL;
		break;
	case Permission::SubscribeSetting:
		url = QString::fromStdString(SOOP_SUBSCRIBE_SETTING_URL);
		break;
	default:
		return;
	}
	MAINFRAME->NavigateDefaultBrowser(url);
}

void AFQSettingBroadInfoWidget::_qslotClickedHideStreamButton()
{
	if (ui->pushButton_HideStream->isChecked()) {
		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
								   QT_UTF8(""), QTStr("BroadInfo.HideBroad.Msg"));
	}
}

void AFQSettingBroadInfoWidget::_qslotClickedCategoryButton()
{
	if (AFOutputUtil::IsStreamActive()) {
		OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
		if (source) {
			obs_media_state media_state = obs_source_media_get_state(source);
			if (OBS_MEDIA_STATE_PLAYING == media_state ||
				OBS_MEDIA_STATE_OPENING == media_state ||
				OBS_MEDIA_STATE_BUFFERING == media_state ||
				OBS_MEDIA_STATE_PAUSED == media_state)
			{
				const char* id = obs_source_get_id(source);
				const char* name = obs_source_get_display_name(id);
				QString msg = QTStr("Caution.SOOPMediaSource.DisableMessage5").arg(name);
				AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
					"", msg, false, true, "", 0, 0, "type1");
				return;
			}
		}
	}

	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	broadInfo->RequestCategoryListAPI();

	AFQCategoryDialog* categoryDialog = new AFQCategoryDialog(MAINFRAME, m_tempCategoryNum);
	connect(categoryDialog, &AFQCategoryDialog::qsignalCategoryChanged,
		this, &AFQSettingBroadInfoWidget::_qslotCategoryChanged);

	categoryDialog->exec();
}

void AFQSettingBroadInfoWidget::_qslotClickedAddTagButton()
{
	AFQAddTagDialog* addTagDialog = new AFQAddTagDialog(this);
	connect(addTagDialog, &AFQAddTagDialog::qsignalTagChanged,
			this, &AFQSettingBroadInfoWidget::_qslotStreamTagChanged);

	addTagDialog->exec(); // Modal
}

void AFQSettingBroadInfoWidget::_qslotCategoryChanged(const std::string& categoryNum, const std::string& categoryName)
{
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();

	m_tempCategoryName = categoryName;
	m_tempCategoryNum = categoryNum;

	const int prevCategoryNum = broadInfo->CategoryNumber();
	const int tempCategoryNum = stoi(m_tempCategoryNum);

	bool adultContentCheck = SOOP_SRC_MANAGER.GetSoopVodAdultContentCheck(tempCategoryNum);
	if (adultContentCheck) {
		ui->pushButton_AgeRestricted->SetChecked(true);
	}
	else {
		adultContentCheck = SOOP_SRC_MANAGER.GetSoopVodAdultContentCheck(prevCategoryNum);
		if (adultContentCheck) {
			ui->pushButton_AgeRestricted->SetChecked(false);
		}
	}

	_RefreshCategoryName();
	_qslotDataChanged();
}

void AFQSettingBroadInfoWidget::_qslotStreamTagChanged(const std::vector<std::string>& tags)
{
	m_tempStreamTags = tags;

	_RefreshTags();
	_qslotDataChanged();
}

void AFQSettingBroadInfoWidget::_qslotClickedAdultOnlyButton()
{
	//쉬는시간 예외처리 
	if (BREAKTIME_MANAGER.IsActive())
	{
		bool current = ui->pushButton_AgeRestricted->isChecked();
		ui->pushButton_AgeRestricted->SetChecked(!current);

		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", QTStr("breaktime.broadset.restricted"), false, true, "", 0, 0, "type1");		
		return;
	}

	if (ui->pushButton_AgeRestricted->isChecked()) {
		const char* date = config_get_string(USERCONFIG, "BroadInfo", AGE_RESTRICTION_POLICY_DATE_CHECK);
		bool open = IsDateBeforeToday(date);
		if (!open)
			return;

		AFQAgeRestrictionPolicyDialog* adultAlertDialog = new AFQAgeRestrictionPolicyDialog(this);
		if (QDialog::Accepted != adultAlertDialog->exec()) {
			ui->pushButton_AgeRestricted->SetChecked(false);
			return;
		}
	}
	else
	{
		const int tempCategoryNum = stoi(m_tempCategoryNum);
		bool adultContentCheck = SOOP_SRC_MANAGER.GetSoopVodAdultContentCheck(tempCategoryNum);
		if (adultContentCheck) {
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
									   "", QTStr("Caution.SOOPMediaSource.DisableMessage7"), false, true);
			ui->pushButton_AgeRestricted->SetChecked(true);
			return;
		}

		OBSSource source = SOOP_SRC_MANAGER.GetSoopMediaSource();
		if (source) {
			if (AFOutputUtil::IsStreamActive())
			{
				const char* id = obs_source_get_id(source);
				VodInfo_s info = SOOP_SRC_MANAGER.GetCurVodInfo(id);
				if (info.is_adult)
				{
					AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
											   "", QTStr("Caution.SOOPMediaSource.DisableMessage6"), false, true);
					ui->pushButton_AgeRestricted->SetChecked(true);
					return;
				}
			}
		}
	}
}

void AFQSettingBroadInfoWidget::_qslotClickedUsePasswordButton()
{
	//쉬는시간 예외처리
	if (BREAKTIME_MANAGER.IsActive())
	{
		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
								   "", QTStr("breaktime.broadset.restricted"), false, true, "", 0, 0, "type1");
		return;
	}

	if (ui->pushButton_SubscribeBroad->isChecked())
	{
		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
								   "", QTStr("PasswordOn.Reject.Subscribe.Broad"), false, false, "", -1, 166);
		return;
	}

	AFQPasswordSettingDialog* passwordSettingDialog = new AFQPasswordSettingDialog(
															this,
															m_tempUsePassword,
															QT_UTF8(m_tempPassword.c_str()));

	connect(passwordSettingDialog, &AFQPasswordSettingDialog::qsignalClickedUsePassword,
			this, &AFQSettingBroadInfoWidget::_qslotUsePasswordChanged);

	passwordSettingDialog->exec();
}

void AFQSettingBroadInfoWidget::_qslotClickedWatermarkButton()
{
	AFQWatermarkPositionSettingDialog* watermarkDialog = new AFQWatermarkPositionSettingDialog(
																					this,
																					m_watermarkPosTexts,
																					m_tempWatermarkPos - 1);
	connect(watermarkDialog, &AFQWatermarkPositionSettingDialog::qsignalWatermarkPositionChanged,
			this, &AFQSettingBroadInfoWidget::_qslotWatermarkPositionChanged);

	watermarkDialog->exec();
}

void AFQSettingBroadInfoWidget::_qslotClickedStickerItemButton()
{
	if (!m_stickerItemsDialog)
		m_stickerItemsDialog = new AFQStickerItemsDialog(this);
	m_stickerItemsDialog->exec();
}

void AFQSettingBroadInfoWidget::_qslotClickedSubscribeBroadButton(bool checked)
{
	if (m_tempUsePassword)
		ui->pushButton_SubscribeBroad->SetChecked(false);

	bool sendSubscribeBroad = true;

	QMap<QString, AFQStreamAccount*> liveList =  m_streamSetting->GetLiveChannels();

	if (AFOutputUtil::IsStreamActive())
	{
		AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
								   "", QTStr("StreamOn.Reject.Subscribe.Broad"), false, true, "", -1, 166);
		sendSubscribeBroad = false;
	}
	else if (liveList.contains(PLATFORM_SOOP) && liveList.count() > 1)
	{
		if (checked)
		{
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
									   "", QTStr("Simulcast.Reject.Subscribe.Broad"), false, true, "", -1, 166);
			sendSubscribeBroad = false;
		}
	}
	else if (ui->widget_UsePassword->property("usePassword").toBool())
	{
		if (checked)
		{
			AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
									   "", QTStr("PasswordOn.Reject.Subscribe.Broad"), false, true, "", -1, 166);
			sendSubscribeBroad = false;

		}
	}

	if (sendSubscribeBroad)
	{
		int subTier = checked ? 2 : 0;
		AUTH_CONTEXT.GetSoopBroadInfo()->SetSubscribeBroad(subTier);
		//AUTH_CONTEXT.SendSoopBroadInfoSetting(true);
	}
	else
	{
		AFQToggleButton* sub = reinterpret_cast<AFQToggleButton*>(sender());
		sub->blockSignals(true);
		sub->SetChecked(!checked);
		sub->blockSignals(false);
	}
}

//AFBroadInfoDockWidget::qslotReceiveSubscribeBroadAvailable
void AFQSettingBroadInfoWidget::_qslotReceiveSubscribeBroadAvailableSetting(const QByteArray& responseData)
{
	bool currentSubBroadState = ui->pushButton_SubscribeBroad->isChecked();

	std::string jsonString = responseData.toStdString();
	std::string err;
	err.clear();

	AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", QTStr("API.Failed"), false, true, "", -1, 200);

	ui->pushButton_SubscribeBroad->setChecked(!currentSubBroadState);
	ui->pushButton_SubscribeBroad->ChangeState(!currentSubBroadState);
}

void AFQSettingBroadInfoWidget::_qslotUsePasswordChanged(bool usePassword, const QString& password)
{
	m_tempUsePassword = usePassword;
	m_tempPassword = QT_TO_UTF8(password);
	
	bool passwordProp = ui->widget_UsePassword->property("usePassword").toBool();

	_qslotDataChanged();

	// Change only if the state is different
	if (passwordProp == usePassword)
		return;

	ui->widget_UsePassword->setProperty("usePassword", usePassword);

	if (usePassword)
		ui->label_PasswordUsage->setText(QTStr("InUse"));
	else
		ui->label_PasswordUsage->setText(QTStr("NoUse"));

}

void AFQSettingBroadInfoWidget::_qslotWatermarkPositionChanged(int watermarkPos)
{
	m_tempWatermarkPos = watermarkPos + 1;

	QString watermarkPositionText = _GetWatermarkPosTextByIndex(watermarkPos);
	ui->label_WatermarkPos->setText(watermarkPositionText);

	_qslotDataChanged();
}

void AFQSettingBroadInfoWidget::_qslotDataChanged()
{
	if (!m_loading)
	{
		m_dataChanged = true;
		sender()->setProperty("changed", QVariant(true));

		emit qsignalDataChanged();
	}
}

void AFQSettingBroadInfoWidget::_qslotTitleFocus(bool focusin)
{
	AFQFocusAwareLineEdit* lineEdit = qobject_cast<AFQFocusAwareLineEdit*>(sender());
	if (focusin)
		m_previousTitle = lineEdit->text();
	else
		if (lineEdit->text() == "")
			lineEdit->setText(m_previousTitle);
}


void AFQSettingBroadInfoWidget::_qslotSendBroadInfoReceived(int result, QString msg)
{
	blockSignals(true);
	if (result == 1)
		LoadBroadInfoDatas();
	blockSignals(false);
}

void AFQSettingBroadInfoWidget::_qslotRequestBroadInfoReceived(int result, QString msg)
{
	blockSignals(true);
	if (result == 1)
		LoadBroadInfoDatas();
	else
	{
		AUTH_CONTEXT.RequestBroadInfoAPI();
	}

	blockSignals(false);
}


void AdjustButtonSize(QPushButton* button)
{
	if (button) {
		QString text = button->text();

		QFont boldFont = button->font();
		boldFont.setWeight(QFont::DemiBold);
		QFontMetrics fm(boldFont);

		int width = fm.horizontalAdvance(text);
		button->setMinimumWidth(width);
	}
}

void AFQSettingBroadInfoWidget::_Init()
{
	// Top Menu
	ui->pushButton_BroadInfo->setCheckable(true);
	ui->pushButton_BroadAttribute->setCheckable(true);
	ui->pushButton_Permission->setCheckable(true);

	ui->pushButton_BroadInfo->setChecked(true);
	ui->pushButton_BroadAttribute->setChecked(false);
	ui->pushButton_Permission->setChecked(false);

	std::string absPath;
	GetDataFilePath("assets", absPath);
	QString categoryArrow = QString("%1/common-component/default/ic_category_arrow.svg").arg(absPath.c_str());

	QPixmap rawIcon(categoryArrow);
	QPixmap shiftedIcon(rawIcon.size());
	shiftedIcon.fill(Qt::transparent);

	QPainter painter(&shiftedIcon);
	painter.drawPixmap(0, 1, rawIcon);
	painter.end();

	ui->pushButton_BroadCategory->setIcon(QIcon(shiftedIcon));

	ui->pushButton_BroadCategory->setLayoutDirection(Qt::RightToLeft);

	ui->pushButton_AddTags->setIconSize(QSize(19, 19));
	ui->pushButton_AddTags->setProperty("buttonType", "addTagButton");
	ui->pushButton_AddTags->setToolTip(QTStr("BroadInfo.AddTagTooltip"));

	// Set LineEdit Max Text Length
	ui->widget_BroadTitle->SetMaxLength(75);
	ui->widget_EndingMessage->SetMaxLength(40);

	//
	ui->label_SubscribeSetting->SetElidedAnimation(true);

	// Watermark Locale
	_InitWatermarkLocale();

	// Set ToolTip Text
	_InitToolTip();
	// Set Current Page
	_qslotSetCurrentPage(BroadInfoPage::BroadInfo_Page);
	// Connect Signals
	_ConnectEvents();
	// Load Broad Info
	LoadBroadInfoDatas();

	_StartReceivingActiveItemInfoTimer();

	_ConnectDataChangeEvents();

	repaint();
	QApplication::processEvents();

	AdjustButtonSize(ui->pushButton_BroadInfo);
	AdjustButtonSize(ui->pushButton_BroadAttribute);
	AdjustButtonSize(ui->pushButton_Permission);

	m_loading = false;
}

void AFQSettingBroadInfoWidget::_InitToolTip()
{
	ui->pushButton_BroadInfoLangTip->SetExplanationText(QTStr("BroadInfo.Lang.ToolTip"), ENUM_TOOLTIP_POSITION::BottomCenter); // 방송 언어
	
	ui->pushButton_AgeRestrictedTip->SetExplanationText(QTStr("BroadInfo.AgeRestricted.ToolTip"), ENUM_TOOLTIP_POSITION::BottomCenter);
	ui->pushButton_RejectVisitTip->SetExplanationText(QTStr("BroadInfo.RejectVisit.ToolTip"), ENUM_TOOLTIP_POSITION::BottomCenter);
	ui->pushButton_HideStreamTip->SetExplanationText(QTStr("BroadInfo.HideBroad.ToolTip"), ENUM_TOOLTIP_POSITION::BottomCenter);
	ui->pushButton_PaidAdsTip->SetExplanationText(QTStr("BroadInfo.PaidAds.ToolTip"), ENUM_TOOLTIP_POSITION::BottomCenter);
	ui->pushButton_SubscribeBroadTip->SetExplanationText(QTStr("Subscribe.Broad.Info"), ENUM_TOOLTIP_POSITION::BottomCenter);
	ui->pushButton_SubscribeSettingTip->SetExplanationText(QTStr("BroadInfo.SubscribeSetting.Tooltip"), ENUM_TOOLTIP_POSITION::BottomCenter);
	
}

void AFQSettingBroadInfoWidget::_InitWatermarkLocale()
{
	m_watermarkPosTexts.clear();
	m_watermarkPosTexts.push_back(QT_UTF8(Str("TopLeft")));
	m_watermarkPosTexts.push_back(QT_UTF8(Str("TopCenter")));
	m_watermarkPosTexts.push_back(QT_UTF8(Str("TopRight")));
	m_watermarkPosTexts.push_back(QT_UTF8(Str("BottomLeft")));
	m_watermarkPosTexts.push_back(QT_UTF8(Str("BottomCenter")));
	m_watermarkPosTexts.push_back(QT_UTF8(Str("BottomRight")));
}

bool AFQSettingBroadInfoWidget::SaveSettings()
{
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	//
	QString prevTitle = QString::fromStdString(broadInfo->Title());
	QString editedTitle = ui->widget_BroadTitle->GetText();
	if (prevTitle != editedTitle)
	{
		QString filterTitle;
		for (QChar ch : editedTitle) {
			if (ch.unicode() != 0x03 && ch.unicode() != 0x04 &&
				ch.unicode() != 0x05 && ch.unicode() != 0x06 &&
				ch.unicode() != 0x10 && ch.unicode() != 0x13)
			{
				filterTitle.append(ch);
			}
		}
		editedTitle = filterTitle;

		editedTitle.replace('\r', ' ');
		editedTitle.replace('\n', ' ');

		editedTitle = editedTitle.trimmed();
	}

	broadInfo->SetTitle(QT_TO_UTF8(editedTitle));
	if (stoi(m_tempCategoryNum) != broadInfo->CategoryNumber()) {
		AFQSceneListItem* clickedItem = SCENE_CONTEXT.GetCurSelectedSceneItem();

		obs_source_t* src = obs_scene_get_source(clickedItem->GetScene());

		MAINFRAME->SetCurrentScene(src);

		struct FindMinsimChk {
			bool found = false;
			int nCnt = 0;
		} findMinsimChk;
		FindMinsimChk info;

		obs_scene_enum_items(clickedItem->GetScene(), [](obs_scene_t*, obs_sceneitem_t* item, void* param)->bool {
			auto* f = static_cast<FindMinsimChk*>(param);
			auto src = obs_sceneitem_get_source(item);
			const char* id = obs_source_get_id(src);
			if (0 == strcmp(id, "soop_chat_source_mood_check")) {
				bool bVisible = obs_source_showing(src);
				if (bVisible) {
					f->found = true;
					f->nCnt += 1;
				}
			}
			return true;
		}, &info);
	}

	broadInfo->SetCategory(stoi(m_tempCategoryNum));
	broadInfo->SetHashTags(m_tempStreamTags);
	broadInfo->Lang(ui->comboBox_Lang->currentData().toString().toUtf8().constData());
	broadInfo->SetEndingMessage(QT_TO_UTF8(ui->widget_EndingMessage->GetText()));

	// Broad Attribute Page
	broadInfo->SetAdultOnly(ui->pushButton_AgeRestricted->isChecked());

	broadInfo->SetUsePassword(m_tempUsePassword);
	broadInfo->SetPassword(m_tempPassword);
	broadInfo->SetBroadTuneOut(ui->pushButton_RejectVisit->isChecked());
	broadInfo->SetBroadHidden(ui->pushButton_HideStream->isChecked());
	broadInfo->SetPaidPromotion(ui->pushButton_PaidAds->isChecked());
	broadInfo->SetSubscribeBroad(ui->pushButton_SubscribeBroad->isChecked());
	
	config_set_bool(USERCONFIG, "BroadInfo", "NotifyOnTitleChange",
					ui->pushButton_TitleChangedPopup->isChecked());

	broadInfo->SetWaterMarkPosition(m_tempWatermarkPos);

	bool sendSub = false;
	if (m_previousSubscribe != ui->pushButton_SubscribeBroad->isChecked())
		sendSub = true;

	AUTH_CONTEXT.SendSoopBroadInfoSetting(sendSub);

	m_previousSubscribe = ui->pushButton_SubscribeBroad->isChecked();

	AFSourceUtil::ChangeAIManagerUrl();

	return true;
}

void AFQSettingBroadInfoWidget::LoadBroadInfoDatas()
{
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();

	// Save temporary broad info datas
	m_tempPassword = broadInfo->Password();
	m_tempUsePassword = broadInfo->UsePassword();
	m_tempStreamTags = broadInfo->HashTags();
	m_tempWatermarkPos = broadInfo->WaterMarkPosition();
	m_tempCategoryName = broadInfo->Category();

	m_tempCategoryNum = broadInfo->CategoryNumberString(broadInfo->CategoryNumber());

	// Broad info page
	ui->widget_BroadTitle->SetText(QT_UTF8(broadInfo->Title().c_str()));
	m_titleOnLoad = QT_UTF8(broadInfo->Title().c_str());

	ui->pushButton_BroadCategory->setText(QT_UTF8(broadInfo->Category().c_str()));
	ui->widget_EndingMessage->SetText(QT_UTF8(broadInfo->EndingMessage().c_str()));

	_RefreshCategoryName();

	bool addTag = _RefreshTags();
	
	ui->comboBox_Lang->clear();
	for (const auto& item : broadInfo->LangVector())
		ui->comboBox_Lang->addItem(item.first.c_str(), item.second.c_str());

	auto index = ui->comboBox_Lang->findData(broadInfo->Lang().c_str());
	if (index != -1)
		ui->comboBox_Lang->setCurrentIndex(index);

	// Broad attribute page
	ui->pushButton_AgeRestricted->SetChecked(broadInfo->AdultOnly());
	ui->pushButton_RejectVisit->SetChecked(broadInfo->BroadTuneOut());
	ui->pushButton_HideStream->SetChecked(broadInfo->BroadHidden());
	ui->pushButton_PaidAds->SetChecked(broadInfo->PaidPromotion());

	ui->widget_UsePassword->setProperty("usePassword", broadInfo->UsePassword());
	if (broadInfo->UsePassword())
		ui->label_PasswordUsage->setText(QTStr("InUse"));
	else
		ui->label_PasswordUsage->setText(QTStr("NoUse"));

	ui->pushButton_SubscribeBroad->SetChecked(broadInfo->SubscribeBroad());
	m_previousSubscribe = broadInfo->SubscribeBroad();
	
	int watermarkPositionIdx = broadInfo->WaterMarkPosition() - 1;
	QString watermarkPositionText = _GetWatermarkPosTextByIndex(watermarkPositionIdx);
	ui->label_WatermarkPos->setText(watermarkPositionText);
	ui->pushButton_TitleChangedPopup->SetChecked(config_get_bool(USERCONFIG, "BroadInfo", "NotifyOnTitleChange"));
}

bool AFQSettingBroadInfoWidget::GetSubscribeLive()
{
	return ui->pushButton_SubscribeBroad->isChecked();
}

void AFQSettingBroadInfoWidget::TabButtonClick(BroadInfoPage tabNum)
{
	switch (tabNum)
	{
	case BroadInfoPage::BroadInfo_Page:
		ui->pushButton_BroadInfo->clicked();
		break;
	case BroadInfoPage::BroadAttribute_Page:
		ui->pushButton_BroadAttribute->clicked();
		break;
	case BroadInfoPage::Permission_Page:
		ui->pushButton_Permission->clicked();
		break;
	}
}

void AFQSettingBroadInfoWidget::_ConnectEvents()
{
	AFQBroadInfo* broadinfo = AUTH_CONTEXT.GetSoopBroadInfo();
	connect(broadinfo, &AFQBroadInfo::qsignalSendBroadInfoResult, this, &AFQSettingBroadInfoWidget::_qslotSendBroadInfoReceived);
	connect(broadinfo, &AFQBroadInfo::qsignalBroadInfoReceived, this, &AFQSettingBroadInfoWidget::_qslotRequestBroadInfoReceived);

	// Top menu
	connect(ui->pushButton_BroadInfo, &QPushButton::clicked,
			this, [this] {_qslotSetCurrentPage(BroadInfoPage::BroadInfo_Page); });
	connect(ui->pushButton_BroadAttribute, &QPushButton::clicked,
			this, [this] {_qslotSetCurrentPage(BroadInfoPage::BroadAttribute_Page); });
	connect(ui->pushButton_Permission, &QPushButton::clicked,
			this, [this] {_qslotSetCurrentPage(BroadInfoPage::Permission_Page); });

	AFQFocusAwareLineEdit* lineedit = ui->widget_BroadTitle->GetLineEdit();
	connect(lineedit, &AFQFocusAwareLineEdit::qsignalFocusChanged, this, &AFQSettingBroadInfoWidget::_qslotTitleFocus);

	connect(ui->pushButton_AddTags, &QPushButton::clicked, this, &AFQSettingBroadInfoWidget::_qslotClickedAddTagButton);
	connect(ui->pushButton_BroadCategory, &QPushButton::clicked, this, &AFQSettingBroadInfoWidget::_qslotClickedCategoryButton);

	connect(ui->comboBox_Lang, &QComboBox::currentIndexChanged, this, [this]() { _qslotDataChanged(); });

	// Broad attribute page
	connect(ui->pushButton_AgeRestricted, &QPushButton::clicked, this, &AFQSettingBroadInfoWidget::_qslotClickedAdultOnlyButton);
	connect(ui->pushButton_HideStream, &QPushButton::clicked, this, &AFQSettingBroadInfoWidget::_qslotClickedHideStreamButton);
	connect(ui->widget_UsePassword, &AFQHoverWidget::qsignalMouseClick, this, &AFQSettingBroadInfoWidget::_qslotClickedUsePasswordButton);
	connect(ui->widget_WatermarkPos, &AFQHoverWidget::qsignalMouseClick, this, &AFQSettingBroadInfoWidget::_qslotClickedWatermarkButton);
	connect(ui->widget_StickerItem, &AFQHoverWidget::qsignalMouseClick, this, &AFQSettingBroadInfoWidget::_qslotClickedStickerItemButton);

	//subscribe
	connect(ui->pushButton_SubscribeBroad, &QPushButton::clicked, this, &AFQSettingBroadInfoWidget::_qslotClickedSubscribeBroadButton);
	//subscribe

	// Permission page
	connect(ui->widget_ManageFanClub, &AFQHoverWidget::qsignalMouseClick,
			this, [this] {_qslotClickedManagePermission(Permission::FanClub); });
	connect(ui->widget_ManageVODPermission, &AFQHoverWidget::qsignalMouseClick,
			this, [this] {_qslotClickedManagePermission(Permission::VODAuth); });
	connect(ui->widget_ManageSupporter, &AFQHoverWidget::qsignalMouseClick,
			this, [this] {_qslotClickedManagePermission(Permission::Subscription); });
	connect(ui->widget_ManageEditPermission, &AFQHoverWidget::qsignalMouseClick,
			this, [this] {_qslotClickedManagePermission(Permission::Editor); });
	connect(ui->widget_ManageBlackList, &AFQHoverWidget::qsignalMouseClick,
			this, [this] {_qslotClickedManagePermission(Permission::BlackList); });
	connect(ui->widget_ManageClip, &AFQHoverWidget::qsignalMouseClick,
			this, [this] {_qslotClickedManagePermission(Permission::UserClip_BlackList); });
	connect(ui->widget_SubscribeSetting, &AFQHoverWidget::qsignalMouseClick,
		this, [this] {_qslotClickedManagePermission(Permission::SubscribeSetting); });

}

void AFQSettingBroadInfoWidget::_ConnectDataChangeEvents()
{
	AFSettingUtils::HookWidget(ui->widget_BroadTitle, this, TEXT_CHANGED, BROADINFO_CHANGED);
	AFSettingUtils::HookWidget(ui->widget_EndingMessage, this, TEXT_CHANGED, BROADINFO_CHANGED);
	AFSettingUtils::HookWidget(ui->pushButton_AgeRestricted, this, BUTTON_CLICKED, BROADINFO_CHANGED);
	AFSettingUtils::HookWidget(ui->pushButton_RejectVisit, this, BUTTON_CLICKED, BROADINFO_CHANGED);
	AFSettingUtils::HookWidget(ui->pushButton_HideStream, this, BUTTON_CLICKED, BROADINFO_CHANGED);
	AFSettingUtils::HookWidget(ui->pushButton_PaidAds, this, BUTTON_CLICKED, BROADINFO_CHANGED);
	AFSettingUtils::HookWidget(ui->pushButton_TitleChangedPopup, this, BUTTON_CLICKED, BROADINFO_CHANGED);
	AFSettingUtils::HookWidget(ui->pushButton_SubscribeBroad, this, BUTTON_TOGGLED, BROADINFO_CHANGED);
}

void AFQSettingBroadInfoWidget::_RefreshCategoryName()
{
	std::string fullCategoryName = AUTH_CONTEXT.FullCategoryName(stoi(m_tempCategoryNum));
	QString displayText = QString::fromStdString(fullCategoryName);
	if (displayText.isEmpty())
		displayText = "";

	ui->pushButton_BroadCategory->setText(QString::fromStdString(m_tempCategoryName));
	ui->pushButton_BroadCategory->setToolTip(displayText);
}

#define ACTIVE_ITEM_INFO_REFRESH_TIME 1000 * 10 // 10 sec
void AFQSettingBroadInfoWidget::_StartReceivingActiveItemInfoTimer()
{
	if (!m_receiveActiveItemInfoTimer)
	{
		m_receiveActiveItemInfoTimer = new QTimer(this);
		m_receiveActiveItemInfoTimer->setInterval(ACTIVE_ITEM_INFO_REFRESH_TIME);
		connect(m_receiveActiveItemInfoTimer, &QTimer::timeout, 
				this, &AFQSettingBroadInfoWidget::_qslotReceiveActiveItemInfo);
	}

	_qslotReceiveActiveItemInfo();

	if (!m_receiveActiveItemInfoTimer->isActive())
		m_receiveActiveItemInfoTimer->start();
}

void AFQSettingBroadInfoWidget::_StopReceivingActiveItemInfoTimer()
{
	if (m_receiveActiveItemInfoTimer)
	{
		m_receiveActiveItemInfoTimer->stop();
		delete m_receiveActiveItemInfoTimer;
		m_receiveActiveItemInfoTimer = nullptr;
	}
}

void AFQSettingBroadInfoWidget::_qslotReceiveActiveItemInfo()
{
	AUTH_CONTEXT.RequestStickerItemInfo();

	// Update UI
	QString stickerUsageText;
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	if (broadInfo->IsUsingItems()) {
		stickerUsageText = QTStr("InUse");
	}
	else if(broadInfo->IsHavingItems()){
		stickerUsageText = QTStr("InHave");
	}
	else {
		stickerUsageText = QTStr("StickerItem.Show");
	}
	
	ui->label_StickerItemUsage->setText(stickerUsageText);

	if (m_stickerItemsDialog && m_stickerItemsDialog->isVisible())
		m_stickerItemsDialog->UpdateUI();
}

bool AFQSettingBroadInfoWidget::_RefreshTags()
{
	RemoveAllChildInLayout(ui->scrollAreaWidgetContents_Tags->layout());

	bool retVal = _AddTags(m_tempStreamTags);
	return retVal;
}

bool AFQSettingBroadInfoWidget::_AddTags(std::vector<std::string> tags)
{
	int width = 0;

	int streamTagCount = tags.size();
	if (streamTagCount < 1)
		return false;

	for (int i = 0; i < streamTagCount; i++)
	{
		QString tagText = QT_UTF8(tags[i].c_str());
		QLabel* tagLabel = new QLabel(tagText, this);
		tagLabel->setToolTip(tagText);
		tagLabel->setProperty("labelType", "Tag");
		tagLabel->setFixedHeight(27);
		tagLabel->adjustSize();
		tagLabel->setAlignment(Qt::AlignCenter);
		width += tagLabel->size().width() + 6;
		ui->scrollAreaWidgetContents_Tags->layout()->addWidget(tagLabel);
	}
	if (width > 370)
		width = 370;
	ui->scrollArea_Tags->setFixedWidth(width);
	return true;
}

QString AFQSettingBroadInfoWidget::_GetWatermarkPosTextByIndex(int idx)
{
	if (idx < 0 || idx >= m_watermarkPosTexts.size())
		return "";

	return m_watermarkPosTexts[idx];
}
