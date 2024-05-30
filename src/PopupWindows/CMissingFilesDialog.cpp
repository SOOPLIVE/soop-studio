#include "CMissingFilesDialog.h"


#include <QLineEdit>
#include <QToolButton>
#include <QFileDialog>


#include "qt-wrapper.h"


#include <util/platform.h>


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
    : AFQRoundedDialogBase(parent, Qt::WindowFlags(), false),
      ui(new Ui::AFQMissingFilesDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->setupUi(this);

    for (size_t i = 0; i < obs_missing_files_count(files); i++) {
        obs_missing_file_t *f =
            obs_missing_files_get_file(files, (int)i);

        const char *oldPath = obs_missing_file_get_path(f);
        const char *name = obs_missing_file_get_source_name(f);

        AFQMissingFileColumnWidget* widget = new AFQMissingFileColumnWidget(this);
        widget->AddMissingFileWidget(name, oldPath);
        m_qFiles.insert(i, widget);
        ui->widget_Contents->layout()->addWidget(widget);
    }
    
    
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    

    QString found =
        QT_UTF8(localeManager.Str("MissingFiles.NumFound"))
            .arg("0",
                 QString::number(obs_missing_files_count(files)));

    ui->found->setText(found);

    fileStore = files;

    connect(ui->doneButton, &QPushButton::clicked, this,
        &AFQMissingFilesDialog::saveFiles);
    connect(ui->browseButton, &QPushButton::clicked, this,
        &AFQMissingFilesDialog::browseFolders);
    connect(ui->cancelButton, &QPushButton::clicked, this,
        &AFQMissingFilesDialog::close);
    connect(ui->pushButton_Close, &QPushButton::clicked, this,
        &AFQMissingFilesDialog::close);

    ui->pushButton_Close->setProperty("buttonType", "closeButton");

    this->SetHeightFixed(true);
    this->SetWidthFixed(true);

    
}

AFQMissingFilesDialog::~AFQMissingFilesDialog()
{
    obs_missing_files_destroy(fileStore);
}

void AFQMissingFilesDialog::saveFiles()
{
    for (int i = 0; i < m_qFiles.length(); i++) {
        MissingFilesState state = m_qFiles[i]->GetState();
        if (state != MissingFilesState::Missing) {
            obs_missing_file_t *f =
                obs_missing_files_get_file(fileStore, i);

            QString path = m_qFiles[i]->GetNewName();

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
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    
    
    QString dir = QFileDialog::getExistingDirectory(
        this, QT_UTF8(localeManager.Str("MissingFiles.SelectDir")), "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir != "") {
        dir += "/";
        _FileCheckLoop(dir, true);
    }
}

void AFQMissingFilesDialog::_FileCheckLoop(QString path, bool skipPrompt)
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();

    m_bLoop = false;
    QUrl url = QUrl().fromLocalFile(path);
    QString dir =
        url.toDisplayString(QUrl::RemoveScheme | QUrl::RemoveFilename |
            QUrl::PreferLocalFile);

    bool prompted = skipPrompt;

    for (int i = 0; i < m_qFiles.length(); i++) {
        if (m_qFiles[i]->GetState() != MissingFilesState::Missing)
            continue;

        QUrl origFile = QUrl().fromLocalFile(m_qFiles[i]->GetOriginalName());
        QString filename = origFile.fileName();
        QString testFile = dir + filename;

        if (os_file_exists(testFile.toStdString().c_str())) {
            if (!prompted) {
                int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                    this,
                    "",
                    QT_UTF8(localeManager.Str("MissingFiles.AutoSearchText")));

                if (result == QDialog::Rejected)
                    break;

                prompted = true;
            }
            m_qFiles[i]->SetNewName(testFile);
            m_qFiles[i]->qslotEditingFinished();
        }
    }
    m_bLoop = true;
}

void AFQMissingFileColumnWidget::AddMissingFileWidget(const char* name, const char* oldPath)
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();

    QHBoxLayout* totalLayout = new QHBoxLayout();
    totalLayout->setSpacing(10);
    totalLayout->setContentsMargins(27, 0, 30, 0);

    m_qChangedCheck = new QPushButton(this);
    m_qChangedCheck->setCheckable(true);
    m_qChangedCheck->setChecked(false);
    m_qChangedCheck->setFixedSize(18, 18);
    m_qChangedCheck->setEnabled(false);
    m_qChangedCheck->setObjectName("pushButton_Checked");
    totalLayout->addWidget(m_qChangedCheck);

    ///////NAME///////
    QHBoxLayout* namelayout = new QHBoxLayout();
    namelayout->setSpacing(6);
    namelayout->setContentsMargins(8, 0, 8, 0);

    AFIconContext& iconContext = AFIconContext::GetSingletonInstance();

    OBSSourceAutoRelease source = obs_get_source_by_name(name);
    QIcon sourceicon = iconContext.GetSourceIcon(obs_source_get_id(source));
    QLabel* iconLabel = new QLabel(this);
    iconLabel->setPixmap(sourceicon.pixmap(18,18)); 
    iconLabel->setFixedSize(18, 18);
    m_qSourceName = new AFQElidedSlideLabel(this);
    m_qSourceName->setText(name);
    m_qSourceName->setToolTip(name);

    namelayout->addWidget(iconLabel);
    namelayout->addWidget(m_qSourceName);

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

    m_qMissingPath = new AFQElidedSlideLabel(this);

    QFileInfo fi(oldPath);
    m_qMissingPath->setText(fi.fileName());
    m_qMissingPath->setToolTip(oldPath);
    missinglayout->addWidget(m_qMissingPath);

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

    m_qNewFilePath = new QLineEdit(this);
    m_qNewFilePath->setObjectName("lineEdit_NewPath");
    m_qNewFilePath->setFixedHeight(40);
    connect(m_qNewFilePath, &QLineEdit::editingFinished,
        this, &AFQMissingFileColumnWidget::qslotEditingFinished);

    m_qFindPathButton = new QPushButton(this);
    m_qFindPathButton->setFixedSize(30, 30);
    m_qFindPathButton->setObjectName("pushButton_FindPath");
    connect(m_qFindPathButton, &QPushButton::clicked,
        this, &AFQMissingFileColumnWidget::qslotBrowseFilePath);

    m_qPathResetButton = new QPushButton(this);
    m_qPathResetButton->setFixedSize(30, 30);
    m_qPathResetButton->setObjectName("pushButton_ResetPath");
    connect(m_qPathResetButton, &QPushButton::clicked,
        this, &AFQMissingFileColumnWidget::qslotClearFilePath);

    newLayout->addWidget(m_qNewFilePath);
    newLayout->addWidget(m_qFindPathButton);
    newLayout->addWidget(m_qPathResetButton);

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

    m_qStatusLabel = new AFQElidedSlideLabel(this);
    m_qStatusLabel->setText(localeManager.Str("MissingFiles.Missing"));
    m_qStatusLabel->setToolTip(m_qStatusLabel->text());
    m_State = MissingFilesState::Missing;

    statusLayout->addWidget(m_qStatusLabel);

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
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();

    QString currentPath = m_qNewFilePath->text();
    /*if (currentPath.isEmpty() ||
        currentPath.compare(QT_UTF8(localeManager.Str("MissingFiles.Clear"))) == 0)
        currentPath = defaultPath;*/

    bool isSet = false;
    QString newPath = QFileDialog::getOpenFileName(
        this, QT_UTF8(localeManager.Str("MissingFiles.SelectFile")),
        currentPath, nullptr);

#ifdef __APPLE__
    // TODO: Revisit when QTBUG-42661 is fixed
    this->window()->raise();
#endif

    if (!newPath.isEmpty()) {
        m_qNewFilePath->setText(newPath);
        m_qChangedCheck->setChecked(true);
        auto& localeManager = AFLocaleTextManager::GetSingletonInstance();


        QFileInfo fi(newPath);
        if (m_qMissingPath->text().compare(fi.fileName()) == 0)
        {
            m_qStatusLabel->setText(localeManager.Str("MissingFiles.Found"));
            m_qStatusLabel->setToolTip(m_qStatusLabel->text());
            m_State = MissingFilesState::Found;
        }
        else
        {
            m_qStatusLabel->setText(localeManager.Str("MissingFiles.Replaced"));
            m_qStatusLabel->setToolTip(m_qStatusLabel->text());
            m_State = MissingFilesState::Replaced;
        }
    }
}

void AFQMissingFileColumnWidget::qslotClearFilePath()
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();
    m_qChangedCheck->setChecked(true);
    m_qNewFilePath->clear();
    m_qNewFilePath->setText(localeManager.Str("MissingFiles.Cleared"));
    m_qStatusLabel->setText(localeManager.Str("MissingFiles.Cleared"));
    m_State = MissingFilesState::Cleared;
    m_qStatusLabel->setToolTip(m_qStatusLabel->text());
}

void AFQMissingFileColumnWidget::qslotEditingFinished()
{
    auto& localeManager = AFLocaleTextManager::GetSingletonInstance();

    if (m_qNewFilePath->text().isNull() || m_qNewFilePath->text().isEmpty())
    {
        m_qStatusLabel->setText(localeManager.Str("MissingFiles.Missing"));
        m_qStatusLabel->setToolTip(m_qStatusLabel->text());
        m_State = MissingFilesState::Missing;
        return;
    }

    QFileInfo fi(m_qNewFilePath->text());
    if (m_qMissingPath->text().compare(fi.fileName()) == 0)
    {
        m_qStatusLabel->setText(localeManager.Str("MissingFiles.Found"));
        m_qStatusLabel->setToolTip(m_qStatusLabel->text());
        m_State = MissingFilesState::Found;
    }
    else
    {
        m_qStatusLabel->setText(localeManager.Str("MissingFiles.Replaced"));
        m_qStatusLabel->setToolTip(m_qStatusLabel->text());
        m_State = MissingFilesState::Replaced;
    }
    m_qChangedCheck->setChecked(true);
}
