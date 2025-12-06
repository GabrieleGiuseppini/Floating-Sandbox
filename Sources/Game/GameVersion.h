/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-01-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/Version.h>

#include <cassert>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define APPLICATION_VERSION_MAJOR               1
#define APPLICATION_VERSION_MINOR               19
#define APPLICATION_VERSION_PATCH               2
#define APPLICATION_VERSION_BUILD               4

#define APPLICATION_VERSION_LONG_STR    STRINGIZE(APPLICATION_VERSION_MAJOR)        \
                                        "." STRINGIZE(APPLICATION_VERSION_MINOR)    \
                                        "." STRINGIZE(APPLICATION_VERSION_PATCH)    \
                                        "." STRINGIZE(APPLICATION_VERSION_BUILD)

#define APPLICATION_VERSION_SHORT_STR   STRINGIZE(APPLICATION_VERSION_MAJOR)        \
                                        "." STRINGIZE(APPLICATION_VERSION_MINOR)    \
                                        "." STRINGIZE(APPLICATION_VERSION_PATCH)

#define APPLICATION_NAME                     "Floating Sandbox"
#define APPLICATION_NAME_WITH_SHORT_VERSION  APPLICATION_NAME " " APPLICATION_VERSION_SHORT_STR
#define APPLICATION_NAME_WITH_LONG_VERSION   APPLICATION_NAME " " APPLICATION_VERSION_LONG_STR

#define APPLICATION_DOWNLOAD_URL        "https://gamejolt.com/games/floating-sandbox/353572"

inline std::string ApplicationName = APPLICATION_NAME;

//////////////////////////////////////////////////////////////

Version constexpr CurrentGameVersion = Version(
    APPLICATION_VERSION_MAJOR,
    APPLICATION_VERSION_MINOR,
    APPLICATION_VERSION_PATCH,
    APPLICATION_VERSION_BUILD);
