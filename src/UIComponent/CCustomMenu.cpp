#include "CCustomMenu.h"

#include <QTimer>

AFQCustomMenu::AFQCustomMenu(QWidget* parent, bool isSubMenu) : QMenu(parent)
{
    _InitCustomMenu(isSubMenu);
}

AFQCustomMenu::AFQCustomMenu(QString MenuTitle, QWidget* parent, bool isSubMenu)
    :QMenu(parent)
{
    _InitCustomMenu(isSubMenu);
    setTitle(MenuTitle);
}

AFQCustomMenu::~AFQCustomMenu()
{

}

void AFQCustomMenu::qslotMoveMenu()
{
    QTimer::singleShot(0, [this]() {
        move(pos() + QPoint(9, 0));
    });
}

void AFQCustomMenu::show(QPoint pos)
{
    move(pos);
    QWidget::show();
    QWidget::setFocus();
}

void AFQCustomMenu::AddAction(QAction* action)
{
    CustomActionWidget* customWidget = new CustomActionWidget(action, this);
    addAction(action);
}

void AFQCustomMenu::closeEvent(QCloseEvent* event)
{
    emit qsignalMenuClosed();
}

void AFQCustomMenu::_InitCustomMenu(bool isSubMenu)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint | windowFlags() | Qt::NoDropShadowWindowHint);

    if (isSubMenu)
        connect(this, &AFQCustomMenu::aboutToShow, this, &AFQCustomMenu::qslotMoveMenu);
}
