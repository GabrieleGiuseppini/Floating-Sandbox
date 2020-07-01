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
	, mIsTextDirty(true)
	, mIsUltraViolentModeIndicatorDirty(true)
{
}

void NotificationLayer::SetCanvasSize(int width, int height)
{
	LogMessage("TODOTEST: NotificationLayer::SetCanvasSize(", width, ",", height, ")");

	// Force re-upload of everything that is screen-size dependent
	mIsTextDirty = true;
	mIsUltraViolentModeIndicatorDirty = true;
}

void NotificationLayer::SetStatusTextEnabled(bool isEnabled)
{
	mIsStatusTextEnabled = isEnabled;

	// Text needs to be re-uploaded
	mIsTextDirty = true;
}

void NotificationLayer::SetExtendedStatusTextEnabled(bool isEnabled)
{
	mIsExtendedStatusTextEnabled = isEnabled;

	// Text needs to be re-uploaded
	mIsTextDirty = true;
}

void NotificationLayer::SetStatusTexts(
    float immediateFps,
    float averageFps,
    PerfStats const & lastDeltaPerfStats,
    PerfStats const & totalPerfStats,
    uint64_t lastDeltaFrameCount,
    uint64_t totalFrameCount,
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
		mIsTextDirty = true;
    }

    if (mIsExtendedStatusTextEnabled)
    {
        float const lastUpdateDurationMillisecondsPerFrame = lastDeltaFrameCount != 0
            ? static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(lastDeltaPerfStats.TotalUpdateDuration).count()) / 1000.0f / static_cast<float>(lastDeltaFrameCount)
            : 0.0f;

        float const avgUpdateDurationMillisecondsPerFrame = totalFrameCount != 0
            ? static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(totalPerfStats.TotalUpdateDuration).count()) / 1000.0f / static_cast<float>(totalFrameCount)
            : 0.0f;

        float const lastRenderUploadDurationMillisecondsPerFrame = lastDeltaFrameCount != 0
            ? static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(lastDeltaPerfStats.TotalRenderUploadDuration).count()) / 1000.0f / static_cast<float>(lastDeltaFrameCount)
            : 0.0f;

		float const avgRenderUploadDurationMillisecondsPerFrame = totalFrameCount != 0
			? static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(totalPerfStats.TotalRenderUploadDuration).count()) / 1000.0f / static_cast<float>(totalFrameCount)
			: 0.0f;

		float const lastRenderDrawDurationMillisecondsPerFrame = lastDeltaFrameCount != 0
			? static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(lastDeltaPerfStats.TotalRenderDrawDuration).count()) / 1000.0f / static_cast<float>(lastDeltaFrameCount)
			: 0.0f;

		float const avgRenderDrawDurationMillisecondsPerFrame = totalFrameCount != 0
			? static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(totalPerfStats.TotalRenderDrawDuration).count()) / 1000.0f / static_cast<float>(totalFrameCount)
			: 0.0f;

		float const avgWaitForRenderUploadDurationMillisecondsPerFrame = totalFrameCount != 0
			? static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(totalPerfStats.TotalWaitForRenderUploadDuration).count()) / 1000.0f / static_cast<float>(totalFrameCount)
			: 0.0f;

		float const avgWaitForRenderDrawDurationMillisecondsPerFrame = totalFrameCount != 0
			? static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(totalPerfStats.TotalWaitForRenderDrawDuration).count()) / 1000.0f / static_cast<float>(totalFrameCount)
			: 0.0f;

        std::ostringstream ss;

		{
			ss.fill('0');

			ss << std::fixed
				<< std::setprecision(2)
				<< "UPD:" << avgUpdateDurationMillisecondsPerFrame << "MS" << " (" << lastUpdateDurationMillisecondsPerFrame << "MS) "
				<< "UPL:" << avgRenderUploadDurationMillisecondsPerFrame << "MS" << " (" << lastRenderUploadDurationMillisecondsPerFrame << "MS) "
				<< "DRW:" << avgRenderDrawDurationMillisecondsPerFrame << "MS" << " (" << lastRenderDrawDurationMillisecondsPerFrame << "MS)"
				;

			mStatusTextLines[1] = ss.str();
		}

		ss.str("");

		{
			ss.fill('0');

			ss << std::fixed
				<< std::setprecision(2)
				<< "WAIT(UPD:" << avgWaitForRenderDrawDurationMillisecondsPerFrame << "MS" << " DRW:" << avgWaitForRenderDrawDurationMillisecondsPerFrame << "MS)"
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
				<< " GENTEX:" << renderStatistics.LastRenderedShipGenericMipMappedTextures
				<< " FLM:" << renderStatistics.LastRenderedShipFlames;

			ss << std::fixed << std::setprecision(2)
				<< " ZM:" << zoom
				<< " CAM:" << camera.x << ", " << camera.y;

			mStatusTextLines[3] = ss.str();
		}

		// Text needs to be re-uploaded
		mIsTextDirty = true;
    }
}

void NotificationLayer::AddEphemeralTextLine(
	std::string const & text,
	std::chrono::duration<float> lifetime)
{
	// Store ephemeral line
	mEphemeralTextLines.emplace_back(text, lifetime);

	// Text needs to be re-uploaded
	mIsTextDirty = true;
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

	// Text needs to be re-uploaded
	mIsTextDirty = true;
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
			mIsTextDirty = true;
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
					mIsTextDirty = true;

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
					mIsTextDirty = true;

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
					mIsTextDirty = true;

					break;
				}
			}

			// Advance
			if (doDeleteLine)
			{
				it = mEphemeralTextLines.erase(it);

				// Text needs to be re-uploaded
				mIsTextDirty = true;
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
	// Upload text, if needed
	//

	if (mIsTextDirty)
	{
		// Tell render context we're starting with text
		renderContext.UploadNotificationTextStart();

		//
		// Upload status text
		//

		{
			int effectiveOrdinal = 0;

			UploadStatusTextLine(mStatusTextLines[0], mIsStatusTextEnabled, effectiveOrdinal, renderContext);

			for (size_t i = 1; i < mStatusTextLines.size(); ++i)
			{
				UploadStatusTextLine(mStatusTextLines[i], mIsExtendedStatusTextEnabled, effectiveOrdinal, renderContext);
			}
		}

		//
		// Ephemeral lines
		//

		{
			vec2f screenOffset = vec2f::zero(); // Cumulative vertical offset
			for (auto const & etl : mEphemeralTextLines)
			{
				switch (etl.State)
				{
					case EphemeralTextLine::StateType::FadingIn:
					{
						// Upload text
						renderContext.UploadNotificationTextLine(
							etl.Text,
							TextPositionType::TopRight,
							screenOffset,
							std::min(1.0f, etl.CurrentStateProgress),
							FontType::GameText);

						// Update offset of next line
						screenOffset.y += 1.0f;

						break;
					}

					case EphemeralTextLine::StateType::Displaying:
					{
						// Upload text
						renderContext.UploadNotificationTextLine(
							etl.Text,
							TextPositionType::TopRight,
							screenOffset,
							1.0f,
							FontType::GameText);

						// Update offset of next line
						screenOffset.y += 1.0f;

						break;
					}

					case EphemeralTextLine::StateType::FadingOut:
					{
						// Upload text
						renderContext.UploadNotificationTextLine(
							etl.Text,
							TextPositionType::TopRight,
							screenOffset,
							1.0f - std::min(1.0f, etl.CurrentStateProgress),
							FontType::GameText);

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
		}

		// No more dirty
		mIsTextDirty = false;

		// Tell render context we're done with text
		renderContext.UploadNotificationTextEnd();
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

		renderContext.UploadNotificationTextLine(
			line,
			TextPositionType::TopLeft,
			screenOffset,
			1.0f,
			FontType::StatusText);
	}
}