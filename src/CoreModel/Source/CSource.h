#pragma once

#include <obs.hpp>

#include <QWidget>

enum class CenterType {
	Scene,
	Vertical,
	Horizontal,
};

struct SourceCopyInfo {
	OBSWeakSource weak_source;
	bool visible = false;
	obs_sceneitem_crop crop = {0,};
	obs_transform_info transform = {0,};
	obs_blending_method blend_method = obs_blending_method::OBS_BLEND_METHOD_DEFAULT;
	obs_blending_type blend_mode = obs_blending_type::OBS_BLEND_NORMAL;
};

struct LabelSourceData
{
public:
	// move CTOR remove
	LabelSourceData(const LabelSourceData&) = delete;
	LabelSourceData(LabelSourceData&&) = delete;
	LabelSourceData& operator=(const LabelSourceData&) = delete;
	LabelSourceData& operator=(LabelSourceData&&) = delete;
	//

	inline LabelSourceData(size_t size)
		:labelSize((int)size),
		labelRatioSize(0.f),
		labelSetText(false),
		labelText(""),
		labelOutline(false),
		labelOulineHexColor(0x000000),
		labelOulineSize(0),
		labelRatioOulineSize(0.f),
        labelColor1(0),
        labelColor2(0),
        labelFontFlag(1) {};


	int			labelSize;
	float		labelRatioSize;
	bool		labelSetText;
	const char*	labelText;
	bool		labelOutline;
	int			labelOulineHexColor;
	int			labelOulineSize;
	float		labelRatioOulineSize;
    int         labelColor1;
    int         labelColor2;
    int         labelFontFlag;
};


class AFSourceUtil final
{
#pragma region QT Field, CTOR/DTOR
public:
	AFSourceUtil() = delete;
	~AFSourceUtil() = delete;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	static inline OBSScene		CnvtToOBSScene(OBSSource source);

	static inline bool			RemoveSimpleCallback(void*, obs_source_t* pSrc);
	static inline bool			GetNameSimpleCallback(void* paramVecStr, obs_source_t* pSrc);

	static obs_source_t*		CreateLabelSource(LabelSourceData& refData);


	//
	static const char*			GetSourceDisplayName(const char* id);

	static QString				GetPlaceHodlerText(const char* id);
	static bool					AddNewSource(QWidget* parent, const char* id, 
											 const char* name, const bool visible, 
											 OBSSource& newSource, obs_transform_info* transform = nullptr,
											 bool useUndoRedo = true);

	static void					AddExistingSource(const char* name, bool visible, bool duplicate,
												  obs_transform_info* transform, obs_sceneitem_crop* crop,
												  obs_blending_method* blend_method,
												  obs_blending_type* blend_mode);

	static void					AddExistingSource(OBSSource source, bool visible, bool duplicate,
												  obs_transform_info* transform, obs_sceneitem_crop* crop,
												  obs_blending_method* blend_method,
												  obs_blending_type* blend_mode);

	static bool					QueryRemoveSource(obs_source_t* source);

	static bool					ShouldShowContextPopupFromProps(obs_source_t* source);
	static bool					ShouldShowContextPopup(obs_source_t* source);
	static bool					ShouldShowProperties(obs_source_t* source);

    static bool					GetSelectedSourceItemOne(obs_scene_t*, obs_sceneitem_t* item, void* param);
	static bool					GetSelectedSourceItems(obs_scene_t*, obs_sceneitem_t* item, void* param);
    static void					SelectedItemOne(obs_scene_t* scene, void* param);
	static void					RemoveSourceItems(OBSScene scene);

	//
	static void					SourcePaste(SourceCopyInfo& info, bool dup);

	// for transform
	static void					RotateSourceFromMenu(float degree);
	static void					FlipSourceFromMenu(float x, float y);
	static bool					RotateSelectedSources(obs_scene_t* /* scene */,
													  obs_sceneitem_t* item, void* param);
	static void					FitSourceToScreenFromMenu(obs_bounds_type boundsType);
	static void					SetCenterToScreenFromMenu(CenterType centerType);

#pragma endregion public func

#pragma region private func
	static OBSScene				GetCurrentScene();
	static OBSSource			GetCurrentSource();
#pragma endregion private func

#pragma region public member var

#pragma endregion public member var

#pragma region private member var
#pragma endregion private member var
};


#include "CSource.inl"
