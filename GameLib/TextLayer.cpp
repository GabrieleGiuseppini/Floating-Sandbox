/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextLayer.h"

#include <cassert>
#include <iomanip>
#include <sstream>

TextLayer::TextLayer()
    : mIsStatusTextEnabled(false)
    , mStatusText("- (-) --:--")
    , mStatusTextHandle(NoneRenderedTextHandle)
    , mIsStatusTextDirty(false)
{
}

void TextLayer::SetStatusTextEnabled(bool isEnabled)
{
    mIsStatusTextEnabled = isEnabled;
}

void TextLayer::SetStatusText(
    float immediateFps,
    float averageFps,
    std::chrono::duration<float> elapsedGameSeconds)
{
    int elapsedSecondsGameInt = static_cast<int>(roundf(elapsedGameSeconds.count()));
    int minutesGame = elapsedSecondsGameInt / 60;
    int secondsGame = elapsedSecondsGameInt - (minutesGame * 60);

    //
    // Build text
    //

    std::ostringstream ss;

    ss.fill('0');

    ss << std::fixed << std::setprecision(2) << averageFps << " (" << immediateFps << ")"
        << " " << std::setw(2) << minutesGame << ":" << std::setw(2) << secondsGame;

    mStatusText = ss.str();

    mIsStatusTextDirty = true;
}

void TextLayer::Update()
{
    // Nop for the moment; this will change text properties when
    // we'll have animated text   
}

void TextLayer::Render(Render::RenderContext & renderContext)
{
    // Check whether we need to flip the state of the status text
    if (mIsStatusTextEnabled)
    {
        if (NoneRenderedTextHandle == mStatusTextHandle)
        {
            // Create status text
            mStatusTextHandle = renderContext.AddText(
                mStatusText,
                TextPositionType::TopLeft,
                1.0f,
                FontType::StatusText);
        }
        else if (mIsStatusTextDirty)
        {
            // Update status text
            renderContext.UpdateText(
                mStatusTextHandle,
                mStatusText,
                1.0f);
        }

        mIsStatusTextDirty = false;
    }
    else if (NoneRenderedTextHandle != mStatusTextHandle)
    {
        // Turn off status text
        renderContext.ClearText(mStatusTextHandle);
        mStatusTextHandle = NoneRenderedTextHandle;
    }
}
