#include "CBorderPopupBaseWidget.h"
#include "ui_dock-popup-widget.h"

#include "Application/CApplication.h"

#include <QCloseEvent>

AFQBorderPopupBaseWidget::AFQBorderPopupBaseWidget(QWidget* parent, Qt::WindowFlags flag) :
	AFCQMainBaseWidget(parent, flag),
	ui(new Ui::AFQBorderPopupBaseWidget)
{
	ui->setupUi(this);
	ui->verticalLayout_dockContentsArea->setContentsMargins(1, 1, 1, 1);
}

AFQBorderPopupBaseWidget::~AFQBorderPopupBaseWidget()
{
	delete ui;
}

void AFQBorderPopupBaseWidget::AddWidget(QWidget* widget)
{
	ui->verticalLayout_dockContentsArea->addWidget(widget);
}

void AFQBorderPopupBaseWidget::AddCustomWidget(QCefWidget* widget)
{
	m_qCustomBrowserWidget = widget;
	ui->verticalLayout_dockContentsArea->addWidget(widget);
}

void AFQBorderPopupBaseWidget::SetBlockType(const char* type, int typeNum)
{
	m_cBlockType = type;
	m_iBlockType = typeNum;
}

void AFQBorderPopupBaseWidget::SetUrl(const std::string& url)
{
	if (m_qCustomBrowserWidget)
	{
		m_qCustomBrowserWidget->setURL(url);
	}
}

QWidget* AFQBorderPopupBaseWidget::GetWidgetByName(const QString& name)
{
	return this->findChild<QWidget*>(name);
}

void AFQBorderPopupBaseWidget::closeEvent(QCloseEvent* event)
{
	if (m_bCustom)
	{
        if (!event->isAccepted()) {
            return;
        }

        static int panel_version = -1;
        if (panel_version == -1) {
            panel_version = obs_browser_qcef_version();
        }

        if (panel_version >= 2 && !!m_qCustomBrowserWidget) {
            m_qCustomBrowserWidget->closeBrowser();
        }
		QString uuid = property("uuid").toString();
		emit qsignalCloseWidget(uuid);
	}
	else
	{
		if (m_iBlockType != -1)
		{
			if (App()->GetMainView()->GetMainWindow() && (App()->GetMainView()->GetMainWindow()->isWidgetType()))
			{
				App()->GetMainView()->GetMainWindow()->SwitchToDock(m_cBlockType, m_bToDock);
			}
		}
	}
}
