#pragma once

#include <string>

#include <util/util.hpp>
#include <browser-panel.hpp>


struct QCef;
struct QCefCookieManager;


class AFCefManager final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFCefManager& GetSingletonInstance()
    {
        static AFCefManager* instance = nullptr;
        if (instance == nullptr)
            instance = new AFCefManager;
        return *instance;
    };
    ~AFCefManager();
private:
    // singleton constructor
    AFCefManager() = default;
    AFCefManager(const AFCefManager&) = delete;
    AFCefManager(/* rValue */AFCefManager&& other) noexcept = delete;
    AFCefManager& operator=(const AFCefManager&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void                                InitContext();
    void                                FinContext();
    
    
    void                                CheckExistingCookieId();
    void                                InitPanelCookieManager();
    void                                DestroyPanelCookieManager();
    void                                DeleteCookies();
    
    void                                DuplicateCurrentCookieProfile(ConfigFile& config);
    void                                InitBrowserPanelSafeBlock();
    
    
    QCef*                               GetCef() { return m_pCef; };
    QCefCookieManager*                  GetCefCookieManager() { return m_pPanelCookies; };
#pragma endregion public func

#pragma region private func
private:
    std::string                         _GenId();
#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
    QCef*                               m_pCef = nullptr;
    QCefCookieManager*                  m_pPanelCookies = nullptr;
    bool                                m_cef_js_avail = false;
#pragma endregion private member var
};
