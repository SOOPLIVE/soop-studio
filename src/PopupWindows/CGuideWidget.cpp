#include "CGuideWidget.h"
#include "ui_program-guide-item.h"
#include "ui_program-guide-widget.h"
#include "platform/platform.hpp"
#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"

#define STEP_TEXT "STEP "

AFQProgramGuideItemWidget::AFQProgramGuideItemWidget(QWidget* parent,
	int stepNum) :
	AFQHoverWidget(parent),
	ui(new Ui::AFQProgramGuideItemWidget),
	m_nStepNum(stepNum)
{
	ui->setupUi(this);

	ui->label_StepText->setText(STEP_TEXT + QString::number(stepNum));
}

AFQProgramGuideItemWidget::~AFQProgramGuideItemWidget()
{
	delete ui;
}

void AFQProgramGuideItemWidget::SetGuideItemState(const GuideItemState state)
{
	if (m_eState == state)
		return;

	m_eState = state;
	std::string absPath;

	if (m_eState == GuideItemState::Hidden)
	{
		// BG
		QString style = "QWidget {border-radius: 20px; background-color: %1;}";
		ui->widget_ContentsArea->setStyleSheet(style.arg(m_strInactiveBGColor));
		
		// Icon
		GetDataFilePath(m_strHiddenIconPath, absPath);
		QString qstrImgPath(absPath.data());
		style = "QLabel {image: url(%1) 0 0 0 0 stretch stretch;}";
		ui->label_StepIcon->setStyleSheet(style.arg(qstrImgPath));

		// Step Text
		style = "QLabel{ color: rgba(255, 255, 255, 20%);\
							font-size: 14px;\
							font-weight: 700; }";
		ui->label_StepText->setStyleSheet(style);

		ui->label_MissionText->setVisible(false);
		ui->widget_ArrowIconArea->setVisible(false);

		this->setFixedSize(QSize(380, 80));
	}
	else if (m_eState == GuideItemState::Visible)
	{
		// BG
		QString style = "QWidget {border-radius: 20px; background-color: %1;}";
		ui->widget_ContentsArea->setStyleSheet(style.arg(m_strActiveBGColor));
		
		// Icon
		GetDataFilePath(m_strVisibleIconPath, absPath);
		QString qstrImgPath(absPath.data());
		style = "QLabel {image: url(%1) 0 0 0 0 stretch stretch;}";
		ui->label_StepIcon->setStyleSheet(style.arg(qstrImgPath));

		// Step Text
		style = "QLabel{ color: rgba(36, 39, 45, 70%);\
							font-size: 14px;\
							font-weight: 700; }";
		ui->label_StepText->setStyleSheet(style);

		// Mission Text
		style = "QLabel{ color: #24272D;\
						font-size: 16px;\
						font-weight: 700;\
						line-height: normal; }";
		ui->label_MissionText->setStyleSheet(style);
		ui->label_MissionText->setText(m_strMissionText);
		ui->label_MissionText->setVisible(true);
		ui->widget_ArrowIconArea->setVisible(true);

		this->setFixedSize(QSize(380, 120));
	}
	else // Completed
	{
		// BG
		QString style = "QWidget {border-radius: 20px; background-color: %1;}";
		ui->widget_ContentsArea->setStyleSheet(style.arg(m_strInactiveBGColor));

		// Icon
		GetDataFilePath(m_strCompletedIconPath, absPath);
		QString qstrImgPath(absPath.data());
		style = "QLabel {image: url(%1) 0 0 0 0 stretch stretch;}";
		ui->label_StepIcon->setStyleSheet(style.arg(qstrImgPath));

		// Step Text
		style = "QLabel{ color: rgba(255, 255, 255, 40%);\
							font-size: 14px;\
							font-weight: 700; }";
		ui->label_StepText->setStyleSheet(style);

		// Mission Text
		style = "QLabel{ color: #FFF;\
						font-size: 16px;\
						font-weight: 500;}";
		ui->label_MissionText->setStyleSheet(style);
		ui->label_MissionText->setText(m_strCompletedText);
		ui->label_MissionText->setVisible(true);
		ui->widget_ArrowIconArea->setVisible(false);

		this->setFixedSize(QSize(380, 80));
	}
}

void AFQProgramGuideItemWidget::SetContentText(const QString missionTxt, const QString completedTxt)
{
	m_strMissionText = missionTxt;
	m_strCompletedText = completedTxt;
}

void AFQProgramGuideItemWidget::SetIconPath(const char* hiddenIcon, const char* visibleIcon, const char* completedIcon)
{
	m_strHiddenIconPath = hiddenIcon;
	m_strVisibleIconPath = visibleIcon;
	m_strCompletedIconPath = completedIcon;
}

void AFQProgramGuideItemWidget::SetBGColor(const QString activeColor, const QString inactiveColor)
{
	m_strActiveBGColor = activeColor;
	m_strInactiveBGColor = inactiveColor;
}

AFQProgramGuideWidget::AFQProgramGuideWidget(QWidget* parent, Qt::WindowFlags flag) :
	AFCQMainBaseWidget(parent, flag),
	ui(new Ui::AFQProgramGuideWidget)
{
	ui->setupUi(this);
	setMouseTracking(true);
	installEventFilter(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setAttribute(Qt::WA_TranslucentBackground);
	setWindowTitle(QTStr("Basic.MainMenu.Help.Guide"));

	_InitGuide();
	StartGuide();

	connect(ui->pushButton_OkBtn, &QPushButton::clicked, this, &AFQProgramGuideItemWidget::close);
    this->SetWidthFixed(true);
    this->SetHeightFixed(true);
}

AFQProgramGuideWidget::~AFQProgramGuideWidget()
{
	_ClearGuideItems();
	delete ui;
}


void AFQProgramGuideWidget::qslotMouseClicked()
{
	AFQProgramGuideItemWidget* guideItem = reinterpret_cast<AFQProgramGuideItemWidget*>(sender());
	int guideNum = guideItem->GetGuideItem();

	emit qsignalMissionTrigger(guideNum);
}

void AFQProgramGuideWidget::_SetGuideAllCleared()
{
	ui->label_Title->setText("");
	ui->widget_GuideItemContainer->setVisible(false);
	ui->widget_AllClearTextArea->setVisible(true);

	// BG
	std::string absPath;
	GetDataFilePath("assets/guide/guide-all-clear-bg.png", absPath);
	QString qstrImgPath(absPath.data());
	//QString style = "QWidget #widget_GuideContentArea { image: url(%1); background: none; }";
	QString style = "QWidget #widget_GuideContentArea { background: none; image: url(%1); }";
	ui->widget_GuideContentArea->setStyleSheet(style.arg(qstrImgPath));

	style = "QPushButton {background: #D1FF01;\
						color: #25282E;\
						font-size: 15px;\
						font-weight: 700;}";
	ui->pushButton_OkBtn->setStyleSheet(style);
}

void AFQProgramGuideWidget::NextMission(int index)
{
	if (m_nCurrentStep != index)
		return;

	auto& authManager = AFAuthManager::GetSingletonInstance();
	

	if (m_nCurrentStep + 1 >= m_guideItems.size())
	{
		_SetGuideAllCleared();
		return;
	}

	m_guideItems[m_nCurrentStep]->SetGuideItemState(AFQProgramGuideItemWidget::GuideItemState::Completed);
	disconnect(m_guideItems[m_nCurrentStep], &AFQHoverWidget::qsignalMouseClick, 
		this, &AFQProgramGuideWidget::qslotMouseClicked);
	m_nCurrentStep++;

	m_guideItems[m_nCurrentStep]->SetGuideItemState(AFQProgramGuideItemWidget::GuideItemState::Visible);
	connect(m_guideItems[m_nCurrentStep], &AFQHoverWidget::qsignalMouseClick, 
		this, &AFQProgramGuideWidget::qslotMouseClicked);

	if (m_nCurrentStep == 1 && authManager.GetCntChannel() > 0)
	{
		NextMission(1);
	}

	if (m_nCurrentStep == 2 && App()->GetMainView()->IsStreamActive())
	{
		NextMission(2);
	}
}

void AFQProgramGuideWidget::StartGuide()
{
	m_nCurrentStep = 0;

	ui->widget_GuideItemContainer->setVisible(true);
	ui->widget_AllClearTextArea->setVisible(false);

	QString style = "QWidget #widget_GuideContentArea { background: #24272D; border-image:none; }";
	ui->widget_GuideContentArea->setStyleSheet(style);
	ui->label_Title->setText(m_strGuideTitleText);

	m_guideItems[0]->SetGuideItemState(AFQProgramGuideItemWidget::GuideItemState::Visible);
	m_guideItems[1]->SetGuideItemState(AFQProgramGuideItemWidget::GuideItemState::Hidden);
	m_guideItems[2]->SetGuideItemState(AFQProgramGuideItemWidget::GuideItemState::Hidden);
}

void AFQProgramGuideWidget::_InitGuide()
{
	auto& locale = AFLocaleTextManager::GetSingletonInstance();

	_ClearGuideItems();

	AFQProgramGuideItemWidget* step1 = new AFQProgramGuideItemWidget(this, 1);
	AFQProgramGuideItemWidget* step2 = new AFQProgramGuideItemWidget(this, 2);
	AFQProgramGuideItemWidget* step3 = new AFQProgramGuideItemWidget(this, 3);

	connect(step1, &AFQHoverWidget::qsignalMouseClick, this, &AFQProgramGuideWidget::qslotMouseClicked);

	// Text
	m_strGuideTitleText = QTStr("Basic.Guide.Mission.Title");
	step1->SetContentText(QTStr("Basic.Guide.Mission.Step1"),
						  QTStr("Basic.Guide.Mission.ClearedStep1"));
	step2->SetContentText(QTStr("Basic.Guide.Mission.Step2"),
						  QTStr("Basic.Guide.Mission.ClearedStep2"));
	step3->SetContentText(QTStr("Basic.Guide.Mission.Step3"));
	//

	// Icon
	const char* strHiddenIcon = "assets/guide/step-locked-icon.svg";
	step1->SetIconPath(strHiddenIcon, 
					   "assets/guide/step1-default-icon.svg",
					   "assets/guide/step-checked-icon.svg");
	step2->SetIconPath(strHiddenIcon,
						"assets/guide/step2-default-icon.svg",
						"assets/guide/step-checked-icon.svg");
	step3->SetIconPath(strHiddenIcon,
						"assets/guide/step3-default-icon.svg");
	//

	// Color
	QString inactiveColor = "#333";
	step1->SetBGColor("#D1FF01", inactiveColor);
	step2->SetBGColor("#D1FF01", inactiveColor);
	step3->SetBGColor("#D1FF01", inactiveColor);
	//

	ui->verticalLayout_GuideItemContainer->addWidget(step1);
	ui->verticalLayout_GuideItemContainer->addWidget(step2);
	ui->verticalLayout_GuideItemContainer->addWidget(step3);

	m_guideItems.push_back(step1);
	m_guideItems.push_back(step2);
	m_guideItems.push_back(step3);
}

void AFQProgramGuideWidget::_ClearGuideItems()
{
	if (m_guideItems.size() > 0)
	{
		for (AFQProgramGuideItemWidget* vol : m_guideItems)
			delete vol;
		m_guideItems.clear();
	}
}