/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-30
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameException.h"

#ifdef _DEBUG

inline void Verify(bool expression)
{
    if (!expression)
    {
        throw GameException("Verification failed!");
    }
}

#endif