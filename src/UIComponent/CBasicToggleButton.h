
#ifndef AFQTOGGLEBUTTON_H
#define AFQTOGGLEBUTTON_H

#include <QtWidgets>

//Forward Class
class QEvent;

class AFQToggleButton : public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(int offset READ offset WRITE setOffset)
    Q_PROPERTY(QBrush brush READ brush WRITE setBrush)

public:
    AFQToggleButton(QWidget* parent = nullptr);
    ~AFQToggleButton() {};

public:
    //QSize sizeHint() const override;

    QBrush brush() const {
        return m_slideBrush;
    }
    void setBrush(const QBrush& brush) {
        m_slideBrush = brush;
    }

    int offset() const {
        return m_buttonX;
    }
    void setOffset(int offset) {
        m_buttonX = offset;
        update();
    }

    void SetChecked(bool checked);
    void ChangeState(bool state);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void _Init();

private:
    bool m_switchOn = false;
    bool m_animatingLock = false;
    int m_buttonX = 0;
    //int m_buttonY = 0;
    int m_buttonHeight = 10;
    //int m_buttonMargin = 3;

    QBrush m_buttonBrush = Qt::white;
    QBrush m_slideBrush = QBrush(QColor(22, 196, 40)); // Checked
    QBrush m_offSlideBrush = QBrush(QColor(51, 51, 51)); // Unchecked

    QBrush m_disabledButtonBrush = QBrush(QColor(80, 82, 87));
    QBrush m_disabledSlideBrush = QBrush(QColor(29, 57, 87)); // Disabled-Checked
    QBrush m_disabledOffSlideBrush = QBrush(QColor(45, 48, 53)); // Disabled-Unchecked

    QPropertyAnimation* m_pSwitchAnimation = new QPropertyAnimation(this, "offset", this);
};

#endif // AFQTOGGLEBUTTON_H