/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-04-04
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SysSpecifics.h"

#include <sstream>
#include <string>

class BuildInfo
{
public:

    static BuildInfo GetBuildInfo()
    {
        return BuildInfo();
    }

    std::string ToString() const
    {
        std::stringstream ss;

        ss
            << mArchitecture << " "
            << mOS << " "
            << mBuildFlavor << " "
            << "(" << mBuildDate << ")";

        return ss.str();
    }

private:

    BuildInfo()
    {
#if FS_IS_ARCHITECTURE_ARM_32()
        mArchitecture = "ARM 32-bit";
#elif FS_IS_ARCHITECTURE_ARM_64()
        mArchitecture = "ARM 64-bit";
#elif FS_IS_ARCHITECTURE_X86_32()
        mArchitecture = "x86 32-bit";
#elif FS_IS_ARCHITECTURE_X86_64()
        mArchitecture = "x86 64-bit";
#else
        mArchitecture = "<ARCH?>";
#endif

#if FS_IS_OS_ANDROID()
        mOS = "Android";
#elif FS_IS_OS_LINUX()
        mOS = "Linux";
#elif FS_IS_OS_MACOS()
        mOS = "MacOS";
#elif FS_IS_OS_WINDOWS()
        mOS = "Windows";
#else
        mOS = "<OS?>";
#endif

        mBuildDate = __DATE__;

#ifdef _DEBUG
        mBuildFlavor = "DEBUG";
#else
        mBuildFlavor = "RELEASE";
#endif
    }

    std::string mArchitecture;
    std::string mOS;
    std::string mBuildDate;
    std::string mBuildFlavor;
};