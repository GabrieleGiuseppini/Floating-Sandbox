/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2024-07-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RollingText.h"

#include <cassert>

/*
 * Facts:
 *	- Lines in NotificationRenderContext are sticky
 */

unsigned int constexpr LinesPerHalf = 4;
unsigned int constexpr MaxLines = LinesPerHalf + 1 + LinesPerHalf;
unsigned int constexpr LineHeight = 8;
unsigned int constexpr MidYOffset = LinesPerHalf * LineHeight;

RollingText::RollingText()
	: mIsDirty(false)
{
}

void RollingText::AddLine(
	std::string const & text,
	std::chrono::duration<float> lifetime)
{
	//
	// Add line
	//

	mLines.emplace_front(text, lifetime);

	//
	// Scroll all other lines up
	//

	unsigned int nextLineMinYOffset = 0 + LineHeight;
	for (auto lineIt = std::next(mLines.begin()); lineIt != mLines.end(); ++lineIt)
	{
		if (lineIt->YOffset < nextLineMinYOffset)
		{
			// Scroll this line
			unsigned int scrollExtent = nextLineMinYOffset - lineIt->YOffset;
			lineIt->YOffset += scrollExtent;

			// See if out of scope
			if (lineIt->YOffset >= MaxLines * LineHeight)
			{
				// Must be the last one
				assert(std::next(lineIt) == mLines.end());

				// Remove it
				mLines.pop_back();

				// We're done
				break;
			}

			// Next line must start one line above this one
			nextLineMinYOffset = lineIt->YOffset + LineHeight;
		}
		else
		{
			// No need to continue
			break;
		}
	}

	// Remember we're dirty now
	mIsDirty = true;
}

void RollingText::Update(float simulationTime)
{
	//
	// Scroll all lines up, except for middle one - if engaged - unless it really has to;
	// in the process, remove lines that are expired
	//

	unsigned int nextLineMinYOffset = 0;
	for (auto lineIt = mLines.begin(); lineIt != mLines.end(); ++lineIt)
	{
		if (lineIt->YOffset == MidYOffset
			&& nextLineMinYOffset < MidYOffset)
		{
			// This is a midline - and it doesn't have to move

			// Advance state machine
			if (!lineIt->StartCentralRestSimulationTime.has_value())
			{
				// Start resting
				lineIt->StartCentralRestSimulationTime = simulationTime;
			}
			else if (simulationTime < *lineIt->StartCentralRestSimulationTime + lineIt->Lifetime.count())
			{
				// Still resting
			}
			else
			{
				// Can start moving
				lineIt->StartCentralRestSimulationTime.reset();

				// Scroll it
				lineIt->YOffset++;

				// Remember we're dirty now
				mIsDirty = true;
			}
		}
		else
		{
			// Scroll it
			lineIt->YOffset++;

			// Remember we're dirty now
			mIsDirty = true;
		}

		// Check whether too far
		if (lineIt->YOffset >= MaxLines * LineHeight)
		{
			// Must be the last one
			assert(std::next(lineIt) == mLines.end());

			// Remove it
			mLines.pop_back();

			// Remember we're dirty now
			mIsDirty = true;

			// We're done
			break;
		}

		// Calculate next min line
		nextLineMinYOffset = lineIt->YOffset + LineHeight;
	}
}

void RollingText::RenderUpload(NotificationRenderContext & notificationRenderContext)
{
	if (mIsDirty)
	{
		notificationRenderContext.UploadNotificationTextStart();

		for (Line const & line : mLines)
		{
			float const yOffset = static_cast<float>(line.YOffset) / static_cast<float>(LineHeight);
			float constexpr HalfHeightLow = static_cast<float>(LinesPerHalf); // Also y offset of line at mid
			float constexpr HalfHeightHigh = static_cast<float>(LinesPerHalf + 1);
			float const alpha = (yOffset <= HalfHeightLow)
				? 1.0f - (HalfHeightLow - yOffset) / HalfHeightLow
				: 1.0f - (yOffset - HalfHeightLow) / HalfHeightHigh;

			notificationRenderContext.UploadNotificationTextLine(
				line.Text,
				AnchorPositionType::TopRight,
				vec2f(0.0f, static_cast<float>(MaxLines) - yOffset),
				alpha);
		}

		notificationRenderContext.UploadNotificationTextEnd();

		mIsDirty = false;
	}
}

void RollingText::Reset()
{
	mLines.clear();

	// Remember to upload
	mIsDirty = true;
}