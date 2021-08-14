/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-11-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "RenderContext.h"

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
        GameParameters const & gameParameters);

    void Upload(Render::RenderContext & renderContext) const;

private:

    void RegenerateStars(unsigned int numberOfStars);

    struct MovingStarState;
    bool UpdateMovingStarStateMachine(MovingStarState & state);

    static MovingStarState MakeMovingStarState();

    static float MakeNextMovingStarInterval();

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
        Star MovingStar;
        vec2f Direction;
        float Speed;
        float Brightness;

        MovingStarState(
            vec2f const & startPosition,
            float brightness,
            vec2f const & direction,
            float speed)
            : MovingStar(startPosition, brightness)
            , Direction(direction)
            , Speed(speed)
        {}
    };

    std::optional<MovingStarState> mCurrentMovingStarState;
    float mNextMovingStarSimulationTime;
};

}
