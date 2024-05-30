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


class AFBasicPreview;
class AFModelPreview;
class AFGraphicsContext;
class AFSceneContext;



class AFDrawLinePreview final
{
#pragma region QT Field, CTOR/DTOR
public:
	AFDrawLinePreview() = default;
	~AFDrawLinePreview();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
	static void					DrawRectOBSGFX(float thickness, vec2 scale);
	static void					DrawLineOBSGFX(float x1, float y1, float x2, float y2,
											   float thickness, vec2 scale);
	static void					DrawSquareAtPosOBSGFX(float x, float y, float pixelRatio);

	void						SetUnsafeAccessContext(AFGraphicsContext* pGraphicsContext, 
													   AFSceneContext* pSceneContext);
    
    void                        SetModelPreview(AFModelPreview* pValue) { m_pPreviewModel = pValue; };
    

	void						DrawOverflow();
	void						DrawSceneEditing(const vec2& startPos, const vec2& mousePos, 
                                                 bool selectionBox, float dpiValue = 1.f);
	void						DrawSpacingHelpers(float dpiValue = 1.f);

	void						SetSelectColor(QColor color);
	void						SetCropColor(QColor color);
	void						SetHoverColor(QColor color);

	inline void					SetOverflowHidden(bool hidden) { m_bOverflowHidden = hidden; }
	inline void					SetOverflowSelectionHidden(bool hidden) { m_bOverflowSelectionHidden = hidden; }
	inline void					SetOverflowAlwaysVisible(bool visible) { m_bOverflowAlwaysVisible = visible; }
	inline bool					GetOverflowSelectionHidden() const { return m_bOverflowSelectionHidden; }
    inline bool GetOverflowAlwaysVisible() const {
        return m_bOverflowAlwaysVisible || m_bOverflowSelectionHidden;
    }
#pragma endregion public func

#pragma region private func
private:
	// callback for libobs
	static bool					_DrawSelectedOverflow(obs_scene_t* scene, obs_sceneitem_t* item, void* param);
	static bool					_DrawSelectedItem(obs_scene_t* scene, obs_sceneitem_t* item, void* param);
	//

    
	void						_DrawLabel(OBSSource source, vec3& pos, vec3& viewport);
	void						_DrawSpacingLine(vec3& start, vec3& end, vec3& viewport, float pixelRatio);

	void						_SetLabelText(int sourceIndex, int px);


	bool						_DrawSelectionBox(float x1, float y1, float x2, float y2,
												  gs_vertbuffer_t* rectFill, float dpiValue = 1.f);
	void						_RenderSpacingHelper(int sourceIndex, vec3& start, vec3& end,
													 vec3& viewport, float pixelRatio);

#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
	// Fast Access Context
	AFGraphicsContext*				m_pInitedContextGraphics = nullptr;
	AFSceneContext*					m_pInitedContextScene = nullptr;
	//

  
    bool m_bOverflowHidden = false;
    bool m_bOverflowSelectionHidden = false;
    bool m_bOverflowAlwaysVisible = false;
    //

	QColor m_selColor;
	QColor m_cropColor;
	QColor m_hoverColor;
    
    float                           m_fDpiValueLastDraw = 1.f;
	float							m_fGroupRot = 0.0f;
    
    AFModelPreview*                 m_pPreviewModel = nullptr;

	gs_vertbuffer_t*				m_vbBox = nullptr;
	gs_vertbuffer_t*				m_vbRectFill = nullptr;
    gs_vertbuffer_t*                m_vbCircleFill = nullptr;
	gs_texture_t*					m_texOverflow = nullptr;

	OBSSourceAutoRelease			m_obsSpacerLabel[RECT_INDX_CNT];
	int								m_iarrSpacerPx[RECT_INDX_CNT] = { 0 };
#pragma endregion private member var
};
