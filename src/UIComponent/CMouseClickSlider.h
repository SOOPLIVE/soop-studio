#ifndef CMOUSECLICKSLIDER_H
#define CMOUSECLICKSLIDER_H

#include <QSlider>
#include <QMouseEvent>

class AFQMouseClickSlider : public QSlider
{
#pragma region QT Field, CTOR/DTOR
	Q_OBJECT

public:
	AFQMouseClickSlider(QWidget* parent = 0) : QSlider(parent) {};
	~AFQMouseClickSlider() = default;
#pragma endregion QT Field, CTOR/DTOR

#pragma region protected func
protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
#pragma endregion protected func

#pragma region private func, var
private:
	void _SetValueToMousePos(QMouseEvent* event);
#pragma endregion private func, var

	Qt::MouseButton m_mouseButton = Qt::MouseButton::NoButton;
};

#endif // CMOUSECLICKSLIDER_H