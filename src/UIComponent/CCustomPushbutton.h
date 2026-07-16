#ifndef AFQCUSTOMPUSHBUTTON_H
#define AFQCUSTOMPUSHBUTTON_H

#include <QPushButton>
#include <QTimer>

class AFQCustomPushbutton : public QPushButton
{
    Q_OBJECT

public:
    explicit AFQCustomPushbutton(QWidget* parent = nullptr);

signals:
    void qsignalButtonEnter();
    void qsignalButtonLeave();
    void qsignalMouseMove();
    void qsignalMouseStop();
	void qsignalButtonDoubleClicked();
    void qsignalMousePressed();
    void qsignalMouseReleased();

public:
    void SetButtonKeyValue(int keyValue) { m_keyValue = keyValue; };
    int ButtonKeyValue() { return m_keyValue; };

protected:
    bool event(QEvent* event) override;

private:
    bool m_hoverDelay = false;
    int m_keyValue = 0;
};


class AFQHoverOnlyPushButton : public QPushButton
{
    Q_OBJECT

public:
    explicit AFQHoverOnlyPushButton(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* obj, QEvent* event);
};
#endif // AFQCUSTOMPUSHBUTTON_H
