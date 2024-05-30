
#ifndef AFQTOGGLEBUTTON_H
#define AFQTOGGLEBUTTON_H

#include <QtWidgets>

//Forward Class
class QEvent;

class AFQToggleButton : public QAbstractButton
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT
    Q_PROPERTY(int offset READ offset WRITE setOffset)
    Q_PROPERTY(QBrush brush READ brush WRITE setBrush)

#pragma endregion QT Field, CTOR/DTOR

#pragma region class initializer, destructor
public:
    AFQToggleButton(QWidget* parent = nullptr);
    ~AFQToggleButton() {};
#pragma endregion class initializer, destructor

#pragma region public func
    QSize sizeHint() const override;

    QBrush brush() const {
        return m_SlideBrush;
    }
    void setBrush(const QBrush& brush) {
        m_SlideBrush = brush;
    }

    int offset() const {
        return m_iButtonX;
    }
    void setOffset(int offset) {
        m_iButtonX = offset;
        update();
    }

    void ChangeState(bool state);
#pragma endregion public func

#pragma region protected func
    void paintEvent(QPaintEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
#pragma endregion protected func

#pragma region private member var
    bool m_bSwitchOn = false;
    bool m_bAnimatingLock = false;
    int m_iButtonX = 0;
    int m_iButtonY = 0;
    int m_iButtonHeight = 10;
    int m_iButtonMargin = 3;
    QBrush m_ButtonBrush = Qt::white;
    QBrush m_SlideBrush = QBrush(QColor(22, 196, 40));
    QBrush m_OffSlideBrush = QBrush(QColor(51, 51, 51));
    QPropertyAnimation* m_SwitchAnimation = new QPropertyAnimation(this, "offset", this);
#pragma endregion private member var
};

#endif // AFQTOGGLEBUTTON_H
