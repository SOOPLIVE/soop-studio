#pragma once


#include <string>
#include <vector>
#include <queue>

#include <obs-frontend-api.h>
#include <util/util.hpp>


// Def Type
typedef std::vector<std::pair<std::string, std::string>> tLOCALE_NAME;



// Forward

class AFLocaleTextManager final
{
public:
    AFLocaleTextManager() = default;
    ~AFLocaleTextManager() = default;

public:
    static tLOCALE_NAME GetLocaleNames();

    bool InitLocale();

    std::string& GetCurrentLocaleStr() { return m_currLocale; };
    inline const char* GetCurrentLocale() { return m_currLocale.c_str(); };  
    inline lookup_t* GetTextLookup() const { return m_baseLookup; };    
    inline const char* Str(const char* lookup) { return getString(lookup); };

    bool TranslateString(const char* lookupVal, const char** out) const;

private:
    const char* getString(const char* lookupValue) const;

private:
    std::deque<obs_frontend_translate_ui_cb>    m_translatorHooks;

    std::string m_currLocale;

    TextLookup m_baseLookup;
};
