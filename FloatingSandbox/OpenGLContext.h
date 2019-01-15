/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-01-04
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GPUCalc/IOpenGLContext.h>

/*
 * Implementation of the IOpenGLContext interface for an OpenGL context
 * created with wxWidgets.
 */
class OpenGLContext : public IOpenGLContext
{
public:


public:

    void Activate() override;
};