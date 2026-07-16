#pragma once

#include <string>

#include <util/util.hpp>
#include <browser-panel.hpp>


struct QCef;
struct QCefCookieManager;


class AFCefManager final
{
public:
    AFCefManager();
    ~AFCefManager();

public:
    void CheckExistingCookieId();
    void InitPanelCookieManager();
    void DestroyPanelCookieManager();
    void DeleteCookies();
         
    void DuplicateCurrentCookieProfile(ConfigFile& config);
    void InitBrowserPanelSafeBlock();
    
    QCef* GetCef() { return m_pCef; };
    QCefCookieManager* GetCefCookieManager() { return m_pPanelCookies; };

    QCefWidget* createWidget(QWidget* parent, const std::string& url,
                             QCefCookieManager* cookie_manager = nullptr,
                             const std::string& headers = "", bool dummy = true);

    void SetSoopCookie(std::string cookie);

private:
    QCef* m_pCef = nullptr;
    QCefCookieManager* m_pPanelCookies = nullptr;
    bool m_cef_js_avail = false;
};
