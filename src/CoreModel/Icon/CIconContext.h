#pragma once

#include <map>
#include <QIcon>

class AFIconContext final
{
#pragma region QT Field, CTOR/DTOR
public:
    AFIconContext() = default;
    ~AFIconContext() = default;
#pragma endregion QT Field, CTOR/DTOR

#pragma region public func
public:
    void    InitContext();
    QIcon   GetSourceIcon(const char* id);
    QIcon   GetSceneIcon() const;
    QIcon   GetGroupIcon() const;
    QIcon   GetHotkeyConflictIcon() const;

#pragma endregion public func


#pragma region private func
private:
    void _LoadStudioIcon();

    // For Source List
    void _LoadSourceIcon(QString id);
    void _LoadSceneIcon();
    void _LoadGroupIcon();

    QIcon _GetSourceIcon(QString id) const;

    void _LoadHotkeyConflictIcon();

#pragma endregion private func

#pragma region private var
private:
    std::map<QString, QIcon> m_sourceIcons;

    QIcon m_groupIcon;
    QIcon m_sceneIcon;

    QIcon m_hotkeyConflictIcon;

#pragma endregion private var
};
