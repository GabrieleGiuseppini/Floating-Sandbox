/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-02-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/ImageSize.h>
#include <GameCore/GameTypes.h>

#include <cstdint>
#include <functional>
#include <optional>

struct RenderDeviceProperties
{
    LogicalPixelSize InitialCanvasSize;
    int LogicalToPhysicalPixelFactor;

    std::optional<bool> DoForceNoGlFinish;
    std::optional<bool> DoForceNoMultithreadedRendering;

    std::function<void()> InitialMakeRenderContextCurrentFunction;
    std::function<void()> SwapRenderBuffersFunction;

    RenderDeviceProperties(
        LogicalPixelSize initialCanvasSize,
        int logicalToPhysicalPixelFactor,
        std::optional<bool> doForceNoGlFinish,
        std::optional<bool> doForceNoMultithreadedRendering,
        std::function<void()> initialMakeRenderContextCurrentFunction,
        std::function<void()> swapRenderBuffersFunction)
        : InitialCanvasSize(initialCanvasSize)
        , LogicalToPhysicalPixelFactor(logicalToPhysicalPixelFactor)
        , DoForceNoGlFinish(doForceNoGlFinish)
        , DoForceNoMultithreadedRendering(doForceNoMultithreadedRendering)
        , InitialMakeRenderContextCurrentFunction(std::move(initialMakeRenderContextCurrentFunction))
        , SwapRenderBuffersFunction(std::move(swapRenderBuffersFunction))
    {}
};
