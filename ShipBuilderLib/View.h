/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShaderTypes.h"
#include "ShipBuilderTypes.h"
#include "ViewModel.h"

#include <Game/ResourceLocator.h>

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/ShaderManager.h>

#include <functional>
#include <memory>

namespace ShipBuilder {

/*
 * This class is the entry point of the entire OpenGL rendering subsystem, providing
 * the API for rendering, which is agnostic about the render platform implementation.
 */
class View
{
public:

    View(
        DisplayLogicalSize initialDisplaySize,
        int logicalToPhysicalPixelFactor,
        std::function<void()> swapRenderBuffersFunction,
        ResourceLocator const & resourceLocator);

    void SetDisplayLogicalSize(DisplayLogicalSize const & logicalSize)
    {
        mViewModel.SetDisplayLogicalSize(logicalSize);

        RefreshOrthoMatrix();
    }

public:

    void Render();

private:

    void RefreshOrthoMatrix();

private:

    ViewModel mViewModel;
    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;
    std::function<void()> const mSwapRenderBuffersFunction;

    //
    // VAOs
    //

    GameOpenGLVBO mTestVBO;
    GameOpenGLVAO mTestVAO;
};

}