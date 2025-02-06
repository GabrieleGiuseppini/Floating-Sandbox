/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <Core/GameRandomEngine.h>

#include <cassert>
#include <limits>

namespace Physics {

Stars::Stars()
    : mStars()
    , mStarCountDirtyForRendering()
    // Moving stars state machine
    , mCurrentMovingStarState()
    , mNextMovingStarSimulationTime(std::numeric_limits<float>::max()) // Will be recalculated
{
}

void Stars::Update(
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    //
    // 1. See if we have to update the number of stars
    //

    if (mStars.size() != simulationParameters.NumberOfStars)
    {
        RegenerateStars(simulationParameters.NumberOfStars);

        // Clear state machine
        mCurrentMovingStarState.reset();

        if (!mStars.empty())
        {
            // Re-schedule next state machine
            mNextMovingStarSimulationTime = currentSimulationTime + CalculateNextMovingStarInterval();
        }
        else
        {
            // Schedule state machine for NEVER
            mNextMovingStarSimulationTime = std::numeric_limits<float>::max();
        }
    }

    //
    // 2. Update moving stars state machine
    //

    if (mCurrentMovingStarState.has_value())
    {
        assert(mStars.size() > 0);

        // Update current state machine
        if (!UpdateMovingStarStateMachine(
            *mCurrentMovingStarState,
            currentSimulationTime,
            mStars[0]))
        {
            // Done
            mCurrentMovingStarState.reset();

            // Schedule next state machine
            mNextMovingStarSimulationTime = currentSimulationTime + CalculateNextMovingStarInterval();
        }
    }
    else if (currentSimulationTime > mNextMovingStarSimulationTime)
    {
        //
        // Kick off the state machine
        //

        mCurrentMovingStarState = MakeMovingStarStateMachine(currentSimulationTime);
    }
}

void Stars::Upload(RenderContext & renderContext) const
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

Stars::MovingStarState Stars::MakeMovingStarStateMachine(float currentSimulationTime)
{
    if (GameRandomEngine::GetInstance().GenerateUniformBoolean(0.5f))
    {
        //
        // Satellite
        //
        // - From left to right in a straight line, in the upper portion of the sky

        return MovingStarState(
            MovingStarState::MovingStarType::Satellite,
            vec2f(-1.0f, GameRandomEngine::GetInstance().GenerateUniformReal(0.0f, 0.98f)),
            vec2f(0.05f, 0.0f),
            GameRandomEngine::GetInstance().GenerateUniformReal(0.45f, 1.0f), // Brightness - constant
            currentSimulationTime);
    }
    else
    {
        //
        // Shooting star
        //
        // - From top to bottom, converging towards the center of the screen
        //

        float const startX = GameRandomEngine::GetInstance().GenerateUniformReal(-1.0f, 1.0f);
        float const endX = GameRandomEngine::GetInstance().GenerateUniformReal(-0.15f, 0.15f);
        vec2f const direction = vec2f(endX - startX, -1.0f).normalise();

        return MovingStarState(
            MovingStarState::MovingStarType::ShootingStar,
            vec2f(startX, 1.0f),
            direction * 0.8f,
            GameRandomEngine::GetInstance().GenerateUniformReal(0.9f, 1.0f), // Brightness - max
            currentSimulationTime);
    }
}

bool Stars::UpdateMovingStarStateMachine(
    MovingStarState & state,
    float currentSimulationTime,
    Star & movingStar)
{
    bool doContinue = true;

    switch (state.Type)
    {
        case MovingStarState::MovingStarType::Satellite:
        {
            vec2f const newPosition =
                state.StartPosition
                + state.Velocity * (currentSimulationTime - state.StartSimulationTime);

            movingStar.PositionNdc = newPosition;
            movingStar.Brightness = state.Brightness;

            if (newPosition.x > 1.0f)
            {
                doContinue = false;
            }

            break;
        }

        case MovingStarState::MovingStarType::ShootingStar:
        {
            vec2f const newPosition =
                state.StartPosition
                + state.Velocity * (currentSimulationTime - state.StartSimulationTime);

            // Brightness parabole
            float constexpr MidY = 0.5f;
            float constexpr a = 1.0f / (MidY * (MidY - 1.0f));
            float constexpr b = -a;
            float const brightnessCoeff = a * newPosition.y * newPosition.y + b * newPosition.y;

            movingStar.PositionNdc = newPosition;
            movingStar.Brightness = state.Brightness * brightnessCoeff;

            if (newPosition.y < 0.0f)
            {
                doContinue = false;
            }

            break;
        }
    }

    if (!doContinue)
    {
        // Park the star
        movingStar.PositionNdc = vec2f(-1.0f, -1.0f);
        movingStar.Brightness = 0.0f;
    }

    // Remember to refresh the moving star
    assert(mStars.size() > 0);
    mStarCountDirtyForRendering = std::max(
        size_t(1),
        mStarCountDirtyForRendering.value_or(0));

    return doContinue;
}

float Stars::CalculateNextMovingStarInterval()
{
    float constexpr RateSeconds = 10.0f;

    return GameRandomEngine::GetInstance().GenerateExponentialReal(1.0f / RateSeconds);
}

}