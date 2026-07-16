#include "CMakeDirectory.h"

#include "qt-wrappers.hpp"
#include "util/platform.h"

#include "Application/CApplication.h"

#include "Common/StudioDefine.h"

//
namespace AFMakeDirectoryUtil
{
    bool MakeUserDirs()
    {
        char path[512] = { 0, };
        if (GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/basic").c_str()) <= 0)
            return false;
        if (!DoMkDir(path))
            return false;

        if (GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/logs").c_str()) <= 0)
            return false;
        if (!DoMkDir(path))
            return false;

        if (GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/logs/api").c_str()) <= 0)
            return false;
        if (!DoMkDir(path))
            return false;

        if (GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/profiler_data").c_str()) <= 0)
            return false;
        if (!DoMkDir(path))
            return false;

#ifdef _WIN32
        if (GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/crashes").c_str()) <= 0)
            return false;
        if (!DoMkDir(path))
            return false;
#endif

#ifdef WHATSNEW_ENABLED
        if (GetAppConfigPath(path, sizeof(path), "SOOPStudio/updates") <= 0)
            return false;
        if (!_DoMkDir(path))
            return false;
#endif

        if (GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/plugin_config").c_str()) <= 0)
            return false;
        if (!DoMkDir(path))
            return false;


        return true;
    }

    bool MakeUserProfileDirs()
    {
        char path[512];

        if (GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/basic/profiles").c_str()) <= 0)
            return false;
        if (!DoMkDir(path))
            return false;

        if (GetAppConfigPath(path, sizeof(path), (LOCAL_FOLDER_NAME + "/basic/scenes").c_str()) <= 0)
            return false;
        if (!DoMkDir(path))
            return false;


        return true;
    }
    bool DoMkDir(const char* path)
    {
        if (os_mkdirs(path) == MKDIR_ERROR)
        {
            OBSErrorBox(NULL, "Failed to create directory %s", path);
            return false;
        }
        return true;
    }
}