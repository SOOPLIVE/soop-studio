#ifndef AFQCUSTOMPUSHBUTTON_H
#define AFQCUSTOMPUSHBUTTON_H

#include <QPushButton>
#include <QTimer>
#include <QGraphicsOpacityEffect>

class AFQCustomPushbutton : public QPushButton
{
#pragma region QT Field
    Q_OBJECT
public:
    explicit AFQCustomPushbutton(QWidget* parent = nullptr);

signals:
    void qsignalButtonEnter();
    void qsignalButtonLeave();
    void qsignalMouseMove();
    void qsignalMouseStop();
	void qsignalButtonDoubleClicked();

public slots:
    void qslotDelayTimeout();

#pragma endregion QT Field

#pragma region public func
public:
    void SetOpacityValue();
    void SetButtonKeyValue(int keyValue) { m_iKeyValue = keyValue; };
    int ButtonKeyValue() { return m_iKeyValue; };
#pragma endregion public func

#pragma region protected func

protected:
    bool event(QEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
    void _ChangeOpacity();
#pragma endregion private func

#pragma region private member var
private:
    bool m_bHoverDelay = false;
    QTimer* m_iDelayTime = nullptr;
    int m_iKeyValue = 0;
    
    QGraphicsOpacityEffect* m_qOpacityEffect = nullptr;
#pragma endregion private member var
};

#endif // AFQCUSTOMPUSHBUTTON_H
