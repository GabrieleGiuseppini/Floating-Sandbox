/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-09-03
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#ifdef _MSC_VER
#include <float.h>
#endif

#include <xmmintrin.h>

inline void EnableFloatingPointExceptions()
{
#ifdef _MSC_VER
    // Enable all floating point exceptions except these
    unsigned int fp_control_state = _controlfp(_EM_INEXACT | _EM_UNDERFLOW, _MCW_EM);
    (void)fp_control_state;
#else
    // Have no idea how to do this on other compilers...
#endif
}

inline void EnableFloatingPointFlushToZero()
{
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
}
