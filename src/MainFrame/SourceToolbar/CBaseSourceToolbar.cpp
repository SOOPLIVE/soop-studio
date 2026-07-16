#include "CBaseSourceToolbar.h"

#include <QPushButton>

#include "qt-wrappers.hpp"
#include <Application/CApplication.h>

#include "CoreModel/Source/CSource.h"

#include "MainFrame/CMainFrame.h"

CBaseSourceToolbar::CBaseSourceToolbar(QWidget* parent, OBSSource source, bool not_load_props) :
	QWidget(parent),
	m_weakSource(OBSGetWeakRef(source)),
	props(nullptr, nullptr)
{
	if(!not_load_props)
		props = properties_t(obs_source_properties(source), obs_properties_destroy);
}

CBaseSourceToolbar::~CBaseSourceToolbar()
{
}

void CBaseSourceToolbar::UpdateSourceComboToolbarProperties(QComboBox* combo,
															OBSSource source,
															obs_properties_t* props, 
															const char* prop_name, 
															bool is_int)
{
	std::string cur_id;

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	if (is_int) {
		cur_id = std::to_string(obs_data_get_int(settings, prop_name));
	}
	else {
		cur_id = obs_data_get_string(settings, prop_name);
	}

	combo->blockSignals(true);

	obs_property_t* p = obs_properties_get(props, prop_name);
	int cur_idx = FillPropertyCombo(combo, p, cur_id, is_int);

	//if (cur_idx == -1 || obs_property_list_item_disabled(p, cur_idx)) {
		if (cur_idx == -1) {
			//combo->insertItem(
			//	0,
			//	QTStr("Basic.Settings.Audio.UnknownAudioDevice"));
			cur_idx = 0;
		}

		//SetComboItemEnabled(combo, cur_idx, false);
	//}

	combo->setCurrentIndex(cur_idx);
	combo->blockSignals(false);
}

void CBaseSourceToolbar::UpdateSourceComboToolbarValue(QComboBox* combo,
													   OBSSource source,
													   int idx,
													   const char* prop_name,
													   bool is_int)
{
	QString id = combo->itemData(idx).toString();

	OBSDataAutoRelease settings = obs_data_create();
	if (is_int) {
		obs_data_set_int(settings, prop_name, id.toInt());
	}
	else {
		obs_data_set_string(settings, prop_name, QT_TO_UTF8(id));
	}
	obs_source_update(source, settings);
}

int CBaseSourceToolbar::FillPropertyCombo(QComboBox* c,
										  obs_property_t* p, 
										  const std::string& cur_id, 
										  bool is_int)
{
	size_t count = obs_property_list_item_count(p);
	int cur_idx = -1;

	for (size_t i = 0; i < count; i++) {
		const char* name = obs_property_list_item_name(p, i);
		std::string id;

		if (is_int) {
			id = std::to_string(obs_property_list_item_int(p, i));
		}
		else {
			const char* val = obs_property_list_item_string(p, i);
			id = val ? val : "";
		}

		if (cur_id == id)
			cur_idx = (int)i;

		c->addItem(name, id.c_str());
	}

	return cur_idx;
}

void CBaseSourceToolbar::SaveOldProperties(obs_source_t* source)
{
	oldData = obs_data_create();

	OBSDataAutoRelease oldSettings = obs_source_get_settings(source);
	obs_data_apply(oldData, oldSettings);
	obs_data_set_string(oldData, "undo_suuid", obs_source_get_uuid(source));
}

void CBaseSourceToolbar::SetUndoProperties(obs_source_t* source, bool repeatable)
{
	if (!oldData) {
		blog(LOG_ERROR, "%s: somehow oldData was null.", __FUNCTION__);
		return;
	}

	OBSSource currentSceneSource = SCENE_CONTEXT.GetCurrentSceneSource();
	if (!currentSceneSource)
		return;

	std::string scene_uuid = obs_source_get_uuid(currentSceneSource);
	auto main = MAINFRAME;
	auto undo_redo = [scene_uuid = std::move(scene_uuid), main](const std::string& data) {
		OBSDataAutoRelease settings = obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(settings, "undo_suuid"));
		obs_source_reset_settings(source, settings);

		OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());
		main->SetCurrentScene(scene_source.Get(), true);
		main->UpdateContextToolBarDeferred();
	};

	OBSDataAutoRelease new_settings = obs_data_create();
	OBSDataAutoRelease curr_settings = obs_source_get_settings(source);
	obs_data_apply(new_settings, curr_settings);
	obs_data_set_string(new_settings, "undo_suuid", obs_source_get_uuid(source));

	std::string undo_data(obs_data_get_json(oldData));
	std::string redo_data(obs_data_get_json(new_settings));

	if (undo_data.compare(redo_data) != 0)
		UNDO_STACK.AddAction(QTStr("Undo.Properties").arg(obs_source_get_name(source)),
							 undo_redo, undo_redo, undo_data, redo_data, repeatable);

	oldData = nullptr;
}