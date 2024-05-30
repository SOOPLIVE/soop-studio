#include "CMultiview.h"

#include <QApplication>
#include <QWidget>


#include <obs.hpp>


#include "Application/CApplication.h"
#include "Blocks/SceneSourceDock/CSceneSourceDockWidget.h"
//



#include "Common/MathMiscUtils.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Graphics/CGraphicsMiscUtils.inl"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Locale/CLocaleTextManager.h"
#include "CoreModel/Scene/CScene.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "CoreModel/Source/CSource.h"


AFMultiview::AFMultiview()
{
    AFGraphicsContext::InitSafeAreas(&actionSafeMargin, &graphicsSafeMargin,
                                     &fourByThreeSafeMargin, &leftLine, &topLine, &rightLine);
}

AFMultiview::~AFMultiview()
{
    for (OBSWeakSource &weakSrc : multiviewScenes)
    {
        OBSSource src = OBSGetStrongRef(weakSrc);
        if (src)
            obs_source_dec_showing(src);
    }

    obs_enter_graphics();
    gs_vertexbuffer_destroy(actionSafeMargin);
    gs_vertexbuffer_destroy(graphicsSafeMargin);
    gs_vertexbuffer_destroy(fourByThreeSafeMargin);
    gs_vertexbuffer_destroy(leftLine);
    gs_vertexbuffer_destroy(topLine);
    gs_vertexbuffer_destroy(rightLine);
    obs_leave_graphics();
}

void AFMultiview::Update(bool drawLabel)
{
    auto& textManager = AFLocaleTextManager::GetSingletonInstance();
    m_bRenderLabel = drawLabel;
    
    multiviewScenes.clear();
    multiviewLabels.clear();

    struct obs_video_info ovi;
    obs_get_video_info(&ovi);

    uint32_t w = ovi.base_width;
    uint32_t h = ovi.base_height;
    fw = float(w);
    fh = float(h);
    ratio = fw / fh;

    struct obs_frontend_source_list scenes = {};
    //obs_frontend_get_scenes(&scenes);

    AFSceneContext& sceneContext = AFSceneContext::GetSingletonInstance();

    SceneItemVector& sceneItems = sceneContext.GetSceneItemVector();
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
    std::string textPreview = textManager.Str("StudioMode.Preview");
    
    LabelSourceData textData(labelSize);
    textData.labelSetText = true;
    textData.labelText = textPreview.c_str();
    textData.labelRatioSize = (float)(1.f / 9.81f);
    textData.labelOutline = false;
    textData.labelFontFlag = 0;
    textData.labelColor1 = 0xFFFFE000;
    textData.labelColor2 = 0xFFFFE000;  // abgr : previewColor
    
    multiviewLabels.emplace_back(AFSourceUtil::CreateLabelSource(textData));
    
    //std::string textProgram = "Program";
    std::string textProgram = textManager.Str("StudioMode.Program");
    textData.labelText = textProgram.c_str();
    textData.labelColor1 = 0xFF01FFD1;
    textData.labelColor2 = 0xFF01FFD1;  // abgr : programColor
    
    multiviewLabels.emplace_back(AFSourceUtil::CreateLabelSource(textData));


    pvwprgCX = fw / 2;
    pvwprgCY = fh / 2;

    maxSrcs = 8;
    

    ppiCX = pvwprgCX - thicknessx2;
    ppiCY = pvwprgCY - thicknessx2;
    ppiScaleX = (pvwprgCX - thicknessx2) / fw;
    ppiScaleY = (pvwprgCY - thicknessx2) / fh;


    scenesCX = pvwprgCX / 2;
    scenesCY = pvwprgCY / 2;
    

    siCX = scenesCX - thicknessx2;
    siCY = scenesCY - thicknessx2;
    siScaleX = (scenesCX - thicknessx2) / fw;
    siScaleY = (scenesCY - thicknessx2) / fh;

    numSrcs = 0;
    size_t i = 0;
    textData.labelRatioSize = (float)(1.f / 9.81f);
    textData.labelColor1 = 0;
    textData.labelColor2 = 0;
    while (i < scenes.sources.num && numSrcs < maxSrcs)
    {
        obs_source_t *src = scenes.sources.array[i++];
        OBSDataAutoRelease data = obs_source_get_private_settings(src);

        obs_data_set_default_bool(data, "show_in_multiview", true);
        if (!obs_data_get_bool(data, "show_in_multiview"))
            continue;

        // We have a displayable source.
        numSrcs++;

        multiviewScenes.emplace_back(OBSGetWeakRef(src));
        obs_source_inc_showing(src);


        std::string text;
        text += " ";
        text += obs_source_get_name(src);
        text += " ";

        textData.labelText = text.c_str();

        multiviewLabels.emplace_back(AFSourceUtil::CreateLabelSource(textData));
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
    auto& sceneContext =  AFSceneContext::GetSingletonInstance();
    
    uint32_t targetCX, targetCY;
    int x, y;
    float scale;

    targetCX = (uint32_t)fw;
    targetCY = (uint32_t)fh;

    GetScaleAndCenterPos(targetCX, targetCY, cx, cy, x, y, scale);
    
    float fixRatio = 1.f / scale * m_fDpi;
    
    OBSSource previewSrc = AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrOBSScene());
    OBSSource programSrc = OBSGetStrongRef(sceneContext.GetProgramOBSScene());
    
    
    
    AFStateAppContext* tmpStateApp = AFConfigManager::GetSingletonInstance().GetStates();
    bool studioMode = false;
    if (tmpStateApp)
        //studioMode = tmpStateApp->IsPreviewProgramMode();
        studioMode = tmpStateApp->JustCheckPreviewProgramMode();

    
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

        StartGraphicsViewRegion(vX, vY, vCX, vCY, oL, oR, oT, oB);
    };

    auto calcBaseSource = [&](size_t i) {
        if (i < 4)
        {
            sourceX = (float(i) * scenesCX);

//            if (m_bRenderOnlySource)
//                sourceY = pvwprgCY / 2;
//            else
                sourceY = pvwprgCY;
        }
        else 
        {
            sourceX = (float(i - 4) * scenesCX);
            
//            if (m_bRenderOnlySource)
//                sourceY = pvwprgCY / 2 + scenesCY;
//            else
                sourceY = pvwprgCY + scenesCY;
        }
        
        siX = sourceX + thickness;
        siY = sourceY + thickness;
    };

    auto calcPreviewProgram = [&](bool program) {
        if(studioMode)
        {
            sourceX = thickness;
            sourceY = thickness;
            labelX = thickness;
            labelY = thickness;
            if (program) {
                sourceX += pvwprgCX;
                labelX += pvwprgCX;
            }
        }
        else
        {
            sourceX = thickness + pvwprgCX / 2;
            sourceY = thickness;
            labelX = pvwprgCX / 2 + thickness;
            labelY = thickness;
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
    StartGraphicsViewRegion(x, y, targetCX * scale, targetCY * scale, 0.0f, fw, 0.0f,
            fh);

    // Change the background color to highlight all sources
    drawBox(fw, fh, outerColor);

    /* ----------------------------- */
    /* draw sources                  */

    for (size_t i = 0; i < maxSrcs; i++)
    {
        // Handle all the offsets
        calcBaseSource(i);

        if (i >= numSrcs)
        {
            // Just paint the background and continue
            paintAreaWithColor(sourceX, sourceY, scenesCX, scenesCY,
                               outerColor);
            paintAreaWithColor(siX - guideLineSize,
                               siY - guideLineSize,
                               siCX + guideLineSizex2,
                               siCY + guideLineSizex2,
                               blackColor);
            paintAreaWithColor(siX, siY, siCX, siCY,
                               backgroundColor);
            continue;
        }

        OBSSource src = OBSGetStrongRef(multiviewScenes[i]);

        // We have a source. Now chose the proper highlight color
        uint32_t colorVal = blackColor;
        if (src == programSrc)
            colorVal = programColor;
        else if (src == previewSrc)
            colorVal = studioMode ? previewColor : programColor;

        // Paint the background
        paintAreaWithColor(siX - guideLineSize,
                           siY - guideLineSize,
                           siCX + guideLineSizex2,
                           siCY + guideLineSizex2,
                           colorVal);
        paintAreaWithColor(siX, siY, siCX, siCY, backgroundColor);

        /* ----------- */

        // Render the source
        gs_matrix_push();
        gs_matrix_translate3f(siX, siY, 0.0f);
        gs_matrix_scale3f(siScaleX, siScaleY, 1.0f);
        setRegion(siX, siY, siCX, siCY);
        obs_source_video_render(src);
        EndGraphicsViewRegion();
        gs_matrix_pop();

        /* ----------- */

        // Render the label
        if (!m_bRenderLabel)
            continue;

        obs_source *label = multiviewLabels[i + 2];
        if (!label)
            continue;

        offset = labelOffset(label, scenesCX);

        gs_matrix_push();
        gs_matrix_translate3f(sourceX + thickness,
                              sourceY + thickness, 0.0f);
        gs_matrix_scale3f(ppiScaleX, ppiScaleY, 1.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        drawBox(obs_source_get_width(label) / ppiScaleX +
                 labelLeftMarginx2 + labelLeftMarginx2 + guideLineSize,
                obs_source_get_height(label) / ppiScaleY +
                 labeTopMarginx2 + labeTopMarginx2 +
                int(sourceY * 0.015f),
                labelColor);
        gs_matrix_pop();
        
        gs_matrix_push();
        gs_matrix_translate3f(sourceX + thickness + (labelLeftMargin / scale * m_fDpi),
                              sourceY + thickness + (labeTopMargin / scale * m_fDpi), 0.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        obs_source_video_render(label);
        gs_matrix_pop();
    }


    if (m_bRenderOnlySource)
        return;
    

    /* ----------------------------- */
    /* draw preview                  */

    obs_source_t *previewLabel = multiviewLabels[0];
    offset = labelOffset(previewLabel, pvwprgCX);
    calcPreviewProgram(false);

    paintAreaWithColor(sourceX - guideLineSize,
                       sourceY - guideLineSize,
                       ppiCX + guideLineSizex2,
                       ppiCY + guideLineSizex2,
                       blackColor);
    
    // Paint the background
    paintAreaWithColor(sourceX, sourceY, ppiCX, ppiCY, backgroundColor);

    // Scale and Draw the preview
    if (studioMode)
    {
        gs_matrix_push();
        gs_matrix_translate3f(sourceX, sourceY, 0.0f);
        gs_matrix_scale3f(ppiScaleX, ppiScaleY, 1.0f);
        setRegion(sourceX, sourceY, ppiCX, ppiCY);
        
        obs_source_video_render(previewSrc);
        
        EndGraphicsViewRegion();
        gs_matrix_pop();
        
        /* ----------- */
        
        // Draw the Label
        gs_matrix_push();
        gs_matrix_translate3f(labelX, labelY, 0.0f);
        gs_matrix_scale3f(ppiScaleX, ppiScaleY, 1.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        drawBox(obs_source_get_width(previewLabel) / ppiScaleX+
                 labelLeftMarginx2 + labelLeftMarginx2 + guideLineSize,
                obs_source_get_height(previewLabel) / ppiScaleY+
                 labeTopMarginx2 + labeTopMarginx2 +
                 int(pvwprgCX * 0.015f),
                labelColor);
        gs_matrix_pop();
        
        gs_matrix_push();
        gs_matrix_translate3f(labelX + (labelLeftMargin / scale * m_fDpi),
                              labelY + (labeTopMargin / scale * m_fDpi), 0.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        obs_source_video_render(previewLabel);
        gs_matrix_pop();
    }
    

    /* ----------------------------- */
    /* draw program                  */

    obs_source_t *programLabel = multiviewLabels[1];
    offset = labelOffset(programLabel, pvwprgCX);
    calcPreviewProgram(true);

    paintAreaWithColor(sourceX - guideLineSize,
                       sourceY - guideLineSize,
                       ppiCX + guideLineSizex2,
                       ppiCY + guideLineSizex2,
                       blackColor);
    
    paintAreaWithColor(sourceX, sourceY, ppiCX, ppiCY, backgroundColor);

    // Scale and Draw the mainPreview or program
    gs_matrix_push();
    gs_matrix_translate3f(sourceX, sourceY, 0.0f);
    gs_matrix_scale3f(ppiScaleX, ppiScaleY, 1.0f);
    setRegion(sourceX, sourceY, ppiCX, ppiCY);
    obs_render_main_texture();
    EndGraphicsViewRegion();
    gs_matrix_pop();
    

    /* ----------- */

    // Draw the Label
    if (m_bRenderLabel)
    {
        gs_matrix_push();
        gs_matrix_translate3f(labelX, labelY, 0.0f);
        gs_matrix_scale3f(ppiScaleX, ppiScaleY, 1.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        drawBox(obs_source_get_width(programLabel) / ppiScaleX +
                 labelLeftMarginx2 + labelLeftMarginx2 + guideLineSize,
                obs_source_get_height(programLabel) / ppiScaleY +
                 labeTopMarginx2 + labeTopMarginx2 +
                 int(pvwprgCX * 0.015f),
                labelColor);
        gs_matrix_pop();

        gs_matrix_push();
        gs_matrix_translate3f(labelX + (labelLeftMargin / scale * m_fDpi),
            labelY + (labeTopMargin / scale * m_fDpi), 0.0f);
        gs_matrix_scale3f(fixRatio, fixRatio, 1.0f);
        obs_source_video_render(programLabel);
        gs_matrix_pop();
    }


    EndGraphicsViewRegion();
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

   
    if (float(cx) / float(cy) > ratio) {
        int validX = cy * ratio;
        minX = (cx / 2) - (validX / 2);
        maxX = (cx / 2) + (validX / 2);
    } else {
        int validY = cx / ratio;
        maxY = (cy / 2) + (validY / 2);
    }

    minY = (cy / 2);

    if (!(x < minX || x > maxX || y < minY || y > maxY))
    {
        pos = (x - minX) / ((maxX - minX) / 4);
        if (y > minY + ((maxY - minY) / 2))
            pos += 4;
    }
    

    if (pos < 0 || pos >= (int)numSrcs)
        return nullptr;
    
    return OBSGetStrongRef(multiviewScenes[pos]);
}
