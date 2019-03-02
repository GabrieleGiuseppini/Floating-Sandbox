/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-10-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextLayer.h"

#include <cassert>
#include <iomanip>
#include <sstream>

TextLayer::TextLayer(
    bool isStatusTextEnabled,
    bool isExtendedStatusTextEnabled)
    : mIsStatusTextEnabled(isStatusTextEnabled)
    , mIsExtendedStatusTextEnabled(isExtendedStatusTextEnabled)
    , mStatusTextLines()
    , mStatusTextHandle(NoneRenderedTextHandle)
    , mIsStatusTextDirty(false)
{
}

void TextLayer::SetStatusTextEnabled(bool isEnabled)
{
    mIsStatusTextEnabled = isEnabled;
}

void TextLayer::SetExtendedStatusTextEnabled(bool isEnabled)
{
    mIsExtendedStatusTextEnabled = isEnabled;
}

void TextLayer::SetStatusText(
    float immediateFps,
    float averageFps,
    std::chrono::duration<float> elapsedGameSeconds,
    bool isPaused,
    float zoom,
    float totalUpdateToRenderDurationRatio,
    float lastUpdateToRenderDurationRatio,
    Render::RenderStatistics const & renderStatistics)
{
    int elapsedSecondsGameInt = static_cast<int>(roundf(elapsedGameSeconds.count()));
    int minutesGame = elapsedSecondsGameInt / 60;
    int secondsGame = elapsedSecondsGameInt - (minutesGame * 60);

    //
    // Build text
    //

    mStatusTextLines.clear();

    if (mIsStatusTextEnabled)
    {
        std::ostringstream ss;

        ss.fill('0');

        ss << std::fixed << std::setprecision(2)
            << "FPS:" << averageFps << " (" << immediateFps << ")"
            << " " << std::setw(2) << minutesGame << ":" << std::setw(2) << secondsGame;

        if (isPaused)
            ss << " (PAUSED)";

        mStatusTextLines.emplace_back(ss.str());
    }

    if (mIsExtendedStatusTextEnabled)
    {
        std::ostringstream ss;

        ss.fill('0');

        ss << std::fixed << std::setprecision(2)
            << "U/R:" << (100.0f * totalUpdateToRenderDurationRatio) << "% (" << (100.0f * lastUpdateToRenderDurationRatio) << "%)"
            << " ZOOM:" << zoom;

        mStatusTextLines.emplace_back(ss.str());

        ss.str("");

        ss
            << "RPS:" << renderStatistics.LastRenderedShipRopes
            << " SPR:" << renderStatistics.LastRenderedShipSprings
            << " TRI:" << renderStatistics.LastRenderedShipTriangles
            << " PLANES:" << renderStatistics.LastRenderedShipPlanes
            << " GENTEX:" << renderStatistics.LastRenderedShipGenericTextures
            << " EPH:" << renderStatistics.LastRenderedShipEphemeralPoints;

        mStatusTextLines.emplace_back(ss.str());
    }

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
    if (mIsStatusTextEnabled || mIsExtendedStatusTextEnabled)
    {
        if (NoneRenderedTextHandle == mStatusTextHandle)
        {
            // Create status text
            mStatusTextHandle = renderContext.AddText(
                mStatusTextLines,
                TextPositionType::TopLeft,
                1.0f,
                FontType::StatusText);
        }
        else if (mIsStatusTextDirty)
        {
            // Update status text
            renderContext.UpdateText(
                mStatusTextHandle,
                mStatusTextLines,
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