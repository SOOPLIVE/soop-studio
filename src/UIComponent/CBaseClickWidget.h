#pragma once

#include <QWidget>
#include <QMouseEvent>

class AFQBaseClickWidget: public QWidget
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

#pragma region class initializer, destructor
public:
    explicit AFQBaseClickWidget(QWidget* parent = nullptr);
    ~AFQBaseClickWidget() {};
#pragma endregion class initializer, destructor

signals:
    void                qsignalLeftClicked(bool checked);
    void                qsignalRightClicked(bool checked);
    void                qsignalLeftClick();
    void                qsignalRightClick();
    void                qsignalLeftReleased();
    void                qsignalRightReleased();
    void                qsignalHoverEntered();
    void                qsignalHoverLeaved();
    
#pragma endregion QT Field

#pragma region public func
public:
    void SetCheckable(bool checkable) { m_bCheckable = checkable; };
    bool GetCheckable(){ return m_bCheckable; };

    void SetChecked(bool checked) { m_bChecked = checked; }
    bool GetChecked() { return m_bChecked; }

#pragma endregion public func

#pragma region protected func
protected:
    void                mousePressEvent(QMouseEvent* event) override;
    void                mouseReleaseEvent(QMouseEvent* event) override;
    void                paintEvent(QPaintEvent*) override;
    bool                eventFilter(QObject* obj, QEvent* event) override;
#pragma endregion protected func

#pragma region private member var
private:
    bool                m_bChecked = false;
    bool                m_bCheckable = false;
#pragma endregion private member var

};
