#include "CMissingFilesDialog.h"


#include <QLineEdit>
#include <QToolButton>
#include <QFileDialog>

#include <util/platform.h>

#include "Application/CApplication.h"

#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Icon/CIconContext.h"
#include "UIComponent/CMessageBox.h"


enum MissingFilesColumn {
    Source,
    OriginalPath,
    NewPath,
    State,

    Count
};

enum MissingFilesRole { EntryStateRole = Qt::UserRole, NewPathsToProcessRole };

/**********************************************************
  Delegate - Presents cells in the grid.
**********************************************************/


AFQMissingFilesDialog::AFQMissingFilesDialog(obs_missing_files_t *files, QWidget *parent)
    : AFTTopBaseDialog(parent),
      ui(new Ui::AFQMissingFilesDialog)
{
    ui->setupUi(this);
    setModal(false);
    setWindowFlags(Qt::Window);

    for (size_t i = 0; i < obs_missing_files_count(files); i++) {
        obs_missing_file_t *f =
            obs_missing_files_get_file(files, (int)i);

        const char *oldPath = obs_missing_file_get_path(f);
        const char *name = obs_missing_file_get_source_name(f);

        AFQMissingFileColumnWidget* widget = new AFQMissingFileColumnWidget(this);
        widget->AddMissingFileWidget(name, oldPath);
        m_files.insert(i, widget);
        ui->widget_Contents->layout()->addWidget(widget);
    }
 
    QString found =
        QTStr("MissingFiles.NumFound")
            .arg("0",
                 QString::number(obs_missing_files_count(files)));

    ui->found->setText(found);

    m_pFileStore = files;

    connect(ui->doneButton, &QPushButton::clicked, this,
        &AFQMissingFilesDialog::saveFiles);
    connect(ui->browseButton, &QPushButton::clicked, this,
        &AFQMissingFilesDialog::browseFolders);
    connect(ui->cancelButton, &QPushButton::clicked, this,
        &AFQMissingFilesDialog::close);
    connect(ui->pushButton_Close, &QPushButton::clicked, this,
        &AFQMissingFilesDialog::close);

    ui->pushButton_Close->setProperty("buttonType", "closeButton");

    SetWidthResizeEnabled(false);
    SetHeightResizeEnabled(false);
}

AFQMissingFilesDialog::~AFQMissingFilesDialog()
{
    obs_missing_files_destroy(m_pFileStore);
}

void AFQMissingFilesDialog::saveFiles()
{
    for (int i = 0; i < m_files.length(); i++) {
        MissingFilesState state = m_files[i]->GetState();
        if (state != MissingFilesState::Missing) {
            obs_missing_file_t *f =
                obs_missing_files_get_file(m_pFileStore, i);

            QString path = m_files[i]->GetNewName();

            if (state == MissingFilesState::Cleared) {
                obs_missing_file_issue_callback(f, "");
            } else {
                char *p = bstrdup(path.toStdString().c_str());
                obs_missing_file_issue_callback(f, p);
                bfree(p);
            }
        }
    }

    QDialog::accept();
}

void AFQMissingFilesDialog::browseFolders()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, QTStr("MissingFiles.SelectDir"), "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir != "") {
        dir += "/";
        _FileCheckLoop(dir, true);
    }
}

void AFQMissingFilesDialog::_FileCheckLoop(QString path, bool skipPrompt)
{
    m_loop = false;
    QUrl url = QUrl().fromLocalFile(path);
    QString dir =
        url.toDisplayString(QUrl::RemoveScheme | QUrl::RemoveFilename |
            QUrl::PreferLocalFile);

    bool prompted = skipPrompt;

    for (int i = 0; i < m_files.length(); i++) {
        if (m_files[i]->GetState() != MissingFilesState::Missing)
            continue;

        QUrl origFile = QUrl().fromLocalFile(m_files[i]->GetOriginalName());
        QString filename = origFile.fileName();
        QString testFile = dir + filename;

        if (os_file_exists(testFile.toStdString().c_str())) {
            if (!prompted) {
                int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                    this,
                    "",
                    QTStr("MissingFiles.AutoSearchText"));

                if (result == QDialog::Rejected)
                    break;

                prompted = true;
            }
            m_files[i]->SetNewName(testFile);
            m_files[i]->qslotEditingFinished();
        }
    }
    m_loop = true;
}

void AFQMissingFileColumnWidget::AddMissingFileWidget(const char* name, const char* oldPath)
{
    QHBoxLayout* totalLayout = new QHBoxLayout();
    totalLayout->setSpacing(10);
    totalLayout->setContentsMargins(27, 0, 30, 0);

    m_pChangedCheck = new QPushButton(this);
    m_pChangedCheck->setCheckable(true);
    m_pChangedCheck->setChecked(false);
    m_pChangedCheck->setFixedSize(18, 18);
    m_pChangedCheck->setEnabled(false);
    m_pChangedCheck->setObjectName("pushButton_Checked");
    totalLayout->addWidget(m_pChangedCheck);

    ///////NAME///////
    QHBoxLayout* namelayout = new QHBoxLayout();
    namelayout->setSpacing(6);
    namelayout->setContentsMargins(8, 0, 8, 0);

    OBSSourceAutoRelease source = obs_get_source_by_name(name);
    if (source)
    {
        QIcon sourceicon = ICON_CONTEXT.GetSourceIcon(obs_source_get_id(source));
        QLabel* iconLabel = new QLabel(this);
        iconLabel->setPixmap(sourceicon.pixmap(24, 24));
        iconLabel->setFixedSize(24, 24);

        namelayout->addWidget(iconLabel);
    }

    m_pSourceName = new AFQElidedSlideLabel(this);
    m_pSourceName->setText(name);
    m_pSourceName->setToolTip(name);

    namelayout->addWidget(m_pSourceName);

    QWidget* nameWidget = new QWidget(this);
    nameWidget->setObjectName("widget_SourceName");
    nameWidget->setLayout(namelayout);
    nameWidget->setFixedSize(170, 40);

    totalLayout->addWidget(nameWidget);
    ///////NAME///////

    ///////OldPath///////
    QHBoxLayout* missinglayout = new QHBoxLayout();
    missinglayout->setSpacing(6);
    missinglayout->setContentsMargins(8, 0, 8, 0);

    m_pMissingPath = new AFQElidedSlideLabel(this);

    QFileInfo fi(oldPath);
    m_pMissingPath->setText(fi.fileName());
    m_pMissingPath->setToolTip(oldPath);
    missinglayout->addWidget(m_pMissingPath);

    QWidget* missingWidget = new QWidget(this);
    missingWidget->setObjectName("widget_MissingPathDir");
    missingWidget->setLayout(missinglayout);
    missingWidget->setFixedSize(210, 40);

    totalLayout->addWidget(missingWidget);
    ///////OldPath///////

    ///////NewPath///////
    QHBoxLayout* newLayout = new QHBoxLayout();
    newLayout->setSpacing(3);
    newLayout->setContentsMargins(0, 0, 8, 0);

    m_pNewFilePath = new QLineEdit(this);
    m_pNewFilePath->setObjectName("lineEdit_NewPath");
    m_pNewFilePath->setFixedHeight(40);
    connect(m_pNewFilePath, &QLineEdit::editingFinished,
        this, &AFQMissingFileColumnWidget::qslotEditingFinished);

    m_pFindPathButton = new QPushButton(this);
    m_pFindPathButton->setFixedSize(30, 30);
    m_pFindPathButton->setObjectName("pushButton_FindPath");
    connect(m_pFindPathButton, &QPushButton::clicked,
        this, &AFQMissingFileColumnWidget::qslotBrowseFilePath);

    m_pPathResetButton = new QPushButton(this);
    m_pPathResetButton->setFixedSize(30, 30);
    m_pPathResetButton->setObjectName("pushButton_ResetPath");
    connect(m_pPathResetButton, &QPushButton::clicked,
        this, &AFQMissingFileColumnWidget::qslotClearFilePath);

    newLayout->addWidget(m_pNewFilePath);
    newLayout->addWidget(m_pFindPathButton);
    newLayout->addWidget(m_pPathResetButton);

    QWidget* newWidget = new QWidget(this);
    newWidget->setObjectName("widget_NewPathDir");
    newWidget->setLayout(newLayout);
    newWidget->setFixedSize(210, 40);

    totalLayout->addWidget(newWidget);
    ///////NewPath///////

    ///////Status///////
    QHBoxLayout* statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(0);
    statusLayout->setContentsMargins(8, 0, 8, 0);

    m_pStatusLabel = new AFQElidedSlideLabel(this);
    m_pStatusLabel->setText(Str("MissingFiles.Missing"));
    m_pStatusLabel->setToolTip(m_pStatusLabel->text());
    m_state = MissingFilesState::Missing;

    statusLayout->addWidget(m_pStatusLabel);

    QWidget* statusWidget = new QWidget(this);
    statusWidget->setObjectName("widget_Status");
    statusWidget->setLayout(statusLayout);
    statusWidget->setFixedSize(114, 40);

    totalLayout->addWidget(statusWidget);

    ///////Status///////

    setLayout(totalLayout);
}

void AFQMissingFileColumnWidget::qslotBrowseFilePath()
{
    QString currentPath = m_pNewFilePath->text();
    /*if (currentPath.isEmpty() ||
        currentPath.compare(QT_UTF8(localeManager.Str("MissingFiles.Clear"))) == 0)
        currentPath = defaultPath;*/

    bool isSet = false;
    QString newPath = QFileDialog::getOpenFileName(
        this, QTStr("MissingFiles.SelectFile"),
        currentPath, nullptr);

#ifdef __APPLE__
    // TODO: Revisit when QTBUG-42661 is fixed
    this->window()->raise();
#endif

    if (!newPath.isEmpty()) {
        m_pNewFilePath->setText(newPath);
        m_pChangedCheck->setChecked(true);

        QFileInfo fi(newPath);
        if (m_pMissingPath->text().compare(fi.fileName()) == 0)
        {
            m_pStatusLabel->setText(Str("MissingFiles.Found"));
            m_pStatusLabel->setToolTip(m_pStatusLabel->text());
            m_state = MissingFilesState::Found;
        }
        else
        {
            m_pStatusLabel->setText(Str("MissingFiles.Replaced"));
            m_pStatusLabel->setToolTip(m_pStatusLabel->text());
            m_state = MissingFilesState::Replaced;
        }
    }
}

void AFQMissingFileColumnWidget::qslotClearFilePath()
{
    m_pChangedCheck->setChecked(true);
    m_pNewFilePath->clear();
    m_pNewFilePath->setText(Str("MissingFiles.Cleared"));
    m_pStatusLabel->setText(Str("MissingFiles.Cleared"));
    m_state = MissingFilesState::Cleared;
    m_pStatusLabel->setToolTip(m_pStatusLabel->text());
}

void AFQMissingFileColumnWidget::qslotEditingFinished()
{
    if (m_pNewFilePath->text().isNull() || m_pNewFilePath->text().isEmpty())
    {
        m_pStatusLabel->setText(Str("MissingFiles.Missing"));
        m_pStatusLabel->setToolTip(m_pStatusLabel->text());
        m_state = MissingFilesState::Missing;
        return;
    }

    QFileInfo fi(m_pNewFilePath->text());
    if (m_pMissingPath->text().compare(fi.fileName()) == 0)
    {
        m_pStatusLabel->setText(Str("MissingFiles.Found"));
        m_pStatusLabel->setToolTip(m_pStatusLabel->text());
        m_state = MissingFilesState::Found;
    }
    else
    {
        m_pStatusLabel->setText(Str("MissingFiles.Replaced"));
        m_pStatusLabel->setToolTip(m_pStatusLabel->text());
        m_state = MissingFilesState::Replaced;
    }
    m_pChangedCheck->setChecked(true);
}
