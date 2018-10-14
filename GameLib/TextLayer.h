/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"
#include "RenderContext.h"

#include <chrono>

class TextLayer
{
public:

    TextLayer();

    void SetStatusTextEnabled(bool isEnabled);

    void SetStatusText(
        float immediateFps,
        float averageFps,
        std::chrono::duration<float> elapsedGameSeconds);
    
    void Update();

    void Render(Render::RenderContext & renderContext);

private:

    //
    // Current state
    //
    
    bool mIsStatusTextEnabled;
    std::string mStatusText;
    RenderedTextHandle mStatusTextHandle;
    bool mIsStatusTextDirty;
};

