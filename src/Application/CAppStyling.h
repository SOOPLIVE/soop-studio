#ifndef AFQAPPSTYLING_H
#define AFQAPPSTYLING_H

#include <string>

#include <obs.hpp>
#include <qwidget.h>

struct AFThemeMeta {
    bool dark;
    std::string parent;
    std::string author;
};

class CAppStyling final
{
public:
    CAppStyling() {}
    ~CAppStyling() {}

public:
    bool                InitStyle(QPalette palette);
    inline const char*  CurrentTheme() const { return m_currentTheme.c_str(); }
    std::string         GetTheme(std::string name, std::string path);
    std::string         SetParentTheme(std::string name);
    void                ParseExtraThemeData(const char* path);
    bool                SetTheme(std::string name, std::string path = "");
    static AFThemeMeta* ParseThemeMeta(const char* path);
    void                AddExtraThemeColor(QPalette& pal, int group, const char* name,
                                           uint32_t color, bool colorAlpha = false);
    void                AssignColorPalette(QPalette& pal, QPalette::ColorRole role, uint color,
                                           QPalette::ColorGroup group, bool colorAlpha = false);
    void                SetStyle(QWidget* widget);

private:
    std::string         m_currentTheme;
    QPalette            m_defaultPalette;
    bool                m_themeDarkMode = true;

public:
};

#endif // AFQAPPSTYLING_H
