#include "CUndoStack.h"

#include <util/util.hpp>
#include "Application/CApplication.h"

#include "MainFrame/CMainFrame.h"

#define MAX_STACK_SIZE  (5000)

//
void AFUndoStack::EnableInternal()
{
    m_lastIsRepeatable = false;

    m_pActionMainUndo->setDisabled(false);
    if(!m_redoList.empty()) {
        m_pActionMainRedo->setDisabled(false);
    }
}
void AFUndoStack::DisableInternal()
{
    m_lastIsRepeatable = false;
    m_pActionMainUndo->setDisabled(true);
    m_pActionMainRedo->setDisabled(true);
}
void AFUndoStack::ClearRedo()
{
    m_redoList.clear();
}
//

AFUndoStack::AFUndoStack(AFMainFrame* main)
{
    m_pActionMainUndo = new QAction(main);
    m_pActionMainRedo = new QAction(main);
    //
    QObject::connect(&m_repeatResetTimer, &QTimer::timeout, this,
                     &AFUndoStack::qslotResetRepeatableState);
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

    //m_pActionMainUndo->setText(QTStr("Undo.Undo"));
    //m_pActionMainRedo->setText(QTStr("Undo.Redo"));

    m_pActionMainUndo->setDisabled(true);
    m_pActionMainRedo->setDisabled(true);
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

    //m_pActionMainUndo->setText(QTStr("Undo.Item.Undo").arg(name));
    m_pActionMainUndo->setEnabled(true);

    //m_pActionMainRedo->setText(QTStr("Undo.Item.Redo"));
    m_pActionMainRedo->setEnabled(true);
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

    //m_pActionMainRedo->setText(QTStr("Undo.Item.Redo").arg(item.name));
    m_pActionMainRedo->setEnabled(true);

    if(m_undoList.empty()) {
        m_pActionMainUndo->setDisabled(true);
        //m_pActionMainUndo->setText(QTStr("Undo.Undo"));
    } else {
        //m_pActionMainUndo->setText(QTStr("Undo.Item.Undo").arg(m_undoList.front().name));
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

    //m_pActionMainUndo->setText(QTStr("Undo.Item.Undo").arg(item.name));
    m_pActionMainUndo->setEnabled(true);

    if(m_redoList.empty()) {
        m_pActionMainRedo->setDisabled(true);
        //m_pActionMainRedo->setText(QTStr("Undo.Redo"));
    } else {
        //m_pActionMainRedo->setText(QTStr("Undo.Item.Redo").arg(m_redoList.front().name));
    }
}
//

void AFUndoStack::AddActionRename(std::string& prevName, std::string& newName, obs_source_t* source)
{
    std::string scene_uuid = obs_source_get_uuid(source);
	auto undo = [scene_uuid, prevName](const std::string& data) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
		obs_source_set_name(source, prevName.c_str());
		};

    auto redo = [newName](const std::string& data) {
        OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
        obs_source_set_name(source, newName.c_str());
        };

    std::string source_uuid(obs_source_get_uuid(source));
    AddAction(QTStr("Undo.Rename").arg(newName.c_str()),
        undo, redo, source_uuid, source_uuid);
}

void AFUndoStack::qslotResetRepeatableState()
{
    m_lastIsRepeatable = false;
}

//
bool save_undo_source_enum(obs_scene_t* /*scene*/, obs_sceneitem_t* item, void* p)
{
    obs_source_t* source = obs_sceneitem_get_source(item);
    if(obs_obj_is_private(source) && !obs_source_removed(source))
        return true;

    obs_data_array_t* array = (obs_data_array_t*)p;

    /* check if the source is already stored in the array */
    const char* name = obs_source_get_name(source);
    const size_t count = obs_data_array_count(array);
    for(size_t i = 0; i < count; i++) {
        OBSDataAutoRelease sourceData = obs_data_array_item(array, i);
        if(strcmp(name, obs_data_get_string(sourceData, "name")) == 0)
            return true;
    }

    if(obs_source_is_group(source))
        obs_scene_enum_items(obs_group_from_source(source), save_undo_source_enum, p);

    OBSDataAutoRelease source_data = obs_save_source(source);
    obs_data_array_push_back(array, source_data);
    return true;
}
void undo_redo(const std::string& data)
{
    OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
    OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "scene_uuid"));
    DYNAMIC_COMPOSIT->SetCurrentScene(source.Get(), true);
    obs_scene_load_transform_states(data.c_str());
}