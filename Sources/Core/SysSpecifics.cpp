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
#elif FS_IS_OS_ANDROID()
#pragma message ("OS:FS_OS_ANDROID")
#else
#pragma message ("OS:<UNKNOWN>")
#endif

#if FS_IS_PLATFORM_PC()
#pragma message ("PLATFORM:FS_PLATFORM_PC")
#elif FS_IS_PLATFORM_MOBILE()
#pragma message ("PLATFORM:FS_PLATFORM_MOBILE")
#else
#pragma message ("PLATFORM:<UNKNOWN>")
#endif

#define STR1(x) #x
#define STR(x) STR1(x)
#pragma message ("ARM NEON:" STR(FS_IS_ARM_NEON()))