/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../SimulationParameters.h"

#include <Render/RenderContext.h>

#include <memory>
#include <optional>

namespace Physics
{

class Stars
{
public:

    Stars();

    void Update(
        float currentSimulationTime,
        SimulationParameters const & simulationParameters);

    void Upload(RenderContext & renderContext) const;

private:

    void RegenerateStars(unsigned int numberOfStars);

    struct Star;
    struct MovingStarState;

    MovingStarState MakeMovingStarStateMachine(float currentSimulationTime);

    bool UpdateMovingStarStateMachine(
        MovingStarState & state,
        float currentSimulationTime,
        Star & movingStar);

    static float CalculateNextMovingStarInterval();

private:

    struct Star
    {
        vec2f PositionNdc;
        float Brightness;

        Star(
            vec2f const & positionNdc,
            float brightness)
            : PositionNdc(positionNdc)
            , Brightness(brightness)
        {}
    };

    std::vector<Star> mStars;

    mutable std::optional<size_t> mStarCountDirtyForRendering;

    //
    // Moving stars state machine
    //

    struct MovingStarState
    {
        enum class MovingStarType
        {
            Satellite,
            ShootingStar
        };

        MovingStarType Type;
        vec2f StartPosition;
        vec2f Velocity;
        float Brightness;
        float StartSimulationTime;

        MovingStarState(
            MovingStarType type,
            vec2f const & startPosition,
            vec2f const & velocity,
            float brightness,
            float startSimulationTime)
            : Type(type)
            , StartPosition(startPosition)
            , Velocity(velocity)
            , Brightness(brightness)
            , StartSimulationTime(startSimulationTime)
        {}
    };

    std::optional<MovingStarState> mCurrentMovingStarState;
    float mNextMovingStarSimulationTime;
};

}
