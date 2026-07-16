#include "CCategoryDialog.h"
#include "ui_category-dialog.h"

#include <QUrlQuery>

#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "Common/StudioDefine.h"


AFQCategoryDialog::AFQCategoryDialog(QWidget* parent, const std::string& categoryNum)
	: AFTTopBaseDialog(parent),
	ui(new Ui::AFQCategoryDialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);

#ifdef _WIN32
	AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowTitle(QTStr("BroadCategory"));
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif
	SetWidthResizeEnabled(false);
	SetHeightResizeEnabled(false);
	_Init(categoryNum);

}

AFQCategoryDialog::~AFQCategoryDialog()
{
	delete ui;
}

void AFQCategoryDialog::_qslotCategoryDataReceived(const QCefQuery& query)
{
	std::string err;
}

void AFQCategoryDialog::_qslotSoopGeoBlockCheckAPIResponse(const QByteArray& responseData)
{
	std::string jsonString = responseData.toStdString();
	std::string err;

	emit qsignalCategoryChanged( m_pendingCategoryNum, m_pendingCategoryName.toStdString());
	close();
}

void AFQCategoryDialog::showEvent(QShowEvent* event)
{
	if (MAINFRAME->IsSmallResolution())
	{
		setFixedHeight(550);
		resize(width(), 550);
	}
}

void AFQCategoryDialog::closeEvent(QCloseEvent* event)
{
	if (m_pCefWidget) 
	{
		static int panel_version = -1;
		if (panel_version == -1)
			panel_version = obs_browser_qcef_version();

		if (panel_version >= 2 && !!m_pCefWidget)
			m_pCefWidget->closeBrowser();
	}

	event->accept();
}

void AFQCategoryDialog::_Init(const std::string& categoryNum)
{
	QUrl url(QString::fromStdString(CATEGORY_SETTING_URL));
	QUrlQuery query;

	// Set Query
	query.addQueryItem("noTitle", "true");
	query.addQueryItem("categoryNum", QString::fromStdString(categoryNum));
	query.addQueryItem("mode", "dark");
	url.setQuery(query);

	std::string fullUrl = url.toString().toStdString();

	m_pCefWidget = CEFMANAGER.createWidget(nullptr, fullUrl);
	if (m_pCefWidget) 
	{
		connect(m_pCefWidget, SIGNAL(cefQueryRequest(const QCefQuery&)), this, SLOT(_qslotCategoryDataReceived(const QCefQuery&)));
		ui->verticalLayout_Contents->addWidget(m_pCefWidget);
	}
	
	connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQCategoryDialog::close);
}
