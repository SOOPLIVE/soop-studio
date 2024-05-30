#ifndef CBLOCKPOPUP_H
#define CBLOCKPOPUP_H

#include <QWidget>
#include <QHBoxLayout>
#include <QAbstractButton>
#include <QList>

namespace Ui {
class AFQBlockPopup;
}

class AFQBalloonWidget;
class AFMainDynamicComposit;
class AFBlockButtonWidget;
class AFQMustRaiseMainFrameEventFilter;


class AFQBlockPopup : public QWidget
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

#pragma region class initializer, destructor
public:
    explicit AFQBlockPopup(QWidget* parent = nullptr);
    ~AFQBlockPopup();
#pragma endregion class initializer, destructor

public slots:
    void qslotShowTooltip(int key);
    void qslotCloseTooltip();
    void qslotBlockButtonClicked();

signals:
    void qsignalHidePopup();
    void qsignalBlockButtonTriggered(bool, int);
    void qsignalShowSetting(bool);
    void qsignalToggleLockPopup();
    void qsignalMouseEntered();
    void qsignalLockTriggered();
#pragma endregion QT Field



#pragma region public func
public:
    bool BlockWindowInit(bool locked, AFMainDynamicComposit* mainWindowBasic = nullptr);
    void BlockButtonToggled(bool show, int blockType);


#pragma endregion public func

#pragma region protected func
protected:
    bool event(QEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
#pragma endregion protected func

#pragma region private func
private:
    void _SetBlockButtons();
    void _SetBlockButton(QString buttonType);
#pragma endregion private func


#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
    Ui::AFQBlockPopup* ui;
    AFQBalloonWidget* m_BalloonToolTip = nullptr;
    QSize m_ButtonSize = QSize(50, 50);
    int m_iButtonCount = 0;
    QList<AFBlockButtonWidget*> m_ButtonWidgetList;
    
    bool m_bInstalledEventFilter = false;
    AFQMustRaiseMainFrameEventFilter* m_pMainRaiseEventFilter = nullptr;
#pragma endregion private member var
};

#endif // CBLOCKPOPUP_H
