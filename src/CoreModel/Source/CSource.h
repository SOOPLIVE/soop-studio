#pragma once

#include <obs.hpp>
#include <QWidget>

class AFQVolControl;

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
    bool can_duplicate = false;
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
    const char* labelText;
    bool		labelOutline;
    int			labelOulineHexColor;
    int			labelOulineSize;
    float		labelRatioOulineSize;
    int         labelColor1;
    int         labelColor2;
    int         labelFontFlag;
};

namespace AFSourceUtil
{
    OBSScene	    CnvtToOBSScene(OBSSource source);

    bool		    RemoveSimpleCallback(void*, obs_source_t* pSrc);
    bool		    GetNameSimpleCallback(void* paramVecStr, obs_source_t* pSrc);

    obs_source_t*   CreateLabelSource(LabelSourceData& refData);


    //
    const char*     GetSourceDisplayName(const char* id);

    QString			GetPlaceHodlerText(const char* id);
    bool			AddSavvySource(const char* id, OBSSource& newSource);
    bool			AddNewSource(QWidget* parent, const char* id,
                                 const char* name, const bool visible,
                                 OBSSource& newSource, obs_transform_info* transform = nullptr,
                                 obs_data_t* settings = nullptr, bool addOnProgramMode = false);


    void			AddExistingSource(const char* name, bool visible, bool duplicate,
                                      obs_transform_info* transform, obs_sceneitem_crop* crop,
                                      obs_blending_method* blend_method,
                                      obs_blending_type* blend_mode);

    void			AddExistingSource(OBSSource source, bool visible, bool duplicate,
                                      obs_transform_info* transform, obs_sceneitem_crop* crop,
                                      obs_blending_method* blend_method,
                                      obs_blending_type* blend_mode);

    bool			QueryRemoveSource(obs_source_t* source);

    // !! locate obs - window-basic-main.cpp 6101 line - [34ef67e]
    bool			ShouldShowContextPopupFromProps(obs_source_t* source);
    bool			ShouldShowContextPopup(obs_source_t* source);
    bool			ShouldShowProperties(obs_source_t* source);

    // !! locate obs - window-basic-main.cpp 6233 line - [34ef67e]
    bool			GetSelectedSourceItemOne(obs_scene_t*, obs_sceneitem_t* item, void* param);
    bool			GetSelectedSourceItems(obs_scene_t*, obs_sceneitem_t* item, void* param);
    void			SelectedItemOne(obs_scene_t* scene, void* param);
    void			RemoveSourceItems(OBSScene scene);

    //
    void            RemoveSingleSourceFromAllScenes(const char* targetId);
    void			SourcePaste(SourceCopyInfo& info, bool dup);

    // for transform
    // !! locate obs - window-basic-main.cpp 8746 line - [34ef67e]
    void			ResetTransform();
    void			RotateSourceFromMenu(float degree);
    void			FlipSourceFromMenu(float x, float y);
    bool			RotateSelectedSources(obs_scene_t* /* scene */,
                                                  obs_sceneitem_t* item, void* param);
    void			FitSourceToScreenFromMenu(obs_bounds_type boundsType);
    void			SetCenterToScreenFromMenu(CenterType centerType);

    // for source signal Callback
    void			SourceCreated(void* data, calldata_t* params);
    void			SourceRemoved(void* data, calldata_t* params);
    void			SourceActivated(void* data, calldata_t* params);
    void			SourceDeactivated(void* data, calldata_t* params);
    void			SourceAudioActivated(void* data, calldata_t* params);
    void			SourceAudioDeactivated(void* data, calldata_t* params);
    void			SourceRenamed(void* data, calldata_t* params);

    // for audio
    bool			SourceMixerHidden(obs_source_t* source);
    void			SetSourceMixerHidden(obs_source_t* source, bool hidden);
    bool			SourceVolumeLocked(obs_source_t* source);
    AFQVolControl*  CompareAudio(AFQVolControl* audio1, AFQVolControl* audio2);

    // for soop vod source
    bool			IsSoopSource(const char* id);
    bool			IsSoopSource(obs_source_t* source);
    bool		    IsSoopMediaSource(const char* id, bool exceptDirectBroad = false);
    bool		    IsSoopMediaSource(obs_source_t* source, bool exceptDirectBroad = false);
    bool			IsSoopVodSource(const char* id);
    bool			IsSoopVodSource(obs_source_t* source);
    bool			IsMustTopSource(const char* id);
    bool			IsMustTopSource(obs_source_t* source);
    bool			IsSoopKBOSource(const char* id);
    bool			IsSoopKBOSource(obs_source_t* source);
    bool			IsSoopFootballSource(const char* id);
    bool			IsSoopFootballSource(obs_source_t* source);
    
    bool            IsBrowserSizeStretch(const char* id);

    bool            IsForceDuplicateSource(const char* id);

	const char*			GetNameSoopVodSourceFromId(const char* id);
	int					CheckAddSoopVodSource(const char* id);
	int					CheckAddSoopKBOSource(const char* id);
    int                 CheckAddSoopAI(const char* id, bool isSimulcastCheck = false);
    void                ChangeAIManagerUrl();
    std::string         GetAIManagerURL();
	int					CheckAddSoopFootballSource(const char* id);

    void            SetUndoRedoAddSource(const char* id, const char* source_name, bool visible);
};
