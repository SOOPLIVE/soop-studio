#pragma once

#include "UIComponent/CBaseClickWidget.h"


class AFTransitionWidgetStudioMode: public AFQBaseClickWidget
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

#pragma region class initializer, destructor
public:
    explicit AFTransitionWidgetStudioMode(QWidget* parent = nullptr);
    ~AFTransitionWidgetStudioMode() {};
#pragma endregion class initializer, destructor
    
#pragma endregion QT Field


#pragma region protected func
protected:
    void                paintEvent(QPaintEvent* event) override;
#pragma endregion protected func

    
#pragma region private member var
private:
    QColor              m_LineColor = QColor(0, 224, 255);
#pragma endregion private member var
};
