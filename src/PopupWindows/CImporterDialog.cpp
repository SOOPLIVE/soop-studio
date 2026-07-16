#include "CImporterDialog.h"


#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QToolButton>
#include <QMimeData>
#include <QStyledItemDelegate>
#include <QDirIterator>
#include <QDropEvent>

#include "utils/importers/importers.hpp"

#include "qt-wrappers.hpp"

#include "Common/StudioDefine.h"

#include "MainFrame/CMainFrame.h"

#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/OBSData/CLoadSaveManager.h"

#include "UIComponent/CMessageBox.h"



enum ImporterColumn {
    Selected,
    Name,
    Path,
    Program,

    Count
};

enum ImporterEntryRole {
    EntryStateRole = Qt::UserRole,
    NewPath,
    AutoPath,
    CheckEmpty
};

#define IMPORTER_ITEM_HEIGHT 60
#define IMPORTER_TOOL_BTN_SIZE 30
#define IMPORTER_ITEM_MARGIN 10


/**********************************************************
  Delegate - Presents cells in the grid.
**********************************************************/

//ImporterEntryPathItemDelegate::ImporterEntryPathItemDelegate()
//    : QStyledItemDelegate()
//{
//}


//QWidget *ImporterEntryPathItemDelegate::createEditor(
//    QWidget *parent, const QStyleOptionViewItem & /* option */,
//    const QModelIndex &index) const
//{
//    bool empty = index.model()
//                 ->index(index.row(), ImporterColumn::Path)
//                 .data(ImporterEntryRole::CheckEmpty)
//                 .value<bool>();
//
//    QSizePolicy buttonSizePolicy(QSizePolicy::Policy::Minimum,
//                     QSizePolicy::Policy::Expanding,
//                     QSizePolicy::ControlType::PushButton);
//
//    QWidget *container = new QWidget(parent);
//    container->setObjectName("ImporterEditorWidget");
//
//    auto browseCallback = [this, container]() {
//        const_cast<ImporterEntryPathItemDelegate *>(this)->handleBrowse(
//            container);
//    };
//
//    auto clearCallback = [this, container]() {
//        const_cast<ImporterEntryPathItemDelegate *>(this)->handleClear(
//            container);
//    };
//
//    QHBoxLayout *layout = new QHBoxLayout();
//    layout->setContentsMargins(0, 0, 0, 0);
//    layout->setSpacing(0);
//
//    QLineEdit *text = new QLineEdit();
//    text->setFixedHeight(40);
//    text->setObjectName("lineEdit_Text");
//    text->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding,
//                    QSizePolicy::Policy::Expanding,
//                    QSizePolicy::ControlType::LineEdit));
//    layout->addWidget(text);
//
//    QObject::connect(text, &QLineEdit::editingFinished, this,
//             &ImporterEntryPathItemDelegate::updateText);
//
//    QToolButton *browseButton = new QToolButton();
//    browseButton->setObjectName("ImporterBrowseBtn");
//    //browseButton->setText("...");
//    //browseButton->setSizePolicy(buttonSizePolicy);
//    browseButton->setFixedSize(QSize(IMPORTER_TOOL_BTN_SIZE, IMPORTER_TOOL_BTN_SIZE));
//    layout->addWidget(browseButton);
//
//    container->connect(browseButton, &QToolButton::clicked, browseCallback);
//
//    // The "clear" button is not shown in output cells
//    // or the insertion point's input cell.
//    if (!empty) {
//        QToolButton *clearButton = new QToolButton();
//        clearButton->setObjectName("ImporterClearBtn");
//        //clearButton->setText("X");
//        //clearButton->setSizePolicy(buttonSizePolicy);
//        clearButton->setFixedSize(QSize(IMPORTER_TOOL_BTN_SIZE, IMPORTER_TOOL_BTN_SIZE));
//        layout->addWidget(clearButton);
//
//        container->connect(clearButton, &QToolButton::clicked,
//                   clearCallback);
//    }
//
//    container->setLayout(layout);
//    //container->setFocusProxy(text);
//    return container;
//}
//
//void ImporterEntryPathItemDelegate::setEditorData(
//    QWidget *editor, const QModelIndex &index) const
//{
//    QLineEdit *text = editor->findChild<QLineEdit *>();
//    text->setText(index.data().toString());
//    editor->setProperty(PATH_LIST_PROP, QVariant());
//}
//
//void ImporterEntryPathItemDelegate::setModelData(QWidget *editor,
//                         QAbstractItemModel *model,
//                         const QModelIndex &index) const
//{
//    // We use the PATH_LIST_PROP property to pass a list of
//    // path strings from the editor widget into the model's
//    // NewPathsToProcessRole. This is only used when paths
//    // are selected through the "browse" or "delete" buttons
//    // in the editor. If the user enters new text in the
//    // text box, we simply pass that text on to the model
//    // as normal text data in the default role.
//    QVariant pathListProp = editor->property(PATH_LIST_PROP);
//    if (pathListProp.isValid()) {
//        QStringList list =
//            editor->property(PATH_LIST_PROP).toStringList();
//        model->setData(index, list, ImporterEntryRole::NewPath);
//    } else {
//        QLineEdit *lineEdit = editor->findChild<QLineEdit *>();
//        model->setData(index, lineEdit->text());
//    }
//}
//
//void ImporterEntryPathItemDelegate::paint(QPainter *painter,
//                      const QStyleOptionViewItem &option,
//                      const QModelIndex &index) const
//{
//    QStyleOptionViewItem localOption = option;
//    initStyleOption(&localOption, index);
//
//    QApplication::style()->drawControl(QStyle::CE_ItemViewItem,
//                       &localOption, painter);
//}
//
//void ImporterEntryPathItemDelegate::handleBrowse(QWidget *container)
//{
//    QString Pattern = "(*.json *.bpres *.xml *.xconfig)";
//
//    QLineEdit *text = container->findChild<QLineEdit *>();
//
//    QString currentPath = text->text();
//
//    bool isSet = false;
//    QStringList paths = OpenFiles(
//        container, QT_UTF8(Str("Importer.SelectCollection")), currentPath,
//        QT_UTF8(Str("Importer.Collection")) + QString(" ") + Pattern);
//
//    if (!paths.empty()) {
//        container->setProperty(PATH_LIST_PROP, paths);
//        isSet = true;
//    }
//
//    if (isSet)
//        emit commitData(container);
//}
//
//void ImporterEntryPathItemDelegate::handleClear(QWidget *container)
//{
//    // An empty string list will indicate that the entry is being
//    // blanked and should be deleted.
//    container->setProperty(PATH_LIST_PROP, QStringList());
//
//    emit commitData(container);
//}
//
//void ImporterEntryPathItemDelegate::updateText()
//{
//    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(sender());
//    QWidget *editor = lineEdit->parentWidget();
//    emit commitData(editor);
//}

/**
    Model
**/

//int ImporterModel::rowCount(const QModelIndex &) const
//{
//    return options.length() + 1;
//}
//
//int ImporterModel::columnCount(const QModelIndex &) const
//{
//    return ImporterColumn::Count;
//}
//
//QVariant ImporterModel::data(const QModelIndex &index, int role) const
//{
//    QVariant result = QVariant();
//
//    if (index.row() >= options.length()) {
//        if (role == ImporterEntryRole::CheckEmpty)
//            result = true;
//        else
//            return QVariant();
//    } else if (role == Qt::DisplayRole) {
//        switch (index.column()) {
//        case ImporterColumn::Path:
//            result = options[index.row()].path;
//            break;
//        case ImporterColumn::Program:
//            result = options[index.row()].program;
//            break;
//        case ImporterColumn::Name:
//            result = options[index.row()].name;
//        }
//    } else if (role == Qt::EditRole) {
//        if (index.column() == ImporterColumn::Name) {
//            result = options[index.row()].name;
//        }
//    } else if (role == Qt::CheckStateRole) {
//        switch (index.column()) {
//        case ImporterColumn::Selected:
//            if (options[index.row()].program != "")
//                result = options[index.row()].selected
//                         ? Qt::Checked
//                         : Qt::Unchecked;
//            else
//                result = Qt::Unchecked;
//        }
//    } else if (role == ImporterEntryRole::CheckEmpty) {
//        result = options[index.row()].empty;
//    }
//
//    return result;
//}
//
//Qt::ItemFlags ImporterModel::flags(const QModelIndex &index) const
//{
//    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
//
//    if (index.column() == ImporterColumn::Selected &&
//        index.row() != options.length()) 
//    {
//        flags |= Qt::ItemIsUserCheckable;
//    } 
//    else if (index.column() == ImporterColumn::Path ||
//           (index.column() == ImporterColumn::Name &&
//            index.row() != options.length())) 
//    {
//        flags |= Qt::ItemIsEditable;
//    }
//
//    return flags;
//}
//
//void ImporterModel::checkInputPath(int row)
//{
//    ImporterEntry &entry = options[row];
//
//    if (entry.path.isEmpty()) {
//        entry.program = "";
//        entry.empty = true;
//        entry.selected = false;
//        entry.name = "";
//    } else {
//        entry.empty = false;
//
//        std::string program = DetectProgram(entry.path.toStdString());
//        entry.program = QT_UTF8(Str(program.c_str()));
//
//        if (program.empty()) {
//            entry.selected = false;
//        } else {
//            std::string name =
//                GetSCName(entry.path.toStdString(), program);
//            entry.name = name.c_str();
//        }
//    }
//
//    emit dataChanged(index(row, 0), index(row, ImporterColumn::Count));
//}
//
//bool ImporterModel::setData(const QModelIndex &index, const QVariant &value,
//                int role)
//{
//    if (role == ImporterEntryRole::NewPath) {
//        QStringList list = value.toStringList();
//
//        if (list.size() == 0) {
//            if (index.row() < options.size()) {
//                beginRemoveRows(QModelIndex(), index.row(),
//                        index.row());
//                options.removeAt(index.row());
//                endRemoveRows();
//            }
//        } else {
//            if (list.size() > 0 && index.row() < options.length()) {
//                options[index.row()].path = list[0];
//                checkInputPath(index.row());
//
//                list.removeAt(0);
//            }
//
//            if (list.size() > 0) {
//                int row = index.row();
//                int lastRow = row + list.size() - 1;
//                beginInsertRows(QModelIndex(), row, lastRow);
//
//                for (QString path : list) {
//                    ImporterEntry entry;
//                    entry.path = path;
//
//                    options.insert(row, entry);
//
//                    row++;
//                }
//
//                endInsertRows();
//
//                for (row = index.row(); row <= lastRow; row++) {
//                    checkInputPath(row);
//                }
//            }
//        }
//    } else if (index.row() == options.length()) {
//        QString path = value.toString();
//
//        if (!path.isEmpty()) {
//            ImporterEntry entry;
//            entry.path = path;
//            entry.selected = role != ImporterEntryRole::AutoPath;
//            entry.empty = false;
//
//            beginInsertRows(QModelIndex(), options.length() + 1,
//                    options.length() + 1);
//            options.append(entry);
//            endInsertRows();
//
//            checkInputPath(index.row());
//        }
//    } else if (index.column() == ImporterColumn::Selected) {
//        bool select = value.toBool();
//
//        options[index.row()].selected = select;
//    } else if (index.column() == ImporterColumn::Path) {
//        QString path = value.toString();
//        options[index.row()].path = path;
//
//        checkInputPath(index.row());
//    } else if (index.column() == ImporterColumn::Name) {
//        QString name = value.toString();
//        options[index.row()].name = name;
//    }
//
//    emit dataChanged(index, index);
//
//    return true;
//}
//
//QVariant ImporterModel::headerData(int section, Qt::Orientation orientation, int role) const
//{
//    QVariant result = QVariant();
//
//    if (role == Qt::DisplayRole &&
//        orientation == Qt::Orientation::Horizontal) {
//        switch (section) {
//        case ImporterColumn::Path:
//            result = QT_UTF8(Str("Importer.Path"));
//            break;
//        case ImporterColumn::Program:
//            result = QT_UTF8(Str("Importer.Program"));
//            break;
//        case ImporterColumn::Name:
//            result = QT_UTF8(Str("Importer.Name"));
//        }
//    }
//
//    return result;
//}

/**
    Window
**/

AFQImporterDialog::AFQImporterDialog(QWidget *parent)
    : AFTTopBaseDialog(parent),
    ui(new Ui::AFQImporterDialog)
    //  optionsModel(new ImporterModel),
{
    setAcceptDrops(true);


    ui->setupUi(this);
    
#ifdef __APPLE__
    setWindowFlags(Qt::Window|Qt::WindowCloseButtonHint|Qt::CustomizeWindowHint);
    ui->titleWidget->hide();
#endif

    ui->label_Name->setText(QTStr("Name"));
    ui->label_Path->setText(QTStr("Importer.Path"));
    ui->label_Platform->setText(QTStr("Importer.Program"));
    ui->pushButton_Close->setProperty("buttonType", "closeButton");
    ui->labelTitle->setProperty("labelType", "labelTitle");

    //RIP TableView
    //ui->tableView->setModel(optionsModel);
    //ui->tableView->horizontalHeader()->setFixedHeight(IMPORTER_ITEM_HEIGHT);
    //ui->tableView->verticalHeader()->setDefaultSectionSize(IMPORTER_ITEM_HEIGHT);
    //ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    //ui->tableView->setItemDelegateForColumn(
    //    ImporterColumn::Path, new ImporterEntryPathItemDelegate());
    //ui->tableView->horizontalHeader()->setSectionResizeMode(
    //    QHeaderView::ResizeMode::ResizeToContents);
    //ui->tableView->horizontalHeader()->setSectionResizeMode(
    //    ImporterColumn::Path, QHeaderView::ResizeMode::Stretch);
    //ui->tableView->horizontalHeader()->setSectionResizeMode(
    //    ImporterColumn::Name, QHeaderView::ResizeMode::Stretch);

    //connect(optionsModel, &ImporterModel::dataChanged, this,
    //    &AFQImporterDialog::dataChanged);

    //ui->tableView->setEditTriggers(
    //    QAbstractItemView::EditTrigger::CurrentChanged);
    //RIP TableView

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QTStr("OK"));
    ui->buttonBox->button(QDialogButtonBox::Open)->setText(QTStr("Add"));

    connect(ui->buttonBox->button(QDialogButtonBox::Ok),
        &QPushButton::clicked, this, &AFQImporterDialog::importCollections);
    connect(ui->buttonBox->button(QDialogButtonBox::Open),
        &QPushButton::clicked, this, &AFQImporterDialog::qslotBrowseImport);
    connect(ui->buttonBox->button(QDialogButtonBox::Close),
        &QPushButton::clicked, this, &AFQImporterDialog::close);
    connect(ui->pushButton_Close, &QPushButton::clicked, this, &AFQImporterDialog::close);

    ImportersInit();

    //config_t* userConfig = USERCONFIG;
    //
    //Auto Scene Collection Hide
    //bool autoSearchPrompt = config_get_bool(userConfig,  "General", "AutoSearchPrompt");

    //if (!autoSearchPrompt) {
    //    // Need Check Alert Box

    //    bool result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, this,
    //        QT_UTF8(""),
    //        localeManager.Str("Importer.AutomaticCollectionText"));

    //    if (result == QDialog::Accepted)
    //    {
    //        config_set_bool(userConfig, "General", "AutomaticCollectionSearch", true);
    //    }
    //    else
    //    {
    //        config_set_bool(userConfig, "General", "AutomaticCollectionSearch", false);
    //    }

    //    config_set_bool(userConfig, "General", "AutoSearchPrompt", true);
    //}

    //bool autoSearch = config_get_bool(userConfig, "General", "AutomaticCollectionSearch");

    //OBSImporterFiles f;
    //if (autoSearch)
    //    f = ImportersFindFiles();

    //for (size_t i = 0; i < f.size(); i++) {
    //    QString path = f[i].c_str();
    //    path.replace("\\", "/");
    //    AddImportOption(path, true, nullptr);
    //}
    //Auto Scene Collection Hide

    //f.clear();
    //Auto Scene Collection Hide

    qslotMakeNewRow();

    //RIP TableView
    //ui->tableView->resizeColumnsToContents();
    /*ui->tableView->setColumnWidth(ImporterColumn::Selected, 20 + IMPORTER_ITEM_MARGIN);
    ui->tableView->setColumnWidth(ImporterColumn::Name, 140 + IMPORTER_ITEM_MARGIN);
    ui->tableView->setColumnWidth(ImporterColumn::Path, 344 + IMPORTER_ITEM_MARGIN);
    ui->tableView->setColumnWidth(ImporterColumn::Program, 130);
    
    QModelIndex index =
        optionsModel->createIndex(optionsModel->rowCount() - 1, 2);
    QMetaObject::invokeMethod(ui->tableView, "setCurrentIndex",
                  Qt::QueuedConnection,
                  Q_ARG(const QModelIndex &, index));*/
    //RIP TableView
}

void AFQImporterDialog::AddImportOption(QString path, bool automatic, AFQImporterRowWidget* addRow)
{
/*  QStringList list;
    list.append(path);*/

    ImporterEntryInfo entry;
    entry.path = path;
    entry.selected = automatic ?  ImporterEntryRole::AutoPath : ImporterEntryRole::NewPath;

    std::string program = DetectProgram(entry.path.toStdString());
    entry.program = QTStr(program.c_str());

    if (program.empty()) {
        entry.selected = false;
    }
    else {
        std::string name =
            GetSCName(entry.path.toStdString(), program);
        entry.name = name.c_str();
    }

    if (addRow == nullptr)
    {
        AFQImporterRowWidget* row = new AFQImporterRowWidget(this);
        row->AddNewRow();
        row->AddInfoRow(entry.name, entry.path, entry.program);
        row->SetCheckBoxChecked(automatic);
        connect(row, &AFQImporterRowWidget::qsignalDeleteRow, this, &AFQImporterDialog::qslotDeleteRow);
        connect(row, &AFQImporterRowWidget::qsignalBrowseImport, this, &AFQImporterDialog::qslotBrowseSingleImport);
        ui->widget_Table->layout()->addWidget(row);
        m_importEntryWidget.append(row);
    }
    else
    {
        addRow->AddInfoRow(entry.name, entry.path, entry.program);
    }
    

    //RIP TableView
    /*QModelIndex insertIndex = optionsModel->index(
        optionsModel->rowCount() - 1, ImporterColumn::Path);

    optionsModel->setData(insertIndex, list,
                  automatic ? ImporterEntryRole::AutoPath
                    : ImporterEntryRole::NewPath);*/

     //RIP TableView
}

void AFQImporterDialog::dropEvent(QDropEvent *ev)
{
    /*for (QUrl url : ev->mimeData()->urls()) {
        QFileInfo fileInfo(url.toLocalFile());
        if (fileInfo.isDir()) {

            QDirIterator dirIter(fileInfo.absoluteFilePath(),
                         QDir::Files);

            while (dirIter.hasNext()) {
                addImportOption(dirIter.next(), false);
            }
        } else {
            addImportOption(fileInfo.canonicalFilePath(), false);
        }
    }*/
}

void AFQImporterDialog::dragEnterEvent(QDragEnterEvent *ev)
{
    /*if (ev->mimeData()->hasUrls())
        ev->accept();*/
}



//bool GetUnusedName(std::string &name)
//{
//    if (!LoadSaveUtil::SceneCollectionExists(name.c_str()))
//        return false;
//
//    std::string newName;
//    int inc = 2;
//    do {
//        newName = name;
//        newName += " ";
//        newName += std::to_string(inc++);
//    } while (LoadSaveUtil::SceneCollectionExists(newName.c_str()));
//
//    name = newName;
//    return true;
//}

void AFQImporterDialog::importCollections()
{    
    setEnabled(false);

    const std::string OBSSceneCollectionPath = "/basic/scenes/";

    std::filesystem::path path1 = USERSCENES_PATH;
    const std::filesystem::path sceneCollectionLocation =
        USERSCENES_PATH / std::filesystem::u8path(OBSSceneCollectionPath);

    foreach (AFQImporterRowWidget* widget, m_importEntryWidget) 
    {
        bool check = widget->GetChecked();
        if (!check)
            continue;

        std::string pathStr = widget->GetPath();
        std::string nameStr = widget->GetName();
    }

    close();
}

//void AFQImporterDialog::dataChanged()
//{
    //ui->tableView->resizeColumnToContents(ImporterColumn::Name);
//}

void AFQImporterDialog::qslotMakeNewRow()
{
    AFQImporterRowWidget* row = reinterpret_cast<AFQImporterRowWidget*>(sender());
    disconnect(row, &AFQImporterRowWidget::qsignalNeedNewRow, this, &AFQImporterDialog::qslotMakeNewRow);

    AFQImporterRowWidget* newrow = new AFQImporterRowWidget(this);
    connect(newrow, &AFQImporterRowWidget::qsignalNeedNewRow, this, &AFQImporterDialog::qslotMakeNewRow);
    connect(newrow, &AFQImporterRowWidget::qsignalDeleteRow, this, &AFQImporterDialog::qslotDeleteRow);
    connect(newrow, &AFQImporterRowWidget::qsignalBrowseImport, this, &AFQImporterDialog::qslotBrowseSingleImport);
    newrow->AddNewRow();
    m_pNewRow = newrow;
    m_importEntryWidget.append(newrow);
    ui->widget_Table->layout()->addWidget(newrow);
}

void AFQImporterDialog::qslotDeleteRow()
{
    AFQImporterRowWidget* rowWidget = dynamic_cast<AFQImporterRowWidget*>(sender());
    rowWidget->close();
}

void AFQImporterDialog::qslotBrowseSingleImport()
{
    QString Pattern = "(*.json *.bpres *.xml *.xconfig)";

    QString path = OpenFile(this, QTStr("Importer.SelectCollection"), "",
                            QTStr("Importer.Collection") + QString(" ") + Pattern);

    if (!path.isEmpty()) {
        AFQImporterRowWidget* row = reinterpret_cast<AFQImporterRowWidget*>(sender());
        AddImportOption(path, false, row);
    }
}

void AFQImporterDialog::qslotBrowseImport()
{
    QString Pattern = "(*.json *.bpres *.xml *.xconfig)";

    QStringList paths = OpenFiles(
        this, QTStr("Importer.SelectCollection"), "",
        QTStr("Importer.Collection") + QString(" ") + Pattern);

    if (!paths.empty()) {

        m_pNewRow->close();
        m_importEntryWidget.removeOne(m_pNewRow);
        for (int i = 0; i < paths.count(); i++) {
            AddImportOption(paths[i], false, nullptr);
        }
        qslotMakeNewRow();
    }
}

void AFQImporterRowWidget::qslotEditPathFinished()
{
    if (!m_pPathEdit->text().isEmpty())
    {
        m_pNameEdit->setEnabled(true);
        m_pCheckBox->setEnabled(true);
        m_pCheckBox->setChecked(true);
        m_pDeleteButton->setEnabled(true);
        disconnect(m_pPathEdit, &QLineEdit::textChanged, this, &AFQImporterRowWidget::qslotEditPathFinished);
        emit qsignalNeedNewRow();
    }
}

void AFQImporterRowWidget::AddNewRow()
{
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(IMPORTER_ITEM_MARGIN);

    m_pCheckBox = new QCheckBox(this);
    m_pCheckBox->setText("");
    m_pCheckBox->setFixedSize(18, 18);
    m_pCheckBox->setEnabled(false);

    m_pNameEdit = new QLineEdit(this);
    m_pNameEdit->setFixedSize(140, 40);
    m_pNameEdit->setEnabled(false);
    m_pNameEdit->setObjectName("lineEdit_Name");

    /////// Path Widget 
    QWidget* pathWidget = new QWidget(this);
    pathWidget->setFixedSize(344, 40);
    pathWidget->setObjectName("widget_PathWidget");

    m_pPathEdit = new QLineEdit(this);
    m_pPathEdit->setFixedSize(284, 40);
    m_pPathEdit->setObjectName("lineEdit_Path");
    connect(m_pPathEdit, &QLineEdit::textChanged, this, &AFQImporterRowWidget::qslotEditPathFinished);

    QPushButton* pathButton = new QPushButton(this);
    pathButton->setObjectName("pushButton_Browser");
    pathButton->setProperty("buttonType", "hthreeDots");
    pathButton->setFixedSize(30, 30);
    connect(pathButton, &QPushButton::clicked, this, &AFQImporterRowWidget::qsignalBrowseImport);

    m_pDeleteButton = new QPushButton(this);
    m_pDeleteButton->setObjectName("pushButton_Delete");
    m_pDeleteButton->setFixedSize(30, 30);
    m_pDeleteButton->setEnabled(false);
    connect(m_pDeleteButton, &QPushButton::clicked, this, &AFQImporterRowWidget::qsignalDeleteRow);

    QHBoxLayout* pathlayout = new QHBoxLayout();
    pathlayout->setContentsMargins(0, 0, 0, 0);
    pathlayout->setSpacing(0);

    pathlayout->addWidget(m_pPathEdit);
    pathlayout->addWidget(pathButton);
    pathlayout->addWidget(m_pDeleteButton);

    pathWidget->setLayout(pathlayout);

    /////// Path Widget 

    m_pProgramEdit = new QLineEdit(this);
    m_pProgramEdit->setFixedSize(130, 40);
    m_pProgramEdit->setEnabled(false);
    m_pProgramEdit->setObjectName("lineEdit_Program");

    layout->addWidget(m_pCheckBox);
    layout->addWidget(m_pNameEdit);
    layout->addWidget(pathWidget);
    layout->addWidget(m_pProgramEdit);

    setLayout(layout);
    m_init = true;
}

void AFQImporterRowWidget::AddInfoRow(QString name, QString path, QString program)
{
    if (m_init)
    {
        m_pCheckBox->setEnabled(true);
        m_pNameEdit->setText(name);
        m_pNameEdit->setEnabled(true);
        m_pPathEdit->setText(path);
        m_pProgramEdit->setText(program);
        m_pDeleteButton->setEnabled(true);
    }
}

void AFQImporterRowWidget::SetCheckBoxChecked(bool checked)
{
    m_pCheckBox->setChecked(!checked);
}

bool AFQImporterRowWidget::GetChecked()
{
    return m_pCheckBox->isChecked();
}

std::string AFQImporterRowWidget::GetName()
{
    return m_pNameEdit->text().toStdString();
}

std::string AFQImporterRowWidget::GetPath()
{
    return m_pPathEdit->text().toStdString();
}

std::string AFQImporterRowWidget::GetProgram()
{
    return m_pProgramEdit->text().toStdString();
}
