/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <string>

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define VERSION_MAJOR               1
#define VERSION_MINOR               8
#define VERSION_REVISION            0
#define VERSION_BUILD               0

// TODOHERE: APPLICATION_VERSION_STR, APPLICATION_NAME, APPLICATION_NAME_WITH_VERSION
#define VER_PRODUCT_VERSION_STR     STRINGIZE(VERSION_MAJOR)        \
                                    "." STRINGIZE(VERSION_MINOR)    \
                                    "." STRINGIZE(VERSION_REVISION) \
                                    "." STRINGIZE(VERSION_BUILD)    \

#define VER_PRODUCTNAME_STR                 "Floating Sandbox"
#define VER_PRODUCTNAME_WITH_VERSION_STR    VER_PRODUCTNAME_STR " " VER_PRODUCT_VERSION_STR

#define VERSION STRINGIZE(VERSION_MAJOR) "." STRINGIZE(VERSION_MINOR) "." STRINGIZE(VERSION_REVISION)

inline std::string ApplicationName = VER_PRODUCTNAME_STR;

enum class VersionFormat
{
    Short,
    Long,
    LongWithDate
};

inline std::string GetVersionInfo(VersionFormat versionFormat)
{
    switch (versionFormat)
    {
        case VersionFormat::Short:
        {
            return std::string(VERSION);
        }

        case VersionFormat::Long:
        {
            return std::string(ApplicationName + " v" VERSION);
        }

        case VersionFormat::LongWithDate:
        {
            return std::string(ApplicationName + " v" VERSION " (" __DATE__ ")");
        }

        default:
        {
            assert(false);
            return "";
        }
    }
}
