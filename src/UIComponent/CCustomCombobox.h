#pragma once

#include <QComboBox>
#include <QToolTip>
#include <QHelpEvent>
#include <QModelIndex>
#include <QAbstractItemView>
#include <QStyledItemDelegate>

//    From Black.qss
//    /* Common QListWidget */
//    QListWidget::item {  padding: 6px; ... }
#define QLISTWIDGET_PADDING_VALUE   6

class AFQCustomCombobox : public QComboBox
{
#pragma region QT Field
    Q_OBJECT
public:
    explicit AFQCustomCombobox(QWidget* parent = nullptr);

#pragma endregion QT Field

#pragma region public func
public:
#pragma endregion public func

#pragma region protected func

protected:
    virtual void wheelEvent(QWheelEvent* event) override;
    virtual void showPopup() override;
#pragma endregion protected func

#pragma region private func
private:
#pragma endregion private func

#pragma region private member var
#pragma endregion private member var
};


class AFQElidedToolTipDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    bool helpEvent(QHelpEvent* event, QAbstractItemView* view,
        const QStyleOptionViewItem& option, const QModelIndex& index) override {

        if (event->type() == QEvent::ToolTip && index.isValid()) {
            int width = (view->viewport()->width() - (QLISTWIDGET_PADDING_VALUE * 2));
            QString text = index.data(Qt::DisplayRole).toString();
            QFontMetrics fontMetrics(option.font);
            QString elidedText = fontMetrics.elidedText(text, Qt::ElideRight, width);

            if (elidedText != text) {
                QToolTip::showText(event->globalPos(), text, view->viewport());
                return true;
            }
            else {
                QToolTip::hideText();
                event->ignore();
            }
        }
        return false;
    }
};