/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2019-12-03
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/

#include <GameCore/GameRandomEngine.h>
#include <GameCore/GameTypes.h>

namespace Physics {

struct Ship::ExplosionStateMachine : public Ship::StateMachine
{
public:

    float const StartSimulationTime;
    PlaneId const Plane;
    vec2f const CenterPosition;
    float const Strength;
    float const PersonalitySeed;

    float CurrentProgress;
    bool IsFirstFrame;

    ExplosionStateMachine(
        float startSimulationTime,
        PlaneId planeId,
        vec2f const & centerPosition,
        float strength)
        : StateMachine(StateMachineType::Explosion)
        , StartSimulationTime(startSimulationTime)
        , Plane(planeId)
        , CenterPosition(centerPosition)
        , Strength(strength)
        , PersonalitySeed(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal())
        , CurrentProgress(0.0f)
        , IsFirstFrame(true)
    {}
};

}