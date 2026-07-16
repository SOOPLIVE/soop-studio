#include "CMainWindowRenderModel.h"


#include <obs.hpp>

#include "display-helpers.hpp"

#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#include "Common/MathMiscUtils.h"
#include "UIComponent/CBasicPreview.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "ViewModel/MainWindow/CMainWindowAccesser.h"


#define PREVIEW_EDGE_SIZE 0


void AFMainWindowRenderModel::RenderMain(void* data, uint32_t, uint32_t)
{
    AFMainWindowRenderModel* caller = static_cast<AFMainWindowRenderModel*>(data);
    
    if (caller->m_pMainPreview == nullptr)
        return;
        
    GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "RenderMain");

    auto& graphicContext = GRAPHIC_CONTEXT;
    //    
    obs_video_info ovi;
    obs_get_video_info(&ovi);

    float previewScale = graphicContext.GetMainPreviewScale();
    graphicContext.SetMainPreviewCX(int(previewScale * float(ovi.base_width)));
    graphicContext.SetMainPreviewCY(int(previewScale * float(ovi.base_height)));


    gs_viewport_push();
    gs_projection_push();

    obs_display_t* display = caller->m_pMainPreview->GetDisplay();
    uint32_t width, height;
    int32_t previewX = graphicContext.GetMainPreviewX();
    int32_t previewY = graphicContext.GetMainPreviewY();

    obs_display_size(display, &width, &height);
    float right = float(width) - previewX;
    float bottom = float(height) - previewY;

    gs_ortho(-previewX, right, -previewY, bottom, -100.0f, 100.0f);

    caller->m_pMainPreview->DrawOverflow();
    
    /* --------------------------------------- */

    gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height),
        -100.0f, 100.0f);

    gs_set_viewport(previewX, previewY,
                    graphicContext.GetMainPreviewCX(),
                    graphicContext.GetMainPreviewCY());

    if (STATEAPP.JustCheckPreviewProgramMode())
    {
        _DrawBackdrop(float(ovi.base_width),
                      float(ovi.base_height));
        
        OBSScene scene = SCENE_CONTEXT.GetCurrentScene();
        obs_source_t *source = obs_scene_get_source(scene);
        if (source)
            obs_source_video_render(source);
    }
    else
        obs_render_main_texture_src_color_only();

    gs_load_vertexbuffer(nullptr);

    /* --------------------------------------- */

    gs_ortho(-previewX, right, -previewY, bottom, -100.0f,
        100.0f);
    gs_reset_viewport();

    //Safe Area Function Need Check
    int32_t targetCX = graphicContext.GetMainPreviewCX();
    int32_t targetCY = graphicContext.GetMainPreviewCY();

    if (caller->m_pMainPreview->GetShowSafeAreas()) {
        RenderSafeAreas(graphicContext.GetActionSafeMargin(), targetCX, targetCY);
        RenderSafeAreas(graphicContext.GetGraphicsSafeMargin(), targetCX, targetCY);
        RenderSafeAreas(graphicContext.GetFourByThreeSafeMargin(), targetCX, targetCY);
        RenderSafeAreas(graphicContext.GetLeftLine(), targetCX, targetCY);
        RenderSafeAreas(graphicContext.GetTopLine(), targetCX, targetCY);
        RenderSafeAreas(graphicContext.GetRightLine(), targetCX, targetCY);
    }
    //Safe Area Function Need Check
    
    auto* mainWindowViewModels = g_viewModelsDynamic.UnSafeGetInstace();
    float tmpDPI = 1.f;
    if (mainWindowViewModels != nullptr)
        tmpDPI = mainWindowViewModels->m_renderModel.GetDPIValue();
    
    caller->m_pMainPreview->DrawSceneEditing(tmpDPI);

    if (caller->m_pMainPreview->GetDrawSpacingHelpers())
        caller->m_pMainPreview->DrawSpacingHelpers(tmpDPI);

    gs_projection_pop();
    gs_viewport_pop();
    
    GS_DEBUG_MARKER_END();
}

void AFMainWindowRenderModel::RenderProgram(void *data, uint32_t, uint32_t)
{
    AFMainWindowRenderModel* caller = static_cast<AFMainWindowRenderModel*>(data);
    
    if (caller->m_programDisplay == nullptr)
        return;
    
    
    GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "RenderProgram");

    auto& graphicContext = GRAPHIC_CONTEXT;
    //
    obs_video_info ovi;
    obs_get_video_info(&ovi);

    float programScale = graphicContext.GetProgramPreviewScale();
    graphicContext.SetProgramPreviewCX(int(programScale * float(ovi.base_width)));
    graphicContext.SetProgramPreviewCY(int(programScale * float(ovi.base_height)));
        
    int32_t previewY = graphicContext.GetProgramPreviewY();
    
    gs_viewport_push();
    gs_projection_push();

    /* --------------------------------------- */

    gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height), -100.0f, 100.0f);
    gs_set_viewport(graphicContext.GetProgramPreviewX(),
                    previewY,
                    graphicContext.GetProgramPreviewCX(),
                    graphicContext.GetProgramPreviewCY());

    obs_render_main_texture_src_color_only();
    gs_load_vertexbuffer(nullptr);

    /* --------------------------------------- */

    gs_projection_pop();
    gs_viewport_pop();

    GS_DEBUG_MARKER_END();
}

void AFMainWindowRenderModel::ResizePreview(uint32_t cx, uint32_t cy)
{
    if (m_pMainPreview == nullptr)
        return;
    
    
    QSize targetSize;
    bool isFixedScaling = m_pMainPreview->IsFixedScaling();

    /* resize preview panel to fix to the top section of the window */
    targetSize = GetPixelSize(m_pMainPreview);

    auto& graphicContext = GRAPHIC_CONTEXT;
    //
    obs_video_info ovi;
    obs_get_video_info(&ovi);

    int32_t previewX = graphicContext.GetMainPreviewX();
    int32_t previewY = graphicContext.GetMainPreviewY();
    float previewScale = graphicContext.GetMainPreviewScale();


    if (isFixedScaling)
    {
        m_pMainPreview->ClampScrollingOffsets();
        previewScale = m_pMainPreview->GetScalingAmount();
        GetCenterPosFromFixedScale(int(cx), int(cy),
                                   targetSize.width() - PREVIEW_EDGE_SIZE * 2,
                                   targetSize.height() - PREVIEW_EDGE_SIZE * 2,
                                   previewX, previewY, previewScale);
        previewX += m_pMainPreview->GetScrollX();
        previewY += m_pMainPreview->GetScrollY();
    }
    else
    {
        GetScaleAndCenterPos(int(cx), int(cy),
                            targetSize.width() - PREVIEW_EDGE_SIZE * 2,
                            targetSize.height() - PREVIEW_EDGE_SIZE * 2,
                            previewX, previewY, previewScale);
    }

    previewX += float(PREVIEW_EDGE_SIZE);
    previewY += float(PREVIEW_EDGE_SIZE);
    
    //m_pMainPreview->SetScalingAmount(previewScale);

    graphicContext.SetMainPreviewX(previewX);
    graphicContext.SetMainPreviewY(previewY);
    graphicContext.SetMainPreviewScale(previewScale);
}

void AFMainWindowRenderModel::RemoveCallbackMainDisplay()
{
    if (m_pMainPreview == nullptr)
        return;
    
    obs_display_remove_draw_callback(m_pMainPreview->GetDisplay(),
                                     AFMainWindowRenderModel::RenderMain, this);
}

void AFMainWindowRenderModel::CreateProgramDisplay()
{
    m_programDisplay = new AFQTDisplay();
}

void AFMainWindowRenderModel::ReleaseProgramDisplay()
{
    auto& sceneContext = SCENE_CONTEXT;
    OBSWeakSource lastScene = sceneContext.GetLastScene();
    
    delete m_programDisplay;
    
    if (lastScene) 
    {
        OBSSource actualLastScene = OBSGetStrongRef(lastScene);
        if (actualLastScene)
            obs_source_dec_showing(actualLastScene);
        lastScene = nullptr;
        sceneContext.SetLastScene(nullptr);
    }

    sceneContext.SetProgramScene(nullptr);
    sceneContext.SetSwapScene(nullptr);

//    prevFTBSource = nullptr;
}

void AFMainWindowRenderModel::ResizeProgram(uint32_t cx, uint32_t cy)
{
    if (m_programDisplay == nullptr)
        return;

    auto& graphicContext = GRAPHIC_CONTEXT;
    //
    QSize targetSize;
    int32_t programX = graphicContext.GetProgramPreviewX();
    int32_t programY = graphicContext.GetProgramPreviewY();
    float programScale = graphicContext.GetProgramPreviewScale();
    
    /* resize program panel to fix to the top section of the window */
    targetSize = GetPixelSize(m_programDisplay);
    GetScaleAndCenterPos(int(cx), int(cy),
                         targetSize.width() - PREVIEW_EDGE_SIZE * 2,
                         targetSize.height() - PREVIEW_EDGE_SIZE * 2,
                         programX, programY, programScale);

    programX += float(PREVIEW_EDGE_SIZE);
    programY += float(PREVIEW_EDGE_SIZE);
        
    graphicContext.SetProgramPreviewX(programX);
    graphicContext.SetProgramPreviewY(programY);
    graphicContext.SetProgramPreviewScale(programScale);
}

void AFMainWindowRenderModel::SetProgramScene()
{
    OBSScene curScene = SCENE_CONTEXT.GetCurrentScene();

    OBSSceneAutoRelease dup;
    if (STATEAPP.GetSceneDuplicationMode() == true)
    {
        dup = obs_scene_duplicate(curScene,
                                  obs_source_get_name(obs_scene_get_source(curScene)),
                                  STATEAPP.GetEditPropertiesMode()
                                  ? OBS_SCENE_DUP_PRIVATE_COPY
                                  : OBS_SCENE_DUP_PRIVATE_REFS);
    } 
    else
        dup = std::move(OBSScene(curScene));
    

    OBSSourceAutoRelease transition = obs_get_output_source(0);
    obs_source_t *dup_source = obs_scene_get_source(dup);
    obs_transition_set(transition, dup_source);

    if (curScene) 
    {
        obs_source_t *source = obs_scene_get_source(curScene);
        obs_source_inc_showing(source);
        SCENE_CONTEXT.SetLastScene(source);
        SCENE_CONTEXT.SetProgramScene(source);
    }
}

void AFMainWindowRenderModel::ResetProgramScene()
{
    OBSSource actualProgramScene = OBSGetStrongRef(SCENE_CONTEXT.GetProgramScene());
    if (!actualProgramScene)
        actualProgramScene = AFSceneUtil::CnvtToOBSSource(SCENE_CONTEXT.GetCurrentScene());
    else
        DYNAMIC_COMPOSIT->SetCurrentScene(actualProgramScene, true);
    SCENE_CONTEXT.TransitionToScene(actualProgramScene, true);
}

void AFMainWindowRenderModel::_DrawBackdrop(float cx, float cy)
{
    GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "DrawBackdrop");

    gs_effect_t *solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_eparam_t *color = gs_effect_get_param_by_name(solid, "color");
    gs_technique_t *tech = gs_effect_get_technique(solid, "Solid");

    vec4 colorVal;
    vec4_set(&colorVal, 0.0f, 0.0f, 0.0f, 1.0f);
    gs_effect_set_vec4(color, &colorVal);

    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);
    gs_matrix_push();
    gs_matrix_identity();
    gs_matrix_scale3f(float(cx), float(cy), 1.0f);

    gs_load_vertexbuffer(GRAPHIC_CONTEXT.GetBoxVB());
    gs_draw(GS_TRISTRIP, 0, 0);

    gs_matrix_pop();
    gs_technique_end_pass(tech);
    gs_technique_end(tech);

    gs_load_vertexbuffer(nullptr);

    GS_DEBUG_MARKER_END();
}