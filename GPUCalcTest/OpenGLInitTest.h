/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "TestCase.h"

#include <GameOpenGL/GameOpenGL.h>

class OpenGLInitTest : public TestCase
{
public:

    OpenGLInitTest()
        : TestCase("OpenGLInit")
    {}

protected:

    virtual void InternalRun() override
    {
        GameOpenGL::InitOpenGL();
    }
};
