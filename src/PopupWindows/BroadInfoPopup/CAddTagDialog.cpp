#include "CAddTagDialog.h"
#include "ui_add-tag-dialog.h"

#include <QUrlQuery>

#include "CoreModel/Auth/CAuthManager.h"

#include "Application/CApplication.h"
#include "Common/StudioDefine.h"

AFQAddTagDialog::AFQAddTagDialog(QWidget* parent)
	: AFTTopBaseDialog(parent),
	ui(new Ui::AFQAddTagDialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
#ifdef _WIN32
	AFQBlockManager::ApplyMoveInAllArea(this);
#elif defined(__APPLE__)
    setWindowTitle(QTStr("BroadTag"));
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleFrame->hide();
#endif
	SetWidthResizeEnabled(false);
	SetHeightResizeEnabled(false);

	_Init();
}

AFQAddTagDialog::~AFQAddTagDialog()
{
	delete ui;
}

void AFQAddTagDialog::_qslotAddTagDataReceived(const QCefQuery& query)
{
	QWidget* senderWidget = qobject_cast<QWidget*>(sender());
}

void AFQAddTagDialog::closeEvent(QCloseEvent* event)
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

void AFQAddTagDialog::_Init()
{
	QUrl url(QString::fromStdString(TAG_SETTING_URL));
	QUrlQuery query;

	// HashTags
	AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
	std::string hashTags = broadInfo->MergeTag(broadInfo->HashTags());

	if (!hashTags.empty()) {
		query.addQueryItem("hashtags", QString::fromStdString(hashTags));
	}
	query.addQueryItem("mode", "dark");

	url.setQuery(query);
	//

	std::string fullUrl = url.toString().toStdString();
	m_pCefWidget = CEFMANAGER.createWidget(this, fullUrl);
	if (m_pCefWidget) 
	{
		connect(m_pCefWidget, SIGNAL(cefQueryRequest(const QCefQuery&)), this, SLOT(_qslotAddTagDataReceived(const QCefQuery&)));
		ui->verticalLayout_Contents->addWidget(m_pCefWidget);
	}
	
	connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQAddTagDialog::close);
}
