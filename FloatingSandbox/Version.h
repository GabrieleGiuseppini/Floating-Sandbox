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

#define APPLICATION_VERSION_MAJOR               1
#define APPLICATION_VERSION_MINOR               11
#define APPLICATION_VERSION_REVISION            1
#define APPLICATION_VERSION_BUILD               0

#define APPLICATION_VERSION_LONG_STR    STRINGIZE(APPLICATION_VERSION_MAJOR)        \
                                        "." STRINGIZE(APPLICATION_VERSION_MINOR)    \
                                        "." STRINGIZE(APPLICATION_VERSION_REVISION) \
                                        "." STRINGIZE(APPLICATION_VERSION_BUILD)

#define APPLICATION_VERSION_SHORT_STR   STRINGIZE(APPLICATION_VERSION_MAJOR)        \
                                        "." STRINGIZE(APPLICATION_VERSION_MINOR)    \
                                        "." STRINGIZE(APPLICATION_VERSION_REVISION)

#define APPLICATION_NAME                 "Floating Sandbox"
#define APPLICATION_NAME_WITH_VERSION    APPLICATION_NAME " " APPLICATION_VERSION_SHORT_STR

inline std::string ApplicationName = APPLICATION_NAME;
inline std::string ApplicationNameWithVersion = APPLICATION_NAME_WITH_VERSION;

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
            return std::string(APPLICATION_VERSION_LONG_STR);
        }

        case VersionFormat::Long:
        {
            return std::string(APPLICATION_NAME_WITH_VERSION);
        }

        case VersionFormat::LongWithDate:
        {
            return std::string(APPLICATION_NAME_WITH_VERSION " (" __DATE__ ")");
        }

        default:
        {
            assert(false);
            return "";
        }
    }
}
