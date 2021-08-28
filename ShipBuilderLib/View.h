/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"
#include "ViewModel.h"

#include <Game/ResourceLocator.h>

#include <GameOpenGL/GameOpenGL.h>

#include <functional>

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
    }

public:

    void Render();

private:

    ViewModel mViewModel;
    std::function<void()> const mSwapRenderBuffersFunction;
};

}