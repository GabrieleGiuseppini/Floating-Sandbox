/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-12-29
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

class IOpenGLContext
{
public:

    virtual ~IOpenGLContext()
    {}

    virtual void Activate() = 0;
};