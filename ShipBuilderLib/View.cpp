/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "View.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>
#include <GameCore/SysSpecifics.h>

namespace ShipBuilder {

View::View(
    int logicalToPhysicalPixelFactor,
    std::function<void()> swapRenderBuffersFunction,
    ResourceLocator const & resourceLocator)
    : mSwapRenderBuffersFunction(swapRenderBuffersFunction)
{
    //
    // Initialize global OpenGL settings
    //

    // Set anti-aliasing for lines
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Enable blending for alpha transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    // Disable depth test
    glDisable(GL_DEPTH_TEST);

}

void View::Render()
{
    //
    // Initialize
    //

    // Clear canvas
    glClearColor(0.3f, 0.1f, 0.1f, 1.0f); // TODO
    glClear(GL_COLOR_BUFFER_BIT);

    // Flip the back buffer onto the screen
    mSwapRenderBuffersFunction();
}

}