#pragma once

#include <qobject.h>
#include <qstring.h>
#include <qtimer.h>
#include <qaction.h>

#include <deque>
#include <functional>
#include <string>
#include <memory>

class AFMainFrame;

class AFUndoStack : public QObject
{
	Q_OBJECT

    typedef std::function<void(const std::string& data)> undo_redo_cb;
    typedef std::function<void(bool isUndo)> func;

    struct undo_redo_t {
        QString name;
        std::string undoData;
        std::string redoData;
        undo_redo_cb undoCallback;
        undo_redo_cb redoCallback;
    };

    std::deque<undo_redo_t> m_undoList;
    std::deque<undo_redo_t> m_redoList;
    int m_disableRefs = 0;
    bool m_enabled = true;
    bool m_lastIsRepeatable = false;

    QTimer m_repeatResetTimer;

    inline bool IsEnabled() const { return !m_disableRefs && m_enabled; }

    void EnableInternal();
    void DisableInternal();
    void ClearRedo();

#pragma region QT Field, CTOR/DTOR
public:
    AFUndoStack(AFMainFrame* main);

#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void Enable();
    void Disable();
    void PushDisabled();
    void PopDisabled();

    void Clear();
    void AddAction(const QString& name,
                   const undo_redo_cb& undoCallback,
                   const undo_redo_cb& redoCallback,
                   const std::string& undoData,
                   const std::string& redoData,
                   bool repeatable = false);
    void Undo();
    void Redo();

signals:

#pragma endregion public func

#pragma region private func
private:

#pragma endregion private func

#pragma region private slot func
private slots:
    void qSlotResetRepeatableState();

#pragma endregion private slot func

#pragma region public member var
public:
    QAction*    m_actionMainUndo;
    QAction*    m_actionMainRedo;

#pragma endregion public member var

#pragma region private member var
private:

#pragma endregion private member var
};
