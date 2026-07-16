#include "CBorderPopupBaseWidget.h"
#include "ui_dock-popup-widget.h"

#include "Application/CApplication.h"

#include <QCloseEvent>
#include <QMouseEvent>

#ifdef __APPLE__
#include "macUI/macos_ui_helper.h"
#endif

// No Type = -1
AFQBorderPopupBaseWidget::AFQBorderPopupBaseWidget(int windowtype, QWidget* parent, Qt::WindowFlags flag,
	bool widthResizable, bool heightResizable) :
	AFTTopBaseWidget(parent, flag),
	ui(new Ui::AFQBorderPopupBaseWidget)
{
	ui->setupUi(this);
	ui->titleFrame->hide();

#ifdef __APPLE__
	const ENUM_WINDOW_TYPE val = static_cast<ENUM_WINDOW_TYPE>(windowtype);
    switch(val)
    {
        case ENUM_WINDOW_TYPE::BroadInfo:
        case ENUM_WINDOW_TYPE::SceneSource:
        case ENUM_WINDOW_TYPE::AudioMixer:
            addMacOSTitleBarButton((void*)this->winId(), windowtype, (void*)this);
            break;
            
        default:
            break;
            
    }
    setWindowFlags(Qt::Dialog);
#endif
	
	SetWidthResizeEnabled(widthResizable);
	SetHeightResizeEnabled(heightResizable);	

	ui->dockPopupContainerLayout->setContentsMargins(1, 1, 1, 1);
	m_blockType = windowtype;
}

AFQBorderPopupBaseWidget::AFQBorderPopupBaseWidget(QString uuid, int customtype, QWidget* parent, Qt::WindowFlags flag,
	bool widthResizable, bool heightResizable) :
	AFTTopBaseWidget(parent, flag),
	ui(new Ui::AFQBorderPopupBaseWidget)
{
	ui->setupUi(this);
	ui->titleFrame->hide();

	SetWidthResizeEnabled(widthResizable);
	SetHeightResizeEnabled(heightResizable);

	ui->dockPopupContainerLayout->setContentsMargins(1, 1, 1, 1);
	setProperty("uuid", uuid);
	setProperty("customType", customtype);
	connect(this, &AFQBorderPopupBaseWidget::qsignalCloseCustom,
		MAIN_BLOCKMANAGER, &AFQBlockManager::qslotCloseCustom);

	connect(this, &AFQBorderPopupBaseWidget::qsignalHideCustom,
		MAIN_BLOCKMANAGER, &AFQBlockManager::qslotHideCustom);


	AFQBorderPopupBaseWidget* parentPopup = qobject_cast<AFQBorderPopupBaseWidget*>(parent);
	if (parentPopup)
		connect(parentPopup, &AFQBorderPopupBaseWidget::qsignalCloseChildren,
			this, &AFQBorderPopupBaseWidget::close);
}

AFQBorderPopupBaseWidget::~AFQBorderPopupBaseWidget()
{
	m_searchDlg = nullptr;

	delete ui;
}

void AFQBorderPopupBaseWidget::qslotDataReceivedFromBrowser(const QCefQuery& query)
{
	qDebug() << "Get Query";
	emit qsignalDataFromBrowser(m_blockType, query);
}

void AFQBorderPopupBaseWidget::qslotReceivedCreateAfterCefBrowser()
{
	emit qsignalCreateAfterCefBrowser(m_blockType);
}

void AFQBorderPopupBaseWidget::qslotReceivedLoadEndCefBrowser()
{
	emit qsignalLoadEndCefBrowser(m_blockType);
}

void AFQBorderPopupBaseWidget::qslotShowSearchTextDialog()
{
	ShowSearchTextDialog();
}

void AFQBorderPopupBaseWidget::qslotSearchText(const QString& text, bool matchCase, bool forward, bool findNext)
{
	std::string str = text.toStdString();
	SearchText(str, matchCase, forward, findNext);
}

void AFQBorderPopupBaseWidget::qslotStopSearchText(bool clearSelection)
{
	StopSearchText(clearSelection);
}

void AFQBorderPopupBaseWidget::qslotMainFrameMovedOrResized()
{
	if (!m_isMagnetPopup)
		return;
	
	QPoint mainFrameGlobalPos = MAINFRAME->geometry().topLeft();
	QRect mainFrameGeo(mainFrameGlobalPos, MAINFRAME->size());

	QPoint newPopupPos = mainFrameGeo.topLeft() + m_snapOffset;

	switch (m_snapState.h) {
	case HSnap::LeftToLeft:   newPopupPos.setX(mainFrameGeo.left()); break;
	case HSnap::LeftToRight:  newPopupPos.setX(mainFrameGeo.right()); break;
	case HSnap::RightToLeft:  newPopupPos.setX(mainFrameGeo.left() - width()); break;
	case HSnap::RightToRight: newPopupPos.setX(mainFrameGeo.right() - width()); break;
	case HSnap::None: default: break;
	}

	switch (m_snapState.v) {
	case VSnap::TopToTop:     newPopupPos.setY(mainFrameGeo.top()); break;
	case VSnap::TopToBottom:  newPopupPos.setY(mainFrameGeo.bottom()); break;
	case VSnap::BottomToTop:  newPopupPos.setY(mainFrameGeo.top() - height()); break;
	case VSnap::BottomToBottom: newPopupPos.setY(mainFrameGeo.bottom() - height()); break;
	case VSnap::None: default: break;
	}

	if (m_snapState.h == HSnap::None &&
		m_snapState.v == VSnap::None)
		return;

	this->raise();
	move(newPopupPos);
}

void AFQBorderPopupBaseWidget::SetIsMagnetPopup(bool isMagnetPopup)
{
	m_isMagnetPopup = isMagnetPopup;
	if (m_isMagnetPopup) {
		m_previousPosition = _GetFinalPosToMainFrame(this->geometry().topLeft());
		connect(MAINFRAME, &AFMainFrame::qsignalmovedOrResized, this, &AFQBorderPopupBaseWidget::qslotMainFrameMovedOrResized, Qt::UniqueConnection);
	}
	else {
		disconnect(MAINFRAME, &AFMainFrame::qsignalmovedOrResized, this, &AFQBorderPopupBaseWidget::qslotMainFrameMovedOrResized);
	}
}


void AFQBorderPopupBaseWidget::AddWidget(QWidget* widget)
{
	m_pContent = widget;
	ui->dockPopupContainerLayout->addWidget(widget);
}

void AFQBorderPopupBaseWidget::AddCefWidget(QCefWidget* widget, std::string strUrl)
{
	m_isCef = true;
	m_cefUrl = strUrl;
	m_pCustomBrowserWidget = widget;
	ui->dockPopupContainerLayout->addWidget(widget);
}

void AFQBorderPopupBaseWidget::SetUrl(const std::string& url)
{
	if (m_isCef)
	{
		if (m_pCustomBrowserWidget)
		{
			if(url != m_cefUrl)
				m_pCustomBrowserWidget->setURL(url);
			m_cefUrl = url;
		}
	}
}

QWidget* AFQBorderPopupBaseWidget::GetWidgetByName(const QString& name)
{
	return this->findChild<QWidget*>(name);
}

void AFQBorderPopupBaseWidget::ExecuteScript(const std::string& script)
{
	if (m_isCef)
		m_pCustomBrowserWidget->executeJavaScript(script);
}

void AFQBorderPopupBaseWidget::ReloadCefWidget()
{
	if (m_isCef)
		m_pCustomBrowserWidget->reloadPage();
}

void AFQBorderPopupBaseWidget::SetDockContentsMargin(int left, int top, int right, int bottom)
{
	ui->dockPopupContainerLayout->setContentsMargins(left, top, right, bottom);
}

DWORD AFQBorderPopupBaseWidget::GetPid()
{
	return GetCurrentProcessId();
}

void AFQBorderPopupBaseWidget::ShowSearchTextDialog()
{
	if (m_searchDlg)
		m_searchDlg->close();

	m_searchDlg = new AFQSearchDialog(this);

	connect(m_searchDlg, &AFQSearchDialog::qsignalSearchRequested, 
			this, &AFQBorderPopupBaseWidget::qslotSearchText);
	connect(m_searchDlg, &AFQSearchDialog::qsignalStopSearchText, 
			this, &AFQBorderPopupBaseWidget::qslotStopSearchText);

	m_searchDlg->setModal(false);
	m_searchDlg->show();
}

void AFQBorderPopupBaseWidget::SearchText(std::string& text, bool matchCase, bool forward, bool findNext)
{
	if (m_isCef)
		m_pCustomBrowserWidget->searchText(text, matchCase, forward, findNext);
}

void AFQBorderPopupBaseWidget::StopSearchText(bool clearSelection)
{
	if (m_isCef)
		m_pCustomBrowserWidget->stopSearchtext(clearSelection);
}

void AFQBorderPopupBaseWidget::MoveMagnet(QPoint movePos)
{
	QPoint proposedPos = movePos;
	QPoint finalPos = proposedPos;

	const int snapDistance = 12;
	finalPos = _GetFinalPosToMainFrame(proposedPos, snapDistance);
	
	move(finalPos);
}

void AFQBorderPopupBaseWidget::RefreshMagnetOffset(QPoint point)
{
	if (m_isMagnetPopup)
	{
		QRect mainFrameGeo(MAINFRAME->geometry().topLeft(), MAINFRAME->size());
		m_snapOffset = point - mainFrameGeo.topLeft();
	}
	
}

void AFQBorderPopupBaseWidget::MoveToPrevious()
{
	move(m_previousPosition);
}

void AFQBorderPopupBaseWidget::hideEvent(QHideEvent* event)
{
	if (m_isHidePopup)
	{
		emit qsignalCloseChildren();
		QString uuid = property("uuid").toString();
		int type = property("customType").toInt();
		emit qsignalHideCustom(uuid, type);
	}
}

void AFQBorderPopupBaseWidget::closeEvent(QCloseEvent* event)
{
	if (m_isCef && m_blockType > ENUM_WINDOW_TYPE::BLOCKITER)
	{
		if (!event->isAccepted())
			return;

		if (!m_isHidePopup)
		{
			static int panel_version = -1;
			if (panel_version == -1)
				panel_version = obs_browser_qcef_version();

			if (panel_version >= 2 && !!m_pCustomBrowserWidget)
				m_pCustomBrowserWidget->closeBrowser();
		}
	}


#ifdef __APPLE__
    removeMacOSTitleBarButton((void*)this->winId(), m_blockType);
#endif
    
	if (m_blockType != -1)
	{
		emit qsignalCloseChildren();
		if(m_blockType == ENUM_WINDOW_TYPE::SoopChat)
			emit qsignalCloseChatEventTriggered(m_toDock);
		else
			emit qsignalCloseEventTriggered(m_blockType);
	}
	else
	{
		emit qsignalCloseChildren();
		QString uuid = property("uuid").toString();
		int type = property("customType").toInt();
		emit qsignalCloseCustom(uuid, type);
	}

	event->accept();
}

void AFQBorderPopupBaseWidget::showEvent(QShowEvent* event)
{
	//if (!MAINFRAME->IsSmallResolution())
	//{
	//	if (m_blockType == ENUM_WINDOW_TYPE::Mission || m_blockType == ENUM_WINDOW_TYPE::SAVVYReaction)
	//		resize(550, 880);
	//}
}

void AFQBorderPopupBaseWidget::mousePressEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
		m_checkIsPressed = true;
		event->accept();
	}
}

void AFQBorderPopupBaseWidget::mouseReleaseEvent(QMouseEvent* event)
{
	if (event->button() == Qt::LeftButton) {
		m_checkIsPressed = false;
		event->accept();
	}
}

void AFQBorderPopupBaseWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (event->buttons() & Qt::LeftButton) {
		if (m_isMagnetPopup && m_checkIsPressed) {
			if (isMaximized())
			{
				showNormal();

				QSize normalSize = minimumSize();
				QPoint cursorPos = event->globalPos();
				int x = cursorPos.x() - normalSize.width() / 2;
				int y = cursorPos.y() - 10; 
				MoveMagnet(QPoint(x, y));

				m_dragPosition = event->globalPos() - frameGeometry().topLeft();
				return;
			}
			MoveMagnet(event->globalPosition().toPoint() - m_dragPosition);
		}
		else
		{
			if (isMaximized())
			{
				showNormal();

				QSize normalSize = minimumSize();
				QPoint cursorPos = event->globalPos();
				int x = cursorPos.x() - normalSize.width() / 2;
				int y = cursorPos.y() - 10;
				MoveMagnet(QPoint(x, y));

				m_dragPosition = event->globalPos() - frameGeometry().topLeft();
				return;
			}

			if(m_isCef && m_checkIsPressed)
				move(event->globalPosition().toPoint() - m_dragPosition);
		}
		event->accept();
	}
}

//bool AFQBorderPopupBaseWidget::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
//{
//#ifdef _WIN32
//	PMSG msg = (PMSG)message;
//
//	if (msg->message == WM_NCHITTEST) {
//
//		AFTTopBaseWidget::nativeEvent(eventType, message, result);
//		if (*result >= HTLEFT && *result <= HTBOTTOMRIGHT)
//			return true;
//
//		*result = HTCLIENT;
//
//		return true;
//	}
//#endif
//	return AFTTopBaseWidget::nativeEvent(eventType, message, result);
//}

QPoint AFQBorderPopupBaseWidget::_GetFinalPosToMainFrame(const QPoint& proposedPos, int snapDistance)
{
	QPoint finalPos = proposedPos;
	QRect proposedGeo(proposedPos, this->size());
	//QPoint mainFrameGlobalPos = MAINFRAME->mapToGlobal(QPoint(0, 0));
	//
	//QRect mainFrameGeo(mainFrameGlobalPos, MAINFRAME->size());
	QRect mainFrameGeo(MAINFRAME->geometry().topLeft(), MAINFRAME->size());

	QRect snapZone = mainFrameGeo.adjusted(-snapDistance, -snapDistance, snapDistance, snapDistance);

	bool adjacency = false;
	bool horizontal_adjacency = (snapZone.right() + 1 == proposedGeo.left()) || (proposedGeo.right() + 1 == snapZone.left());
	if (horizontal_adjacency) {
		int overlap_top = std::max(snapZone.top(), proposedGeo.top());
		int overlap_bottom = std::min(snapZone.bottom(), proposedGeo.bottom());
		if (overlap_top <= overlap_bottom) {
			adjacency = true;
		}
	}
	bool vertical_adjacency = (snapZone.bottom() + 1 == proposedGeo.top()) || (proposedGeo.bottom() + 1 == snapZone.top());
	if (!adjacency && vertical_adjacency) {
		int overlap_left = std::max(snapZone.left(), proposedGeo.left());
		int overlap_right = std::min(snapZone.right(), proposedGeo.right());
		if (overlap_left <= overlap_right) {
			adjacency = true;
		}
	}

	if (0 == snapDistance)
		snapDistance = 1;

	if (adjacency || proposedGeo.intersects(snapZone)) {
		m_snapState = { HSnap::None, VSnap::None };

		if (qAbs(proposedGeo.left() - mainFrameGeo.left()) <= snapDistance) {
			finalPos.setX(mainFrameGeo.left());
			m_snapState.h = HSnap::LeftToLeft;
		}
		else if (qAbs(proposedGeo.right() - mainFrameGeo.right()) <= snapDistance) {
			finalPos.setX(mainFrameGeo.right() - proposedGeo.width());
			m_snapState.h = HSnap::RightToRight;
		}
		else if (qAbs(proposedGeo.left() - mainFrameGeo.right()) <= snapDistance) {
			finalPos.setX(mainFrameGeo.right());
			m_snapState.h = HSnap::LeftToRight;
		}
		else if (qAbs(proposedGeo.right() - mainFrameGeo.left()) <= snapDistance) {
			finalPos.setX(mainFrameGeo.left() - proposedGeo.width());
			m_snapState.h = HSnap::RightToLeft;
		}

		if (qAbs(proposedGeo.top() - mainFrameGeo.top()) <= snapDistance) {
			finalPos.setY(mainFrameGeo.top());
			m_snapState.v = VSnap::TopToTop;
		}
		else if (qAbs(proposedGeo.bottom() - mainFrameGeo.bottom()) <= snapDistance) {
			finalPos.setY(mainFrameGeo.bottom() - proposedGeo.height());
			m_snapState.v = VSnap::BottomToBottom;
		}
		else if (qAbs(proposedGeo.top() - mainFrameGeo.bottom()) <= snapDistance) {
			finalPos.setY(mainFrameGeo.bottom());
			m_snapState.v = VSnap::TopToBottom;
		}
		else if (qAbs(proposedGeo.bottom() - mainFrameGeo.top()) <= snapDistance) {
			finalPos.setY(mainFrameGeo.top() - proposedGeo.height());
			m_snapState.v = VSnap::BottomToTop;
		}
		if (finalPos != proposedPos) {
			m_snapOffset = finalPos - mainFrameGeo.topLeft();
		}
	}
	else {
		m_snapState = { HSnap::None, VSnap::None };
	}

	return finalPos;
}