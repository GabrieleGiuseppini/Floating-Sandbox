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
    // Storm script:
    //  - 1: Cloud + Wind buildup + Cloud darkening goes ep
    //  - 2: Ambient light darkening goes up
    //  - TODOHERE: lightnings (+thunder)
    //  - TODOHERE: rain
    //  - TODOHERE: targeted lightnings
    //  - 0.5f
    //  - TODOHERE: targeted lightnings
    //  - TODOHERE: rain
    //  - TODOHERE: lightnings (+thunder)
    //  - 2: Ambient light darkening goes down
    //  - 1: Cloud + Wind buildup + Cloud darkening goes down

    float constexpr Phase1UpStart = 0.0f;
    float constexpr Phase2UpStart = 0.1f;
    float constexpr Phase1UpEnd = 0.125f; // 1/8    
    float constexpr Phase2UpEnd = 0.175f;

    float constexpr Phase2DownStart = 0.825f;
    float constexpr Phase1DownStart = 0.875f;
    float constexpr Phase2DownEnd = 0.9f;    
    float constexpr Phase1DownEnd = 1.0f;

    // Calculate progress of storm: 0.0f = beginning, 1.0f = end
    float progressStep =
        std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(now - mLastStormUpdateTimestamp).count()
        / std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(gameParameters.StormDuration).count();

    mCurrentStormProgress += progressStep;

    if (mCurrentStormProgress < 0.5f)
    { 
        // Up
        float upProgress = mCurrentStormProgress;

        // Phase 1
        float phase1Progress = SmoothStep(Phase1UpStart, Phase1UpEnd, upProgress);
        mParameters.WindSpeed = phase1Progress * 40.0f;
        mParameters.NumberOfClouds = static_cast<int>(40.0f * phase1Progress);
        mParameters.CloudsSize = 1.0f; // TODO
        mParameters.CloudDarkening = 1.0f - phase1Progress / 2.3f;

        // Phase 2
        float phase2Progress = SmoothStep(Phase2UpStart, Phase2UpEnd, upProgress);
        // TODOTEST: checking if it's just the cloud darkening alone that makes for bad colors
        //mParameters.AmbientDarkening = 1.0f - phase2Progress / 5.0f;

        // TODO: other phases
    }
    else
    {
        // Down
        float downProgress = 1.0f - mCurrentStormProgress;

        // Phase 1
        float phase1Progress = SmoothStep(Phase1DownStart, Phase1DownEnd, downProgress);
        mParameters.WindSpeed = phase1Progress * 40.0f;
        mParameters.NumberOfClouds = static_cast<int>(40.0f * phase1Progress);
        mParameters.CloudsSize = 1.0f; // TODO
        mParameters.CloudDarkening = 1.0f - phase1Progress / 2.3f;

        // Phase 2
        float phase2Progress = SmoothStep(Phase2DownStart, Phase2DownEnd, downProgress);
        mParameters.AmbientDarkening = 1.0f - phase2Progress / 5.0f;
    }


    //
    // See if it's time to stop the storm
    //

    if (mCurrentStormProgress >= 1.0f)
    {
        // Turn off storm
        TurnStormOff();        
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