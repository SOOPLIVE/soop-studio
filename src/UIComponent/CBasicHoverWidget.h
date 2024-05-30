#pragma once

#include <QWidget>
#include <QEvent>

class AFQHoverWidget : public QWidget
{
#pragma region QT Field, CTOR/DTOR
	Q_OBJECT

#pragma region class initializer, destructor
public:
	explicit AFQHoverWidget(QWidget* parent = nullptr);
	~AFQHoverWidget() {};
#pragma endregion class initializer, destructor

signals:
	void qsignalHoverEnter();
	void qsignalHoverLeave();
	void qsignalMouseClick();
#pragma endregion QT Field


#pragma region public func
public:

#pragma endregion public func

#pragma region protected func
protected:
	bool event(QEvent* e) override;
	void paintEvent(QPaintEvent*) override;
#pragma endregion protected func
};

