#pragma once

#include <QDialog>

#include "ui_scene-select-dialog.h"

#include "UIComponent/CRoundedDialogBase.h"

namespace Ui {
    class AFQSceneSelectDialog;
}

// exist : delete focus QStyledItemDelegate
#include <QStyledItemDelegate>
#include <QPainter>
class NoFocusListTempDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionViewItem optionNoFocus = option;
        optionNoFocus.state &= ~QStyle::State_HasFocus;
        QStyledItemDelegate::paint(painter, optionNoFocus, index);
    }
};

class AFQSceneSelectDialog : public AFQRoundedDialogBase
{
#pragma region QT Field, CTOR/DTOR
    Q_OBJECT

public:
    explicit AFQSceneSelectDialog(QWidget* parent);
    ~AFQSceneSelectDialog();

private slots:
    void qSlotButtonBoxClicked(QAbstractButton* button);
    void qSlotCloseButtonClicked();

#pragma endregion QT Field

#pragma region public member var
public:
    QString m_sourceName;
#pragma endregion public member var

#pragma region private member var
private:
    std::unique_ptr<Ui::AFQSceneSelectDialog> ui;
#pragma endregion private member var

};