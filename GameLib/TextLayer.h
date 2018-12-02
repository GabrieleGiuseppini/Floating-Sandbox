/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"
#include "RenderContext.h"

#include <chrono>
#include <string>
#include <vector>

class TextLayer
{
public:

    TextLayer(
        bool isStatusTextEnabled,
        bool isExtendedStatusTextEnabled);

    void SetStatusTextEnabled(bool isEnabled);

    void SetExtendedStatusTextEnabled(bool isEnabled);

    void SetStatusText(
        float immediateFps,
        float averageFps,
        std::chrono::duration<float> elapsedGameSeconds,
        bool isPaused,
        float zoom,
        float totalUpdateToRenderDurationRatio,
        float lastUpdateToRenderDurationRatio,
        Render::RenderStatistics const & renderStatistics);

    void Update();

    void Render(Render::RenderContext & renderContext);

private:

    //
    // Current state
    //

    bool mIsStatusTextEnabled;
    bool mIsExtendedStatusTextEnabled;
    std::vector<std::string> mStatusTextLines;
    RenderedTextHandle mStatusTextHandle;
    bool mIsStatusTextDirty;
};

