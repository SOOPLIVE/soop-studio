#pragma once

#include <QDockWidget>

class AFQBaseDockWidget : public QDockWidget
{
#pragma region QT Field, CTOR/DTOR
	Q_OBJECT

signals:
	void closed();

public slots:
	void qslotTopLevelChanged(bool toplevel);

private slots:
#pragma endregion QT Field, CTOR/DTOR


#pragma region class initializer, destructor
public:
	explicit AFQBaseDockWidget(QWidget* parent = nullptr);
#pragma endregion class initializer, destructor


#pragma region public func
public:
	void SetStyle();
#pragma endregion public func

#pragma region public func
protected:
	bool event(QEvent* e) override;
	void closeEvent(QCloseEvent* event) override;
#pragma endregion public func


#pragma region private func
private:
	bool _IsOverTitleArea(QPoint point);
#pragma endregion private func


#pragma region public member var
public:
#pragma endregion public member var

#pragma region private member var
private:
	bool m_bIsFloating = false;
	bool m_bIsPressed = false;
#pragma endregion private member var
};
