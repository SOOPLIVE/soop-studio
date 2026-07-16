#pragma once

#include <string>

#if _WIN32
#include <util/threading-windows.h>
#else
#include <util/threading-posix.h>
#endif

// Forward



class AFStateAppContext final
{
#pragma region QT Field, CTOR/DTOR
public:
    AFStateAppContext() = default;
	~AFStateAppContext() = default;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    bool            GetEditPropertiesMode() { return m_editPropertiesMode; };
    void            SetEditPropertiesMode(bool value) { m_editPropertiesMode = value; };
    bool            GetSceneDuplicationMode() { return m_sceneDuplicationMode; };
    void            SetSceneDuplicationMode(bool value) { m_sceneDuplicationMode = value; };
    bool            GetSwapScenesMode() { return m_swapScenesMode; };
    void            SetSwapScenesMode(bool value) { m_swapScenesMode = value; };
    bool            JustCheckPreviewProgramMode() { return m_previewProgramMode; };
    inline bool     IsPreviewProgramMode() const { return os_atomic_load_bool(&m_previewProgramMode); }
    bool            SetPreviewProgramMode(bool value);
#pragma endregion public func

#pragma region private func
#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
    bool            m_editPropertiesMode = false;
    bool            m_sceneDuplicationMode = true;
    bool            m_swapScenesMode = false;
    
    volatile bool   m_previewProgramMode = false;
#pragma endregion private member var
};
