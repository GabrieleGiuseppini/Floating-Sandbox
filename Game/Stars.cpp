/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

#include <cassert>

namespace Physics {

Stars::Stars()
    : mStars()
    , mStarCountDirtyForRendering()
    // Moving stars state machine
    , mCurrentMovingStarState()
    , mNextMovingStarSimulationTime(0.0f + MakeNextMovingStarInterval())
{
    LogMessage("NEXT: ", mNextMovingStarSimulationTime);
}

void Stars::Update(
    float currentSimulationTime,
    GameParameters const & gameParameters)
{
    //
    // 1. See if we have to update the number of stars
    //

    if (mStars.size() != gameParameters.NumberOfStars)
    {
        RegenerateStars(gameParameters.NumberOfStars);
    }

    //
    // 2. Update moving stars state machine
    //

    if (mCurrentMovingStarState.has_value())
    {
        // Update current state machine
        if (!UpdateMovingStarStateMachine(*mCurrentMovingStarState))
        {
            LogMessage("END");

            // Done
            mCurrentMovingStarState.reset();

            // Schedule next state machine
            mNextMovingStarSimulationTime = currentSimulationTime + MakeNextMovingStarInterval();

            LogMessage("NEXT: ", mNextMovingStarSimulationTime);
        }
    }
    else
    {
        //
        // See if it's time to kick off the state machine
        //

        if (currentSimulationTime > mNextMovingStarSimulationTime)
        {
            // Start state machine
            mCurrentMovingStarState = MakeMovingStarState();

            LogMessage("START: ", mCurrentMovingStarState->Speed);
        }
    }
}

void Stars::Upload(Render::RenderContext & renderContext) const
{
    if (mStarCountDirtyForRendering.has_value())
    {
        assert(*mStarCountDirtyForRendering <= mStars.size());

        renderContext.UploadStarsStart(*mStarCountDirtyForRendering, mStars.size());

        for (size_t s = 0; s < *mStarCountDirtyForRendering; ++s)
        {
            auto const & star = mStars[s];
            renderContext.UploadStar(s, star.PositionNdc, star.Brightness);
        }

        renderContext.UploadStarsEnd();

        mStarCountDirtyForRendering.reset();
    }
}

//////////////////////////////////////////////////////////////////////////////

void Stars::RegenerateStars(unsigned int numberOfStars)
{
    mStars.clear();
    mStars.reserve(numberOfStars);

    if (numberOfStars > 0)
    {
        // Park first star here, which will be the moving star
        mStars.emplace_back(vec2f(-1.0f, -1.0f), 0.0f);

        // Do other stars
        for (unsigned int s = 1; s < numberOfStars; ++s)
        {
            mStars.emplace_back(
                vec2f(
                    GameRandomEngine::GetInstance().GenerateUniformReal(-1.0f, +1.0f),
                    GameRandomEngine::GetInstance().GenerateUniformReal(-1.0f, +1.0f)),
                GameRandomEngine::GetInstance().GenerateUniformReal(0.25f, +1.0f));
        }
    }

    mStarCountDirtyForRendering = numberOfStars;
}

bool Stars::UpdateMovingStarStateMachine(MovingStarState & state)
{
    auto & star = state.MovingStar;

    star.PositionNdc += state.Direction * state.Speed * GameParameters::SimulationStepTimeDuration<float>;

    // See if has left the screen
    if (star.PositionNdc.x < 0.0f
        || star.PositionNdc.x > 1.0f
        || star.PositionNdc.y < 0.0f
        || star.PositionNdc.y > 1.0f)
    {
        // Done
        return false;
    }

    if (!mStars.empty())
    {
        mStars[0] = star;

        mStarCountDirtyForRendering = std::max(
            size_t(1),
            mStarCountDirtyForRendering.value_or(0));
    }

    return true;
}

Stars::MovingStarState Stars::MakeMovingStarState()
{
    float const speed = GameRandomEngine::GetInstance().GenerateUniformBoolean(0.5f)
        ? 0.05f // Satellite
        : 0.9f; // Shooting star

    return MovingStarState(
        vec2f::zero(),  // TODOTEST
        GameRandomEngine::GetInstance().GenerateUniformReal(0.35f, +1.0f), // Brightness
        vec2f(1.0f, 0.0f),  // TODOTEST
        speed);
}

float Stars::MakeNextMovingStarInterval()
{
    // TODOTEST: GameParameters
    float const rateSeconds = 2.0f;

    return GameRandomEngine::GetInstance().GenerateExponentialReal(1.0f / rateSeconds);
}

}