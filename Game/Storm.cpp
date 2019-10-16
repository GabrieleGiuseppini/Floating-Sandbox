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

        // Turn on storm
        mIsInStorm = true;
        mCurrentStormProgress = 0.0f;
        mLastStormUpdateTimestamp = now;
    }


    //
    // Update storm step
    //

    float progressStep =
        std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(now - mLastStormUpdateTimestamp).count()
        / std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(gameParameters.StormDuration).count();

    mCurrentStormProgress += progressStep;

    // TODOTEST
    float TODO = sinf(Pi<float> * mCurrentStormProgress);
    mParameters.WindSpeed = TODO * 40.0f;
    mParameters.CloudDarkening = 1.0f - TODO / 2.2f;
    mParameters.AmbientDarkening = 1.0f - TODO / 5.0f;


    //
    // See if it's time to stop the storm
    //

    if (mCurrentStormProgress >= 1.0f)
    {
        // Turn off storm
        mIsInStorm = false;
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
}

}