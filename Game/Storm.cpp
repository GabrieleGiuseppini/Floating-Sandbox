/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameMath.h>
#include <GameCore/GameRandomEngine.h>

#include <chrono>

namespace Physics {

// The number of thunders we want per second
float constexpr ThunderRate = 1.0f / 10.0f;

// The number of lightnings we want per second
float constexpr LightningRate = 1.0f / 10.0f;

// The number of poisson samples we perform in a second
float constexpr PoissonSampleRate = 4.0f;
GameWallClock::duration constexpr PoissonSampleDeltaT = std::chrono::duration_cast<GameWallClock::duration>(
	std::chrono::duration<float>(1.0f / PoissonSampleRate));

Storm::Storm(
	World & parentWorld,
	std::shared_ptr<GameEventDispatcher> gameEventDispatcher)
	: mParentWorld(parentWorld)
	, mGameEventHandler(std::move(gameEventDispatcher))
	, mParameters()
	, mIsInStorm(false)
	, mCurrentStormProgress(0.0f)
	, mLastStormUpdateTimestamp(GameWallClock::GetInstance().Now())
	// We want XRate things every 1 seconds, and in 1 second we perform PoissonSampleRate samplings,
	// hence we want 1/PoissonSampleRate things per sample interval
	, mMinThunderCdf(1.0f - exp(-(ThunderRate / 2.0f) / PoissonSampleRate))
	, mOneThunderCdf(1.0f - exp(-ThunderRate / PoissonSampleRate))
	, mMaxThunderCdf(1.0f - exp(-(ThunderRate * 4.0f) / PoissonSampleRate))
	, mMinLightningCdf(1.0f - exp(-(LightningRate / 2.0f) / PoissonSampleRate))
	, mOneLightningCdf(1.0f - exp(-LightningRate / PoissonSampleRate))
	, mMaxLightningCdf(1.0f - exp(-(LightningRate * 4.0f) / PoissonSampleRate))
	, mNextThunderPoissonSampleTimestamp(GameWallClock::GetInstance().Now())
	, mNextBackgroundLightningPoissonSampleTimestamp(GameWallClock::GetInstance().Now())
	, mNextForegroundLightningPoissonSampleTimestamp(GameWallClock::GetInstance().Now())
	, mLightnings()
{
}

void Storm::Update(
	float currentSimulationTime,
	GameParameters const & gameParameters)
{
    auto const now = GameWallClock::GetInstance().Now();

	//
	// Lightnings state machines
	//

	UpdateLightnings(now, currentSimulationTime, gameParameters);


	//
	// Storm state machine
	//

    if (!mIsInStorm)
    {
        //
        // Decide whether we want to trigger a storm or not
        //

        // TODOHERE
        return;
        /*
        TurnStormOn(now);
        */
    }


    //
    // Update storm step
    //

    // Storm script

    float constexpr WindUpStart = 0.0f;
    float constexpr CloudsUpStart = 0.0f;
	float constexpr ThunderStart = 0.08f;
    float constexpr AmbientDarkeningUpStart = 0.09f;    	
	float constexpr RainUpStart = 0.09f;
    float constexpr CloudsUpEnd = 0.1f;
	float constexpr BackgroundLightningStart = 0.11f;
    float constexpr WindUpEnd = 0.12f;
    float constexpr AmbientDarkeningUpEnd = 0.125f;
	float constexpr RainUpEnd = 0.35f;
	float constexpr ForegroundLightningStart = 0.36f;
        
	float constexpr ForegroundLightningEnd = 0.74f;
	float constexpr RainDownStart = 0.75f;
    float constexpr CloudsDownStart = 0.8f;
	float constexpr BackgroundLightningEnd = 0.8f;
	float constexpr ThunderEnd = 0.83f;	
    float constexpr CloudsDownEnd = 0.88f;
    float constexpr WindDownStart = 0.88f;	
	float constexpr AmbientDarkeningDownStart = 0.9f;	
	float constexpr RainDownEnd = 0.905f;
    float constexpr AmbientDarkeningDownEnd = 0.97f;
    float constexpr WindDownEnd = 1.0f;

	float const maxWindSpeed =
		40.0f
		* gameParameters.StormStrengthAdjustment
		* (gameParameters.IsUltraViolentMode ? 4.0f : 1.0f);
	float constexpr MaxClouds = 30.0f;
	float constexpr MinCloudSize = 1.85f;
	float constexpr MaxCloudSize = 5.2f;
	float const maxRainDensity = MixPiecewiseLinear(
		0.1f, 0.4f, 0.9f,
		GameParameters::MinStormStrengthAdjustment, 
		GameParameters::MaxStormStrengthAdjustment, 
		gameParameters.StormStrengthAdjustment);
	float const maxDarkening = MixPiecewiseLinear(
		0.01f, 0.25f, 0.75f,
		GameParameters::MinStormStrengthAdjustment,
		GameParameters::MaxStormStrengthAdjustment,
		gameParameters.StormStrengthAdjustment);

    // Calculate progress of storm: 0.0f = beginning, 1.0f = end
    float progressStep =
        std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(now - mLastStormUpdateTimestamp).count()
        / std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(gameParameters.StormDuration).count();

    mCurrentStormProgress += progressStep;

	//
	// Concentric stages
	//

    if (mCurrentStormProgress < 0.5f)
    { 
        // Up - from 0.0  to 0.5
        float upProgress = mCurrentStormProgress;

        // Wind
        float windSmoothProgress = SmoothStep(WindUpStart, WindUpEnd, upProgress);
        mParameters.WindSpeed = windSmoothProgress * maxWindSpeed;

        // Clouds
        float cloudsLinearProgress = Clamp((upProgress - CloudsUpStart) / (CloudsUpEnd - CloudsUpStart), 0.0f, 1.0f);
        mParameters.NumberOfClouds = static_cast<int>(MaxClouds * cloudsLinearProgress);
        mParameters.CloudsSize = MinCloudSize + (MaxCloudSize - MinCloudSize) * cloudsLinearProgress;
        mParameters.CloudDarkening = (cloudsLinearProgress < 0.5f) ? 0.65f : (cloudsLinearProgress < 0.9f ? 0.56f : 0.4f);

        // Ambient darkening
        float ambientDarkeningSmoothProgress = SmoothStep(AmbientDarkeningUpStart, AmbientDarkeningUpEnd, upProgress);
        mParameters.AmbientDarkening = 1.0f - ambientDarkeningSmoothProgress * maxDarkening;

		// Rain
		if (gameParameters.DoRainWithStorm)
		{
			float rainSmoothProgress = SmoothStep(RainUpStart, RainUpEnd, upProgress);
			mParameters.RainDensity = rainSmoothProgress * maxRainDensity;
		}
		else
		{
			mParameters.RainDensity = 0.0f;
		}
    }
    else
    {
        // Down - from 0.5 to 1.0
        float downProgress = mCurrentStormProgress;

        // Wind
        float windSmoothProgress = 1.0f - SmoothStep(WindDownStart, WindDownEnd, downProgress);
        mParameters.WindSpeed = windSmoothProgress * maxWindSpeed;

        // Clouds
        // from 1.0 to 0.0
        float cloudsLinearProgress = 1.0f - Clamp((downProgress - CloudsDownStart) / (CloudsDownEnd - CloudsDownStart), 0.0f, 1.0f);
        mParameters.NumberOfClouds = static_cast<int>(MaxClouds * cloudsLinearProgress);
        mParameters.CloudsSize = MinCloudSize + (MaxCloudSize - MinCloudSize) * cloudsLinearProgress;
        mParameters.CloudDarkening = (cloudsLinearProgress < 0.5f) ? 1.0f : (cloudsLinearProgress < 0.9f ? 0.56f : 0.4f);
        
        // Ambient darkening
        float ambientDarkeningSmoothProgress = 1.0f - SmoothStep(AmbientDarkeningDownStart, AmbientDarkeningDownEnd, downProgress);
        mParameters.AmbientDarkening = 1.0f - ambientDarkeningSmoothProgress * maxDarkening;

		// Rain
		if (gameParameters.DoRainWithStorm)
		{
			float rainSmoothProgress = 1.0f - SmoothStep(RainDownStart, RainDownEnd, downProgress);
			mParameters.RainDensity = rainSmoothProgress * maxRainDensity;
		}
		else
		{
			mParameters.RainDensity = 0.0f;
		}
    }


	//
	// Thunder stage
	//

	if (mCurrentStormProgress >= ThunderStart && mCurrentStormProgress <= ThunderEnd)
	{
		// Check if it's time to sample poisson
		if (now >= mNextThunderPoissonSampleTimestamp)
		{
			//
			// Check if we should do a thunder
			//

			float const thunderCdf = MixPiecewiseLinear(
				mMinThunderCdf,
				mOneThunderCdf,
				mMaxThunderCdf,
				GameParameters::MinStormStrengthAdjustment,
				GameParameters::MaxStormStrengthAdjustment,
				gameParameters.StormStrengthAdjustment);

			if (GameRandomEngine::GetInstance().GenerateUniformBoolean(thunderCdf))
			{
				// Do thunder!
				mGameEventHandler->OnThunder();
			}

			// Schedule next poisson sampling
			mNextThunderPoissonSampleTimestamp = now + PoissonSampleDeltaT;
		}
	}


	//
	// Lightning stage
	//

	// See if should trigger a background lightning
	bool hasTriggeredLightning = false;
	if (mCurrentStormProgress >= BackgroundLightningStart && mCurrentStormProgress <= BackgroundLightningEnd)
	{
		// Check if it's time to sample poisson		
		if (now >= mNextBackgroundLightningPoissonSampleTimestamp)
		{
			//
			// Check if we should do a background lightning
			// 

			float const backgroundLightningCdf = MixPiecewiseLinear(
				mMinLightningCdf,
				mOneLightningCdf,
				mMaxLightningCdf,
				GameParameters::MinStormStrengthAdjustment,
				GameParameters::MaxStormStrengthAdjustment,
				gameParameters.StormStrengthAdjustment);

			if (GameRandomEngine::GetInstance().GenerateUniformBoolean(backgroundLightningCdf))
			{
				// Do background lightning!
				DoTriggerBackgroundLightning(now);
				hasTriggeredLightning = true;
			}

			// Schedule next poisson sampling
			mNextBackgroundLightningPoissonSampleTimestamp = now + PoissonSampleDeltaT;
		}
	}

	// See if should trigger a foreground lightning
	if (!hasTriggeredLightning
		&& mCurrentStormProgress >= ForegroundLightningStart && mCurrentStormProgress <= ForegroundLightningEnd)
	{
		// Check if it's time to sample poisson
		if (now >= mNextForegroundLightningPoissonSampleTimestamp)
		{
			//
			// Check if we should do a foreground lightning
			// 

			float const foregroundLightningCdf = 
				MixPiecewiseLinear(
					mMinLightningCdf,
					mOneLightningCdf,
					mMaxLightningCdf,
					GameParameters::MinStormStrengthAdjustment,
					GameParameters::MaxStormStrengthAdjustment,
					gameParameters.StormStrengthAdjustment)
				/ 1.8f;

			if (GameRandomEngine::GetInstance().GenerateUniformBoolean(foregroundLightningCdf))
			{
				// Check whether we do have a target
				auto const target = mParentWorld.FindSuitableLightningTarget();
				if (!!target)
				{
					// Do foreground lightning!
					DoTriggerForegroundLightning(now, *target);
					hasTriggeredLightning = true;
				}
			}

			// Schedule next poisson sampling
			mNextForegroundLightningPoissonSampleTimestamp = now + PoissonSampleDeltaT;
		}
	}


    //
    // See if it's time to stop the storm
    //

    if (mCurrentStormProgress >= 1.0f)
    {
        // Turn off storm
        TurnStormOff();

        // Reset storm parameters
        mParameters.Reset();
    }


	//
	// Notify quantities
	//

	mGameEventHandler->OnRainUpdated(mParameters.RainDensity);


    //
    // Remember the last storm update timestamp
    //

    mLastStormUpdateTimestamp = now;
}

void Storm::Upload(Render::RenderContext & renderContext) const
{
	//
    // Upload ambient darkening
	//

    renderContext.UploadStormAmbientDarkening(mParameters.AmbientDarkening);

	//
	// Upload rain
	//

	renderContext.UploadRain(mParameters.RainDensity);

	//
	// Upload lightnings
	//

	UploadLightnings(renderContext);
}

void Storm::TriggerStorm()
{
    if (!mIsInStorm)
    {
        // Turn on storm
        TurnStormOn(GameWallClock::GetInstance().Now());
    }
}

void Storm::TriggerLightning()
{
	// Do a foreground lightning if we have a target and if we feel like doing it	
	if (GameRandomEngine::GetInstance().GenerateUniformBoolean(0.2f))
	{
		auto target = mParentWorld.FindSuitableLightningTarget();
		if (!!target)
		{
			DoTriggerForegroundLightning(GameWallClock::GetInstance().Now(), *target);
			return;
		}
	}

	// No luck, do a background lightning
	DoTriggerBackgroundLightning(GameWallClock::GetInstance().Now());
}

void Storm::TurnStormOn(GameWallClock::time_point now)
{
    mIsInStorm = true;
    mCurrentStormProgress = 0.0f;
    mLastStormUpdateTimestamp = now;

	mGameEventHandler->OnStormBegin();
}

void Storm::TurnStormOff()
{
    mIsInStorm = false;

	mGameEventHandler->OnStormEnd();
}

void Storm::DoTriggerBackgroundLightning(GameWallClock::time_point now)
{
	// Choose NDC x
	float const ndcX = GameRandomEngine::GetInstance().GenerateUniformReal(-0.95f, +0.95f);

	// Enqueue state machine
	mLightnings.emplace_back(
		LightningStateMachine::LightningType::Background,
		GameRandomEngine::GetInstance().GenerateNormalizedUniformReal(),
		now,
		ndcX,
		std::nullopt);

	// Notify
	mGameEventHandler->OnLightning();
}

void Storm::DoTriggerForegroundLightning(
	GameWallClock::time_point now,
	vec2f const & targetWorldPosition)
{
	// Enqueue state machine
	mLightnings.emplace_back(
		LightningStateMachine::LightningType::Foreground,
		GameRandomEngine::GetInstance().GenerateNormalizedUniformReal(),
		now,
		std::nullopt,
		targetWorldPosition);

	// Notify
	mGameEventHandler->OnLightning();
}

void Storm::UpdateLightnings(
	GameWallClock::time_point now,
	float currentSimulationTime,
	GameParameters const & gameParameters)
{
	for (auto it = mLightnings.begin(); it != mLightnings.end(); /*incremented in loop*/)
	{
		//
		// Calculate progress of lightning: 0.0f = beginning, 1.0f = end
		//

		float constexpr LightningDuration = 0.6f;

		it->Progress = std::min(1.0f,
			std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(now - it->StartTimestamp).count()
			/ LightningDuration);

		// Complete vertical development at t=0.3
		it->RenderProgress = SmoothStep(-0.1f, 0.3f, it->Progress);

		if (it->RenderProgress == 1.0f
			&& !it->HasNotifiedTouchdown)
		{
			if (LightningStateMachine::LightningType::Foreground == it->Type)
			{
				// Notify touchdown on world
				assert(!!(it->TargetWorldPosition));
				mParentWorld.ApplyLightning(
					*(it->TargetWorldPosition), 
					currentSimulationTime,
					gameParameters);
			}

			it->HasNotifiedTouchdown = true;
		}

		if (it->Progress == 1.0f)
		{
			// This lightning is complete
			it = mLightnings.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Storm::UploadLightnings(Render::RenderContext & renderContext) const
{
	renderContext.UploadLightningsStart(mLightnings.size());

	for (auto const & l : mLightnings)
	{
		switch (l.Type)
		{
			case LightningStateMachine::LightningType::Background:
			{
				assert(!!(l.NdcX));

				renderContext.UploadBackgroundLightning(
					*(l.NdcX),
					l.Progress,
					l.RenderProgress,
					l.PersonalitySeed);

				break;
			}

			case LightningStateMachine::LightningType::Foreground:
			{
				assert(!!(l.TargetWorldPosition));

				renderContext.UploadForegroundLightning(
					*(l.TargetWorldPosition),
					l.Progress,
					l.RenderProgress,
					l.PersonalitySeed);

				break;
			}
		}
	}

	renderContext.UploadLightningsEnd();
}

}