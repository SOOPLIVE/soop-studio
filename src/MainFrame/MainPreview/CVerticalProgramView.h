#ifndef CVERTICALPROGRAMVIEW_H
#define CVERTICALPROGRAMVIEW_H

#include <QWidget>
#include <QString>
#include <QPointer>

#include "UIComponent/CCustomMenu.h"

namespace Ui {
class AFQVerticalProgramView;
}

class AFQVerticalProgramView : public QWidget
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

#pragma region class initializer, destructor
public:
    explicit AFQVerticalProgramView(QWidget* parent = nullptr);
    ~AFQVerticalProgramView();
#pragma endregion class initializer, destructor

public slots:
    //void            qslotChangeLayoutStrech();
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
//
//
#pragma endregion QT Field
//
//
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
    Ui::AFQVerticalProgramView* ui;
#pragma endregion private member var
};

#endif // CVERTICALPROGRAMVIEW_H
