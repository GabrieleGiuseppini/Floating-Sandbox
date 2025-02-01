/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-02-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>

#include <cstdint>
#include <functional>
#include <optional>

struct RenderDeviceProperties
{
    DisplayLogicalSize InitialCanvasSize;
    int LogicalToPhysicalDisplayFactor;

    std::optional<bool> DoForceNoGlFinish;
    std::optional<bool> DoForceNoMultithreadedRendering;

    std::function<void()> MakeRenderContextCurrentFunction;
    std::function<void()> SwapRenderBuffersFunction;

    RenderDeviceProperties(
        DisplayLogicalSize initialCanvasSize,
        int logicalToPhysicalDisplayFactor,
        std::optional<bool> doForceNoGlFinish,
        std::optional<bool> doForceNoMultithreadedRendering,
        std::function<void()> makeRenderContextCurrentFunction,
        std::function<void()> swapRenderBuffersFunction)
        : InitialCanvasSize(initialCanvasSize)
        , LogicalToPhysicalDisplayFactor(logicalToPhysicalDisplayFactor)
        , DoForceNoGlFinish(doForceNoGlFinish)
        , DoForceNoMultithreadedRendering(doForceNoMultithreadedRendering)
        , MakeRenderContextCurrentFunction(std::move(makeRenderContextCurrentFunction))
        , SwapRenderBuffersFunction(std::move(swapRenderBuffersFunction))
    {}
};
