#pragma once


#include "CMainWindowRenderModel.h"


#include "Common/TSafeSingleton.h"



class AFMainWindowAccesser final
{
#pragma region QT Field, CTOR/DTOR
public:
    AFMainWindowAccesser() = default;
    ~AFMainWindowAccesser() = default;;
#pragma endregion QT Field, CTOR/DTOR

#pragma region private member var
public:
    AFMainWindowRenderModel         m_RenderModel;
    
    
    
    
    
    
#pragma endregion private member var
};


extern AFSafeSingleton<AFMainWindowAccesser>    g_ViewModelsDynamic;
