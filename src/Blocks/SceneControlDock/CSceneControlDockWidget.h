#pragma once

#include <QEvent>
#include <QWidget>
#include <QPointer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "Blocks/CBaseDockWidget.h"


#define SCENE_CONTROL_MIN_SIZE_WIDTH 720
#define SCENE_CONTROL_MIN_SIZE_HEIGTH 486


class AFSimpleHoverEventFilter : public QObject
{
#pragma region QT Field
    Q_OBJECT
signals:
    void qsignalHoverEventOccurred(QEvent::Type type);
#pragma endregion QT Field
    
#pragma region protected function
protected:
    bool eventFilter(QObject *watched, QEvent *event) override 
    {
        if (event->type() == QEvent::HoverEnter ||
            event->type() == QEvent::HoverLeave ||
            event->type() == QEvent::HoverMove)
        {
            emit qsignalHoverEventOccurred(event->type());
            return true;
        }
        return QObject::eventFilter(watched, event);
    };
#pragma endregion protected function
};

class AFSceneControlDockWidget final : public AFQBaseDockWidget
{
#pragma region QT Field
	Q_OBJECT
    
#pragma region class initializer, destructor
public:
    explicit AFSceneControlDockWidget(QWidget* parent = nullptr);
    ~AFSceneControlDockWidget();
#pragma endregion class initializer, destructor

signals:
    void        qSignalTransitionButtonClicked();

private slots:
    void        qslotChangedTopLevel(bool value);
    void        qslotChangedVisibility(bool value);
    void        qslotTransitionUIStyle(QEvent::Type type);

    
#pragma endregion QT Field
    
    
#pragma region public function
public:
    void        ChangeLayoutStudioMode(bool studioMode);
#pragma endregion QT public function
    
    
#pragma region public function
private:
    void        _InitLayout();
    void        _ReleaseLayoutObj();
#pragma endregion QT public function
    
    
#pragma region private member
private:
    bool                                m_bNeverDocked = true;
    
    QWidget*                            m_pTransitionAreaContents = nullptr;
    QHBoxLayout*                        m_pTransitionAreaLayout = nullptr;
    QPushButton*                        m_pTransitionButton = nullptr;
    QLabel*                             m_pTransitionImg = nullptr;
    
    QWidget*                            m_pExpandedAreaContents = nullptr;
    QHBoxLayout*                        m_pExpandedAreaLayout = nullptr;

    
    
    QWidget                             m_Contents;
    QVBoxLayout                         m_MainLayout;
    QPointer<QWidget>                   m_MultiView = nullptr;
#pragma endregion QT private member
};
