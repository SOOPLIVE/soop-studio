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
#pragma region QT Field, CTOR/DTOR
public:
    static AFLocaleTextManager& GetSingletonInstance()
    {
        static AFLocaleTextManager* instance = nullptr;
        if (instance == nullptr)
            instance = new AFLocaleTextManager;
        return *instance;
    };
    ~AFLocaleTextManager() {};
private:
    // singleton constructor
    AFLocaleTextManager() = default;
    AFLocaleTextManager(const AFLocaleTextManager&) = delete;
    AFLocaleTextManager(/* rValue */AFLocaleTextManager&& other) noexcept = delete;
    AFLocaleTextManager& operator=(const AFLocaleTextManager&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    static tLOCALE_NAME                 GetLocaleNames();


    bool                                InitLocale();

    std::string&                        GetCurrentLocaleStr() { return m_strCurrLocale; };
    inline const char*                  GetCurrentLocale() { return m_strCurrLocale.c_str(); };  
    inline lookup_t*                    GetTextLookup() const { return m_BaseLookup; };    
    inline const char*                  Str(const char* lookup) { return _GetString(lookup); };

    bool                                TranslateString(const char* lookupVal, const char** out) const;
#pragma endregion public func

#pragma region private func
private:
    inline const char*                  _GetString(const char* lookupValue) const;

#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
    std::deque<obs_frontend_translate_ui_cb>    m_queTranslatorHooks;

    std::string                                 m_strCurrLocale;

    TextLookup                                  m_BaseLookup;
#pragma endregion private member var
};


inline const char* AFLocaleTextManager::_GetString(const char* lookupValue) const
{
    return m_BaseLookup.GetString(lookupValue);
};

