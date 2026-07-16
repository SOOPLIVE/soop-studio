#pragma once

#include <obs.hpp>

#include <QDialog>
#include <QPointer>
#include <QStyledItemDelegate>

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

#include "ui_missing-files-dialog.h"

#include "UIComponent/CTopBaseWindow.h"
#include "UIComponent/CElidedSlideLabel.h"

class MissingFilesModel;

enum MissingFilesState { Missing, Found, Replaced, Cleared };
Q_DECLARE_METATYPE(MissingFilesState);

class AFQMissingFileColumnWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AFQMissingFileColumnWidget(QWidget* parent = nullptr) {};
    ~AFQMissingFileColumnWidget() {};

    void AddMissingFileWidget(const char* name, const char* oldPath);
    QString GetSourceName() { return m_pSourceName->text(); };
    QString GetOriginalName() { return m_pMissingPath->text(); };
    QString GetNewName() { return m_pNewFilePath->text(); };
    MissingFilesState GetState() { return m_state; };
    
    void SetNewName(QString path) { m_pNewFilePath->setText(path); };

public slots:
    void qslotBrowseFilePath();
    void qslotClearFilePath();
    void qslotEditingFinished();

private:
    AFQElidedSlideLabel* m_pSourceName;
    AFQElidedSlideLabel* m_pMissingPath;
    QLineEdit* m_pNewFilePath;
    QPushButton* m_pPathResetButton;
    QPushButton* m_pFindPathButton;
    AFQElidedSlideLabel* m_pStatusLabel;
    QPushButton* m_pChangedCheck;

    MissingFilesState m_state;
};

class AFQMissingFilesDialog : public AFTTopBaseDialog
{
    Q_OBJECT

    std::unique_ptr<Ui::AFQMissingFilesDialog> ui;

public:
    explicit AFQMissingFilesDialog(obs_missing_files_t* files,
                                   QWidget* parent = nullptr);
    virtual ~AFQMissingFilesDialog() override;

private:
    void saveFiles();
    void browseFolders();

    obs_missing_files_t* m_pFileStore;
private:

    bool m_loop = true;

    QList<AFQMissingFileColumnWidget*> m_files;

    void _FileCheckLoop(QString path, bool skipPrompt);
};
