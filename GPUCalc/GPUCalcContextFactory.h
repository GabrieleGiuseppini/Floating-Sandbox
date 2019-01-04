/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GPUCalcContext.h"

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

    void RegisterOpenGLContextFactory(std::function<void()> openGLContextFactory)
    {
        assert(!mOpenGLContextFactory);
        mOpenGLContextFactory = std::move(openGLContextFactory);
    }

    std::unique_ptr<GPUCalcContext> CreateContext();

private:

    GPUCalcContextFactory()
    {}

    std::function<void()> mOpenGLContextFactory;
};