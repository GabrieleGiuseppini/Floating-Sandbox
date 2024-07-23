/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

#include <GameCore/GameRandomEngine.h>

namespace Physics {

PhysicsProbeGadget::PhysicsProbeGadget(
    GlobalGadgetId id,
    ElementIndex pointIndex,
    World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    IShipPhysicsHandler & shipPhysicsHandler,
    Points & shipPoints,
    Springs & shipSprings)
    : Gadget(
        id,
        GadgetType::PhysicsProbe,
        pointIndex,
        parentWorld,
        std::move(gameEventDispatcher),
        shipPhysicsHandler,
        shipPoints,
        shipSprings)
    , mState(State::PingOff)
    , mNextStateTransitionTimePoint(GameWallClock::GetInstance().Now() + PingOffInterval)
{
}

bool PhysicsProbeGadget::Update(
    GameWallClock::time_point currentWallClockTime,
    float /*currentSimulationTime*/,
    Storm::Parameters const & /*stormParameters*/,
    GameParameters const & /*gameParameters*/)
{
    switch (mState)
    {
        case State::PingOff:
        {
            if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                //
                // Transition to PingOn state
                //

                mState = State::PingOn;

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + PingOnInterval;

                // Emit reading
                mGameEventHandler->OnPhysicsProbeReading(
                    mShipPoints.GetVelocity(mPointIndex),
                    mShipPoints.GetTemperature(mPointIndex),
                    mParentWorld.GetOceanSurface().GetDepth(mShipPoints.GetPosition(mPointIndex)),
                    mShipPoints.GetInternalPressure(mPointIndex));
            }

            return true;
        }

        case State::PingOn:
        {
            if (currentWallClockTime > mNextStateTransitionTimePoint)
            {
                //
                // Transition to PingOff state
                //

                mState = State::PingOff;

                // Schedule next transition
                mNextStateTransitionTimePoint = currentWallClockTime + PingOffInterval;
            }

            return true;
        }
    }

    assert(false);
    return true;
}

void PhysicsProbeGadget::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    auto & shipRenderContext = renderContext.GetShipRenderContext(shipId);

    switch (mState)
    {
        case State::PingOff:
        {
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::PhysicsProbe, 0),
                GetPosition(),
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

        case State::PingOn:
        {
            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::PhysicsProbe, 0),
                GetPosition(),
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            shipRenderContext.UploadGenericMipMappedTextureRenderSpecification(
                GetPlaneId(),
                TextureFrameId(Render::GenericMipMappedTextureGroups::PhysicsProbePing, 0),
                GetPosition(),
                1.0,
                GetRotationBaseAxis(),
                GetRotationOffsetAxis(),
                1.0f);

            break;
        }

    }
}

}