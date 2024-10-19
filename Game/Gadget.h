/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameEventDispatcher.h"
#include "GameParameters.h"
#include "Physics.h"
#include "RenderContext.h"

#include <GameCore/GameTypes.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/Vectors.h>

#include <cassert>
#include <functional>
#include <memory>
#include <optional>

namespace Physics
{

/*
 * Base class of all gadgets. Each gadget type has a specialization that takes care
 * of its own state machine.
 */
class Gadget
{
public:

    virtual ~Gadget()
    {}

    /*
     * Returns the ID of this gadget.
     */
    GlobalGadgetId GetId() const
    {
        return mId;
    }

    /*
     * Returns the type of this gadget.
     */
    GadgetType GetType() const
    {
        return mType;
    }

    /*
     * Returns the mass of this gadget.
     */
    virtual float GetMass() const = 0;

    /*
     * Updates the gadget's state machine.
     *
     * Returns false when the gadget has "expired" and thus can be deleted.
     */
    virtual bool Update(
        GameWallClock::time_point currentWallClockTime,
        float currentSimulationTime,
        Storm::Parameters const & stormParameters,
        GameParameters const & gameParameters) = 0;

    /*
     * Checks whether the gadget is in a state that allows it to be removed.
     */
    virtual bool MayBeRemoved() const = 0;

    /*
     * Invoked when the gadget is removed from outside (not by own state machine).
     */
    virtual void OnExternallyRemoved() = 0;

    /*
     * Invoked when the neighborhood of the gadget has been disturbed.
     */
    virtual void OnNeighborhoodDisturbed() = 0;

    /*
     * Uploads rendering information to the render context.
     */
    virtual void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const = 0;

    /*
     * Invoked when the spring tracked by the gadget is destroyed.
     */
    void OnTrackedSpringDestroyed()
    {
        assert(mTrackedSpringIndex.has_value());

        // Freeze current rotation offset
        mFrozenRotationOffsetAxis =
            mShipSprings.GetEndpointBPosition(*mTrackedSpringIndex, mShipPoints)
            - mShipSprings.GetEndpointAPosition(*mTrackedSpringIndex, mShipPoints);

        // Remember we are not tracking a spring anymore
        mTrackedSpringIndex.reset();
    }

    /*
     * Gets the point that the gadget is attached to.
     */
    inline ElementIndex GetPointIndex() const
    {
        return mPointIndex;
    }

    /*
     * Gets the spring that the gadget is tracking, or none if the gadget is not
     * ttacking any springs.
     */
    inline std::optional<ElementIndex> GetTrackedSpringIndex() const
    {
        return mTrackedSpringIndex;
    }

    /*
     * Returns the position of this gadget.
     */
    inline vec2f const GetPosition() const
    {
        return mShipPoints.GetPosition(mPointIndex);
    }

protected:

    Gadget(
        GlobalGadgetId id,
        GadgetType type,
        ElementIndex pointIndex,
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        IShipPhysicsHandler & shipPhysicsHandler,
        Points & shipPoints,
        Springs & shipSprings)
        : mId(id)
        , mType(type)
        , mPointIndex(pointIndex)
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventDispatcher))
        , mShipPhysicsHandler(shipPhysicsHandler)
        , mShipPoints(shipPoints)
        , mShipSprings(shipSprings)
        //
        , mTrackedSpringIndex(GetTrackedSpringIndex(pointIndex, shipPoints))
        , mRotationBaseAxis(shipSprings.GetEndpointBPosition(*mTrackedSpringIndex, shipPoints) - shipSprings.GetEndpointAPosition(*mTrackedSpringIndex, shipPoints))
        , mFrozenRotationOffsetAxis(std::nullopt)
        , mPersonalitySeed(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal())
    {
    }

    static inline ElementIndex GetTrackedSpringIndex(
        ElementIndex pointIndex,
        Points const & shipPoints)
    {
        assert(!shipPoints.GetConnectedSprings(pointIndex).ConnectedSprings.empty());
        return shipPoints.GetConnectedSprings(pointIndex).ConnectedSprings[0].SpringIndex;
    }

    /*
     * Returns the base rotation axis.
     */
    inline vec2f const & GetRotationBaseAxis() const
    {
        return mRotationBaseAxis;
    }

    /*
     * Returns the rotation axis of this gadget.
     */
    inline vec2f const GetRotationOffsetAxis() const
    {
        if (mFrozenRotationOffsetAxis.has_value())
        {
            return *mFrozenRotationOffsetAxis;
        }
        else
        {
            assert(mTrackedSpringIndex.has_value());
            return mShipSprings.GetEndpointBPosition(*mTrackedSpringIndex, mShipPoints)
                - mShipSprings.GetEndpointAPosition(*mTrackedSpringIndex, mShipPoints);
        }
    }

    /*
     * Returns the plane ID of this gadget.
     */
    inline PlaneId GetPlaneId() const
    {
        return mShipPoints.GetPlaneId(mPointIndex);
    }

    /*
     * Returns the personality seed of this gadget, i.e.
     * a uniform normalized random value.
     */
    inline float GetPersonalitySeed() const
    {
        return mPersonalitySeed;
    }

protected:

    // Our ID
    GlobalGadgetId const mId;

    // The type of this gadget
    GadgetType const mType;

    // The index of the particle that we're attached to
    ElementIndex const mPointIndex;

    // Our parent world
    World & mParentWorld;

    // The game event handler
    std::shared_ptr<GameEventDispatcher> mGameEventHandler;

    // The handler to invoke for acting on the ship
    IShipPhysicsHandler & mShipPhysicsHandler;

    // The container of all the ship's points
    Points & mShipPoints;

    // The container of all the ship's springs
    Springs & mShipSprings;

private:

    // The index of the spring that we're tracking, or none
    // when the gadget has stopped tracking a spring
    std::optional<ElementIndex> mTrackedSpringIndex;

    // The basis orientation axis
    vec2f const mRotationBaseAxis;

    // The last rotation axis of the spring tracked by this gadget,
    // if the gadget has stopped tracking a spring; otherwise, none
    std::optional<vec2f> mFrozenRotationOffsetAxis;

    // The random personality seed
    float const mPersonalitySeed;
};

}
