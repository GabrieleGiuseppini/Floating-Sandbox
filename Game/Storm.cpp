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
    float constexpr AmbientDarkeningUpStart = 0.09f;    
    float constexpr CloudsUpEnd = 0.1f;
    float constexpr WindUpEnd = 0.12f;
    float constexpr AmbientDarkeningUpEnd = 0.125f;
        
    float constexpr CloudsDownStart = 0.8f;
    float constexpr CloudsDownEnd = 0.88f;
    float constexpr WindDownStart = 0.88f;
    float constexpr AmbientDarkeningDownStart = 0.9f;    
    float constexpr AmbientDarkeningDownEnd = 0.97f;
    float constexpr WindDownEnd = 1.0f;


    float constexpr MaxClouds = 50.0f;
    float constexpr MinCloudSize = 1.85f;
    float constexpr MaxCloudSize = 2.35f;


    // Calculate progress of storm: 0.0f = beginning, 1.0f = end
    float progressStep =
        std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(now - mLastStormUpdateTimestamp).count()
        / std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(gameParameters.StormDuration).count();

    mCurrentStormProgress += progressStep;

    if (mCurrentStormProgress < 0.5f)
    { 
        // Up - from 0.0  to 0.5
        float upProgress = mCurrentStormProgress;

        // Wind
        float windSmoothProgress = SmoothStep(WindUpStart, WindUpEnd, upProgress);
        mParameters.WindSpeed = windSmoothProgress * 40.0f;

        // Clouds
        float cloudsLinearProgress = Clamp((upProgress - CloudsUpStart) / (CloudsUpEnd - CloudsUpStart), 0.0f, 1.0f);
        mParameters.NumberOfClouds = static_cast<int>(MaxClouds * cloudsLinearProgress);
        mParameters.CloudsSize = MinCloudSize + (MaxCloudSize - MinCloudSize) * cloudsLinearProgress;
        mParameters.CloudDarkening = (cloudsLinearProgress < 0.5f) ? 0.65f : (cloudsLinearProgress < 0.9f ? 0.56f : 0.4f);

        // Ambient darkening
        float ambientDarkeningSmoothProgress = SmoothStep(AmbientDarkeningUpStart, AmbientDarkeningUpEnd, upProgress);
        mParameters.AmbientDarkening = 1.0f - ambientDarkeningSmoothProgress / 5.0f;

        // TODO: other phases
    }
    else
    {
        // Down - from 0.5 to 1.0
        float downProgress = mCurrentStormProgress;

        // Wind
        float windSmoothProgress = 1.0f - SmoothStep(WindDownStart, WindDownEnd, downProgress);
        mParameters.WindSpeed = windSmoothProgress * 40.0f;

        // Clouds
        // from 1.0 to 0.0
        float cloudsLinearProgress = 1.0f - Clamp((downProgress - CloudsDownStart) / (CloudsDownEnd - CloudsDownStart), 0.0f, 1.0f);
        mParameters.NumberOfClouds = static_cast<int>(MaxClouds * cloudsLinearProgress);
        mParameters.CloudsSize = MinCloudSize + (MaxCloudSize - MinCloudSize) * cloudsLinearProgress;
        mParameters.CloudDarkening = (cloudsLinearProgress < 0.5f) ? 1.0f : (cloudsLinearProgress < 0.9f ? 0.56f : 0.4f);
        
        // Ambient darkening
        float ambientDarkeningSmoothProgress = 1.0f - SmoothStep(AmbientDarkeningDownStart, AmbientDarkeningDownEnd, downProgress);
        mParameters.AmbientDarkening = 1.0f - ambientDarkeningSmoothProgress / 5.0f;

        // TODO: other phases
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

    // TODO: lightnings

    // TODO: rain
}

void Storm::TriggerStorm()
{
    if (!mIsInStorm)
    {
        // Turn on storm
        TurnStormOn(GameWallClock::GetInstance().Now());
    }
}

void Storm::TurnStormOn(GameWallClock::time_point now)
{
    mIsInStorm = true;
    mCurrentStormProgress = 0.0f;
    mLastStormUpdateTimestamp = now;
}

void Storm::TurnStormOff()
{
    mIsInStorm = false;
}

}