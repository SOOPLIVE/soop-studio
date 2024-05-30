#pragma once

#include <QMenu>
#include <QHBoxLayout>
#include <QLabel>

class CustomActionWidget : public QWidget {
public:
	explicit CustomActionWidget(QAction* action, QWidget* parent = nullptr) : QWidget(parent) {
		QHBoxLayout* layout = new QHBoxLayout(this);
		QLabel* textLabel = new QLabel(action->text());
		QLabel* shortcutLabel = new QLabel(action->shortcut().toString());
		layout->addWidget(textLabel);
		layout->addStretch(); 
		layout->addWidget(shortcutLabel);
	}
};

class AFQCustomMenu : public QMenu
{
    Q_OBJECT
#pragma region class initializer, destructor
public:
	AFQCustomMenu(QWidget* parent = nullptr, bool isSubMenu = false);
	AFQCustomMenu(QString MenuTitle, QWidget* parent = nullptr, bool isSubMenu = false);
	~AFQCustomMenu();
#pragma endregion class initializer, destructor

#pragma region QT Field, CTOR/DTOR
public slots:
	void qslotMoveMenu();

signals:
	void qsignalMenuClosed();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	void show(QPoint pos);
	void AddAction(QAction* action);
#pragma endregion public func

#pragma region protected func
protected:
	virtual void closeEvent(QCloseEvent *event) override;
#pragma endregion protected func

#pragma region private func
private:
	void _InitCustomMenu(bool isSubMenu);
#pragma endregion private func
};
