#include "CSource.h"

#if defined(_WIN32)
    #include <Windows.h>
#elif defined(__APPLE__)
    #include <CoreFoundation/CoreFoundation.h>
    #include <CoreText/CoreText.h>
#endif


#include "qt-wrapper.h"
#include "util/dstr.h"
#include "graphics/matrix4.h"

//
#include "Common/MathMiscUtils.h"

// CoreModel
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Scene/CSceneContext.h"

// for UI
#include "UIComponent/CMessageBox.h"
#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"
#include "Blocks/SceneSourceDock/CSourceListView.h"


// inline char* get_new_source_name(const char* name, const char* format)
char* get_new_source_name_temp(const char* name, const char* format)
{
	struct dstr new_name = { 0 };
	int inc = 0;

	dstr_copy(&new_name, name);

	for (;;) {
		OBSSourceAutoRelease existing_source =
			obs_get_source_by_name(new_name.array);
		if (!existing_source)
			break;

		dstr_printf(&new_name, format, name, ++inc + 1);
	}

	return new_name.array;
}

#if defined(_WIN32)
static std::string WideCharToUTF8(const wchar_t* wideStr)
{
	int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
	if (utf8Length == 0)
		return "";
	

	std::string utf8Str(utf8Length, 0);
	WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &utf8Str[0], utf8Length, nullptr, nullptr);

	return utf8Str;
}
#endif

obs_source_t* AFSourceUtil::CreateLabelSource(LabelSourceData& refData)
{
	OBSDataAutoRelease settings = obs_data_create();
	OBSDataAutoRelease font = obs_data_create();

    char fontName[1024] = {0,};
#if defined(_WIN32)
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(NONCLIENTMETRICS);

	if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0))
	{
		std::string defaultFontName = WideCharToUTF8(ncm.lfMessageFont.lfFaceName);
		obs_data_set_string(font, "face", defaultFontName.c_str());
	}
	else
		obs_data_set_string(font, "face", "Arial");

#elif defined(__APPLE__)
    char lang[256] = {0,};
    
    CTFontRef systemFont = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, 0.0, NULL);
    CFArrayRef sysLocaleIDs = CFLocaleCopyPreferredLanguages();
    CFStringRef sysLocaleID = (CFStringRef)CFArrayGetValueAtIndex(sysLocaleIDs, 0);

    CFStringRef testLangString = CFStringCreateWithCString(kCFAllocatorDefault,
                                                           AFLocaleTextManager::GetSingletonInstance().Str("Basic.Scene"),
                                                           kCFStringEncodingUTF8);
    CTFontRef systemFontLang = CTFontCreateForStringWithLanguage(systemFont, testLangString,
                                                                 CFRangeMake(0, CFStringGetLength(testLangString)),
                                                                 sysLocaleID);
    CFStringRef familyNameLang = CTFontCopyFullName(systemFontLang);
    
    
    if (familyNameLang)
    {
        CFStringGetCString(familyNameLang, fontName, sizeof(fontName), kCFStringEncodingUTF8);
        CFRelease(familyNameLang);
        
        obs_data_set_string(font, "face", fontName);
    }
    else
        obs_data_set_string(font, "face", "Helvetica");
    
    
    if (sysLocaleIDs)
        CFRelease(sysLocaleIDs);
    
    if (systemFontLang)
        CFRelease(systemFontLang);
    
    if (systemFont)
        CFRelease(systemFont);
#else
	obs_data_set_string(font, "face", "Monospace");
#endif
    
    if (refData.labelFontFlag != 1)  // default == Bold text
        obs_data_set_int(font, "flags", refData.labelFontFlag);
    else
        obs_data_set_int(font, "flags", 1);

	int size = 0;
	if (IsNearlyZero(refData.labelRatioSize))
		size = refData.labelSize;
	else
		size = int((float)refData.labelSize * refData.labelRatioSize);

	obs_data_set_int(font, "size", size);

	obs_data_set_obj(settings, "font", font);

	if (refData.labelColor1 != 0)
		obs_data_set_int(settings, "color", refData.labelColor1);

	if (refData.labelColor1 != 0)
		obs_data_set_int(settings, "color1", refData.labelColor1);
	
    if (refData.labelColor2 != 0)
        obs_data_set_int(settings, "color2", refData.labelColor2);

	if (refData.labelSetText) {
        QString fontName = obs_data_get_string(font, "face");
        QFont tempFont(fontName);
        QFontMetrics fontMetrics(tempFont);

        QString elidedQString = fontMetrics.elidedText(
            QString(refData.labelText), Qt::ElideRight, refData.labelSize);
        std::string elidedStdString = elidedQString.toStdString();
        const char* elidedChar = elidedStdString.c_str();
        obs_data_set_string(settings, "text", elidedChar);
		/* ---- end ---- */
    }

    obs_data_set_bool(settings, "outline", refData.labelOutline);
    
#ifdef _WIN32
	if (refData.labelOutline)
	{
		obs_data_set_int(settings, "outline_color", refData.labelOulineHexColor);

		int outLineSize = 0;
		if (IsNearlyZero(refData.labelRatioSize))
			outLineSize = refData.labelOulineSize;
		else
			outLineSize = int((float)refData.labelOulineSize * 
							  refData.labelRatioOulineSize);

		obs_data_set_int(settings, "outline_size", outLineSize);
	}
#endif


#ifdef _WIN32
	const char* text_source_id = "text_gdiplus";
#else
	const char* text_source_id = "text_ft2_source";
#endif

	return obs_source_create_private(text_source_id, NULL, settings);
};


const char* AFSourceUtil::GetSourceDisplayName(const char* id)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	if (strcmp(id, "scene") == 0)
		return locale.Str("Basic.Scene");
	else if (strcmp(id, "group") == 0)
		return locale.Str("Group");
	const char* v_id = obs_get_latest_input_type_id(id);
	return obs_source_get_display_name(v_id);
}


QString AFSourceUtil::GetPlaceHodlerText(const char* id)
{
	QString placeHolderText{ QT_UTF8(GetSourceDisplayName(id)) };
	QString text{ placeHolderText };

	int i = 2;
	OBSSourceAutoRelease source = nullptr;
	while ((source = obs_get_source_by_name(QT_TO_UTF8(text)))) {
		text = QString("%1 %2").arg(placeHolderText).arg(i++);
	}

	return text;
}

bool AFSourceUtil::AddNewSource(QWidget* parent, const char* id,
							    const char* name, const bool visible,
							    OBSSource& newSource, obs_transform_info* transform,
								bool useUndoRedo)
{
	bool success = false;

	OBSScene scene = AFSceneContext::GetSingletonInstance().GetCurrOBSScene();
	if (!scene)
		return false;

	OBSSourceAutoRelease source = obs_get_source_by_name(name);
	if (source && parent) {

		AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

		AFCMessageBox::information(parent, locale.Str("NameExists.Title"),
										   locale.Str("NameExists.Text"));
	}
	else {
		const char* v_id = obs_get_latest_input_type_id(id);
		source = obs_source_create(v_id, name, NULL, nullptr);

		if (source) {
			AddSourceData data;
			data.source = source;
			data.visible = visible;
			data.transform = transform;

			obs_enter_graphics();
			obs_scene_atomic_update(scene, AFSceneContext::AddSource, &data);
			obs_leave_graphics();

			newSource = source;

			if(useUndoRedo)
			{
                /* set monitoring if source monitors by default */
                AFMainFrame* main = App()->GetMainView();
                AFMainDynamicComposit* mainComposit = main->GetMainWindow();
                std::string source_name = obs_source_get_name(AFSourceUtil::GetCurrentSource());
                auto undo = [source_name, mainComposit](const std::string& data) {
                    OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
                    obs_source_remove(source);

                    OBSSourceAutoRelease scene_source = obs_get_source_by_name(source_name.c_str());
                    mainComposit->SetCurrentScene(scene_source.Get(), true);
                };
                OBSDataAutoRelease wrapper = obs_data_create();
                obs_data_set_string(wrapper, "id", id);
                OBSSceneItemAutoRelease item = obs_scene_sceneitem_from_source(AFSourceUtil::GetCurrentScene(), newSource);
                obs_data_set_int(wrapper, "item_id", obs_sceneitem_get_id(item));
                obs_data_set_string(wrapper, "name", source_name.c_str());
                obs_data_set_bool(wrapper, "visible", visible);

                auto redo = [source_name, mainComposit](const std::string& data) {
                    OBSSourceAutoRelease scene_source = obs_get_source_by_name(source_name.c_str());
                    mainComposit->SetCurrentScene(scene_source.Get(), true);

                    OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
                    OBSSource source;
                    AddNewSource(NULL, obs_data_get_string(dat, "id"),
                                 obs_data_get_string(dat, "name"),
                                 obs_data_get_bool(dat, "visible"), source, nullptr, false);
                    OBSSceneItemAutoRelease item = obs_scene_sceneitem_from_source(AFSourceUtil::GetCurrentScene(), source);
                    obs_sceneitem_set_id(item, (int64_t)obs_data_get_int(dat, "item_id"));
                };
                main->m_undo_s.AddAction(QTStr("Undo.Add").arg(source_name.c_str()),
                                         undo, redo,
                                         std::string(obs_source_get_name(newSource)),
                                         std::string(obs_data_get_json(wrapper)));
			}

			success = true;
		}
	}

	return success;
}

void AFSourceUtil::AddExistingSource(const char* name, bool visible, bool duplicate,
									 obs_transform_info* transform, obs_sceneitem_crop* crop,
									 obs_blending_method* blend_method,
									 obs_blending_type* blend_mode)
{
	OBSSourceAutoRelease source = obs_get_source_by_name(name);
	if (source) {
		AddExistingSource(source.Get(), visible, duplicate, transform, crop,
						  blend_method, blend_mode);
	}
}

void AFSourceUtil::AddExistingSource(OBSSource source, bool visible, bool duplicate,
									 obs_transform_info* transform, obs_sceneitem_crop* crop,
									 obs_blending_method* blend_method,
									 obs_blending_type* blend_mode)
{
	OBSScene scene = AFSceneContext::GetSingletonInstance().GetCurrOBSScene();
	if (!scene)
		return;

	if (duplicate) {
		OBSSource from = source;
		char* new_name = get_new_source_name_temp(
			obs_source_get_name(source), "%s %d");
		source = obs_source_duplicate(from, new_name, false);
		obs_source_release(source);
		bfree(new_name);

		if (!source)
			return;
	}

	AddSourceData data;
	data.source = source;
	data.visible = visible;
	data.transform = transform;
	data.crop = crop;
	data.blend_method = blend_method;
	data.blend_mode = blend_mode;

	obs_enter_graphics();
	obs_scene_atomic_update(scene, AFSceneContext::AddSource, &data);
	obs_leave_graphics();
}

bool AFSourceUtil::QueryRemoveSource(obs_source_t* source)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

	if (obs_source_get_type(source) == OBS_SOURCE_TYPE_SCENE &&
		!obs_source_is_group(source)) {

		int count = sceneContext.GetSceneItemSize();

		if (count == 1) {
			int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, 
													App()->GetMainView(),
													locale.Str("FinalScene.Title"), 
													locale.Str("FinalScene.Text"),
													true, true);
			return false;
		}
	}

	const char* name = obs_source_get_name(source);
	QString text = locale.Str("ConfirmRemove.Text");
	text = text.arg(QT_UTF8(name));

	int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
											App()->GetMainView(),
											locale.Str("ConfirmRemove.Title"), 
											text,
											true, true);

	return (result == QDialog::Accepted);

}

bool AFSourceUtil::ShouldShowContextPopupFromProps(obs_source_t* source)
{
	if (!source)
		return false;

	const char* id = obs_source_get_id(source);

	if (0 == strcmp(id, "ffmpeg_source")) {
		OBSDataAutoRelease s = obs_source_get_settings(source);
		bool is_local_file = obs_data_get_bool(s, "is_local_file");
		if (!is_local_file)
			return false;
		else
			return true;
	}

	if (0 == strcmp(id, "browser_source"))
		return true;

	if (0 == strcmp(id, "slideshow"))
		return true;

	return false;
}

bool AFSourceUtil::ShouldShowContextPopup(obs_source_t* source)
{
	if (!source)
		return false;

	const char* id = obs_source_get_id(source);

	if (0 == strcmp(id,"ffmpeg_source"))
		return true;

	if (0 == strcmp(id, "slideshow"))
		return true;

	return false;
}

bool AFSourceUtil::ShouldShowProperties(obs_source_t* source)
{
	if (!source)
		return false;

	const char* sourceId = obs_source_get_id(source);

	if (strcmp(sourceId, "group") == 0)
		return false;

	if (!obs_source_configurable(source))
		return false;

	uint32_t caps = obs_source_get_output_flags(source);
	if ((caps & OBS_SOURCE_CAP_DONT_SHOW_PROPERTIES) != 0)
		return false;

	return true;
}

bool AFSourceUtil::GetSelectedSourceItemOne(obs_scene_t* scene,
											obs_sceneitem_t* item,
											void* param)
{
	obs_sceneitem_t* selectedItem = reinterpret_cast<obs_sceneitem_t*>(param);
    if (obs_sceneitem_is_group(item))
            obs_sceneitem_group_enum_items(item, GetSelectedSourceItemOne,
                                           param);

    obs_sceneitem_select(item, (selectedItem == item));

	return true;
}

bool AFSourceUtil::GetSelectedSourceItems(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
	std::vector<OBSSceneItem>& items =
		*reinterpret_cast<std::vector<OBSSceneItem> *>(param);

	if (obs_sceneitem_selected(item)) {
		items.emplace_back(item);
	}
	else if (obs_sceneitem_is_group(item)) {
		obs_sceneitem_group_enum_items(item, GetSelectedSourceItems, &items);
	}
	return true;
}


void AFSourceUtil::SelectedItemOne(obs_scene_t* scene, void* param) 
{
    obs_scene_enum_items(scene, GetSelectedSourceItemOne, param);
}

void AFSourceUtil::RemoveSourceItems(OBSScene scene)
{
	AFLocaleTextManager& locale = AFLocaleTextManager::GetSingletonInstance();

	std::vector<OBSSceneItem> items;
	obs_source_t* scene_source = obs_scene_get_source(scene);

	obs_scene_enum_items(scene, GetSelectedSourceItems, &items);

	if (!items.size())
		return;

	///* ------------------------------------- */
	///* confirm action with user              */

	bool confirmed = false;

	if (items.size() > 1) {
		QString text = locale.Str("ConfirmRemove.TextMultiple");
		text = text.arg(QString::number(items.size()));

		int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
												App()->GetMainView(),
												QT_UTF8(""), text, 
												true, true);
		if (result == QDialog::Accepted)
			confirmed = true;
	}
	else {
		OBSSceneItem& item = items[0];
		obs_source_t* source = obs_sceneitem_get_source(item);
		if (source && QueryRemoveSource(source))
			confirmed = true;
	}

	if (!confirmed)
		return;

	AFMainFrame* main = App()->GetMainView();
	/* ----------------------------------------------- */
	/* save undo data                                  */
	OBSData undo_data = main->BackupScene(scene_source);


	/* ----------------------------------------------- */
	/* remove items                                    */
	for (auto& item : items) {
		obs_source_t* source = obs_sceneitem_get_source(item);
		//obs_source_remove(source);
		obs_sceneitem_remove(item);
	}

	/* ----------------------------------------------- */
	/* save redo data                                  */

	OBSData redo_data = main->BackupScene(scene_source);

	/* ----------------------------------------------- */
	/* add undo/redo action                            */

	QString action_name;
	if (items.size() > 1) {
		action_name = QTStr("Undo.Sources.Multi").arg(QString::number(items.size()));
	}
	else {
		QString str = QTStr("Undo.Delete");
		action_name = str.arg(obs_source_get_name(obs_sceneitem_get_source(items[0])));
	}
	main->CreateSceneUndoRedoAction(action_name, undo_data, redo_data);

}

void AFSourceUtil::SourcePaste(SourceCopyInfo& info, bool dup)
{
	OBSSource source = OBSGetStrongRef(info.weak_source);
	if (!source)
		return;

	AddExistingSource(source, info.visible, dup, &info.transform, &info.crop,
		&info.blend_method, &info.blend_mode);
}

static void GetItemBox(obs_sceneitem_t* item, vec3& tl, vec3& br)
{
	matrix4 boxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);

	vec3_set(&tl, M_INFINITE, M_INFINITE, 0.0f);
	vec3_set(&br, -M_INFINITE, -M_INFINITE, 0.0f);

	auto GetMinPos = [&](float x, float y) {
		vec3 pos;
		vec3_set(&pos, x, y, 0.0f);
		vec3_transform(&pos, &pos, &boxTransform);
		vec3_min(&tl, &tl, &pos);
		vec3_max(&br, &br, &pos);
	};

	GetMinPos(0.0f, 0.0f);
	GetMinPos(1.0f, 0.0f);
	GetMinPos(0.0f, 1.0f);
	GetMinPos(1.0f, 1.0f);
}

static vec3 GetItemTL(obs_sceneitem_t* item)
{
	vec3 tl, br;
	GetItemBox(item, tl, br);
	return tl;
}

static void SetItemTL(obs_sceneitem_t* item, const vec3& tl)
{
	vec3 newTL;
	vec2 pos;

	obs_sceneitem_get_pos(item, &pos);
	newTL = GetItemTL(item);
	pos.x += tl.x - newTL.x;
	pos.y += tl.y - newTL.y;
	obs_sceneitem_set_pos(item, &pos);
}

void AFSourceUtil::RotateSourceFromMenu(float degree_)
{
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), AFSourceUtil::RotateSelectedSources, &degree_);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));

	AFMainFrame* main = App()->GetMainView();
	main->m_undo_s.AddAction(QTStr("Undo.Transform.Rotate").arg(obs_source_get_name(GetCurrentSource())),
							 undo_redo, undo_redo, undo_data, redo_data);
}

static bool MultiplySelectedItemScale(obs_scene_t* /* scene */,
	obs_sceneitem_t* item, void* param)
{
	vec2& mul = *reinterpret_cast<vec2*>(param);

	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, MultiplySelectedItemScale,
			param);
	if (!obs_sceneitem_selected(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	vec3 tl = GetItemTL(item);

	vec2 scale;
	obs_sceneitem_get_scale(item, &scale);
	vec2_mul(&scale, &scale, &mul);
	obs_sceneitem_set_scale(item, &scale);

	obs_sceneitem_force_update_transform(item);

	SetItemTL(item, tl);

	return true;
}

void AFSourceUtil::FlipSourceFromMenu(float x, float y)
{
	QString name;
	if(x < 0)
		name = QTStr("Undo.Transform.HFlip");
	else if(y < 0)
		name = QTStr("Undo.Transform.VFlip");
	else
		return;
	//
	vec2 scale;
	vec2_set(&scale, x, y);

	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), MultiplySelectedItemScale, &scale);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	AFMainFrame* main = App()->GetMainView();
	main->m_undo_s.AddAction(name.arg(obs_source_get_name(GetCurrentSource())),
							 undo_redo, undo_redo, undo_data, redo_data);
}

bool AFSourceUtil::RotateSelectedSources(obs_scene_t* /* scene */,
									     obs_sceneitem_t* item, void* param)
{
	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, RotateSelectedSources,
			param);
	if (!obs_sceneitem_selected(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	float rot = *reinterpret_cast<float*>(param);

	vec3 tl = GetItemTL(item);

	rot += obs_sceneitem_get_rot(item);
	if (rot >= 360.0f)
		rot -= 360.0f;
	else if (rot <= -360.0f)
		rot += 360.0f;
	obs_sceneitem_set_rot(item, rot);

	obs_sceneitem_force_update_transform(item);

	SetItemTL(item, tl);

	return true;
}

static bool CenterAlignSelectedItems(obs_scene_t* /* scene */,
	obs_sceneitem_t* item, void* param)
{
	obs_bounds_type boundsType =
		*reinterpret_cast<obs_bounds_type*>(param);

	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, CenterAlignSelectedItems,
			param);
	if (!obs_sceneitem_selected(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	obs_transform_info itemInfo;
	vec2_set(&itemInfo.pos, 0.0f, 0.0f);
	vec2_set(&itemInfo.scale, 1.0f, 1.0f);
	itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
	itemInfo.rot = 0.0f;

	vec2_set(&itemInfo.bounds, float(ovi.base_width),
		float(ovi.base_height));
	itemInfo.bounds_type = boundsType;
	itemInfo.bounds_alignment = OBS_ALIGN_CENTER;

	obs_sceneitem_set_info(item, &itemInfo);

	return true;
}

void AFSourceUtil::FitSourceToScreenFromMenu(obs_bounds_type boundsType)
{
	QString name;
	if(obs_bounds_type::OBS_BOUNDS_SCALE_INNER == boundsType)
		name = QTStr("Undo.Transform.FitToScreen");
	else if(obs_bounds_type::OBS_BOUNDS_STRETCH == boundsType)
		name = QTStr("Undo.Transform.StretchToScreen");
	else
		return;
	//
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), CenterAlignSelectedItems, &boundsType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	AFMainFrame* main = App()->GetMainView();
	main->m_undo_s.AddAction(name.arg(obs_source_get_name(GetCurrentSource())),
							 undo_redo, undo_redo, undo_data, redo_data);
}

void CenterSelectedSceneItems(const CenterType& centerType)
{
	AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();
	AFQSourceListView* sourceListView = sceneContext.GetSourceListViewPtr();

	std::vector<OBSSceneItem> items;

	for (auto& selectedSource : sourceListView->selectionModel()->selectedIndexes()) {
		OBSSceneItem item = sourceListView->Get(selectedSource.row());
		if (!item)
			continue;

		obs_transform_info oti;
		obs_sceneitem_get_info(item, &oti);

		obs_source_t* source = obs_sceneitem_get_source(item);
		float width = float(obs_source_get_width(source)) * oti.scale.x;
		float height =
			float(obs_source_get_height(source)) * oti.scale.y;

		if (width == 0.0f || height == 0.0f)
			continue;

		items.emplace_back(item);

	}

	if (!items.size())
		return;

	// Get center x, y coordinates of items
	vec3 center;

	float top = M_INFINITE;
	float left = M_INFINITE;
	float right = 0.0f;
	float bottom = 0.0f;

	for (auto& item : items) {
		vec3 tl, br;

		GetItemBox(item, tl, br);

		left = (std::min)(tl.x, left);
		top = (std::min)(tl.y, top);
		right = (std::max)(br.x, right);
		bottom = (std::max)(br.y, bottom);
	}

	center.x = (right + left) / 2.0f;
	center.y = (top + bottom) / 2.0f;
	center.z = 0.0f;

	// Get coordinates of screen center
	obs_video_info ovi;
	obs_get_video_info(&ovi);

	vec3 screenCenter;
	vec3_set(&screenCenter, float(ovi.base_width), float(ovi.base_height),
		0.0f);

	vec3_mulf(&screenCenter, &screenCenter, 0.5f);

	// Calculate difference between screen center and item center
	vec3 offset;
	vec3_sub(&offset, &screenCenter, &center);

	// Shift items by offset
	for (auto& item : items) {
		vec3 tl, br;

		GetItemBox(item, tl, br);

		vec3_add(&tl, &tl, &offset);

		vec3 itemTL = GetItemTL(item);

		if (centerType == CenterType::Vertical)
			tl.x = itemTL.x;
		else if (centerType == CenterType::Horizontal)
			tl.y = itemTL.y;

		SetItemTL(item, tl);
	}
}

void AFSourceUtil::SetCenterToScreenFromMenu(CenterType centerType)
{
	QString name;
	if(CenterType::Scene == centerType)
		name = QTStr("Undo.Transform.Center");
	else if(CenterType::Vertical == centerType)
		name = QTStr("Undo.Transform.VCenter");
	else if(CenterType::Horizontal == centerType)
		name = QTStr("Undo.Transform.HCenter");
	else
		return;
	//
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	CenterSelectedSceneItems(centerType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	AFMainFrame* main = App()->GetMainView();
	main->m_undo_s.AddAction(name.arg(obs_source_get_name(GetCurrentSource())),
							 undo_redo, undo_redo, undo_data, redo_data);
}

OBSScene AFSourceUtil::GetCurrentScene()
{
	return AFSceneContext::GetSingletonInstance().GetCurrOBSScene();
}
OBSSource AFSourceUtil::GetCurrentSource()
{
	return OBSSource(obs_scene_get_source(GetCurrentScene()));
}