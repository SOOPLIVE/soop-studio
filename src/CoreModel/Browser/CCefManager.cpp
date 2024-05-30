#include "CCefManager.h"

#include <random>

#include <QDir>

#include "qt-wrapper.h"


#include "CoreModel/Config/CConfigManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"


void AFCefManager::InitContext()
{
    m_pCef = obs_browser_init_panel();
    m_cef_js_avail = m_pCef && obs_browser_qcef_version() >= 3;
}

void AFCefManager::FinContext()
{
    DestroyPanelCookieManager();
    delete m_pCef;
    m_pCef = nullptr;
}

void AFCefManager::CheckExistingCookieId()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    if (config_has_user_value(confManager.GetBasic(), "Panels", "CookieId"))
        return;

    config_set_string(confManager.GetBasic(), "Panels", "CookieId",
                      _GenId().c_str());
}

void AFCefManager::InitPanelCookieManager()
{
    if (!m_pCef)
        return;
    if (m_pPanelCookies)
        return;

    CheckExistingCookieId();


    
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    const char* cookie_id =
        config_get_string(confManager.GetBasic(), "Panels", "CookieId");

    std::string sub_path;
    sub_path += "ANENTAStudio_profile_cookies/";
    sub_path += cookie_id;

    m_pPanelCookies = m_pCef->create_cookie_manager(sub_path);
}

void AFCefManager::DestroyPanelCookieManager()
{
    if (m_pPanelCookies)
    {
        m_pPanelCookies->FlushStore();
        delete m_pPanelCookies;
        m_pPanelCookies = nullptr;
    }
}

void AFCefManager::DeleteCookies()
{
    if (m_pPanelCookies)
        m_pPanelCookies->DeleteCookies("", "");
}

void AFCefManager::DuplicateCurrentCookieProfile(ConfigFile &config)
{
    if (!m_pCef)
        return;

    
    auto& confManager = AFConfigManager::GetSingletonInstance();

    std::string cookie_id =
        config_get_string(confManager.GetBasic(), "Panels", "CookieId");

    std::string src_path;
    src_path += "ANENTAStudio_profile_cookies/";
    src_path += cookie_id;

    std::string new_id = _GenId();

    std::string dst_path;
    dst_path += "ANENTAStudio_profile_cookies/";
    dst_path += new_id;

    BPtr<char> src_path_full = m_pCef->get_cookie_path(src_path);
    BPtr<char> dst_path_full = m_pCef->get_cookie_path(dst_path);

    QDir srcDir(src_path_full.Get());
    QDir dstDir(dst_path_full.Get());

    if (srcDir.exists())
    {
        if (!dstDir.exists())
            dstDir.mkdir(dst_path_full.Get());

        QStringList files = srcDir.entryList(QDir::Files);
        for (const QString &file : files)
        {
            QString src = QString(src_path_full);
            QString dst = QString(dst_path_full);
            src += QDir::separator() + file;
            dst += QDir::separator() + file;
            QFile::copy(src, dst);
        }
    }

    config_set_string(config, "Panels", "CookieId",
                      cookie_id.c_str());
    config_set_string(confManager.GetBasic(), "Panels", "CookieId",
                      new_id.c_str());
}

void AFCefManager::InitBrowserPanelSafeBlock()
{
    if (!m_pCef)
        return;
    if (m_pCef->init_browser())
    {
        InitPanelCookieManager();
        return;
    }

    
    auto& localeText = AFLocaleTextManager::GetSingletonInstance();
    
    ExecThreadedWithoutBlocking([this] { m_pCef->wait_for_browser_init(); },
                                QT_UTF8(localeText.Str("BrowserPanelInit.Title")),
                                QT_UTF8(localeText.Str("BrowserPanelInit.Text")));
    InitPanelCookieManager();
}

std::string AFCefManager::_GenId()
{
    std::random_device rd;
    std::mt19937_64 e2(rd());
    std::uniform_int_distribution<uint64_t> dist(0, 0xFFFFFFFFFFFFFFFF);

    uint64_t id = dist(e2);

    char id_str[20];
    snprintf(id_str, sizeof(id_str), "%16llX", (unsigned long long)id);
    return std::string(id_str);
}

