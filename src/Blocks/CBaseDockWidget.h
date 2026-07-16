#pragma once

#include <QDockWidget>
#include "CoreModel/Browser/CCefManager.h"

#include "PopupWindows/CSearchTextDialog.h"

class AFQBaseDockWidget : public QDockWidget
{
	Q_OBJECT

signals:
	void closed();
	void qsignalCloseDock(int type);
	void qsignalDataFromBrowser(int blockType, const QCefQuery& query);
	void qsignalCreateAfterCefBrowser(int type);
	void qsignalLoadEndCefBrowser(int type);

public slots:
	void qslotTopLevelChanged(bool toplevel);
	void qslotDataReceivedFromBrowserToDock(const QCefQuery& query);
	void qslotReceivedCreateAfterCefBrowser();
	void qslotReceivedLoadEndCefBrowser();
	void qslotShowSearchTextDialog();
	void qslotSearchText(const QString& text, bool matchCase, bool forward, bool findNext);
	void qslotStopSearchText(bool clearSelection);

public:
	explicit AFQBaseDockWidget(int type, QWidget* parent = nullptr);   
	explicit AFQBaseDockWidget(QString uuid, int customtype, QWidget* parent = nullptr);

public:
	void SetPressedState(bool pressed) { m_isPressed = pressed; }
	void SetStyle();
	int GetDockType() { return m_dockType; };
	bool IsCefDock() { return m_isCef; };
	QCefWidget* GetCefWidget() { return m_cefWidget; };
	void AddCefWidget(QCefWidget* cefwidget, std::string url = "");
	void SetUrl(std::string url);
	void SetCloseCef(bool closeCef) { m_closeCef = closeCef; };
	void SetToPopup(bool setPopup) { m_isToPopup = setPopup; };

	void ShowSearchTextDialog();
	void SearchText(std::string& text, bool matchCase, bool forward, bool findNext);
	void StopSearchText(bool clearSelection);

protected:
	bool event(QEvent* e) override;
	void closeEvent(QCloseEvent* event) override;

private:
	bool _IsOverTitleArea(QPoint point);

private:
	bool    m_isFloating = false;
	bool    m_isPressed = false;
	bool    m_isTabbed = false;   
	bool    m_isCef = false;
	bool    m_isToPopup = false;

	int     m_dockType = -1;
	QCefWidget* m_cefWidget = nullptr;
	QWidget* m_marginWidget = nullptr;

	std::string m_cefUrl;
	bool m_closeCef = true;

	QPointer<AFQSearchDialog> m_searchDlg;
};
