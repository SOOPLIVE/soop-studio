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
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

#pragma region class initializer, destructor
public:
    explicit AFQProgramView(QWidget* parent = nullptr);
    ~AFQProgramView();
#pragma endregion class initializer, destructor

public slots:
    void            qslotChangeLayoutStrech();
    void            qslotToggleSwapScenesMode();
    void            qslotToggleEditProperties();
    void            qslotToggleSceneDuplication();
    void            qslotTransitionTriggered();
    
private slots:
    void            _qslotTransitionClicked();
    void            _qslotMenuTransitionClicked();

    void            _qslotSetTransitionWidgetStyleDefault();
    void            _qslotSetTransitionWidgetStyleHovered();
    void            _qslotSetTransitionButtonStyleDefault();
    void            _qslotSetTransitionButtonStyleHovered();
    
#pragma endregion QT Field


#pragma region public func
public:
    bool            Initialize();
    
    void            InsertDisplays(QWidget* mainDisplay, QWidget* programDisplay);
    
    void            ChangeEditSceneName(QString name);
    void            ChangeLiveSceneName(QString name);

    void            ToggleSceneLabel(bool visible);
#pragma endregion public func


#pragma region private func
private:
    void            _SetButtons();
    void            _InitValueLabels();
    void            _InitTransitionMenu();
#pragma endregion private func


#pragma region private member var
private:
    QPointer<AFQCustomMenu>         m_qTransitionMenu = nullptr;

    QAction*                        m_qDuplicateScene;
    QAction*                        m_qEditProperties;
    QAction*                        m_qSwapScenesAction;
    Ui::AFQProgramView*             ui;
#pragma endregion private member var
};
