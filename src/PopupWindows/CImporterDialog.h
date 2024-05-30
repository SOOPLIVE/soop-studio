#pragma once

#include <obs.hpp>

#include <QDialog>
#include <QPointer>
#include <QStyledItemDelegate>
#include <QFileInfo>
#include "ui_importer-dialog.h"

#include "UIComponent/CRoundedDialogBase.h"

//class ImporterModel;
class QCheckBox;
class QLineEdit;
class QPushButton;

class AFQImporterRowWidget : public QWidget {
    Q_OBJECT

public:
    explicit AFQImporterRowWidget(QWidget* parent = nullptr) {};

public slots:
    void qslotEditPathFinished();

signals:
    void qsignalNeedNewRow();
    void qsignalDeleteRow();
    void qsignalBrowseImport();

public:
    void AddNewRow();
    void AddInfoRow(QString name, QString path, QString program);

    void SetCheckBoxChecked(bool checked);

    bool GetChecked();
    std::string GetName();
    std::string GetPath();
    std::string GetProgram();

private:
    QCheckBox* m_qCheckBox = nullptr;
    QLineEdit* m_qNameEdit = nullptr;
    QLineEdit* m_qPathEdit = nullptr;
    QLineEdit* m_qProgramEdit = nullptr;

    QPushButton* m_qDeleteButton = nullptr;

    bool m_bInit = false;
};

class AFQImporterDialog : public AFQRoundedDialogBase {
    Q_OBJECT

    //QPointer<ImporterModel> optionsModel;
    std::unique_ptr<Ui::AFQImporterDialog> ui;

public:
    explicit AFQImporterDialog(QWidget *parent = nullptr);

    void AddImportOption(QString path, bool automatic, AFQImporterRowWidget* addRow);

protected:
    virtual void dropEvent(QDropEvent *ev) override;
    virtual void dragEnterEvent(QDragEnterEvent *ev) override;

public slots:
    void importCollections();
    //void dataChanged();
    void qslotMakeNewRow();
    void qslotDeleteRow();
    void qslotBrowseSingleImport();
    void qslotBrowseImport();


private:
    struct ImporterEntryInfo {
        QString path;
        QString program;
        QString name;

        bool selected;
    };

    QList<AFQImporterRowWidget*> m_qImportEntryWidget;

    AFQImporterRowWidget* m_qNewRow = nullptr;
};
//
//class ImporterModel : public QAbstractTableModel {
//    Q_OBJECT
//
//    friend class AFQImporterDialog;
//
//public:
//    ImporterModel(QObject *parent = 0) : QAbstractTableModel(parent) {}
//
//    int rowCount(const QModelIndex &parent = QModelIndex()) const;
//    int columnCount(const QModelIndex &parent = QModelIndex()) const;
//    QVariant data(const QModelIndex &index, int role) const;
//    QVariant headerData(int section, Qt::Orientation orientation,
//                int role = Qt::DisplayRole) const;
//    Qt::ItemFlags flags(const QModelIndex &index) const;
//    bool setData(const QModelIndex &index, const QVariant &value, int role);
//
//private:
//    struct ImporterEntry {
//        QString path;
//        QString program;
//        QString name;
//
//        bool selected;
//        bool empty;
//    };
//
//    QList<ImporterEntry> options;
//
//    void checkInputPath(int row);
//};

//class ImporterEntryPathItemDelegate : public QStyledItemDelegate {
//    Q_OBJECT
//
//public:
//    ImporterEntryPathItemDelegate();
//
//    virtual QWidget *createEditor(QWidget *parent,
//                      const QStyleOptionViewItem & /* option */,
//                      const QModelIndex &index) const override;
//
//    virtual void setEditorData(QWidget *editor,
//                   const QModelIndex &index) const override;
//    virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
//                  const QModelIndex &index) const override;
//    virtual void paint(QPainter *painter,
//               const QStyleOptionViewItem &option,
//               const QModelIndex &index) const override;
//
//private:
//    const char *PATH_LIST_PROP = "pathList";
//
//    void handleBrowse(QWidget *container);
//    void handleClear(QWidget *container);
//
//private slots:
//    void updateText();
//};