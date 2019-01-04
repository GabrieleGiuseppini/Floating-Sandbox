/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "IGameEventHandler.h"
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
            ConnectedComponentId connectedComponentId,
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

            // Freeze current midpoint position, rotation offset, and connected component

            mMidpointPosition = mShipSprings.GetMidpointPosition(*mSpringIndex, mShipPoints);

            mRotationOffsetAxis = mShipSprings.GetPointBPosition(*mSpringIndex, mShipPoints)
                - mShipSprings.GetPointAPosition(*mSpringIndex, mShipPoints);

            assert(
                mShipPoints.GetConnectedComponentId(mShipSprings.GetPointAIndex(*mSpringIndex))
                == mShipPoints.GetConnectedComponentId(mShipSprings.GetPointBIndex(*mSpringIndex)));

            mConnectedComponentId = mShipPoints.GetConnectedComponentId(mShipSprings.GetPointAIndex(*mSpringIndex));

            // Remember we don't have a spring index anymore

            mSpringIndex.reset();
        }
        else
        {
            assert(!!mMidpointPosition);
            assert(!!mRotationOffsetAxis);
            assert(!!mConnectedComponentId);
        }
    }

    /*
     * Returns the ID of this bomb.
     */
    ObjectId GetId() const
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
        if (!!mMidpointPosition)
        {
            return *mMidpointPosition;
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
        if (!!mRotationOffsetAxis)
        {
            return *mRotationOffsetAxis;
        }
        else
        {
            assert(!!mSpringIndex);
            return mShipSprings.GetPointBPosition(*mSpringIndex, mShipPoints)
                - mShipSprings.GetPointAPosition(*mSpringIndex, mShipPoints);
        }
    }

    /*
     * Returns the ID of the connected component of this bomb.
     */
    ConnectedComponentId GetConnectedComponentId() const
    {
        if (!!mConnectedComponentId)
        {
            return *mConnectedComponentId;
        }
        else
        {
            assert(!!mSpringIndex);
            assert(mShipPoints.GetConnectedComponentId(mShipSprings.GetPointAIndex(*mSpringIndex))
                == mShipPoints.GetConnectedComponentId(mShipSprings.GetPointBIndex(*mSpringIndex)));
            return mShipPoints.GetConnectedComponentId(mShipSprings.GetPointAIndex(*mSpringIndex));
        }
    }

protected:

    Bomb(
        ObjectId id,
        BombType type,
        ElementIndex springIndex,
        World & parentWorld,
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        IPhysicsHandler & physicsHandler,
        Points & shipPoints,
        Springs & shipSprings)
        : mId(id)
        , mParentWorld(parentWorld)
        , mGameEventHandler(std::move(gameEventHandler))
        , mPhysicsHandler(physicsHandler)
        , mShipPoints(shipPoints)
        , mShipSprings(shipSprings)
        , mRotationBaseAxis(shipPoints.GetPosition(shipSprings.GetPointBIndex(springIndex)) - shipPoints.GetPosition(shipSprings.GetPointAIndex(springIndex)))
        , mType(type)
        , mSpringIndex(springIndex)
        , mMidpointPosition(std::nullopt)
        , mRotationOffsetAxis(std::nullopt)
        , mConnectedComponentId(std::nullopt)
    {
    }

    // Our ID
    ObjectId const mId;

    // Our parent world
    World & mParentWorld;

    // The game event handler
    std::shared_ptr<IGameEventHandler> mGameEventHandler;

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
    std::optional<vec2f> mMidpointPosition;

    // The last rotation axis of the spring of this bomb, if the bomb has been detached from its spring;
    // otherwise, none
    std::optional<vec2f> mRotationOffsetAxis;

    // The connected component ID of this bomb, if the bomb has been detached from its spring; otherwise, none
    std::optional<ConnectedComponentId> mConnectedComponentId;
};

}
