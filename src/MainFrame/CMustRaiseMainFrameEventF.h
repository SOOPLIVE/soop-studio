#pragma once

#include "Application/CApplication.h"

#include "MainFrame/CMainFrame.h"

class AFQMustRaiseMainFrameEventFilter : public QObject
{
#pragma region QT Field, CTOR/DTOR
public:
    AFQMustRaiseMainFrameEventFilter(QObject *parent = nullptr) :
        QObject(parent) {}
#pragma endregion QT Field, CTOR/DTOR

#pragma region protected func
protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::Expose)
        {
            if (m_checkMainRaise == false)
            {
                m_checkMainRaise = true;
                m_needMainRaise = true;
            }
        }
        
        if (event->type() == QEvent::Type::Paint)
        {
            if (m_needMainRaise && m_checkMainRaise)
            {
                m_needMainRaise = false;
                MAINFRAME->raise();
            }
        }
        
        if (event->type() == QEvent::Type::FocusOut)
            m_checkMainRaise = false;
        
        return QObject::eventFilter(obj, event);
    }
#pragma endregion protected func

#pragma region private member var
private:
    bool        m_checkMainRaise = false;
    bool        m_needMainRaise = false;
#pragma endregion private member var
};
