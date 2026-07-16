#include "CCateChangeDialog.h"

#include "ui_category-change-dialog.h"

#include "CoreModel/Auth/CAuthManager.h"

#include "MainFrame/CMainFrame.h"

#include "PopupWindows/BroadInfoPopup/CAgeRestrictionPolicyDialog.h"

AFQCateChangeDialog::AFQCateChangeDialog(QWidget* parent, const char* id) :
	AFTTopBaseDialog(parent),
	ui(new Ui::AFQCateChangeDialog)
{
	ui->setupUi(this);

	ui->frameCategoryInfo->hide();
	ui->frameAnimeCategoryInfo->hide();
	ui->animeAdultInfoFrame->hide();

#ifdef _WIN32
    ui->titleFrame->setProperty("MoveInAllArea", true);
#elif defined(__APPLE__)
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif
    
	m_sourceId = id;

	connect(ui->buttonClose, &QPushButton::clicked, 
		this, &AFQCateChangeDialog::qslotCloseButtonClicked);

	connect(ui->pushButton_Change, &QPushButton::clicked,
		this, &AFQCateChangeDialog::qslotAcceptButtonClicked);

	connect(ui->pushButton_Cancel, &QPushButton::clicked,
		this, &AFQCateChangeDialog::qslotCloseButtonClicked);

	connect(ui->pushButton_AdultInfo, &QPushButton::clicked,
		this, &AFQCateChangeDialog::qslotShowAdultInfoButtonClicked);
}

AFQCateChangeDialog::~AFQCateChangeDialog()
{
	delete ui;
}


void AFQCateChangeDialog::AddAllowedCategoryInfo(std::list<int>& categoryNums)
{
	m_allowedCateLists = categoryNums;

	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	if (!broadInfo)
		return;

	const char* displayName = obs_source_get_display_name(m_sourceId.c_str());

	QString str = QString(QTStr("Basic.ChangeCategoryPopup.InfoMessage"))
		.arg(displayName ? displayName : "");

	ui->label_Info->setText(str);

	setFixedSize(480, 340);

	if (!m_pButtonGroup) {
		m_pButtonGroup = new QButtonGroup(this);
		m_pButtonGroup->setExclusive(true);
	}

	int idx = 0;

	for (int categoryNum : m_allowedCateLists) {
		QPushButton* cateButton = new QPushButton(this);
		cateButton->setObjectName("pushButton_ChangeCategory");
		cateButton->setCheckable(true);
		cateButton->setChecked(idx == 0);

		QString categoryName;
		QString formatCategory;

		broadInfo->ReceiveCategoryString(categoryNum, categoryName);

		if (categoryNum % 10000 != 0) {
			QString parentCategory;
			broadInfo->ReceiveCategoryString(categoryNum, parentCategory, true);
			formatCategory = QString("• %1 > %2")
				.arg(parentCategory)
				.arg(categoryName);
		}
		else {
			formatCategory = QString("• %1").arg(categoryName);
		}

		cateButton->setText(formatCategory);

		m_pButtonGroup->addButton(cateButton, categoryNum);
		ui->layoutSelectCategory->addWidget(cateButton);

		idx++;
	}

	if (idx != 1) {
		const int adjustHeight = 83 + (29 * (idx - 1));
		ui->frameCategoryInfo->setFixedHeight(adjustHeight);
	}

	ui->frameCategoryInfo->show();

	MoveToParentCenter();

	SOOP_API_HANDLER->getAPIfromId(GET_CATEGORY_LIST, {},
		this, "qslotResponseCategorysAPI");
}

void AFQCateChangeDialog::AddAllowedAnimeAdultCategoryInfo(int categoryNum)
{
	m_allowedAdultCateNum = categoryNum;

	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	if (!broadInfo)
		return;

	QString str = QString(QTStr("Basic.ChangeCategoryPopup.InfoMessageAdultContent"));
	ui->label_Info->setText(str);

	setFixedSize(480, 370);

	ui->pushButton_AnimeCategory->setCheckable(true);
	ui->pushButton_AnimeCategory->setChecked(true);

	// Before : Receive API Category Data
	QString categoryName;
	QString parentCategory;

	broadInfo->ReceiveCategoryString(m_allowedAdultCateNum, categoryName);
	broadInfo->ReceiveCategoryString(m_allowedAdultCateNum, parentCategory, true);

	const QString formatCategory =
		QString("%1 > %2").arg(parentCategory).arg(categoryName);

	ui->pushButton_AnimeCategory->setText(formatCategory);

	m_pButtonGroup = new QButtonGroup(this);
	m_pButtonGroup->setExclusive(true);
	m_pButtonGroup->addButton(ui->pushButton_AnimeCategory, m_allowedAdultCateNum);

	ui->label_adultValue->setText(QTStr("Basic.ChangeCategoryPopup.AdultSetting_On"));

	ui->frameAnimeCategoryInfo->show();
	ui->animeAdultInfoFrame->show();

	MoveToParentCenter();

	SOOP_API_HANDLER->getAPIfromId(GET_CATEGORY_LIST, {},
		this, "qslotResponseAdultCategorysAPI");
}

void AFQCateChangeDialog::MoveToParentCenter()
{
	QWidget* baseParent = parentWidget();

	if (!baseParent)
		baseParent = MAINFRAME;

	if (!baseParent)
		return;

	QWidget* parentWindow = baseParent->window();
	if (!parentWindow)
		parentWindow = baseParent;

	adjustSize();

	const QRect parentRect = parentWindow->frameGeometry();

	QPoint pos =
		parentRect.center() - QPoint(width() / 2, height() / 2);

	QScreen* screen = QGuiApplication::screenAt(parentRect.center());
	if (!screen)
		screen = QGuiApplication::primaryScreen();

	if (screen) {
		const QRect available = screen->availableGeometry();

		pos.setX(qBound(
			available.left(),
			pos.x(),
			available.right() - width()
		));

		pos.setY(qBound(
			available.top(),
			pos.y(),
			available.bottom() - height()
		));
	}

	move(pos);
}

void AFQCateChangeDialog::qslotResponseCategorysAPI(const QByteArray& responseData)
{
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	if (!broadInfo)
		return;

	broadInfo->RefreshCategoryList(responseData);

	if (!m_pButtonGroup)
		return;

	for (QAbstractButton* button : m_pButtonGroup->buttons()) {
		QPushButton* cateButton = qobject_cast<QPushButton*>(button);
		if (!cateButton)
			continue;

		const int categoryNum = m_pButtonGroup->id(button);

		QString categoryName;
		QString formatCategory;

		broadInfo->ReceiveCategoryString(categoryNum, categoryName);

		if (categoryNum % 10000 != 0) {
			QString parentCategory;
			broadInfo->ReceiveCategoryString(categoryNum, parentCategory, true);
			formatCategory = QString("• %1 > %2")
				.arg(parentCategory)
				.arg(categoryName);
		}
		else {
			formatCategory = QString("• %1").arg(categoryName);
		}

		cateButton->setText(formatCategory);
	}
}

void AFQCateChangeDialog::qslotResponseAdultCategorysAPI(const QByteArray& responseData)
{
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	if (!broadInfo)
		return;

	broadInfo->RefreshCategoryList(responseData);

	QString categoryName;
	QString parentCategory;

	broadInfo->ReceiveCategoryString(m_allowedAdultCateNum, categoryName);
	broadInfo->ReceiveCategoryString(m_allowedAdultCateNum, parentCategory, true);

	const QString formatCategory =
		QString("%1 > %2").arg(parentCategory).arg(categoryName);

	if (ui->pushButton_AnimeCategory->text() != formatCategory)
		ui->pushButton_AnimeCategory->setText(formatCategory);
}

void AFQCateChangeDialog::qslotCloseButtonClicked()
{
	close();
}

void AFQCateChangeDialog::qslotAcceptButtonClicked()
{
	m_allowedCategory = m_pButtonGroup->checkedId();

	accept();
}

void AFQCateChangeDialog::qslotShowAdultInfoButtonClicked()
{
	AFQAgeRestrictionPolicyDialog* dialog = new AFQAgeRestrictionPolicyDialog(this);
	dialog->exec();
}
