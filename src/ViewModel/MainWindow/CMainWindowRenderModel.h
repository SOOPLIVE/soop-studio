#pragma once

#include <QPointer>

#include "UIComponent/CQtDisplay.h"


class AFBasicPreview;
class AFConfigManager;
class AFGraphicsContext;
class AFSceneContext;


class AFMainWindowRenderModel final
{
#pragma region QT Field, CTOR/DTOR
public:
    AFMainWindowRenderModel() = default;
    ~AFMainWindowRenderModel() = default;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    // libobs callback
    static void                                 RenderMain(void *data, uint32_t, uint32_t);
    static void                                 RenderProgram(void *data, uint32_t, uint32_t);
    //
    
    void                                        SetUnsafeAccessContext(AFConfigManager* pConfigManager,
                                                                       AFGraphicsContext* pGraphicsContext,
                                                                       AFSceneContext* pScnenContext)
                                                {
                                                    m_pInitedConfigManager = pConfigManager;
                                                    m_pInitedContextGraphics = pGraphicsContext;
                                                    m_pInitedContextScene = pScnenContext;
                                                };
    void                                        SetMainPreview(AFBasicPreview* pPreview)
                                                {
                                                    m_pMainPreview = pPreview;
                                                };
    
    float                                       GetDPIValue() { return m_fDpi;};
    void                                        SetDPIValue(float value) { m_fDpi = value; };
    AFQTDisplay*                                GetProgramDisplay() { return m_ProgramDisplay.get(); };
    
    void                                        ResizePreview(uint32_t cx, uint32_t cy);
    void                                        RemoveCallbackMainDisplay();
    
    
    void                                        CreateProgramDisplay();
    void                                        ReleaseProgramDisplay();
    void                                        ResizeProgram(uint32_t cx, uint32_t cy);
    
    void                                        SetProgramScene();
    void                                        ResetProgramScene();
#pragma endregion public func

    
#pragma region private func
private:
    static void                                 _DrawBackdrop(float cx, float cy);
#pragma region private func


#pragma region private member var
private:
    // Fast Access Context
    static AFConfigManager*                     m_pInitedConfigManager;
    static AFGraphicsContext*                   m_pInitedContextGraphics;
    static AFSceneContext*                      m_pInitedContextScene;
    //
    
    float                                       m_fDpi = 1.0;
    AFBasicPreview*                             m_pMainPreview = nullptr;
    QPointer<AFQTDisplay>                       m_ProgramDisplay;
#pragma endregion private member var
};
