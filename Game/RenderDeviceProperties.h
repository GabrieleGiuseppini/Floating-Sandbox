/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-02-01
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <GameCore/ImageSize.h>

#include <cstdint>
#include <functional>
#include <optional>

struct RenderDeviceProperties
{
    ImageSize InitialCanvasSize;

    std::optional<bool> DoForceNoGlFinish;
    std::optional<bool> DoForceNoMultithreadedRendering;

    std::function<void()> MakeRenderContextCurrentFunction;
    std::function<void()> SwapRenderBuffersFunction;

    RenderDeviceProperties(
        ImageSize initialCanvasSize,
        std::optional<bool> doForceNoGlFinish,
        std::optional<bool> doForceNoMultithreadedRendering,
        std::function<void()> makeRenderContextCurrentFunction,
        std::function<void()> swapRenderBuffersFunction)
        : InitialCanvasSize(initialCanvasSize)
        , DoForceNoGlFinish(doForceNoGlFinish)
        , DoForceNoMultithreadedRendering(doForceNoMultithreadedRendering)
        , MakeRenderContextCurrentFunction(std::move(makeRenderContextCurrentFunction))
        , SwapRenderBuffersFunction(std::move(swapRenderBuffersFunction))
    {}
};
