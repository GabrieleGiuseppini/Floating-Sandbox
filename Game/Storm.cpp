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

// The number of poisson samples we perform in a second
float constexpr PoissonSampleRate = 4.0f;
GameWallClock::duration constexpr PoissonSampleDeltaT = std::chrono::duration_cast<GameWallClock::duration>(
	std::chrono::duration<float>(1.0f / PoissonSampleRate));

Storm::Storm(std::shared_ptr<GameEventDispatcher> gameEventDispatcher)
	: mGameEventHandler(std::move(gameEventDispatcher))
	, mParameters()
	, mIsInStorm(false)
	, mCurrentStormProgress(0.0f)
	// We want ThunderRate thunders every 1 seconds, and in 1 second we perform PoissonSampleRate samplings,
	// hence we want 1/PoissonSampleRate thunders per sample interval
	, mThunderCdf(1.0f - exp(-ThunderRate / PoissonSampleRate))
	, mLastStormUpdateTimestamp(GameWallClock::GetInstance().Now())
	, mNextThunderPoissonSampleTimestamp(GameWallClock::GetInstance().Now())
{
}

void Storm::Update(GameParameters const & gameParameters)
{
    auto const now = GameWallClock::GetInstance().Now();

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
    float constexpr WindUpEnd = 0.12f;
    float constexpr AmbientDarkeningUpEnd = 0.125f;
	float constexpr RainUpEnd = 0.35f;
        
	float constexpr RainDownStart = 0.75f;
    float constexpr CloudsDownStart = 0.8f;
	float constexpr ThunderEnd = 0.83f;
    float constexpr CloudsDownEnd = 0.88f;
    float constexpr WindDownStart = 0.88f;	
	float constexpr AmbientDarkeningDownStart = 0.9f;	
	float constexpr RainDownEnd = 0.905f;
    float constexpr AmbientDarkeningDownEnd = 0.97f;
    float constexpr WindDownEnd = 1.0f;


	float constexpr MaxClouds = 30.0f;
    float constexpr MinCloudSize = 1.85f;
    float constexpr MaxCloudSize = 5.2f;


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
        mParameters.WindSpeed = windSmoothProgress * gameParameters.StormMaxWindSpeed;

        // Clouds
        float cloudsLinearProgress = Clamp((upProgress - CloudsUpStart) / (CloudsUpEnd - CloudsUpStart), 0.0f, 1.0f);
        mParameters.NumberOfClouds = static_cast<int>(MaxClouds * cloudsLinearProgress);
        mParameters.CloudsSize = MinCloudSize + (MaxCloudSize - MinCloudSize) * cloudsLinearProgress;
        mParameters.CloudDarkening = (cloudsLinearProgress < 0.5f) ? 0.65f : (cloudsLinearProgress < 0.9f ? 0.56f : 0.4f);

        // Ambient darkening
        float ambientDarkeningSmoothProgress = SmoothStep(AmbientDarkeningUpStart, AmbientDarkeningUpEnd, upProgress);
        mParameters.AmbientDarkening = 1.0f - ambientDarkeningSmoothProgress / 5.0f;

		// Rain
		if (gameParameters.DoRainWithStorm)
		{
			float rainSmoothProgress = SmoothStep(RainUpStart, RainUpEnd, upProgress);
			mParameters.RainDensity = rainSmoothProgress;
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
        mParameters.WindSpeed = windSmoothProgress * gameParameters.StormMaxWindSpeed;

        // Clouds
        // from 1.0 to 0.0
        float cloudsLinearProgress = 1.0f - Clamp((downProgress - CloudsDownStart) / (CloudsDownEnd - CloudsDownStart), 0.0f, 1.0f);
        mParameters.NumberOfClouds = static_cast<int>(MaxClouds * cloudsLinearProgress);
        mParameters.CloudsSize = MinCloudSize + (MaxCloudSize - MinCloudSize) * cloudsLinearProgress;
        mParameters.CloudDarkening = (cloudsLinearProgress < 0.5f) ? 1.0f : (cloudsLinearProgress < 0.9f ? 0.56f : 0.4f);
        
        // Ambient darkening
        float ambientDarkeningSmoothProgress = 1.0f - SmoothStep(AmbientDarkeningDownStart, AmbientDarkeningDownEnd, downProgress);
        mParameters.AmbientDarkening = 1.0f - ambientDarkeningSmoothProgress / 5.0f;

		// Rain
		if (gameParameters.DoRainWithStorm)
		{
			float rainSmoothProgress = 1.0f - SmoothStep(RainDownStart, RainDownEnd, downProgress);
			mParameters.RainDensity = rainSmoothProgress;
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
			// Check if we should do a thunder
			if (GameRandomEngine::GetInstance().GenerateUniformBoolean(mThunderCdf))
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
    // Upload ambient darkening
    renderContext.UploadStormAmbientDarkening(mParameters.AmbientDarkening);

	// Upload rain
	renderContext.UploadRain(mParameters.RainDensity);

    // TODO: lightnings
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
	// TODOHERE
	mGameEventHandler->OnLightning();
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

}