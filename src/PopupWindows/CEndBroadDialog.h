#ifndef CENDBROADDIALOG_H
#define CENDBROADDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QPointer>
#include <QMovie>

#include "UIComponent/CTopBaseWindow.h"

namespace Ui {
class AFQEndBroadDialog;
}

class AFQEndBroadDialog : public AFTTopBaseDialog
{
#pragma region QT Field
    Q_OBJECT

public:
    explicit AFQEndBroadDialog(QWidget* parent = nullptr, bool studioEnd = false);
    ~AFQEndBroadDialog();

private slots:
    void _qslotEndBroadButtonClicked();

public slots:
    void qslotRefreshWaitTime(int t1, QString t2);
    void qslotWaitTimeTimerTimeout();
    void qslotBroadCloseAPIResponsed(const QByteArray& responseData);
#pragma endregion QT Field

#pragma region public func
public:
    void EndBroadInfoInit(bool ReplayAvailable);
    void ResetBroadEndUI();
#pragma endregion public func

#pragma region protected func
protected:
    void showEvent(QShowEvent* event) override;
#pragma endregion protected func

#pragma region private var
private:
    Ui::AFQEndBroadDialog* ui;

    bool m_replayAvailable = false;
    bool m_isStudioEnd = false;
    int  m_broadWaitTime = -1;

    QPointer<QTimer> m_waitTimeTimer;
    QPointer<QMovie> m_wheelMovie;

#pragma endregion private var
};

#endif // CENDBROADDIALOG_H
