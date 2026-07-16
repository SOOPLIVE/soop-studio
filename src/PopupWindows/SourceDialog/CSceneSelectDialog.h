#pragma once

#include <QDialog>
#include <QStyledItemDelegate>
#include <QPainter>

#include "ui_scene-select-dialog.h"

#include "UIComponent/CTopBaseWindow.h"

namespace Ui {
    class AFQSceneSelectDialog;
}

class NoFocusListTempDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionViewItem optionNoFocus = option;
        optionNoFocus.state &= ~QStyle::State_HasFocus;
        QStyledItemDelegate::paint(painter, optionNoFocus, index);
    }
};

class AFQSceneSelectDialog : public AFTTopBaseDialog
{
    Q_OBJECT

public:
    explicit AFQSceneSelectDialog(QWidget* parent);
    ~AFQSceneSelectDialog();

private slots:
    void qSlotButtonBoxClicked(QAbstractButton* button);
    void qSlotCloseButtonClicked();

public:
    QString m_sourceName;

private:
    std::unique_ptr<Ui::AFQSceneSelectDialog> ui;
};