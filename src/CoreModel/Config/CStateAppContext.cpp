#include "CStateAppContext.h"
#include "Application/CApplication.h"

#include "MainFrame/CMainFrame.h"



bool AFStateAppContext::SetPreviewProgramMode(bool value)
{
    if (IsPreviewProgramMode() == value)
        return false;
    
    os_atomic_set_bool(&m_previewProgramMode, value);
    
    if (IsPreviewProgramMode()) {}
    else 
    {
        MAINFRAME->EnableTransitionState(true);
    }

    return true;
}
