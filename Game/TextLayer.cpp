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
    , mStatusTextLines()
	, mAreStatusTextLinePositionsDirty(false)
{
}

void TextLayer::SetStatusTextEnabled(bool isEnabled)
{
	mIsStatusTextEnabled = isEnabled;

	// Positions need to be recalculated
	mAreStatusTextLinePositionsDirty = true;
}

void TextLayer::SetExtendedStatusTextEnabled(bool isEnabled)
{
	mIsExtendedStatusTextEnabled = isEnabled;

	// Positions need to be recalculated
	mAreStatusTextLinePositionsDirty = true;
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

    if (mIsStatusTextEnabled)
    {
        std::ostringstream ss;

        ss.fill('0');

        ss << std::fixed << std::setprecision(2)
            << "FPS:" << averageFps << " (" << immediateFps << ")"
            << " " << std::setw(2) << minutesGame << ":" << std::setw(2) << secondsGame;

        if (isPaused)
            ss << " (PAUSED)";

		mStatusTextLines[0].Text = ss.str();
		mStatusTextLines[0].IsTextDirty = true;
    }

    if (mIsExtendedStatusTextEnabled)
    {
        std::ostringstream ss;

        ss.fill('0');

        ss << std::fixed << std::setprecision(2)
            << "U/R:" << (100.0f * totalUpdateToRenderDurationRatio) << "% (" << (100.0f * lastUpdateToRenderDurationRatio) << "%)"
            << " ZOOM:" << zoom
            << " CAM:" << camera.x << ", " << camera.y;

		mStatusTextLines[1].Text = ss.str();
		mStatusTextLines[1].IsTextDirty = true;

        ss.str("");

        ss
            << "PNT:" << renderStatistics.LastRenderedShipPoints
            << " RPS:" << renderStatistics.LastRenderedShipRopes
            << " SPR:" << renderStatistics.LastRenderedShipSprings
            << " TRI:" << renderStatistics.LastRenderedShipTriangles
            << " PLN:" << renderStatistics.LastRenderedShipPlanes
            << " GENTEX:" << renderStatistics.LastRenderedShipGenericTextures
            << " FLM:" << renderStatistics.LastRenderedShipFlames;

		mStatusTextLines[2].Text = ss.str();
		mStatusTextLines[2].IsTextDirty = true;
    }
}

void TextLayer::Update(float /*now*/)
{
	//
	// Status text
	//

	{
		int ordinal = 0;

		UpdateStatusTextLine(mStatusTextLines[0], mIsStatusTextEnabled, mAreStatusTextLinePositionsDirty, ordinal);

		for (size_t i = 1; i <= 2; ++i)
		{
			UpdateStatusTextLine(mStatusTextLines[i], mIsExtendedStatusTextEnabled, mAreStatusTextLinePositionsDirty, ordinal);
		}

		mAreStatusTextLinePositionsDirty = false;
	}
}

void TextLayer::UpdateStatusTextLine(
	StatusTextLine & line,
	bool isEnabled,
	bool arePositionsDirty,
	int & ordinal)
{
	// Check whether we need to flip the state of the status text
	if (isEnabled)
	{
		//
		// This line is enabled
		//

		vec2f offset(
			0.0f,
			static_cast<float>(ordinal++) * mTextRenderContext->GetLineScreenHeight(FontType::StatusText));

		if (NoneRenderedTextHandle == line.Handle)
		{
			// Create status text
			line.Handle = mTextRenderContext->AddTextLine(
				line.Text,
				TextPositionType::TopLeft,
				offset,
				1.0f,
				FontType::StatusText);

			line.IsTextDirty = false;
		}
		else if (line.IsTextDirty || arePositionsDirty)
		{
			// Update status text
			mTextRenderContext->UpdateTextLine(
				line.Handle,
				line.Text,
				offset);

			line.IsTextDirty = false;
		}		
	}
	else 
	{
		//
		// This line is not enabled
		//

		if (NoneRenderedTextHandle != line.Handle)
		{
			// Turn off line altogether
			mTextRenderContext->ClearTextLine(line.Handle);
			line.Handle = NoneRenderedTextHandle;

			// Initialize text
			line.Text = "";
			line.IsTextDirty = false;
		}
	}
}