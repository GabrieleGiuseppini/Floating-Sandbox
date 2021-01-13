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
    GadgetId GetId() const
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
     * Invoked when the gadget is removed by the user.
     */
    virtual void OnRemoved() = 0;

    /*
     * Invoked when the neighborhood of the spring has been disturbed;
     * includes the spring that the gadget is attached to.
     */
    virtual void OnNeighborhoodDisturbed() = 0;

    /*
     * Uploads rendering information to the render context.
     */
    virtual void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const = 0;

    /*
     * If the gadget is attached, saves its current position and detaches itself from the Springs container;
     * otherwise, it's a nop.
     */
    void DetachIfAttached()
    {
        if (!!mSpringIndex)
        {
            // Detach gadget

            assert(mShipSprings.IsGadgetAttached(*mSpringIndex));

            mShipSprings.DetachGadget(
                *mSpringIndex,
                mShipPoints);

            // Freeze current midpoint position, rotation offset, and plane ID

            mFrozenMidpointPosition = mShipSprings.GetMidpointPosition(*mSpringIndex, mShipPoints);

            mFrozenRotationOffsetAxis = mShipSprings.GetEndpointBPosition(*mSpringIndex, mShipPoints)
                - mShipSprings.GetEndpointAPosition(*mSpringIndex, mShipPoints);

            mFrozenPlaneId = mShipSprings.GetPlaneId(*mSpringIndex, mShipPoints);

            // Remember we don't have a spring index anymore

            mSpringIndex.reset();
        }
        else
        {
            assert(!!mFrozenMidpointPosition);
            assert(!!mFrozenRotationOffsetAxis);
            assert(!!mFrozenPlaneId);
        }
    }

    /*
     * Gets the spring that the gadget is attached to, or none if the gadget is not
     * attached to any springs.
     */
    std::optional<ElementIndex> GetAttachedSpringIndex() const
    {
        return mSpringIndex;
    }

    /*
     * Returns the midpoint position of the spring to which this gadget is attached.
     */
    vec2f const GetPosition() const
    {
        if (!!mFrozenMidpointPosition)
        {
            return *mFrozenMidpointPosition;
        }
        else
        {
            assert(!!mSpringIndex);
            return mShipSprings.GetMidpointPosition(*mSpringIndex, mShipPoints);
        }
    }

    /*
     * Returns the rotation axis of the spring to which this gadget is attached.
     */
    vec2f const GetRotationOffsetAxis() const
    {
        if (!!mFrozenRotationOffsetAxis)
        {
            return *mFrozenRotationOffsetAxis;
        }
        else
        {
            assert(!!mSpringIndex);
            return mShipSprings.GetEndpointBPosition(*mSpringIndex, mShipPoints)
                - mShipSprings.GetEndpointAPosition(*mSpringIndex, mShipPoints);
        }
    }

    /*
     * Returns the ID of the plane of this gadget.
     */
    PlaneId GetPlaneId() const
    {
        if (!!mFrozenPlaneId)
        {
            return *mFrozenPlaneId;
        }
        else
        {
            assert(!!mSpringIndex);
            return mShipSprings.GetPlaneId(*mSpringIndex, mShipPoints);
        }
    }

    /*
     * Returns the personality seed of this gadget, i.e.
     * a uniform normalized random value.
     */
    float GetPersonalitySeed() const
    {
        return mPersonalitySeed;
    }

protected:

    Gadget(
        GadgetId id,
        GadgetType type,
        ElementIndex springIndex,
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        IShipPhysicsHandler & shipPhysicsHandler,
        Points & shipPoints,
        Springs & shipSprings)
        : mId(id)
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventDispatcher))
        , mShipPhysicsHandler(shipPhysicsHandler)
        , mShipPoints(shipPoints)
        , mShipSprings(shipSprings)
        , mRotationBaseAxis(shipSprings.GetEndpointBPosition(springIndex, shipPoints) - shipSprings.GetEndpointAPosition(springIndex, shipPoints))
        , mType(type)
        , mSpringIndex(springIndex)
        , mFrozenMidpointPosition(std::nullopt)
        , mFrozenRotationOffsetAxis(std::nullopt)
        , mFrozenPlaneId(std::nullopt)
        , mPersonalitySeed(GameRandomEngine::GetInstance().GenerateNormalizedUniformReal())
    {
    }

    // Our ID
    GadgetId const mId;

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

    // The basis orientation axis
    vec2f const mRotationBaseAxis;

private:

    // The type of this gadget
    GadgetType const mType;

    // The index of the spring that we're attached to, or none
    // when the gadget has been detached
    std::optional<ElementIndex> mSpringIndex;

    // The position of the midpoint of the spring of this gadget, if the gadget has been detached from its spring;
    // otherwise, none
    std::optional<vec2f> mFrozenMidpointPosition;

    // The last rotation axis of the spring of this gadget, if the gadget has been detached from its spring;
    // otherwise, none
    std::optional<vec2f> mFrozenRotationOffsetAxis;

    // The plane ID of this gadget, if the gadget has been detached from its spring; otherwise, none
    std::optional<PlaneId> mFrozenPlaneId;

    // The random personality seed
    float const mPersonalitySeed;
};

}
