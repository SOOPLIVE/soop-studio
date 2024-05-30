#include "CStateAppContext.h"




bool AFStateAppContext::SetPreviewProgramMode(bool value)
{
    if (IsPreviewProgramMode() == value)
        return false;
    
    os_atomic_set_bool(&m_bPreviewProgramMode, value);
    
    return true;
}
