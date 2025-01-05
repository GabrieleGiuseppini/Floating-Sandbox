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
    float const BlastForceMagnitude; // N
    float const BlastForceRadius; // m
    float const BlastHeat; // KJ/s
    float const BlastHeatRadius; // m
    float const RenderRadiusOffset; // On top of blast force radius
    ExplosionType const Type;
    float const PersonalitySeed;

    bool IsFirstIteration; // Independent from progress, as progress might start at Dt already
    float CurrentRenderProgress;

    ExplosionStateMachine(
        float startSimulationTime,
        PlaneId planeId,
        vec2f const & centerPosition,
        float blastForceMagnitude,
        float blastForceRadius,
        float blastHeat,
        float blastHeatRadius,
        float renderRadiusOffset, // On top of blastForceRadius
        ExplosionType type)
        : StateMachine(StateMachineType::Explosion)
        , StartSimulationTime(startSimulationTime)
        , Plane(planeId)
        , CenterPosition(centerPosition)
        , BlastForceMagnitude(blastForceMagnitude)
        , BlastForceRadius(blastForceRadius)
        , BlastHeat(blastHeat)
        , BlastHeatRadius(blastHeatRadius)
        , RenderRadiusOffset(renderRadiusOffset)
        , Type(type)
        , PersonalitySeed(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal())
        , IsFirstIteration(true)
        , CurrentRenderProgress(0.0f)
    {}
};

}