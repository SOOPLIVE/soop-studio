#include "CBaseDockWidget.h"
#include "QMouseEvent"
#include "QHoverEvent"
#include "QPainter"

#include <QStyle>

#include "Application/CApplication.h"

#include "MainFrame/CMainFrame.h"

AFQBaseDockWidget::AFQBaseDockWidget(int type, QWidget* parent) :
	QDockWidget(parent)
{
	this->setMouseTracking(true);
	this->setAttribute(Qt::WA_Hover);
	this->installEventFilter(this);

	connect(this, &QDockWidget::topLevelChanged, this, &AFQBaseDockWidget::qslotTopLevelChanged);
	m_dockType = type;
}

AFQBaseDockWidget::AFQBaseDockWidget(QString uuid, int customtype, QWidget* parent) :
	QDockWidget(parent)
{
	this->setMouseTracking(true);
	this->setAttribute(Qt::WA_Hover);
	this->installEventFilter(this);

	setProperty("uuid", uuid);
	setProperty("customType", customtype);

	connect(this, &QDockWidget::topLevelChanged, this, &AFQBaseDockWidget::qslotTopLevelChanged);
}


void AFQBaseDockWidget::qslotTopLevelChanged(bool toplevel)
{
	if(!m_isPressed)
		DYNAMIC_COMPOSIT->CheckDocksState();
	m_isFloating = toplevel;
	SetStyle();

	titleBarWidget()->adjustSize();
}

void AFQBaseDockWidget::qslotDataReceivedFromBrowserToDock(const QCefQuery& query)
{
	emit qsignalDataFromBrowser(m_dockType, query);
}

void AFQBaseDockWidget::qslotReceivedCreateAfterCefBrowser()
{
	emit qsignalCreateAfterCefBrowser(m_dockType);
}

void AFQBaseDockWidget::qslotReceivedLoadEndCefBrowser()
{
	emit qsignalLoadEndCefBrowser(m_dockType);
}

void AFQBaseDockWidget::qslotShowSearchTextDialog()
{
	ShowSearchTextDialog();
}

void AFQBaseDockWidget::qslotSearchText(const QString& text, bool matchCase, bool forward, bool findNext)
{
	std::string str = text.toStdString();
	SearchText(str, matchCase, forward, findNext);
}

void AFQBaseDockWidget::qslotStopSearchText(bool clearSelection)
{
	StopSearchText(clearSelection);
}

void AFQBaseDockWidget::closeEvent(QCloseEvent* event)
{
	emit qsignalCloseDock(m_dockType);

	if (m_cefWidget)
	{
		if (m_closeCef)
		{
			static int panel_version = -1;
			if (panel_version == -1) {
				panel_version = obs_browser_qcef_version();
			}

			if (panel_version >= 2 && !!m_cefWidget) {
				m_cefWidget->closeBrowser();
			}
		}
	}
}

bool AFQBaseDockWidget::event(QEvent* e)
{
	switch (e->type()) {
	case QEvent::MouseButtonPress: {
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
		if (mouseEvent->button() == Qt::LeftButton) {
			if (features() != QDockWidget::NoDockWidgetFeatures)
			{
				if (_IsOverTitleArea(mouseEvent->position().toPoint()))
				{
					m_isPressed = true;
					SetStyle();
				}
			}
		}
		break;
	}
	case QEvent::MouseButtonRelease: {
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);
		if (mouseEvent->button() == Qt::LeftButton) {
			if (_IsOverTitleArea(mouseEvent->position().toPoint()))
			{
				m_isPressed = false;
				SetStyle();
			}
		}

		DYNAMIC_COMPOSIT->CheckDocksState();
		break;
	}
	default:
		break;
	}
	return QDockWidget::event(e);
}

void AFQBaseDockWidget::SetStyle()
{
	if (!titleBarWidget() || !widget())
		return;

	// title
	titleBarWidget()->setProperty("dock", true);
	titleBarWidget()->setProperty("floating", m_isFloating);
	titleBarWidget()->setProperty("pressed", m_isPressed);

	titleBarWidget()->style()->unpolish(titleBarWidget());
	titleBarWidget()->style()->polish(titleBarWidget());

	// dock contents
	QWidget* dockContents = widget()->findChild<QWidget*>("dockContainer");
	if (dockContents) {
		dockContents->setProperty("dock", true);
		dockContents->setProperty("floating", m_isFloating);
		dockContents->setProperty("pressed", m_isPressed);
		dockContents->setProperty("tabbed", m_isTabbed);
		dockContents->style()->unpolish(dockContents);
		dockContents->style()->polish(dockContents);
	}

	if (m_isCef)
	{
		if (m_marginWidget) {
			m_marginWidget->setProperty("dock", true);
			m_marginWidget->setProperty("floating", m_isFloating);
			m_marginWidget->setProperty("pressed", m_isPressed);
			m_marginWidget->setProperty("tabbed", m_isTabbed);
			m_marginWidget->style()->unpolish(m_marginWidget);
			m_marginWidget->style()->polish(m_marginWidget);
		}
	}
}

void AFQBaseDockWidget::AddCefWidget(QCefWidget* widget, std::string url)
{
	m_isCef = true;
	m_cefUrl = url;

	m_cefWidget = widget;
	m_marginWidget = new QWidget(this);
	m_marginWidget->setObjectName("widget_DockGap");
	QVBoxLayout* layout = new QVBoxLayout();
	layout->setContentsMargins(2, 6, 2, 2);
	layout->addWidget(m_cefWidget);
	m_marginWidget->setLayout(layout);
	setWidget(m_marginWidget);
	m_cefWidget->show();

}

void AFQBaseDockWidget::SetUrl(std::string url)
{
	if (m_isCef)
	{
		if (m_cefWidget)
		{
			if (m_cefUrl != url)
				m_cefWidget->setURL(url);
			m_cefUrl = url;
		}
	}
}

void AFQBaseDockWidget::ShowSearchTextDialog()
{
	if (m_searchDlg)
		m_searchDlg->close();

	m_searchDlg = new AFQSearchDialog(this);

	connect(m_searchDlg, &AFQSearchDialog::qsignalSearchRequested,
		this, &AFQBaseDockWidget::qslotSearchText);
	connect(m_searchDlg, &AFQSearchDialog::qsignalStopSearchText,
		this, &AFQBaseDockWidget::qslotStopSearchText);

	m_searchDlg->setModal(false);
	m_searchDlg->show();
}

void AFQBaseDockWidget::SearchText(std::string& text, bool matchCase, bool forward, bool findNext)
{
	if (m_isCef)
		m_cefWidget->searchText(text, matchCase, forward, findNext);
}

void AFQBaseDockWidget::StopSearchText(bool clearSelection)
{
	if (m_isCef)
		m_cefWidget->stopSearchtext(clearSelection);
}

bool AFQBaseDockWidget::_IsOverTitleArea(QPoint point)
{
	if (!titleBarWidget())
		return false;

	QRect titleArea = titleBarWidget()->rect();
	if (titleArea.contains(point))
		return true;
	return false;
}
