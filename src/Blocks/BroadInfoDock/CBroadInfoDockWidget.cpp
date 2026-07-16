#include "CBroadInfoDockWidget.h"
#include "ui_broad-info-dock.h"

#include <QScrollbar>

#include "qt-wrappers.hpp"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/OBSOutput/COutput.h"
#include "CoreModel/SOOPSource/CSoopMediaSourceManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "CUserCountWidget.h"
#include "CUserUpWidget.h"
#include "PopupWindows/SettingPopup/CSettingStreamAreaWidget.h"
#include "PopupWindows/BroadInfoPopup/CPasswordSettingDialog.h"
#include "PopupWindows/BroadInfoPopup/CAgeRestrictionPolicyDialog.h"
#include "PopupWindows/BroadInfoPopup/CAddTagDialog.h"
#include "PopupWindows/BroadInfoPopup/CCategoryDialog.h"
#include "UIComponent/CMessageAlert.h"
#include "UIComponent/CMessageBox.h"
#if 0
#include "PopupWindows/BroadInfoPopup/CWatermarkPositionSettingDialog.h"
#include "PopupWindows/BroadInfoPopup/CSoopBroadcastNoticeDialog.h"
#include "PopupWindows/BroadInfoPopup/CVodAutoUploadNoticeDialog.h"
#include "PopupWindows/BroadInfoPopup/CCategoryCautionDialog.h"
#endif
#include "Utils/BreaktimeManager.h"

#include "MainFrame/CMainFrame.h"
#include "CoreModel/Source/CSource.h"

#define LAYOUT_CHANGE_HEIGHT 180
//#define LAYOUT_CHANGE_WIDTH 430


AFBroadInfoDockWidget::AFBroadInfoDockWidget(QWidget *parent) 
    : QWidget(parent),
    ui(new Ui::AFBroadInfoDockWidget)
{
    ui->setupUi(this);
    //
    _Init();

}

AFBroadInfoDockWidget::~AFBroadInfoDockWidget()
{
    qslotStopStreamingInfoTimer();
    delete m_finishEditingTimer;
    delete ui;
}

void AFBroadInfoDockWidget::qslotStartStreamingInfoTimer()
{
    if (!m_pViewerTimer)
    {
        m_pViewerTimer = new QTimer();
        connect(m_pViewerTimer, &QTimer::timeout, this, &AFBroadInfoDockWidget::_qslotRefreshViewer);
    }

    //if (!m_pUpTimer)
    //{
    //    m_pUpTimer = new QTimer();
    //    connect(m_pUpTimer, &QTimer::timeout, this, &AFBroadInfoDockWidget::_qslotRefreshUp);
    //}

    m_pViewerTimer->setInterval(30 * 1000);
    //m_pUpTimer->setInterval(60 * 1000);

    m_pViewerTimer->start();
    //m_pUpTimer->start();
}

void AFBroadInfoDockWidget::qslotStopStreamingInfoTimer()
{
    if (m_pViewerTimer)
    {
        m_pViewerTimer->stop();
        delete m_pViewerTimer;
        m_pViewerTimer = nullptr;
    }

    //if (m_pUpTimer)
    //{
    //    m_pUpTimer->stop();
    //    delete m_pUpTimer;
    //    m_pUpTimer = nullptr;
    //}
}

void AFBroadInfoDockWidget::qslotSreamingToggled(bool stream)
{
    if (stream)
        qslotStartStreamingInfoTimer();
    else
        qslotStopStreamingInfoTimer();
}

void AFBroadInfoDockWidget::qslotReceiveSubscribeBroadAvailable(const QByteArray& responseData)
{
    bool currentSubBroadState = ui->pushButton_Subscribe->isChecked();

    std::string jsonString = responseData.toStdString();
    std::string err;
    err.clear();

    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                               "",QTStr("API.Failed"), false, true, "", -1, 200);

    ui->pushButton_Subscribe->setChecked(!currentSubBroadState);
}

void AFBroadInfoDockWidget::_qslotBroadTitleLineSizeChanged(bool multiLine)
{
    //방송 정보 독 최소 사이즈 일 때 카테고리 버튼이랑 태그 쪽 짤리는데 stylesheet 바꾸면 해결됨. 이유는 모름..
    ui->widget_BroadTag->setStyleSheet("#widget_BroadTag{background:transparent;}");

    m_titleMultiLine = multiLine;
	if (multiLine) {
        ui->widget_BroadTitle->setFixedHeight(48);
        ui->stackedWidget_Title->setFixedHeight(48);
        ui->page_Label->setFixedHeight(48);
        ui->page_LineEdit->setFixedHeight(24);
        ui->widget_ViewerInfo->setFixedHeight(48);
		ui->widget_EditBroadTitle->setFixedHeight(24);
        ui->widget_BroadInfo->setFixedHeight(90);

        if(m_layoutDir == LayoutDirection::Horizontal)
            ui->widget_Buttons->setFixedHeight(90);
        else
            ui->widget_Buttons->setFixedHeight(QWIDGETSIZE_MAX);
	}
	else {
		ui->widget_EditBroadTitle->setFixedHeight(24);
		ui->page_LineEdit->setFixedHeight(24);
		ui->page_Label->setFixedHeight(24);
        ui->widget_ViewerInfo->setFixedHeight(24);
		ui->stackedWidget_Title->setFixedHeight(24);
		ui->widget_BroadTitle->setFixedHeight(24);
        ui->widget_BroadInfo->setFixedHeight(70);

        if (m_layoutDir == LayoutDirection::Horizontal)
            ui->widget_Buttons->setFixedHeight(70);
        else
            ui->widget_Buttons->setFixedHeight(QWIDGETSIZE_MAX);
	}
    _RefreshTags();
}

void AFBroadInfoDockWidget::_qslotClickedEditingBroadTitleButton()
{
    if (ui->pushButton_EditBroadTitle->isChecked())
        _StartEditingBroadTitle();
    else 
    {
        m_finishEditingTimer->stop();
        qslotFinishEditingBroadTitle();
    }
}

void AFBroadInfoDockWidget::qslotFinishEditingBroadTitle()
{
    ui->pushButton_EditBroadTitle->setChecked(false);
    ui->stackedWidget_Title->setCurrentIndex(0);

    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(ui->widget_BroadTitle->layout());

    int index = 0;
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem* item = layout->itemAt(i);
        if (item->spacerItem()) {
            index = i;
        }
    }

    layout->setStretch(index, 10);

    // Title Not Changed
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    QString prevTitle = QString::fromStdString(broadInfo->Title());

    QString editedTitle = ui->widget_EditBroadTitle->GetText();
    if (editedTitle == prevTitle || editedTitle == "")
        return;

    // filtering unicode
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

    // Set Broad Title
    ui->label_BroadTitle->updateText(editedTitle);  // use updateText(QString&)

    // Update Broad Info
    broadInfo->SetTitle(QT_TO_UTF8(editedTitle));
    AUTH_CONTEXT.SendSoopBroadInfoSetting();

    MAINFRAME->BroadInfoTimerStart();

    MAIN_BLOCKMANAGER->ShowVodSplit(this, prevTitle);
}

void AFBroadInfoDockWidget::_qslotHideUserCountChanged(bool hideUserCount)
{
    _SetUserCount(hideUserCount);
    m_hideUserCount = hideUserCount;
}

void AFBroadInfoDockWidget::_qslotClickedCategory() 
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

    AFQCategoryDialog* categoryDialog = new AFQCategoryDialog(MAINFRAME, broadInfo->CategoryNumberString(broadInfo->CategoryNumber()));
    connect(categoryDialog, &AFQCategoryDialog::qsignalCategoryChanged,
        this, &AFBroadInfoDockWidget::_qslotCategoryChanged);

    categoryDialog->exec();
}

void AFBroadInfoDockWidget::_qslotClickedAddTagButton()
{
    AFQAddTagDialog* addTagDialog = new AFQAddTagDialog(MAINFRAME);
    connect(addTagDialog, &AFQAddTagDialog::qsignalTagChanged,
            this, &AFBroadInfoDockWidget::_qslotStreamTagChanged);

    addTagDialog->exec();
}

void AFBroadInfoDockWidget::_qslotCategoryChanged(const std::string& categoryNum, const std::string& categoryName)
{
    UNUSED_PARAMETER(categoryName);

    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    const int prevCategoryNum = broadInfo->CategoryNumber();

    auto& breaktime = BREAKTIME_MANAGER;
    if (IsBreaktimeRestrictedCategory(categoryNum)
        && breaktime.IsActive()
        && !breaktime.GetScene().isEmpty())
    {
          AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                     "", QTStr("breaktime.category.restricted"),
                                     false, true, "", 0, 0, "type1");
        return;
    }

    broadInfo->SetCategory(stoi(categoryNum));

    // ani category adult content check
    bool adultContentCheck = SOOP_SRC_MANAGER.GetSoopVodAdultContentCheck(stoi(categoryNum));
    if (adultContentCheck) {
        broadInfo->SetAdultOnly(true);
        ui->pushButton_AdultOnly->setChecked(true);
    }
    else {
        adultContentCheck = SOOP_SRC_MANAGER.GetSoopVodAdultContentCheck(prevCategoryNum);
        if (adultContentCheck) {
            broadInfo->SetAdultOnly(false);
            ui->pushButton_AdultOnly->setChecked(false);
        }
    }

    AFQSceneListItem* clickedItem = SCENE_CONTEXT.GetCurSelectedSceneItem();

    obs_source_t* src = obs_scene_get_source(clickedItem->GetScene());

    MAINFRAME->SetCurrentScene(src);

    SceneItemVector& sceneItemVector = SCENE_CONTEXT.GetSceneItemVector();

    struct FindMinsimChk { bool found = false; int nCnt = 0; } findMinsimChk;
    FindMinsimChk info;
    obs_scene_enum_items(clickedItem->GetScene(),
                         [](obs_scene_t*, obs_sceneitem_t* item, void* param)->bool
    {
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

    AUTH_CONTEXT.SendSoopBroadInfoSetting(false);
    _RefreshCategoryName();
}

bool AFBroadInfoDockWidget::IsBreaktimeRestrictedCategory(const std::string& categoryNum)
{
    int num = 0;
    try {
        num = std::stoi(categoryNum);
    }
    catch (...) {
        return false;
    }

    QString cat = QString("%1").arg(num, 8, 10, QLatin1Char('0'));
    QString prefix = cat.mid(0, 4);

    return (prefix == "0035" 
        || prefix == "0039"
        || prefix == "0094"
        || prefix == "0096");
}
void AFBroadInfoDockWidget::_qslotStreamTagChanged(const std::vector<std::string>& tags)
{
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    std::vector<std::string> temp = broadInfo->HashTags();
    broadInfo->SetHashTags(tags);
    AUTH_CONTEXT.SendSoopBroadInfoSetting();
    _RefreshTags();
}

void AFBroadInfoDockWidget::_qslotClickedAdultOnlyButton(bool checked)
{
    //쉬는시간 예외처리
    if (BREAKTIME_MANAGER.IsActive())
    {
        ui->pushButton_AdultOnly->setChecked(!checked);
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                   "", QTStr("breaktime.broadset.restricted"),
                                   false, true, "", 0, 0, "type1");           
       return;
    }

    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    if (!checked)
    {
        bool adultContentCheck = SOOP_SRC_MANAGER.GetSoopVodAdultContentCheck(broadInfo->CategoryNumber());
        if (adultContentCheck) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                       "", QTStr("Caution.SOOPMediaSource.DisableMessage7"), false, true);
            ui->pushButton_AdultOnly->setChecked(true);
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
                    QString message = QTStr("Caution.SOOPMediaSource.DisableMessage6").arg(obs_source_get_display_name(id));
                    AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                               "", message, false, true);
                    ui->pushButton_AdultOnly->setChecked(true);
                    return;
                }
            }
        }
    }

    if (checked) {
        const char* date = config_get_string(USERCONFIG, "BroadInfo", AGE_RESTRICTION_POLICY_DATE_CHECK);
        bool open = IsDateBeforeToday(date);
        if (open)
        {
            AFQAgeRestrictionPolicyDialog* adultAlertDialog = new AFQAgeRestrictionPolicyDialog(MAINFRAME);
            if (QDialog::Accepted != adultAlertDialog->exec()) {
                ui->pushButton_AdultOnly->setChecked(false);
                return;
            }
        }
        else
        {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", QTStr("Caution.AgeRestricted.On"), false, true);
        }
    }

    bool temp = broadInfo->AdultOnly();
    broadInfo->SetAdultOnly(checked);
    AUTH_CONTEXT.SendSoopBroadInfoSetting(false);
}

void AFBroadInfoDockWidget::_qslotClickedUsePasswordButton()
{
    //쉬는시간 예외처리
    if (BREAKTIME_MANAGER.IsActive())
    {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                   "", QTStr("breaktime.broadset.restricted"),
                                   false, true, "", 0, 0, "type1");        
        return;
    }

    if (ui->pushButton_Subscribe->isChecked())
    {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                   "", QTStr("PasswordOn.Reject.Subscribe.Broad"),
                                   false, true, "", -1, 166);

        ui->pushButton_UsePassword->blockSignals(true);
        ui->pushButton_UsePassword->setChecked(false);
        ui->pushButton_UsePassword->blockSignals(false);
        return;
    }

    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();

    AFQPasswordSettingDialog* passwordSettingDialog = new AFQPasswordSettingDialog(
                                                            MAINFRAME, 
                                                            broadInfo->UsePassword(),
                                                            QT_UTF8(broadInfo->Password().c_str()));
   

    connect(passwordSettingDialog, &AFQPasswordSettingDialog::qsignalClickedUsePassword,
            this, &AFBroadInfoDockWidget::_qslotUsePasswordChanged);
    passwordSettingDialog->exec();
}

void AFBroadInfoDockWidget::_qslotClickedRejectVisitButton(bool checked)
{
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    bool temp = broadInfo->BroadTuneOut();
    broadInfo->SetBroadTuneOut(checked);
    AUTH_CONTEXT.SendSoopBroadInfoSetting();
}

void AFBroadInfoDockWidget::_qslotUsePasswordChanged(bool usePassword, const QString& password)
{
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    bool temp = broadInfo->UsePassword();
    std::string tempPassword = broadInfo->Password();

    broadInfo->SetUsePassword(usePassword);
    broadInfo->SetPassword(QT_TO_UTF8(password));
    AUTH_CONTEXT.SendSoopBroadInfoSetting();
    ui->pushButton_UsePassword->setChecked(usePassword);

    AFSourceUtil::ChangeAIManagerUrl();
}

//subscribe
void AFBroadInfoDockWidget::_qslotClickedSubscribeBroad(bool checked)
{
    bool sendSubscribeBroad = true;

    QMap<std::string, std::string> liveChannels = AUTH_CONTEXT.GetLiveChannels();

    if (AFOutputUtil::IsStreamActive())
    {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                   "", QTStr("StreamOn.Reject.Subscribe.Broad"),
                                   false, true, "",  - 1, 166);
        ui->pushButton_Subscribe->setChecked(!checked);
        return;
    }
    else if (liveChannels.contains(PLATFORM_SOOP) && liveChannels.count() > 1)
    {
        if (checked)
        {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                       "", QTStr("Simulcast.Reject.Subscribe.Start.Broad"),
                                       false, true, "", -1, 166);
            sendSubscribeBroad = false;

        }
    }
    else if (ui->pushButton_UsePassword->isChecked())
    {
        if (checked)
        {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                       "", QTStr("PasswordOn.Reject.Subscribe.Broad"),
                                       false, true, "", -1, 166);
            sendSubscribeBroad = false;
        }
    }

    if (sendSubscribeBroad)
    {
        int subTier = checked ? 2 : 0;
        AUTH_CONTEXT.GetSoopBroadInfo()->SetSubscribeBroad(subTier);
        AUTH_CONTEXT.SendSoopBroadInfoSetting(true);
    }
    else
    {
        QPushButton* sub = reinterpret_cast<QPushButton*>(sender());
        sub->blockSignals(true);
        sub->setChecked(false);
        sub->blockSignals(false);

        AUTH_CONTEXT.GetSoopBroadInfo()->SetSubscribeBroad(false);
    }

}

void AFBroadInfoDockWidget::_qslotClickedUserCountButton(bool checked)
{
    if (checked) {
		ui->pushButton_UserCount->setProperty("pressed", checked);
		PolishStyleSheet(ui->pushButton_UserCount);
    }

    AFUserCountWidget* userCountDetailsWidget = new AFUserCountWidget();
    connect(userCountDetailsWidget, &AFUserCountWidget::qsignalHideUserCountChanged,
            this, &AFBroadInfoDockWidget::_qslotHideUserCountChanged);

    // Set Position
    int parentWidth = ui->pushButton_UserCount->width();
    int gap = 5;
    QSize currentWidgetSize = userCountDetailsWidget->size();
    QPoint globalPos = ui->pushButton_UserCount->mapToGlobal(QPoint(0, 0));
    int moveY = currentWidgetSize.height() + gap;

    userCountDetailsWidget->move(QPoint(globalPos.x(), globalPos.y() - moveY));
    userCountDetailsWidget->show();
}

void AFBroadInfoDockWidget::_qslotClickedUPButton(bool checked)
{
    _qslotRefreshUp();

    if (checked) {
        ui->pushButton_UserUp->setProperty("pressed", checked);
        PolishStyleSheet(ui->pushButton_UserUp);
    }

    AFUserUpWidget* userUpDetailsWidget = new AFUserUpWidget();

    // Set Position
    int parentWidth = ui->pushButton_UserUp->width();
    int gap = 5;
    QSize currentWidgetSize = userUpDetailsWidget->size();
    QPoint globalPos = ui->pushButton_UserUp->mapToGlobal(QPoint(0, 0));
    int moveY = currentWidgetSize.height() + gap;

    userUpDetailsWidget->move(QPoint(globalPos.x(), globalPos.y() - moveY));
    userUpDetailsWidget->show();
}

void AFBroadInfoDockWidget::_qslotRefreshUI(int result, QString message)
{
    if (result == 1)
    {
        LoadBroadInfoUI();
    }
    else
    {
        if (result == -999)
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                                       "", QTStr("BroadInfo.Api.Failed"), false, true);
        else
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                                       "", message, false, true);
    }
}

void AFBroadInfoDockWidget::_qslotSendBroadInfoReceived(int result, QString message)
{
    QString message_ = message;
    if (result == -999)
    {
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
                                   "", QTStr("BroadInfo.Api.Failed"), false, true);
    }
    else
    {
        if (result != 1)
        {
            if (result == -202 || result == -203) //password failed ------ NEED CHECK
            {

            }
            else if (result == -1)
            {
                if (message.contains("stream dashboard title includes forbidden word."))
                {
                    message_ = QTStr("GL.Title.Forbidden");
                }
                else if (message.contains("stream dashboard not created."))
                {
                    message_ = QTStr("GL.Dashboard.Missing");
                }
                else if (message.contains("stream dashboard category not found."))
                {
                    message_ = QTStr("GL.No.Category");
                }
                else if (message.contains("stream dashboard kr category ban."))
                {
                    message_ = QTStr("GL.KR.Category.Ban");
                }
            }

            AUTH_CONTEXT.RequestBroadInfoAPI();
            if (m_lastErrorMsg != message_) {
                m_lastErrorMsg = message_;
                QString mbText = "";
                int msg_result = 0;
                if (_CheckGLFailMessage(message_, mbText))
                {
                    msg_result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                                            "", mbText, false, true, "부적절한 키워드로 인해 제한되었습니다.");
                }
                else
                {
                    msg_result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                                            "", message_, false, true);
                }
                if (msg_result == QDialog::Accepted) {
                    m_lastErrorMsg = "";
                }
            }            
        }
    }
}


void AFBroadInfoDockWidget::_qslotCheckSubscribeBroadDockonStart()
{
    disconnect(MAINFRAME, &AFMainFrame::qsignalMainShowEventTriggered, this, &AFBroadInfoDockWidget::_qslotCheckSubscribeBroadDockonStart);

    QTimer::singleShot(100, [this] {
        AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
        if (broadInfo->SubscribeBroad() > 0)
        {
            if (ui->pushButton_UsePassword->isChecked())
            {
                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME,
                                           "", QTStr("PasswordOn.Reject.Subscribe.Broad"),
                                           false, true, "", -1, 166);

                ui->pushButton_UsePassword->setChecked(false);
            }
        }
        });
}

void AFBroadInfoDockWidget::_qslotBroadInfoAPIErrorReceived(int code, QString message)
{
    AUTH_CONTEXT.RequestBroadInfoAPI();
    QString failMessage = message;
    int msgWidth = 0;
    int msgHeight = 0;

    bool showMessage = true;

    if (code == -1304)
    {
        QString inhibit = "";
        QRegularExpression re("'([^']+)'");
        QRegularExpressionMatch match = re.match(message);

        if (match.hasMatch())
            inhibit = match.captured(1);

        msgHeight = 220;
        failMessage = QTStr("BroadInfo.Illegal.Title").arg(inhibit);
    }
    else if (code == -1311)
    {
        failMessage = QTStr("BroadInfo.Illegal.EndingMessge");
    }
    else if (code == -1313)
    {
        failMessage = QTStr("BroadInfo.Illegal.Socket");
    }
    else if (code == -1316)
    {
        failMessage = QTStr("BroadInfo.Illegal.Backslash");
    }
    else if (code == -1321)
    {
        failMessage = QTStr("BroadInfo.Illegal.BlankTitle");
    }
    else if (code == -1331)
    {
        failMessage = QTStr("BroadInfo.Illegal.Underage");
    }
    else if (code == -1334)
    {
        QString inhibit = "";
        QRegularExpression re("'([^']+)'");
        QRegularExpressionMatch match = re.match(message);

        if (match.hasMatch())
            inhibit = match.captured(1);

        failMessage = QTStr("BroadInfo.Illegal.Sexual.Title").arg(inhibit);
        std::string locale = LOCALE_CONTEXT.GetCurrentLocaleStr();
        if (locale == "ko-KR")
            msgHeight = 250;
        else
            msgHeight = 280;
    }
    else if (code == -1350 || code == -1305 || code == -1318 || code == -1327 || code == -1328 || code == -1329 || code == -1349)
    {
        AUTH_CONTEXT.GetSoopBroadInfo()->SetPassword("");
        AUTH_CONTEXT.GetSoopBroadInfo()->SetUsePassword(false);
        showMessage = false;
    }

    if(showMessage)
        AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", failMessage, false, true, "", msgWidth, msgHeight);
}

void AFBroadInfoDockWidget::_qslotRefreshUp()
{
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    broadInfo->ReceiveUp();
}

void AFBroadInfoDockWidget::_qslotRefreshUpReceived(int result, QString message)
{
    if (result == 1)
    {
        QLocale locale = QLocale(QLocale::Korean);
        AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
        QString upCount = QString(" %1").arg(locale.toString(broadInfo->UpTotal()));
        ui->pushButton_UserUp->setText(upCount);
    }
}

void AFBroadInfoDockWidget::_qslotRefreshViewer()
{
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    broadInfo->ReceiveViewer();
}

void AFBroadInfoDockWidget::_qslotRefreshViewerRecveived(int result, QString message)
{
    if (result == 1)
    {
        if (m_hideUserCount)
        {
            ui->pushButton_UserCount->setText(" -");
        }
        else
        {
            QLocale locale = QLocale(QLocale::Korean);
            AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
            int totalViewer = broadInfo->CurrentViewer() + broadInfo->RelayViewer();

            QString viewerCount = QString(" %1").arg(locale.toString(totalViewer));

            ui->pushButton_UserCount->setText(viewerCount);
        }
    }
    else
    {
        blog(LOG_ERROR, "soop user stat api fail.. [%d] [%s]", result, message.toStdString().c_str());
    }
}

void AFBroadInfoDockWidget::_qslotResetViewer()
{
    if(m_hideUserCount)
        ui->pushButton_UserCount->setText(" -");
    else
        ui->pushButton_UserCount->setText(" 0");



    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    broadInfo->ResetAllViewerCount();
}

void AFBroadInfoDockWidget::LoadBroadInfoUI()
{
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    if (broadInfo)
    {
        QString title = QString::fromStdString(broadInfo->Title());

        ui->label_BroadTitle->updateText(title);
        ui->widget_EditBroadTitle->SetText(title);

        _RefreshCategoryName();

        bool addTag = _RefreshTags();
        ui->pushButton_AdultOnly->setChecked(broadInfo->AdultOnly());
        ui->pushButton_UsePassword->setChecked(broadInfo->UsePassword());
        ui->pushButton_RejectVisit->setChecked(broadInfo->BroadTuneOut());
        ui->pushButton_Subscribe->setChecked(broadInfo->SubscribeBroad());
    }
}

void AFBroadInfoDockWidget::resizeEvent(QResizeEvent* event)
{
    _CheckLayout();

    int maxWidth = event->size().width() / 2;
    ui->frameCategory->setMaximumWidth(maxWidth);
}

void AFBroadInfoDockWidget::showEvent(QShowEvent* event)
{
    _CheckLayout();
}

void AFBroadInfoDockWidget::_Init() 
{
    ui->label_BroadTitle->setText("방송 제목을 입력하세요.");
    ui->label_BroadCategory->setText("카테고리 선택 >");

    ui->stackedWidget_Title->setCurrentIndex(0);

    ui->label_BroadTitle->SetMultiLine(true);

    ui->widget_EditBroadTitle->SetMaxLength(75);
    ui->widget_EditBroadTitle->SetMaxLengthVisible(false);
    ui->widget_EditBroadTitle->SetUseBroadInfoDock(true);

    ui->widget_EditBroadTitle->hide();
    ui->pushButton_EditBroadTitle->setCheckable(true);

    ui->label_BroadCategory->setProperty("labelType", "Category");
    ui->label_BroadCategory->setAlignment(Qt::AlignCenter);
    ui->label_BroadCategory->setMinimumWidth(20);
    ui->label_BroadCategory->setProperty("showHandCursor", true);
    ui->label_BroadCategory->setProperty("centerPosition", true);

    ui->pushButton_AdultOnly->setCheckable(true);
    ui->pushButton_UsePassword->setCheckable(true);
    ui->pushButton_RejectVisit->setCheckable(true);
    ui->pushButton_Subscribe->setCheckable(true);

    m_finishEditingTimer = new QTimer(this);
    m_finishEditingTimer->setInterval(200);
    m_finishEditingTimer->setSingleShot(true);

    m_hideUserCount = config_get_bool(USERCONFIG, "BroadInfo", "HideUserCount");
    _SetUserCount(m_hideUserCount);
    _SetUpCount();
    _InitToolTip();
    LoadBroadInfoUI();
    _ConnectEvents();

    _qslotRefreshUp();

    _SwitchLayout(LayoutDirection::Vertical);
}

void AFBroadInfoDockWidget::_ConnectEvents()
{
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    connect(broadInfo, &AFQBroadInfo::qsignalBroadInfoReceived, this, &AFBroadInfoDockWidget::_qslotRefreshUI);
    connect(broadInfo, &AFQBroadInfo::qsignalSendBroadInfoResult, this, &AFBroadInfoDockWidget::_qslotSendBroadInfoReceived);

    connect(broadInfo, &AFQBroadInfo::qsignalBroadInfoAPIError, this, &AFBroadInfoDockWidget::_qslotBroadInfoAPIErrorReceived);

    connect(broadInfo, &AFQBroadInfo::qsignalUpResult, this, &AFBroadInfoDockWidget::_qslotRefreshUpReceived);
    connect(broadInfo, &AFQBroadInfo::qsignalViewerResult, this, &AFBroadInfoDockWidget::_qslotRefreshViewerRecveived);

    connect(MAINFRAME, &AFMainFrame::qsignalBroadToggled, this, &AFBroadInfoDockWidget::qslotSreamingToggled);
    connect(MAINFRAME, &AFMainFrame::qsignalMainShowEventTriggered, this, &AFBroadInfoDockWidget::_qslotCheckSubscribeBroadDockonStart);

    // Broad Title
    connect(ui->label_BroadTitle, &AFQElidedSlideLabel::qsignalTextMultiLineChanged,
            this, &AFBroadInfoDockWidget::_qslotBroadTitleLineSizeChanged);
    connect(ui->pushButton_EditBroadTitle, &QPushButton::clicked,
            this, &AFBroadInfoDockWidget::_qslotClickedEditingBroadTitleButton);
    connect(ui->widget_EditBroadTitle, &AFQPrefixLengthAwareLineEdit::qsignalEditFinished,
            m_finishEditingTimer, qOverload<>(&QTimer::start));
    connect(m_finishEditingTimer, &QTimer::timeout,
            this, &AFBroadInfoDockWidget::qslotFinishEditingBroadTitle);

    // Stream Tag
    connect(ui->label_BroadCategory, &AFQElidedSlideLabel::qsignalHoverEnter,
            ui->label_BroadCategory, &AFQElidedSlideLabel::qslotHoverLabel);
    connect(ui->label_BroadCategory, &AFQElidedSlideLabel::qsignalHoverLeave,
            ui->label_BroadCategory, &AFQElidedSlideLabel::qslotLeaveButton);
    connect(ui->frameCategory, &AFQClickableFrame::qsignalFrameClicked, this, &AFBroadInfoDockWidget::_qslotClickedCategory);

    connect(ui->pushButton_AddTag, &QPushButton::clicked, this, &AFBroadInfoDockWidget::_qslotClickedAddTagButton);


    // User Count
    connect(ui->pushButton_UserCount, &QPushButton::clicked, this, &AFBroadInfoDockWidget::_qslotClickedUserCountButton);

    // UP
    connect(ui->pushButton_UserUp, &QPushButton::clicked, this, &AFBroadInfoDockWidget::_qslotClickedUPButton);

    // Adult Only
    connect(ui->pushButton_AdultOnly, &QPushButton::clicked, this, &AFBroadInfoDockWidget::_qslotClickedAdultOnlyButton);
    // Use Password
    connect(ui->widget_UsePassword, &AFQHoverWidget::qsignalMouseClick, this, &AFBroadInfoDockWidget::_qslotClickedUsePasswordButton);
    // Reject Visit
    connect(ui->pushButton_RejectVisit, &QPushButton::clicked, this, &AFBroadInfoDockWidget::_qslotClickedRejectVisitButton);

    //subscribe
    connect(ui->pushButton_Subscribe, &QPushButton::clicked, this, &AFBroadInfoDockWidget::_qslotClickedSubscribeBroad);

    connect(MAINFRAME, &AFMainFrame::qsignalBroadToggled, this, &AFBroadInfoDockWidget::_qslotResetViewer);
}

void AFBroadInfoDockWidget::_InitToolTip()
{
    ui->pushButton_EditBroadTitle->setToolTip(QTStr("BroadInfo.EditTitle"));
    ui->pushButton_UserCount->setToolTip(QTStr("BroadInfo.UserCountDetails"));
    ui->pushButton_AdultOnly->setToolTip(QTStr("BroadInfo.SetAgeRestrict"));
    ui->widget_UsePassword->setToolTip(QTStr("BroadInfo.SetPassword"));
    ui->pushButton_RejectVisit->setToolTip(QTStr("BroadInfo.SetRejectVisit"));
    
    //subscribe
    ui->pushButton_Subscribe->setToolTip(QTStr("Subscribe.Broad"));
}

void AFBroadInfoDockWidget::_CheckLayout()
{
    if (this->height() >= LAYOUT_CHANGE_HEIGHT)
        _SwitchLayout(LayoutDirection::Vertical);
    else
        _SwitchLayout(LayoutDirection::Horizontal);
}

void AFBroadInfoDockWidget::_SwitchLayout(LayoutDirection dir)
{
    if (m_layoutDir == dir)
        return;

    m_layoutDir = dir;

	if (dir == LayoutDirection::Vertical)
	{
		QVBoxLayout* contentslayout = new QVBoxLayout();
        contentslayout->setContentsMargins(14, 10, 14, 16);
        contentslayout->setSpacing(16);
		contentslayout->addWidget(ui->widget_BroadInfo);

        ui->horizontalSpacer_3->changeSize(0, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->horizontalSpacer_4->changeSize(10, 10, QSizePolicy::Expanding, QSizePolicy::Fixed);
        ui->widget_Buttons->setFixedHeight(QWIDGETSIZE_MAX);
		contentslayout->addWidget(ui->widget_Spacer);

        ui->horizontalSpacer_ViewerLeft->changeSize(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->horizontalSpacer_ViewerRight->changeSize(10, 20, QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->widget_ViewerInfo->layout()->invalidate();
        ui->widget_ViewerInfo->layout()->update();

        ui->horizontalSpacer_ButtonRight->changeSize(10, 20, QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->widget_BroadOptionLayout->layout()->invalidate();
        ui->widget_BroadOptionLayout->layout()->update();
        

		delete ui->dockContainer->layout();
		ui->dockContainer->setLayout(contentslayout);

        ui->horizontalSpacer_ButtonLeft->changeSize(0, 20, QSizePolicy::Fixed, QSizePolicy::Expanding);
        ui->horizontalSpacer_ButtonRight->changeSize(10, 20, QSizePolicy::Expanding, QSizePolicy::Expanding);        

        ui->widget_ButtonVerticalSpacer->show();
	}
	else // Horizontal
	{
		QHBoxLayout* contentslayout = new QHBoxLayout();
        contentslayout->setContentsMargins(14, 10, 14, 14);
		contentslayout->addWidget(ui->widget_BroadInfo);

        int buttonWidgetSize = 70;
        if (m_titleMultiLine)
            buttonWidgetSize = 90;
        ui->widget_Buttons->setFixedHeight(buttonWidgetSize);

        contentslayout->addWidget(ui->widget_Spacer);

        contentslayout->setStretch(0, 10);
        contentslayout->setStretch(1, 1);

        ui->horizontalSpacer_ViewerLeft->changeSize(10, 20, QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->horizontalSpacer_ViewerRight->changeSize(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->widget_ViewerInfo->layout()->invalidate();
        ui->widget_ViewerInfo->layout()->update();

        ui->horizontalSpacer_ButtonRight->changeSize(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        ui->widget_BroadOptionLayout->layout()->invalidate();
        ui->widget_BroadOptionLayout->layout()->update();

        delete ui->dockContainer->layout();
        ui->dockContainer->setLayout(contentslayout);
        ui->horizontalSpacer_ButtonLeft->changeSize(10, 20, QSizePolicy::Expanding, QSizePolicy::Expanding);
        ui->horizontalSpacer_ButtonRight->changeSize(0, 20, QSizePolicy::Fixed, QSizePolicy::Expanding);

        ui->widget_ButtonVerticalSpacer->hide();
	}
}

void AFBroadInfoDockWidget::_VerticalBroadOptionButton()
{
    QGridLayout* layout = new QGridLayout(ui->widget_BroadOption);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(ui->pushButton_AdultOnly, 0, 0);
    layout->addWidget(ui->widget_UsePassword, 0, 2);
    layout->addWidget(ui->pushButton_RejectVisit, 1, 0);
    layout->addWidget(ui->pushButton_Subscribe, 1, 2);

    delete ui->widget_BroadOption->layout();
    ui->widget_BroadOption->setLayout(layout);

    ui->widget_BroadOption->setFixedSize(48 + 12, 48 + 12);
}

void AFBroadInfoDockWidget::_HorizontalBroadOptionButton()
{
    QList<QPushButton*> buttonList = ui->widget_BroadOption->layout()->findChildren<QPushButton*>();
    int buttonCount = buttonList.count();

    QHBoxLayout* hLayout = new QHBoxLayout();    
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(6);

    int widthVal = 0;
    int heightVal = 24;

    hLayout->addWidget(ui->pushButton_AdultOnly);
    widthVal += ui->pushButton_AdultOnly->width();

    hLayout->addWidget(ui->pushButton_Subscribe);
    widthVal += ui->pushButton_Subscribe->width();
    //subscribe

    delete ui->widget_BroadOption->layout();
    ui->widget_BroadOption->setLayout(hLayout);

    ui->widget_BroadOption->setFixedSize(widthVal, heightVal);
}

void AFBroadInfoDockWidget::_StartEditingBroadTitle()
{
    ui->stackedWidget_Title->setCurrentIndex(1);

    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(ui->widget_BroadTitle->layout());

    int index = 0;
    for (int i = 0; i < layout->count(); ++i) {
        QLayoutItem* item = layout->itemAt(i);
        if (item->spacerItem()) {
            index = i;
        }
    }

    layout->setStretch(index, 0);

    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    QString title = QString::fromStdString(broadInfo->Title());

    ui->widget_EditBroadTitle->SetText(title);
    ui->widget_EditBroadTitle->SelectAll();
    ui->widget_EditBroadTitle->SetFocus();
    ui->widget_EditBroadTitle->show();
    m_existingTitle = title;

    MAINFRAME->BroadInfoTimerStop();
}

void AFBroadInfoDockWidget::_RefreshCategoryName()
{
    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    std::string fullCategoryName = AUTH_CONTEXT.FullCategoryName(broadInfo->CategoryNumber());

    QString displayText = QString::fromStdString(fullCategoryName);
    if (displayText.isEmpty())
        displayText = "";

    ui->label_BroadCategory->setText(QString::fromStdString(broadInfo->Category()));
    ui->label_BroadCategory->setToolTip(displayText);
}

bool AFBroadInfoDockWidget::_RefreshTags()
{
    RemoveAllChildInLayout(ui->scrollAreaWidgetContents_Tags->layout());

    ui->scrollAreaWidgetContents_Tags->layout()->setContentsMargins(0, 0, 0, 0);

    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
    bool addStream = _AddTags(broadInfo->HashTags());

    return addStream;
}

bool AFBroadInfoDockWidget::_AddTags(std::vector<std::string> tags)
{
    int width = 0;
    // Add Category Tags
    int tagCount = tags.size();
    if (tagCount < 1) {
        ui->scrollAreaWidgetContents_Tags->setFixedWidth(width);
        ui->widget_Tags->setMaximumWidth(width);
        return false;
    }

    for (int i = 0; i < tagCount; i++)
    {
        QString tagText = QT_UTF8(tags[i].c_str());
        QLabel* tagLabel = new QLabel(tagText, this);
        tagLabel->setProperty("labelType", "Tag");
        tagLabel->setToolTip(tagText);
        tagLabel->setFixedHeight(27);
        tagLabel->setAlignment(Qt::AlignCenter);
        tagLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        tagLabel->adjustSize();

        width += tagLabel->size().width() + 6;

        ui->scrollAreaWidgetContents_Tags->layout()->addWidget(tagLabel);
        ui->widget_Tags->setMaximumWidth(width);
    }

    ui->scrollAreaWidgetContents_Tags->setFixedWidth(width);

    return true;
}

bool AFBroadInfoDockWidget::_CheckTitleBanWord(const QString& title)
{
    AFChannelData* pSoopChannel = nullptr;
    AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, pSoopChannel);
    QString userID = QString::fromStdString(pSoopChannel->pAuthData->channelID);
    QString encodedTitle = EncodeRFC3986(title);

    return false;
}

void AFBroadInfoDockWidget::_SetUserCount(bool bHideUserCount) 
{
    if (bHideUserCount)
        ui->pushButton_UserCount->setText(" -");
    else
    {
        QLocale locale = QLocale(QLocale::Korean);
        AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
        int totalViewer = broadInfo->CurrentViewer() + broadInfo->RelayViewer();
        QString viewerCount = QString(" %1").arg(locale.toString(totalViewer));

        ui->pushButton_UserCount->setText(viewerCount);
    }
}

void AFBroadInfoDockWidget::_SetUpCount()
{
    ui->pushButton_UserUp->setText("0");
}

bool AFBroadInfoDockWidget::_CheckGLFailMessage(const QString& input, QString& rawText)
{
    QString blockMessage = "부적절한 키워드로 인해 제한되었습니다. 다른 제목을 입력해 주시기 바랍니다.";
    QString retry = " 다른 제목을 입력해 주시기 바랍니다.";
    int index = input.indexOf(blockMessage);
    if (index != -1) {
        QString msg = input.left(index).trimmed();
        if (msg.length() > 15) 
            rawText = msg.left(15) + "..." + "\n" + retry;
        else 
            rawText = msg + "\n" + retry;
        return true;
    }

    return false;
}