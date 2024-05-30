#include "CMainWindowRenderModel.h"


#include <obs.hpp>


#include "Application/CApplication.h"
#include "MainFrame/CMainFrame.h"
#include "MainFrame/DynamicCompose/CMainDynamicComposit.h"

#include "Common/MathMiscUtils.h"
#include "UIComponent/CBasicPreview.h"
#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Config/CStateAppContext.h"
#include "CoreModel/Graphics/CGraphicsContext.h"
#include "CoreModel/Scene/CScene.h"
#include "CoreModel/Scene/CSceneContext.h"
#include "ViewModel/MainWindow/CMainWindowAccesser.h"



// Fast Access Context
AFConfigManager*                            AFMainWindowRenderModel::m_pInitedConfigManager = nullptr;
AFGraphicsContext*                          AFMainWindowRenderModel::m_pInitedContextGraphics = nullptr;
AFSceneContext*                             AFMainWindowRenderModel::m_pInitedContextScene = nullptr;
//


#define PREVIEW_EDGE_SIZE 0


void AFMainWindowRenderModel::RenderMain(void* data, uint32_t, uint32_t)
{
    AFMainWindowRenderModel* caller = static_cast<AFMainWindowRenderModel*>(data);
    
    if (m_pInitedConfigManager == nullptr ||
        m_pInitedContextGraphics == nullptr ||
        m_pInitedContextScene == nullptr ||
        caller->m_pMainPreview == nullptr)
        return;
    
    
    GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "RenderMain");
    
    obs_video_info ovi;
    obs_get_video_info(&ovi);

    float previewScale = m_pInitedContextGraphics->GetMainPreviewScale();
    m_pInitedContextGraphics->SetMainPreviewCX(int(previewScale * float(ovi.base_width)));
    m_pInitedContextGraphics->SetMainPreviewCY(int(previewScale * float(ovi.base_height)));


    gs_viewport_push();
    gs_projection_push();

    obs_display_t* display = caller->m_pMainPreview->GetDisplay();
    uint32_t width, height;
    int32_t previewX = m_pInitedContextGraphics->GetMainPreviewX();
    int32_t previewY = m_pInitedContextGraphics->GetMainPreviewY();
    

    AFStateAppContext* tmpStateApp = m_pInitedConfigManager->GetStates();
//    if (tmpStateApp->JustCheckPreviewProgramMode())
//        previewY = 0;
    

    obs_display_size(display, &width, &height);
    float right = float(width) - previewX;
    float bottom = float(height) - previewY;

    gs_ortho(-previewX, right, -previewY, bottom, -100.0f, 100.0f);

    caller->m_pMainPreview->DrawOverflow();
    
    /* --------------------------------------- */

    gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height),
        -100.0f, 100.0f);

    gs_set_viewport(previewX, previewY,
                    m_pInitedContextGraphics->GetMainPreviewCX(),
                    m_pInitedContextGraphics->GetMainPreviewCY());

    if (tmpStateApp->JustCheckPreviewProgramMode())
    {
        _DrawBackdrop(float(ovi.base_width),
                      float(ovi.base_height));
        
        OBSScene scene = m_pInitedContextScene->GetCurrOBSScene();
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

    int32_t targetCX = m_pInitedContextGraphics->GetMainPreviewCX();
    int32_t targetCY = m_pInitedContextGraphics->GetMainPreviewCY();

    if (caller->m_pMainPreview->GetShowSafeAreas()) {
        m_pInitedContextGraphics->RenderSafeAreas(m_pInitedContextGraphics->GetActionSafeMargin(), 
            targetCX, targetCY);
        m_pInitedContextGraphics->RenderSafeAreas(m_pInitedContextGraphics->GetGraphicsSafeMargin(),
            targetCX, targetCY);
        m_pInitedContextGraphics->RenderSafeAreas(m_pInitedContextGraphics->GetFourByThreeSafeMargin(),
            targetCX, targetCY);
        m_pInitedContextGraphics->RenderSafeAreas(m_pInitedContextGraphics->GetLeftLine(), 
            targetCX, targetCY);
        m_pInitedContextGraphics->RenderSafeAreas(m_pInitedContextGraphics->GetTopLine(), 
            targetCX, targetCY);
        m_pInitedContextGraphics->RenderSafeAreas(m_pInitedContextGraphics->GetRightLine(), 
            targetCX, targetCY);
    }
    
    auto* mainWindowViewModels = g_ViewModelsDynamic.UnSafeGetInstace();
    float tmpDPI = 1.f;
    if (mainWindowViewModels != nullptr)
        tmpDPI = mainWindowViewModels->m_RenderModel.GetDPIValue();
    
    caller->m_pMainPreview->DrawSceneEditing(tmpDPI);

    // need check Helpers
    if (caller->m_pMainPreview->GetDrawSpacingHelpers())
        caller->m_pMainPreview->DrawSpacingHelpers(tmpDPI);

    gs_projection_pop();
    gs_viewport_pop();
    
    GS_DEBUG_MARKER_END();
}

void AFMainWindowRenderModel::RenderProgram(void *data, uint32_t, uint32_t)
{
    AFMainWindowRenderModel* caller = static_cast<AFMainWindowRenderModel*>(data);
    
    if (m_pInitedContextGraphics == nullptr ||
        caller->m_ProgramDisplay == nullptr)
        return;
    
    
    GS_DEBUG_MARKER_BEGIN(GS_DEBUG_COLOR_DEFAULT, "RenderProgram");

    obs_video_info ovi;

    obs_get_video_info(&ovi);

    float programScale = m_pInitedContextGraphics->GetProgramPreviewScale();
    m_pInitedContextGraphics->SetProgramPreviewCX(int(programScale * float(ovi.base_width)));
    m_pInitedContextGraphics->SetProgramPreviewCY(int(programScale * float(ovi.base_height)));
    
    
    int32_t previewY = m_pInitedContextGraphics->GetProgramPreviewY();
    if (m_pInitedConfigManager != nullptr)
    {
        AFStateAppContext* tmpStateApp = m_pInitedConfigManager->GetStates();
//        if (tmpStateApp->JustCheckPreviewProgramMode())
//            previewY = 0;
    }

    
    gs_viewport_push();
    gs_projection_push();

    /* --------------------------------------- */

    gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height),
         -100.0f, 100.0f);
    gs_set_viewport(m_pInitedContextGraphics->GetProgramPreviewX(),
                    previewY,
                    m_pInitedContextGraphics->GetProgramPreviewCX(),
                    m_pInitedContextGraphics->GetProgramPreviewCY());

    obs_render_main_texture_src_color_only();
    gs_load_vertexbuffer(nullptr);

    /* --------------------------------------- */

    gs_projection_pop();
    gs_viewport_pop();

    GS_DEBUG_MARKER_END();
}

void AFMainWindowRenderModel::ResizePreview(uint32_t cx, uint32_t cy)
{
    if (m_pInitedContextGraphics == nullptr ||
        m_pMainPreview == nullptr)
        return;
    
    
    QSize targetSize;
    bool isFixedScaling = m_pMainPreview->IsFixedScaling();

    /* resize preview panel to fix to the top section of the window */
    targetSize = GetPixelSize(m_pMainPreview);


    obs_video_info ovi;
    obs_get_video_info(&ovi);


    int32_t previewX = m_pInitedContextGraphics->GetMainPreviewX();
    int32_t previewY = m_pInitedContextGraphics->GetMainPreviewY();
    float previewScale = m_pInitedContextGraphics->GetMainPreviewScale();


    if (isFixedScaling)
    {
        m_pMainPreview->ClampScrollingOffsets();
        previewScale = m_pMainPreview->GetScalingAmount();
        GetCenterPosFromFixedScale(
            int(cx), int(cy),
            targetSize.width() - PREVIEW_EDGE_SIZE * 2,
            targetSize.height() - PREVIEW_EDGE_SIZE * 2, previewX,
            previewY, previewScale);
        previewX += m_pMainPreview->GetScrollX();
        previewY += m_pMainPreview->GetScrollY();
    }
    else
    {
        GetScaleAndCenterPos(int(cx), int(cy),
                            targetSize.width() - PREVIEW_EDGE_SIZE * 2,
                            targetSize.height() -
                            PREVIEW_EDGE_SIZE * 2,
                            previewX, previewY, previewScale);
    }

    previewX += float(PREVIEW_EDGE_SIZE);
    previewY += float(PREVIEW_EDGE_SIZE);


    m_pInitedContextGraphics->SetMainPreviewX(previewX);
    m_pInitedContextGraphics->SetMainPreviewY(previewY);
    m_pInitedContextGraphics->SetMainPreviewScale(previewScale);
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
    m_ProgramDisplay = new AFQTDisplay();
}

void AFMainWindowRenderModel::ReleaseProgramDisplay()
{
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    
    
    OBSWeakSource lastScene = sceneContext.GetLastOBSScene();
    
    delete m_ProgramDisplay;
    
    if (lastScene) 
    {
        OBSSource actualLastScene = OBSGetStrongRef(lastScene);
        if (actualLastScene)
            obs_source_dec_showing(actualLastScene);
        lastScene = nullptr;
        sceneContext.SetLastOBSScene(nullptr);
    }

    sceneContext.SetProgramOBSScene(nullptr);
    sceneContext.SetSwapOBSScene(nullptr);

//    prevFTBSource = nullptr;
}

void AFMainWindowRenderModel::ResizeProgram(uint32_t cx, uint32_t cy)
{
    if (m_pInitedContextGraphics == nullptr ||
        m_ProgramDisplay == nullptr)
        return;
    
    QSize targetSize;

    int32_t programX = m_pInitedContextGraphics->GetProgramPreviewX();
    int32_t programY = m_pInitedContextGraphics->GetProgramPreviewY();
    float programScale = m_pInitedContextGraphics->GetProgramPreviewScale();
    
    /* resize program panel to fix to the top section of the window */
    targetSize = GetPixelSize(m_ProgramDisplay);
    GetScaleAndCenterPos(int(cx), int(cy),
                 targetSize.width() - PREVIEW_EDGE_SIZE * 2,
                 targetSize.height() - PREVIEW_EDGE_SIZE * 2,
                 programX, programY, programScale);

    programX += float(PREVIEW_EDGE_SIZE);
    programY += float(PREVIEW_EDGE_SIZE);
    
    
    m_pInitedContextGraphics->SetProgramPreviewX(programX);
    m_pInitedContextGraphics->SetProgramPreviewY(programY);
    m_pInitedContextGraphics->SetProgramPreviewScale(programScale);
}

void AFMainWindowRenderModel::SetProgramScene()
{
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    auto& confManager = AFConfigManager::GetSingletonInstance();
    auto* tmpStateApp = confManager.GetStates();
    
    
    OBSScene curScene = sceneContext.GetCurrOBSScene();

    OBSSceneAutoRelease dup;
    if (tmpStateApp->GetSceneDuplicationMode() == true)
    {
        dup = obs_scene_duplicate(
            curScene,
            obs_source_get_name(
                obs_scene_get_source(curScene)),
                tmpStateApp->GetEditPropertiesMode()
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
        //
        obs_source_t *source = obs_scene_get_source(curScene);
        obs_source_inc_showing(source);
        sceneContext.SetLastOBSScene(source);
        sceneContext.SetProgramOBSScene(source);
    }
}

void AFMainWindowRenderModel::ResetProgramScene()
{
    auto& sceneContext = AFSceneContext::GetSingletonInstance();
    OBSSource actualProgramScene = OBSGetStrongRef(sceneContext.GetProgramOBSScene());
    if (!actualProgramScene)
        actualProgramScene = AFSceneUtil::CnvtToOBSSource(sceneContext.GetCurrOBSScene());
    else
        App()->GetMainView()->GetMainWindow()->SetCurrentScene(actualProgramScene, true);
    AFSceneUtil::TransitionToScene(actualProgramScene, true);
}

void AFMainWindowRenderModel::_DrawBackdrop(float cx, float cy)
{
    if (m_pInitedContextGraphics == nullptr)
        return;
    
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

    gs_load_vertexbuffer(m_pInitedContextGraphics->GetBoxVB());
    gs_draw(GS_TRISTRIP, 0, 0);

    gs_matrix_pop();
    gs_technique_end_pass(tech);
    gs_technique_end(tech);

    gs_load_vertexbuffer(nullptr);

    GS_DEBUG_MARKER_END();
}
