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
    float const BlastRadius; // m
    float const BlastForce; // N
    float const BlastHeat; // KJ
    float const RenderRadiusOffset; // On top of blast radius
    ExplosionType const Type;
    float const PersonalitySeed;

    bool IsFirstIteration; // Independent from progress, as progress might start at Dt already
    float CurrentRenderProgress;

    ExplosionStateMachine(
        float startSimulationTime,
        PlaneId planeId,
        vec2f const & centerPosition,
        float blastRadius,
        float blastForce,
        float blastHeat,
        float renderRadiusOffset,
        ExplosionType type)
        : StateMachine(StateMachineType::Explosion)
        , StartSimulationTime(startSimulationTime)
        , Plane(planeId)
        , CenterPosition(centerPosition)
        , BlastRadius(blastRadius)
        , BlastForce(blastForce)
        , BlastHeat(blastHeat)
        , RenderRadiusOffset(renderRadiusOffset)
        , Type(type)
        , PersonalitySeed(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal())
        , IsFirstIteration(true)
        , CurrentRenderProgress(0.0f)
    {}
};

}