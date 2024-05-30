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
	bool			GetEnableHotkeysInFocus() { return m_bEnableHotkeysInFocus; };
	void			SetEnableHotkeysInFocus(bool value) { m_bEnableHotkeysInFocus = value; };
	bool			GetEnableHotkeysOutOfFocus() { return m_bEnableHotkeysOutOfFocus; };
	void			SetEnableHotkeysOutOfFocus(bool value) { m_bEnableHotkeysOutOfFocus = value; };
    bool            GetEditPropertiesMode() { return m_bEditPropertiesMode; };
    void            SetEditPropertiesMode(bool value) { m_bEditPropertiesMode = value; };
    bool            GetSceneDuplicationMode() { return m_bSceneDuplicationMode; };
    void            SetSceneDuplicationMode(bool value) { m_bSceneDuplicationMode = value; };
    bool            GetSwapScenesMode() { return m_bSwapScenesMode; };
    void            SetSwapScenesMode(bool value) { m_bSwapScenesMode = value; };
    bool            JustCheckPreviewProgramMode() { return m_bPreviewProgramMode; };
    inline bool     IsPreviewProgramMode() const { return os_atomic_load_bool(&m_bPreviewProgramMode); }
    bool            SetPreviewProgramMode(bool value);
#pragma endregion public func

#pragma region private func
#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
	bool			m_bEnableHotkeysInFocus = true;
	bool			m_bEnableHotkeysOutOfFocus = true;
    bool            m_bEditPropertiesMode = false;
    bool            m_bSceneDuplicationMode = true;
    bool            m_bSwapScenesMode = true;
    
    volatile bool   m_bPreviewProgramMode = false;
#pragma endregion private member var
};
