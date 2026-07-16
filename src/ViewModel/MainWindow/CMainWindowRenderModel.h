#pragma once

#include <QPointer>

#include "UIComponent/CQtDisplay.h"


class CBasicPreview;
class AFGraphicsContext;

class AFMainWindowRenderModel final
{
public:
    AFMainWindowRenderModel() = default;
    ~AFMainWindowRenderModel() = default;

public:
    // libobs callback
    static void RenderMain(void *data, uint32_t, uint32_t);
    static void RenderProgram(void *data, uint32_t, uint32_t);

    void SetMainPreview(CBasicPreview* pPreview) { m_pMainPreview = pPreview; };

    float GetDPIValue() { return m_dpi;};
    void SetDPIValue(float value) { m_dpi = value; };
    AFQTDisplay* GetProgramDisplay() { return m_programDisplay.get(); };
                          
    void ResizePreview(uint32_t cx, uint32_t cy);
    void RemoveCallbackMainDisplay();
         
         
    void CreateProgramDisplay();
    void ReleaseProgramDisplay();
    void ResizeProgram(uint32_t cx, uint32_t cy);
         
    void SetProgramScene();
    void ResetProgramScene();
                          
private:                  
    static void _DrawBackdrop(float cx, float cy);
                          
private:                  
    float m_dpi = 1.0;
    CBasicPreview* m_pMainPreview = nullptr;
    QPointer<AFQTDisplay> m_programDisplay;
};