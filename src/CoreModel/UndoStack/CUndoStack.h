#pragma once

#include <deque>
#include <functional>
#include <string>
#include <memory>

#include <QTimer>
#include "qt-wrappers.hpp"

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

public:
    AFUndoStack(AFMainFrame* main);

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

    // Change Source Name
    void AddActionRename(std::string& prevName, std::string& newName, obs_source_t* source);

private slots:
    void qslotResetRepeatableState();

public:
    QAction* m_pActionMainUndo = nullptr;
    QAction* m_pActionMainRedo = nullptr;
};
//
extern bool save_undo_source_enum(obs_scene_t* /*scene*/, obs_sceneitem_t* item, void* p);
extern void undo_redo(const std::string& data);