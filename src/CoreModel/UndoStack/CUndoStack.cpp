#include "CUndoStack.h"

#include <util/util.hpp>
#include "Application/CApplication.h"

#define MAX_STACK_SIZE  (5000)

//
void AFUndoStack::EnableInternal()
{
    m_lastIsRepeatable = false;

    m_actionMainUndo->setDisabled(false);
    if(!m_redoList.empty()) {
        m_actionMainRedo->setDisabled(false);
    }
}
void AFUndoStack::DisableInternal()
{
    m_lastIsRepeatable = false;
    m_actionMainUndo->setDisabled(true);
    m_actionMainRedo->setDisabled(true);
}
void AFUndoStack::ClearRedo()
{
    m_redoList.clear();
}
//

AFUndoStack::AFUndoStack(AFMainFrame* main)
{
    m_actionMainUndo = new QAction(main);
    m_actionMainRedo = new QAction(main);
    //
    QObject::connect(&m_repeatResetTimer, &QTimer::timeout, this,
                     &AFUndoStack::qSlotResetRepeatableState);
    m_repeatResetTimer.setSingleShot(true);
    m_repeatResetTimer.setInterval(3000);
}
//

void AFUndoStack::Enable()
{
    m_enabled = true;
    if(IsEnabled()) {
        EnableInternal();
    }
}
void AFUndoStack::Disable()
{
    if(IsEnabled()) {
        DisableInternal();
    }
    m_enabled = false;
}
void AFUndoStack::PushDisabled()
{
    if(IsEnabled()) {
        DisableInternal();
    }
    m_disableRefs++;
}
void AFUndoStack::PopDisabled()
{
    m_disableRefs--;
    if(IsEnabled()) {
        EnableInternal();
    }
}

void AFUndoStack::Clear()
{
    m_undoList.clear();
    m_redoList.clear();
    m_lastIsRepeatable = false;

    //actionMainUndo->setText(QTStr("Undo.Undo"));
    //actionMainRedo->setText(QTStr("Undo.Redo"));

    m_actionMainUndo->setDisabled(true);
    m_actionMainRedo->setDisabled(true);
}
void AFUndoStack::AddAction(const QString& name,
                            const undo_redo_cb& undoCallback,
                            const undo_redo_cb& redoCallback,
                            const std::string& undoData,
                            const std::string& redoData,
                            bool repeatable)
{
    if(!IsEnabled())
        return;

    while(m_undoList.size() >= MAX_STACK_SIZE)
    {
        undo_redo_t item = m_undoList.back();
        m_undoList.pop_back();
    }
    if(repeatable) {
        m_repeatResetTimer.start();
    }
    if(m_lastIsRepeatable && repeatable &&
       name == m_undoList[0].name) {
        m_undoList[0].redoCallback = redoCallback;
        m_undoList[0].redoData = redoData;
    }

    undo_redo_t newItem = { name, undoData, redoData, undoCallback, redoCallback };
    m_lastIsRepeatable = repeatable;
    m_undoList.push_front(newItem);
    ClearRedo();

    //actionMainUndo->setText(QTStr("Undo.Item.Undo").arg(name));
    m_actionMainUndo->setEnabled(true);

    //actionMainRedo->setText(QTStr("Undo.Item.Redo"));
    m_actionMainRedo->setEnabled(true);
}
void AFUndoStack::Undo()
{
    if(m_undoList.empty() || !IsEnabled())
        return;

    m_lastIsRepeatable = false;

    undo_redo_t item = m_undoList.front();
    item.undoCallback(item.undoData);
    m_redoList.push_front(item);
    m_undoList.pop_front();

    //actionMainRedo->setText(QTStr("Undo.Item.Redo").arg(item.name));
    m_actionMainRedo->setEnabled(true);

    if(m_undoList.empty()) {
        m_actionMainUndo->setDisabled(true);
        //actionMainUndo->setText(QTStr("Undo.Undo"));
    } else {
        //actionMainUndo->setText(QTStr("Undo.Item.Undo").arg(m_undoList.front().name));
    }
}
void AFUndoStack::Redo()
{
    if(m_redoList.empty() || !IsEnabled())
        return;

    m_lastIsRepeatable = false;

    undo_redo_t item = m_redoList.front();
    item.redoCallback(item.redoData);
    m_undoList.push_front(item);
    m_redoList.pop_front();

    //actionMainUndo->setText(QTStr("Undo.Item.Undo").arg(item.name));
    m_actionMainUndo->setEnabled(true);

    if(m_redoList.empty()) {
        m_actionMainRedo->setDisabled(true);
        //actionMainRedo->setText(QTStr("Undo.Redo"));
    } else {
        //actionMainRedo->setText(QTStr("Undo.Item.Redo").arg(m_redoList.front().name));
    }
}
//

void AFUndoStack::qSlotResetRepeatableState()
{
    m_lastIsRepeatable = false;
}