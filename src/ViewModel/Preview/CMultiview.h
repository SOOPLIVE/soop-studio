#pragma once

#include <vector>

#include <obs.hpp>

class QWidget;
class AFMultiview final
{
#pragma region QT Field, CTOR/DTOR
public:
    AFMultiview();
	~AFMultiview();
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void                        Update(bool drawLabel);
    void                        Render(uint32_t cx, uint32_t cy);
    OBSSource                   GetSourceByPosition(int x, int y, QWidget* rectWidget = nullptr);
    void                        SetOnlyRenderSources(bool value) { m_renderOnlySource = value; };
    void                        SetRenderLabel(bool value) { m_renderLabel = value; };
    float                       GetDpi() { return m_dpi; };
    void                        SetDpi(float value) { m_dpi = value; };
#pragma endregion public func

#pragma region private func
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
    bool                        m_renderOnlySource = false;
    bool                        m_renderLabel = true;
    
    
    size_t                      m_maxSrcs, m_numSrcs;
    gs_vertbuffer_t*            m_pActionSafeMargin = nullptr;
    gs_vertbuffer_t*            m_pGraphicsSafeMargin = nullptr;
    gs_vertbuffer_t*            m_pFourByThreeSafeMargin = nullptr;
    gs_vertbuffer_t*            m_pLeftLine = nullptr;
    gs_vertbuffer_t*            m_pTopLine = nullptr;
    gs_vertbuffer_t*            m_pRightLine = nullptr;

    std::vector<OBSWeakSource>  m_multiviewScenes;
    std::vector<OBSSourceAutoRelease> m_multiviewLabels;
    
    
    float                       m_dpi = 1.f;

    const uint16_t              m_labelSize = 128;
    const uint16_t              m_labelLeftMargin = 10;
    const uint16_t              m_labelLeftMarginx2 = m_labelLeftMargin * 2;
    const uint16_t              m_labeTopMargin = 4;
    const uint16_t              m_labeTopMarginx2 = m_labeTopMargin * 2;
    const uint16_t              m_guideLineSize = 5;
    const uint16_t              m_guideLineSizex2 = m_guideLineSize * 2;
    
    
    // Multiview position helpers
    float                       m_thickness = 20;
    float                       m_offset, m_thicknessx2 = m_thickness * 2,
                                m_pvwprgCX, m_pvwprgCY, m_sourceX,m_sourceY,
                                m_labelX, m_labelY, m_scenesCX, m_scenesCY, m_ppiCX, m_ppiCY,
                                m_x, m_y, m_cX, m_cY, m_ppiScaleX, m_ppiScaleY, m_scaleX,
                                m_scaleY, m_w, m_h, m_ratio;

    // argb colors
    static const uint32_t s_blackColor = 0xFF000000;
    static const uint32_t s_outerColor = 0xFF24272D;
    static const uint32_t s_labelColor = 0xCE000000;
    static const uint32_t s_backgroundColor = 0xFF181B20;
    static const uint32_t s_programColor = 0xFF00E0FF;
    static const uint32_t s_previewColor = 0xFFFFFFFF;

#pragma endregion private member var
};

static inline void startRegion(int vX, int vY, int vCX, int vCY, float oL, float oR, float oT, float oB)
{
    gs_projection_push();
    gs_viewport_push();
    gs_set_viewport(vX, vY, vCX, vCY);
    gs_ortho(oL, oR, oT, oB, -100.0f, 100.0f);
}

static inline void endRegion()
{
    gs_viewport_pop();
    gs_projection_pop();
}