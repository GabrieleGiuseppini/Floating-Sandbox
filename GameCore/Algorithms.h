/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-09-09
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "SysSpecifics.h"
#include "Vectors.h"

namespace Algorithms {

inline void CalculateVectorLengthsAndDirs(
    float * restrict inOutXBuffer,
    float * restrict inOutYBuffer,
    float * restrict outLengthBuffer,
    size_t elementCount)
{
    // TODOHERE
    for (size_t i = 0; i < elementCount; ++i)
    {
        float d = std::sqrt(inOutXBuffer[i] * inOutXBuffer[i] + inOutYBuffer[i] * inOutYBuffer[i]);
        inOutXBuffer[i] /= d;
        inOutYBuffer[i] /= d;
        outLengthBuffer[i] = d;
    }
}

}