/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GPUCalculator.h"

#include <GameCore/Vectors.h>

#include <filesystem>

class TestGPUCalculator : public GPUCalculator
{
public:

    void Add(
        vec2f const * a,
        vec2f const * b,
        vec2f * result);

private:

    friend class GPUCalculatorFactory;

    TestGPUCalculator(
        std::unique_ptr<IOpenGLContext> openGLContext,
        std::filesystem::path const & shadersRootDirectory,
        size_t dataPoints);

private:

    size_t const mDataPoints;

    GameOpenGLVBO mVertexVBO;
    GameOpenGLFramebuffer mFramebuffer;
    GameOpenGLRenderbuffer mColorRenderbuffer;
};