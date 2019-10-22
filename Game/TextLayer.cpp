/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-10-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TextLayer.h"

#include <cassert>
#include <iomanip>
#include <sstream>

TextLayer::TextLayer(
	std::shared_ptr<Render::TextRenderContext> textRenderContext,
    bool isStatusTextEnabled,
    bool isExtendedStatusTextEnabled)
    : mTextRenderContext(std::move(textRenderContext))
	// StatusText state
	, mIsStatusTextEnabled(isStatusTextEnabled)
	, mIsExtendedStatusTextEnabled(isExtendedStatusTextEnabled)
    , mTextLines()
    , mTextHandle(NoneRenderedTextHandle)
    , mIsTextDirty(false)
{
}

void TextLayer::SetStatusTexts(
    float immediateFps,
    float averageFps,
    std::chrono::duration<float> elapsedGameSeconds,
    bool isPaused,
    float zoom,
    vec2f const & camera,
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

    mTextLines.clear();

    if (mIsStatusTextEnabled)
    {
        std::ostringstream ss;

        ss.fill('0');

        ss << std::fixed << std::setprecision(2)
            << "FPS:" << averageFps << " (" << immediateFps << ")"
            << " " << std::setw(2) << minutesGame << ":" << std::setw(2) << secondsGame;

        if (isPaused)
            ss << " (PAUSED)";

        mTextLines.emplace_back(ss.str());
    }

    if (mIsExtendedStatusTextEnabled)
    {
        std::ostringstream ss;

        ss.fill('0');

        ss << std::fixed << std::setprecision(2)
            << "U/R:" << (100.0f * totalUpdateToRenderDurationRatio) << "% (" << (100.0f * lastUpdateToRenderDurationRatio) << "%)"
            << " ZOOM:" << zoom
            << " CAM:" << camera.x << ", " << camera.y;

        mTextLines.emplace_back(ss.str());

        ss.str("");

        ss
            << "PNT:" << renderStatistics.LastRenderedShipPoints
            << " RPS:" << renderStatistics.LastRenderedShipRopes
            << " SPR:" << renderStatistics.LastRenderedShipSprings
            << " TRI:" << renderStatistics.LastRenderedShipTriangles
            << " PLN:" << renderStatistics.LastRenderedShipPlanes
            << " GENTEX:" << renderStatistics.LastRenderedShipGenericTextures
            << " FLM:" << renderStatistics.LastRenderedShipFlames;

        mTextLines.emplace_back(ss.str());
    }

    mIsTextDirty = true;
}

void TextLayer::Update(float /*now*/)
{
    // Check whether we need to flip the state of the status text
    if (mIsStatusTextEnabled || mIsExtendedStatusTextEnabled)
    {
        if (NoneRenderedTextHandle == mTextHandle)
        {
            // Create status text
            mTextHandle = mTextRenderContext->AddText(
                mTextLines,
                TextPositionType::TopLeft,
                1.0f,
                FontType::StatusText);
        }
        else if (mIsTextDirty)
        {
            // Update status text
			mTextRenderContext->UpdateText(
                mTextHandle,
                mTextLines,
                1.0f);
        }

        mIsTextDirty = false;
    }
    else if (NoneRenderedTextHandle != mTextHandle)
    {
        // Turn off status text
		mTextRenderContext->ClearText(mTextHandle);
        mTextHandle = NoneRenderedTextHandle;
    }
}