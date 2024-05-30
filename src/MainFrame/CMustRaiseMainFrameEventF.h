#pragma once

#include "Application/CApplication.h"

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
            if (m_bCeckMainRaise == false)
            {
                m_bCeckMainRaise = true;
                m_bNeedMainRaise = true;
            }
        }
        
        if (event->type() == QEvent::Type::Paint)
        {
            if (m_bNeedMainRaise && m_bCeckMainRaise)
            {
                m_bNeedMainRaise = false;
                App()->GetMainView()->raise();
            }
        }
        
        if (event->type() == QEvent::Type::FocusOut)
            m_bCeckMainRaise = false;
        
        return QObject::eventFilter(obj, event);
    }
#pragma endregion protected func

#pragma region private member var
private:
    bool        m_bCeckMainRaise = false;
    bool        m_bNeedMainRaise = false;
#pragma endregion private member var
};
