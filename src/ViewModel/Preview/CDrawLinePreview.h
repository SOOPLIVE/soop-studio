#pragma once

#include <obs.hpp>
#include <graphics/graphics.h>
#include <graphics/vec2.h>
#include <graphics/matrix4.h>

#include <QColor>

#define RECT_INDX_CNT 4

#define HANDLE_RADIUS 4.0f
#define HANDLE_SEL_RADIUS (HANDLE_RADIUS * 1.5f)
#define HELPER_ROT_BREAKPOINT 45.0f

class CBasicPreview;
class CModelPreview;
class AFGraphicsContext;

class CDrawLinePreview final
{

public:
	CDrawLinePreview() = default;
	~CDrawLinePreview();

public:
	static void DrawRect(float thickness, vec2 scale);
	static void DrawLine(float x1, float y1, float x2, float y2,
									float thickness, vec2 scale);
	static void DrawSquareAtPosOBSGFX(float x, float y, float pixelRatio);

    void SetModelPreview(CModelPreview* pValue) { previewModel = pValue; };
    
	void DrawOverflow();
	void DrawSceneEditing(const vec2& startPos, const vec2& mousePos, 
                                                 bool selectionBox, float dpiValue = 1.f);
	void DrawSpacingHelpers(float dpiValue = 1.f);

	void SetSelectColor(QColor color);
	void SetCropColor(QColor color);
	void SetHoverColor(QColor color);

	inline void SetOverflowHidden(bool hidden) { overflowHidden = hidden; }
	inline void SetOverflowSelectionHidden(bool hidden) { overflowSelectionHidden = hidden; }
	inline void SetOverflowAlwaysVisible(bool visible) { overflowAlwaysVisible = visible; }
	inline bool GetOverflowSelectionHidden() const { return overflowSelectionHidden; }
    inline bool GetOverflowAlwaysVisible() const {
        return overflowAlwaysVisible || overflowSelectionHidden;
    }

private:
	// callback for libobs
	static bool DrawSelectedOverflow(obs_scene_t* scene, obs_sceneitem_t* item, void* param);
	static bool DrawSelectedItem(obs_scene_t* scene, obs_sceneitem_t* item, void* param);

	void DrawLabel(OBSSource source, vec3& pos, vec3& viewport);
	void DrawSpacingLine(vec3& start, vec3& end, vec3& viewport, float pixelRatio);
	void SetLabelText(int sourceIndex, int px);
	bool DrawSelectionBox(float x1, float y1, float x2, float y2,											  gs_vertbuffer_t* rectFill, float dpiValue = 1.f);
	void RenderSpacingHelper(int sourceIndex, vec3& start, vec3& end,
													 vec3& viewport, float pixelRatio);

private:
    // OverFlow States
    bool overflowHidden = false;
    bool overflowSelectionHidden = false;
    bool overflowAlwaysVisible = false;

	QColor selColor;
	QColor cropColor;
	QColor hoverColor;
    
    float dpiValueLastDraw = 1.f;
	float groupRot = 0.0f;
    
    CModelPreview* previewModel = nullptr;

	gs_texture_t* overflow = nullptr;
	gs_vertbuffer_t* rectFill = nullptr;
    gs_vertbuffer_t* circleFill = nullptr;

	OBSSourceAutoRelease spacerLabel[RECT_INDX_CNT];
	int spacerPx[RECT_INDX_CNT] = { 0 };
};
