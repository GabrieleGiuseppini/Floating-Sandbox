/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-10-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "NotificationLayer.h"

#include <GameCore/GameWallClock.h>

#include <cassert>
#include <iomanip>
#include <sstream>

using namespace std::literals::chrono_literals;

NotificationLayer::NotificationLayer(bool isUltraViolentMode)
    : // StatusText
	  mIsStatusTextEnabled(true)
	, mIsExtendedStatusTextEnabled(false)
    , mStatusTextLines()
	// Ephemeral text
	, mEphemeralTextLines()
	// Ultra-Violent Mode
	, mIsUltraViolentModeIndicatorOn(isUltraViolentMode)
	// State
	, mIsStatusTextDirty(true)
	, mIsGameTextDirty(true)
	, mIsUltraViolentModeIndicatorDirty(true)
{
}

void NotificationLayer::SetStatusTextEnabled(bool isEnabled)
{
	mIsStatusTextEnabled = isEnabled;

	// Text needs to be re-uploaded
	mIsStatusTextDirty = true;
}

void NotificationLayer::SetExtendedStatusTextEnabled(bool isEnabled)
{
	mIsExtendedStatusTextEnabled = isEnabled;

	// Text needs to be re-uploaded
	mIsStatusTextDirty = true;
}

void NotificationLayer::SetStatusTexts(
    float immediateFps,
    float averageFps,
    PerfStats const & lastDeltaPerfStats,
    PerfStats const & totalPerfStats,
    std::chrono::duration<float> elapsedGameSeconds,
    bool isPaused,
    float zoom,
    vec2f const & camera,
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

		mStatusTextLines[0] = ss.str();

		// Text needs to be re-uploaded
		mIsStatusTextDirty = true;
    }

    if (mIsExtendedStatusTextEnabled)
    {
        std::ostringstream ss;

		{
			ss.fill('0');

			ss << std::fixed
				<< std::setprecision(2)
				<< "UPD:" << totalPerfStats.TotalUpdateDuration.ToRatio<std::chrono::milliseconds>() << "MS"
				<< " (W=" << lastDeltaPerfStats.TotalWaitForRenderUploadDuration.ToRatio<std::chrono::milliseconds>() << "MS +"
				<< " " << lastDeltaPerfStats.TotalNetUpdateDuration.ToRatio<std::chrono::milliseconds>() << "MS)"
				<< " UPL:(W=" << lastDeltaPerfStats.TotalWaitForRenderDrawDuration.ToRatio<std::chrono::milliseconds>() << "MS +"
				<< " " << lastDeltaPerfStats.TotalNetRenderUploadDuration.ToRatio<std::chrono::milliseconds>() << "MS)"
				;

			mStatusTextLines[1] = ss.str();
		}

		ss.str("");

		{
			ss.fill('0');

			ss << std::fixed
				<< std::setprecision(2)
				<< "RND:" << totalPerfStats.TotalRenderDrawDuration.ToRatio<std::chrono::milliseconds>() << "MS"
				<< " (" << lastDeltaPerfStats.TotalRenderDrawDuration.ToRatio<std::chrono::milliseconds>() << "MS"
				<< " CL=" << lastDeltaPerfStats.TotalCloudsRenderDrawDuration.ToRatio<std::chrono::milliseconds>() << "MS"
				<< " OS=" << lastDeltaPerfStats.TotalOceanSurfaceRenderDrawDuration.ToRatio<std::chrono::milliseconds>() << "MS)"
				<< " (MT=" << lastDeltaPerfStats.TotalMainThreadRenderDrawDuration.ToRatio<std::chrono::milliseconds>() << "MS)"
				;

			mStatusTextLines[2] = ss.str();
		}

        ss.str("");

		{
			ss << "PNT:" << renderStatistics.LastRenderedShipPoints
				<< " RPS:" << renderStatistics.LastRenderedShipRopes
				<< " SPR:" << renderStatistics.LastRenderedShipSprings
				<< " TRI:" << renderStatistics.LastRenderedShipTriangles
				<< " PLN:" << renderStatistics.LastRenderedShipPlanes
				<< " GTMM:" << renderStatistics.LastRenderedShipGenericMipMappedTextures
				<< " FLM:" << renderStatistics.LastRenderedShipFlames;

			ss << std::fixed << std::setprecision(2)
				<< " ZM:" << zoom
				<< " CAM:" << camera.x << ", " << camera.y;

			mStatusTextLines[3] = ss.str();
		}

		// Text needs to be re-uploaded
		mIsStatusTextDirty = true;
    }
}

void NotificationLayer::AddEphemeralTextLine(
	std::string const & text,
	std::chrono::duration<float> lifetime)
{
	// Store ephemeral line
	mEphemeralTextLines.emplace_back(text, lifetime);

	// Text needs to be re-uploaded
	mIsGameTextDirty = true;
}

void NotificationLayer::SetUltraViolentModeIndicator(bool isUltraViolentMode)
{
	mIsUltraViolentModeIndicatorOn = isUltraViolentMode;

	// Indicator needs to be re-uploaded
	mIsUltraViolentModeIndicatorDirty = true;
}

void NotificationLayer::Reset()
{
	// Nuke all ephemeral lines
	mEphemeralTextLines.clear();
	mIsGameTextDirty = true;
}

void NotificationLayer::Update(float now)
{
	// This method is invoked after guaranteeing that there is
	// no pending RenderUpload, hence all the NotificationRenderContext
	// CPU buffers are now safe to be changed

	//
	// Update ephemeral lines
	//

	{
		// 1) Trim initial lines if we've got too many
		while (mEphemeralTextLines.size() > 8)
		{
			mEphemeralTextLines.pop_front();

			// Text needs to be re-uploaded
			mIsGameTextDirty = true;
		}

		// 2) Update state of remaining ones
		for (auto it = mEphemeralTextLines.begin(); it != mEphemeralTextLines.end(); )
		{
			bool doDeleteLine = false;

			switch (it->State)
			{
				case EphemeralTextLine::StateType::Initial:
				{
					// Initialize fade-in
					it->State = EphemeralTextLine::StateType::FadingIn;
					it->CurrentStateStartTimestamp = now;

					[[fallthrough]];
				}

				case EphemeralTextLine::StateType::FadingIn:
				{
					it->CurrentStateProgress = GameWallClock::Progress(now, it->CurrentStateStartTimestamp, 500ms);

					// See if time to transition
					if (it->CurrentStateProgress >= 1.0f)
					{
						it->State = EphemeralTextLine::StateType::Displaying;
						it->CurrentStateStartTimestamp = now;
						it->CurrentStateProgress = 0.0f;
					}

					// Text needs to be re-uploaded (for alpha)
					mIsGameTextDirty = true;

					break;
				}

				case EphemeralTextLine::StateType::Displaying:
				{
					it->CurrentStateProgress = GameWallClock::Progress(now, it->CurrentStateStartTimestamp, it->Lifetime);

					// See if time to transition
					if (it->CurrentStateProgress >= 1.0f)
					{
						it->State = EphemeralTextLine::StateType::FadingOut;
						it->CurrentStateStartTimestamp = now;
						it->CurrentStateProgress = 0.0f;
					}

					break;
				}

				case EphemeralTextLine::StateType::FadingOut:
				{
					it->CurrentStateProgress = GameWallClock::Progress(now, it->CurrentStateStartTimestamp, 500ms);

					// See if time to transition
					if (it->CurrentStateProgress >= 1.0f)
					{
						it->State = EphemeralTextLine::StateType::Disappearing;
						it->CurrentStateStartTimestamp = now;
						it->CurrentStateProgress = 0.0f;
					}

					// Text needs to be re-uploaded (for alpha)
					mIsGameTextDirty = true;

					break;
				}

				case EphemeralTextLine::StateType::Disappearing:
				{
					it->CurrentStateProgress = GameWallClock::Progress(now, it->CurrentStateStartTimestamp, 500ms);

					// See if time to turn off
					if (it->CurrentStateProgress >= 1.0f)
					{
						doDeleteLine = true;
					}

					// Text needs to be re-uploaded (for vertical offset)
					mIsGameTextDirty = true;

					break;
				}
			}

			// Advance
			if (doDeleteLine)
			{
				it = mEphemeralTextLines.erase(it);

				// Text needs to be re-uploaded
				mIsGameTextDirty = true;
			}
			else
			{
				++it;
			}
		}
	}
}

void NotificationLayer::RenderUpload(Render::RenderContext & renderContext)
{
	//
	// Upload status text, if needed
	//

	if (mIsStatusTextDirty)
	{
		renderContext.UploadTextNotificationStart(Render::FontType::StatusText);

		int effectiveOrdinal = 0;

		UploadStatusTextLine(mStatusTextLines[0], mIsStatusTextEnabled, effectiveOrdinal, renderContext);

		for (size_t i = 1; i < mStatusTextLines.size(); ++i)
		{
			UploadStatusTextLine(mStatusTextLines[i], mIsExtendedStatusTextEnabled, effectiveOrdinal, renderContext);
		}

		renderContext.UploadTextNotificationEnd(Render::FontType::StatusText);

		mIsStatusTextDirty = false;
	}

	//
	// Upload ephemeral lines, if needed
	//

	if (mIsGameTextDirty)
	{
		renderContext.UploadTextNotificationStart(Render::FontType::GameText);

		vec2f screenOffset = vec2f::zero(); // Cumulative vertical offset
		for (auto const & etl : mEphemeralTextLines)
		{
			switch (etl.State)
			{
				case EphemeralTextLine::StateType::FadingIn:
				{
					// Upload text
					renderContext.UploadTextNotificationLine(
						Render::FontType::GameText,
						etl.Text,
						Render::AnchorPositionType::TopRight,
						screenOffset,
						std::min(1.0f, etl.CurrentStateProgress));

					// Update offset of next line
					screenOffset.y += 1.0f;

					break;
				}

				case EphemeralTextLine::StateType::Displaying:
				{
					// Upload text
					renderContext.UploadTextNotificationLine(
						Render::FontType::GameText,
						etl.Text,
						Render::AnchorPositionType::TopRight,
						screenOffset,
						1.0f);

					// Update offset of next line
					screenOffset.y += 1.0f;

					break;
				}

				case EphemeralTextLine::StateType::FadingOut:
				{
					// Upload text
					renderContext.UploadTextNotificationLine(
						Render::FontType::GameText,
						etl.Text,
						Render::AnchorPositionType::TopRight,
						screenOffset,
						1.0f - std::min(1.0f, etl.CurrentStateProgress));

					// Update offset of next line
					screenOffset.y += 1.0f;

					break;
				}

				case EphemeralTextLine::StateType::Disappearing:
				{
					// Update offset of next line
					screenOffset.y += (1.0f - std::min(1.0f, etl.CurrentStateProgress));

					break;
				}

				default:
				{
					// Do not upload
				}
			}
		}

		renderContext.UploadTextNotificationEnd(Render::FontType::GameText);

		mIsGameTextDirty = false;
	}

	//
	// Upload ultra-violent mode indicator, if needed
	//

	if (mIsUltraViolentModeIndicatorDirty)
	{
		renderContext.UploadTextureNotificationStart();

		if (mIsUltraViolentModeIndicatorOn)
		{
			renderContext.UploadTextureNotification(
				TextureFrameId(Render::GenericLinearTextureGroups::UVModeNotification, 0),
				Render::AnchorPositionType::BottomRight,
				vec2f::zero(),
				1.0f);
		}

		renderContext.UploadTextureNotificationEnd();

		mIsUltraViolentModeIndicatorDirty = false;
	}
}

void NotificationLayer::UploadStatusTextLine(
	std::string & line,
	bool isEnabled,
	int & effectiveOrdinal,
	Render::RenderContext & renderContext)
{
	if (isEnabled)
	{
		//
		// This line is enabled, upload it
		//

		vec2f screenOffset(
			0.0f,
			static_cast<float>(effectiveOrdinal++));

		renderContext.UploadTextNotificationLine(
			Render::FontType::StatusText,
			line,
			Render::AnchorPositionType::TopLeft,
			screenOffset,
			1.0f);
	}
}