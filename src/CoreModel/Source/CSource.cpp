#include "CSource.h"

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CoreText.h>
#endif

#include "qt-wrappers.hpp"

#include "util/dstr.h"
#include "graphics/matrix4.h"

//
#include "Common/MathMiscUtils.h"

// CoreModel
#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/OBSOutput/COutput.h"

// for UI
#include "UIComponent/CMessageBox.h"
#include "UIComponent/CMessageAlert.h"
#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"
#include "MainFrame/Output/COutput.h"
#include "Blocks/SceneSourceDock/CSourceListView.h"
#include "Blocks/AudioMixerDock/CVolumeControl.h"

#include "Common/StudioDefine.h"

struct SOURCE_INFO {
    const char* id;
    const char* name;
};

static SOURCE_INFO g_soopSources[] = {

	{ "soop_chat_source_chat",		"" },
	{ "soop_chat_source_notice",	"" },
	{ "soop_chat_source_goal",		"" },
	{ "soop_chat_source_banner",	"" },
	{ "soop_chat_source_subtitle",	"" },
	{ "soop_chat_source_c_mission",	"" },
	{ "soop_chat_source_timer",		"" },
	{ "soop_chat_source_score",		"" },
	{ "soop_chat_source_anmSubtitle", "" },
	{ "soop_chat_source_mood_check",	"" },

	{ "soop_videoballoon_source",	"" },

	{ "soop_kbo_graphic_source_all",		"" },
	{ "soop_kbo_graphic_source_score",		"" },
	{ "soop_kbo_graphic_source_stadium",	"" },
	{ "soop_kbo_graphic_source_player",		"" },
	{ "soop_kbo_graphic_source_livetext",	"" },

    { "soop_football_graphic_source_all",		"" },
    { "soop_football_graphic_source_player",	"" },   
    { "soop_football_graphic_source_change",	"" },
    { "soop_football_graphic_source_score",		"" },
    { "soop_football_graphic_source_livetext",	"" },

	{ "soop_commerce_source_goal",	"" },
	{ "soop_commerce_source_rank",	"" },

    { "soop_mission_source_challenge",	"" },
    { "soop_mission_source_battle_joinusers",	"" },
    { "soop_mission_source_battle_fundingrank",	"" },

    { "soop_anivod_source",			"" },
    { "soop_sportvod_source",		""},
    { "soop_dramavod_source",		"" },
    { "soop_movievod_source",		""},

    { "soop_directbroad_source",	"" },
    { "soop_tv_cable_source",		""},

    { "soop_particle_effect_source", ""},
};

static SOURCE_INFO g_soopMediaSources[] = {
    { "soop_anivod_source",		 "" },
    { "soop_sportvod_source",	 ""},
    { "soop_dramavod_source",	 "" },
    { "soop_movievod_source",	 ""},
    { "soop_directbroad_source", "" },
    { "soop_tv_cable_source",	 ""},
};

static SOURCE_INFO g_soopVodSources[] = {
    { "soop_anivod_source",		"" },
    { "soop_sportvod_source",	""},
    { "soop_dramavod_source",	"" },
    { "soop_movievod_source",	""},
};

static SOURCE_INFO g_mustTopSources[] = {
    { "soop_anivod_source",		"" },
    { "soop_sportvod_source",	""},
    { "soop_dramavod_source",	"" },
    { "soop_movievod_source",	""},
    { "soop_tv_cable_source",	""},
};

static SOURCE_INFO g_soopKBOSources[] = {
    { "soop_kbo_graphic_source_all",		"" },
    { "soop_kbo_graphic_source_score",		"" },
    { "soop_kbo_graphic_source_stadium",	"" },
    { "soop_kbo_graphic_source_player",		"" },
    { "soop_kbo_graphic_source_livetext",	"" },
};

static SOURCE_INFO g_soopFootballSources[] = {
    { "soop_football_graphic_source_all",		"" },
    { "soop_football_graphic_source_player",	"" },
    { "soop_football_graphic_source_change",	"" },
    { "soop_football_graphic_source_score",		"" },
    { "soop_football_graphic_source_livetext",	"" },
};

static SOURCE_INFO g_browserSizeStretch[] = {
	{ "browser_source",		"" },
	{ "soop_chat_source_chat",		"" },
	{ "soop_chat_source_notice",	"" },
	{ "soop_chat_source_goal",		"" },
	{ "soop_chat_source_banner",	"" },
	{ "soop_chat_source_subtitle",	"" },
	{ "soop_chat_source_c_mission",	"" },
	{ "soop_mission_source_battle_joinusers",	"" },
	{ "soop_mission_source_battle_fundingrank",	"" },
	{ "soop_chat_source_score",	"" },
	{ "soop_commerce_source_goal",	"" },
	{ "soop_commerce_source_rank",	"" },
	{ "soop_chat_source_anmSubtitle", "" },
	{ "soop_chat_source_mood_check",	"" },
};

// inline char* get_new_source_name(const char* name, const char* format)
char* get_new_source_name_temp(const char* name, const char* format)
{
    struct dstr new_name = {0};
    int inc = 0;

    dstr_copy(&new_name, name);

    for(;;) {
        OBSSourceAutoRelease existing_source =
            obs_get_source_by_name(new_name.array);
        if(!existing_source)
            break;

        dstr_printf(&new_name, format, name, ++inc + 1);
    }

    return new_name.array;
}

inline void GetItemBox(obs_sceneitem_t* item, vec3& tl, vec3& br)
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
inline vec3 GetItemTL(obs_sceneitem_t* item)
{
    vec3 tl, br;
    GetItemBox(item, tl, br);
    return tl;
}
inline void SetItemTL(obs_sceneitem_t* item, const vec3& tl)
{
    vec3 newTL;
    vec2 pos;

    obs_sceneitem_get_pos(item, &pos);
    newTL = GetItemTL(item);
    pos.x += tl.x - newTL.x;
    pos.y += tl.y - newTL.y;
    obs_sceneitem_set_pos(item, &pos);
}
inline  bool reset_tr(obs_scene_t* /* scene */, obs_sceneitem_t* item, void*)
{
    if(obs_sceneitem_is_group(item))
        obs_sceneitem_group_enum_items(item, reset_tr, nullptr);
    if(!obs_sceneitem_selected(item))
        return true;
    if(obs_sceneitem_locked(item))
        return true;

    obs_sceneitem_defer_update_begin(item);

    obs_transform_info info;
    vec2_set(&info.pos, 0.0f, 0.0f);
    vec2_set(&info.scale, 1.0f, 1.0f);
    info.rot = 0.0f;
    info.alignment = OBS_ALIGN_TOP | OBS_ALIGN_LEFT;
    info.bounds_type = OBS_BOUNDS_NONE;
    info.bounds_alignment = OBS_ALIGN_CENTER;
    vec2_set(&info.bounds, 0.0f, 0.0f);
    obs_sceneitem_set_info(item, &info);

    obs_sceneitem_crop crop = {};
    obs_sceneitem_set_crop(item, &crop);

    obs_sceneitem_defer_update_end(item);

    return true;
}
inline bool MultiplySelectedItemScale(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param)
{
    vec2& mul = *reinterpret_cast<vec2*>(param);

    if(obs_sceneitem_is_group(item))
        obs_sceneitem_group_enum_items(item, MultiplySelectedItemScale, param);
    if(!obs_sceneitem_selected(item))
        return true;
    if(obs_sceneitem_locked(item))
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

inline bool CenterAlignSelectedItems(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param)
{
    obs_bounds_type boundsType = *reinterpret_cast<obs_bounds_type*>(param);

    if(obs_sceneitem_is_group(item))
        obs_sceneitem_group_enum_items(item, CenterAlignSelectedItems, param);
    if(!obs_sceneitem_selected(item))
        return true;
    if(obs_sceneitem_locked(item))
        return true;

    obs_video_info ovi;
    obs_get_video_info(&ovi);

    obs_transform_info itemInfo;
    vec2_set(&itemInfo.pos, 0.0f, 0.0f);
    vec2_set(&itemInfo.scale, 1.0f, 1.0f);
    itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
    itemInfo.rot = 0.0f;

    obs_source_t* source = obs_sceneitem_get_source(item);
    if (AFSourceUtil::IsSoopMediaSource(source)) {

        OBSDataAutoRelease settings = obs_source_get_settings(source);
        const float originW = obs_data_get_int(settings, "origin_width");
        const float originH = obs_data_get_int(settings, "origin_height");

        if (originW > 0.0f && originH > 0.0f) {

            const float sx = float(ovi.base_width) / originW;
            const float sy = float(ovi.base_height) / originH;
            const float scale = std::min(sx, sy);

            itemInfo.scale.x = scale;
            itemInfo.scale.y = scale;

            boundsType = OBS_BOUNDS_NONE;
        }
    }

    vec2_set(&itemInfo.bounds, float(ovi.base_width), float(ovi.base_height));
    itemInfo.bounds_type = boundsType;
    itemInfo.bounds_alignment = OBS_ALIGN_CENTER;

    obs_sceneitem_set_info(item, &itemInfo);

    return true;
}
inline void CenterSelectedSceneItems(const CenterType& centerType)
{
    AFQSourceListView* sourceListView = SCENE_CONTEXT.GetSourceListViewPtr();

    std::vector<OBSSceneItem> items;

    for(auto& selectedSource : sourceListView->selectionModel()->selectedIndexes()) {
        OBSSceneItem item = sourceListView->Get(selectedSource.row());
        if(!item)
            continue;

        obs_transform_info oti;
        obs_sceneitem_get_info(item, &oti);

        obs_source_t* source = obs_sceneitem_get_source(item);
        float width = float(obs_source_get_width(source)) * oti.scale.x;
        float height = float(obs_source_get_height(source)) * oti.scale.y;

        if(width == 0.0f || height == 0.0f)
            continue;

        items.emplace_back(item);
    }

    if(!items.size())
        return;

    // Get center x, y coordinates of items
    vec3 center;

    float top = M_INFINITE;
    float left = M_INFINITE;
    float right = 0.0f;
    float bottom = 0.0f;

    for(auto& item : items) {
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
    vec3_set(&screenCenter, float(ovi.base_width), float(ovi.base_height), .0f);
    vec3_mulf(&screenCenter, &screenCenter, 0.5f);

    // Calculate difference between screen center and item center
    vec3 offset;
    vec3_sub(&offset, &screenCenter, &center);

    // Shift items by offset
    for(auto& item : items) {
        vec3 tl, br;

        GetItemBox(item, tl, br);

        vec3_add(&tl, &tl, &offset);

        vec3 itemTL = GetItemTL(item);

        if(centerType == CenterType::Vertical)
            tl.x = itemTL.x;
        else if(centerType == CenterType::Horizontal)
            tl.y = itemTL.y;

        SetItemTL(item, tl);
    }
}

inline bool FindSoopVodSource(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
    OBSSceneItem& destItem = *reinterpret_cast<OBSSceneItem*>(param);

    obs_source_t* source = obs_sceneitem_get_source(item);
    const char* id = obs_source_get_id(source);

    bool findSoopSource = false;
    int sourceSize = sizeof(g_soopMediaSources) / sizeof(g_soopMediaSources[0]);
    for(int idx = 0; idx < sourceSize; idx++) {
        if(0 == strcmp(id, g_soopMediaSources[idx].id)) {
            findSoopSource = true;
            break;
        }
    }

    if(findSoopSource) {
        destItem = item;
        return false;
    }

    return true;
};

inline bool FindSoopKBOSource(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
    OBSSceneItem& destItem = *reinterpret_cast<OBSSceneItem*>(param);

    obs_source_t* source = obs_sceneitem_get_source(item);
    const char* id = obs_source_get_id(source);

    bool findSoopSource = false;
    int sourceSize = sizeof(g_soopKBOSources) / sizeof(g_soopKBOSources[0]);
    for(int idx = 0; idx < sourceSize; idx++) {
        if(0 == strcmp(id, g_soopKBOSources[idx].id)) {
            findSoopSource = true;
            break;
        }
    }

    if(findSoopSource) {
        destItem = item;
        return false;
    }

    return true;
};

inline bool FindSoopFootballSource(obs_scene_t*, obs_sceneitem_t* item, void* param)
{
    OBSSceneItem& destItem = *reinterpret_cast<OBSSceneItem*>(param);

    obs_source_t* source = obs_sceneitem_get_source(item);
    const char* id = obs_source_get_id(source);

    bool findSoopSource = false;
    int sourceSize = sizeof(g_soopFootballSources) / sizeof(g_soopFootballSources[0]);
    for (int idx = 0; idx < sourceSize; idx++) {
        if (0 == strcmp(id, g_soopFootballSources[idx].id)) {
            findSoopSource = true;
            break;
        }
    }

    if (findSoopSource) {
        destItem = item;
        return false;
    }

    return true;
};

//
inline OBSDataAutoRelease TransformToData(const obs_transform_info& info)
{
    OBSDataAutoRelease data = obs_data_create();

    obs_data_set_vec2(data, "pos", &info.pos);
    obs_data_set_double(data, "rot", info.rot);
    obs_data_set_vec2(data, "scale", &info.scale);

    obs_data_set_int(data, "alignment", (int)info.alignment);
    obs_data_set_int(data, "bounds_type", (int)info.bounds_type);
    obs_data_set_int(data, "bounds_alignment", (int)info.bounds_alignment);
    obs_data_set_vec2(data, "bounds", &info.bounds);

    return data;
}

inline obs_transform_info DataToTransform(OBSData data)
{
    obs_transform_info info;
    memset(&info, 0, sizeof(info));

    obs_data_get_vec2(data, "pos", &info.pos);
    info.rot = (float)obs_data_get_double(data, "rot");
    obs_data_get_vec2(data, "scale", &info.scale);

    info.alignment = (uint32_t)obs_data_get_int(data, "alignment");
    info.bounds_type = (obs_bounds_type)obs_data_get_int(data, "bounds_type");
    info.bounds_alignment = (uint32_t)obs_data_get_int(data, "bounds_alignment");
    obs_data_get_vec2(data, "bounds", &info.bounds);

    return info;
}

#if defined(_WIN32)
static std::string WideCharToUTF8(const wchar_t* wideStr)
{
    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, nullptr, 0, nullptr, nullptr);
    if(utf8Length == 0)
        return "";


    std::string utf8Str(utf8Length, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, &utf8Str[0], utf8Length, nullptr, nullptr);

    return utf8Str;
}
#endif

namespace AFSourceUtil
{
    OBSScene CnvtToOBSScene(OBSSource source)
    {
        return OBSScene(obs_scene_from_source(source));
    };

    bool RemoveSimpleCallback(void*, obs_source_t* pSrc)
    {
        obs_source_remove(pSrc);
        return true;
    };

    bool GetNameSimpleCallback(void* paramVecStr, obs_source_t* pSrc)
    {
        auto vecNames = static_cast<std::vector<std::string> *>(paramVecStr);
        vecNames->push_back(obs_source_get_name(pSrc));
        return true;
    };

    obs_source_t* CreateLabelSource(LabelSourceData& refData)
    {
        OBSDataAutoRelease settings = obs_data_create();
        OBSDataAutoRelease font = obs_data_create();

#if defined(_WIN32)
        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(NONCLIENTMETRICS);

        if(SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0))
        {
            std::string defaultFontName = WideCharToUTF8(ncm.lfMessageFont.lfFaceName);
            obs_data_set_string(font, "face", defaultFontName.c_str());
        } else
            obs_data_set_string(font, "face", "Arial");

#elif defined(__APPLE__)
        char lang[256] = {0,};

        CTFontRef systemFont = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, 0.0, NULL);
        CFArrayRef sysLocaleIDs = CFLocaleCopyPreferredLanguages();
        CFStringRef sysLocaleID = (CFStringRef)CFArrayGetValueAtIndex(sysLocaleIDs, 0);

        CFStringRef testLangString = CFStringCreateWithCString(kCFAllocatorDefault,
                                                               Str("Basic.Scene"),
                                                               kCFStringEncodingUTF8);
        CTFontRef systemFontLang = CTFontCreateForStringWithLanguage(systemFont, testLangString,
                                                                     CFRangeMake(0, CFStringGetLength(testLangString)),
                                                                     sysLocaleID);
        CFStringRef familyNameLang = CTFontCopyFullName(systemFontLang);


        if(familyNameLang)
        {
            CFStringGetCString(familyNameLang, fontName, sizeof(fontName), kCFStringEncodingUTF8);
            CFRelease(familyNameLang);

            obs_data_set_string(font, "face", fontName);
        } else
            obs_data_set_string(font, "face", "Helvetica");


        if(sysLocaleIDs)
            CFRelease(sysLocaleIDs);

        if(systemFontLang)
            CFRelease(systemFontLang);

        if(systemFont)
            CFRelease(systemFont);
#else
        obs_data_set_string(font, "face", "Monospace");
#endif

        if(refData.labelFontFlag != 1)  // default == Bold text
            obs_data_set_int(font, "flags", refData.labelFontFlag);
        else
            obs_data_set_int(font, "flags", 1);

        int size = 0;
        if(IsNearlyZero(refData.labelRatioSize))
            size = refData.labelSize;
        else
            size = int((float)refData.labelSize * refData.labelRatioSize);

        obs_data_set_int(font, "size", size);

        obs_data_set_obj(settings, "font", font);

        if(refData.labelColor1 != 0)
            obs_data_set_int(settings, "color", refData.labelColor1);

        if(refData.labelColor1 != 0)
            obs_data_set_int(settings, "color1", refData.labelColor1);

        if(refData.labelColor2 != 0)
            obs_data_set_int(settings, "color2", refData.labelColor2);

        if(refData.labelSetText) {

            QString fontName = obs_data_get_string(font, "face");
            QFont tempFont(fontName);
            QFontMetrics fontMetrics(tempFont);

            QString elidedQString = fontMetrics.elidedText(QString(refData.labelText), Qt::ElideRight, refData.labelSize);
            std::string elidedStdString = elidedQString.toStdString();
            const char* elidedChar = elidedStdString.c_str();
            obs_data_set_string(settings, "text", elidedChar);

        }

        obs_data_set_bool(settings, "outline", refData.labelOutline);

#ifdef _WIN32
        if(refData.labelOutline)
        {
            obs_data_set_int(settings, "outline_color", refData.labelOulineHexColor);

            int outLineSize = 0;
            if(IsNearlyZero(refData.labelRatioSize))
                outLineSize = refData.labelOulineSize;
            else
                outLineSize = int((float)refData.labelOulineSize * refData.labelRatioOulineSize);

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


    const char* GetSourceDisplayName(const char* id)
    {
        if(strcmp(id, "scene") == 0)
            return Str("Basic.Scene");
        else if(strcmp(id, "group") == 0)
            return Str("Group");
        const char* v_id = obs_get_latest_input_type_id(id);
        return obs_source_get_display_name(v_id);
    }

    QString GetPlaceHodlerText(const char* id)
    {
        if(strcmp(id, "text_gdiplus_v2") == 0)
            id = const_cast<char*>("text_gdiplus");

        QString placeHolderText {QT_UTF8(GetSourceDisplayName(id))};
        QString text {placeHolderText};

        int i = 2;
        OBSSourceAutoRelease source = nullptr;
        while((source = obs_get_source_by_name(QT_TO_UTF8(text)))) {
            text = QString("%1 %2").arg(placeHolderText).arg(i++);
        }

        return text;
    }

    bool AddSavvySource(const char* id, OBSSource& newSource)
    {
        bool success = false;

        OBSSourceAutoRelease source;
        const char* v_id = obs_get_latest_input_type_id(id);
        source = obs_source_create(v_id, QUuid::createUuid()
                                         .toString(QUuid::WithoutBraces)
                                         .toStdString().c_str(), NULL, nullptr);

        if(source) {
            newSource = source;
            success = true;
        }

        return success;
    }

    bool AddNewSource(QWidget* parent, const char* id,
                      const char* name, const bool visible,
                      OBSSource& newSource, obs_transform_info* transform, obs_data_t* settings, bool addOnProgramMode)

    {
        bool success = false;

        OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
        if(!scene)
            return false;

        OBSSourceAutoRelease source = obs_get_source_by_name(name);
        if(source && parent) {

            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, parent,
                                       Str("NameExists.Title"),
                                       Str("NameExists.Text"));
        } else {
            const char* v_id = obs_get_latest_input_type_id(id);
            source = obs_source_create(v_id, name, settings, nullptr);

            if(source) {
                AddSourceData data;
                data.source = source;
                data.visible = visible;
                data.transform = transform;

                obs_enter_graphics();
                obs_scene_atomic_update(scene, AFSceneContext::AddSource, &data);


                if (obs_frontend_preview_program_mode_active() && addOnProgramMode) {
                    OBSSourceAutoRelease transition = obs_get_output_source(0);
                    if (transition) {
                        OBSSourceAutoRelease activeSource = obs_transition_get_active_source(transition);
                        OBSScene programScene = obs_scene_from_source(activeSource);

                        if (programScene && programScene != scene) {
                            obs_scene_atomic_update(programScene, AFSceneContext::AddSource, &data);
                        }
                    }
                }

                obs_leave_graphics();

                newSource = source;

                success = true;
            }
        }

        return success;
    }

    void AddExistingSource(const char* name, bool visible, bool duplicate,
                           obs_transform_info* transform, obs_sceneitem_crop* crop,
                           obs_blending_method* blend_method,
                           obs_blending_type* blend_mode)
    {
        OBSSourceAutoRelease source = obs_get_source_by_name(name);
        if(source) {
            AddExistingSource(source.Get(), visible, duplicate, transform, crop,
                              blend_method, blend_mode);
        }
    }

    void AddExistingSource(OBSSource source, bool visible, bool duplicate,
                                         obs_transform_info* transform, obs_sceneitem_crop* crop,
                                         obs_blending_method* blend_method,
                                         obs_blending_type* blend_mode)
    {
        OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
        if(!scene)
            return;
        
        OBSSourceAutoRelease duplicatedSource;
        if(duplicate) {
            OBSSource from = source;
            char* new_name = get_new_source_name_temp(obs_source_get_name(from), "%s %d");
            const char* source_id = obs_source_get_id(from);
            if (IsForceDuplicateSource(source_id)) {
                OBSDataAutoRelease fromSettings = obs_source_get_settings(from);
                OBSDataAutoRelease newSettings = obs_data_create();
                obs_data_apply(newSettings, fromSettings);

                duplicatedSource = obs_source_create(source_id, new_name, newSettings, nullptr);

                if (duplicatedSource)
                    obs_source_copy_filters(duplicatedSource, from);
            }
            else {
                duplicatedSource = obs_source_duplicate(from, new_name, false);
            }
            bfree(new_name);

            if (!duplicatedSource)
                return;

            source = duplicatedSource;
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

    bool QueryRemoveSource(obs_source_t* source)
    {
        if(obs_source_get_type(source) == OBS_SOURCE_TYPE_SCENE &&
            !obs_source_is_group(source)) {

            int count = SCENE_CONTEXT.GetSceneItemSize();

            if(count == 1) {
                int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok,
                                                        MAINFRAME,
                                                        Str("FinalScene.Title"),
                                                        Str("FinalScene.Text"),
                                                        true, true);
                return false;
            }
        }

        std::string id = obs_source_get_id(source);
        if (id.compare("soop_aimanager_source") == 0)
        {
            bool checked = false;
            int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                MAINFRAME,
                "",
                QTStr("AIManager.Delete.Info"),
                true, true, QTStr("AIManager.Delete"), 410, 250, QTStr("Remove"), QTStr("AIManager.TurnOff.Auto"), &checked);

            if(checked)
                AUTH_CONTEXT.GetSoopBroadInfo()->SetAIManagerTime(0);

            return (result == QDialog::Accepted);
        }

        const char* name = obs_source_get_name(source);
        QString text = Str("ConfirmRemove.Text");
        text = text.arg(QT_UTF8(name));

        int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                MAINFRAME,
                                                Str("ConfirmRemove.Title"),
                                                text,
                                                true, true);

        return (result == QDialog::Accepted);
    }

    bool ShouldShowContextPopupFromProps(obs_source_t* source)
    {
        if(!source)
            return false;

        const char* id = obs_source_get_id(source);
        if(0 == strcmp(id, "browser_source"))
            return true;

        return false;
    }

    bool ShouldShowContextPopup(obs_source_t* source)
    {
        if(!source)
            return false;

        const char* id = obs_source_get_id(source);

        if(0 == strcmp(id, "ffmpeg_source"))
            return true;

        if(0 == strcmp(id, "ffmpeg_list_source"))
            return true;

        if(0 == strcmp(id, "slideshow"))
            return true;

        return false;
    }

    bool ShouldShowProperties(obs_source_t* source)
    {
        if(!source)
            return false;

        const char* sourceId = obs_source_get_id(source);

        if(strcmp(sourceId, "group") == 0)
            return false;

        if(!obs_source_configurable(source))
            return false;

        uint32_t caps = obs_source_get_output_flags(source);
        if((caps & OBS_SOURCE_CAP_DONT_SHOW_PROPERTIES) != 0)
            return false;

        return true;
    }

    bool GetSelectedSourceItemOne(obs_scene_t* scene, obs_sceneitem_t* item, void* param)
    {
        obs_sceneitem_t* selectedItem = reinterpret_cast<obs_sceneitem_t*>(param);
        if(obs_sceneitem_is_group(item))
            obs_sceneitem_group_enum_items(item, GetSelectedSourceItemOne, param);

        obs_sceneitem_select(item, (selectedItem == item));

        return true;
    }

    bool GetSelectedSourceItems(obs_scene_t*, obs_sceneitem_t* item, void* param)
    {
        std::vector<OBSSceneItem>& items =
            *reinterpret_cast<std::vector<OBSSceneItem> *>(param);

        if(obs_sceneitem_selected(item)) {
            items.emplace_back(item);
        } else if(obs_sceneitem_is_group(item)) {
            obs_sceneitem_group_enum_items(item, GetSelectedSourceItems, &items);
        }
        return true;
    }

    void SelectedItemOne(obs_scene_t* scene, void* param)
    {
        obs_scene_enum_items(scene, GetSelectedSourceItemOne, param);
    }

    void RemoveSourceItems(OBSScene scene)
    {
        std::vector<OBSSceneItem> items;
        obs_source_t* scene_source = obs_scene_get_source(scene);

        obs_scene_enum_items(scene, GetSelectedSourceItems, &items);

        if(!items.size())
            return;

        ///* ------------------------------------- */
        ///* confirm action with user              */

        bool confirmed = false;

        if(items.size() > 1) {
            QString text = Str("ConfirmRemove.TextMultiple");
            text = text.arg(QString::number(items.size()));

            int result = AFQMessageBox::ShowMessage(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                    MAINFRAME,
                                                    QT_UTF8(""), text,
                                                    true, true);
            if(result == QDialog::Accepted)
                confirmed = true;
        } else {
            OBSSceneItem& item = items[0];
            obs_source_t* source = obs_sceneitem_get_source(item);
            if(source && QueryRemoveSource(source))
                confirmed = true;
        }

        if(!confirmed)
            return;

        /* ----------------------------------------------- */
        /* save undo data                                  */
        OBSData undo_data = MAINFRAME->BackupScene(scene_source);


        /* ----------------------------------------------- */
        /* remove items                                    */

        for(auto& item : items) {
            obs_source_t* source = obs_sceneitem_get_source(item);
            bool bVisible = obs_source_showing(source);
            QString src_name = QString("%1").arg(obs_source_get_id(source));		// minsim
            if(0 == src_name.compare("soop_chat_source_mood_check")) {
                int nCnt = config_get_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt");
                if(bVisible)
                    --nCnt;

                config_set_int(USERCONFIG, "SARSA", "UseMinsimCheckCnt", nCnt > 0 ? nCnt : 0);

                if(nCnt <= 0 && AFOutputUtil::IsStreamActive()) {
                    AFQBroadInfo* broadInfo = AUTH_CONTEXT.GetSoopBroadInfo();
                    auto endTime = std::chrono::steady_clock::now();
                    auto startTime = AUTH_CONTEXT.GetMinsimCheckStartTime();
                    if(startTime != std::chrono::steady_clock::time_point {}) {
                        AUTH_CONTEXT.InitMinsimCheckStartTime();
                        AUTH_CONTEXT.SetMinsimCheckThemeInfo("default");
                    }
                }
            }
            obs_sceneitem_remove(item);
        }

        /* ----------------------------------------------- */
        /* save redo data                                  */

        OBSData redo_data = MAINFRAME->BackupScene(scene_source);

        /* ----------------------------------------------- */
        /* add undo/redo action                            */

        QString action_name;
        if(items.size() > 1) {
            action_name = QTStr("Undo.Sources.Multi").arg(QString::number(items.size()));
        } else {
            QString str = QTStr("Undo.Delete");
            action_name = str.arg(obs_source_get_name(obs_sceneitem_get_source(items[0])));
        }
        MAINFRAME->CreateSceneUndoRedoAction(action_name, undo_data, redo_data);

    }

    bool RemoveSingleItemCallback(obs_scene_t* scene, obs_sceneitem_t* item, void* data)
    {
        const char* targetId = static_cast<const char*>(data);
        obs_source_t* source = obs_sceneitem_get_source(item);

        if (source) {
            const char* currentId = obs_source_get_id(source);

            if (currentId && strcmp(currentId, targetId) == 0) {
                AFMainFrame* main = App()->GetMainView();
                obs_source_t* scene_source = obs_scene_get_source(scene);

                OBSData undo_data = main->BackupScene(scene_source);
                obs_sceneitem_remove(item);
                OBSData redo_data = main->BackupScene(scene_source);

                QString action_name = QTStr("Undo.Delete").arg(targetId);
                main->CreateSceneUndoRedoAction(action_name, undo_data, redo_data);

                QMetaObject::invokeMethod(main, [=]() {
                    main->GetMainWindow()->UpdateSourceToolBar(true);
                    }, Qt::QueuedConnection);

                return false;
            }
        }
        return true;
    }

    bool EnumSingleRemoveCallback(void* data, obs_source_t* scene_source)
    {
        obs_scene_t* scene = obs_scene_from_source(scene_source);

        if (scene) {
            obs_scene_enum_items(scene, RemoveSingleItemCallback, data);
        }
        return true;
    }


    void RemoveSingleSourceFromAllScenes(const char* targetId)
    {
        obs_enum_scenes(EnumSingleRemoveCallback, const_cast<char*>(targetId));
    }

    void SourcePaste(SourceCopyInfo& info, bool dup)
    {
        OBSSource source = OBSGetStrongRef(info.weak_source);
        if(!source)
            return;

        AddExistingSource(source, info.visible, dup, &info.transform, &info.crop,
            &info.blend_method, &info.blend_mode);
    }

    void ResetTransform()
    {
        OBSScene curScene = SCENE_CONTEXT.GetCurrentScene();
        OBSDataAutoRelease wrapper = obs_scene_save_transform_states(curScene, false);
        obs_scene_enum_items(curScene, reset_tr, nullptr);
        OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(curScene, false);

        std::string undo_data(obs_data_get_json(wrapper));
        std::string redo_data(obs_data_get_json(rwrapper));
        OBSSource curSource = SCENE_CONTEXT.GetCurrentSceneSource();
        UNDO_STACK.AddAction(QTStr("Undo.Transform.Reset").arg(obs_source_get_name(curSource)),
                             undo_redo, undo_redo, undo_data, redo_data);

        obs_scene_enum_items(SCENE_CONTEXT.GetCurrentScene(), reset_tr, nullptr);
    }

    void RotateSourceFromMenu(float degree_)
    {
        OBSDataAutoRelease wrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), false);
        obs_scene_enum_items(SCENE_CONTEXT.GetCurrentScene(), RotateSelectedSources, &degree_);
        OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), false);

        std::string undo_data(obs_data_get_json(wrapper));
        std::string redo_data(obs_data_get_json(rwrapper));

        UNDO_STACK.AddAction(QTStr("Undo.Transform.Rotate").arg(obs_source_get_name(SCENE_CONTEXT.GetCurrentSceneSource())),
                             undo_redo, undo_redo, undo_data, redo_data);
    }

    void FlipSourceFromMenu(float x, float y)
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

        OBSDataAutoRelease wrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), false);
        obs_scene_enum_items(SCENE_CONTEXT.GetCurrentScene(), MultiplySelectedItemScale, &scale);
        OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), false);

        std::string undo_data(obs_data_get_json(wrapper));
        std::string redo_data(obs_data_get_json(rwrapper));
        UNDO_STACK.AddAction(name.arg(obs_source_get_name(SCENE_CONTEXT.GetCurrentSceneSource())),
                             undo_redo, undo_redo, undo_data, redo_data);
    }

    bool RotateSelectedSources(obs_scene_t* /* scene */, obs_sceneitem_t* item, void* param)
    {
        if(obs_sceneitem_is_group(item))
            obs_sceneitem_group_enum_items(item, RotateSelectedSources,
                param);
        if(!obs_sceneitem_selected(item))
            return true;
        if(obs_sceneitem_locked(item))
            return true;

        float rot = *reinterpret_cast<float*>(param);

        vec3 tl = GetItemTL(item);

        rot += obs_sceneitem_get_rot(item);
        if(rot >= 360.0f)
            rot -= 360.0f;
        else if(rot <= -360.0f)
            rot += 360.0f;
        obs_sceneitem_set_rot(item, rot);

        obs_sceneitem_force_update_transform(item);

        SetItemTL(item, tl);

        return true;
    }

    void FitSourceToScreenFromMenu(obs_bounds_type boundsType)
    {
        QString name;
        if(obs_bounds_type::OBS_BOUNDS_SCALE_INNER == boundsType)
            name = QTStr("Undo.Transform.FitToScreen");
        else if(obs_bounds_type::OBS_BOUNDS_STRETCH == boundsType)
            name = QTStr("Undo.Transform.StretchToScreen");
        else
            return;
        //
        OBSDataAutoRelease wrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), false);
        obs_scene_enum_items(SCENE_CONTEXT.GetCurrentScene(), CenterAlignSelectedItems, &boundsType);
        OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), false);

        std::string undo_data(obs_data_get_json(wrapper));
        std::string redo_data(obs_data_get_json(rwrapper));
        UNDO_STACK.AddAction(name.arg(obs_source_get_name(SCENE_CONTEXT.GetCurrentSceneSource())),
                             undo_redo, undo_redo, undo_data, redo_data);
    }

    void SetCenterToScreenFromMenu(CenterType centerType)
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
        OBSDataAutoRelease wrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), false);
        CenterSelectedSceneItems(centerType);
        OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(SCENE_CONTEXT.GetCurrentScene(), false);

        std::string undo_data(obs_data_get_json(wrapper));
        std::string redo_data(obs_data_get_json(rwrapper));
        UNDO_STACK.AddAction(name.arg(obs_source_get_name(SCENE_CONTEXT.GetCurrentSceneSource())),
                             undo_redo, undo_redo, undo_data, redo_data);
    }

    void SourceCreated(void* data, calldata_t* params)
    {
        obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");

        if(obs_scene_from_source(source) != NULL) {
            QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data),
                                      "qslotAddSceneFromCallback", WaitConnection(), Q_ARG(OBSSource, OBSSource(source)));
        }
    }

    void SourceRemoved(void* data, calldata_t* params)
    {
        obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");

        if(obs_scene_from_source(source) != NULL)
            QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data),
                                      "qslotRemoveSceneFromCallback", Q_ARG(OBSSource, OBSSource(source)));
    }

    void SourceActivated(void* data, calldata_t* params)
    {
        obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");
        uint32_t flags = obs_source_get_output_flags(source);

        if(flags & OBS_SOURCE_AUDIO) {
            QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data),
                                      "qslotActivateAudioSource", Q_ARG(OBSSource, OBSSource(source)));
        }
    }

    void SourceDeactivated(void* data, calldata_t* params)
    {
        obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");
        uint32_t flags = obs_source_get_output_flags(source);

        if(flags & OBS_SOURCE_AUDIO) {
            QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data),
                                      "qslotDeactivateAudioSource", Q_ARG(OBSSource, OBSSource(source)));
        }
    }

    void SourceAudioActivated(void* data, calldata_t* params)
    {
        obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");

        if(obs_source_active(source))
            QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data),
                                      "qslotActivateAudioSource", Q_ARG(OBSSource, OBSSource(source)));

    }

    void SourceAudioDeactivated(void* data, calldata_t* params)
    {
        obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");
        QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data),
                                  "qslotDeactivateAudioSource", Q_ARG(OBSSource, OBSSource(source)));
    }

    void SourceRenamed(void* data, calldata_t* params)
    {
        obs_source_t* source = (obs_source_t*)calldata_ptr(params, "source");
        const char* newName = calldata_string(params, "new_name");
        const char* prevName = calldata_string(params, "prev_name");

        QMetaObject::invokeMethod(static_cast<AFMainFrame*>(data),
                                  "qslotRenameSources", Q_ARG(OBSSource, source),
                                  Q_ARG(QString, QT_UTF8(newName)),
                                  Q_ARG(QString, QT_UTF8(prevName)));

        blog(LOG_INFO, "Source '%s' renamed to '%s'", prevName, newName);
    }

    bool SourceMixerHidden(obs_source_t* source)
    {
        OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
        bool hidden = obs_data_get_bool(priv_settings, "mixer_hidden");

        return hidden;
    }


    void SetSourceMixerHidden(obs_source_t* source, bool hidden)
    {
        OBSDataAutoRelease priv_settings =
            obs_source_get_private_settings(source);
        obs_data_set_bool(priv_settings, "mixer_hidden", hidden);
    }

    bool SourceVolumeLocked(obs_source_t* source)
    {
        OBSDataAutoRelease priv_settings = obs_source_get_private_settings(source);
        bool lock = obs_data_get_bool(priv_settings, "volume_locked");

        return lock;
    }

    AFQVolControl* CompareAudio(AFQVolControl* audio1, AFQVolControl* audio2)
    {
        if(audio1 == nullptr)
            return audio2;
        if(audio2 == nullptr)
            return audio1;

        // if current volume locked
        if(SourceVolumeLocked(audio1->GetSource())) {
            if(SourceVolumeLocked(audio2->GetSource())) {
                if((audio1->IsMuted() == true && audio2->IsMuted()) == true ||
                    (audio1->IsMuted() == false && audio2->IsMuted() == false))
                {
                    audio1 = (audio1->GetVolume() > audio2->GetVolume()) ? audio1 : audio2;
                } else
                    audio1 = (audio1->IsMuted() == false) ? audio1 : audio2;
            } else
                audio1 = audio2;
        } else
        {
            if(SourceVolumeLocked(audio2->GetSource()))
                return audio1;
            if((audio1->IsMuted() == true && audio2->IsMuted()) == true ||
                (audio1->IsMuted() == false && audio2->IsMuted() == false))
            {
                audio1 = (audio1->GetVolume() > audio2->GetVolume()) ? audio1 : audio2;
            } else
                audio1 = (audio1->IsMuted() == false) ? audio1 : audio2;
        }

        return audio1;
    }

    bool IsSoopSource(const char* id)
    {
        int sourceSize = sizeof(g_soopSources) / sizeof(g_soopSources[0]);
        for(int idx = 0; idx < sourceSize; idx++) {
            if(0 == strcmp(id, g_soopSources[idx].id))
                return true;
        }

        return false;
    }

    bool IsSoopSource(obs_source_t* source)
    {
        if(!source)
            return false;

        const char* id = obs_source_get_id(source);
        return IsSoopSource(id);
    }

    bool IsSoopMediaSource(const char* id, bool exceptDirectBroad)
    {
        int sourceSize = sizeof(g_soopMediaSources) / sizeof(g_soopMediaSources[0]);
        for(int idx = 0; idx < sourceSize; idx++) {

            if(exceptDirectBroad) {
                if(0 == strcmp(id, "soop_directbroad_source"))
                    continue;
            }

            if(0 == strcmp(id, g_soopMediaSources[idx].id))
                return true;
        }

        return false;
    }

    bool IsSoopMediaSource(obs_source_t* source, bool exceptDirectBroad)
    {
        if(!source)
            return false;

        const char* id = obs_source_get_id(source);
        return IsSoopMediaSource(id, exceptDirectBroad);
    }

    bool IsSoopVodSource(const char* id)
    {
        int sourceSize = sizeof(g_soopVodSources) / sizeof(g_soopVodSources[0]);
        for(int idx = 0; idx < sourceSize; idx++) {
            if(0 == strcmp(id, g_soopVodSources[idx].id))
                return true;
        }
        return false;
    }

    bool IsSoopVodSource(obs_source_t* source)
    {
        if(!source)
            return false;

        const char* id = obs_source_get_id(source);
        return IsSoopVodSource(id);
    }

    bool IsMustTopSource(const char* id)
    {
        int sourceSize = sizeof(g_mustTopSources) / sizeof(g_mustTopSources[0]);
        for(int idx = 0; idx < sourceSize; idx++) {
            if(0 == strcmp(id, g_mustTopSources[idx].id))
                return true;
        }

        return false;
    }

    bool IsMustTopSource(obs_source_t* source)
    {
        if(!source)
            return false;

        const char* id = obs_source_get_id(source);
        return IsMustTopSource(id);
    }


    bool IsSoopKBOSource(const char* id)
    {
        int sourceSize = sizeof(g_soopKBOSources) / sizeof(g_soopKBOSources[0]);
        for(int idx = 0; idx < sourceSize; idx++) {
            if(0 == strcmp(id, g_soopKBOSources[idx].id))
                return true;
        }

        return false;
    }

    bool IsSoopKBOSource(obs_source_t* source)
    {
        if(!source)
            return false;

        const char* id = obs_source_get_id(source);
        return IsSoopKBOSource(id);
    }

    bool IsSoopFootballSource(const char* id)
    {
        int sourceSize = sizeof(g_soopFootballSources) / sizeof(g_soopFootballSources[0]);
        for (int idx = 0; idx < sourceSize; idx++) {
            if (0 == strcmp(id, g_soopFootballSources[idx].id))
                return true;
        }

        return false;
    }

    bool IsSoopFootballSource(obs_source_t* source)
    {
        if (!source)
            return false;

        const char* id = obs_source_get_id(source);
        return IsSoopFootballSource(id);
    }

    bool IsBrowserSizeStretch(const char* id)
    {
        int sourceSize = sizeof(g_browserSizeStretch) / sizeof(g_browserSizeStretch[0]);
        for(int idx = 0; idx < sourceSize; idx++) {
            if(0 == strcmp(id, g_browserSizeStretch[idx].id))
                return true;
        }

        return false;
    }

    bool IsForceDuplicateSource(const char* id)
    {
        if (!id)
            return false;

        /* Recreate these sources instead of obs_source_duplicate().
        * dshow_input uses a shared DShowInstance internally, but duplicated OBS
        * sources must remain independent for source-local settings and filters. */
        return	strcmp(id, "ffmpeg_source") == 0 ||
                strcmp(id, "ffmpeg_list_source") == 0 ||
                strcmp(id, "monitor_capture") == 0 ||
                strcmp(id, "dshow_input") == 0;
    }

    const char* GetNameSoopVodSourceFromId(const char* id)
    {
        int sourceSize = sizeof(g_soopMediaSources) / sizeof(g_soopMediaSources[0]);
        for(int idx = 0; idx < sourceSize; idx++) {
            if(0 == strcmp(id, g_soopMediaSources[idx].id))
                return g_soopMediaSources[idx].name;
        }

        return "";
    }

    static bool FindSoopAI(obs_scene_t*, obs_sceneitem_t* item, void* param)
    {
        OBSSceneItem& destItem = *reinterpret_cast<OBSSceneItem*>(param);

        obs_source_t* source = obs_sceneitem_get_source(item);
        const char* id = obs_source_get_id(source);

        bool findAIManager = false;
        if (0 == strcmp(id, "soop_aimanager_source")) {
            findAIManager = true;
        }

        if (findAIManager) {
            destItem = item;
            return false;
        }

        return true;
    };

    int CheckAddSoopVodSource(const char* id)
    {
        if(false == IsSoopMediaSource(id))
            return 0;

        int exceptionType = 0;
        QString exceptionMsg;
        obs_scene_t* scene_t = nullptr;
        obs_sceneitem_t* soopVodSceneItem = nullptr;

        if(!MAIN_OUTPUT->IsStreamingOnlySoop()) {
            exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2").arg(obs_source_get_display_name(id));
            exceptionType = 1;
        } else {
            auto findSoopSource = [id, &exceptionType, &exceptionMsg, &scene_t, &soopVodSceneItem](obs_source_t* scene_source) {

                obs_scene_t* scene = obs_scene_from_source(scene_source);
                const char* sceneName = obs_source_get_name(scene_source);

                OBSSceneItem item;
                obs_scene_enum_items(scene, FindSoopVodSource, &item);

                if(!item)
                    obs_scene_enum_items(scene, FindSoopKBOSource, &item);

                if(!item)
                    obs_scene_enum_items(scene, FindSoopFootballSource, &item);

                if(item)
                {
                    obs_source_t* source = obs_sceneitem_get_source(item);
                    const char* _id = obs_source_get_id(source);
                    if(0 == strcmp(id, _id))
                    {
                        exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage3")
                            .arg(obs_source_get_display_name(_id))
                            .arg(sceneName);

                        exceptionType = 2;
                        return false;
                    }

                    OBSSource curSceneSource = SCENE_CONTEXT.GetCurrentSceneSource();
                    if(0 == strcmp(sceneName, obs_source_get_name(curSceneSource)))
                    {
                        if(IsSoopKBOSource(source)) {
                            exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage4")
                                .arg(QTStr("Basic.SelectedSourcePopup.KBO"))
                                .arg(obs_source_get_display_name(id));

                            exceptionType = 3;
                            return false;
                        }

                        if (IsSoopFootballSource(source)) {
                            exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage4")
                                .arg(QTStr("Basic.SelectedSourcePopup.Football"))
                                .arg(obs_source_get_display_name(id));

                            exceptionType = 5;
                            return false;
                        }
                    }

                    if(IsSoopMediaSource(source))
                    {
                        exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage8")
                            .arg(sceneName);

                        scene_t = scene;
                        soopVodSceneItem = item;

                        exceptionType = 4;
                        return false;
                    }

                }

                return true;
            };

            using FindSoopSource_t = decltype(findSoopSource);
            obs_enum_scenes([](void* data, obs_source_t* source) {
                return (*reinterpret_cast<FindSoopSource_t*>(data))(source);
            }, &findSoopSource);
        }

        if (4 == exceptionType) {

            AFQMessagBoxAlert alert(MAINFRAME, "", exceptionMsg, QTStr("AddAfterClear"), "", true);

            if (QDialog::Accepted == alert.exec())
            {
                obs_source_t* source = obs_scene_get_source(scene_t);
                SOOP_SRC_MANAGER.SetSoopMediaSource(nullptr, true);

                OBSData undo_data = MAINFRAME->BackupScene(source);
                obs_sceneitem_remove(soopVodSceneItem);
                OBSData redo_data = MAINFRAME->BackupScene(source);
                MAINFRAME->CreateSceneUndoRedoAction(QTStr("Undo.Delete"), undo_data, redo_data);

                return 0; // 0 return => Add Source
            }
            return exceptionType;
        }


        if(0 != exceptionType) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", exceptionMsg);
            return exceptionType;
        }

        return 0;
    }

    int CheckAddSoopKBOSource(const char* id)
    {
        if(false == IsSoopKBOSource(id))
            return 0;

        int exceptionType = 0;
        QString exceptionMsg;

        if(!MAIN_OUTPUT->IsStreamingOnlySoop()) {
            exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2")
                .arg(QTStr("Basic.SelectedSourcePopup.KBO"));
            exceptionType = 1;
        } else
        {
            auto findSoopSource = [id, &exceptionType, &exceptionMsg](obs_source_t* scene_source) {

                obs_scene_t* scene = obs_scene_from_source(scene_source);
                const char* sceneName = obs_source_get_name(scene_source);

                OBSSceneItem item;
                obs_scene_enum_items(scene, FindSoopVodSource, &item);

                if(item)
                {
                    obs_source_t* source = obs_sceneitem_get_source(item);
                    OBSSource curSceneSource = SCENE_CONTEXT.GetCurrentSceneSource();
                    if(0 == strcmp(sceneName, obs_source_get_name(curSceneSource)))
                    {
                        if(IsSoopMediaSource(source)) {
                            exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage4")
                                .arg(obs_source_get_display_name(obs_source_get_id(source)))
                                .arg(QTStr("Basic.SelectedSourcePopup.KBO"));

                            exceptionType = 3;
                            return false;
                       }                       
                    }
                }

                obs_scene_enum_items(scene, FindSoopFootballSource, &item);
                if (item)
                {
                    obs_source_t* source = obs_sceneitem_get_source(item);
                    const char* _id = obs_source_get_id(source);


                    OBSSource curSceneSource = SCENE_CONTEXT.GetCurrentSceneSource();
                    if (0 == strcmp(sceneName, obs_source_get_name(curSceneSource)))
                    {
                        if (IsSoopFootballSource(source)) {
                            exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage4")
                                .arg(QTStr("Basic.SelectedSourcePopup.KBO"))
                                .arg(QTStr("Basic.SelectedSourcePopup.Football"));

                            exceptionType = 5;
                            return false;
                        }
                    }
                }

                return true;
            };

            using FindSoopSource_t = decltype(findSoopSource);
            obs_enum_scenes([](void* data, obs_source_t* source) {
                return (*reinterpret_cast<FindSoopSource_t*>(data))(source);
            }, &findSoopSource);
        }

        if(0 != exceptionType) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", exceptionMsg);
            return exceptionType;
        }

        return 0;
    }

    int CheckAddSoopAI(const char* id, bool isSimulcastCheck)
    {
        if (0 != strcmp(id, "soop_aimanager_source"))
            return 0;

        int exceptionType = 0;
        QString exceptionMsg;

        bool simulcast = isSimulcastCheck;

        auto findSoopSource = [id, simulcast, &exceptionType, &exceptionMsg](obs_source_t* scene_source) {

            obs_scene_t* scene = obs_scene_from_source(scene_source);
            const char* sceneName = obs_source_get_name(scene_source);

            OBSSceneItem item;
            obs_scene_enum_items(scene, FindSoopAI, &item);

            if (item)
            {
                obs_source_t* source = obs_sceneitem_get_source(item);
                const char* id = obs_source_get_id(source);

                if (0 == strcmp(id, "soop_aimanager_source"))
                {
                    QString trashName = QString("%1_trash_%2")
                        .arg(obs_source_get_name(source))
                        .arg(QDateTime::currentMSecsSinceEpoch());

                    obs_source_set_name(source, trashName.toUtf8().constData());
                    obs_source_remove(source);

                    exceptionType = 1;

                    exceptionMsg = QTStr("Caution.Deleted.AIManager");
                    if (simulcast)
                    {
                        exceptionMsg = QTStr("Caution.Simulcast.AIManager");
                        exceptionType = 2;
                    }

                    AFMainFrame* main = App()->GetMainView();
                    QMetaObject::invokeMethod(main, [=]() {
                        main->GetMainWindow()->UpdateSourceToolBar(true);
                        }, Qt::QueuedConnection);

                    return false;
                }
            }

            return true;
            };

        using FindSoopSource_t = decltype(findSoopSource);
        obs_enum_scenes([](void* data, obs_source_t* source) {
            return (*reinterpret_cast<FindSoopSource_t*>(data))(source);
            }, &findSoopSource);

        if (!MAIN_OUTPUT->IsStreamingOnlySoop() && !simulcast) {
            exceptionMsg = QTStr("Caution.Simulcast.AIManager");
            exceptionType = 2;
        }

        if (0 != exceptionType)
        {
            if (1 == exceptionType)
            {
                QMetaObject::invokeMethod(MAINFRAME, [=]() {
                    int alertWidth = 400;
                    int alertHeight = 166;
                    if (simulcast)
                        alertWidth = 500;
                    AFQMessageBox::ShowModalessOnButtonAlert(MAINFRAME, exceptionMsg, alertWidth, alertHeight);
                    }, Qt::QueuedConnection);
            }
            else
            {
                AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", exceptionMsg);
            }
        }

        return exceptionType;

    }

    void ChangeAIManagerUrl()
    {
        obs_source_t* found_source = nullptr;

        obs_enum_sources([](void* param, obs_source_t* source) -> bool {
            const char* id = obs_source_get_id(source);
            if (id && strcmp(id, "soop_aimanager_source") == 0) {
                obs_source_t** result = (obs_source_t**)param;
                *result = obs_source_get_ref(source);
                return false;
            }
            return true;
            }, &found_source);

        if (found_source) {
            OBSDataAutoRelease settings = obs_source_get_settings(found_source);

            std::string url = GetAIManagerURL();

            obs_data_set_string(settings, "url", url.c_str());
            obs_source_update(found_source, settings);

            obs_source_release(found_source); 
        }
    }

    std::string GetAIManagerURL()
    {
        bool isAvailable = AUTH_CONTEXT.GetSoopBroadInfo()->UsePassword();
        std::string boolStr = isAvailable ? "false" : "true";

        std::string url = std::string(SOOP_AIMANAGER_URL) + boolStr;

        if (AFOutputUtil::IsStreamActive()) {
            url += "&broadNo=" + std::to_string(AUTH_CONTEXT.GetSoopBroadInfo()->BroadNumber());
        }
        return url;
    }

    int CheckAddSoopFootballSource(const char* id)
    {
        if (false == IsSoopFootballSource(id))
            return 0;

        int exceptionType = 0;
        QString exceptionMsg;

        if (!MAIN_OUTPUT->IsStreamingOnlySoop()) {
            exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage2")
                .arg(QTStr("Basic.SelectedSourcePopup.Football"));
            exceptionType = 1;
        }
        else
        {
            auto findSoopSource = [id, &exceptionType, &exceptionMsg](obs_source_t* scene_source) {

                obs_scene_t* scene = obs_scene_from_source(scene_source);
                const char* sceneName = obs_source_get_name(scene_source);

                OBSSceneItem item;
                obs_scene_enum_items(scene, FindSoopVodSource, &item);

                if (item)
                {
                    obs_source_t* source = obs_sceneitem_get_source(item);
                    OBSSource curSceneSource = SCENE_CONTEXT.GetCurrentSceneSource();
                    if (0 == strcmp(sceneName, obs_source_get_name(curSceneSource)))
                    {
                        if (IsSoopMediaSource(source)) {
                            exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage4")
                                .arg(obs_source_get_display_name(obs_source_get_id(source)))
                                .arg(QTStr("Basic.SelectedSourcePopup.Football"));

                            exceptionType = 3;
                            return false;
                        }
                    }
                }


                obs_scene_enum_items(scene, FindSoopKBOSource, &item);
                if (item)
                {
                    obs_source_t* source = obs_sceneitem_get_source(item);
                    const char* _id = obs_source_get_id(source);


                    OBSSource curSceneSource = SCENE_CONTEXT.GetCurrentSceneSource();
                    if (0 == strcmp(sceneName, obs_source_get_name(curSceneSource)))
                    {
                        if (IsSoopKBOSource(source)) {
                            exceptionMsg = QTStr("Caution.SOOPMediaSource.DisableMessage4")
                                .arg(QTStr("Basic.SelectedSourcePopup.Football"))
                                .arg(QTStr("Basic.SelectedSourcePopup.KBO"));

                            exceptionType = 5;
                            return false;
                        }
                    }
                }

                return true;
                };

            using FindSoopSource_t = decltype(findSoopSource);
            obs_enum_scenes([](void* data, obs_source_t* source) {
                return (*reinterpret_cast<FindSoopSource_t*>(data))(source);
                }, &findSoopSource);
        }

        if (0 != exceptionType) {
            AFQMessageBox::ShowMessage(QDialogButtonBox::Ok, MAINFRAME, "", exceptionMsg);
            return exceptionType;
        }

        return 0;
    }

    //
    void SetUndoRedoAddSource(const char* id, const char* source_name, bool visible)
    {
        /* set monitoring if source monitors by default */
        OBSSource source = SCENE_CONTEXT.GetCurrentSceneSource();
        std::string scene_name = obs_source_get_name(source);
        auto undo = [scene_name](const std::string& data)
        {
            OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
            obs_source_remove(source);

            OBSSourceAutoRelease scene_source = obs_get_source_by_name(scene_name.c_str());
            DYNAMIC_COMPOSIT->SetCurrentScene(scene_source.Get(), true);
        };

        OBSDataAutoRelease wrapper = obs_data_create();
        obs_data_set_string(wrapper, "id", id);
        OBSSceneItemAutoRelease item = obs_scene_sceneitem_from_source(SCENE_CONTEXT.GetCurrentScene(), source);
        obs_data_set_int(wrapper, "item_id", obs_sceneitem_get_id(item));
        obs_data_set_string(wrapper, "name", source_name);
        obs_data_set_bool(wrapper, "visible", visible);

        // add source info
        {
            // setttings
            OBSSourceAutoRelease add_source = obs_get_source_by_name(source_name);
            OBSDataAutoRelease settings = obs_source_get_settings(add_source);
            obs_data_set_obj(wrapper, "settings", settings);

            // transform
            item = obs_scene_sceneitem_from_source(SCENE_CONTEXT.GetCurrentScene(), add_source);

            obs_transform_info transform;
            obs_sceneitem_get_info(item, &transform);

            OBSDataAutoRelease transformData = TransformToData(transform);
            obs_data_set_obj(wrapper, "transformData", transformData);
        }

        auto redo = [scene_name](const std::string& data)
        {
            OBSSourceAutoRelease scene_source = obs_get_source_by_name(scene_name.c_str());
            DYNAMIC_COMPOSIT->SetCurrentScene(scene_source.Get(), true);

            OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
            OBSSource source;
            AddNewSource(NULL, obs_data_get_string(dat, "id"),
                         obs_data_get_string(dat, "name"),
                         obs_data_get_bool(dat, "visible"), source, nullptr);

            OBSSceneItemAutoRelease item = obs_scene_sceneitem_from_source(SCENE_CONTEXT.GetCurrentScene(), source);

            OBSData settings = obs_data_get_obj(dat, "settings");
            if(settings) {
                obs_source_update(source, settings);
            }

            OBSData transformData = obs_data_get_obj(dat, "transformData");
            if(transformData) {
                obs_transform_info transform = DataToTransform(transformData);
                obs_sceneitem_set_info(item, &transform);
            }

            obs_sceneitem_set_id(item, (int64_t)obs_data_get_int(dat, "item_id"));
        };
        UNDO_STACK.AddAction(QTStr("Undo.Add").arg(source_name),
                             undo, redo, source_name,
                             std::string(obs_data_get_json(wrapper)));
    }
}