#ifndef CMAINFRAMEGUIDE_H
#define CMAINFRAMEGUIDE_H

#include "UIComponent/CBasicHoverWidget.h"

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

#pragma region protected func
protected:
    bool event(QEvent* e) override;
#pragma endregion protected func
    
public:
    void SceneSourceGuide(QRect position);
    void LoginGuide(QRect position);
    void BroadGuide(QRect position);

private:
    Ui::AFMainFrameGuide *ui;
    
    bool m_bInstalledEventFilter = false;
    AFQMustRaiseMainFrameEventFilter* m_pMainRaiseEventFilter = nullptr;
};

#endif // CMAINFRAMEGUIDE_H
