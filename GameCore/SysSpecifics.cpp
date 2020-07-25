/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-07-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SysSpecifics.h"

#if defined(FS_ARCHITECTURE_ARM)
#pragma message ("ARCHITECTURE:FS_ARCHITECTURE_ARM")
#elif defined(FS_ARCHITECTURE_X86_32)
#pragma message ("ARCHITECTURE:FS_ARCHITECTURE_X86_32")
#elif defined(FS_ARCHITECTURE_X86_64)
#pragma message ("ARCHITECTURE:FS_ARCHITECTURE_X86_64")
#else
#pragma message ("ARCHITECTURE:<UNKNOWN>")
#endif

#if defined(FS_OS_LINUX)
#pragma message ("OS:FS_OS_LINUX")
#elif defined(FS_OS_MACOS)
#pragma message ("OS:FS_OS_LINUX")
#elif defined(FS_OS_WINDOWS)
#pragma message ("OS:FS_OS_WINDOWS")
#else
#pragma message ("OS:<UNKNOWN>")
#endif