/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-07-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SysSpecifics.h"

#if FS_IS_ARCHITECTURE_ARM_32()
#pragma message ("ARCHITECTURE:FS_ARCHITECTURE_ARM_32")
#elif FS_IS_ARCHITECTURE_ARM_64()
#pragma message ("ARCHITECTURE:FS_ARCHITECTURE_ARM_64")
#elif FS_IS_ARCHITECTURE_X86_32()
#pragma message ("ARCHITECTURE:FS_ARCHITECTURE_X86_32")
#elif FS_IS_ARCHITECTURE_X86_64()
#pragma message ("ARCHITECTURE:FS_ARCHITECTURE_X86_64")
#else
#pragma message ("ARCHITECTURE:<UNKNOWN>")
#endif

#if FS_IS_OS_LINUX()
#pragma message ("OS:FS_OS_LINUX")
#elif FS_IS_OS_MACOS()
#pragma message ("OS:FS_OS_MACOS")
#elif FS_IS_OS_WINDOWS()
#pragma message ("OS:FS_OS_WINDOWS")
#else
#pragma message ("OS:<UNKNOWN>")
#endif