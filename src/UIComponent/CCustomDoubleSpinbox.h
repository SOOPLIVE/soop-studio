#pragma once

#include <QDoubleSpinBox>

class AFQCustomDoubleSpinbox : public QDoubleSpinBox
{
#pragma region QT Field
    Q_OBJECT
public:
    explicit AFQCustomDoubleSpinbox(QWidget* parent = nullptr);

#pragma endregion QT Field

#pragma region public func
public:
#pragma endregion public func

#pragma region protected func

protected:
    virtual void wheelEvent(QWheelEvent* event) override;
#pragma endregion protected func

#pragma region private func
private:
#pragma endregion private func

#pragma region private member var
#pragma endregion private member var
};

