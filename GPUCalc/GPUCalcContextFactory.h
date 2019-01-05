/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GPUCalcContext.h"
#include "IOpenGLContext.h"
#include "TestGPUCalcContext.h"

#include <cassert>
#include <functional>
#include <memory>

class GPUCalcContextFactory
{
public:

    static GPUCalcContextFactory & GetInstance()
    {
        static GPUCalcContextFactory * instance = new GPUCalcContextFactory();

        return *instance;
    }

    void RegisterOpenGLContextFactory(std::function<std::unique_ptr<IOpenGLContext>()> openGLContextFactory)
    {
        assert(!mOpenGLContextFactory);
        mOpenGLContextFactory = std::move(openGLContextFactory);
    }

    std::unique_ptr<TestGPUCalcContext> CreateTestContext(size_t dataPoints)
    {
        assert(!!mOpenGLContextFactory);

        return std::unique_ptr<TestGPUCalcContext>(
            new TestGPUCalcContext(
                mOpenGLContextFactory(),
                dataPoints));
    }

private:

    GPUCalcContextFactory()
    {}

    std::function<std::unique_ptr<IOpenGLContext>()> mOpenGLContextFactory;
};