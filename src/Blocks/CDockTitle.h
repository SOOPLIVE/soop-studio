#ifndef AFCDOCKTITLE_H
#define AFCDOCKTITLE_H

#include <QFrame>
#include <QMouseEvent>
#include <QAbstractButton>

#include "UIComponent/CCustomMenu.h"

class AFQHoverWidget;
class QPushButton;

namespace Ui {
class AFDockTitle;
}

class AFDockTitle : public QFrame
{
#pragma region QT Field
    Q_OBJECT
        Q_PROPERTY(bool moving READ IsMoving WRITE SetMoving)
        Q_PROPERTY(bool moveandfloat READ IsMoveAndFloat)

public:
    explicit AFDockTitle(QWidget* parent = nullptr);
    ~AFDockTitle();

public slots:
    void qslotShowMenu();
    void qslotShowSceneSourceMenu();
    void qslotHideSceneSourceMenu();
    void qslotToggleDock();
    void qslotMaximumPopupTriggered();
    void qslotMinimumPopupTriggered();
    void qslotCloseButtonTriggered();
    void qslotCloseCustomBroserTriggered();
    void qslotMaximizeCustomBrowserTriggered();
    void qslotMinimizeCustomBrowserTriggered();

    void qslotAddSceneTriggered();
    void qslotAddSourceTriggered();
    void qslotTransitionScenePopup();
    void qslotShowSceneControlDock();

    void qslotChangeMaximizeIcon(bool maximize);

signals:
    void qsignalMaximumPopup(const char*);
    void qsignalMinimumPopup(const char*);
    void qsignalClose(const char*);
    void qsignalCustomBrowserClose(QString);
    void qsignalCustomBrowserMaximize(QString);
    void qsignalCustomBrowserMinimize(QString);
    void qsignalToggleDock(bool, const char*);
    void qsignalAddScene();
    void qsignalAddSource();
    void qsignalTransitionScenePopup();
    void qsignalShowSceneControlDock();
#pragma endregion QT Field

#pragma region public func
public:
    void Initialize(bool onlyPopup, QString text, const char* BlockType);
    void Initialize(QString customName);
    void AddButton(QAbstractButton* button);
    void AddButton(QList<QAbstractButton*> buttons);
    void ChangeLabelText(QString text);
    QString GetLabelText();
    void SetToggleWindowToDockButton(bool checked); //false: dock Button
    void DeleteTreeDotsButton();
    void DeleteSceneCollection();
    const char* GetBlockType() { return m_sBlockType; };

    bool IsMoving() const;
    void SetMoving(bool moving);
    bool IsFloating() const;
    void SetFloating(bool floating);

    bool IsMoveAndFloat() const;
    void ChangeMaximizedIcon(bool isMaximized);

#pragma endregion public func

#pragma region protected func
protected:
#pragma endregion protected func

#pragma region private func
private:
    void _ToggleWindowToDock(bool toggle);
    void _MakeCustomMenu();

#pragma endregion private func

#pragma region private member var
private:
    Ui::AFDockTitle *ui;
    AFQCustomMenu* m_qMenu = nullptr;
    AFQCustomMenu* m_qAddSceneSourceMenu = nullptr;
    
    QAction* m_ToggleDockAction;
    QAction* m_CloseAction;
    QAction* m_TransitionScene;

    QAction*    m_AddSceneAction;
    QAction*    m_AddSourceAction;

    QPoint m_dragPosition;
    int m_normalModeWidth = 0;
    bool m_isMaximized;
    bool m_dragInitiated;

    const char* m_sBlockType;
    bool m_bPopup = true;

    bool m_bMoving;
    bool m_bFloating;
    bool m_bIsMaximized = false;
#pragma endregion private member var
};

#endif // AFCDOCKTITLE_H
