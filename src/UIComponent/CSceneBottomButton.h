#pragma once

#include <QLabel>
#include <QAction>
#include <QPointer>

#include "obs.hpp"

#include "UIComponent/CElidedSlideLabel.h"

#include "Blocks/SceneSourceDock/CSceneListItem.h"

class AFQSceneAction : public QAction
{
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

private:
    // OBS var
    OBSScene m_obsScene;
};

class AFQSceneBottomButton : public QFrame
{
    Q_OBJECT

public:
    explicit AFQSceneBottomButton(QWidget* parent, OBSScene scene, int index, QString name);
    ~AFQSceneBottomButton();
 
public:
    void SetSelectedState(bool selected);
    OBSScene GetObsScene() { return m_obsScene; }

private:
    void _ShowPreveiw(bool on);

signals:
    void qsignalSceneButtonClicked(OBSScene scene);
    void qsignalSceneButtonDoubleClicked(OBSScene scene);

    void qsignalHoverButton(QString id);
    void qsignalLeaveButton();

private slots:
    void qslotTimerHoverPreview();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    // Qt widget
    QLabel* m_plabelSceneIndex = nullptr;
    AFQElidedSlideLabel* m_pLabelSceneName = nullptr;
    QFrame* m_pFrameMiddleLine = nullptr;

    QTimer* m_pTimerHoverPreview = nullptr;

    QPointer<AFQSceneListPreview> m_sceneListPreviewWidget = nullptr;

    // OBS var
    OBSScene m_obsScene;
    bool m_selected = false;
    int m_labelNameWidth = 0;
};