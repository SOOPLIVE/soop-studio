#include "CInstantVODSaverWidget.h"
#include "ui_instant-vod-saver-widget.h"

#include "qt-wrappers.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "UIComponent/CLengthAwareCustomLineEdit.h"

AFQInstantVodSaverWidget::AFQInstantVodSaverWidget(QWidget* parent, QString prevTitle) :
	AFTTopBaseDialog(parent),
	ui(new Ui::AFQInstantVodSaverWidget)
{
	ui->setupUi(this);
	
	AFQBlockManager::ApplyMoveInAllArea(this);

	ui->label_Notice->setVisible(!prevTitle.isEmpty());
	if (prevTitle.isEmpty())
	{
		auto height = this->height() - 36;
		setFixedHeight(height);
	}
	
	ui->widget_Checkbox->setVisible(!prevTitle.isEmpty());
	if (prevTitle.isEmpty())
	{
		int height = this->height() - 22;
		setFixedHeight(height);
	}
	
	_Init(prevTitle);
}

AFQInstantVodSaverWidget::~AFQInstantVodSaverWidget()
{
	delete ui;
}

void AFQInstantVodSaverWidget::_qslotLineEditChanged(bool focus)
{
	if (focus)
	{
		m_previousTitle = ui->lineEdit_VodTitle->text();
	}
	else
	{
		if (ui->lineEdit_VodTitle->text() == "")
			ui->lineEdit_VodTitle->setText(m_previousTitle);
	}
}

void AFQInstantVodSaverWidget::_qslotClickedSaveButton()
{
	QString failMessage = QTStr("SaveVodNow.Failed");
	if (MAINFRAME->CheckSplitVodAvailable())
	{
		std::string userId;
		AUTH_CONTEXT.GetChannelID(PLATFORM_SOOP, userId);

		std::string vodTitle = ui->lineEdit_VodTitle->text().toStdString();
		std::string vodHashTag = AUTH_CONTEXT.MergeHashTag();

		QList<QVariant> queryValues = { };
		std::string responseData = SOOP_API_HANDLER->getAPIfromId(GET_BROADTITLE_ENABLE, queryValues);

		m_vodSaveSuccess = AUTH_CONTEXT.VodRequestVodSave(vodTitle, vodHashTag);
		close();
		return;
	}

	AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, nullptr,
							   "", failMessage, false, true);
}

void AFQInstantVodSaverWidget::_qslotClickedCancelButton()
{
	close();
}

void AFQInstantVodSaverWidget::closeEvent(QCloseEvent* event)
{
	bool dontOpen = ui->checkBox_DontOpenPopup->isChecked() ? false : true;
	config_set_bool(USERCONFIG, "BroadInfo", "NotifyOnTitleChange", dontOpen);
	config_save_safe(USERCONFIG, "tmp", nullptr);
}

void AFQInstantVodSaverWidget::_Init(QString prevTitle)
{
	setWindowTitle(QTStr("SaveVodNow"));
	ui->label_Title->setText(QTStr("SaveVodNow"));

	ui->pushButton_Close->setProperty("buttonType", "closeButton");
	connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQInstantVodSaverWidget::close);

	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	
	if (!prevTitle.isEmpty())
	{
		ui->lineEdit_VodTitle->setText(prevTitle);
		m_previousTitle = prevTitle;
	}
	else
	{
		ui->lineEdit_VodTitle->setText(QString::fromStdString(broadInfo->Title()));
		m_previousTitle = QString::fromStdString(broadInfo->Title());
	}

	if (!broadInfo->SubscribeBroad())
	{
		ui->label_Subscribe->hide();
		int height = this->height() - 20;
		setFixedHeight(height);
	}

	ui->pushButton_Save->setProperty("pushButtonTheme", "type2");
	PolishStyleSheet(ui->pushButton_Save);

	ui->pushButton_Cancel->setProperty("pushButtonTheme", "type4");
	PolishStyleSheet(ui->pushButton_Cancel);

	// Connect signal
	connect(ui->pushButton_Save, &QPushButton::clicked,
		this, &AFQInstantVodSaverWidget::_qslotClickedSaveButton);

	connect(ui->pushButton_Cancel, &QPushButton::clicked,
		this, &AFQInstantVodSaverWidget::_qslotClickedCancelButton);

	connect(ui->lineEdit_VodTitle, &AFQFocusAwareLineEdit::qsignalFocusChanged,
		this, &AFQInstantVodSaverWidget::_qslotLineEditChanged);

	ui->pushButton_QuestionMark->SetExplanationText(QTStr("SaveVodNow.Tooltip"));
	ui->pushButton_QuestionMark->setProperty("buttonType", "questionmarkButton");

	bool dontOpen = config_get_bool(USERCONFIG, "BroadInfo", "NotifyOnTitleChange") ? false : true;
	ui->checkBox_DontOpenPopup->setChecked(dontOpen);
}
