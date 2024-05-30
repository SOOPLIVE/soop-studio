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
    void                        SetOnlyRenderSources(bool value) { m_bRenderOnlySource = value; };
    void                        SetRenderLabel(bool value) { m_bRenderLabel = value; };
    float                       GetDpi() { return m_fDpi; };
    void                        SetDpi(float value) { m_fDpi = value; };
#pragma endregion public func

#pragma region private func
#pragma endregion private func

#pragma region public member var
#pragma endregion public member var

#pragma region private member var
private:
    bool                        m_bRenderOnlySource = false;
    bool                        m_bRenderLabel = true;
    
    
    size_t                      maxSrcs, numSrcs;
    gs_vertbuffer_t*            actionSafeMargin = nullptr;
    gs_vertbuffer_t*            graphicsSafeMargin = nullptr;
    gs_vertbuffer_t*            fourByThreeSafeMargin = nullptr;
    gs_vertbuffer_t*            leftLine = nullptr;
    gs_vertbuffer_t*            topLine = nullptr;
    gs_vertbuffer_t*            rightLine = nullptr;

    std::vector<OBSWeakSource>  multiviewScenes;
    std::vector<OBSSourceAutoRelease> multiviewLabels;
    
    
    float                       m_fDpi = 1.f;

    const uint16_t              labelSize = 128;
    const uint16_t              labelLeftMargin = 10;
    const uint16_t              labelLeftMarginx2 = labelLeftMargin * 2;
    const uint16_t              labeTopMargin = 4;
    const uint16_t              labeTopMarginx2 = labeTopMargin * 2;
    const uint16_t              guideLineSize = 5;
    const uint16_t              guideLineSizex2 = guideLineSize * 2;
    
    
    // Multiview position helpers
    float                       thickness = 20;
    float                       offset, thicknessx2 = thickness * 2,
                                pvwprgCX, pvwprgCY, sourceX,sourceY,
                                labelX, labelY, scenesCX, scenesCY, ppiCX, ppiCY,
                                siX, siY, siCX, siCY, ppiScaleX, ppiScaleY, siScaleX,
                                siScaleY, fw, fh, ratio;

    // argb colors
    static const uint32_t blackColor = 0xFF000000;
    static const uint32_t outerColor = 0xFF24272D;
    static const uint32_t labelColor = 0xCE000000;
    static const uint32_t backgroundColor = 0xFF181B20;
    static const uint32_t programColor = 0xFFD1FF01;
    static const uint32_t previewColor = 0xFF00E0FF;

#pragma endregion private member var
};
