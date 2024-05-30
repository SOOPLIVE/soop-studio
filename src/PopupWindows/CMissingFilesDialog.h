#pragma once

#include <obs.hpp>

#include <QDialog>
#include <QPointer>
#include <QStyledItemDelegate>

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

#include "ui_missing-files-dialog.h"

#include "UIComponent/CRoundedDialogBase.h"
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
    QString GetSourceName() { return m_qSourceName->text(); };
    QString GetOriginalName() { return m_qMissingPath->text(); };
    QString GetNewName() { return m_qNewFilePath->text(); };
    MissingFilesState GetState() { return m_State; };
    
    void SetNewName(QString path) { m_qNewFilePath->setText(path); };

public slots:
    void qslotBrowseFilePath();
    void qslotClearFilePath();
    void qslotEditingFinished();

private:
    AFQElidedSlideLabel* m_qSourceName;
    AFQElidedSlideLabel* m_qMissingPath;
    QLineEdit* m_qNewFilePath;
    QPushButton* m_qPathResetButton;
    QPushButton* m_qFindPathButton;
    AFQElidedSlideLabel* m_qStatusLabel;
    QPushButton* m_qChangedCheck;

    MissingFilesState m_State;
};

class AFQMissingFilesDialog : public AFQRoundedDialogBase
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

    obs_missing_files_t* fileStore;
private:

    bool m_bLoop = true;

    QList<AFQMissingFileColumnWidget*> m_qFiles;

    void _FileCheckLoop(QString path, bool skipPrompt);
};