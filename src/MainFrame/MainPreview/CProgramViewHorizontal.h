#pragma once

#include <QWidget>
#include <QString>
#include <QPointer>
#include "UIComponent/CCustomMenu.h"

namespace Ui {
    class AFQProgramView;
}

class AFQProgramView: public QWidget
{
    Q_OBJECT

public:
    explicit AFQProgramView(QWidget* parent = nullptr);
    ~AFQProgramView();

public slots:
    void qslotChangeLayoutStrech();
    void qslotToggleSwapScenesMode();
    void qslotToggleEditProperties();
    void qslotToggleSceneDuplication();
    void qslotTransitionTriggered();
    
private slots:
    void _qslotTransitionClicked();
    void _qslotMenuTransitionClicked();

public:
    bool Initialize();
         
    void InsertDisplays(QWidget* mainDisplay, QWidget* programDisplay);
         
    void ChangeEditSceneName(QString name);
    void ChangeLiveSceneName(QString name);
         
    void ToggleSceneLabel(bool visible);

private:
    void _SetButtons();
    void _InitValueLabels();
    void _InitTransitionMenu();

private:
    QPointer<AFQCustomMenu> m_transitionMenu = nullptr;

    QAction* m_pDuplicateScene = nullptr;
    QAction* m_pEditProperties = nullptr;
    QAction* m_pSwapScenesAction = nullptr;
    Ui::AFQProgramView* ui = nullptr;
};