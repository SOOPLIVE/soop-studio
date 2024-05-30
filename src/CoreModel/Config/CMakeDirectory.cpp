#include "CMakeDirectory.h"

#include <util/platform.h>

#include "include/qt-wrapper.h"

#include "CoreModel/Config/CConfigManager.h"

bool AFMakeDirectory::MakeUserDirs()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    char path[512];

    if (confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/basic") <= 0)
        return false;
    if (!_DoMkDir(path))
        return false;

    if (confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/logs") <= 0)
        return false;
    if (!_DoMkDir(path))
        return false;

    if (confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/profiler_data") <= 0)
        return false;
    if (!_DoMkDir(path))
        return false;

#ifdef _WIN32
    if (confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/crashes") <= 0)
        return false;
    if (!_DoMkDir(path))
        return false;
#endif

#ifdef WHATSNEW_ENABLED
    if (confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/updates") <= 0)
        return false;
    if (!_DoMkDir(path))
        return false;
#endif

    if (confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/plugin_config") <= 0)
        return false;
    if (!_DoMkDir(path))
        return false;

    
    return true;
}

bool AFMakeDirectory::MakeUserProfileDirs()
{
    auto& confManager = AFConfigManager::GetSingletonInstance();
    
    char path[512];

    if (confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/basic/profiles") <= 0)
        return false;
    if (!_DoMkDir(path))
        return false;

    if (confManager.GetConfigPath(path, sizeof(path), "SOOPStudio/basic/scenes") <= 0)
        return false;
    if (!_DoMkDir(path))
        return false;

    
    return true;
}

bool AFMakeDirectory::_DoMkDir(const char* path)
{
    if (os_mkdirs(path) == MKDIR_ERROR)
    {
        AFErrorBox(NULL, "Failed to create directory %s", path);
        return false;
    }

    
    return true;
}
