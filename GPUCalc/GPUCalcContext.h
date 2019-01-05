/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IOpenGLContext.h"

#include <cassert>
#include <memory>

/*
 * Base class of task-specific contextes that perform calculations on the GPU.
 */
class GPUCalcContext
{
protected:

    GPUCalcContext(std::unique_ptr<IOpenGLContext> openGLContext);

    void ActivateOpenGLContext()
    {
        assert(!!mOpenGLContext);

        mOpenGLContext->Activate();
    }

private:

    std::unique_ptr<IOpenGLContext> const mOpenGLContext;
};