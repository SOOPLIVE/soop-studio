#ifndef CBROADINFODOCKWIDGET_H
#define CBROADINFODOCKWIDGET_H

#include <QWidget>
#include <QPointer>
#include <QBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QTimer>
#include <QScrollArea>

namespace Ui {
    class AFBroadInfoDockWidget;
}

class AFBroadInfoDockWidget : public QWidget
{
    Q_OBJECT

    enum class LayoutDirection 
    {
        Vertical,
        Horizontal
    };

public:
    explicit AFBroadInfoDockWidget(QWidget *parent = nullptr);
    ~AFBroadInfoDockWidget();

public slots:
    void qslotStartStreamingInfoTimer();
    void qslotStopStreamingInfoTimer();
    void qslotSreamingToggled(bool stream);
    void qslotReceiveSubscribeBroadAvailable(const QByteArray& responseData);

    void qslotFinishEditingBroadTitle();

private slots:
    void _qslotBroadTitleLineSizeChanged(bool multiLine);
    void _qslotClickedEditingBroadTitleButton();
    void _qslotHideUserCountChanged(bool hideUserCount);
    void _qslotClickedCategory();
    void _qslotClickedAddTagButton();
    void _qslotCategoryChanged(const std::string& categoryNum, const std::string& categoryName);
    void _qslotStreamTagChanged(const std::vector<std::string>& tags);
    void _qslotClickedAdultOnlyButton(bool checked);
    void _qslotClickedUsePasswordButton();
    void _qslotClickedRejectVisitButton(bool checked);
    void _qslotUsePasswordChanged(bool usePassword, const QString& password);
    void _qslotClickedSubscribeBroad(bool checked);

    void _qslotClickedUserCountButton(bool checked);
    void _qslotClickedUPButton(bool checked);
    void _qslotRefreshUI(int result, QString message);
    void _qslotSendBroadInfoReceived(int result, QString message);

    void _qslotCheckSubscribeBroadDockonStart();
    void _qslotBroadInfoAPIErrorReceived(int code, QString message);

    void _qslotRefreshUp();
    void _qslotRefreshUpReceived(int result, QString message);
    void _qslotRefreshViewer();
    void _qslotRefreshViewerRecveived(int result, QString message);
    void _qslotResetViewer();

public:
    void LoadBroadInfoUI();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void _Init();
    void _ConnectEvents();
    void _InitToolTip();

    void _CheckLayout();
    void _SwitchLayout(LayoutDirection dir);
    void _VerticalBroadOptionButton();
    void _HorizontalBroadOptionButton();
    void _StartEditingBroadTitle();
    void _RefreshCategoryName();

    bool _RefreshTags();
    bool _AddTags(std::vector<std::string> tags);
    bool _CheckTitleBanWord(const QString& title);

    void _SetUserCount(bool bHideUserCount = false);
    void _SetUpCount();

    bool _CheckGLFailMessage(const QString& input, QString& rawText);
    bool IsBreaktimeRestrictedCategory(const std::string& categoryNum);

private:
    Ui::AFBroadInfoDockWidget* ui = nullptr;

    LayoutDirection m_layoutDir = LayoutDirection::Horizontal;
    QPointer<QTimer> m_finishEditingTimer;

    QString m_existingTitle;

    QTimer* m_pViewerTimer = nullptr;
    QTimer* m_pUpTimer = nullptr;

    QString m_lastErrorMsg;

    bool m_hideUserCount = false;
    bool m_titleMultiLine = false;
    //QPointer<QVBoxLayout> m_broadVLayout;
    //QPointer<QHBoxLayout> m_broadHLayout;
};

#endif // CBROADINFODOCKWIDGET_H
