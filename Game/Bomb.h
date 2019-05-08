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
 * Base class of all bombs. Each bomb type has a specialization that takes care
 * of its own state machine.
 */
class Bomb
{
public:

    /*
     * Interface required by bombs for acting on the physical world.
     */
    struct IPhysicsHandler
    {
        virtual void DoBombExplosion(
            vec2f const & blastPosition,
            float sequenceProgress,
            GameParameters const & gameParameters) = 0;

        virtual void DoAntiMatterBombPreimplosion(
            vec2f const & centerPosition,
            float sequenceProgress,
            GameParameters const & gameParameters) = 0;

        virtual void DoAntiMatterBombImplosion(
            vec2f const & centerPosition,
            float sequenceProgress,
            GameParameters const & gameParameters) = 0;

        virtual void DoAntiMatterBombExplosion(
            vec2f const & centerPosition,
            float sequenceProgress,
            GameParameters const & gameParameters) = 0;
    };

public:

    /*
     * Updates the bomb's state machine.
     *
     * Returns false when the bomb has "expired" and thus can be deleted.
     */
    virtual bool Update(
        GameWallClock::time_point currentWallClockTime,
        GameParameters const & gameParameters) = 0;

    /*
     * Checks whether the bomb is in a state that allows it to be removed.
     */
    virtual bool MayBeRemoved() const = 0;

    /*
     * Invoked when the bomb is removed by the user.
     */
    virtual void OnBombRemoved() = 0;

    /*
     * Invoked when the neighborhood of the spring has been disturbed;
     * includes the spring that the bomb is attached to.
     */
    virtual void OnNeighborhoodDisturbed() = 0;

    /*
     * Uploads rendering information to the render context.
     */
    virtual void Upload(
        ShipId shipId,
        Render::RenderContext & renderContext) const = 0;

    /*
     * If the bomb is attached, saves its current position and detaches itself from the Springs container;
     * otherwise, it's a nop.
     */
    void DetachIfAttached()
    {
        if (!!mSpringIndex)
        {
            // Detach bomb

            assert(mShipSprings.IsBombAttached(*mSpringIndex));

            mShipSprings.DetachBomb(
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
     * Returns the ID of this bomb.
     */
    BombId GetId() const
    {
        return mId;
    }

    /*
     * Returns the type of this bomb.
     */
    BombType GetType() const
    {
        return mType;
    }

    /*
     * Gets the spring that the bomb is attached to, or none if the bomb is not
     * attached to any springs.
     */
    std::optional<ElementIndex> GetAttachedSpringIndex() const
    {
        return mSpringIndex;
    }

    /*
     * Returns the midpoint position of the spring to which this bomb is attached.
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
     * Returns the rotation axis of the spring to which this bomb is attached.
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
     * Returns the ID of the plane of this bomb.
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

protected:

    Bomb(
        BombId id,
        BombType type,
        ElementIndex springIndex,
        World & parentWorld,
        std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
        IPhysicsHandler & physicsHandler,
        Points & shipPoints,
        Springs & shipSprings)
        : mId(id)
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventDispatcher))
        , mPhysicsHandler(physicsHandler)
        , mShipPoints(shipPoints)
        , mShipSprings(shipSprings)
        , mRotationBaseAxis(shipSprings.GetEndpointBPosition(springIndex, shipPoints) - shipSprings.GetEndpointAPosition(springIndex, shipPoints))
        , mType(type)
        , mSpringIndex(springIndex)
        , mFrozenMidpointPosition(std::nullopt)
        , mFrozenRotationOffsetAxis(std::nullopt)
        , mFrozenPlaneId(std::nullopt)
    {
    }

    // Our ID
    BombId const mId;

    // Our parent world
    World & mParentWorld;

    // The game event handler
    std::shared_ptr<GameEventDispatcher> mGameEventHandler;

    // The handler to invoke for acting on the world
    IPhysicsHandler & mPhysicsHandler;

    // The container of all the ship's points
    Points & mShipPoints;

    // The container of all the ship's springs
    Springs & mShipSprings;

    // The basis orientation axis
    vec2f const mRotationBaseAxis;

private:

    // The type of this bomb
    BombType const mType;

    // The index of the spring that we're attached to, or none
    // when the bomb has been detached
    std::optional<ElementIndex> mSpringIndex;

    // The position of the midpoint of the spring of this bomb, if the bomb has been detached from its spring;
    // otherwise, none
    std::optional<vec2f> mFrozenMidpointPosition;

    // The last rotation axis of the spring of this bomb, if the bomb has been detached from its spring;
    // otherwise, none
    std::optional<vec2f> mFrozenRotationOffsetAxis;

    // The plane ID of this bomb, if the bomb has been detached from its spring; otherwise, none
    std::optional<PlaneId> mFrozenPlaneId;
};

}
