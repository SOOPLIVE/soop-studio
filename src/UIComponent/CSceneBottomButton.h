#pragma once

#include <QLabel>
#include <QAction>
#include <QPointer>

#include "obs.hpp"

#include "Utils/AFScreenshotObj.h"

#include "UIComponent/CElidedSlideLabel.h"

class AFQSceneAction : public QAction
{
#pragma region QT Field
    Q_OBJECT

public:
    AFQSceneAction(QString name, QObject* parent, OBSScene scene) : QAction(name, parent), m_obsScene(scene) {
        connect(this, &QAction::triggered, this, &AFQSceneAction::onTriggered);
    }
    ~AFQSceneAction() { }

signals:
    void qsignalSceneButtonClicked(OBSScene scene);
    void qsignalSceneButtonDoubleClicked(OBSScene scene);

private slots:
    void onTriggered() {
        qsignalSceneButtonClicked(m_obsScene);
    }
#pragma endregion QT Field


#pragma region private func
private:
    // OBS var
    OBSScene m_obsScene;

#pragma endregion private func

};

class AFQSceneBottomButton : public QFrame
{
#pragma region QT Field
    Q_OBJECT

public:
    explicit AFQSceneBottomButton(QWidget* parent, OBSScene scene, int index, QString name);
    ~AFQSceneBottomButton();
 
public:
    void     SetSelectedState(bool selected);
    OBSScene GetObsScene() { return m_obsScene; }

private:
    void     _ShowPreveiw(bool on);

signals:
    void qsignalSceneButtonClicked(OBSScene scene);
    void qsignalSceneButtonDoubleClicked(OBSScene scene);

    void qsignalHoverButton(QString id);
    void qsignalLeaveButton();

private slots:
    void qslotTimerScreenShot();
    void qslotTimerHoverPreview();
    void qslotSetScreenShotPreview();

#pragma endregion QT Field




#pragma region protected func
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

#pragma endregion protected func



#pragma region private member var
private:
    // Qt widget
    QLabel* m_labelSceneIndex = nullptr;
    AFQElidedSlideLabel* m_labelSceneName = nullptr;
    QFrame* m_frameMiddleLine = nullptr;

    QLabel* m_pScreenshotScene = nullptr;
    QTimer* m_timerScreenShot = nullptr;
    QTimer* m_timerHoverPreview = nullptr;
    QPointer<AFQScreenShotObj>	m_pScreenshotObj;

    // OBS var
    OBSScene m_obsScene;
    bool     m_bSelected = false;
    int      m_labelNameWidth = 0;

#pragma endregion private member var

};