/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-10-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "RenderContext.h"

#include <GameCore/GameWallClock.h>

namespace Physics
{

class Storm
{

public:

    Storm()
        : mParameters()
        , mIsInStorm(false)
        , mCurrentStormProgress(0.0f)
        , mLastStormUpdateTimestamp(GameWallClock::GetInstance().Now())
    {}

    void Update(GameParameters const & gameParameters);

    void Upload(Render::RenderContext & renderContext) const;

public:

    struct Parameters
    {
        float WindSpeed; // Km/h, absolute (on top of current direction)
        unsigned int NumberOfClouds;
        float CloudDarkening; // [0.0f = full darkness, 1.0 = no darkening]
        float AmbientDarkening; // [0.0f = full darkness, 1.0 = no darkening]

        Parameters()
            : WindSpeed(0.0f)
            , NumberOfClouds(0)
            , CloudDarkening(1.0f)
            , AmbientDarkening(1.0f)
        {}
    };

    Parameters const & GetParameters() const
    {
        return mParameters;
    }

private:

    Parameters mParameters;

    //
    // State machine
    //

    // Flag indicating whether we are in a storm or waiting for one
    bool mIsInStorm;

    // The current progress of the storm, when in a storm: [0.0, 1.0]
    float mCurrentStormProgress;

    // The timestamp at which we last did a storm update
    GameWallClock::time_point mLastStormUpdateTimestamp;
};

}
