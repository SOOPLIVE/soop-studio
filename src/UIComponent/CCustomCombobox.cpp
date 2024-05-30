#include "CCustomCombobox.h"

#include <QStyle>
#include <QListView>
#include <QWheelEvent>

#define QSS_QCOMBOBOX_LISTVIEW  QString("QAbstractItemView {"           \
                                        "border:none;"   \
                                        "border:1px solid #191919;"   \
                                        "outline: 0px;"                 \
                                        "background: #24272D;"  \
                                        "}" \
                                        "QComboBox QAbstractItemView::item {"   \
                                        "padding-left: 12px;"   \
                                        "height: 40px;" \
                                        "background: rgba(255, 255, 255, 6%);"  \
                                        "color: rgba(255, 255, 255, 60%);"  \
                                        "}" \
                                        "QComboBox QAbstractItemView::item:selected {"  \
                                        "background: rgba(255, 255, 255, 12%);" \
                                        "color: #00E0FF;"   \
                                        "}")


AFQCustomCombobox::AFQCustomCombobox(QWidget *parent)
    : QComboBox{parent}
{
    AFQElidedToolTipDelegate* delegate = new AFQElidedToolTipDelegate(this);
    this->setItemDelegate(delegate);

    // Remove Shadow
    QFrame* frame = this->findChild<QFrame*>();
    if (frame) {
        frame->setWindowFlags(Qt::Popup | Qt::NoDropShadowWindowHint);
    }

    QListView* listView = qobject_cast<QListView*>(view());
    if (listView) {
        listView->setStyleSheet(QSS_QCOMBOBOX_LISTVIEW);
    }

}

void AFQCustomCombobox::wheelEvent(QWheelEvent* event)
{
	event->ignore();
}

void AFQCustomCombobox::showPopup()
{
    QComboBox::showPopup();

    QListView* listView = qobject_cast<QListView*>(view());
    if (listView) {
        listView->setStyleSheet(QSS_QCOMBOBOX_LISTVIEW);
    }
}