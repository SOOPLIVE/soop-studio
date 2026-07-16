#ifndef CMAINFRAMEGUIDE_H
#define CMAINFRAMEGUIDE_H

#include "UIComponent/CBasicHoverWidget.h"
#include <QSvgWidget>

namespace Ui {
class AFMainFrameGuide;
}

class AFQMustRaiseMainFrameEventFilter;

class AFMainFrameGuide : public AFQHoverWidget
{
    Q_OBJECT

public:
    explicit AFMainFrameGuide(QWidget* parent = nullptr);
    ~AFMainFrameGuide();

public slots:
    void qslotLoginTriggered();
    void qslotSceneSourceTriggered();
    void qslotBroadTriggered();

signals:
    void qsignalLoginTrigger();
    void qsignalSceneSourceTriggered(bool show, int key);
    void qsignalBroadTriggered();
    void qsignalMissionCleared(int currentMission);
    void qsignalCloseGuide();

#pragma region protected func
protected:
    bool event(QEvent* e) override;
    virtual void resizeEvent(QResizeEvent* event) override;

#pragma endregion protected func
    
public:
    void SceneSourceGuide(QRect position);
    void LoginGuide(QRect position);
    void BroadGuide(QRect position);
    void TutorialInit(QRect geo);
    void TutorialPosition(QRect gnb, QRect channel, QRect broad, QRect button);

private:
    Ui::AFMainFrameGuide *ui;
    
    QSvgWidget* m_pGnbGuide;
    QSvgWidget* m_pChannelGuide;
    QSvgWidget* m_pBroadGuide;
    QSvgWidget* m_pButtonGuide;

    bool m_installedEventFilter = false;
    AFQMustRaiseMainFrameEventFilter* m_pMainRaiseEventFilter = nullptr;
};

#endif // CMAINFRAMEGUIDE_H
