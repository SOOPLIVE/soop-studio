#include "CMultiview.h"

#include <QApplication>
#include <QWidget>


#include <obs.hpp>

#include "display-helpers.hpp"


#include "Application/CApplication.h"
#include "Blocks/SceneSourceDock/CSceneSourceDockWidget.h"

#include "Common/MathMiscUtils.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"


AFMultiview::AFMultiview()
{
    InitSafeAreas(&m_pActionSafeMargin, &m_pGraphicsSafeMargin,
                  &m_pFourByThreeSafeMargin, &m_pLeftLine, &m_pTopLine, &m_pRightLine);
}

AFMultiview::~AFMultiview()
{
    for (OBSWeakSource &weakSrc : m_multiviewScenes)
    {
        OBSSource src = OBSGetStrongRef(weakSrc);
        if (src)
            obs_source_dec_showing(src);
    }

    obs_enter_graphics();
    gs_vertexbuffer_destroy(m_pActionSafeMargin);
    gs_vertexbuffer_destroy(m_pGraphicsSafeMargin);
    gs_vertexbuffer_destroy(m_pFourByThreeSafeMargin);
    gs_vertexbuffer_destroy(m_pLeftLine);
    gs_vertexbuffer_destroy(m_pTopLine);
    gs_vertexbuffer_destroy(m_pRightLine);
    obs_leave_graphics();
}

void AFMultiview::Update(bool drawLabel)
{
    m_renderLabel = drawLabel;
    
    m_multiviewScenes.clear();
    m_multiviewLabels.clear();

    struct obs_video_info ovi;
    obs_get_video_info(&ovi);

    uint32_t w = ovi.base_width;
    uint32_t h = ovi.base_height;
    m_w = float(w);
    m_h = float(h);
    m_ratio = m_w / m_h;

    struct obs_frontend_source_list scenes = {};
    //obs_frontend_get_scenes(&scenes);

    SceneItemVector& sceneItems = SCENE_CONTEXT.GetSceneItemVector();
    if (sceneItems.empty() == false)
    {
        size_t sceneCount = sceneItems.size();
        for (size_t i = 0; i < sceneCount; i++) 
        {
            OBSScene scene = sceneItems.at(i)->GetScene();
            obs_source_t *source = obs_scene_get_source(scene);

            if (obs_source_get_ref(source) != nullptr)
                da_push_back(scenes.sources, &source);
        }
    }
    //

    //std::string textPreview = "Preview";
    std::string textPreview = Str("StudioMode.Preview");
    
    LabelSourceData textData(m_labelSize);
    textData.labelSetText = true;
    textData.labelText = textPreview.c_str();
    textData.labelRatioSize = (float)(1.f / 9.81f);
    textData.labelOutline = false;
    textData.labelFontFlag = 0;
    textData.labelColor1 = 0xFFFFFFFF;
    textData.labelColor2 = 0xFFFFFFFF;  // abgr : previewColor
    
    m_multiviewLabels.emplace_back(AFSourceUtil::CreateLabelSource(textData));
    
    //std::string textProgram = "Program";
    std::string textProgram = Str("StudioMode.Program");
    textData.labelText = textProgram.c_str();
    textData.labelColor1 = 0xFFFFE000;
    textData.labelColor2 = 0xFFFFE000;  // abgr : programColor
    
    m_multiviewLabels.emplace_back(AFSourceUtil::CreateLabelSource(textData));


    m_pvwprgCX = m_w / 2;
    m_pvwprgCY = m_h / 2;

    m_maxSrcs = 8;
    

    m_ppiCX = m_pvwprgCX - m_thicknessx2;
    m_ppiCY = m_pvwprgCY - m_thicknessx2;
    m_ppiScaleX = (m_pvwprgCX - m_thicknessx2) / m_w;
    m_ppiScaleY = (m_pvwprgCY - m_thicknessx2) / m_h;


    m_scenesCX = m_pvwprgCX / 2;
    m_scenesCY = m_pvwprgCY / 2;
    

    m_cX = m_scenesCX - m_thicknessx2;
    m_cY = m_scenesCY - m_thicknessx2;
    m_scaleX = (m_scenesCX - m_thicknessx2) / m_w;
    m_scaleY = (m_scenesCY - m_thicknessx2) / m_h;

    m_numSrcs = 0;
    size_t i = 0;
    textData.labelRatioSize = (float)(1.f / 9.81f);
    textData.labelColor1 = 0;
    textData.labelColor2 = 0;
    while (i < scenes.sources.num && m_numSrcs < m_maxSrcs)
    {
        obs_source_t *src = scenes.sources.array[i++];
        OBSDataAutoRelease data = obs_source_get_private_settings(src);

        obs_data_set_default_bool(data, "show_in_multiview", true);
        if (!obs_data_get_bool(data, "show_in_multiview"))
            continue;

        // We have a displayable source.
        m_numSrcs++;

        m_multiviewScenes.emplace_back(OBSGetWeakRef(src));
        obs_source_inc_showing(src);


        std::string text;
        text += " ";
        text += obs_source_get_name(src);
        text += " ";
        textData.labelText = text.c_str();

        m_multiviewLabels.emplace_back(AFSourceUtil::CreateLabelSource(textData));
    }

    obs_frontend_source_list_free(&scenes);
}

static inline uint32_t labelOffset(obs_source_t *label, uint32_t cx)
{
    uint32_t w = obs_source_get_width(label);

    int n = 4; // Twice of scale factor of preview and program scenes

    w = uint32_t(w * ((1.0f) / n));
    return (cx / 2) - w;
}

void AFMultiview::Render(uint32_t cx, uint32_t cy)
{   
    uint32_t targetCX, targetCY;
    int x, y;
    float scale;

    targetCX = (uint32_t)m_w;
    targetCY = (uint32_t)m_h;

    GetScaleAndCenterPos(targetCX, targetCY, cx, cy, x, y, scale);
    
    float fixRatio = 1.f / scale * m_dpi;
    
    OBSSource previewSrc = AFSceneUtil::CnvtToOBSSource(SCENE_CONTEXT.GetCurrentScene());
    OBSSource programSrc = OBSGetStrongRef(SCENE_CONTEXT.GetProgramScene());
    
    
    
    bool studioMode = false;
    //studioMode = STATEAPP.IsPreviewProgramMode();
    studioMode = STATEAPP.JustCheckPreviewProgramMode();

    
    auto drawBox = [&](float cx, float cy, uint32_t colorVal) {
        gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
        gs_eparam_t *color =
            gs_effect_get_param_by_name(solid, "color");

        gs_effect_set_color(color, colorVal);
        while (gs_effect_loop(solid, "Solid"))
            gs_draw_sprite(nullptr, 0, (uint32_t)cx, (uint32_t)cy);
    };

    auto setRegion = [&](float bx, float by, float cx, float cy) {
        float vX = int(x + bx * scale);
        float vY = int(y + by * scale);
        float vCX = int(cx * scale);
        float vCY = int(cy * scale);

        float oL = bx;
        float oT = by;
        float oR = (bx + cx);
        float oB = (by + cy);

        startRegion(vX, vY, vCX, vCY, oL, oR, oT, oB);
    };

    auto calcBaseSource = [&](size_t i) {
        if (i < 4)
        {
            m_sourceX = (float(i) * m_scenesCX);

//            if (m_renderOnlySource)
//                m_sourceY = m_pvwprgCY / 2;
//            else
            m_sourceY = m_pvwprgCY;
        }
        else 
        {
            m_sourceX = (float(i - 4) * m_scenesCX);
            
//            if (m_renderOnlySource)
//                m_sourceY = m_pvwprgCY / 2 + m_scenesCY;
//            else
            m_sourceY = m_pvwprgCY + m_scenesCY;
        }
        
        m_x = m_sourceX + m_thickness;
        m_y = m_sourceY + m_thickness;
    };

    auto calcPreviewProgram = [&](bool program) {
        if(studioMode)
        {
            m_sourceX = m_thickness;
            m_sourceY = m_thickness;
            m_labelX = m_thickness;
            m_labelY = m_thickness;
            if (program) {
                m_sourceX += m_pvwprgCX;
                m_labelX += m_pvwprgCX;
            }
        }
        else
        {
            m_sourceX = m_thickness + m_pvwprgCX / 2;
            m_sourceY = m_thickness;
            m_labelX = m_pvwprgCX / 2 + m_thickness;
            m_labelY = m_thickness;
        }
        
    };

    auto paintAreaWithColor = [&](float tx, float ty, float cx, float cy,
                                  uint32_t color,
                                  bool fix = false, float rx = 1.f, float ry = 1.f) {
        gs_matrix_push();
        gs_matrix_translate3f(tx, ty, 0.0f);
        if (fix)
        {
            gs_matrix_scale3f(rx, ry, 1.0f);
            gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        }
        drawBox(cx, cy, color);
        gs_matrix_pop();
    };

    // Define the whole usable region for the multiview
    startRegion(x, y, targetCX * scale, targetCY * scale, 0.0f, m_w, 0.0f, m_h);

    // Change the background color to highlight all sources
    drawBox(m_w, m_h, s_outerColor);

    /* ----------------------------- */
    /* draw sources                  */

    for (size_t i = 0; i < m_maxSrcs; i++)
    {
        // Handle all the offsets
        calcBaseSource(i);

        if (i >= m_numSrcs)
        {
            // Just paint the background and continue
            paintAreaWithColor(m_sourceX, m_sourceY, m_scenesCX, m_scenesCY, s_outerColor);
            paintAreaWithColor(m_x - m_guideLineSize,
                               m_y - m_guideLineSize,
                               m_cX + m_guideLineSizex2,
                               m_cY + m_guideLineSizex2,
                               s_blackColor);
            paintAreaWithColor(m_x, m_y, m_cX, m_cY, s_backgroundColor);
            continue;
        }

        OBSSource src = OBSGetStrongRef(m_multiviewScenes[i]);

        // We have a source. Now chose the proper highlight color
        uint32_t colorVal = s_blackColor;
        if (src == programSrc)
            colorVal = s_programColor;
        else if (src == previewSrc)
            colorVal = studioMode ? s_previewColor : s_programColor;

        // Paint the background
        paintAreaWithColor(m_x - m_guideLineSize,
                           m_y - m_guideLineSize,
                           m_cX + m_guideLineSizex2,
                           m_cY + m_guideLineSizex2,
                           colorVal);
        paintAreaWithColor(m_x, m_y, m_cX, m_cY, s_backgroundColor);
        /* ----------- */

        // Render the source
        gs_matrix_push();
        gs_matrix_translate3f(m_x, m_y, 0.0f);
        gs_matrix_scale3f(m_scaleX, m_scaleY, 1.0f);
        setRegion(m_x, m_y, m_cX, m_cY);
        obs_source_video_render(src);
        endRegion();
        gs_matrix_pop();

        /* ----------- */

        // Render the label
        if (!m_renderLabel)
            continue;

        obs_source *label = m_multiviewLabels[i + 2];
        if (!label)
            continue;

        m_offset = labelOffset(label, m_scenesCX);

        gs_matrix_push();
        gs_matrix_translate3f(m_sourceX + m_thickness,
                              m_sourceY + m_thickness, 0.0f);
        gs_matrix_scale3f(m_ppiScaleX, m_ppiScaleY, 1.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        drawBox(obs_source_get_width(label) / m_ppiScaleX +
                m_labelLeftMarginx2 + m_labelLeftMarginx2 + m_guideLineSize,
                obs_source_get_height(label) / m_ppiScaleY +
                m_labeTopMarginx2 + m_labeTopMarginx2 +
                int(m_sourceY * 0.015f),
                s_labelColor);
        gs_matrix_pop();
        
        gs_matrix_push();
        gs_matrix_translate3f(m_sourceX + m_thickness + (m_labelLeftMargin / scale * m_dpi),
                              m_sourceY + m_thickness + (m_labeTopMargin / scale * m_dpi), 0.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        obs_source_video_render(label);
        gs_matrix_pop();
    }


    if (m_renderOnlySource)
        return;
    

    /* ----------------------------- */
    /* draw preview                  */

    obs_source_t *previewLabel = m_multiviewLabels[0];
    m_offset = labelOffset(previewLabel, m_pvwprgCX);
    calcPreviewProgram(false);

    // guide Line
    paintAreaWithColor(m_sourceX - m_guideLineSize,
                       m_sourceY - m_guideLineSize,
                       m_ppiCX + m_guideLineSizex2,
                       m_ppiCY + m_guideLineSizex2,
                       s_blackColor);
    
    // Paint the background
    paintAreaWithColor(m_sourceX, m_sourceY, m_ppiCX, m_ppiCY, s_backgroundColor);

    // Scale and Draw the preview
    if (studioMode)
    {
        gs_matrix_push();
        gs_matrix_translate3f(m_sourceX, m_sourceY, 0.0f);
        gs_matrix_scale3f(m_ppiScaleX, m_ppiScaleY, 1.0f);
        setRegion(m_sourceX, m_sourceY, m_ppiCX, m_ppiCY);
        
        obs_source_video_render(previewSrc);
        
        endRegion();
        gs_matrix_pop();
        
        /* ----------- */
        
        // Draw the Label
        gs_matrix_push();
        gs_matrix_translate3f(m_labelX, m_labelY, 0.0f);
        gs_matrix_scale3f(m_ppiScaleX, m_ppiScaleY, 1.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        drawBox(obs_source_get_width(previewLabel) / m_ppiScaleX+
                m_labelLeftMarginx2 + m_labelLeftMarginx2 + m_guideLineSize,
                obs_source_get_height(previewLabel) / m_ppiScaleY+
                m_labeTopMarginx2 + m_labeTopMarginx2 +
                 int(m_pvwprgCX * 0.015f),
                s_labelColor);
        gs_matrix_pop();
        
        gs_matrix_push();
        gs_matrix_translate3f(m_labelX + (m_labelLeftMargin / scale * m_dpi),
                              m_labelY + (m_labeTopMargin / scale * m_dpi), 0.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        obs_source_video_render(previewLabel);
        gs_matrix_pop();
    }
    

    /* ----------------------------- */
    /* draw program                  */

    obs_source_t *programLabel = m_multiviewLabels[1];
    m_offset = labelOffset(programLabel, m_pvwprgCX);
    calcPreviewProgram(true);

    // guide Line
    paintAreaWithColor(m_sourceX - m_guideLineSize,
                       m_sourceY - m_guideLineSize,
                       m_ppiCX + m_guideLineSizex2,
                       m_ppiCY + m_guideLineSizex2,
                       s_blackColor);
    
    paintAreaWithColor(m_sourceX, m_sourceY, m_ppiCX, m_ppiCY, s_backgroundColor);

    // Scale and Draw the mainPreview or program
    gs_matrix_push();
    gs_matrix_translate3f(m_sourceX, m_sourceY, 0.0f);
    gs_matrix_scale3f(m_ppiScaleX, m_ppiScaleY, 1.0f);
    setRegion(m_sourceX, m_sourceY, m_ppiCX, m_ppiCY);
    obs_render_main_texture();
    endRegion();
    gs_matrix_pop();
    

    /* ----------- */

    // Draw the Label
    if (m_renderLabel)
    {
        gs_matrix_push();
        gs_matrix_translate3f(m_labelX, m_labelY, 0.0f);
        gs_matrix_scale3f(m_ppiScaleX, m_ppiScaleY, 1.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        drawBox(obs_source_get_width(programLabel) / m_ppiScaleX +
                m_labelLeftMarginx2 + m_labelLeftMarginx2 + m_guideLineSize,
                obs_source_get_height(programLabel) / m_ppiScaleY +
                m_labeTopMarginx2 + m_labeTopMarginx2 +
                 int(m_pvwprgCX * 0.015f),
                s_labelColor);
        gs_matrix_pop();

        gs_matrix_push();
        gs_matrix_translate3f(m_labelX + (m_labelLeftMargin / scale * m_dpi),
                              m_labelY + (m_labeTopMargin / scale * m_dpi), 0.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        obs_source_video_render(programLabel);
        gs_matrix_pop();
    }

    endRegion();
}

OBSSource AFMultiview::GetSourceByPosition(int x, int y, QWidget* rectWidget/* = nullptr*/)
{
    int pos = -1;
    QWidget *rec = nullptr;
    if (rectWidget != nullptr)
        rec = rectWidget;
    else
        rec = QApplication::activeWindow();
    if (!rec)
        return nullptr;
    int cx = rec->width();
    int cy = rec->height();
    int minX = 0;
    int minY = 0;
    int maxX = cx;
    int maxY = cy;

   
    if (float(cx) / float(cy) > m_ratio) {
        int validX = cy * m_ratio;
        minX = (cx / 2) - (validX / 2);
        maxX = (cx / 2) + (validX / 2);
    } else {
        int validY = cx / m_ratio;
        maxY = (cy / 2) + (validY / 2);
    }

    minY = (cy / 2);

    if (!(x < minX || x > maxX || y < minY || y > maxY))
    {
        pos = (x - minX) / ((maxX - minX) / 4);
        if (y > minY + ((maxY - minY) / 2))
            pos += 4;
    }
    

    if (pos < 0 || pos >= (int)m_numSrcs)
        return nullptr;
    
    return OBSGetStrongRef(m_multiviewScenes[pos]);
}
