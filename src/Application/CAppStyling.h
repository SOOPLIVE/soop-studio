#ifndef AFQAPPSTYLING_H
#define AFQAPPSTYLING_H

#include <string>
#include "CApplication.h"
#include <util/util.hpp>

struct AFThemeMeta {
    bool dark;
    std::string parent;
    std::string author;
};

class AFQAppStyling final
{
#pragma region QT Field, CTOR/DTOR
public:
    static AFQAppStyling& GetSingletonInstance()
    {
        static AFQAppStyling* instance = nullptr;
        if (instance == nullptr)
            instance = new AFQAppStyling;
        return *instance;
    };
    ~AFQAppStyling() = default;
private:
    // singleton constructor
    AFQAppStyling() = default;
    AFQAppStyling(const AFQAppStyling&) = delete;
    AFQAppStyling(/* rValue */AFQAppStyling&& other) noexcept = delete;
    AFQAppStyling& operator=(const AFQAppStyling&) = delete;
    //
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    bool InitStyle(QPalette palette);
    inline const char* CurrentTheme() const { return m_CurrentTheme.c_str(); }
    std::string GetTheme(std::string name, std::string path);
    std::string SetParentTheme(std::string name);
    void ParseExtraThemeData(const char* path);
    bool SetTheme(std::string name, std::string path = "");
    static AFThemeMeta* ParseThemeMeta(const char* path);
    void AddExtraThemeColor(QPalette& pal, int group, const char* name,
        uint32_t color);
    void SetStyle(QWidget* widget);
#pragma endregion public func

#pragma region private func
private:

#pragma endregion private func
#pragma region public member var

#pragma endregion public member var
#pragma region private member var
private:
    std::string m_CurrentTheme;
    QPalette m_DefaultPalette;
    bool m_ThemeDarkMode = true;
#pragma endregion private member var

public:
};

#endif // AFQAPPSTYLING_H
