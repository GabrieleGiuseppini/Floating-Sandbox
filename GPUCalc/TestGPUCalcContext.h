/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GPUCalcContext.h"

#include <GameCore/Vectors.h>

class TestGPUCalcContext : public GPUCalcContext
{
public:

    void Add(
        vec2f const * a,
        vec2f const * b,
        vec2f * result);

private:

    friend class GPUCalcContextFactory;

    TestGPUCalcContext(
        std::unique_ptr<IOpenGLContext> openGLContext,
        size_t dataPoints);

private:

    size_t const mDataPoints;
};