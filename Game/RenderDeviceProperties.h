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

    std::function<void()> MakeRenderContextCurrentFunction;
    std::function<void()> SwapRenderBuffersFunction;

    RenderDeviceProperties(
        LogicalPixelSize initialCanvasSize,
        int logicalToPhysicalPixelFactor,
        std::optional<bool> doForceNoGlFinish,
        std::optional<bool> doForceNoMultithreadedRendering,
        std::function<void()> makeRenderContextCurrentFunction,
        std::function<void()> swapRenderBuffersFunction)
        : InitialCanvasSize(initialCanvasSize)
        , LogicalToPhysicalPixelFactor(logicalToPhysicalPixelFactor)
        , DoForceNoGlFinish(doForceNoGlFinish)
        , DoForceNoMultithreadedRendering(doForceNoMultithreadedRendering)
        , MakeRenderContextCurrentFunction(std::move(makeRenderContextCurrentFunction))
        , SwapRenderBuffersFunction(std::move(swapRenderBuffersFunction))
    {}
};
