#pragma once

#include <QLabel>

//Forward Class

class AFQRoundedTextLabel : public QLabel
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT
    Q_PROPERTY(QBrush brush READ GetBrush WRITE SetBrush)

#pragma endregion QT Field, CTOR/DTOR

#pragma region class initializer, destructor
public:
    explicit AFQRoundedTextLabel(QWidget* parent = nullptr);
    ~AFQRoundedTextLabel() {};
#pragma endregion class initializer, destructor

#pragma region public func
    void                SetContentMargins(int l, int t, int r, int b);
    
    QBrush              GetBrush() const { return m_LineBrush; };
    void                SetBrush(const QBrush& brush) { m_LineBrush = brush; };
#pragma endregion public func

#pragma region protected func
    void                paintEvent(QPaintEvent* event) override;
#pragma endregion protected func

#pragma region private member var
    QBrush              m_LineBrush = QBrush(QColor(255, 255, 255, 51));
#pragma endregion private member var
};

