
#ifndef AFQROLLANIMATIONFRAME_H
#define AFQROLLANIMATIONFRAME_H

#include <QFrame>
#include <QList>

class AFQLoginToggleFrame;

class AFQRollAnimationFrame : public QFrame
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFQRollAnimationFrame(QWidget* parent = nullptr);
    ~AFQRollAnimationFrame() {};

#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void AddFrame(QString platformName, QString iconPath);
#pragma endregion public func

#pragma region protected func
protected:
    void wheelEvent(QWheelEvent* e) override;
#pragma endregion protected func

#pragma region private func
private:
    void _RollAnimationWheelUp();
    void _RollAnimationWheelDown();
#pragma endregion private func

#pragma region private member var
private:
    int m_iFrontFrameCount = 0;

    bool m_bAnimatingLock = false;

    QList<AFQLoginToggleFrame*> m_FramesList = QList<AFQLoginToggleFrame*>();

    const QRect m_TopRect = QRect(15, 0, 128, 28);
    const QRect m_FrontRect = QRect(10, 15, 138, 38);
    const QRect m_BottomRect = QRect(15, 40, 128, 28);
#pragma region private member var
};

#endif // AFQROLLANIMATIONFRAME_H
