/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "OpenGLContext.h"
#include "TestCase.h"

#include <OpenGLCore/GameOpenGL.h>

class OpenGLInitTest : public TestCase
{
public:

    OpenGLInitTest()
        : TestCase("OpenGLInit")
    {}

protected:

    virtual void InternalRun() override
    {
        auto ctx = OpenGLContext();
        ctx.Activate();

        GameOpenGL::InitOpenGL();
    }
};
