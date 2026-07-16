#include "CCefManager.h"

#include <random>

#include <QDir>
#include <QRegularExpression>

#include "qt-wrappers.hpp"

#include "Common/StudioDefine.h"
#include "Common/StringMiscUtils.h"

#include "Application/CApplication.h"

#include "CoreModel/Auth/CAuthManager.h"
#include "CoreModel/Locale/CLocaleTextManager.h"

#include "MainFrame/CMainFrame.h"


AFCefManager::AFCefManager()
    :m_pCef(obs_browser_init_panel()),
    m_cef_js_avail(m_pCef&& obs_browser_qcef_version() >= 3)
{}
AFCefManager::~AFCefManager()
{
    DestroyPanelCookieManager();
    if(m_pCef) {
        delete m_pCef;
        m_pCef = nullptr;
    }
}

void AFCefManager::CheckExistingCookieId()
{
    if (config_has_user_value(ACTIVECONFIG, "Panels", "CookieId"))
        return;

    config_set_string(ACTIVECONFIG, "Panels", "CookieId", GenId().c_str());
}

void AFCefManager::InitPanelCookieManager()
{
    if (!m_pCef)
        return;
    if (m_pPanelCookies)
        return;

    CheckExistingCookieId();

    const char* cookie_id = config_get_string(ACTIVECONFIG, "Panels", "CookieId");

    std::string sub_path;
    sub_path += "ANENTAStudio_profile_cookies/";
    sub_path += cookie_id;

    std::string dst_path = "";

    BPtr<char> src_path_full = m_pCef->get_cookie_path(sub_path);
    BPtr<char> dst_path_full = m_pCef->get_cookie_path(dst_path);

    QDir srcDir(QString::fromUtf8(src_path_full.Get()));
    QDir dstDir(QString::fromUtf8(dst_path_full.Get()));

    if (srcDir.exists())
    {
        if (!dstDir.exists())
            dstDir.mkdir(".");

        QStringList files = srcDir.entryList(QDir::Files);
        for (const QString& file : files)
        {
            QString src = QString(src_path_full) + QDir::separator() + file;
            QString dst = QString(dst_path_full) + QDir::separator() + file;
            QFile::copy(src, dst);
        }

        // copy leveldb
        std::string sub_localstorage_path;
        sub_localstorage_path += "ANENTAStudio_profile_cookies/";
        sub_localstorage_path += cookie_id;
        sub_localstorage_path += "/Local Storage/leveldb/";

        std::string dst_localstorage_path = "/Local Storage/leveldb";

        BPtr<char> src_localstorage_path_full = m_pCef->get_cookie_path(sub_localstorage_path);
        BPtr<char> dst_localstorage_path_full = m_pCef->get_cookie_path(dst_localstorage_path);

        QString srcLevelDB = QString::fromUtf8(src_localstorage_path_full.Get());
        QString dstLevelDB = QString::fromUtf8(dst_localstorage_path_full.Get());
        QDir srcLevelDBDir(srcLevelDB);
        if (srcLevelDBDir.exists()) {
            QDir dstLevelDBDir(dstLevelDB);
            if (!dstLevelDBDir.exists())
                dstLevelDBDir.mkpath(".");

            QStringList levelDBfiles = srcLevelDBDir.entryList(QDir::Files);
            for (const QString& file : levelDBfiles)
            {
                QString src = srcLevelDB + QDir::separator() + file;
                QString dst = dstLevelDB + QDir::separator() + file;

                if (QFile::exists(dst)) {
                    QFile::remove(dst);
                }

                QFile::copy(src, dst);
            }
        }
        srcDir.removeRecursively();
    }

    std::string root_path;
    root_path += "ANENTAStudio_profile_cookies";
    BPtr<char> root_path_full = m_pCef->get_cookie_path(root_path);

    QDir rootDir(root_path_full.Get());
    if (rootDir.exists())
        rootDir.removeRecursively();

    m_pPanelCookies = m_pCef->create_cookie_manager(dst_path);

    AFChannelData* data = nullptr;
    if (AUTH_CONTEXT.GetChannelData(PLATFORM_SOOP, data))
    {
        if (data)
        {
            std::string strCookie = data->pAuthData->cookie;
            SetSoopCookie(strCookie);
        }
    }
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

    std::string cookie_id = config_get_string(ACTIVECONFIG, "Panels", "CookieId");

    std::string src_path;
    src_path += "ANENTAStudio_profile_cookies/";
    src_path += cookie_id;

    std::string new_id = GenId();

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

    config_set_string(config, "Panels", "CookieId", cookie_id.c_str());
    config_set_string(ACTIVECONFIG, "Panels", "CookieId", new_id.c_str());
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

    ExecThreadedWithoutBlocking([this] { m_pCef->wait_for_browser_init(); },
                                QTStr("BrowserPanelInit.Title"),
                                QTStr("BrowserPanelInit.Text"));
    InitPanelCookieManager();
}
//
QCefWidget* AFCefManager::createWidget(QWidget* parent, const std::string& url, QCefCookieManager* cookie_manager, const std::string& headers, bool dummy)
{
    if(!m_pCef)
        return nullptr;

    return m_pCef->create_widget(parent, url, cookie_manager, headers, dummy);
}

void AFCefManager::SetSoopCookie(std::string cookie)
{
    if (!m_pCef)
        return;

    if (!m_pPanelCookies)
        return;

    QString trimmedCookie = cookie.c_str();
    trimmedCookie.remove(QRegularExpression("\\s"));
    std::string cookie_ = trimmedCookie.toStdString();

	m_pPanelCookies->SetCookies(SOOPLIVE_SET_KR_URL, cookie_);

    MAINFRAME->OnSoopEvent(SOOP_FRONTEND_SET_CEF_COOKIES, (void*)cookie_.c_str());
}