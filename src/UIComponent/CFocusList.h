#pragma once

#include <QListWidget>
#include <QStyledItemDelegate>
#include <QPainter>

class NoFocusListDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionViewItem optionNoFocus = option;
        optionNoFocus.state &= ~QStyle::State_HasFocus;
        QStyledItemDelegate::paint(painter, optionNoFocus, index);
    }
};

class AFQFocusList : public QListWidget {
	Q_OBJECT

public:
	AFQFocusList(QWidget* parent);

protected:
	void focusInEvent(QFocusEvent* event) override;
	virtual void dragMoveEvent(QDragMoveEvent* event) override;

signals:
	void GotFocus();
};
