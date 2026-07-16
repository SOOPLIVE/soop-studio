#pragma once
#include "MainFrame/CMainBaseWidget.h"
#include "CoreModel/Browser/CCefManager.h"

#include "UIComponent/CTopBaseWindow.h"

#include "PopupWindows/CSearchTextDialog.h"

#include <windows.h>

namespace Ui {
    class AFQBorderPopupBaseWidget;
}

class AFQBorderPopupBaseWidget final : public AFTTopBaseWidget
{
    Q_OBJECT
#pragma region class initializer, destructor
public:
    explicit AFQBorderPopupBaseWidget(int windowtype, QWidget* parent = nullptr,
        Qt::WindowFlags flag = Qt::WindowFlags(),
        bool widthResizable = true, bool heightResizable = true);
    explicit AFQBorderPopupBaseWidget(QString uuid, int customtype, QWidget* parent = nullptr,
        Qt::WindowFlags flag = Qt::WindowFlags(),
        bool widthResizable = true, bool heightResizable = true);
    ~AFQBorderPopupBaseWidget();

public slots:
    void qslotDataReceivedFromBrowser(const QCefQuery& query);
    void qslotReceivedCreateAfterCefBrowser();
    void qslotReceivedLoadEndCefBrowser();
    void qslotShowSearchTextDialog();
    void qslotSearchText(const QString& text, bool matchCase, bool forward, bool findNext);
    void qslotStopSearchText(bool clearSelection);
    void qslotMainFrameMovedOrResized();

signals:
    void qsignalHideCustom(QString uuid, int type);
    void qsignalCloseCustom(QString uuid, int type);
    void qsignalCloseEventTriggered(int type);
    void qsignalCloseChatEventTriggered(bool skipChildClose);
    void qsignalDataFromBrowser(int blockType, const QCefQuery& query);
    void qsignalCreateAfterCefBrowser(int type);
    void qsignalLoadEndCefBrowser(int type);
    void qsignalCloseChildren();
#pragma endregion class initializer, destructor

#pragma region public func
public:
    void AddWidget(QWidget* widget);
    void AddCefWidget(QCefWidget* widget, std::string strUrl = "");
    void SetToDockValue(bool toDock) { m_toDock = toDock; };
    void SetUrl(const std::string& url);
    void ReloadCefWidget();
    void ExecuteScript(const std::string& script);
    QWidget* GetWidgetByName(const QString& name);
    void SetIsCef(bool iscef) { m_isCef = iscef; };
    bool IsCef() { return m_isCef; };
    void SetIsChatPopup(bool isChatPopup) { m_isChatPopup = isChatPopup; };
    void SetIsHidePopup(bool isHidePopup) { m_isHidePopup = isHidePopup; };
    void SetIsMagnetPopup(bool isMagnetPopup);
    void SetDockContentsMargin(int left, int top, int right, int bottom);

    QCefWidget* CefWidget() { return m_pCustomBrowserWidget; };
    QWidget* ContentWidget() { return m_pContent; };

    //TEST
    DWORD GetPid();

    void ShowSearchTextDialog();
    void SearchText(std::string& text, bool matchCase, bool forward, bool findNext);
    void StopSearchText(bool clearSelection);

    int GetBlockType() { return m_blockType; };
    bool GetIsChatPopup() { return m_isChatPopup; }
    bool GetIsHidePopup() { return m_isHidePopup; }

    void MoveMagnet(QPoint movePos);
    void RefreshMagnetOffset(QPoint point);
    void MoveToPrevious();

#pragma endregion public func

#pragma region protected func
protected:
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

    //bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

#pragma endregion protected func

#pragma region private func
private:
	QPoint _GetFinalPosToMainFrame(const QPoint& proposedPos, int snapDistance = 0);
#pragma endregion private func

#pragma region private member var
private:
    Ui::AFQBorderPopupBaseWidget* ui;
    
    bool m_toDock = false;
    bool m_isCef = false;
    bool m_isChatPopup = false;
    bool m_isHidePopup = false;

    bool m_isMagnetPopup = false;

    QCefWidget* m_pCustomBrowserWidget = nullptr;
    QWidget* m_pContent = nullptr;

    QPointer<AFQSearchDialog> m_searchDlg;

    int m_blockType = -1;
    int m_checkIsPressed = false;

    std::string m_cefUrl = "";

    enum class HSnap { None, LeftToLeft, LeftToRight, RightToLeft, RightToRight };
    enum class VSnap { None, TopToTop, TopToBottom, BottomToTop, BottomToBottom };
    struct SnapState {
        HSnap h = HSnap::None;
        VSnap v = VSnap::None;
    };

    SnapState m_snapState;
    QPoint m_snapOffset;
    QPoint m_dragPosition;
    QPoint m_previousPosition;
#pragma endregion private member var
};

