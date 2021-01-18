/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Physics.h"

namespace Physics {

void Gadgets::Update(
    GameWallClock::time_point currentWallClockTime,
    float currentSimulationTime,
    Storm::Parameters const & stormParameters,
    GameParameters const & gameParameters)
{
    // Run through all gadgets and invoke Update() on each;
    // remove those gadgets that have expired

    for (auto it = mCurrentGadgets.begin(); it != mCurrentGadgets.end(); /* incremented in loop */)
    {
        bool const isActive = (*it)->Update(
            currentWallClockTime,
            currentSimulationTime,
            stormParameters,
            gameParameters);

        if (!isActive)
        {
            //
            // Gadget has expired
            //

            // Gadget has detached itself already
            assert(!(*it)->GetAttachedSpringIndex());

            // Notify (soundless) removal
            mGameEventHandler->OnGadgetRemoved(
                (*it)->GetId(),
                (*it)->GetType(),
                std::nullopt);

            // Remove it from the container
            it = mCurrentGadgets.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!!mCurrentPhysicsProbeGadget)
    {
        bool const isActive = mCurrentPhysicsProbeGadget->Update(
            currentWallClockTime,
            currentSimulationTime,
            stormParameters,
            gameParameters);

        assert(isActive); // Guy never expires
        (void)isActive;
    }
}

void Gadgets::OnPointDetached(ElementIndex pointElementIndex)
{
    auto squareNeighborhoodRadius = NeighborhoodRadius * NeighborhoodRadius;

    auto neighborhoodCenter = mShipPoints.GetPosition(pointElementIndex);

    for (auto & gadget : mCurrentGadgets)
    {
        // Check if the gadget is within the neighborhood of the disturbed point
        float squareGadgetDistance = (gadget->GetPosition() - neighborhoodCenter).squareLength();
        if (squareGadgetDistance < squareNeighborhoodRadius)
        {
            // Tel the gadget that its neighborhood has been disturbed
            gadget->OnNeighborhoodDisturbed();
        }
    }
}

void Gadgets::OnSpringDestroyed(ElementIndex springElementIndex)
{
    auto squareNeighborhoodRadius = NeighborhoodRadius * NeighborhoodRadius;

    auto neighborhoodCenter = mShipSprings.GetMidpointPosition(springElementIndex, mShipPoints);

    for (auto & gadget : mCurrentGadgets)
    {
        // Check if the gadget is attached to this spring
        auto gadgetSpring = gadget->GetAttachedSpringIndex();
        if (!!gadgetSpring && *gadgetSpring == springElementIndex)
        {
            // Detach gadget
            gadget->DetachIfAttached();
        }

        // Check if the gadget is within the neighborhood of the disturbed center
        float squareGadgetDistance = (gadget->GetPosition() - neighborhoodCenter).squareLength();
        if (squareGadgetDistance < squareNeighborhoodRadius)
        {
            // Tel the gadget that its neighborhood has been disturbed
            gadget->OnNeighborhoodDisturbed();
        }
    }
}

std::optional<bool> Gadgets::TogglePhysicsProbeAt(
    vec2f const & targetPos,
    GameParameters const & gameParameters)
{
    float const squareSearchRadius = gameParameters.ToolSearchRadius * gameParameters.ToolSearchRadius;

    if (!!mCurrentPhysicsProbeGadget)
    {
        //
        // We already have a physics probe...
        // ...see if it's in radius, and if so, remove it
        //

        if ((mCurrentPhysicsProbeGadget->GetPosition() - targetPos).squareLength() < squareSearchRadius)
        {
            assert(mCurrentPhysicsProbeGadget->MayBeRemoved());

            // Tell it we're removing it
            mCurrentPhysicsProbeGadget->OnRemoved();

            // Remove it
            mCurrentPhysicsProbeGadget.reset();

            // We've removed a physics probe gadget
            return false;
        }
        else
        {
            // We have a probe but it's far from here...
            // ...can't do anything
            // TODO: wanna remove it in all cases?
            return std::nullopt;
        }
    }


    //
    // No physics probe in ship...
    // ...find closest spring with no gadgets attached within the search radius, and
    // if found, attach probe to it
    //

    ElementIndex nearestCandidateSpringIndex = NoneElementIndex;
    float nearestCandidateSpringDistance = std::numeric_limits<float>::max();

    for (auto springIndex : mShipSprings)
    {
        if (!mShipSprings.IsDeleted(springIndex) && !mShipSprings.IsGadgetAttached(springIndex))
        {
            float const squareDistance = (mShipSprings.GetMidpointPosition(springIndex, mShipPoints) - targetPos).squareLength();
            if (squareDistance < squareSearchRadius)
            {
                // This spring is within the search radius

                // Keep the nearest
                if (squareDistance < squareSearchRadius && squareDistance < nearestCandidateSpringDistance)
                {
                    nearestCandidateSpringIndex = springIndex;
                    nearestCandidateSpringDistance = squareDistance;
                }
            }
        }
    }

    if (NoneElementIndex != nearestCandidateSpringIndex)
    {
        // We have a nearest candidate spring

        // Create gadget
        assert(!mCurrentPhysicsProbeGadget);
        mCurrentPhysicsProbeGadget = std::make_unique<PhysicsProbeGadget>(
            GadgetId(mShipId, mNextLocalGadgetId++),
            nearestCandidateSpringIndex,
            mParentWorld,
            mGameEventHandler,
            mShipPhysicsHandler,
            mShipPoints,
            mShipSprings);

        // Attach gadget to the spring
        mShipSprings.AttachGadget(
            nearestCandidateSpringIndex,
            mCurrentPhysicsProbeGadget->GetMass(),
            mShipPoints);

        // Notify
        mGameEventHandler->OnGadgetPlaced(
            mCurrentPhysicsProbeGadget->GetId(),
            mCurrentPhysicsProbeGadget->GetType(),
            mParentWorld.IsUnderwater(
                mCurrentPhysicsProbeGadget->GetPosition()));

        // We've placed a physic probe gadget
        return true;
    }

    // Can't do anything
    return std::nullopt;
}

void Gadgets::RemovePhysicsProbe()
{
    if (!!mCurrentPhysicsProbeGadget)
    {
        assert(mCurrentPhysicsProbeGadget->MayBeRemoved());

        // Tell it we're removing it
        mCurrentPhysicsProbeGadget->OnRemoved();

        // Remove it
        mCurrentPhysicsProbeGadget.reset();
    }
}

void Gadgets::DetonateRCBombs()
{
    for (auto & gadget : mCurrentGadgets)
    {
        if (GadgetType::RCBomb == gadget->GetType())
        {
            RCBombGadget * rcb = dynamic_cast<RCBombGadget *>(gadget.get());
            rcb->Detonate();
        }
    }
}

void Gadgets::DetonateAntiMatterBombs()
{
    for (auto & gadget : mCurrentGadgets)
    {
        if (GadgetType::AntiMatterBomb == gadget->GetType())
        {
            AntiMatterBombGadget * amb = dynamic_cast<AntiMatterBombGadget *>(gadget.get());
            amb->Detonate();
        }
    }
}

void Gadgets::Upload(
    ShipId shipId,
    Render::RenderContext & renderContext) const
{
    for (auto & gadget : mCurrentGadgets)
    {
        gadget->Upload(shipId, renderContext);
    }

    if (mCurrentPhysicsProbeGadget)
    {
        mCurrentPhysicsProbeGadget->Upload(shipId, renderContext);
    }
}

}