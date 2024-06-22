/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-10-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "NotificationLayer.h"

#include <GameCore/Conversions.h>
#include <GameCore/GameWallClock.h>

#include <cassert>
#include <iomanip>
#include <sstream>

using namespace std::literals::chrono_literals;

NotificationLayer::NotificationLayer(
	bool isUltraViolentMode,
	bool isSoundMuted,
	bool isDayLightCycleOn,
	bool isAutoFocusOn,
	UnitsSystem displayUnitsSystem,
	std::shared_ptr<GameEventDispatcher> gameEventDispatcher)
    : mGameEventDispatcher(std::move(gameEventDispatcher))
	// StatusText
	, mIsStatusTextEnabled(true)
	, mIsExtendedStatusTextEnabled(false)
    , mStatusTextLines()
	, mIsStatusTextDirty(true)
	// Notification text
	, mEphemeralTextLines()
	, mIsNotificationTextDirty(true)
	// Texture notifications
	, mIsUltraViolentModeIndicatorOn(isUltraViolentMode)
	, mIsSoundMuteIndicatorOn(isSoundMuted)
	, mIsDayLightCycleOn(isDayLightCycleOn)
	, mIsAutoFocusOn(isAutoFocusOn)
	, mAreTextureNotificationsDirty(true)
	// Physics probe
	, mPhysicsProbePanelState()
	, mIsPhysicsProbePanelDirty(true)
	, mPhysicsProbeReading()
	, mPhysicsProbeReadingStrings()
	, mArePhysicsProbeReadingStringsDirty(true)
	// Display units system
	, mDisplayUnitsSystem(displayUnitsSystem)
{
	mGameEventDispatcher->RegisterGenericEventHandler(this);
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
    Render::RenderStatistics renderStats)
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

			float const totalNetUpdate = lastDeltaPerfStats.TotalNetUpdateDuration.ToRatio<std::chrono::milliseconds>();
			float const shipsSpringsUpdatePercent = (totalNetUpdate != 0.0f)
				? lastDeltaPerfStats.TotalShipsSpringsUpdateDuration.ToRatio<std::chrono::milliseconds>() * 100.0f / totalNetUpdate
				: 0.0f;
			float const npcsUpdatePercent = (totalNetUpdate != 0.0f)
				? lastDeltaPerfStats.TotalNpcUpdateDuration.ToRatio<std::chrono::milliseconds>() * 100.0f / totalNetUpdate
				: 0.0f;

			ss << std::fixed
				<< std::setprecision(2)
				<< "UPD:" << totalPerfStats.TotalUpdateDuration.ToRatio<std::chrono::milliseconds>() << "MS"
				<< " (W=" << lastDeltaPerfStats.TotalWaitForRenderUploadDuration.ToRatio<std::chrono::milliseconds>() << "MS +"
				<< " " << totalNetUpdate << "MS"
				<< " (S=" << shipsSpringsUpdatePercent << "%) (N=" << npcsUpdatePercent << "%))"
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
				<< " (" << lastDeltaPerfStats.TotalRenderDrawDuration.ToRatio<std::chrono::milliseconds>() << "MS)"
				<< " (UPL=" << lastDeltaPerfStats.TotalUploadRenderDrawDuration.ToRatio<std::chrono::milliseconds>() << "MS"
				<< " MT=" << lastDeltaPerfStats.TotalMainThreadRenderDrawDuration.ToRatio<std::chrono::milliseconds>() << "MS)"
				;

			mStatusTextLines[2] = ss.str();
		}

        ss.str("");

		{
			ss << "PNT:" << renderStats.LastRenderedShipPoints
				<< " RPS:" << renderStats.LastRenderedShipRopes
				<< " SPR:" << renderStats.LastRenderedShipSprings
				<< " TRI:" << renderStats.LastRenderedShipTriangles
				<< " PLN:" << renderStats.LastRenderedShipPlanes
				<< " GTMM:" << renderStats.LastRenderedShipGenericMipMappedTextures
				<< " FLM:" << renderStats.LastRenderedShipFlames;

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
	mIsNotificationTextDirty = true;
}

void NotificationLayer::SetPhysicsProbePanelState(float open)
{
	if (open != mPhysicsProbePanelState.TargetOpen)
	{
		//
		// Change of target
		//

		// Calculate new start time
		float const now = GameWallClock::GetInstance().NowAsFloat();
		if (mPhysicsProbePanelState.TargetOpen == 1.0f)
		{
			// We were opening - and now we're closing
			mPhysicsProbePanelState.CurrentStateStartTime = now - (1.0f - mPhysicsProbePanelState.CurrentOpen) * PhysicsProbePanelState::TransitionDuration.count();
		}
		else
		{
			// We were closing - and now we're opening
			assert(mPhysicsProbePanelState.TargetOpen == 0.0f);
			mPhysicsProbePanelState.CurrentStateStartTime = now - mPhysicsProbePanelState.CurrentOpen * PhysicsProbePanelState::TransitionDuration.count();
		}

		// Store new target
		mPhysicsProbePanelState.TargetOpen = open;
	}
}

void NotificationLayer::SetDisplayUnitsSystem(UnitsSystem value)
{
	mDisplayUnitsSystem = value;

	// Re-format strings with new system
	RegeneratePhysicsProbeReadingStrings();
}

void NotificationLayer::SetUltraViolentModeIndicator(bool isUltraViolentMode)
{
	mIsUltraViolentModeIndicatorOn = isUltraViolentMode;

	// Indicator needs to be re-uploaded
	mAreTextureNotificationsDirty = true;
}

void NotificationLayer::SetSoundMuteIndicator(bool isSoundMuted)
{
	mIsSoundMuteIndicatorOn = isSoundMuted;

	// Indicator needs to be re-uploaded
	mAreTextureNotificationsDirty = true;
}

void NotificationLayer::SetDayLightCycleIndicator(bool isDayLightCycleOn)
{
	mIsDayLightCycleOn = isDayLightCycleOn;

	// Indicator needs to be re-uploaded
	mAreTextureNotificationsDirty = true;
}

void NotificationLayer::SetAutoFocusIndicator(bool isAutoFocusOn)
{
	mIsAutoFocusOn = isAutoFocusOn;

	// Indicator needs to be re-uploaded
	mAreTextureNotificationsDirty = true;
}

void NotificationLayer::Reset()
{
	// Nuke notification text
	mEphemeralTextLines.clear();
	mIsNotificationTextDirty = true;

	// Reset physics probe
	mPhysicsProbePanelState.Reset();
	mIsPhysicsProbePanelDirty = true;
	mPhysicsProbeReadingStrings.reset();
	mArePhysicsProbeReadingStringsDirty = true;

	// Reset interactions
	mHeatBlasterFlameToRender1.reset();
	mHeatBlasterFlameToRender2.reset();
	mFireExtinguisherSprayToRender1.reset();
	mFireExtinguisherSprayToRender2.reset();
	mBlastToolHaloToRender1.reset();
	mBlastToolHaloToRender2.reset();
	mPressureInjectionHaloToRender1.reset();
	mPressureInjectionHaloToRender2.reset();
	mWindSphereToRender1.reset();
	mWindSphereToRender2.reset();
	mLaserCannonToRender1.reset();
	mLaserCannonToRender2.reset();
}

void NotificationLayer::Update(float now)
{
	//
	// Update ephemeral lines
	//

	{
		// 1) Trim initial lines if we've got too many
		while (mEphemeralTextLines.size() > 8)
		{
			mEphemeralTextLines.pop_front();

			// Text needs to be re-uploaded
			mIsNotificationTextDirty = true;
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
					mIsNotificationTextDirty = true;

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
					mIsNotificationTextDirty = true;

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
					mIsNotificationTextDirty = true;

					break;
				}
			}

			// Advance
			if (doDeleteLine)
			{
				it = mEphemeralTextLines.erase(it);

				// Text needs to be re-uploaded
				mIsNotificationTextDirty = true;
			}
			else
			{
				++it;
			}
		}
	}

	//
	// Update physics probe panel
	//

	{
		float elapsed = now - mPhysicsProbePanelState.CurrentStateStartTime;

		if (mPhysicsProbePanelState.CurrentOpen < mPhysicsProbePanelState.TargetOpen)
		{
			//
			// Opening
			//

			// Discount initial delay
			elapsed -= PhysicsProbePanelState::OpenDelayDuration.count();

			if (elapsed < 0.0f)
			{
				// Still in initial delay
				assert(mPhysicsProbePanelState.CurrentOpen == 0.0f);
				return;
			}

			if (mPhysicsProbePanelState.CurrentOpen == 0.0f)
			{
				// First update for opening...
				// ...emit event then
				mGameEventDispatcher->OnPhysicsProbePanelOpened();
			}

			// Calculate new open
			mPhysicsProbePanelState.CurrentOpen = std::min(elapsed / PhysicsProbePanelState::TransitionDuration.count(), 1.0f);

			// Physics panel needs to be re-uploaded
			mIsPhysicsProbePanelDirty = true;
		}
		else if (mPhysicsProbePanelState.CurrentOpen > mPhysicsProbePanelState.TargetOpen)
		{
			//
			// Closing
			//

			if (mPhysicsProbePanelState.CurrentOpen == 1.0f)
			{
				// First update for closing...

				// ...clear reading
				mPhysicsProbeReadingStrings.reset();
				mArePhysicsProbeReadingStringsDirty = true;

				// ...emit panel closed event
				mGameEventDispatcher->OnPhysicsProbePanelClosed();
			}

			// Calculate new open
			mPhysicsProbePanelState.CurrentOpen = 1.0f - std::min(elapsed / PhysicsProbePanelState::TransitionDuration.count(), 1.0f);

			// Physics panel needs to be re-uploaded
			mIsPhysicsProbePanelDirty = true;
		}
	}

	//
	// Update interactions
	//

	mHeatBlasterFlameToRender2 = mHeatBlasterFlameToRender1;
	mHeatBlasterFlameToRender1.reset();

	mFireExtinguisherSprayToRender2 = mFireExtinguisherSprayToRender1;
	mFireExtinguisherSprayToRender1.reset();

	mBlastToolHaloToRender2 = mBlastToolHaloToRender1;
	mBlastToolHaloToRender1.reset();

	mPressureInjectionHaloToRender2 = mPressureInjectionHaloToRender1;
	mPressureInjectionHaloToRender1.reset();

	mWindSphereToRender2 = mWindSphereToRender1;
	mWindSphereToRender1.reset();

	mLaserCannonToRender2 = mLaserCannonToRender1;
	mLaserCannonToRender1.reset();
}

void NotificationLayer::RenderUpload(Render::RenderContext & renderContext)
{
	auto & notificationRenderContext = renderContext.GetNoficationRenderContext();

	//
	// Upload status text, if needed
	//

	if (mIsStatusTextDirty)
	{
		notificationRenderContext.UploadStatusTextStart();

		int effectiveOrdinal = 0;

		UploadStatusTextLine(mStatusTextLines[0], mIsStatusTextEnabled, effectiveOrdinal, notificationRenderContext);

		for (size_t i = 1; i < mStatusTextLines.size(); ++i)
		{
			UploadStatusTextLine(mStatusTextLines[i], mIsExtendedStatusTextEnabled, effectiveOrdinal, notificationRenderContext);
		}

		notificationRenderContext.UploadStatusTextEnd();

		mIsStatusTextDirty = false;
	}

	//
	// Upload notification text, if needed
	//

	if (mIsNotificationTextDirty)
	{
		notificationRenderContext.UploadNotificationTextStart();

		vec2f screenOffset = vec2f::zero(); // Cumulative vertical offset
		for (auto const & etl : mEphemeralTextLines)
		{
			switch (etl.State)
			{
				case EphemeralTextLine::StateType::FadingIn:
				{
					// Upload text
					notificationRenderContext.UploadNotificationTextLine(
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
					notificationRenderContext.UploadNotificationTextLine(
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
					notificationRenderContext.UploadNotificationTextLine(
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
					break;
				}
			}
		}

		notificationRenderContext.UploadNotificationTextEnd();

		mIsNotificationTextDirty = false;
	}

	//
	// Upload texture notifications, when needed
	//

	if (mAreTextureNotificationsDirty)
	{
		notificationRenderContext.UploadTextureNotificationStart();

		if (mIsUltraViolentModeIndicatorOn)
		{
			notificationRenderContext.UploadTextureNotification(
				TextureFrameId(Render::GenericLinearTextureGroups::UVModeNotification, 0),
				Render::AnchorPositionType::BottomRight,
				vec2f(0.0f, 0.0f),
				1.0f);
		}

		if (mIsSoundMuteIndicatorOn)
		{
			notificationRenderContext.UploadTextureNotification(
				TextureFrameId(Render::GenericLinearTextureGroups::SoundMuteNotification, 0),
				Render::AnchorPositionType::BottomRight,
				vec2f(-1.5f, 0.0f),
				1.0f);
		}

		if (mIsDayLightCycleOn)
		{
			notificationRenderContext.UploadTextureNotification(
				TextureFrameId(Render::GenericLinearTextureGroups::DayLightCycleNotification, 0),
				Render::AnchorPositionType::BottomRight,
				vec2f(-3.0f, 0.0f),
				1.0f);
		}

		if (mIsAutoFocusOn)
		{
			notificationRenderContext.UploadTextureNotification(
				TextureFrameId(Render::GenericLinearTextureGroups::AutoFocusNotification, 0),
				Render::AnchorPositionType::BottomRight,
				vec2f(-4.5f, 0.0f),
				1.0f);
		}

		notificationRenderContext.UploadTextureNotificationEnd();

		mAreTextureNotificationsDirty = false;
	}

	//
	// Upload physics probe, if needed
	//

	if (mIsPhysicsProbePanelDirty)
	{
		notificationRenderContext.UploadPhysicsProbePanel(
			mPhysicsProbePanelState.CurrentOpen,
			mPhysicsProbePanelState.TargetOpen > mPhysicsProbePanelState.CurrentOpen); // is opening

		mIsPhysicsProbePanelDirty = false;
	}

	if (mArePhysicsProbeReadingStringsDirty)
	{
		if (mPhysicsProbeReadingStrings.has_value())
		{
			// Upload reading
			notificationRenderContext.UploadPhysicsProbeReading(
				mPhysicsProbeReadingStrings->Speed,
				mPhysicsProbeReadingStrings->Temperature,
				mPhysicsProbeReadingStrings->Depth,
				mPhysicsProbeReadingStrings->Pressure);
		}
		else
		{
			// Clear reading
			notificationRenderContext.UploadPhysicsProbeReadingClear();
		}

		mArePhysicsProbeReadingStringsDirty = false;
	}

	//
	// Upload interactions, if needed
	//

	if (mHeatBlasterFlameToRender2.has_value())
	{
		notificationRenderContext.UploadHeatBlasterFlame(
			mHeatBlasterFlameToRender2->WorldCoordinates,
			mHeatBlasterFlameToRender2->Radius,
			mHeatBlasterFlameToRender2->Action);
	}

	if (mFireExtinguisherSprayToRender2.has_value())
	{
		notificationRenderContext.UploadFireExtinguisherSpray(
			mFireExtinguisherSprayToRender2->WorldCoordinates,
			mFireExtinguisherSprayToRender2->Radius);
	}

	if (mBlastToolHaloToRender2.has_value())
	{
		notificationRenderContext.UploadBlastToolHalo(
			mBlastToolHaloToRender2->WorldCoordinates,
			mBlastToolHaloToRender2->Radius,
			mBlastToolHaloToRender2->RenderProgress,
			mBlastToolHaloToRender2->PersonalitySeed);
	}

	if (mPressureInjectionHaloToRender2.has_value())
	{
		notificationRenderContext.UploadPressureInjectionHalo(
			mPressureInjectionHaloToRender2->WorldCoordinates,
			mPressureInjectionHaloToRender2->FlowMultiplier);
	}

	if (mWindSphereToRender2.has_value())
	{
		notificationRenderContext.UploadWindSphere(
			mWindSphereToRender2->SourcePos,
			mWindSphereToRender2->PreFrontRadius,
			mWindSphereToRender2->PreFrontIntensityMultiplier,
			mWindSphereToRender2->MainFrontRadius,
			mWindSphereToRender2->MainFrontIntensityMultiplier);
	}

	if (mLaserCannonToRender2.has_value())
	{
		notificationRenderContext.UploadLaserCannon(
			mLaserCannonToRender2->Center,
			mLaserCannonToRender2->Strength,
			renderContext.GetViewModel());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void NotificationLayer::OnPhysicsProbeReading(
	vec2f const & velocity,
	float temperature,
	float depth,
	float pressure)
{
	mPhysicsProbeReading.Speed = velocity.length();
	mPhysicsProbeReading.Temperature = temperature;
	mPhysicsProbeReading.Depth = depth;
	mPhysicsProbeReading.Pressure = pressure;

	RegeneratePhysicsProbeReadingStrings();
}

void NotificationLayer::UploadStatusTextLine(
	std::string & line,
	bool isEnabled,
	int & effectiveOrdinal,
	Render::NotificationRenderContext & notificationRenderContext)
{
	if (isEnabled)
	{
		//
		// This line is enabled, upload it
		//

		vec2f screenOffset(
			0.0f,
			static_cast<float>(effectiveOrdinal++));

		notificationRenderContext.UploadStatusTextLine(
			line,
			Render::AnchorPositionType::TopLeft,
			screenOffset,
			1.0f);
	}
}

void NotificationLayer::RegeneratePhysicsProbeReadingStrings()
{
	// Only pass through if the panel is currently fully open
	if (mPhysicsProbePanelState.CurrentOpen == 1.0f)
	{
		// Create reading strings

		float v{ 0.0f };
		float t{ 0.0f };
		float d{ 0.0f };
		float p{ 0.0f };
		switch (mDisplayUnitsSystem)
		{
			case UnitsSystem::SI_Celsius:
			{
				v = mPhysicsProbeReading.Speed;
				t = mPhysicsProbeReading.Temperature - 273.15f;
				d = mPhysicsProbeReading.Depth;
				p = mPhysicsProbeReading.Pressure / GameParameters::AirPressureAtSeaLevel;
				break;
			}

			case UnitsSystem::SI_Kelvin:
			{
				v = mPhysicsProbeReading.Speed;
				t = mPhysicsProbeReading.Temperature;
				d = mPhysicsProbeReading.Depth;
				p = mPhysicsProbeReading.Pressure / GameParameters::AirPressureAtSeaLevel;
				break;
			}

			case UnitsSystem::USCS:
			{
				v = MeterToFoot(mPhysicsProbeReading.Speed);
				t = CelsiusToFahrenheit(mPhysicsProbeReading.Temperature);
				d = MeterToFoot(mPhysicsProbeReading.Depth);
				p = PascalToPsi(mPhysicsProbeReading.Pressure);
				break;
			}
		}

		std::ostringstream ss;

		{
			ss.fill('0');
			ss << std::fixed << std::setprecision(1) << v;
		}
		std::string speedStr = ss.str();

		ss.str("");

		{
			ss.fill('0');
			ss << std::fixed << std::setprecision(1) << t;
		}
		std::string temperatureStr = ss.str();

		ss.str("");

		{
			ss << std::fixed << std::setprecision(0) << d;
		}
		std::string depthStr = ss.str();

		ss.str("");

		{
			ss
				<< std::fixed
				<< (mDisplayUnitsSystem == UnitsSystem::USCS ? std::setprecision(0) : std::setprecision(1))
				<< p;
		}
		std::string pressureStr = ss.str();

		mPhysicsProbeReadingStrings.emplace(
			std::move(speedStr),
			std::move(temperatureStr),
			std::move(depthStr),
			std::move(pressureStr));

		// Reading has to be uploaded
		mArePhysicsProbeReadingStringsDirty = true;
	}
}